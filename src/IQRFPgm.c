/**
 * @file IQRF SPI support library (firmware and config programmer extension)
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
#include "IQRF.h"
#include "IQRFPgm.h"

#define SIZE_OF_CODE_LINE_BUFFER  32

typedef struct {
    uint32_t  HiAddress;
    uint16_t  Address;
    uint16_t  MemoryBlockNumber;
    uint8_t MemoryBlockProcessState;
    uint8_t DataInBufferReady;
    uint8_t DataOverflow;
    uint8_t MemoryBlock[68];
} PREPARE_MEM_BLOCK;

/* Function prototypes */
uint8_t iqrfPgmWriteKeyOrPass(uint8_t Selector, uint8_t *Buffer);
uint8_t iqrfPgmProcessCfgFile(void);
void iqrfPgmMoveOverflowedData(void);
uint8_t iqrfPgmPrepareMemBlock(void);
uint8_t iqrfPgmReadIQRFFileLine(void);
uint8_t iqrfPgmReadHEXFileLine(void);

/* Public variable declarations */
uint8_t IqrfPgmCodeLineBuffer[SIZE_OF_CODE_LINE_BUFFER];
PREPARE_MEM_BLOCK PrepareMemBlock;

/**
 * Checking the format accuracy of the programming file
 * @return result of partial checking operation
 */
uint8_t iqrfPgmCheckCodeFile(void)
{
    static enum {
        INIT_TASK = 0,
        CHECK_PLUGIN_CODE,
        CHECK_HEX_CODE,
        CHECK_CFG_CODE,
    } CheckCodeTaskSM = INIT_TASK;

    uint8_t OperationResult;

    switch (CheckCodeTaskSM) {
    // initialize the checking process
    case INIT_TASK:
        CodeFileInfo.FileByteCnt = 0;
        if (CodeFileInfo.FileType == IQRF_PGM_PLUGIN_FILE_TYPE) {
            CheckCodeTaskSM = CHECK_PLUGIN_CODE;
        } else {
            PrepareMemBlock.DataInBufferReady = 0;
            PrepareMemBlock.DataOverflow = 0;
            if (CodeFileInfo.FileType == IQRF_PGM_HEX_FILE_TYPE) {
                CheckCodeTaskSM = CHECK_HEX_CODE;
            } else {
                if (CodeFileInfo.FileSize < 33)
                    return(IQRF_PGM_ERROR);
                CheckCodeTaskSM = CHECK_CFG_CODE;
            }
        }
        break;

    // check if format of *.IQRF file is correct
    case CHECK_PLUGIN_CODE:
        // read one line from *.IQRF file
        iqrfSuspendDriver();
        OperationResult = iqrfPgmReadIQRFFileLine();
        iqrfRunDriver();
        // if any error in line format
        if (OperationResult == IQRF_PGM_FILE_DATA_ERROR) {
            CheckCodeTaskSM = INIT_TASK;               // initialize state machine
            return(IQRF_PGM_ERROR);                    // return error code
        } else {
            // if end of file
            if (OperationResult == IQRF_PGM_END_OF_FILE) {
                CheckCodeTaskSM = INIT_TASK;          // initialize state machine
                return(IQRF_PGM_SUCCESS);             // file format is correct
            }
        }
        break;

        // check if format of *.HEX of *.trcnfg file is correct
    case CHECK_HEX_CODE:
    case CHECK_CFG_CODE:
        if (CodeFileInfo.FileType == IQRF_PGM_HEX_FILE_TYPE)
            OperationResult = iqrfPgmPrepareMemBlock();
        else
            OperationResult = iqrfPgmProcessCfgFile();
        if (OperationResult != IQRF_PGM_FLASH_BLOCK_READY && OperationResult != IQRF_PGM_EEPROM_BLOCK_READY) {
            CheckCodeTaskSM = INIT_TASK;
            return(OperationResult);
        }
        break;
    }

    // return file processing status in percent
    return(((uint32_t) CodeFileInfo.FileByteCnt * 100) / CodeFileInfo.FileSize);
}

/**
 * Core programming function
 * @return result of partial programming operation
 */
