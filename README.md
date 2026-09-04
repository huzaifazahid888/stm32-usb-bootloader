# STM32 USB Host Bootloader

A custom USB Host bootloader for STM32F4 that updates firmware directly from a USB flash drive.

The bootloader uses USB Host Mass Storage (MSC) and FATFS to read a firmware `.bin` file from the USB drive. It checks the firmware header, compares the firmware with the application already stored in Flash, verifies the application using CRC, and only performs a firmware update when required.

After the update or verification is complete, the bootloader jumps to the user application.

## What it does

* Detects a USB flash drive using USB Host MSC
* Reads the firmware `.bin` file using FATFS
* Reads and validates the application header
* Checks firmware magic number, version, and image size
* Compares the USB firmware with the firmware already stored in Flash
* Erases and programs Flash only when a new firmware image is detected
* Calculates and verifies the application CRC after programming
* Stores the validated application header in a reserved Flash sector
* Checks the application stack pointer before jumping to the user application
* Relocates the interrupt vector table to the application address
* Jumps to the existing application when no USB drive is detected
* Reports the bootloader status and errors over UART

## Firmware File

The firmware file contains a 16-byte application header followed by the application binary.

```text
+------------------------------+
| Magic Number   | 4 bytes     |
| Version        | 4 bytes     |
| Image Size     | 4 bytes     |
| Application CRC| 4 bytes     |
+------------------------------+
|                              |
|      Application Binary      |
|                              |
+------------------------------+
```

The header is used by the bootloader to identify and validate the firmware before programming it.

## Flash Memory Map

The bootloader uses the STM32F407 Flash as follows:

```text
0x08000000  +---------------------------+
            |                           |
            | Bootloader                |
            | 64 KB                     |
            | Sectors 0 - 3             |
0x08010000  +---------------------------+
            |                           |
            | Reserved / App Header     |
            | 64 KB                     |
            | Sector 4                  |
0x08020000  +---------------------------+
            |                           |
            | Application               |
            | 896 KB                    |
            | Sectors 5 - 11            |
            |                           |
0x08100000  +---------------------------+
```

The application starts at:

```text
0x08020000
```

The reserved sector is used to store the header of the last successfully validated application.

## Bootloader Workflow

```text
                         Power On
                            │
                            ▼
                   Initialize Hardware
                            │
                            ▼
                    Initialize USB Host
                            │
                            ▼
              Wait for USB drive for 3 seconds
                            │
                 ┌──────────┴──────────┐
                 │                     │
             USB Found             No USB
                 │                     │
                 ▼                     ▼
            Mount FATFS          Check Stored App
                 │                     │
                 ▼                     ▼
          Open Firmware File       Validate App
                 │                     │
                 ▼                     ▼
          Read Application Header   Check CRC
                 │                     │
                 ▼                     ▼
          Validate Header           Jump to App
                 │
                 ▼
       Compare USB Firmware
       with Installed Firmware
            │            │
            │            │
        Identical      Different
            │            │
            ▼            ▼
       Jump to App    Erase Flash
                         │
                         ▼
                     Program Flash
                          │
                          ▼
                     Verify CRC
                          │
                          ▼
                    Store App Header
                          │
                          ▼
                     Jump to App
```

## Application Configuration

The user application must be configured to start from the bootloader's application address.

### 1. Update the linker file

In the application's `STM32F407VGTX_FLASH.ld`:

```ld
FLASH (rx) : ORIGIN = 0x08020000, LENGTH = 896K
```

This makes the application start at `0x08020000`.

### 2. Relocate the vector table

In the application's `system_stm32f4xx.c`:

```c
#define USER_VECT_TAB_ADDRESS
#define VECT_TAB_OFFSET 0x20000U
```

This relocates the application's interrupt vector table to `0x08020000`.

Both settings must match the bootloader application start address.

## Repository Structure

The main bootloader-specific code is kept inside the `User` directory.

```text
User/
├── inc/
│   ├── bootConfig.h    # Bootloader configuration and memory definitions
│   ├── crc32.h         # CRC definitions
│   ├── flash_if.h      # Flash handling interface
│   ├── serial.h        # UART serial interface
│   └── usbBoot.h       # USB bootloader definitions
│
└── src/
    ├── main.cpp        # Main bootloader flow
    ├── usbBoot.cpp     # USB firmware update and boot logic
    ├── flash_if.c      # Flash erase and programming
    ├── crc32.cpp       # CRC calculation
    ├── serial.cpp      # UART debug output
    └── ...
```

### Main files

**`main.cpp`**

Handles the main bootloader flow, initialization, USB state handling, firmware update process, CRC checking, and application jump.

**`usbBoot.cpp`**

Contains the main bootloader logic related to firmware detection, file handling, application header handling, firmware comparison, and application startup.

**`flash_if.c`**

Handles Flash erase and programming operations.

**`crc32.cpp`**

Handles CRC calculation used for firmware validation.

**`bootConfig.h`**

Contains the main bootloader configuration such as Flash addresses, Flash sizes, application start address, reserved memory, header length, and other bootloader settings.

The memory layout can be changed from the configuration file without changing the main bootloader logic.

## Updating Firmware

1. Build the application firmware.
2. Generate the firmware file with the application header and CRC.
3. Copy the generated `.bin` file to a FAT-formatted USB flash drive.
4. Rename the firmware file to:

```text
app_crc.bin
```

5. Insert the USB flash drive into the STM32F407 board.
6. Power on or reset the board.
7. The bootloader detects the firmware, validates it, and updates the application if required.

If the firmware is already identical, the bootloader does not erase or reprogram the application Flash.

## Application Validation

Before jumping to the application, the bootloader checks the initial stack pointer stored at the beginning of the application vector table.

For the STM32F407, the application starts at:

```text
0x08020000
```

and the initial MSP is expected to point to the STM32F407 SRAM region.

The bootloader also uses the stored application header and CRC to verify the application image.

This provides both a basic startup check and firmware integrity verification.

## Debug Output

The bootloader prints its status over UART, including:

* USB detection
* FATFS mounting
* Firmware file opening
* Header information
* Image size
* CRC value
* Flash programming status
* CRC verification status
* Application jump status
* Errors during the update process

This makes it easier to debug the update process and see exactly what the bootloader is doing.

## Tested On

STM32F407 Discovery Board

## Main Technologies

* STM32F4
* C / C++
* STM32 HAL
* USB Host MSC
* FATFS
* UART
* CRC32
* STM32 Internal Flash
* Custom application bootloader
