# clibspi-mcu

[![Build Status](https://travis-ci.org/iqrfsdk/clibspi-mcu.svg?branch=master)](https://travis-ci.org/iqrfsdk/clibspi-mcu)

IQRF SPI library for uC with TR-7x upload functionality.
Targeted for Arduino boards.

**Supported IQRF OS version of the TR-7x module:**

- IQRF OS v4.02D

## Documentation:

- please, refer to current IQRF Startup Package for updated IQRF SPI specification

## Integration
If the user wishes to use the services of the library, the files [```IQRF.c```](src/IQRF.c), [```IQRF.h```](src/IQRF.h), [```IQRFPgm.c```](src/IQRFPgm.c), [```IQRFPgm.h```](src/IQRFPgm.h), [```IQRFPort.cpp```](src/IQRFPort.cpp) and [```IQRFPort.h```](src/IQRFPort.h) must be included in user's project. If the user does not need programming features for the TR module, the files ```IQRFPgm.c``` and ```IQRFPgm.h``` are not required. The ```IQRFPort.cpp``` and ```IQRFPort.h``` files are platform-dependent and contain a platform interface for a platform-independent library core. If the library is used on a different platform than Arduino, the user must modify the ```IQRFPort.cpp``` and ```IQRFPort.h``` files for the platform used. The following macros and functions must be modified.

**MACROS**
-   ```#define iqrfGetSysTick()``` - Get the system timer value (SysTick)
-   ```#define TICKS_IN_SECOND``` - Frequency of the system timer in Hz
-   ```#define iqrfDelayMs(T)``` - Delay function. Time ```T``` in ms

**FUNCTIONS**
-   ```void iqrfKernelTimingInit(void)``` - Initialize timer to 1000us period. In interrupt service rutine of timer, call the IQRF SPI communication driver ```void iqrfDriver(void)```
-   ```void iqrfKernelTimingFastMode(void)``` - Change the timer period to 200us (time interval for fast SPI communication for TR-7xD modules)
-   ```void iqrfTrPowerOff(void)``` - Turn OFF power supply of TR module
-   ```void iqrfTrPowerOn(void)``` - Turn ON power supply of TR module
-   ```void iqrfTrEnterPgmMode(void)``` - Switch TR module to programming mode
-   ```uint8_t iqrfSendSpiByte(uint8_t Tx_Byte)``` - Send / receive one byte to / from TR module over SPI bus
-   ```void iqrfDeselectTRmodule(void)``` - Deactivate selection signal of TR module
-   ```uint8_t iqrfReadByteFromFile(void)``` - Read one byte from the currently open file, with the new code for TR module

## API functions
-   ```void iqrfInit(T_IQRF_RX_HANDLER UserIqrfRxHandler)``` - Initialize IQRF SPI communication library. ```UserIqrfRxHandler``` is pointer on the user's callback function to process the received packet
-   ```uint8_t iqrfSendData(uint8_t *DataBuffer, uint8_t DataLength)``` - The function sends the data packet to TR module via SPI interface. The user fills the ```DataBuffer``` with its data and defines size of data packet. The function must be called periodically, if returns code ```IQRF_OPERATION_IN_PROGRESS```. Periodically function calling is necessary end, when returns one of the following return codes:
    -   ```IQRF_OPERATION_OK``` - operation OK, data sent successfully
    -   ```IQRF_TR_MODULE_WRITE_ERR```  - operation ERROR, data not sent
    -   ```IQRF_WRONG_DATA_SIZE```  - operation ERROR, wrong data size specified
    -   ```IQRF_TR_MODULE_NOT_READY```  - operation ERROR, TR module is not ready   
    
-   ```void iqrfSendPacket(uint8_t SpiCmd, uint8_t *UserDataBuffer, uint8_t UserDataLength)``` - The function will start the process of sending the packet to the TR module. The packet is sent in the background, by the IQRF SPI communications driver. The user set the ```SpiCmd``` command (see IQRF SPI specification), fills ```UserDataBuffer```  with its data and defines size of data packet. Before calling the function, check the IQRF broadcast buffer status. Use the ```uint8_t iqrfGetTxBufferStatus(void)``` function, to do this.
-   ```void iqrfSuspendDriver(void)``` - Temporary suspend IQRF SPI comunication driver
-   ```void iqrfRunDriver(void)``` - Run suspended IQRF SPI communication driver
-   ```void iqrfTrPowerOff(void)``` - Turn OFF power supply of TR module
-   ```void iqrfTrPowerOn(void)``` - Turn ON power supply of TR module
-   ```void iqrfTrReset(void)``` - Reset TR module
-   ```void iqrfTrEnterPgmMode(void)``` - Switch TR module to programming mode
-   ```void iqrfTrEndPgmMode(void)``` - End of programming mode
-   ```uint8_t iqrfGetSpiStatus(void)``` - Get SPI status of TR module (see IQRF SPI specification). Function returns one of the following return codes:
    -   ```NO_MODULE``` - SPI not working (HW error)
    -   ```SPI_DATA_TRANSFER```  - SPI data transfer in progress
    -   ```SPI_DISABLED```  - SPI not working (disabled)
    -   ```SPI_CRCM_OK```  - SPI not ready (full buffer, last CRCM ok)
    -   ```SPI_CRCM_ERR```  - SPI not ready (full buffer, last CRCM error)
    -   ```COMMUNICATION_MODE```  - SPI ready (communication mode)
    -   ```PROGRAMMING_MODE```  - SPI ready (programming mode)
    -   ```DEBUG_MODE```  - SPI ready (debugging mode)
    -   ```SPI_SLOW_MODE```  - SPI not working in background
    -   ```SPI_USER_STOP```  - state after stopSPI();

-   ```uint8_t iqrfGetLibraryStatus(void)``` - Get status of IQRF SPI communication library. Function returns one of the following return codes:
    -   ```IQRF_READY``` - IQRF communication library is ready
    -   ```IQRF_READ```  - IQRF communication library is busy, processing requests for reading
    -   ```IQRF_WRITE```  - IQRF communication library is busy, processing requests for witing

-   ```uint8_t iqrfGetTxBufferStatus(void)``` - Get status of TX buffer of IQRF SPI communication library. Function returns one of the following return codes:
    -   ```IQRF_BUFFER_FREE``` - Buffer is ready for new packet
    -   ```IQRF_BUFFER_BUSY```  - Buffer is bussy

-   ```uint16_t iqrfGetOsVersion(void)``` - Get TR module OS version
-   ```uint16_t iqrfGetOsBuild(void)``` - Get TR module OS build
-   ```uint32_t iqrfGetModuleId(void)``` - Get TR module ID
-   ```uint16_t iqrfGetMcuType(void)``` - Get TR module MCU type. Function returns one of the following return codes:
    -   ```MCU_UNKNOWN```
    -   ```PIC16LF819```
    -   ```PIC16LF88```
    -   ```PIC16F886```
    -   ```PIC16LF1938```

-   ```uint16_t iqrfGetModuleType(void)``` - Get TR module type. Function returns one of the following return codes:
    -   ```TR_52D```
    -   ```TR_58D_RJ```
    -   ```TR_72D```
    -   ```TR_53D```
    -   ```TR_54D```
    -   ```TR_55D```
    -   ```TR_56D```
    -   ```TR_76D```

-   ```uint16_t iqrfGetFccStatus(void)``` - Get TR module FCC status. Function returns one of the following return codes:
    -   ```FCC_NOT_CERTIFIED```
    -   ```FCC_CERTIFIED```

-   ```uint8_t iqrfGetModuleInfoRawData(uint8_t Cnt)``` - Get specific byte of TR module identification RAW data.

-   ```uint8_t iqrfPgmCheckCodeFile(void)``` - The function checking the format accuracy of the programing file. Use of this function you can to see in the [```Console.ino```](https://github.com/iqrfsdk/clibspi-mcu/blob/master/examples/Console/Console/Console.ino) example file. The user opens the programming file and fills the structure ```IQRF_PGM_FILE_INFO  CodeFileInfo``` with informations about  programming file. The function must be called periodically if it returns the code in the range 0 to 100. Periodically function calling is necessary end, when returns one of the following return codes:
    -   ```IQRF_PGM_SUCCESS``` - programming file is OK
    -   ```IQRF_PGM_ERROR```  - programming file format ERROR

-   ```uint8_t iqrfPgmWriteCodeFile(void)``` - The function writes the programing file to TR module. Use of this function you can to see in the [```Console.ino```](https://github.com/iqrfsdk/clibspi-mcu/blob/master/examples/Console/Console/Console.ino) example file. The user opens the programming file and fills the structure ```IQRF_PGM_FILE_INFO  CodeFileInfo``` with informations about  programming file. The function must be called periodically if it returns the code in the range 0 to 100. Periodically function calling is necessary end, when returns one of the following return codes:
    -   ```IQRF_PGM_SUCCESS``` - programming OK, file has been written successfully
    -   ```IQRF_PGM_ERROR```  - programming ERROR, file hasn't been written successfully

-   ```uint8_t iqrfPgmWriteKeyOrPass(uint8_t BufferContent, uint8_t *Buffer)``` - The function writes the USER PASSWORD or USER KEY to TR module. Use of this function you can to see in the [```Console.ino```](https://github.com/iqrfsdk/clibspi-mcu/blob/master/examples/Console/Console/Console.ino) example file. The user fills the 16 byte buffer with the USER PASSWORD or USER KEY and selects if the USER PASSWORD or USER KEY will be written. The function must be called periodically if it returns the code 0. Periodically function calling is necessary end, when returns one of the following return codes:
    -   ```IQRF_PGM_SUCCESS``` - programming OK, USER PASSWORD or USER KEY has been written successfully
    -   ```IQRF_PGM_ERROR```  - programming ERROR, USER PASSWORD or USER KEY hasn't been written successfully

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
- `pgm pass file.bin`: tests and uploads USER PASSWORD from file `*.bin` into TR module
- `pgm key file.bin`: tests and uploads USER KEY from file `*.bin` into TR module

## License
This library is licensed under Apache License 2.0:

 > Copyright 2015-2017 IQRF Tech s.r.o.
 >
 > Licensed under the Apache License, Version 2.0 (the "License");
 > you may not use this file except in compliance with the License.
 > You may obtain a copy of the License at
 >
 >     http://www.apache.org/licenses/LICENSE-2.0
 >
 > Unless required by applicable law or agreed to in writing, software
 > distributed under the License is distributed on an "AS IS" BASIS,
 > WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 > See the License for the specific language governing permissions and
 > limitations under the License.

