/*
 * This file is part of SDDC_Driver.
 *
 * Copyright (C) 2020 - Oscar Steila
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

#ifndef _H_RADIOHANDLER
#define _H_RADIOHANDLER

#include <array>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "config.h"
#include "FX3Class.h"
#include "radio/RadioHardware.h"
#include "fft_mt_r2iq.h"
#include "dsp/ringbuffer.h"

using namespace std;

class RadioHardware;

enum {
	RESULT_OK,
	RESULT_BIG_STEP,
	RESULT_TOO_HIGH,
	RESULT_TOO_LOW,
	RESULT_NOT_POSSIBLE
};

struct shift_limited_unroll_C_sse_data_s;
typedef struct shift_limited_unroll_C_sse_data_s shift_limited_unroll_C_sse_data_t;

class RadioHandler {
public:
	RadioHandler();
	~RadioHandler();
	sddc_err_t Init(uint8_t dev_index);
	sddc_err_t AttachReal(void (*callback)(void* context, const int16_t*, uint32_t), void* context = nullptr);
	sddc_err_t AttachIQ(void (*callback)(void* context, const sddc_complex_t*, uint32_t), void* context = nullptr);
	sddc_err_t Start(bool convert_r2iq);
	sddc_err_t Stop();

	// --- r2iq --- //
	sddc_err_t	SetDecimation(uint8_t decimate);

	// ----- RF mode ----- //
	sddc_rf_mode_t	GetBestRFMode(uint64_t freq);
	sddc_rf_mode_t	GetRFMode();
	sddc_err_t		SetRFMode(sddc_rf_mode_t mode);

	// --- ADC --- //
	uint32_t	GetADCSampleRate();
	sddc_err_t	SetADCSampleRate(uint32_t samplefreq);

	// --- Tuner --- //
	uint32_t    GetCenterFrequency();
    sddc_err_t  SetCenterFrequency(uint32_t freq);

	// --- Bias tee --- //
	bool		GetBiasT_HF ();
	sddc_err_t	SetBiasT_HF (bool new_state);
	bool		GetBiasT_VHF();
	sddc_err_t	SetBiasT_VHF(bool new_state);

	// --- RF/IF adjustments --- //
	vector<float>   GetRFGainSteps(sddc_rf_mode_t mode = NOMODE);
	array<float, 2> GetRFGainRange(sddc_rf_mode_t mode = NOMODE);
	float           GetRFGain(sddc_rf_mode_t mode = NOMODE);
	sddc_err_t      SetRFGain(float new_att, sddc_rf_mode_t mode = NOMODE);

	vector<float>   GetIFGainSteps(sddc_rf_mode_t mode = NOMODE);
	array<float, 2> GetIFGainRange(sddc_rf_mode_t mode = NOMODE);
	float           GetIFGain(sddc_rf_mode_t mode = NOMODE);
	sddc_err_t      SetIFGain(float new_gain, sddc_rf_mode_t mode = NOMODE);

	// --- Misc --- //
	bool		GetDither();
	sddc_err_t	SetDither(bool new_state);
	bool		GetPGA();
	sddc_err_t	SetPGA(bool new_state);
	bool		GetRand();
	sddc_err_t	SetRand(bool new_state);

	// --- GPIOs --- //
	sddc_err_t	SetLED (sddc_leds_t led, bool on);
	// ----- //

	float getRealSamplesPerSecond() const { return real_samples_per_second; }
	float getIQSamplesPerSecond()   const { return iq_samples_per_second; }

	/// --- Hardware infos --- //
	RadioModel getHardwareModel() { return devModel; }
	const char *getHardwareName() { return hardware->GetName(); }
	uint16_t GetHardwareFirmware() { return devFirmware; }
	// --- //


	void EnableDebug(void (*dbgprintFX3)(const char* fmt, ...), bool (*getconsolein)(char* buf, int maxlen)) 
		{ 
		  this->DbgPrintFX3 = dbgprintFX3; 
		  this->GetConsoleIn = getconsolein;
		};

	bool ReadDebugTrace(uint8_t* pdata, uint8_t len) { return fx3->ReadDebugTrace(pdata, len); }

	// --- Static functions --- //
	static size_t GetDeviceListLength();
	static sddc_err_t GetDevice(uint8_t dev_index, sddc_device_t *dev_pointer);

private:
	fx3class *fx3;

	void CaculateStats();
	void OnDataPacket();

	void (*callbackReal)(void* context, const int16_t *data, uint32_t length);
	void *callbackRealContext;
	void (*callbackIQ)(void* context, const sddc_complex_t *data, uint32_t length);
	void *callbackIQContext;

	void (*DbgPrintFX3)(const char* fmt, ...);
	bool (*GetConsoleIn)(char* buf, int maxlen);

	bool streamRunning = false;
	unsigned long count;    // absolute index

	RadioModel devModel;
	uint16_t devFirmware;

	// transfer variables
	ringbuffer<int16_t> real_buffer;
	ringbuffer<sddc_complex_t> iq_buffer;

	// threads
	std::thread show_stats_thread;
	std::thread submit_thread;

	// --- Stats --- //
	uint32_t count_real_samples   = 0;
	uint32_t count_iq_samples     = 0;
	float real_samples_per_second = 0;
	float iq_samples_per_second   = 0;

	RadioHardware* hardware;
	std::mutex fc_mutex;
    float fc;
	shift_limited_unroll_C_sse_data_t* stateFineTune;
	fft_mt_r2iq* r2iqCntrl;
	bool r2iqEnabled = false;
};

extern unsigned long Failures;

#endif