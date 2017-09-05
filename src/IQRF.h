/**
 * @file IQRF SPI support library
 * @author Dušan Machút <dusan.machut@gmail.com>
 * @author Rostislav Špinar <rostislav.spinar@iqrf.com>
 * @author Roman Ondráček <ondracek.roman@centrum.cz>
 * @version 2.0
 *
 * Copyright 2015-2017 IQRF Tech s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _IQRF_H
#define _IQRF_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "IQRFPorts.h"

// iqrfSendData(...  ) function return codes
#define IQRF_OPERATION_OK             0
#define IQRF_OPERATION_IN_PROGRESS    1
#define IQRF_TR_MODULE_WRITE_ERR      2
#define IQRF_TR_MODULE_NOT_READY      3
#define IQRF_WRONG_DATA_SIZE          4

// MCU type of TR module
#define MCU_UNKNOWN                   0
#define PIC16LF819                    1     // TR-xxx-11A not supported
#define PIC16LF88                     2     // TR-xxx-21A
#define PIC16F886                     3     // TR-31B, TR-52B, TR-53B
#define PIC16LF1938                   4     // TR-52D, TR-54D

// TR module types
#define TR_52D                        0
#define TR_58D_RJ                     1
#define TR_72D                        2
#define TR_53D                        3
#define TR_54D                        8
#define TR_55D                        9
#define TR_56D                        10
#define TR_76D                        11

// FCC cerificate
#define FCC_NOT_CERTIFIED             0
#define FCC_CERTIFIED                 1


//******************************************************************************
//		 	SPI status of TR module (see IQRF SPI user manual)
//******************************************************************************
#define NO_MODULE                   0xFF  // SPI not working (HW error)
#define SPI_DATA_TRANSFER           0xFD  // SPI data transfer in progress
#define SPI_DISABLED                0x00  // SPI not working (disabled)
#define SPI_CRCM_OK                 0x3F  // SPI not ready (full buffer, last CRCM ok)
#define SPI_CRCM_ERR                0x3E  // SPI not ready (full buffer, last CRCM error)
#define COMMUNICATION_MODE          0x80  // SPI ready (communication mode)
#define PROGRAMMING_MODE            0x81  // SPI ready (programming mode)
#define DEBUG_MODE                  0x82  // SPI ready (debugging mode)
#define SPI_SLOW_MODE               0x83  // SPI not working in background
#define SPI_USER_STOP               0x07  // state after stopSPI();

//******************************************************************************
//		 	SPI commands for TR module (see IQRF SPI user manual)
//******************************************************************************
#define SPI_CHECK                   0x00  // Master checks the SPI status of the TR module
#define SPI_WR_RD                   0xF0  // Master reads/writes a packet from/to TR module
#define SPI_RAM_READ                0xF1  // Master reads data from ram in debug mode
#define SPI_EEPROM_READ             0xF2  // Master reads data from eeprom in debug mode
#define SPI_EEPROM_PGM              0xF3  // Master writes data to eeprom in programming mode
#define SPI_MODULE_INFO             0xF5  // Master reads Module Info from TR module
#define SPI_FLASH_PGM               0xF6  // Master writes data to flash in programming mode
#define SPI_PLUGIN_PGM              0xF9  // Master writes plugin data to flash in programming mode

//******************************************************************************
//		 	status of IQRF SPI library
//******************************************************************************
#define IQRF_READY                  0x00  // IQRF support library ready
#define IQRF_READ                   0x01  // IQRF read request processing
#define IQRF_WRITE                  0x02  // IQRF write request processing

//******************************************************************************
//		 	status of IQRF SPI library TX buffer
//******************************************************************************
#define IQRF_BUFFER_FREE            0x00  // buffer is ready for new packet
#define IQRF_BUFFER_BUSY            0x01  // buffer is bussy

typedef struct{                           // TR module info structure
    uint16_t	OsVersion;
    uint16_t	OsBuild;
    uint32_t	ModuleId;
    uint16_t	McuType;
    uint16_t  ModuleType;
    uint16_t  Fcc;
    uint8_t	  ModuleInfoRawData[8];
} T_TR_INFO_STRUCT;


typedef void (*T_IQRF_RX_HANDLER)(uint8_t *DataBuffer, uint8_t DataSize);

extern T_TR_INFO_STRUCT	  IqrfTrInfoStruct;


/**
 * Function initialize IQRF SPI support library
 * @param UserIqrfRxHandler Pointer to user call back function for received packets service
 */
