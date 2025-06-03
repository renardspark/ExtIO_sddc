/*
 * This file is part of SDDC_Driver.
 *
 * Copyright (C) 2020 - Oscar Steila
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



#include <stdio.h>
#include <string.h>
#include <chrono>

#include "RadioHandler.h"
#include "config.h"
#include "../Interface.h"
#include "fft_mt_r2iq.h"
#include "PScope_uti.h"
#include "pffft/pf_mixer.h"

using namespace std;

#define TAG "RadioHandler"


void RadioHandler::OnDataPacket()
{
	auto len_real = real_buffer.getBlockSize();
	auto len_iq   = iq_buffer.getBlockSize();

	//ringbuffer<>* source_buffer = r2iqEnabled ? &outputbuffer : &inputbuffer;

	while(streamRunning)
	{
		if(r2iqEnabled)
		{
			auto buf = iq_buffer.getReadPtr();

			if (!streamRunning)
				break;

			if (fc != 0.0f)
			{
				std::unique_lock<std::mutex> lk(fc_mutex);
				shift_limited_unroll_C_sse_inp_c((complexf*)buf, len_iq, stateFineTune);
			}

			callbackIQ(callbackIQContext, buf, len_iq);

			iq_buffer.ReadDone();
			count_iq_samples += len_iq;
		}
		else
		{
			auto buf = real_buffer.getReadPtr();

			if (!streamRunning)
				break;

			callbackReal(callbackRealContext, buf, len_real);

			real_buffer.ReadDone();
			count_real_samples += len_real;
		}
	}
}

/**
 * @brief Create a new Radio Handler
 * 
 * @param[in] dev_index The index of the SDR to use. You can get the list of
 *   available devices by using the static function of RadioHandler
 */
RadioHandler::RadioHandler():
	DbgPrintFX3(nullptr),
	GetConsoleIn(nullptr),
	hardware(new DummyRadio(nullptr)),
	fc(0.0f)
	
{
	TracePrintln(TAG, "");

	fx3 = CreateUsbHandler();
	stateFineTune = new shift_limited_unroll_C_sse_data_t();
}

sddc_err_t RadioHandler::Init(uint8_t dev_index)
{
	TracePrintln(TAG, "%d", dev_index);

	if(!fx3->Open(dev_index))
	{
		DebugPrintln(TAG, "FX3 open failed");
		return ERR_FX3_OPEN_FAILED;
	}

	uint8_t rdata[4];
	fx3->GetHardwareInfo((uint32_t*)rdata);

	devModel = (RadioModel)rdata[0];
	devFirmware = (rdata[1] << 8) + rdata[2];

	delete hardware; // delete dummy instance
	switch (devModel)
	{
	case HF103:
		hardware = new HF103Radio(fx3);
		break;

	case BBRF103:
		hardware = new BBRF103Radio(fx3);
		break;

	case RX888:
		hardware = new RX888Radio(fx3);
		break;

	case RX888r2:
		hardware = new RX888R2Radio(fx3);
		break;

	case RX888r3:
		hardware = new RX888R3Radio(fx3);
		break;

	case RX999:
		hardware = new RX999Radio(fx3);
		break;

	case RXLUCY:
		hardware = new RXLucyRadio(fx3);
		break;

	default:
		hardware = new DummyRadio(fx3);
		DbgPrintf("WARNING no SDR connected");
		break;
	}

	sddc_err_t ret = hardware->SetRFMode(HFMODE);
	if(ret != ERR_SUCCESS) return ret;

	ret = hardware->SetADCSampleRate(DEFAULT_ADC_FREQ);
	if(ret != ERR_SUCCESS) return ret;

	DebugPrintln(TAG, "Detected radio : %s, firmware %x", hardware->GetName(), devFirmware);

	real_buffer.setBlockSize(transferSamples);

	// May be improved : r2iq assumes that the output buffer has half
	// the size of the input buffer (due to real to complex conversion)
	iq_buffer.setBlockSize(transferSamples/2);

	this->r2iqCntrl = new fft_mt_r2iq();
	r2iqCntrl->Init(hardware->getGain(), &real_buffer, &iq_buffer);

	return ERR_SUCCESS;
}

