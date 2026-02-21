/*
MIT License

Copyright (c) 2026 EngEmil

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
 * @file usb_dfu.c
 * @brief USB DFU (Device Firmware Update) implementation using ChibiOS
 * 
 * This implements USB DFU 1.1 protocol for EngEmil STM32 Bootloader.
 * Uses ChibiOS USB stack for reliable USB communication.
 */

#include "ch.h"
#include "hal.h"
#include "usb_dfu.h"
#include "config.h"
#include "flash_ops.h"
#include "bootloader.h"
#include "stm32c071xx.h"
#include <string.h>

/*===========================================================================*/
/* USB DFU Class-Specific Descriptors                                       */
/*===========================================================================*/

/* DFU Functional Descriptor */
#define DFU_DESC_FUNCTIONAL_SIZE    9

/* DFU Attributes */
#define DFU_ATTR_CAN_DOWNLOAD       0x01
#define DFU_ATTR_CAN_UPLOAD         0x02
#define DFU_ATTR_WILL_DETACH        0x08

/* DFU Class-Specific Requests (bmRequestType = 0x21 for Class/Interface/Host-to-Device) */
#define USB_RTYPE_DFU_REQUEST       (USB_RTYPE_DIR_HOST2DEV | \
                                     USB_RTYPE_TYPE_CLASS | \
                                     USB_RTYPE_RECIPIENT_INTERFACE)

/*===========================================================================*/
/* DFU Context                                                               */
/*===========================================================================*/

static struct {
    dfu_state_t state;
    dfu_status_t status;
    uint32_t current_address;
    uint32_t target_address;       /* For DFUSe Set Address command (0x21) */
    uint16_t block_num;
    uint8_t buffer[DFU_XFER_SIZE] __attribute__((aligned(4)));  /* 4-byte aligned for flash writes */
    uint16_t buffer_len;
    bool download_complete;
    bool erase_done;                /* Track if explicit erase was performed */
    uint32_t poll_timeout;  /* Time in milliseconds for flash operation */
} dfu_ctx;

/*===========================================================================*/
/* USB Descriptors                                                           */
/*===========================================================================*/

/**
 * @brief Device Descriptor (mutable for dynamic VID/PID)
 * 
 * VID and PID are read from the application header if a valid application
 * is present (magic == 0xDEADBEEF). Otherwise, defaults from config.h are used.
 * 
 * VID/PID are at byte offsets 8-9 (VID) and 10-11 (PID) in little-endian format.
 */
static uint8_t vcom_device_descriptor_data[18] = {
    USB_DESC_DEVICE(0x0200,        /* bcdUSB (2.0)                    */
                    0x00,           /* bDeviceClass (per interface)    */
                    0x00,           /* bDeviceSubClass                 */
                    0x00,           /* bDeviceProtocol                 */
                    USB_PACKET_SIZE, /* bMaxPacketSize                 */
                    USB_DEFAULT_VID, /* idVendor (patched at runtime)  */
                    USB_DEFAULT_PID, /* idProduct (patched at runtime) */
                    0x0100,         /* bcdDevice                       */
                    1,              /* iManufacturer                   */
                    2,              /* iProduct                        */
                    3,              /* iSerialNumber                   */
                    1)              /* bNumConfigurations              */
};

/**
 * @brief Device Descriptor wrapper
 */
static USBDescriptor vcom_device_descriptor = {
    sizeof vcom_device_descriptor_data,
    vcom_device_descriptor_data
};

/**
 * @brief Get USB VID/PID from application header or use defaults
 * 
 * If a valid application is present (magic == APP_HEADER_MAGIC), the VID/PID
 * from the application header are returned. Otherwise, the default values
 * from config.h are used.
 * 
 * @param[out] vid  Pointer to store USB Vendor ID
 * @param[out] pid  Pointer to store USB Product ID
 */
static void get_usb_vid_pid(uint16_t *vid, uint16_t *pid) {
    const app_header_t *header = (const app_header_t *)APP_BASE;
    
    if (header->magic == APP_HEADER_MAGIC) {
        /* Valid application header - use its VID/PID */
        *vid = header->usb_vid;
        *pid = header->usb_pid;
    } else {
        /* No valid application - use defaults */
        *vid = USB_DEFAULT_VID;
        *pid = USB_DEFAULT_PID;
    }
}

/**
 * @brief Configuration Descriptor with DFU Interface
 */
