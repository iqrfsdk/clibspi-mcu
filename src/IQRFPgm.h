/**
 * @file IQRF SPI support library (firmware and config programmer extension)
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

#ifndef _IQRF_PGM_H
#define _IQRF_PGM_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define	IQRF_PGM_SUCCESS              200
#define IQRF_PGM_FLASH_BLOCK_READY    220
#define IQRF_PGM_EEPROM_BLOCK_READY   221
#define	IQRF_PGM_ERROR                222

#define IQRF_PGM_FILE_DATA_READY      0
#define IQRF_PGM_FILE_DATA_ERROR      1
#define IQRF_PGM_END_OF_FILE          2

#define	IQRF_PGM_HEX_FILE_TYPE        1
#define IQRF_PGM_PLUGIN_FILE_TYPE     2
#define IQRF_PGM_CFG_FILE_TYPE        3
#define IQRF_PGM_PASS_FILE_TYPE       4
#define IQRF_PGM_KEY_FILE_TYPE        5

#define IQRF_SIZE_OF_FLASH_BLOCK      64
#define IQRF_LICENCED_MEMORY_BLOCKS   96
#define IQRF_MAIN_MEMORY_BLOCKS       48

#define IQRF_CFG_MEMORY_BLOCK         (IQRF_LICENCED_MEMORY_BLOCKS - 2)

#define SERIAL_EEPROM_MIN_ADR         0x0200
#define SERIAL_EEPROM_MAX_ADR         0x09FF
#define IQRF_LICENCED_MEM_MIN_ADR     0x2C00
#define IQRF_LICENCED_MEM_MAX_ADR     0x37FF
#define IQRF_CONFIG_MEM_L_ADR         0x37C0
#define IQRF_CONFIG_MEM_H_ADR         0x37D0
#define IQRF_MAIN_MEM_MIN_ADR         0x3A00
#define IQRF_MAIN_MEM_MAX_ADR         0x3FFF
#define PIC16LF1938_EEPROM_MIN        0xf000
#define PIC16LF1938_EEPROM_MAX        0xf0ff
#define RF_BAND_CFG_ADR               0xC0
#define RFPGM_CFG_ADR                 0xC1
#define ACCESS_PASSWORD_CFG_ADR       0xD0
#define USER_KEY_CFG_ADR              0xD1

/**
 * Checking the format accuracy of the programing file
 * @return result of partial checking operation
 */
uint8_t iqrfPgmCheckCodeFile(void);

/**
 * Core programming function
 * @return result of partial programming operation
 */
uint8_t iqrfPgmWriteCodeFile(void);

/**
 * Core programming function for user password or user key
 * @param BufferContent selects between user key or user password to be written
 * @param Buffer pointer to 16 byte buffer with user password or user key
 * @return result of partial programming operation
 */
uint8_t iqrfPgmWriteKeyOrPass(uint8_t BufferContent, uint8_t *Buffer);

#if defined(__cplusplus)
}
#endif

#endif
