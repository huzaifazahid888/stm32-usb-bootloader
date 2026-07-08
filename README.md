# STM32 USB Host Bootloader

A USB Host bootloader for STM32F4 that updates firmware straight from a USB flash drive, using FATFS to read the binary image off the drive. Plug in a USB stick with the new firmware, power the board, and the bootloader takes care of the rest: it checks if the firmware on the drive is actually different from what's already flashed, and only reprograms if it needs to.

## What it does

- Detects a USB flash drive over USB Host Mass Storage (MSC)
- Reads a `.bin` firmware image off the drive through FATFS
- Compares it against the firmware currently in flash before doing anything
- Erases and reprograms flash only if an update is actually needed
- Jumps to the user application afterward, or immediately if no USB drive is found
- Reports status over UART and through an LED
- Built entirely on STM32 HAL

## Repository Structure

```text
.
├── Core/
│   ├── Inc/          # Application headers
│   ├── Src/          # Application source (main.c, bootloader logic)
│   └── Startup/      # Startup assembly and linker files
│
├── Drivers/          # STM32 HAL drivers
├── FATFS/            # FATFS middleware
├── Middlewares/      # USB middleware
├── USB_HOST/         # USB Host stack
└── README.md
```

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
 Wait up to 3 seconds for USB flash drive
                    │
          ┌─────────┴─────────┐
          │                   │
     USB Detected         Timeout
          │                   │
          ▼                   ▼
      Mount FATFS      Jump to Application
          │
          ▼
  Locate Firmware Image
          │
          ▼
 Compare with Installed Firmware
      │              │
      │              │
 Identical      New Firmware
      │              │
      ▼              ▼
Jump to App    Erase Flash
                     │
                     ▼
              Program Flash
                     │
                     ▼
             Jump to Application
```

## Updating firmware

1. Copy the application binary onto a USB flash drive.
2. Rename it to `BLINK.BIN`.
3. Insert the drive into the board.
4. Power on the board.

From there it's automatic: the bootloader detects the drive, mounts the filesystem, opens the firmware file, compares it against what's installed, programs flash if needed, and launches the application. If there's no USB drive plugged in at all, it just boots straight into whatever's already flashed.

## Flash programming

Programming is done sector-erase followed by word-by-word writes, with verification by comparing memory afterward, and flash gets locked again once programming is done. Since it only reprograms when the firmware is actually different, it avoids unnecessary erase/write cycles on flash.

## Debug output

UART prints out what's happening at each step: USB detection, drive mounting, whether a firmware file was found, its size, flash erase, flash programming, application launch, and any errors along the way. Useful for actually seeing what's going on instead of guessing why it didn't boot.

## Tested on

STM32F407 Discovery Board

## Possible improvements

- CRC verification on the firmware image
- Version checking before triggering an update
- Dual-bank firmware update
- Rollback if a new firmware image fails to boot
- Secure firmware authentication
- Progress indication while programming

