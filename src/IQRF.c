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
#include "IQRF.h"

#define IQRF_PKT_SIZE             68

typedef struct { // SPI interface control structure
    volatile uint8_t SpiStat;
    uint8_t DLEN;
    uint8_t CMD;
    uint8_t PTYPE;
    uint8_t CRCM;
    uint8_t MyCRCS;
    uint8_t CRCS;
    uint8_t PacketLen;
    uint8_t PacketCnt;
    uint8_t PacketRpt;
    uint8_t PacketTxBuffer[IQRF_PKT_SIZE];
    uint8_t PacketRxBuffer[IQRF_PKT_SIZE];
} T_IQRF_SPI_CONTROL;

typedef struct {
    uint8_t BufferFlag;
    uint8_t SpiCmd;
    uint8_t *DataBuffer;
    uint8_t DataLength;
} T_IQRF_PACKET;

#define SPI_STATUS_POOLING_TIME   10        // SPI status pooling time 10ms

#define IQRF_SM_PREPARE_REQUEST       0     // internal states of IQRF operation state machine
#define IQRF_SM_SEND_REQUEST          1
#define IQRF_SM_PROCESS_REQUEST       2
#define IQRF_SM_REQUEST_OK            3
#define IQRF_SM_REQUEST_ERR           4

#define IQRF_READY                    0x00  // IQRF support library ready
#define IQRF_READ                     0x01  // IQRF read request processing
#define IQRF_WRITE                    0x02  // IQRF write request processing

/* Function prototypes */
void iqrfSpiDriver(void);
uint8_t iqrfCrcCalculate(uint8_t *Buffer, uint8_t DataLength);
bool iqrfCrcCheck(uint8_t *Buffer, uint8_t DataLength, uint8_t Ptype);
void iqrfTrInfoTask(void);
void iqrfTrInfoProcess(uint8_t *DataBuffer, uint8_t DataSize);

/* Public variable declarations */
T_IQRF_SPI_CONTROL IqrfSpiControl;
volatile T_IQRF_PACKET IqrfPacket;
T_TR_INFO_STRUCT	IqrfTrInfoStruct;

volatile uint8_t IqrfDataSenderSM = IQRF_SM_PREPARE_REQUEST;
volatile uint8_t IqrfTrInfoReading;

/**
 * Function initialize IQRF SPI support library
 * @param UserIqrfRxHandler Pointer to user call back function for received packets service
 */
void iqrfInit(T_IQRF_RX_HANDLER UserIqrfRxHandler)
{
    IqrfControl.SuspendFlag = false;                    //  initialize library variables
    IqrfControl.Status = IQRF_READY;
    IqrfControl.FastSPI = false;
    IqrfControl.TimeCnt = SPI_STATUS_POOLING_TIME;
    IqrfControl.IqrfRxHandler = iqrfTrInfoProcess;
    IqrfSpiControl.SpiStat = SPI_DISABLED;

    iqrfTrPowerOn();                                     // turn power on for TR module

    iqrfKernelTimingInit();                              // initialize IQRF SPI kernel timing

    // read TR module info
    IqrfTrInfoReading = 4;
    while(IqrfTrInfoReading)
        iqrfTrInfoTask();

    // if connected TR module supports fast SPI mode
    if (iqrfGetModuleType() == TR_72D || iqrfGetModuleType() == TR_76D)
        iqrfKernelTimingFastMode();                     // switch to fast SPI mode

    IqrfControl.IqrfRxHandler = UserIqrfRxHandler;      //  set user RX handler
}

/**
 * Function provides background communication with TR module
 */
void iqrfDriver(void)
{
    if (IqrfControl.SuspendFlag)
        return;

    if (IqrfControl.Status == IQRF_READ || IqrfControl.Status == IQRF_WRITE || !IqrfControl.TimeCnt) {
        iqrfSpiDriver();
        if (IqrfControl.FastSPI == true)
            IqrfControl.TimeCnt = (SPI_STATUS_POOLING_TIME * 5) + 1;
        else
            IqrfControl.TimeCnt = SPI_STATUS_POOLING_TIME + 1;
    }
    IqrfControl.TimeCnt--;
}

