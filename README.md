# clibspi-mcu

[![Build Status](https://travis-ci.org/iqrfsdk/clibspi-mcu.svg?branch=master)](https://travis-ci.org/iqrfsdk/clibspi-mcu)

IQRF SPI library for uC with TR-7x upload functionality.
Targeted for Arduino boards.

**Supported IQRF OS version of the TR-7x module:**

- IQRF OS v4.02D

## Documentation:

- please, refer to current IQRF Startup Package for updated IQRF SPI specification

## Console commands:

- `rst`: clears the screen
- `ls`: lists of files on the SD card
- `stat`: shows SPI status of the TR module (e.g. 0x80, 0x81 ...)
- `trrst`: resets of the TR module
- `trpwroff`: turns off the power of the TR module
- `trpwron`: turns on the power of the TR module
- `trinfo`: shows TR module info
- `trpgmmode`: switches TR module into programming mode
- `send string`: sends ASCII string to the TR module
- `pgm hex file.hex`: tests and uploads file `*.hex` into TR module
- `pgm iqrf file.iqrf`: tests and uploads file `*.iqrf` into TR module
- `pgm trcnfg file.trcnfg`: tests and uploads file `*.trcnfg` into TR module

## Todo:

- upload of the `USER PASSWORD` a `USER KEY`
