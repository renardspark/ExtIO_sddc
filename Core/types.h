/*
 * This file is part of SDDC_Driver.
 *
 * Copyright (C) 2025 - RenardSpark
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _H_TYPES
#define _H_TYPES

#include <stdint.h>

/**
 * @brief List of all the errors which may be returned
 * 
 */
typedef enum sddc_err_t {
	ERR_SUCCESS = 0x00, ///< No error
	ERR_FX3_OPEN_FAILED,
	ERR_FX3_DEVICE_BUSY,
	ERR_FX3_TRANSFER_FAILED,
	ERR_NOT_COMPATIBLE = -0x10, ///< The function is not compatible with the current hardware
	ERR_DECIMATION_OUT_OF_RANGE, ///< The given decimation is out of the allowed range
	ERR_NOT_LED, ///< The selected LED is not an LED
	ERR_BUFFER_SIZE_INVALID
} sddc_err_t;

typedef enum sddc_rf_mode_t {
	NOMODE  = 0x00,
	HFMODE  = 0x01,
	VHFMODE = 0x02
} sddc_rf_mode_t;

typedef enum sddc_leds_t {
	SDDC_LED_YELLOW = 0x01,
	SDDC_LED_RED    = 0x02,
	SDDC_LED_BLUE   = 0x04
} sddc_leds_t;

typedef struct sddc_device_t {
	char product[32];
	char serial_number[32];
} sddc_device_t;

#endif // _H_TYPES