uint8_t iqrfPgmWriteCodeFile(void)
{
    static enum {
        INIT_TASK = 0,
        ENTER_PROG_MODE,
        WAIT_PROG_MODE,
        WRITE_PLUGIN,
        WRITE_HEX,
        WAIT_PROG_END,
        PROG_END,
    } WriteCodeTaskSM = INIT_TASK;

    static uint8_t Attempts;
    static uint8_t OperationResult;
    static uint32_t SysTickTime;

   	switch (WriteCodeTaskSM) {
    case INIT_TASK:     // initialize programming state machine
        Attempts = 1;
        CodeFileInfo.FileByteCnt = 0;
        WriteCodeTaskSM = ENTER_PROG_MODE;
        break;

    case ENTER_PROG_MODE:
        iqrfTrEnterPgmMode();
        SysTickTime = iqrfGetSysTick();
        WriteCodeTaskSM = WAIT_PROG_MODE;
        break;

    case WAIT_PROG_MODE:      // wait for TR module programming mode
        if (iqrfGetSpiStatus() == PROGRAMMING_MODE && iqrfGetLibraryStatus() == IQRF_READY) {
            SysTickTime = iqrfGetSysTick();
            if (CodeFileInfo.FileType == IQRF_PGM_PLUGIN_FILE_TYPE){
                WriteCodeTaskSM = WRITE_PLUGIN;
            } else {
                PrepareMemBlock.DataInBufferReady = 0;
                PrepareMemBlock.DataOverflow = 0;
                PrepareMemBlock.MemoryBlockProcessState = 0;
                WriteCodeTaskSM = WRITE_HEX;
            }
        } else {
            if (iqrfGetSysTick() - SysTickTime >= (TICKS_IN_SECOND / 2)) {
                // in a case, try it twice to enter programming mode
                if (Attempts) {
                    Attempts--;
                    WriteCodeTaskSM = ENTER_PROG_MODE;
                } else {
                    // TR module probably does not work
                    OperationResult = IQRF_PGM_ERROR;
                    WriteCodeTaskSM = PROG_END;
                }
            }
        }
        break;

    case WRITE_PLUGIN:      // write plugin to TR module
        // if no packet is pending to send to TR module
        if (iqrfGetTxBufferStatus() == IQRF_BUFFER_FREE
            && iqrfGetSpiStatus() == PROGRAMMING_MODE
            && iqrfGetLibraryStatus() == IQRF_READY)
        {
            iqrfSuspendDriver();
            Attempts = iqrfPgmReadIQRFFileLine();
            iqrfRunDriver();
            if (Attempts == IQRF_PGM_FILE_DATA_ERROR) {
                OperationResult = IQRF_PGM_ERROR;
                WriteCodeTaskSM = WAIT_PROG_END;                // go to end programming mode
            } else {
                if (Attempts == IQRF_PGM_END_OF_FILE) {
                    OperationResult = IQRF_PGM_SUCCESS;
                    WriteCodeTaskSM = WAIT_PROG_END;            // go to end programming mode
                } else {
                    // send plugin PGM packet
                    iqrfSendPacket(SPI_PLUGIN_PGM, IqrfPgmCodeLineBuffer, 20);
                    SysTickTime = iqrfGetSysTick();
                }
            }
        } else {
            if (iqrfGetSysTick() - SysTickTime >= (TICKS_IN_SECOND / 2)) {
                iqrfTrReset();
                OperationResult = IQRF_PGM_ERROR;
                WriteCodeTaskSM = PROG_END;                       // go to end programming mode
            }
        }
        break;

    case WRITE_HEX:      // write HEX file to TR module
        // if no packet is pending to send to TR module
        if (iqrfGetTxBufferStatus() == IQRF_BUFFER_FREE
            && iqrfGetSpiStatus() == PROGRAMMING_MODE
            && iqrfGetLibraryStatus() == IQRF_READY)
        {
            if (PrepareMemBlock.MemoryBlockProcessState == 0) {
                if (CodeFileInfo.FileType == IQRF_PGM_HEX_FILE_TYPE)
                    OperationResult = iqrfPgmPrepareMemBlock();
                else
                    OperationResult = iqrfPgmProcessCfgFile();
                if (OperationResult != IQRF_PGM_FLASH_BLOCK_READY && OperationResult != IQRF_PGM_EEPROM_BLOCK_READY)
                    WriteCodeTaskSM = WAIT_PROG_END;            // go to end programming mode
            } else {
                if (OperationResult == IQRF_PGM_FLASH_BLOCK_READY) {
                    if (PrepareMemBlock.MemoryBlockProcessState == 2)
                        iqrfSendPacket(SPI_FLASH_PGM, (uint8_t *)&PrepareMemBlock.MemoryBlock[0], 32 + 2);
                    else
                        iqrfSendPacket(SPI_FLASH_PGM, (uint8_t *)&PrepareMemBlock.MemoryBlock[34], 32 + 2);
                } else {
                    iqrfSendPacket(SPI_EEPROM_PGM, (uint8_t *)&PrepareMemBlock.MemoryBlock[0], PrepareMemBlock.MemoryBlock[1] + 2);
                }
                SysTickTime = iqrfGetSysTick();
                PrepareMemBlock.MemoryBlockProcessState--;
            }
        } else {
            if (iqrfGetSysTick() - SysTickTime >= (TICKS_IN_SECOND / 2)) {
                iqrfTrReset();
                OperationResult = IQRF_PGM_ERROR;
                WriteCodeTaskSM = PROG_END;                       // go to end programming mode
            }
        }
        break;

    case WAIT_PROG_END:     // wait until last packet is written to TR module
        if (iqrfGetSpiStatus() == PROGRAMMING_MODE && iqrfGetLibraryStatus() == IQRF_READY) {
            iqrfTrEndPgmMode();
            WriteCodeTaskSM = PROG_END;                           // go to end programming mode
        }
        break;

    case PROG_END:
        // if no packet is pending to send to TR module
        if (iqrfGetTxBufferStatus() == IQRF_BUFFER_FREE && iqrfGetLibraryStatus() == IQRF_READY) {
            WriteCodeTaskSM = INIT_TASK;
            return(OperationResult);
        }
        break;
    }

    // return TR module programming state in %
    return(((uint32_t) CodeFileInfo.FileByteCnt * 100) / CodeFileInfo.FileSize);
}