RadioHandler::~RadioHandler()
{
	delete stateFineTune;
	delete hardware;
	delete fx3;
}

sddc_err_t RadioHandler::AttachReal(void (*callback)(void*context, const int16_t*, uint32_t), void *context)
{
	this->callbackReal = callback;
	this->callbackRealContext = context;

	return ERR_SUCCESS;
}

sddc_err_t RadioHandler::AttachIQ(void (*callback)(void*context, const sddc_complex_t*, uint32_t), void *context)
{
	this->callbackIQ = callback;
	this->callbackIQContext = context;

	return ERR_SUCCESS;
}

/**
 * @brief Create a new Radio Handler
 * 
 * \note This function has no effect if `convert_r2iq` is set to `false` when calling `RadioHandler::Start`
 * 
 * @param[in] decimate A power of 2 of the decimation to apply to the input signal
 * 
 * \code
 *  radio_handler.SetDecimation(0) // No decimation
 *  radio_handler.SetDecimation(1) // Decimate by 2
 *  radio_handler.SetDecimation(4) // Decimate by 16
 * \endcode
 * 
 * \retval ERR_SUCCESS
 * \retval ERR_DECIMATION_OUT_OF_RANGE
 */
sddc_err_t RadioHandler::SetDecimation(uint8_t decimate)
{
	if(r2iqCntrl->setDecimate(decimate) != true)
		return ERR_DECIMATION_OUT_OF_RANGE;

	return ERR_SUCCESS;
}


/**
 * @brief Start the SDR and processing functions
 * 
 * @param[in] convert_r2iq Set to `true` to output IQ data instead of real samples
 * 
 * \retval ERR_SUCCESS
 * \retval ERR_DECIMATION_OUT_OF_RANGE
 */
sddc_err_t RadioHandler::Start(bool convert_r2iq)
{

	TracePrintln(TAG, "%s", convert_r2iq ? "true" : "false");

	// Stop the stream  if it was already running
	sddc_err_t ret = Stop();
	if(ret != ERR_SUCCESS) return ret;

	streamRunning = true;
	count = 0;

	// SDR starts sending frames
	ret = hardware->StartStream();
	if(ret != ERR_SUCCESS) return ret;

	//iq_buffer.setBlockSize(EXT_BLOCKLEN * sizeof(float));

	r2iqEnabled = convert_r2iq;
	if(r2iqEnabled) r2iqCntrl->TurnOn();

	// Driver starts receiving frames
	fx3->StartStream(real_buffer/*, QUEUE_SIZE*/);

	submit_thread = std::thread([this]() {
		this->OnDataPacket();
	});

	show_stats_thread = std::thread([this](void*) {
		this->CaculateStats();
	}, nullptr);

	return ERR_SUCCESS;
}

sddc_err_t RadioHandler::Stop()
{
	TracePrintln(TAG, "");
	DebugPrintln(TAG, "Stream running : %s", streamRunning ? "true" : "false");

	if(streamRunning)
	{
		streamRunning = false; // now waits for threads

		r2iqCntrl->TurnOff();

		fx3->StopStream();

		show_stats_thread.join(); //first to be joined
		DbgPrintf("show_stats_thread join2");

		submit_thread.join();
		DbgPrintf("submit_thread join1");

		sddc_err_t ret = hardware->StopStream(); // SDR stops sending frames
		if(ret != ERR_SUCCESS) return ret;
	}
	return ERR_SUCCESS;
}

sddc_err_t RadioHandler::SetRFMode(sddc_rf_mode_t mode)
{
	TracePrintln(TAG, "%d", mode);
	if(hardware->GetRFMode() != mode)
	{
		DebugPrintln(TAG, "Switching to RF mode %d", mode);

		sddc_err_t ret = hardware->SetRFMode(mode);
		if(ret != ERR_SUCCESS) return ret;

		if(mode == VHFMODE)
			r2iqCntrl->setSideband(true);
		else
			r2iqCntrl->setSideband(false);
	}
	return ERR_SUCCESS;
}

