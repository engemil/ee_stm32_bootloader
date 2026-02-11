#!/usr/bin/env python3
"""
Sign Application Binary with Size and CRC32

This script:
1. Reads the application binary
2. Calculates the firmware size (from vector table to end)
3. Calculates CRC32 of the firmware (from vector table to end)
4. Signs the header with the size and CRC32 at offset 8 and 12
5. Writes the signed binary

Memory layout (ARM Cortex-M0+ requires 256-byte aligned vector table):
  0x00000000: Application header (32 bytes)
  0x00000020: Padding for alignment (224 bytes)
  0x00000100: Vector table + code (firmware data)

Application header structure (32 bytes at offset 0):
  Offset 0:  magic (0xDEADBEEF) - 4 bytes
  Offset 4:  version            - 4 bytes
  Offset 8:  size               - 4 bytes (SIGNED - size from 0x100 to end)
  Offset 12: crc32              - 4 bytes (SIGNED - CRC from 0x100 to end)
  Offset 16: reserved[4]        - 16 bytes

IMPORTANT: CRC is calculated over firmware starting at offset 0x100 (vector table),
NOT from offset 0x20 (after header). The padding bytes are NOT included in CRC.
"""

import sys
import struct
import zlib

HEADER_SIZE = 32  # bytes
VECTOR_TABLE_OFFSET = 0x100  # 256 bytes - ARM Cortex-M0+ alignment requirement
MAGIC = 0xDEADBEEF

def calculate_crc32(data):
    """Calculate CRC32 using same algorithm as bootloader"""
    return zlib.crc32(data) & 0xFFFFFFFF

def sign_binary(input_file, output_file):
    """Sign binary with size and CRC32"""
    
    # Read input binary
    try:
        with open(input_file, 'rb') as f:
            binary_data = f.read()
    except FileNotFoundError:
        print(f"Error: Input file '{input_file}' not found")
        return 1
    
    # Check minimum size
    if len(binary_data) < HEADER_SIZE:
        print(f"Error: Binary too small ({len(binary_data)} bytes)")
        return 1
    
    # Verify magic number
    magic = struct.unpack('<I', binary_data[0:4])[0]
    if magic != MAGIC:
        print(f"Error: Invalid magic number 0x{magic:08X} (expected 0xDEADBEEF)")
        return 1
    
    # Get version
    version = struct.unpack('<I', binary_data[4:8])[0]
    
    # Calculate firmware size (from vector table offset to end)
    # This excludes both the header (32 bytes) and padding (224 bytes)
    firmware_size = len(binary_data) - VECTOR_TABLE_OFFSET
    
    # Extract firmware data (from vector table offset to end)
    # This is what the bootloader will validate with CRC
    firmware_data = binary_data[VECTOR_TABLE_OFFSET:]
    
    # Calculate CRC32 (same as bootloader does from 0x08004100 onwards)
    crc32 = calculate_crc32(firmware_data)
    
    # Create signed header
    signed_header = bytearray(binary_data[:HEADER_SIZE])
    struct.pack_into('<I', signed_header, 8, firmware_size)   # Sign size
    struct.pack_into('<I', signed_header, 12, crc32)          # Sign CRC32
    
    # Create output binary (header + padding + firmware)
    # We need to preserve the padding between header and vector table
    padding = binary_data[HEADER_SIZE:VECTOR_TABLE_OFFSET]
    output_data = bytes(signed_header) + padding + firmware_data
    
    # Write output file
    try:
        with open(output_file, 'wb') as f:
            f.write(output_data)
    except IOError as e:
        print(f"Error: Failed to write output file: {e}")
        return 1
    
    # Print summary
    print("=" * 60)
    print("Application Binary Signing Complete")
    print("=" * 60)
    print(f"Input file:       {input_file}")
    print(f"Output file:      {output_file}")
    print(f"Total size:       {len(output_data)} bytes ({len(output_data)/1024:.2f} KB)")
    print(f"Header size:      {HEADER_SIZE} bytes")
    print(f"Padding size:     {VECTOR_TABLE_OFFSET - HEADER_SIZE} bytes")
    print(f"Firmware size:    {firmware_size} bytes ({firmware_size/1024:.2f} KB)")
    print(f"Magic:            0x{magic:08X}")
    print(f"Version:          0x{version:08X} ({version >> 16}.{(version >> 8) & 0xFF}.{version & 0xFF})")
    print(f"CRC32:            0x{crc32:08X}")
    print("=" * 60)
    
    return 0

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 sign_app_header.py <input.bin> <output.bin>")
        print("")
        print("This script signs the application header with:")
        print("  - Firmware size (bytes from offset 0x100 to end)")
        print("  - CRC32 checksum (of firmware from offset 0x100 to end)")
        print("")
        print("Note: CRC excludes header (32 bytes) and padding (224 bytes).")
        print("The signed binary can then be uploaded via DFU.")
        return 1
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    return sign_binary(input_file, output_file)

if __name__ == '__main__':
    sys.exit(main())
