/**
 * @file Arduino console command processor
 * @author Dušan Machút <dusan.machut@gmail.com>
 * @author Rostislav Špinar <rostislav.spinar@iqrf.com>
 * @author Roman Ondráček <ondracek.roman@centrum.cz>
 * @version 1.2
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

#ifndef _CCP_H
#define _CCP_H

#define SIZE_OF_IN_BUFF   48
#define SIZE_OF_PARAM     43

#define CCP_COMMAND_NOT_FOUND     0
#define CCP_CODE_FILE_OK          1
#define CCP_SD_CARD_ERR           2
#define CCP_CHECKING              3
#define CCP_UPLOADING             4
#define CCP_BAD_PARAMETER         5
#define CCP_FILE_NOT_FOUND        6
#define CCP_FILE_FORMAT_ERR       7
#define CCP_DIR_NOT_FOUND         8
#define CCP_SENDING_DATA          9
#define CCP_DATA_SETNT_OK         10
#define CCP_DATA_SENT_ERR         11
#define CCP_TR_NOT_READY          12
#define CCP_RECEIVED_DATA         13
#define CCP_FILE_WRITE_OK         14
#define CCP_PROGRAMMING_ERR       15

extern char CcpCommandParameter[SIZE_OF_PARAM];
extern const char CrLf[];
extern const char CmdPrompt[];

extern void ccp(void);
extern uint8_t ccpFindCmdParameter(char *DestinationString);
extern uint8_t ccpReadString(char *DestinationString);
extern void sysMsgPrinter(uint16_t Msg);

#endif