/**
 * Sends IQRF data packet to TR module
 * @param DataBuffer Pointer to buffer with IQRF SPI packet
 * @param DataSize size of IQRF SPI packet
 * @return Operation result (IQRF_OPERATION_OK, IQRF_OPERATION_IN_PROGRESS, IQRF_TR_MODULE_WRITE_ERR ... )
 */
uint8_t iqrfSendData(uint8_t *DataBuffer, uint8_t DataLength)
{
    uint8_t OperationResult;
    uint8_t TempSize = DataLength;

    // IQRF operation state machine
    switch (IqrfDataSenderSM) {
    case IQRF_SM_PREPARE_REQUEST:
        if (IqrfSpiControl.SpiStat == SPI_DATA_TRANSFER)
            return (IQRF_OPERATION_IN_PROGRESS);
        if (IqrfSpiControl.SpiStat != COMMUNICATION_MODE)
            return(IQRF_TR_MODULE_NOT_READY);
        if (DataLength == 0 || DataLength > 64)
            return(IQRF_WRONG_DATA_SIZE);
        IqrfDataSenderSM = IQRF_SM_SEND_REQUEST;
        return (IQRF_OPERATION_IN_PROGRESS);
        break;

    // process IQRF write request
    case IQRF_SM_SEND_REQUEST:
        // send IQRF write request
        iqrfSendPacket(SPI_WR_RD, DataBuffer, TempSize);
        // next SM SPI_STATUS_POOLING_TIME
        IqrfDataSenderSM = IQRF_SM_PROCESS_REQUEST;
        return (IQRF_OPERATION_IN_PROGRESS);
        break;

    // process IQRF write request
    case IQRF_SM_PROCESS_REQUEST:
        return (IQRF_OPERATION_IN_PROGRESS);
        break;

    // IQRF write request OK
    case IQRF_SM_REQUEST_OK:
        OperationResult = IQRF_OPERATION_OK;
        break;

    // IQRF write request ERR
    case IQRF_SM_REQUEST_ERR:
        OperationResult = IQRF_TR_MODULE_WRITE_ERR;
        break;
    }

    IqrfDataSenderSM = IQRF_SM_PREPARE_REQUEST;

    return(OperationResult);
}


/**
 * Sends IQRF packet with specific SPI command to TR module
 * @param SpiCmd SPI command for TR module
 * @param UserDataBuffer Pointer to buffer with IQRF SPI packet
 * @param UserDataLength size of IQRF SPI packet
 */
void iqrfSendPacket(uint8_t SpiCmd, uint8_t *UserDataBuffer, uint8_t UserDataLength)
{
    IqrfPacket.SpiCmd = SpiCmd;
    IqrfPacket.DataBuffer = UserDataBuffer;
    IqrfPacket.DataLength = UserDataLength;
    IqrfPacket.BufferFlag = IQRF_BUFFER_BUSY;
}

/**
 * Temporary suspend IQRF communication driver
 */
void iqrfSuspendDriver(void)
{
    // wait until library is ready
    while (IqrfControl.Status == IQRF_READ || IqrfControl.Status == IQRF_WRITE)
        ; /* void */
    // set driver suspend flag
    IqrfControl.SuspendFlag = true;
    // set SPI status
    IqrfSpiControl.SpiStat = SPI_DISABLED;
}

/**
 * Run IQRF communication driver
 */
void iqrfRunDriver(void)
{
    // re-enable IQRF driver running
    IqrfControl.SuspendFlag = false;
}


/**
 * reset TR module
 */
void iqrfTrReset(void)
{
    iqrfTrPowerOff();
    iqrfDelayMs(100);
    iqrfTrPowerOn();
    iqrfDelayMs(1);
}


/**
 * end of programming mode
 */
void iqrfTrEndPgmMode(void)
{
    iqrfTrReset();
    iqrfDelayMs(200);
}


/**
 * get SPI status of TR module
 * @return SPI status of TR module
 */
uint8_t iqrfGetSpiStatus(void)
{
    return(IqrfSpiControl.SpiStat);
}


