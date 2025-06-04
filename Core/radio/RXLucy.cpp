/*
 * This file is part of SDDC_Driver.
 *
 * Copyright (C) 2021 - Oscar Steila
 * Copyright (C) 2021 - Howard Su
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

#include "../RadioHandler.h"

#define ADC_FREQ (64u*1000*1000)
#define IF_FREQ (ADC_FREQ / 4)

#define HIGH_MODE 0x80
#define LOW_MODE 0x00

#define MODE HIGH_MODE

RXLucyRadio::RXLucyRadio(fx3class *fx3)
    : RadioHardware(fx3)
{
    // initialize steps
    for(auto it = if_steps_hf.begin(); it != if_steps_hf.end(); it++)
    {
        int index = it - if_steps_hf.begin();
        int rf_scale = if_steps_hf.size() - index - 1;
        *it = -(
            ((rf_scale & 0x01) != 0) * 0.5f +
            ((rf_scale & 0x02) != 0) * 1.0f +
            ((rf_scale & 0x04) != 0) * 2.0f +
            ((rf_scale & 0x08) != 0) * 4.0f +
            ((rf_scale & 0x010) != 0) * 8.0f +
            ((rf_scale & 0x020) != 0) * 16.0f);
    }

    for (auto it = rf_steps_hf.begin(); it != rf_steps_hf.end(); it++)
    {
        int index = it - rf_steps_hf.begin();
        *it = -1.0f * (rf_steps_hf.size() - index - 1);
    }
}


sddc_rf_mode_t RXLucyRadio::GetBestRFMode(uint64_t freq)
{
    if (freq < 35000ll * 1000) return NOMODE;
    if (freq > 6000ll * 1000 * 1000) return NOMODE;

    if ( freq >= this->sampleRate / 2)
        return VHFMODE;
    else
        return HFMODE;
}


sddc_err_t RXLucyRadio::SetRFAttenuation_HF(int att)
{
    if (att >= rf_steps_hf.size()) att = rf_steps_hf.size() - 1;
    if (att < 0) att = 0;
    uint8_t d = rf_steps_hf.size() - att - 1;

    DbgPrintf("UpdateattRF %f \n", this->rf_steps_hf[att]);
    return Fx3->SetArgument(VHF_ATTENUATOR, d) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}
sddc_err_t RXLucyRadio::SetRFAttenuation_VHF(uint16_t)
{
    return ERR_NOT_COMPATIBLE;
}

sddc_err_t RXLucyRadio::SetIFGain_HF(int att)  //HF103 now
{
    if (att >= if_steps_hf.size()) att = if_steps_hf.size() - 1;
    if (att < 0) att = 0;
    uint8_t d = if_steps_hf.size() - att - 1;

    DbgPrintf("UpdateattRF %f \n", this->if_steps_hf[att]);

    return Fx3->SetArgument(DAT31_ATT, d) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}
sddc_err_t RXLucyRadio::SetIFGain_VHF(int att)
{
    return ERR_NOT_COMPATIBLE;
}

uint32_t RXLucyRadio::GetTunerFrequency_HF()
{
    return 0;
}
sddc_err_t RXLucyRadio::SetCenterFrequency_HF(uint32_t freq)
{
    return ERR_NOT_COMPATIBLE;
}
uint32_t RXLucyRadio::GetTunerFrequency_VHF()
{
    return IF_FREQ;
}
sddc_err_t RXLucyRadio::SetCenterFrequency_VHF(uint32_t freq)
{
    if(!Fx3->Control(TUNERTUNE, freq + IF_FREQ))
        return ERR_FX3_TRANSFER_FAILED;

    freqLO_VHF = freq;

    return ERR_SUCCESS;
}

sddc_err_t RXLucyRadio::SetRFMode(sddc_rf_mode_t mode)
{
    if (mode == VHFMODE)
    {
        // switch to VHF Attenna
        sddc_err_t ret = SetGPIO(VHF_EN);
        if(ret != ERR_SUCCESS) return ret;

        // Initialize VCO

        // Initialize Mixer
        return Fx3->Control(TUNERINIT, (uint32_t)0) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
    }
    else if (mode == HFMODE)
    {
        if(!Fx3->Control(TUNERSTDBY))
            return ERR_FX3_TRANSFER_FAILED;

        return UnsetGPIO(VHF_EN);                // switch to HF Attenna
    }
    return ERR_NOT_COMPATIBLE;
}

vector<float> RXLucyRadio::GetRFSteps_HF()
{
    return this->rf_steps_hf;
}
vector<float> RXLucyRadio::GetRFSteps_VHF()
{
    return vector<float>();
}

vector<float> RXLucyRadio::GetIFSteps_HF()
{
    return this->if_steps_hf;
}
vector<float> RXLucyRadio::GetIFSteps_VHF()
{
    return vector<float>();
}