static const uint8_t vcom_configuration_descriptor_data[27] = {
    /* Configuration Descriptor (9 bytes) */
    USB_DESC_CONFIGURATION(27,              /* wTotalLength (9+9+9)        */
                           1,                /* bNumInterfaces              */
                           1,                /* bConfigurationValue         */
                           0,                /* iConfiguration              */
                           0x80,             /* bmAttributes (bus powered)  */
                           50),              /* bMaxPower (100mA)           */
    
    /* Interface Descriptor (9 bytes) */
    USB_DESC_INTERFACE(0,                   /* bInterfaceNumber            */
                       0,                   /* bAlternateSetting           */
                       0,                   /* bNumEndpoints (DFU uses EP0)*/
                       0xFE,                /* bInterfaceClass (App Spec)  */
                       0x01,                /* bInterfaceSubClass (DFU)    */
                       0x02,                /* bInterfaceProtocol (DFU)    */
                       4),                  /* iInterface                  */
    
    /* DFU Functional Descriptor (9 bytes) */
    USB_DESC_BYTE(DFU_DESC_FUNCTIONAL_SIZE),        /* bLength             */
    USB_DESC_BYTE(0x21),                            /* bDescriptorType (DFU)*/
    USB_DESC_BYTE(DFU_ATTR_CAN_DOWNLOAD |          /* bmAttributes         */
                  DFU_ATTR_WILL_DETACH),
    USB_DESC_WORD(255),                             /* wDetachTimeout (ms)  */
    USB_DESC_WORD(DFU_XFER_SIZE),                   /* wTransferSize        */
    USB_DESC_BCD(0x011A)                            /* bcdDFUVersion (1.1a) */
};

/**
 * @brief Configuration Descriptor wrapper
 */
static const USBDescriptor vcom_configuration_descriptor = {
    sizeof vcom_configuration_descriptor_data,
    vcom_configuration_descriptor_data
};

/**
 * @brief String Descriptors
 */
static const uint8_t vcom_string0[] = {
    USB_DESC_BYTE(4),                       /* bLength                      */
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING),   /* bDescriptorType              */
    USB_DESC_WORD(0x0409)                   /* wLANGID (US English)         */
};

static const uint8_t vcom_string1[] = {
    USB_DESC_BYTE(22),                      /* bLength (2 + 10*2)           */
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING),   /* bDescriptorType              */
    'E', 0, 'n', 0, 'g', 0, 'E', 0, 'm', 0, 'i', 0, 'l', 0, '.', 0,
    'i', 0, 'o', 0
};

static const uint8_t vcom_string2[] = {
    USB_DESC_BYTE(40),                      /* bLength (2 + 19*2)           */
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING),   /* bDescriptorType              */
    'B', 0, 'o', 0, 'o', 0, 't', 0, 'l', 0, 'o', 0, 'a', 0, 'd', 0,
    'e', 0, 'r', 0, ' ', 0, 'D', 0, 'F', 0, 'U', 0, ' ', 0, 'M', 0,
    'o', 0, 'd', 0, 'e', 0
};

static const uint8_t vcom_string3[] = {
    USB_DESC_BYTE(26),                      /* bLength                      */
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING),   /* bDescriptorType              */
    '0', 0, '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0,
    '8', 0, '9', 0, 'A', 0, 'B', 0
};

/* DFUSe interface string descriptor */
/* Format: @Internal Flash  /0x08004000/112*001Kg */
static const uint8_t vcom_string4[] = {
    USB_DESC_BYTE(78),                      /* bLength (2 + 38*2)           */
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING),   /* bDescriptorType              */
    '@', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'n', 0, 'a', 0,
    'l', 0, ' ', 0, 'F', 0, 'l', 0, 'a', 0, 's', 0, 'h', 0, ' ', 0,
    ' ', 0, '/', 0, '0', 0, 'x', 0, '0', 0, '8', 0, '0', 0, '0', 0,
    '4', 0, '0', 0, '0', 0, '0', 0, '/', 0, '1', 0, '1', 0, '2', 0,
    '*', 0, '0', 0, '0', 0, '1', 0, 'K', 0, 'g', 0
};

/**
 * @brief String Descriptors array
 */
static const USBDescriptor vcom_strings[] = {
    {sizeof vcom_string0, vcom_string0},
    {sizeof vcom_string1, vcom_string1},
    {sizeof vcom_string2, vcom_string2},
    {sizeof vcom_string3, vcom_string3},
    {sizeof vcom_string4, vcom_string4}
};

/*===========================================================================*/
/* USB Driver Configuration                                                  */
/*===========================================================================*/

