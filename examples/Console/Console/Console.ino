
/**
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
#include <stddef.h>
#include <stdint.h>
#include <SPI.h>
#include <SD.h>

#include "ccp.h"
#include "console.h"
#include <IQRF.h>
#include <IQRFPgm.h>

#define SD_SS           4           // SD card chip select pin

/*
 * C prototypes
 */

/*
 * C++ prototypes
 */
 void myIqrfRxFunc(uint8_t *DataBuffer, uint8_t DataSize);

/*
 * Variables
 */
uint8_t   SDCardReady;
uint8_t   UserBuffer[16];

extern File CodeFile;

/**
 * Setup peripherals
 */
void setup()
{
    Serial.begin(9600);
    while (!Serial)
        ;   /* void */

    SDCardReady = SD.begin(SD_SS);          // initialize SD card
    SPI.end();                              // end SPI peripheral initialized by SD card library

    SPI.begin();                            // start SPI peripheral for IQRF library
    iqrfInit(myIqrfRxFunc);                 // initialize IQRF library

    Serial.println();
}

/**
 * Main loop
 */
void loop()
{
    // console command processor
    ccp();
}


/**
 * Converts the upper nibble of a binary value to a hexadecimal ASCII byte
 *
 * @param - B binary value to conversion
 * @return - ASCII char converted from upper nibble
 *
 */
char bToHexa_high(uint8_t B)
{
    B >>= 4;
    return ((B > 0x9) ? B+'A'-10 : B+'0');
}

/**
 * Converts the lower nibble of a binary value to a hexadecimal ASCII byte.
 *
 * @param - B binary value to conversion
 * @return - ASCII char converted from lower nibble
 *
 */
char bToHexa_low(uint8_t B)
{
    B &= 0x0F;
    return ((B > 0x9) ? B+'A'-10 : B+'0');
}


/**
 * function called by library after successfully packet receiving
 *
 * @param - DataBuffer pointer to buffer with received data from TR module
 * @param - DataSize size of received SPI packet
 * @return - none
 *
 */
void myIqrfRxFunc(uint8_t *DataBuffer, uint8_t DataSize)
{
    Serial.println(CrLf);
    sysMsgPrinter(CCP_RECEIVED_DATA);
    for (uint8_t Cnt=0; Cnt<DataSize; Cnt++) {
        Serial.print(DataBuffer[Cnt], HEX);
        Serial.print(" ");
    }
    Serial.println();
    for (uint8_t Cnt=0; Cnt<DataSize; Cnt++) {
        if (DataBuffer[Cnt]<32 || DataBuffer[Cnt]>127)
            Serial.write('.');
        else
            Serial.write(DataBuffer[Cnt]);
    }
    Serial.println(CrLf);
    Serial.print(CmdPrompt);                                // print prompt
}


/**
 * Send SPI packet to TR module
 * @param DataBuff Pointer to buffer with SPI packet
 * @param DataSize Size of packet
 * @return Operation result
 */
uint8_t sendMyIqrfPacket(uint8_t *DataBuff, uint8_t DataSize)
{
    uint8_t Message;
    uint8_t OpResult;

    // "Sending data" message
    sysMsgPrinter(CCP_SENDING_DATA);
    // send data and wait for result
    while((OpResult = iqrfSendData(DataBuff, DataSize)) == IQRF_OPERATION_IN_PROGRESS)
        ;   /* void */

    switch(OpResult) {
    case IQRF_OPERATION_OK:                         // data sent OK
        Message = CCP_DATA_SETNT_OK;
        break;
    case IQRF_TR_MODULE_WRITE_ERR:                  // data sending error
        Message = CCP_DATA_SENT_ERR;
        break;
    case IQRF_TR_MODULE_NOT_READY:                  // TR module not ready
        Message = CCP_TR_NOT_READY;
        break;
    case IQRF_WRONG_DATA_SIZE:                      // Data size error
        Message = CCP_BAD_PARAMETER;
        break;
    }
    sysMsgPrinter(Message);

    return(OpResult);
}


const char OSModuleType[] PROGMEM = {"Module type    : "};
const char OSModuleMCU[] PROGMEM = {"Module MCU     : "};
const char OSModuleId[] PROGMEM = {"Module ID      : "};
const char OSModuleOsVer[] PROGMEM = {"OS version     : "};
const char OSModuleFCC[] PROGMEM = {"FCC certification: "};
const char OSModuleIbk[] PROGMEM = {"Module IBK     : "};
/**
 * Print results of trinfo command
 * @param infoBuffer buffer with TR module and OS information data
 * @return none
 */