/**
 * get status of IQRF SPI support library
 * @return status of support library
 */
uint8_t iqrfGetLibraryStatus(void)
{
    return(IqrfControl.Status);
}


/**
 * get status of TX buffer of IQRF SPI support library
 * @return status of TX buffer
 */
uint8_t iqrfGetTxBufferStatus(void)
{
    return(IqrfPacket.BufferFlag);
}


/**
 * Function implements IQRF packet communication over SPI with TR module
 */
void iqrfSpiDriver(void)
{
    // is anything to send / receive
    if (IqrfControl.Status != IQRF_READY) {
        IqrfSpiControl.PacketRxBuffer[IqrfSpiControl.PacketCnt] = iqrfSendSpiByte(IqrfSpiControl.PacketTxBuffer[IqrfSpiControl.PacketCnt]);
        IqrfSpiControl.PacketCnt++;

        if (IqrfSpiControl.PacketCnt==IqrfSpiControl.PacketLen || IqrfSpiControl.PacketCnt==IQRF_PKT_SIZE) {
            iqrfDeselectTRmodule();
            if ((IqrfSpiControl.PacketRxBuffer[IqrfSpiControl.DLEN + 3] == SPI_CRCM_OK)
                && iqrfCrcCheck(IqrfSpiControl.PacketRxBuffer, IqrfSpiControl.DLEN, IqrfSpiControl.PTYPE))
            {
                if (IqrfControl.Status == IQRF_READ)
                    IqrfControl.IqrfRxHandler(&IqrfSpiControl.PacketRxBuffer[2], IqrfSpiControl.DLEN);
                if (IqrfControl.Status==IQRF_WRITE && IqrfDataSenderSM==IQRF_SM_PROCESS_REQUEST)
                    IqrfDataSenderSM = IQRF_SM_REQUEST_OK;
                IqrfControl.Status = IQRF_READY;
            } else {
                if (--IqrfSpiControl.PacketRpt) {
                    IqrfSpiControl.PacketCnt = 0;
                } else {
                    if (IqrfControl.Status == IQRF_WRITE && IqrfDataSenderSM == IQRF_SM_PROCESS_REQUEST)
                        IqrfDataSenderSM = IQRF_SM_REQUEST_ERR;
                    IqrfControl.Status = IQRF_READY;
                }
            }
        }
    } else { // no data to send => SPI status will be updated
        // get SPI status of TR module
        IqrfSpiControl.SpiStat = iqrfSendSpiByte(SPI_CHECK);
        iqrfDeselectTRmodule();

        // if the status is data ready, prepare packet to read it
        if ((IqrfSpiControl.SpiStat & 0xC0) == 0x40) {
            // clear TX buffer
            memset(IqrfSpiControl.PacketTxBuffer, 0, sizeof(IqrfSpiControl.PacketTxBuffer));
            // state 0x40 is 64B ready in TR module
            if (IqrfSpiControl.SpiStat == 0x40)
                IqrfSpiControl.DLEN = 64;
            else
                // clear bit 7,6 - rest is length (1 to 63B)
                IqrfSpiControl.DLEN = IqrfSpiControl.SpiStat & 0x3F;

            IqrfSpiControl.PTYPE = IqrfSpiControl.DLEN;
            IqrfSpiControl.PacketTxBuffer[0] = SPI_WR_RD;
            IqrfSpiControl.PacketTxBuffer[1] = IqrfSpiControl.PTYPE;
            IqrfSpiControl.PacketTxBuffer[IqrfSpiControl.DLEN + 2] =
                iqrfCrcCalculate(IqrfSpiControl.PacketTxBuffer, IqrfSpiControl.DLEN);
            // length of whole packet + (CMD, PTYPE, CRCM, 0)
            IqrfSpiControl.PacketLen = IqrfSpiControl.DLEN + 4;
            // counter of sent bytes
            IqrfSpiControl.PacketCnt = 0;
            // number of attempts to send data
            IqrfSpiControl.PacketRpt = 1;
            // current SPI status must be updated
            IqrfSpiControl.SpiStat = SPI_DATA_TRANSFER;
            // reading from buffer COM of TR module
            IqrfControl.Status = IQRF_READ;
            return;
        }

        // check if packet to send is ready
        if (IqrfPacket.BufferFlag == IQRF_BUFFER_BUSY) {
            // clear TX buffer
            memset(IqrfSpiControl.PacketTxBuffer, 0, sizeof(IqrfSpiControl.PacketTxBuffer));
            IqrfSpiControl.DLEN = IqrfPacket.DataLength;
            IqrfSpiControl.PTYPE = IqrfSpiControl.DLEN | 0x80;
            IqrfSpiControl.PacketTxBuffer[0] = IqrfPacket.SpiCmd;

            // writing to buffer COM of TR module
            IqrfControl.Status = IQRF_WRITE;

            if (IqrfSpiControl.PacketTxBuffer[0] == SPI_MODULE_INFO
                && (IqrfSpiControl.DLEN == 16 || IqrfSpiControl.DLEN == 32))
            {
                IqrfSpiControl.PTYPE &= 0x7F;
                IqrfControl.Status = IQRF_READ;
            }

            IqrfSpiControl.PacketTxBuffer[1] = IqrfSpiControl.PTYPE;
            memcpy (&IqrfSpiControl.PacketTxBuffer[2], IqrfPacket.DataBuffer, IqrfSpiControl.DLEN);

            IqrfSpiControl.PacketTxBuffer[IqrfSpiControl.DLEN + 2] =
                iqrfCrcCalculate(IqrfSpiControl.PacketTxBuffer, IqrfSpiControl.DLEN);
            // length of whole packet + (CMD, PTYPE, CRCM, 0)
            IqrfSpiControl.PacketLen = IqrfSpiControl.DLEN + 4;
            // counter of sent bytes
            IqrfSpiControl.PacketCnt = 0;
            // number of attempts to send data
            IqrfSpiControl.PacketRpt = 3;
            // current SPI status must be updated
            IqrfSpiControl.SpiStat = SPI_DATA_TRANSFER;

            IqrfPacket.BufferFlag = IQRF_BUFFER_FREE;
        }
    }
}