/**
 * Core programming function for user password or user key
 * @param BufferContent selects between user key or user password to be written
 * @param Buffer pointer to 16 byte buffer with user password or user key
 * @return result of partial programming operation
 */
uint8_t iqrfPgmWriteKeyOrPass(uint8_t BufferContent, uint8_t *Buffer)
{
    static enum {
        INIT_TASK = 0,
        ENTER_PROG_MODE,
        WAIT_PROG_MODE,
        WAIT_PROG_END,
        PROG_END,
    } WriteCodeTaskSM = INIT_TASK;

    static uint8_t Attempts;
    static uint8_t OperationResult;
    static uint32_t SysTickTime;

   	switch (WriteCodeTaskSM) {
    case INIT_TASK:     // initialize programming state machine
        Attempts = 1;
        if (BufferContent == IQRF_PGM_PASS_FILE_TYPE)
            PrepareMemBlock.MemoryBlock[0] = 0xD0;
        else
            PrepareMemBlock.MemoryBlock[0] = 0xD1;
        PrepareMemBlock.MemoryBlock[1] = 0x10;
        memcpy((uint8_t *)&PrepareMemBlock.MemoryBlock[2], Buffer, 0x10);
        WriteCodeTaskSM = ENTER_PROG_MODE;
        break;

    case ENTER_PROG_MODE:
        iqrfTrEnterPgmMode();
        SysTickTime = iqrfGetSysTick();
        WriteCodeTaskSM = WAIT_PROG_MODE;
        break;

    case WAIT_PROG_MODE:      // wait for TR module programming mode
        if (iqrfGetSpiStatus() == PROGRAMMING_MODE
            && iqrfGetTxBufferStatus() == IQRF_BUFFER_FREE
            && iqrfGetLibraryStatus() == IQRF_READY)
        {
            // send USER PASSWORD or USER KEY to TR module
            iqrfSendPacket(SPI_EEPROM_PGM, (uint8_t *)&PrepareMemBlock.MemoryBlock[0], PrepareMemBlock.MemoryBlock[1] + 2);
            OperationResult = IQRF_PGM_SUCCESS;
            WriteCodeTaskSM = WAIT_PROG_END;            // go to end programming mode
        } else {
            if (iqrfGetSysTick() - SysTickTime >= (TICKS_IN_SECOND / 2)) {
                // in a case, try it twice to enter programming mode
                if (Attempts) {
                    Attempts--;
                    WriteCodeTaskSM = ENTER_PROG_MODE;
                } else {
                    // TR module probably does not work
                    OperationResult = IQRF_PGM_ERROR;
                    WriteCodeTaskSM = PROG_END;
                }
            }
        }
        break;

    case WAIT_PROG_END:     // wait until last packet is written to TR module
        if (iqrfGetSpiStatus() == PROGRAMMING_MODE && iqrfGetLibraryStatus() == IQRF_READY) {
            iqrfTrEndPgmMode();
            WriteCodeTaskSM = PROG_END;                           // go to end programming mode
        }
        break;

    case PROG_END:
        // if no packet is pending to send to TR module
        if (iqrfGetTxBufferStatus() == IQRF_BUFFER_FREE && iqrfGetLibraryStatus() == IQRF_READY) {
            WriteCodeTaskSM = INIT_TASK;
            return(OperationResult);
        }
        break;
    }

    return(0);
}

