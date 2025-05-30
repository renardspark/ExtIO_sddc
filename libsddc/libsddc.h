/*
 * This file is part of SDDC_Driver.
 *
 * Copyright (C) 2020 - Fraco Venturi
 * Copyright (C) 2020 - Howard Su
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

#ifndef __H_LIBSDDC
#define __H_LIBSDDC

#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include "../Interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Static functions --- //
uint16_t sddc_get_device_count();
sddc_err_t sddc_get_device(uint8_t dev_index, struct sddc_device_t *dev);
// --- //

// ----- libsddc ----- //

typedef void (*sddc_read_async_cb_t)(uint32_t data_size, const float *data,
										void *context);

typedef struct libsddc_handler* libsddc_handler_t;

// Init and destroy
libsddc_handler_t sddc_create();
sddc_err_t	sddc_init(libsddc_handler_t, uint8_t dev_index);
void		sddc_destroy(libsddc_handler_t);




sddc_err_t	sddc_set_stream_callback(
	libsddc_handler_t t,
	sddc_read_async_cb_t callback,
	void *callback_context);







// --- Streaming --- //
sddc_err_t	sddc_start_streaming(libsddc_handler_t t);
sddc_err_t	sddc_stop_streaming(libsddc_handler_t t);

// --- Hardware infos --- //
RadioModel		sddc_get_model(libsddc_handler_t t);
const char*		sddc_get_model_name(libsddc_handler_t t);
uint16_t		sddc_get_firmware(libsddc_handler_t t);
// --- //

// --- RF mode --- //
sddc_rf_mode_t sddc_get_best_rf_mode(libsddc_handler_t t);
sddc_rf_mode_t sddc_get_rf_mode(libsddc_handler_t t);
sddc_err_t sddc_set_rf_mode(libsddc_handler_t t, sddc_rf_mode_t rf_mode);
// --- //

// --- ADC --- //
uint32_t	sddc_get_adc_sample_rate(libsddc_handler_t t);
sddc_err_t	sddc_set_adc_sample_rate(libsddc_handler_t t, uint32_t samplefreq);
// --- //

// --- Bias tee --- //
bool		sddc_get_biast_hf ();
sddc_err_t	sddc_set_biast_hv (bool new_state);
bool		sddc_get_biast_vhf();
sddc_err_t	sddc_set_biast_vhf(bool new_state);

// --- RF/IF adjustments --- //
int			sddc_get_attenuation_steps(libsddc_handler_t t, const float** steps);
sddc_err_t	sddc_set_attenuation(libsddc_handler_t t, int attIndex);
int			sddc_get_gain_steps(libsddc_handler_t t, const float** steps);
sddc_err_t	sddc_set_gain(libsddc_handler_t t, int attIndex);

// --- Tuner --- //
uint32_t	sddc_set_center_frequency(libsddc_handler_t t, uint32_t freq);

// --- Misc --- //
bool		sddc_get_dither();
sddc_err_t	sddc_set_dither(libsddc_handler_t t, bool new_state);
bool		sddc_get_pga(libsddc_handler_t t);
sddc_err_t	sddc_set_pga(libsddc_handler_t t, bool new_state);
bool		sddc_get_rand(libsddc_handler_t t);
sddc_err_t	sddc_set_rand(libsddc_handler_t t, bool new_state);
// --- //

// --- LEDs --- //
sddc_err_t	sddc_set_led(libsddc_handler_t t, sddc_leds_t led, bool on);
// --- //

// --- r2iq only --- //
sddc_err_t sddc_set_decimation(libsddc_handler_t t, uint8_t decimate);
// --- //

#ifdef __cplusplus
}
#endif

#endif /* __LIBSDDC_H */