/**
 * @brief Get descriptor callback
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
                                           uint8_t dtype,
                                           uint8_t dindex,
                                           uint16_t lang) {
    (void)usbp;
    (void)lang;

    switch (dtype) {
    case USB_DESCRIPTOR_DEVICE:
        return &vcom_device_descriptor;
    case USB_DESCRIPTOR_CONFIGURATION:
        return &vcom_configuration_descriptor;
    case USB_DESCRIPTOR_STRING:
        if (dindex < 5)
            return &vcom_strings[dindex];
        break;
    }
    return NULL;
}

/**
 * @brief DFU Request Handler Forward Declaration
 */
static bool dfu_request_hook(USBDriver *usbp);

/**
 * @brief USB Event Callback
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {
    (void)usbp;

    switch (event) {
    case USB_EVENT_RESET:
        /* Reset DFU state on USB reset */
        dfu_ctx.state = DFU_STATE_DFU_IDLE;
        dfu_ctx.status = DFU_STATUS_OK;
        return;
    case USB_EVENT_ADDRESS:
        return;
    case USB_EVENT_CONFIGURED:
        /* USB configured - ready for DFU */
        return;
    case USB_EVENT_SUSPEND:
        return;
    case USB_EVENT_WAKEUP:
        return;
    case USB_EVENT_STALLED:
        return;
    case USB_EVENT_UNCONFIGURED:
        return;
    }
}

/**
 * @brief USB Driver Configuration
 */
static const USBConfig usbcfg = {
    usb_event,
    get_descriptor,
    dfu_request_hook,
    NULL
};

/*===========================================================================*/
/* DFU Protocol Implementation                                               */
/*===========================================================================*/

/**
 * @brief Process DFU_DNLOAD request
 * 
 * Handles both DFUSe special commands (wValue=0) and regular data blocks (wValue>=2).
 * DFUSe commands: 0x21 (Set Address), 0x41 (Erase)
 */
static void dfu_dnload_handler(USBDriver *usbp, uint16_t wValue, uint16_t wLength) {
    /* Check valid states */
    if (dfu_ctx.state != DFU_STATE_DFU_IDLE &&
        dfu_ctx.state != DFU_STATE_DFU_DNLOAD_IDLE) {
        dfu_ctx.status = DFU_STATUS_ERR_STALLEDPKT;
        dfu_ctx.state = DFU_STATE_DFU_ERROR;
        usbStallReceiveI(usbp, 0);
        return;
    }

    /* Zero-length packet = download complete */
    if (wLength == 0) {
        dfu_ctx.state = DFU_STATE_DFU_MANIFEST_SYNC;
        dfu_ctx.download_complete = true;
        usbSetupTransfer(usbp, NULL, 0, NULL);
        return;
    }

    /* Check transfer size */
    if (wLength > DFU_XFER_SIZE) {
        dfu_ctx.status = DFU_STATUS_ERR_STALLEDPKT;
        dfu_ctx.state = DFU_STATE_DFU_ERROR;
        usbStallReceiveI(usbp, 0);
        return;
    }

    /* DFUSe special commands: wValue == 0 */
    if (wValue == 0) {
        /* Mark as special command for usb_dfu_process() */
        dfu_ctx.block_num = 0xFFFF;  /* Sentinel value for special command */
        dfu_ctx.buffer_len = wLength;
        dfu_ctx.state = DFU_STATE_DFU_DNLOAD_SYNC;
        usbSetupTransfer(usbp, dfu_ctx.buffer, wLength, NULL);
        return;
    }

    /* Regular data block: wValue >= 2 */
    dfu_ctx.block_num = wValue;
    dfu_ctx.buffer_len = wLength;
    dfu_ctx.state = DFU_STATE_DFU_DNLOAD_SYNC;
    usbSetupTransfer(usbp, dfu_ctx.buffer, wLength, NULL);
}

/**
 * @brief Process DFU_GETSTATUS request
 */