/**
 * Reading and preparing a configuration data to be programmed into the TR module
 * @param none
 * @return result of config data preparing operation
 */
uint8_t iqrfPgmProcessCfgFile(void)
{
    // prepare image of 32byte configuration block to flash memory
    if (PrepareMemBlock.DataInBufferReady == 0) {
        // initialize block address
        PrepareMemBlock.MemoryBlock[0] = IQRF_CONFIG_MEM_L_ADR & 0x00FF;
        PrepareMemBlock.MemoryBlock[1] = IQRF_CONFIG_MEM_L_ADR >> 8;
        PrepareMemBlock.MemoryBlock[34] = IQRF_CONFIG_MEM_H_ADR & 0x00FF;
        PrepareMemBlock.MemoryBlock[35] = IQRF_CONFIG_MEM_H_ADR >> 8;;
        PrepareMemBlock.MemoryBlockProcessState = 2;
        // read configuration data from file
        iqrfSuspendDriver();
        for (uint8_t Cnt=0; Cnt<32; Cnt++) {
            if (Cnt < 16){
                // first half of configuration
                PrepareMemBlock.MemoryBlock[Cnt*2 + 2] = iqrfReadByteFromFile();
                PrepareMemBlock.MemoryBlock[Cnt*2 + 3] = 0x34;
            } else {
                // second half of configuration
                PrepareMemBlock.MemoryBlock[Cnt*2 + 4] = iqrfReadByteFromFile();
                PrepareMemBlock.MemoryBlock[Cnt*2 + 5] = 0x34;
            }
        }
        // store last configuration byte for next packet
        PrepareMemBlock.MemoryBlockNumber = iqrfReadByteFromFile();
        iqrfRunDriver();

        PrepareMemBlock.DataInBufferReady = 1;
        return(IQRF_PGM_FLASH_BLOCK_READY);
    } else {
        // prepare packet for RFPGM configuration
        if (PrepareMemBlock.DataInBufferReady == 1) {
            PrepareMemBlock.MemoryBlock[0] = RFPGM_CFG_ADR;
            PrepareMemBlock.MemoryBlock[1] = 0x01;
            PrepareMemBlock.MemoryBlock[2] = PrepareMemBlock.MemoryBlockNumber;
            PrepareMemBlock.MemoryBlockProcessState = 1;

            PrepareMemBlock.DataInBufferReady = 2;
            return(IQRF_PGM_EEPROM_BLOCK_READY);
        } else {
            // configuration programming successfully ended
            PrepareMemBlock.DataInBufferReady = 0;
            return(IQRF_PGM_SUCCESS);
        }
    }
}

