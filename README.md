# clibspi-mcu

IQRF SPI library for uC with DCTR upload functionality.
Targeted for Arduino boards.

Console commands:

- rst : clears the screen
- ls : lists of files on the SD card
- stat : shows SPI status of the DCTR module (e.g. 0x80, 0x81 ...)
- trrst : resets of the DCTR module
- trpwroff : turns off the power of the DCTR module
- trpwron : turns on the power of the DCTR module
- trinfo : shows DCTR module info
- trpgmmode : switches DCTR module into programming mode
- send string : sends ASCII string to the DCTR module
- pgm hex file.ext : tests and uploads file *.hex into DCTR module
- pgm iqrf file.ext : tests and uploads file *.iqrf into DCTR module
- pgm trcnfg file.ext : tests and uploads file *.trcnfg into DCTR module

Missing:

- upload of the USER PASSWORD a USER KEY (will be added later)