static void dfu_getstatus_handler(USBDriver *usbp) {
    static uint8_t status_response[6];

    /* Transition state machine based on current state */
    if (dfu_ctx.state == DFU_STATE_DFU_DNLOAD_SYNC) {
        /* Flash operation in progress - transition to DNBUSY */
        
        /* Set poll timeout based on operation type */
        if (dfu_ctx.block_num == 0xFFFF) {
            /* Special command - longer timeout for erase operations */
            dfu_ctx.poll_timeout = 2000;  /* 2 seconds for full app erase */
        } else {
            /* Regular data block write - short timeout */
            dfu_ctx.poll_timeout = 10;  /* 10ms flash write */
        }
        
        dfu_ctx.state = DFU_STATE_DFU_DNBUSY;
    } else if (dfu_ctx.state == DFU_STATE_DFU_DNBUSY) {
        /* Only transition out of DNBUSY if buffer is empty (work completed) */
        if (dfu_ctx.buffer_len == 0) {
            /* Flash operation complete */
            if (dfu_ctx.status == DFU_STATUS_OK) {
                dfu_ctx.state = DFU_STATE_DFU_DNLOAD_IDLE;
            } else {
                dfu_ctx.state = DFU_STATE_DFU_ERROR;
            }
        }
        /* else: still busy, stay in DNBUSY state */
    } else if (dfu_ctx.state == DFU_STATE_DFU_MANIFEST_SYNC) {
        dfu_ctx.state = DFU_STATE_DFU_MANIFEST;
        dfu_ctx.poll_timeout = 0;
    }

    status_response[0] = (uint8_t)dfu_ctx.status;        /* bStatus */
    status_response[1] = (uint8_t)(dfu_ctx.poll_timeout & 0xFF);  /* bwPollTimeout[0] */
    status_response[2] = (uint8_t)((dfu_ctx.poll_timeout >> 8) & 0xFF);  /* bwPollTimeout[1] */
    status_response[3] = (uint8_t)((dfu_ctx.poll_timeout >> 16) & 0xFF); /* bwPollTimeout[2] */
    status_response[4] = (uint8_t)dfu_ctx.state;         /* bState */
    status_response[5] = 0;                              /* iString */

    usbSetupTransfer(usbp, status_response, 6, NULL);
}

/**
 * @brief Process DFU_CLRSTATUS request
 */
static void dfu_clrstatus_handler(USBDriver *usbp) {
    if (dfu_ctx.state == DFU_STATE_DFU_ERROR) {
        dfu_ctx.state = DFU_STATE_DFU_IDLE;
        dfu_ctx.status = DFU_STATUS_OK;
    }
    usbSetupTransfer(usbp, NULL, 0, NULL);
}

/**
 * @brief Process DFU_GETSTATE request
 */
static void dfu_getstate_handler(USBDriver *usbp) {
    static uint8_t state_response[1];
    state_response[0] = (uint8_t)dfu_ctx.state;
    usbSetupTransfer(usbp, state_response, 1, NULL);
}

/**
 * @brief Process DFU_ABORT request
 */
static void dfu_abort_handler(USBDriver *usbp) {
    dfu_ctx.state = DFU_STATE_DFU_IDLE;
    dfu_ctx.status = DFU_STATUS_OK;
    dfu_ctx.block_num = 0;
    dfu_ctx.current_address = APP_BASE;
    dfu_ctx.target_address = APP_BASE;
    dfu_ctx.erase_done = false;
    usbSetupTransfer(usbp, NULL, 0, NULL);
}

/**
 * @brief DFU Class-Specific Request Hook
 */
static bool dfu_request_hook(USBDriver *usbp) {
    /* Handle only DFU class requests */
    if ((usbp->setup[0] & USB_RTYPE_TYPE_MASK) != USB_RTYPE_TYPE_CLASS) {
        return false;
    }
    
    /* Reset bootloader timeout on any DFU activity */
    bootloader_timeout_reset();

    uint8_t bRequest = usbp->setup[1];
    uint16_t wValue = (usbp->setup[3] << 8) | usbp->setup[2];
    uint16_t wLength = (usbp->setup[7] << 8) | usbp->setup[6];

    switch (bRequest) {
    case DFU_REQ_DNLOAD:
        dfu_dnload_handler(usbp, wValue, wLength);
        return true;

    case DFU_REQ_GETSTATUS:
        dfu_getstatus_handler(usbp);
        return true;

    case DFU_REQ_CLRSTATUS:
        dfu_clrstatus_handler(usbp);
        return true;

    case DFU_REQ_GETSTATE:
        dfu_getstate_handler(usbp);
        return true;

    case DFU_REQ_ABORT:
        dfu_abort_handler(usbp);
        return true;

    case DFU_REQ_DETACH:
        /* Detach not needed - already in DFU mode */
        usbSetupTransfer(usbp, NULL, 0, NULL);
        return true;

    default:
        return false;
    }
}

/*===========================================================================*/
/* Public API Implementation                                                 */
/*===========================================================================*/