/**
 * Move overflowed data to active block ready to programming
 */
void iqrfPgmMoveOverflowedData(void)
{
    uint16_t MemBlock;
    // move overflowed data to active block
    memcpy((uint8_t *)&PrepareMemBlock.MemoryBlock[34], (uint8_t *)&PrepareMemBlock.MemoryBlock[0], 34);
    // clear block of memory for overflowed data
    memset((uint8_t *)&PrepareMemBlock.MemoryBlock[0], 0, 34);
    // calculate the data block index
    MemBlock = ((uint16_t)PrepareMemBlock.MemoryBlock[35] << 8) | PrepareMemBlock.MemoryBlock[34];
    PrepareMemBlock.MemoryBlockNumber = MemBlock + 0x10;
    MemBlock++;
    PrepareMemBlock.MemoryBlock[0] = MemBlock & 0x00FF;         // write next block index to image
    PrepareMemBlock.MemoryBlock[1] = MemBlock >> 8;
    PrepareMemBlock.DataOverflow = 0;
    // initialize block process counter (block will be written to TR module in 1 write packet)
    PrepareMemBlock.MemoryBlockProcessState = 1;
}

/**
 * Reading and preparing a block of data to be programmed into the TR module
 * @param none
 * @return result of data preparing operation
 */
uint8_t iqrfPgmPrepareMemBlock(void)
{
    uint16_t MemBlock;
    uint8_t DataCounter;
    uint8_t DestinationIndex;
    uint8_t ValidAddress;
    uint8_t OperationResult;
    uint8_t Cnt;

    // initialize memory block for flash programming
    if (!PrepareMemBlock.DataOverflow) {
        for (Cnt=0; Cnt<sizeof(PrepareMemBlock.MemoryBlock); Cnt+=2) {
            PrepareMemBlock.MemoryBlock[Cnt] = 0xFF;
            PrepareMemBlock.MemoryBlock[Cnt+1] = 0x3F;
        }
    }
    PrepareMemBlock.MemoryBlockNumber = 0;

    while(1) {
        // if no data ready in file buffer
        if (!PrepareMemBlock.DataInBufferReady) {
            iqrfSuspendDriver();
            OperationResult = iqrfPgmReadHEXFileLine();       // read one line from HEX file
            iqrfRunDriver();
            // check result of file reading operation
            if (OperationResult == IQRF_PGM_FILE_DATA_ERROR) {
                return(IQRF_PGM_ERROR);
            } else {
                if (OperationResult == IQRF_PGM_END_OF_FILE) {
                    // if any data are ready to programm to FLASH
                    if (PrepareMemBlock.MemoryBlockNumber) {
                        return(IQRF_PGM_FLASH_BLOCK_READY);
                    } else {
                        if (PrepareMemBlock.DataOverflow) {
                            iqrfPgmMoveOverflowedData();
                            return(IQRF_PGM_FLASH_BLOCK_READY);
                        } else {
                            return(IQRF_PGM_SUCCESS);
                        }
                    }
                }
            }
            PrepareMemBlock.DataInBufferReady = 1;            // set flag, data ready in file buffer
        }

        if (IqrfPgmCodeLineBuffer[3] == 0) {                   // data block ready in file buffer
            // read destination address for data in buffer
            PrepareMemBlock.Address = (PrepareMemBlock.HiAddress
                + ((uint16_t)IqrfPgmCodeLineBuffer[1] << 8)
                + IqrfPgmCodeLineBuffer[2]) / 2;

            if (PrepareMemBlock.DataOverflow)
                iqrfPgmMoveOverflowedData();
            // data for external serial EEPROM
            if (PrepareMemBlock.Address >= SERIAL_EEPROM_MIN_ADR && PrepareMemBlock.Address <= SERIAL_EEPROM_MAX_ADR) {
                // if image of data block is not initialized
                if (PrepareMemBlock.MemoryBlockNumber == 0) {
                    MemBlock = (PrepareMemBlock.Address - 0x200) / 32;          // calculate data block index
                    memset((uint8_t *)&PrepareMemBlock.MemoryBlock[0], 0, 68);  // clear image of data block
                    PrepareMemBlock.MemoryBlock[34] = MemBlock & 0x00FF;        // write block index to image
                    PrepareMemBlock.MemoryBlock[35] = MemBlock >> 8;
                    MemBlock++;                                                 // next block index
                    PrepareMemBlock.MemoryBlock[0] = MemBlock & 0x00FF;         // write next block index to image
                    PrepareMemBlock.MemoryBlock[1] = MemBlock >> 8;
                    PrepareMemBlock.MemoryBlockNumber = PrepareMemBlock.Address / 32;   // remember actual memory block
                    // initialize block process counter (block will be written to TR module in 1 write packet)
                    PrepareMemBlock.MemoryBlockProcessState = 1;
                }

                MemBlock = PrepareMemBlock.Address / 32;                        // calculate actual memory block
                // calculate offset from start of image, where data to be written
                DestinationIndex = (PrepareMemBlock.Address % 32) + 36;
                DataCounter = IqrfPgmCodeLineBuffer[0] / 2;                     // read number of data bytes in file buffer

                // if data in file buffer are from different memory block, write actual image to TR module
                if (PrepareMemBlock.MemoryBlockNumber != MemBlock)
                    return(IQRF_PGM_FLASH_BLOCK_READY);

                // check if all data are inside the image of data block
                if (DestinationIndex + DataCounter > sizeof(PrepareMemBlock.MemoryBlock))
                    PrepareMemBlock.DataOverflow = 1;
                // copy data from file buffer to image of data block
                for (uint8_t Cnt=0; Cnt < DataCounter; Cnt++) {
                    PrepareMemBlock.MemoryBlock[DestinationIndex++] = IqrfPgmCodeLineBuffer[2*Cnt+4];
                    if (DestinationIndex == 68)
                        DestinationIndex = 2;
                }

                if (PrepareMemBlock.DataOverflow) {
                    PrepareMemBlock.DataInBufferReady = 0;                      // process next line from HEX file
                    return(IQRF_PGM_FLASH_BLOCK_READY);
                }
            } else {  // check if data in file buffer are for other memory areas
                MemBlock = PrepareMemBlock.Address / 32;                        // calculate actual memory block
                // calculate offset from start of image, where data to be written
                DestinationIndex = (PrepareMemBlock.Address % 32) * 2;
                if (DestinationIndex < 32)
                    DestinationIndex += 2;
                else
                    DestinationIndex += 4;
                DataCounter = IqrfPgmCodeLineBuffer[0];                         // read number of data bytes in file buffer
                ValidAddress = 0;

                // check if data in file buffer are for main FLASH memory area in TR module
                if (PrepareMemBlock.Address >= IQRF_MAIN_MEM_MIN_ADR
                    && PrepareMemBlock.Address <= IQRF_MAIN_MEM_MAX_ADR)
                {
                    ValidAddress = 1;                                           // set flag, data are for FLASH memory area
                    // check if all data are in main memory area
                    if ((PrepareMemBlock.Address + DataCounter/2) > IQRF_MAIN_MEM_MAX_ADR)
                        DataCounter = (IQRF_MAIN_MEM_MAX_ADR - PrepareMemBlock.Address) * 2;
                    // check if all data are inside the image of data block
                    if (DestinationIndex + DataCounter > sizeof(PrepareMemBlock.MemoryBlock))
                        return(IQRF_PGM_ERROR);
                    // if data in file buffer are from different memory block, write actual image to TR module
                    if (PrepareMemBlock.MemoryBlockNumber)
                        if (PrepareMemBlock.MemoryBlockNumber != MemBlock)
                            return(IQRF_PGM_FLASH_BLOCK_READY);
                } else {
                    // check if data in file buffer are for licensed FLASH memory area in TR module
                    if (PrepareMemBlock.Address >= IQRF_LICENCED_MEM_MIN_ADR
                        && PrepareMemBlock.Address <= IQRF_LICENCED_MEM_MAX_ADR)
                    {
                        ValidAddress = 1;                                       // set flag, data are for FLASH memory area
                        // check if all data are in licensed memory area
                        if ((PrepareMemBlock.Address + DataCounter/2) > IQRF_LICENCED_MEM_MAX_ADR)
                            DataCounter = (IQRF_LICENCED_MEM_MAX_ADR - PrepareMemBlock.Address) * 2;
                        // check if all data are inside the image of data block
                        if (DestinationIndex + DataCounter > sizeof(PrepareMemBlock.MemoryBlock))
                            return(IQRF_PGM_ERROR);
                        // if data in file buffer are from different memory block, write actual image to TR module
                        if (PrepareMemBlock.MemoryBlockNumber)
                            if (PrepareMemBlock.MemoryBlockNumber != MemBlock)
                                return(IQRF_PGM_FLASH_BLOCK_READY);
                    } else {
                        // check if data in file buffer are for internal EEPROM of TR module
                        if (PrepareMemBlock.Address >= PIC16LF1938_EEPROM_MIN
                            && PrepareMemBlock.Address <= PIC16LF1938_EEPROM_MAX)
                        {
                            // if image of data block contains any data, write it to TR module
                            if (PrepareMemBlock.MemoryBlockNumber)
                                return(IQRF_PGM_FLASH_BLOCK_READY);
                            // prepare image of data block for internal EEPROM
                            PrepareMemBlock.MemoryBlock[0] = PrepareMemBlock.Address & 0x00FF;
                            PrepareMemBlock.MemoryBlock[1] = DataCounter / 2;
                            if (PrepareMemBlock.Address + PrepareMemBlock.MemoryBlock[1] > PIC16LF1938_EEPROM_MAX
                                || PrepareMemBlock.MemoryBlock[1] > 32)
                            {
                                return(IQRF_PGM_ERROR);
                            }
                            for (uint8_t Cnt=0; Cnt < PrepareMemBlock.MemoryBlock[1]; Cnt++)
                                PrepareMemBlock.MemoryBlock[Cnt+2] = IqrfPgmCodeLineBuffer[2*Cnt+4];
                            PrepareMemBlock.DataInBufferReady = 0;
                            // initialize block process counter (block will be written to TR module in 1 write packet)
                            PrepareMemBlock.MemoryBlockProcessState = 1;
                            return(IQRF_PGM_EEPROM_BLOCK_READY);
                        }
                    }
                }
                // if destination address is from FLASH memory area
                if (ValidAddress) {
                    // remember actual memory block
                    PrepareMemBlock.MemoryBlockNumber = MemBlock;
                    // initialize block process counter (block will be written to TR module in 2 write packets)
                    PrepareMemBlock.MemoryBlockProcessState = 2;
                    // compute and write destination address of first half of image
                    MemBlock *= 32;
                    PrepareMemBlock.MemoryBlock[0] = MemBlock & 0x00FF;
                    PrepareMemBlock.MemoryBlock[1] = MemBlock >> 8;
                    // compute and write destination address of second half of image
                    MemBlock += 0x0010;
                    PrepareMemBlock.MemoryBlock[34] = MemBlock & 0x00FF;
                    PrepareMemBlock.MemoryBlock[35] = MemBlock >> 8;
                    // copy data from file buffer to image of data block
                    memcpy(&PrepareMemBlock.MemoryBlock[DestinationIndex], &IqrfPgmCodeLineBuffer[4], DataCounter);
                }
            }
        } else {
            if (IqrfPgmCodeLineBuffer[3] == 4)                                  // in file buffer is address info
                PrepareMemBlock.HiAddress = ((uint32_t)IqrfPgmCodeLineBuffer[4] << 24) + ((uint32_t)IqrfPgmCodeLineBuffer[5] << 16);
        }
        PrepareMemBlock.DataInBufferReady = 0;                                  // process next line from HEX file
    }
}