void ccpTrModuleInfo(uint16_t CommandParameter)
{
    uint8_t Ptr, I;
    char TempString[32];

    // decode and print module type
    Serial.print(CrLf);
    strcpy_P(TempString, OSModuleType);
    Serial.print(TempString);

    if (iqrfGetMcuType() == MCU_UNKNOWN) {
        Serial.print("UNKNOWN");
    } else {
        Ptr=0;
        if (iqrfGetModuleId() & 0x80000000L) {
            TempString[Ptr++] = 'D';
            TempString[Ptr++] = 'C';
        }
        TempString[Ptr++] = 'T';
        TempString[Ptr++] = 'R';
        TempString[Ptr++] = '-';

        TempString[Ptr++] = '5';

        switch(iqrfGetModuleType()) {
        case TR_52D:
            TempString[Ptr++] = '2';
            break;
        case TR_58D_RJ:
            TempString[Ptr++] = '8';
            break;
        case TR_72D:
            TempString[Ptr-1] = '7';
            TempString[Ptr++] = '2';
            break;
        case TR_53D:
            TempString[Ptr++] = '3';
            break;
        case TR_54D:
            TempString[Ptr++] = '4';
            break;
        case TR_55D:
            TempString[Ptr++] = '5';
            break;
        case TR_56D:
            TempString[Ptr++] = '6';
            break;
        case TR_76D:
            TempString[Ptr-1] = '7';
            TempString[Ptr++] = '6';
            break;
        default :
            TempString[Ptr++] = 'x';
            break;
        }

        if (iqrfGetMcuType() == PIC16LF1938)
            TempString[Ptr++] = 'D';
        TempString[Ptr++] = 'x';
        TempString[Ptr++] = 0;
        Serial.println(TempString);

        // print module MCU
        strcpy_P(TempString, OSModuleType);
        Serial.print(TempString);
        switch (iqrfGetMcuType()) {
        case PIC16LF819:
            Serial.println("PIC16LF819");
            break;
        case PIC16LF88:
            Serial.println("PIC16LF88");
            break;
        case PIC16F886:
            Serial.println("PIC16F886");
            break;
        case PIC16LF1938:
            Serial.println("PIC16LF1938");
            break;
        }

        // print module ID
        strcpy_P(TempString, OSModuleId);
        Serial.print(TempString);
        Serial.println(iqrfGetModuleId(), HEX);

        // print module IBK
        strcpy_P(TempString, OSModuleIbk);
        Serial.print(TempString);
        I = iqrfGetOsVersion() >> 8;
        // if OS version is 4.03 and more, print IBK
        if ((I > 4) || ((I == 4) && ((iqrfGetOsVersion() & 0x00FF) >= 3))) {
            for (I=0; I<16; I++) {
                TempString[0] = bToHexa_high(iqrfGetModuleIbk(I));
                TempString[1] = bToHexa_low(iqrfGetModuleIbk(I));
                TempString[2] = 0x20;
                TempString[3] = 0x00;
                Serial.print(TempString);
                if (I == 15)
                    Serial.println();
            }
        } else {
            Serial.println("---");
        }

        // print OS version string
        strcpy_P(TempString, OSModuleOsVer);
        Serial.print(TempString);
        Ptr = 0;

        // major version
        TempString[Ptr++] = bToHexa_low(iqrfGetOsVersion() >> 8);
        TempString[Ptr++] = '.';

        // minor version
        I = iqrfGetOsVersion() & 0x00FF;

        if (I < 10) {
            TempString[Ptr++] = '0';
            TempString[Ptr++] = I + '0';
        } else {
            TempString[Ptr++] = '1';
            TempString[Ptr++] = I - 10 + '0';
        }

        if(iqrfGetMcuType() == PIC16LF1938)
            TempString[Ptr++] = 'D';

        // OS build
        TempString[Ptr++] = ' ';
        TempString[Ptr++] = '(';
        TempString[Ptr++] = bToHexa_high(iqrfGetOsBuild() >> 8);
        TempString[Ptr++] = bToHexa_low(iqrfGetOsBuild() >> 8);
        TempString[Ptr++] = bToHexa_high(iqrfGetOsBuild() & 0x00FF);
        TempString[Ptr++] = bToHexa_low(iqrfGetOsBuild() & 0x00FF);
        TempString[Ptr++] = ')';
        TempString[Ptr] = 0;
        Serial.println(TempString);

        strcpy_P(TempString, OSModuleFCC);
        Serial.print(TempString);
        Serial.println(iqrfGetFccStatus() ? "YES" : "NO");
    }

    Serial.println();
}


/**
 * send string to TR module. String is parameter of send command
 * @param CommandParameter parameter from CCP command table
 * @return none
 */
void ccpTrSendData(uint16_t CommandParameter)
{
    uint8_t CcpStringSize;

    CcpStringSize = ccpReadString(CcpCommandParameter);
    sendMyIqrfPacket((uint8_t *)CcpCommandParameter, CcpStringSize);
}


const char ProgressBar[] PROGMEM = {'-','-','-','-','-','-','-','-','-','-',0x0D,0x00};
/**
 * Programm IQRF / HEX /TRCNFG  file to TR module
 * @param CommandParameter parameter from CCP command table
 * @return none
 */