/**
 * @brief Initialize USB DFU
 */
int usb_dfu_init(void) {
    /* Initialize DFU context */
    dfu_ctx.state = DFU_STATE_DFU_IDLE;
    dfu_ctx.status = DFU_STATUS_OK;
    dfu_ctx.current_address = APP_BASE;
    dfu_ctx.target_address = APP_BASE;
    dfu_ctx.block_num = 0;
    dfu_ctx.buffer_len = 0;
    dfu_ctx.download_complete = false;
    dfu_ctx.erase_done = false;
    dfu_ctx.poll_timeout = 0;

    /* Get VID/PID from application header (or use defaults) */
    uint16_t vid, pid;
    get_usb_vid_pid(&vid, &pid);
    
    /* Patch device descriptor with VID/PID (little-endian at offsets 8-11) */
    vcom_device_descriptor_data[8]  = (uint8_t)(vid & 0xFF);
    vcom_device_descriptor_data[9]  = (uint8_t)((vid >> 8) & 0xFF);
    vcom_device_descriptor_data[10] = (uint8_t)(pid & 0xFF);
    vcom_device_descriptor_data[11] = (uint8_t)((pid >> 8) & 0xFF);

    /* Initialize USB driver */
    usbDisconnectBus(&USBD1);
    chThdSleepMilliseconds(100);
    usbStart(&USBD1, &usbcfg);
    usbConnectBus(&USBD1);

    return ERR_SUCCESS;
}

/**
 * @brief Process USB DFU events
 * 
 * This should be called periodically in main loop to:
 * - Process DFUSe special commands (0x21 Set Address, 0x41 Erase)
 * - Write buffered firmware data to flash
 */