/**
 * Convert two ASCII chars to number
 * @param dataByteHi High nibble in ASCII
 * @param dataByteLo Low nibble in ASCII
 * @return Number
 */
uint8_t iqrfPgmConvertToNum(uint8_t dataByteHi, uint8_t dataByteLo)
{
    uint8_t result = 0;

    /* convert High nibble */
    if (dataByteHi >= '0' && dataByteHi <= '9')
        result = (dataByteHi - '0') << 4;
    else if (dataByteHi >= 'a' && dataByteHi <= 'f')
        result = (dataByteHi - 87) << 4;

    /* convert Low nibble */
    if (dataByteLo >= '0' && dataByteLo <= '9')
        result |= (dataByteLo - '0');
    else if (dataByteLo >= 'a' && dataByteLo <= 'f')
        result |= (dataByteLo - 87);

    return(result);
}


/**
 * Read and process line from plugin file
 * @return Return code (IQRF_PGM_FILE_DATA_READY - iqrf file line ready, IQRF_PGM_FILE_DATA_READY - input file format error, IQRF_PGM_END_OF_FILE - end of file)
 */
uint8_t iqrfPgmReadIQRFFileLine(void)
{
    uint8_t FirstChar;
    uint8_t SecondChar;
    uint8_t CodeLineBufferPtr = 0;

repeat_read:
    // read one char from file
    FirstChar = tolower(iqrfReadByteFromFile());

    // read one char from file
    if (FirstChar == '#') {
        // read data to end of line
        while (((FirstChar = iqrfReadByteFromFile()) != 0) && (FirstChar != 0x0D))
            ; /* void */
    }

    // if end of line
    if (FirstChar == 0x0D) {
        // read second code 0x0A
        iqrfReadByteFromFile();
        if (CodeLineBufferPtr == 0)
            // read another line
            goto repeat_read;
        if (CodeLineBufferPtr == 20)
            // line with data read successfully
            return(IQRF_PGM_FILE_DATA_READY);
        else
            // wrong file format (error)
            return(IQRF_PGM_FILE_DATA_ERROR);
    }

    // if end of file
    if (FirstChar == 0)
        return(IQRF_PGM_END_OF_FILE);

    // read second character from code file
    SecondChar = tolower(iqrfReadByteFromFile());
    if (CodeLineBufferPtr >= 20)
        return(IQRF_PGM_FILE_DATA_ERROR);
    // convert chars to number and store to buffer
    IqrfPgmCodeLineBuffer[CodeLineBufferPtr++] = iqrfPgmConvertToNum(FirstChar, SecondChar);
    // read next data
    goto repeat_read;
}

