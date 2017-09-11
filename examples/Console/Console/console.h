/**
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

#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern uint8_t   SDCardReady;

void ccpTrModuleInfo(uint16_t CommandParameter);
void ccpTrSendData(uint16_t CommandParameter);
void ccpPgmFile(uint16_t CommandParameter);

#endif