/**
 * Calculate CRC before master's send
 * @param Buffer SPI Tx buffer
 * @param DataLength Data length
 * @return CRC
 */
uint8_t iqrfCrcCalculate(uint8_t *Buffer, uint8_t DataLength)
{
    uint8_t Crc = 0x5F;
    for (uint8_t I = 0; I < (DataLength + 2); I++)
        Crc ^= Buffer[I];
    return (Crc);
}


/**
 * Confirm CRC from SPI slave upon received data
 * @param Buffer SPI Rx buffer
 * @param DataLength Data length
 * @param type Ptype
 * @return boolean
 */
bool iqrfCrcCheck(uint8_t *Buffer, uint8_t DataLength, uint8_t Ptype)
{
    uint8_t Crc = 0x5F ^ Ptype;
    for (uint8_t I = 2; I < (DataLength + 2); I++)
        Crc ^= Buffer[I];
    if (Buffer[DataLength + 2] == Crc)
        // CRCS OK
        return (true);
    else
        // CRCS error
        return (false);
}


/**
 * Read Module Info from TR module, uses SPI master implementation
 */
void iqrfTrInfoTask(void)
{
    static uint8_t DataToModule[32];
    static uint8_t Attempts;
    static uint32_t SysTickTime;

    static enum {
        INIT_TASK = 0,
        ENTER_PROG_MODE,
        SEND_REQUEST,
        WAIT_INFO,
        DONE
    } TrInfoTaskSM = INIT_TASK;

    switch (TrInfoTaskSM) {
    case INIT_TASK:
        // try enter to programming mode
        Attempts = 1;
        IqrfTrInfoStruct.McuType = MCU_UNKNOWN;
        memset(&DataToModule[0], 0, 32);
        // next state - will read info in PGM mode
        TrInfoTaskSM = ENTER_PROG_MODE;
        break;

    case ENTER_PROG_MODE:
        iqrfTrEnterPgmMode();
        SysTickTime = iqrfGetSysTick();
        TrInfoTaskSM = SEND_REQUEST;
        break;

    case SEND_REQUEST:
        if (iqrfGetSpiStatus() == PROGRAMMING_MODE && iqrfGetLibraryStatus() == IQRF_READY) {
            if (IqrfTrInfoReading == 4)
                // request for basic TR module info
                iqrfSendPacket(SPI_MODULE_INFO, &DataToModule[0], 1);
            else
                // request for extended TR module info
                iqrfSendPacket(SPI_MODULE_INFO, &DataToModule[0], 32);
            // initialize timeout timer
            SysTickTime = iqrfGetSysTick();
            TrInfoTaskSM = WAIT_INFO;
        } else {
            if (iqrfGetSysTick() - SysTickTime >= (TICKS_IN_SECOND / 2)) {
                // in a case, try it twice to enter programming mode
                if (Attempts) {
                    Attempts--;
                    TrInfoTaskSM = ENTER_PROG_MODE;
                } else {
                    // TR module probably does not work
                    TrInfoTaskSM = DONE;
                }
            }
        }
        break;

    // wait for info data from TR module
    case WAIT_INFO:
        if ((IqrfTrInfoReading == 2) || (IqrfTrInfoReading == 1) || (iqrfGetSysTick() - SysTickTime >= (TICKS_IN_SECOND / 2))) {
            if (IqrfTrInfoReading == 2) {
                IqrfTrInfoReading = 3;
                // initialize timeout timer
                SysTickTime = iqrfGetSysTick();
                // next state - read extended edentification info
                TrInfoTaskSM = SEND_REQUEST;
            } else {
                // send end of PGM mode packet
                iqrfTrEndPgmMode();
                // next state
                TrInfoTaskSM = DONE;
            }
        }
        break;

    // the task is finished
    case DONE:
        // if no packet is pending to send to TR module
        if (iqrfGetTxBufferStatus() == IQRF_BUFFER_FREE && iqrfGetLibraryStatus() == IQRF_READY)
            IqrfTrInfoReading = 0;
        break;
    }
}