vector<float> RadioHandler::GetAttenuationSteps()
{
	TracePrintln(TAG, "");
	switch(hardware->GetRFMode())
	{
		case HFMODE:
			return hardware->GetRFSteps_HF();
		case VHFMODE:
			return hardware->GetRFSteps_VHF();
		default:
			return vector<float>();
	}
}
array<float, 2> RadioHandler::GetAttenuationRange()
{
	TracePrintln(TAG, "");
	
	vector<float> att_steps = GetAttenuationSteps();
	DebugPrintln(TAG, "Attenuation : min=%f, max=%f", att_steps.front(), att_steps.back());
	return array<float, 2>{att_steps.front(), att_steps.back()};
}
float RadioHandler::GetAttenuation()
{
	TracePrintln(TAG, "");

	int step = 0;
	switch(hardware->GetRFMode())
	{
		case HFMODE:
			step = hardware->GetRF_HF();
			break;
		case VHFMODE:
			step = hardware->GetRF_VHF();
			break;
		default:
			return 0;
	}

	vector<float> att_steps = GetAttenuationSteps();

	if(step >= 0 && step < att_steps.size())
	{
		DebugPrintln(TAG, "Attenuation = %f, step = %d", att_steps[step], step);
		return att_steps[step];
	}

	return 0;
}
sddc_err_t RadioHandler::SetAttenuation(float new_att)
{
	vector<float> att_steps = GetAttenuationSteps();

	if(att_steps.empty())
		return ERR_NOT_COMPATIBLE;

	if(new_att <= att_steps.front())
		new_att = att_steps.front();

	if(new_att >= att_steps.back())
		new_att = att_steps.back();

	int step = 0;
	for (size_t i = 1; i < att_steps.size(); i++) {
        if(att_steps[i - 1] <= new_att && att_steps[i] >= new_att)
        {
            float gain_middle = (att_steps[i] - att_steps[i - 1]) / 2;
            float value_relative = (new_att - att_steps[i - 1]);
            step = (value_relative > gain_middle) ? i : i - 1;
            break;
        }
    }

	switch(hardware->GetRFMode())
	{
		case HFMODE:
			DebugPrintln(TAG, "HF gain = %f, step = %d", att_steps[step], step);
			return hardware->SetRFAttenuation_HF(step);
		case VHFMODE:
			DebugPrintln(TAG, "VHF gain = %f, step = %d", att_steps[step], step);
			return hardware->SetRFAttenuation_HF(step);
		default:
			return ERR_NOT_COMPATIBLE;
	}
}

