/**
 * @file IQRF SPI support library
 * @author Dušan Machút <dusan.machut@gmail.com>
 * @author Rostislav Špinar <rostislav.spinar@microrisc.com>
 * @author Roman Ondráček <ondracek.roman@centrum.cz>
 * @version 2.0
 *
 * Copyright 2015-2017 MICRORISC s.r.o.
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

#endif
