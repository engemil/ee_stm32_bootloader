#ifndef USB_DFU_H
#define USB_DFU_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief DFU state machine states (USB DFU 1.1 specification)
 */
typedef enum {
    DFU_STATE_DFU_IDLE             = 2,
    DFU_STATE_DFU_DNLOAD_SYNC      = 3,
    DFU_STATE_DFU_DNBUSY           = 4,
    DFU_STATE_DFU_DNLOAD_IDLE      = 5,
    DFU_STATE_DFU_MANIFEST_SYNC    = 6,
    DFU_STATE_DFU_MANIFEST         = 7,
    DFU_STATE_DFU_ERROR            = 10
} dfu_state_t;

/**
 * @brief DFU status codes
 */
typedef enum {
    DFU_STATUS_OK                  = 0x00,
    DFU_STATUS_ERR_TARGET          = 0x01,
    DFU_STATUS_ERR_FILE            = 0x02,
    DFU_STATUS_ERR_WRITE           = 0x03,
    DFU_STATUS_ERR_ERASE           = 0x04,
    DFU_STATUS_ERR_CHECK_ERASED    = 0x05,
    DFU_STATUS_ERR_PROG            = 0x06,
    DFU_STATUS_ERR_VERIFY          = 0x07,
    DFU_STATUS_ERR_ADDRESS         = 0x08,
    DFU_STATUS_ERR_NOTDONE         = 0x09,
    DFU_STATUS_ERR_FIRMWARE        = 0x0A,
    DFU_STATUS_ERR_VENDOR          = 0x0B,
    DFU_STATUS_ERR_USBR            = 0x0C,
    DFU_STATUS_ERR_POR             = 0x0D,
    DFU_STATUS_ERR_UNKNOWN         = 0x0E,
    DFU_STATUS_ERR_STALLEDPKT      = 0x0F
} dfu_status_t;

/**
 * @brief DFU request codes
 */
typedef enum {
    DFU_REQ_DETACH      = 0,
    DFU_REQ_DNLOAD      = 1,
    DFU_REQ_UPLOAD      = 2,
    DFU_REQ_GETSTATUS   = 3,
    DFU_REQ_CLRSTATUS   = 4,
    DFU_REQ_GETSTATE    = 5,
    DFU_REQ_ABORT       = 6
} dfu_request_t;

/**
 * @brief DFU download block size (must align with flash page)
 */
#define DFU_XFER_SIZE   1024

/**
 * @brief DFUSe special commands (used when wValue == 0)
 */
#define DFUSE_CMD_SET_ADDRESS   0x21  /* Set address pointer (5 bytes) */
#define DFUSE_CMD_ERASE         0x41  /* Erase page at address (5 bytes) */
#define DFUSE_CMD_READ_UNPROTECT 0x92 /* Read unprotect (1 byte) */

/**
 * @brief Initialize USB DFU
 * 
 * Configures HSI48 clock, initializes USB peripheral, and sets up DFU mode.
 * 
 * @return 0 on success, negative error code on failure
 */
int usb_dfu_init(void);

/**
 * @brief Process USB DFU events
 * 
 * This function should be called in the main loop to handle USB interrupts
 * and DFU protocol state machine.
 */
void usb_dfu_process(void);

/**
 * @brief Get current DFU state
 * 
 * @return Current DFU state
 */
dfu_state_t usb_dfu_get_state(void);

/**
 * @brief Check if DFU download is complete
 * 
 * @return true if download completed, false otherwise
 */
bool usb_dfu_download_complete(void);

#endif /* USB_DFU_H */