vector<float> RadioHandler::GetGainSteps()
{
	switch(hardware->GetRFMode())
	{
		case HFMODE:
			return hardware->GetIFSteps_HF();
		case VHFMODE:
			return hardware->GetIFSteps_VHF();
		default:
			return vector<float>();
	}
}
array<float, 2> RadioHandler::GetGainRange()
{
	TracePrintln(TAG, "");
	
	vector<float> gain_steps = GetGainSteps();
	DebugPrintln(TAG, "Gain : min=%f, max=%f", gain_steps.front(), gain_steps.back());
	return array<float, 2>{gain_steps.front(), gain_steps.back()};
}
float RadioHandler::GetGain()
{
	TracePrintln(TAG, "");

	int step = 0;
	switch(hardware->GetRFMode())
	{
		case HFMODE:
			step = hardware->GetIF_HF();
			break;
		case VHFMODE:
			step = hardware->GetIF_VHF();
			break;
		default:
			return 0;
	}

	vector<float> gain_steps = GetGainSteps();

	if(step >= 0 && step < gain_steps.size())
	{
		DebugPrintln(TAG, "Gain = %f, step = %d", gain_steps[step], step);
		return gain_steps[step];
	}

	return 0;
}
sddc_err_t RadioHandler::SetGain(float new_gain)
{
	vector<float> gain_steps = GetGainSteps();

	if(gain_steps.empty())
		return ERR_NOT_COMPATIBLE;

	if(new_gain <= gain_steps.front())
		new_gain = gain_steps.front();

	if(new_gain >= gain_steps.back())
		new_gain = gain_steps.back();

	int step = 0;
	for (size_t i = 1; i < gain_steps.size(); i++) {
        if(gain_steps[i - 1] <= new_gain && gain_steps[i] >= new_gain)
        {
            float gain_middle = (gain_steps[i] - gain_steps[i - 1]) / 2;
            float value_relative = (new_gain - gain_steps[i - 1]);
            step = (value_relative > gain_middle) ? i : i - 1;
            break;
        }
    }

	switch(hardware->GetRFMode())
	{
		case HFMODE:
			DebugPrintln(TAG, "HF gain = %f, step = %d", gain_steps[step], step);
			return hardware->SetIFGain_HF(step);
		case VHFMODE:
			DebugPrintln(TAG, "VHF gain = %f, step = %d", gain_steps[step], step);
			return hardware->SetIFGain_VHF(step);
		default:
			return ERR_NOT_COMPATIBLE;
	}
}

uint32_t RadioHandler::GetCenterFrequency()
{
	switch(hardware->GetRFMode())
	{
		case HFMODE:
			return hardware->GetLOFreq_HF();
		case VHFMODE:
			return hardware->GetLOFreq_VHF();
		default:
			return 0;
	}
}
sddc_err_t RadioHandler::SetCenterFrequency(uint32_t wishedFreq)
{
	float fc = 0;

	if(hardware->GetRFMode() == HFMODE)
	{
		sddc_err_t ret = hardware->SetLOFreq_HF(wishedFreq);
		if(ret != ERR_SUCCESS) return ret;

		// we need shift the samples
		uint32_t offset = hardware->GetTunerCarrier_HF();
		DebugPrintln(TAG, "Tuner frequency is at %dHz", offset);
		fc = r2iqCntrl->setFreqOffset(offset / (GetADCSampleRate() / 2.0f));
	}
	else if(hardware->GetRFMode() == VHFMODE)
	{
		sddc_err_t ret = hardware->SetLOFreq_VHF(wishedFreq);
		if(ret != ERR_SUCCESS) return ret;

		// we need shift the samples
		uint32_t offset = hardware->GetTunerCarrier_VHF();
		DebugPrintln(TAG, "Tuner frequency is at %dHz", offset);

		// sign change with sideband used
		fc = -r2iqCntrl->setFreqOffset(offset / (GetADCSampleRate() / 2.0f));
	}
	else
	{
		return ERR_NOT_COMPATIBLE;
	}
	
	if (this->fc != fc)
	{
		std::unique_lock<std::mutex> lk(fc_mutex);
		*stateFineTune = shift_limited_unroll_C_sse_init(fc, 0.0F);
		this->fc = fc;
	}
	return ERR_SUCCESS;
}