/**
 * Read and process line from HEX file
 * @return Return code (IQRF_PGM_FILE_DATA_READY - iqrf file line ready, IQRF_PGM_FILE_DATA_READY - input file format error, IQRF_PGM_END_OF_FILE - end of file)
 */
uint8_t iqrfPgmReadHEXFileLine(void)
{
    uint8_t Sign;
    uint8_t DataByteHi, DataByteLo;
    uint8_t DataByte;
    uint8_t CodeLineBufferPtr = 0;
    uint8_t CodeLineBufferCrc = 0;

    // find start of line or end of file
    while (((Sign = iqrfReadByteFromFile()) != 0) && (Sign != ':'))
        ; /* void */
    // if end of file
    if (Sign == 0)
        return(IQRF_PGM_END_OF_FILE);

    // read data to end of line and convert if to numbers
    for ( ; ; ) {
        // read High nibble
        DataByteHi = tolower(iqrfReadByteFromFile());
        // check end of line
        if (DataByteHi == 0x0A || DataByteHi == 0x0D) {
            if (CodeLineBufferCrc != 0)
                // check line CRC
                return(IQRF_PGM_FILE_DATA_ERROR);
            // stop reading
            return(IQRF_PGM_FILE_DATA_READY);
        }
        // read Low nibble
        DataByteLo = tolower(iqrfReadByteFromFile());
        // convert two ASCII to number
        DataByte = iqrfPgmConvertToNum(DataByteHi, DataByteLo);
        // add to CRC
        CodeLineBufferCrc += DataByte;
        // store to line buffer
        IqrfPgmCodeLineBuffer[CodeLineBufferPtr++] = DataByte;
        if (CodeLineBufferPtr >= SIZE_OF_CODE_LINE_BUFFER)
            return (IQRF_PGM_FILE_DATA_ERROR);
    }
}
