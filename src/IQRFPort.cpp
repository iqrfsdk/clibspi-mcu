/**
 * @file IQRF SPI support library
 * @author Dušan Machút <dusan.machut@iqrf.com>
 * @author Rostislav Špinar <rostislav.spinar@iqrf.com>
 * @author Roman Ondráček <roman.ondracek@iqrf.com>
 * @version 3.1.1
 *
 * Copyright 2015-2018 IQRF Tech s.r.o.
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
#include <Arduino.h>
#include <ctype.h>
#include <SPI.h>
#include <SD.h>
#include <TimerOne.h>
#include "IQRF.h"

extern "C" void iqrfDriver(void);

File CodeFile;
IQRF_PGM_FILE_INFO  CodeFileInfo;
T_IQRF_CONTROL IqrfControl;

/**
 * initialize IQRF SPI kernel timing
 */
void iqrfKernelTimingInit(void)
{
    Timer1.initialize(1000);                             // initialize timer1, call IQRF driver every 1000us
    Timer1.attachInterrupt(iqrfDriver);                  // attaches callback() as a timer overflow interrupt
}


/**
 * switch IQRF SPI kernel timing to fast mode
 */
void iqrfKernelTimingFastMode(void)
{
    Timer1.stop();                                      // stop timer1
    IqrfControl.FastSPI = true;                         // set FastSPI flag
    Timer1.setPeriod(200);                              // call IQRF driver every 200us
    Timer1.start();                                     // start timer 1
}


/**
 * turn OFF power supply of TR module
 */
void iqrfTrPowerOff(void)
{
    pinMode(TR_SS_PIN, OUTPUT);
    pinMode(TR_PWRCTRL_PIN, OUTPUT);
    digitalWrite(TR_SS_PIN, LOW);
    digitalWrite(TR_PWRCTRL_PIN, HIGH);
}


/**
 * turn ON power supply of TR module
 */
void iqrfTrPowerOn(void)
{
    pinMode(TR_SS_PIN, OUTPUT);
    pinMode(TR_PWRCTRL_PIN, OUTPUT);
    digitalWrite(TR_SS_PIN, HIGH);
    digitalWrite(TR_PWRCTRL_PIN, LOW);
}


/**
 * switch TR module to programming mode
 */
void iqrfTrEnterPgmMode(void)
{
    uint32_t SysTickTime;

    iqrfDelayMs(200);
    iqrfSuspendDriver();
    SPI.end();
    pinMode(TR_MOSI_PIN, OUTPUT);
    pinMode(TR_MISO_PIN, INPUT);
    pinMode(TR_SCK_PIN, OUTPUT);
    digitalWrite(TR_SCK_PIN, LOW);
    digitalWrite(TR_MOSI_PIN, LOW);
    iqrfTrReset();
    digitalWrite(TR_SS_PIN, LOW);
    SysTickTime = iqrfGetSysTick();
    do {
        // Copy MOSI to MISO for approx. 500ms => TR into programming mode
        digitalWrite(TR_MOSI_PIN, digitalRead(TR_MISO_PIN));
    } while ((iqrfGetSysTick() - SysTickTime) < (TICKS_IN_SECOND / 2));
    digitalWrite(TR_SS_PIN, HIGH);
    SPI.begin();
    iqrfRunDriver();
}


/**
 * Deselect TR module
 */
void iqrfDeselectTRmodule(void)
{
    digitalWrite(TR_SS_PIN, HIGH);
    IqrfControl.TRmoduleSelected = false;
    SPI.endTransaction();
}


/**
 * Send byte over SPI
 *
 * @param Tx_Byte to send
 * @return Received Rx_Byte
 *
 */
uint8_t iqrfSendSpiByte(uint8_t Tx_Byte)
{
    uint8_t Rx_Byte;

    if (!IqrfControl.TRmoduleSelected) {
        SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE0));
        IqrfControl.TRmoduleSelected = true;
        digitalWrite(TR_SS_PIN, LOW);
        delayMicroseconds(15);
    }

    Rx_Byte = SPI.transfer(Tx_Byte);

    if (IqrfControl.FastSPI == false) {
        delayMicroseconds(15);
        iqrfDeselectTRmodule();
    }

    return (Rx_Byte);
}


/**
 * Read byte from code file
 *
 * @param - none
 * @return - byte from firmware file or 0 = end of file
 *
 */
uint8_t iqrfReadByteFromFile(void)
{
    if (CodeFile.available()) {
        CodeFileInfo.FileByteCnt++;
        return (CodeFile.read());
    } else {
        return(0);
    }
}