void RadioHandler::CaculateStats()
{
	chrono::high_resolution_clock::time_point StartingTime, EndingTime;

	uint8_t  debdata[MAXLEN_D_USB];
	memset(debdata, 0, MAXLEN_D_USB);

	while (streamRunning)
	{
		// --- Reset all counters --- //
		count_real_samples = 0;
		count_iq_samples = 0;

		StartingTime = chrono::high_resolution_clock::now();

#ifdef _DEBUG  
		int nt = 10;
		while (nt-- > 0)
		{
			std::this_thread::sleep_for(0.1s);
			debdata[0] = 0; //clean buffer 
			if (GetConsoleIn != nullptr)
			{
				GetConsoleIn((char *)debdata, MAXLEN_D_USB);
				if (debdata[0] !=0) 
					DbgPrintf("%s", (char*)debdata);
			}

			if (hardware->ReadDebugTrace(debdata, MAXLEN_D_USB) == true) // there are message from FX3 ?
			{
				int len = strlen((char*)debdata);
				if (len > MAXLEN_D_USB - 1) len = MAXLEN_D_USB - 1;
				debdata[len] = 0;
				if ((len > 0)&&(DbgPrintFX3 != nullptr))
				{
					DbgPrintFX3("%s", (char*)debdata);
					memset(debdata, 0, sizeof(debdata));
				}
			}
			
		}
#else
		std::this_thread::sleep_for(1s);
#endif


		EndingTime = chrono::high_resolution_clock::now();
		chrono::duration<float,std::ratio<1,1>> timeElapsed(EndingTime-StartingTime);

		real_samples_per_second = (float)count_real_samples / timeElapsed.count();
		iq_samples_per_second = (float)count_iq_samples / timeElapsed.count();

		DebugPrintln(TAG, "real=%fSps, iq=%fSps", real_samples_per_second, iq_samples_per_second);
	}
	return;
}

sddc_err_t RadioHandler::SetRand(bool new_state)
{
	r2iqCntrl->SetRand(new_state);
	return hardware->SetRand(new_state);
};



// ----- RadioHardware passthrough ----- //
sddc_rf_mode_t  RadioHandler::GetBestRFMode(uint64_t freq) { return hardware->GetBestRFMode(freq); };
sddc_rf_mode_t  RadioHandler::GetRFMode()                  { return hardware->GetRFMode(); };

uint32_t	RadioHandler::GetADCSampleRate()                    { return hardware->GetADCSampleRate(); };
sddc_err_t	RadioHandler::SetADCSampleRate(uint32_t samplefreq) { return hardware->SetADCSampleRate(samplefreq); };

bool 		RadioHandler::GetBiasT_HF ()               { return hardware->GetBiasT_HF(); };
sddc_err_t	RadioHandler::SetBiasT_HF (bool new_state) { return hardware->SetBiasT_HF(new_state); };
bool 		RadioHandler::GetBiasT_VHF()               { return hardware->GetBiasT_VHF(); };
sddc_err_t	RadioHandler::SetBiasT_VHF(bool new_state) { return hardware->SetBiasT_VHF(new_state); };

bool 		RadioHandler::GetDither()               { return hardware->GetDither(); };
sddc_err_t	RadioHandler::SetDither(bool new_state) { return hardware->SetDither(new_state); };
bool 		RadioHandler::GetPGA()                  { return hardware->GetPGA(); };
sddc_err_t	RadioHandler::SetPGA(bool new_state)    { return hardware->SetPGA(new_state); };
bool 		RadioHandler::GetRand()                 { return hardware->GetRand(); };

sddc_err_t	RadioHandler::SetLED   (sddc_leds_t led, bool on) { return hardware->SetLED(led, on); };


// --- Static functions --- //

size_t RadioHandler::GetDeviceListLength()
{
	TracePrintln(TAG, "");
	auto fx3_handler = CreateUsbHandler();
	size_t len = fx3_handler->GetDeviceListLength();
	delete fx3_handler;

	return len;
}
sddc_err_t RadioHandler::GetDevice(uint8_t dev_index, sddc_device_t *dev_pointer)
{
	TracePrintln(TAG, "%d, %p", dev_index, dev_pointer);

	auto fx3_handler = CreateUsbHandler();
	fx3_handler->GetDevice(dev_index, dev_pointer->product, 32, dev_pointer->serial_number, 32);

	/* Old code, I don't know his exact role
	// https://en.wikipedia.org/wiki/West_Bridge
	int retry = 2;
	while ((strncmp("WestBridge", devicelist.dev[idx],sizeof("WestBridge")) != NULL) && retry-- > 0)
		Fx3->Enumerate(idx, devicelist.dev[idx]); // if it enumerates as BootLoader retry
	idx++;
	*/
	delete fx3_handler;

	return ERR_SUCCESS;
}