void iqrfInit(T_IQRF_RX_HANDLER UserIqrfRxHandler);

/**
 * Function provides background communication with TR module
 */
void iqrfDriver(void);

/**
 * Sends IQRF data packet to TR module
 * @param DataBuffer Pointer to buffer with IQRF SPI packet
 * @param DataSize size of IQRF SPI packet
 * @return Operation result (IQRF_OPERATION_OK, IQRF_OPERATION_IN_PROGRESS, IQRF_TR_MODULE_WRITE_ERR ... )
 */
uint8_t iqrfSendData(uint8_t *DataBuffer, uint8_t DataLength);

/**
 * Sends IQRF packet with specific SPI command to TR module
 * @param SpiCmd SPI command for TR module
 * @param UserDataBuffer Pointer to buffer with IQRF SPI packet
 * @param UserDataLength size of IQRF SPI packet
 */
void iqrfSendPacket(uint8_t SpiCmd, uint8_t *UserDataBuffer, uint8_t UserDataLength);

/**
 * Temporary suspend IQRF comunication driver
 */
void iqrfSuspendDriver(void);

/**
 * Run IQRF comunication driver
 */
void iqrfRunDriver(void);

/**
 * reset TR module
 */
void iqrfTrReset(void);

/**
 * turn OFF power supply of TR module
 */
void iqrfTrPowerOff(void);

/**
 * turn ON power supply of TR module
 */
void iqrfTrPowerOn(void);

/**
 * switch TR module to programming mode
 */
void iqrfTrEnterPgmMode(void);

/**
 * end of programming mode
 */
void iqrfTrEndPgmMode(void);

/**
 * get SPI status of TR module
 * @return SPI status of TR module
 */
uint8_t iqrfGetSpiStatus(void);

/**
 * get status of IQRF SPI support library
 * @return status of support library
 */
uint8_t iqrfGetLibraryStatus(void);

/**
 * get status of TX buffer of IQRF SPI support library
 * @return status of TX buffer
 */
uint8_t iqrfGetTxBufferStatus(void);

/**
 * Macro: return TR module OS version
 */
#define iqrfGetOsVersion()		IqrfTrInfoStruct.OsVersion

/**
 * Macro: return TR module OS build
 */
#define iqrfGetOsBuild()		IqrfTrInfoStruct.OsBuild

/**
 * Macro: return TR module ID
 */
#define iqrfGetModuleId()		IqrfTrInfoStruct.ModuleId

/**
 * Macro: return TR module MCU type
 *
 * Overview: macro returns UINT16 data word
 *      0 - unknown type
 *      1 - PIC16LF819
 *      2 - PIC16LF88
 *      3 - PIC16F886
 *      4 - PIC16LF1938
 */
#define iqrfGetMcuType()		IqrfTrInfoStruct.McuType

/**
 * Macro: return TR module MCU type
 *
 * Overview: macro returns UINT16 data word
 *      0 - TR_52D
 *      1 - TR_58D_RJ
 *      2 - TR_72D
 *      3 - TR_53D
 *      8 - TR_54D
 *      9 - TR_55D
 *      10 - TR_56D
 *      11 - TR_76D
 */
#define iqrfGetModuleType()		IqrfTrInfoStruct.ModuleType

/**
 * Macro: return TR module FCC status
 */
#define iqrfGetFccStatus()		IqrfTrInfoStruct.Fcc

/**
 * Macro: return specific byte of TR module identification RAW data
 */
#define iqrfGetModuleInfoRawData(x)	IqrfTrInfoStruct.ModuleInfoRawData[x]

#if defined(__cplusplus)
}
#endif

#endif