void usb_dfu_process(void) {
    /* Reset timeout on flash operations (activity is happening) */
    
    /* Check if we have data to process */
    if (dfu_ctx.state == DFU_STATE_DFU_DNBUSY && dfu_ctx.buffer_len > 0) {
        
        /* Reset timeout when processing data */
        bootloader_timeout_reset();
        
        /* Handle DFUSe special commands (block_num == 0xFFFF) */
        if (dfu_ctx.block_num == 0xFFFF) {
            uint8_t cmd = dfu_ctx.buffer[0];
            
            /* Parse command */
            switch (cmd) {
            case DFUSE_CMD_SET_ADDRESS:  /* 0x21 - Set Address Pointer */
                if (dfu_ctx.buffer_len == 5) {
                    /* Extract 32-bit address (little-endian) */
                    dfu_ctx.target_address = (dfu_ctx.buffer[1] << 0)  |
                                              (dfu_ctx.buffer[2] << 8)  |
                                              (dfu_ctx.buffer[3] << 16) |
                                              (dfu_ctx.buffer[4] << 24);
                    
                    /* Validate address is in application region */
                    if (dfu_ctx.target_address < APP_BASE || 
                        dfu_ctx.target_address >= (APP_BASE + APP_MAX_SIZE)) {
                        dfu_ctx.status = DFU_STATUS_ERR_ADDRESS;
                        dfu_ctx.state = DFU_STATE_DFU_ERROR;
                        return;
                    }
                    
                    dfu_ctx.current_address = dfu_ctx.target_address;
                    dfu_ctx.status = DFU_STATUS_OK;
                } else {
                    /* Invalid command length */
                    dfu_ctx.status = DFU_STATUS_ERR_STALLEDPKT;
                    dfu_ctx.state = DFU_STATE_DFU_ERROR;
                    return;
                }
                break;
                
            case DFUSE_CMD_ERASE:  /* 0x41 - Erase Page */
                if (dfu_ctx.buffer_len == 5) {
                    /* Extract address (for validation, we erase entire app region) */
                    uint32_t erase_addr = (dfu_ctx.buffer[1] << 0)  |
                                           (dfu_ctx.buffer[2] << 8)  |
                                           (dfu_ctx.buffer[3] << 16) |
                                           (dfu_ctx.buffer[4] << 24);
                    
                    /* Validate address is in application region */
                    if (erase_addr < APP_BASE || 
                        erase_addr >= (APP_BASE + APP_MAX_SIZE)) {
                        dfu_ctx.status = DFU_STATUS_ERR_ADDRESS;
                        dfu_ctx.state = DFU_STATE_DFU_ERROR;
                        return;
                    }
                    
                    /* Unlock flash */
                    if (flash_unlock() != ERR_SUCCESS) {
                        dfu_ctx.status = DFU_STATUS_ERR_PROG;
                        dfu_ctx.state = DFU_STATE_DFU_ERROR;
                        return;
                    }
                    
                    /* Erase entire application region (112KB) */
                    if (flash_erase_pages(APP_BASE, APP_MAX_SIZE) != ERR_SUCCESS) {
                        flash_lock();
                        dfu_ctx.status = DFU_STATUS_ERR_ERASE;
                        dfu_ctx.state = DFU_STATE_DFU_ERROR;
                        return;
                    }
                    
                    /* Clear ALL flash status flags after erase */
                    FLASH->SR = FLASH_SR_WRPERR | FLASH_SR_PROGERR | FLASH_SR_EOP;
                    
                    /* LOCK flash after erase (will unlock again for write) */
                    flash_lock();
                    
                    dfu_ctx.erase_done = true;
                    dfu_ctx.current_address = APP_BASE;  /* Reset address for sequential writes */
                    dfu_ctx.status = DFU_STATUS_OK;
                } else {
                    dfu_ctx.status = DFU_STATUS_ERR_STALLEDPKT;
                    dfu_ctx.state = DFU_STATE_DFU_ERROR;
                    return;
                }
                break;
                
            default:
                /* Unknown command */
                dfu_ctx.status = DFU_STATUS_ERR_STALLEDPKT;
                dfu_ctx.state = DFU_STATE_DFU_ERROR;
                return;
            }
            
            /* Command processed, clear buffer */
            dfu_ctx.buffer_len = 0;
            return;
        }
        
        /* Handle regular data blocks */
        
        /* Auto-erase fallback on first data block (block 2) if no explicit erase */
        if (!dfu_ctx.erase_done && dfu_ctx.block_num == 2) {
            /* Block 0-1 reserved for DFUSe commands, data starts at block 2 */
            if (flash_unlock() != ERR_SUCCESS) {
                dfu_ctx.status = DFU_STATUS_ERR_PROG;
                dfu_ctx.state = DFU_STATE_DFU_ERROR;
                return;
            }
            
            if (flash_erase_pages(APP_BASE, APP_MAX_SIZE) != ERR_SUCCESS) {
                flash_lock();
                dfu_ctx.status = DFU_STATUS_ERR_ERASE;
                dfu_ctx.state = DFU_STATE_DFU_ERROR;
                return;
            }
            
            flash_lock();
            dfu_ctx.erase_done = true;
            /* Initialize current_address for sequential writes */
            dfu_ctx.current_address = APP_BASE;
        }
        
        /* Use current_address for write (sequential addressing) */
        uint32_t write_addr = dfu_ctx.current_address;
        
        /* Validate address range */
        if (!flash_is_app_region(write_addr, dfu_ctx.buffer_len)) {
            dfu_ctx.status = DFU_STATUS_ERR_ADDRESS;
            dfu_ctx.state = DFU_STATE_DFU_ERROR;
            return;
        }
        
        /* Sanity check: ensure we have data to write */
        if (dfu_ctx.buffer_len == 0 || dfu_ctx.buffer_len > DFU_XFER_SIZE) {
            dfu_ctx.status = DFU_STATUS_ERR_STALLEDPKT;
            dfu_ctx.state = DFU_STATE_DFU_ERROR;
            return;
        }

        /* Unlock flash for writing (safe to call multiple times) */
        if (flash_unlock() != ERR_SUCCESS) {
            dfu_ctx.status = DFU_STATUS_ERR_PROG;
            dfu_ctx.state = DFU_STATE_DFU_ERROR;
            return;
        }
        
        /* Write firmware block to flash (no erase - already done) */
        if (flash_write(write_addr, dfu_ctx.buffer, dfu_ctx.buffer_len) != ERR_SUCCESS) {
            flash_lock();
            dfu_ctx.status = DFU_STATUS_ERR_WRITE;
            dfu_ctx.state = DFU_STATE_DFU_ERROR;
            return;
        }
        
        /* Lock flash */
        flash_lock();
        
        /* Advance address for next block */
        dfu_ctx.current_address += dfu_ctx.buffer_len;

        /* Clear buffer */
        dfu_ctx.buffer_len = 0;
        dfu_ctx.status = DFU_STATUS_OK;
    }
}

/**
 * @brief Get current DFU state
 */
dfu_state_t usb_dfu_get_state(void) {
    return dfu_ctx.state;
}

/**
 * @brief Check if DFU download is complete
 */
bool usb_dfu_download_complete(void) {
    return dfu_ctx.download_complete;
}