void ccpPgmFile(uint16_t CommandParameter)
{
    uint8_t Message = 0;
    uint8_t TempVariable;
    uint8_t StoreProgress;
    uint8_t Cnt;
    char Filename[26];

    // if SD card is not ready print error msg
    if (!SDCardReady) {
        sysMsgPrinter(CCP_SD_CARD_ERR);
    } else {
        // read file type
        if (ccpFindCmdParameter(CcpCommandParameter)) {
            // check type of file
            if (strcmp("iqrf",CcpCommandParameter) == 0)
                // set PLUGIN filetype
                CodeFileInfo.FileType = IQRF_PGM_PLUGIN_FILE_TYPE;
            else if (strcmp("hex",CcpCommandParameter) == 0)
                // set HEX filetype
                CodeFileInfo.FileType = IQRF_PGM_HEX_FILE_TYPE;
            else if (strcmp("trcnfg",CcpCommandParameter) == 0)
                // set TRCNFG filetype
                CodeFileInfo.FileType = IQRF_PGM_CFG_FILE_TYPE;
            else if (strcmp("pass",CcpCommandParameter) == 0)
                // set USER PASSWORD filetype
                CodeFileInfo.FileType = IQRF_PGM_PASS_FILE_TYPE;
            else if (strcmp("key",CcpCommandParameter) == 0)
                // set USER KEY filetype
                CodeFileInfo.FileType = IQRF_PGM_KEY_FILE_TYPE;
            else
                Message = CCP_BAD_PARAMETER;
        }
        else {
            Message = CCP_BAD_PARAMETER;
        }

        // read file filename
        if (ccpFindCmdParameter(CcpCommandParameter)) {
            // check size of filename
            if (strlen(CcpCommandParameter) > 12)
                Message = CCP_BAD_PARAMETER;
            else
                strcpy(Filename, CcpCommandParameter);
        } else {
            Message = CCP_BAD_PARAMETER;
        }

        // if any ERROR
        if (Message) {
            sysMsgPrinter(Message);
        } else {
            // open code file
            iqrfSuspendDriver();
            CodeFile = SD.open(Filename);
            iqrfRunDriver();

            // if code file exist
            if (CodeFile) {
                // read size of file
                CodeFileInfo.FileSize = CodeFile.size();
                if (CodeFileInfo.FileType == IQRF_PGM_PASS_FILE_TYPE || CodeFileInfo.FileType == IQRF_PGM_KEY_FILE_TYPE) {
                    if (CodeFileInfo.FileSize != 16) {
                        Message = CCP_FILE_FORMAT_ERR;
                    } else {
                        iqrfSuspendDriver();
                        for (Cnt=0; Cnt<16; Cnt++)
                            UserBuffer[Cnt] = iqrfReadByteFromFile();
                        iqrfRunDriver();
                        sysMsgPrinter(CCP_UPLOADING);    // message "Uploading..."
                        // write user password of user key to TR module
                        while ((TempVariable = iqrfPgmWriteKeyOrPass(CodeFileInfo.FileType, UserBuffer)) == 0)
                            ;   /* void */
                        // check result of programming operation
                        if (TempVariable == IQRF_PGM_SUCCESS)
                            Message = CCP_FILE_WRITE_OK;
                        else
                            Message = CCP_PROGRAMMING_ERR;
                    }
                    sysMsgPrinter(Message);
                } else {
                    sysMsgPrinter(CCP_CHECKING);
                    // check if code file is correct
                    while ((TempVariable = iqrfPgmCheckCodeFile()) <= 100)
                        ;   /* void */
                    // if format of code file is correct
                    if (TempVariable == IQRF_PGM_SUCCESS) {
                        sysMsgPrinter(CCP_CODE_FILE_OK);
                        // rewind code file
                        iqrfSuspendDriver();
                        CodeFile.seek(0);
                        iqrfRunDriver();

                        sysMsgPrinter(CCP_UPLOADING);    // message "Uploading..."
                        strcpy_P(Filename, ProgressBar);
                        Serial.print(Filename);
                        StoreProgress = 0;
                        // write code file to TR module
                        while ((TempVariable = iqrfPgmWriteCodeFile()) <= 100) {
                            // during code file storing, print progress bar
                            while (StoreProgress < TempVariable/10) {
                                Serial.write('*');
                                StoreProgress++;
                            }
                        }
                        // if code file written OK
                        if (TempVariable == IQRF_PGM_SUCCESS) {
                            while (StoreProgress < 10) {
                               Serial.write('*');
                               StoreProgress++;
                           }
                           Serial.println();
                           sysMsgPrinter(CCP_FILE_WRITE_OK);
                        } else {
                          Serial.println();
                          sysMsgPrinter(CCP_PROGRAMMING_ERR);
                        }
                    } else {
                        // if format of code file is wrong, print error msg
                        sysMsgPrinter(CCP_FILE_FORMAT_ERR);
                    }
                }
                CodeFile.close();   // close file
            } else {   // if code file not exist, print error msg
                sysMsgPrinter(CCP_FILE_NOT_FOUND);
            }
        }
    }
}
