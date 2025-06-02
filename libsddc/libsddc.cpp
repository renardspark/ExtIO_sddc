/*
 * This file is part of SDDC_Driver.
 *
 * Copyright (C) 2020 - Howard Su
 * Copyright (C) 2021 - Hayati Ayguen
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

#include "libsddc.h"
#include "config.h"
#include "RadioHandler.h"

#include <cstring>

// libsddc handler
struct libsddc_handler
{
	RadioHandler* radio_handler;

	sddc_read_async_cb_t callback;
	void *callback_context;
};

static void Callback(void* context, const sddc_complex_t* data, uint32_t len)
{
	const libsddc_handler_t t = static_cast<libsddc_handler_t>(context);

	if(t->callback)
		t->callback(len, data, t->callback_context);
}

// --- "Static" functions --- //
uint16_t sddc_get_device_count()
{
	return RadioHandler::GetDeviceListLength();
}

sddc_err_t sddc_get_device(uint8_t dev_index, struct sddc_device_t *dev)
{
	return RadioHandler::GetDevice(dev_index, dev);
}
// --- //


libsddc_handler_t sddc_create()
{
	return new libsddc_handler();
}


sddc_err_t sddc_init(libsddc_handler_t t, uint8_t dev_index)
{
	t->radio_handler = new RadioHandler();

	sddc_err_t ret = t->radio_handler->Init(dev_index);
	if(ret != ERR_SUCCESS) return ret;

	ret = t->radio_handler->AttachIQ(Callback, t);
	if(ret != ERR_SUCCESS) return ret;

	return ERR_SUCCESS;
}




void sddc_destroy(libsddc_handler_t t)
{
	if(t->radio_handler)
		delete t->radio_handler;
}


RadioModel sddc_get_model(libsddc_handler_t t)
{
	return t->radio_handler->getHardwareModel();
}

const char *sddc_get_model_name(libsddc_handler_t t)
{
	return t->radio_handler->getHardwareName();
}

uint16_t sddc_get_firmware(libsddc_handler_t t)
{
	return t->radio_handler->GetHardwareFirmware();
}

// --- RF --- //
sddc_rf_mode_t sddc_get_rf_mode(libsddc_handler_t t) { return t->radio_handler->GetRFMode(); }

sddc_err_t sddc_set_rf_mode(libsddc_handler_t t, sddc_rf_mode_t rf_mode) { return t->radio_handler->SetRFMode(rf_mode); }
// --- //

// --- LEDs --- //
sddc_err_t sddc_set_led(libsddc_handler_t t, sddc_leds_t led, bool state)
	{ return t->radio_handler->SetLED(led, state); }



/* ADC functions */
bool sddc_get_dither(libsddc_handler_t t)
{
	return t->radio_handler->GetDither();
}

sddc_err_t sddc_set_dither(libsddc_handler_t t, bool dither)
{
	return t->radio_handler->SetDither(dither);
}

bool sddc_get_pga(libsddc_handler_t t)
{
	return t->radio_handler->GetPGA();
}

sddc_err_t sddc_set_pga(libsddc_handler_t t, bool pga)
{
	return t->radio_handler->SetPGA(pga);
}

bool sddc_get_random(libsddc_handler_t t)
{
	return t->radio_handler->GetRand();
}

sddc_err_t sddc_set_random(libsddc_handler_t t, bool random)
{
	return t->radio_handler->SetRand(random);
}

bool sddc_get_biast_hf(libsddc_handler_t t)
{
	return t->radio_handler->GetBiasT_HF();
}
sddc_err_t sddc_set_biast_hf(libsddc_handler_t t, bool new_state)
{
	return t->radio_handler->SetBiasT_HF(new_state);
}

bool sddc_get_biast_vhf(libsddc_handler_t t)
{
	return t->radio_handler->GetBiasT_VHF();
}

sddc_err_t sddc_set_biast_vhf(libsddc_handler_t t, bool new_state)
{
	return t->radio_handler->SetBiasT_VHF(new_state);
}

uint32_t sddc_set_center_frequency (libsddc_handler_t t, uint32_t freq)
{
	return t->radio_handler->SetCenterFrequency(freq);
}


uint32_t sddc_get_adc_sample_rate(libsddc_handler_t t)
{
	return t->radio_handler->GetADCSampleRate();
}
sddc_err_t sddc_set_adc_sample_rate(libsddc_handler_t t, uint32_t sample_rate)
{
	return t->radio_handler->SetADCSampleRate(sample_rate);
}

sddc_err_t sddc_set_stream_callback(libsddc_handler_t t, sddc_read_async_cb_t callback,
						  void *callback_context)
{
	t->callback = callback;
	t->callback_context = callback_context;
	return ERR_SUCCESS;
}

sddc_err_t sddc_start_streaming(libsddc_handler_t t)
{
	return t->radio_handler->Start(/*convert_r2iq=*/true);
}

sddc_err_t sddc_stop_streaming(libsddc_handler_t t)
{
	return t->radio_handler->Stop();
}

sddc_err_t sddc_set_decimation(libsddc_handler_t t, uint8_t decimate)
{
	return t->radio_handler->SetDecimation(decimate);
}


int sddc_get_attenuation_steps(libsddc_handler_t t, const float** s)
{
	vector<float> steps = t->radio_handler->GetAttenuationSteps();

	*s = (const float*)malloc(steps.size() * sizeof(float));
	memcpy(s, steps.data(), steps.size() * sizeof(float));

	return steps.size();
}
sddc_err_t sddc_set_attenuation(libsddc_handler_t t, int attIndex)
{
	return t->radio_handler->SetAttenuation(attIndex);
}

int sddc_get_gain_steps(libsddc_handler_t t, const float** s)
{
	vector<float> steps = t->radio_handler->GetGainSteps();

	*s = (const float*)malloc(steps.size() * sizeof(float));
	memcpy(s, steps.data(), steps.size() * sizeof(float));

	return steps.size();
}
sddc_err_t sddc_set_gain(libsddc_handler_t t, int attIndex)
{
	return t->radio_handler->SetGain(attIndex);
}