/**
 * Process identification data of TR module
 * @param DataBuffer pointer to buffer with RAW identification data of TR module
 * @param DataSize size of identification data block
 */
void iqrfTrInfoProcess(uint8_t *DataBuffer, uint8_t DataSize)
{
    if (IqrfTrInfoReading == 4) {   // process basic TR module info
        memcpy((uint8_t *) &IqrfTrInfoStruct.ModuleInfoRawData, DataBuffer, sizeof (IqrfTrInfoStruct.ModuleInfoRawData));
        IqrfTrInfoStruct.ModuleId = (uint32_t)DataBuffer[0] << 24
            | (uint32_t)DataBuffer[1] << 16
            | (uint32_t)DataBuffer[2] << 8
            | DataBuffer[3];
        IqrfTrInfoStruct.OsVersion = (uint16_t)(DataBuffer[4] / 16) << 8 | (DataBuffer[4] % 16);
        IqrfTrInfoStruct.McuType = DataBuffer[5] & 0x07;
        IqrfTrInfoStruct.Fcc = (DataBuffer[5] & 0x08) >> 3;
        IqrfTrInfoStruct.ModuleType = DataBuffer[5] >> 4;
        IqrfTrInfoStruct.OsBuild = (uint16_t)DataBuffer[7] << 8 | DataBuffer[6];

        if (((IqrfTrInfoStruct.OsVersion >> 8) > 4)
            || (((IqrfTrInfoStruct.OsVersion >> 8) == 4) && ((IqrfTrInfoStruct.OsVersion & 0x00FF) >= 3)))
        {
            IqrfTrInfoReading = 2;    // read extended identification info
        } else {
            IqrfTrInfoReading = 1;    // end
        }
    } else {  // process extended TR module info
        // copy IBK data to TR module info structure
        memcpy((uint8_t *) &IqrfTrInfoStruct.Ibk[0], DataBuffer+16, 16);
        IqrfTrInfoReading = 1;        // end
    }
}
