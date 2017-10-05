/**
 * @file IQRF SPI support library
 * @author Dušan Machút <dusan.machut@iqrf.com>
 * @author Rostislav Špinar <rostislav.spinar@iqrf.com>
 * @author Roman Ondráček <roman.ondracek@iqrf.com>
 * @version 3.0.0
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

#ifndef IQRF_PORTS_H
#define IQRF_PORTS_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Pins
#if !defined(TR_PWRCTRL_PIN)
#define TR_PWRCTRL_PIN        9           //!< TR power control pin
#endif
#if !defined(TR_SS_PIN)
#define TR_SS_PIN             8           //!< SPI SS pin
#endif

#define TR_MOSI_PIN         PIN_SPI_MOSI  //!< SPI MOSI pin
#define TR_MISO_PIN         PIN_SPI_MISO  //!< SPI MISO pin
#define TR_SCK_PIN          PIN_SPI_SCK   //!< SPI SCK pin

#define TICKS_IN_SECOND     1000

#define iqrfDelayMs(T)    delay(T)

#define iqrfGetSysTick()    millis()

typedef struct {
    uint16_t FileByteCnt;             // size of code file on SD card
    uint16_t FileSize;                // size of code file on SD card
    uint8_t FileType;                 // file type (HEX / IQRF)
}IQRF_PGM_FILE_INFO;

typedef void (*T_IQRF_RX_HANDLER)(uint8_t *DataBuffer, uint8_t DataSize);

typedef struct {
    volatile uint8_t Status;
    uint8_t SuspendFlag;
    uint8_t TRmoduleSelected;
    uint8_t FastSPI;
    uint8_t TimeCnt;
    T_IQRF_RX_HANDLER IqrfRxHandler;
} T_IQRF_CONTROL;

extern IQRF_PGM_FILE_INFO  CodeFileInfo;
extern T_IQRF_CONTROL      IqrfControl;

/**
 * initialize IQRF SPI kernel timing
 */
void iqrfKernelTimingInit(void);

/**
 * switch IQRF SPI kernel timing to fast mode
 */
void iqrfKernelTimingFastMode(void);

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
 * Deselect TR module
 */
void iqrfDeselectTRmodule(void);

/**
 * Send byte over SPI
 *
 * @param Tx_Byte to send
 * @return Received Rx_Byte
 *
 */
uint8_t iqrfSendSpiByte(uint8_t Tx_Byte);

/**
 * Read byte from code file
 *
 * @param - none
 * @return - byte from firmware file or 0 = end of file
 *
 */
uint8_t iqrfReadByteFromFile(void);

#if defined(__cplusplus)
}
#endif

#endif
