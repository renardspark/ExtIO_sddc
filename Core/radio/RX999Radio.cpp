/*
 * This file is part of SDDC_Driver.
 *
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

#include "../RadioHandler.h"

#define ADC_FREQ (128u*1000*1000)
#define IF_FREQ (ADC_FREQ / 4)

#define HIGH_MODE 0x80
#define LOW_MODE 0x00

#define MODE HIGH_MODE

RX999Radio::RX999Radio(fx3class *fx3)
    : RadioHardware(fx3)
{
    // high mode gain = 0.409, start=-30
    // low mode gain = 0.059, start = -30
#if (MODE == HIGH_MODE)
    float ratio = 0.409f;
#else
    float ratio = 0.059f;
#endif
    for (auto it = if_steps_hf.begin(); it != if_steps_hf.end(); it++)
    {
        int index = it - if_steps_hf.begin();
        *it = -30.0f + ratio * (index + 1);
    }
}


sddc_rf_mode_t RX999Radio::GetBestRFMode(uint64_t freq)
{
    if (freq < 10 * 1000) return NOMODE;
    if (freq > 6000ll * 1000 * 1000) return NOMODE;

    if ( freq >= this->sampleRate / 2)
        return VHFMODE;
    else
        return HFMODE;
}

sddc_err_t RX999Radio::SetRFMode(sddc_rf_mode_t mode)
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


sddc_err_t RX999Radio::SetRFAttenuation_HF(int att)
{
    return ERR_NOT_COMPATIBLE;
}
sddc_err_t RX999Radio::SetRFAttenuation_VHF(uint16_t)
{
    return ERR_NOT_COMPATIBLE;
}

uint32_t RX999Radio::GetTunerFrequency_HF()
{
    return freqLO_HF;
}
sddc_err_t RX999Radio::SetCenterFrequency_HF(uint32_t freq)
{
    return ERR_NOT_COMPATIBLE;
}
uint32_t RX999Radio::GetTunerFrequency_VHF()
{
    return IF_FREQ;
}
sddc_err_t RX999Radio::SetCenterFrequency_VHF(uint32_t freq)
{
    int sel;
    // set preselector
    if (freq <= 120*1000*1000) sel = 0b111;
    else if (freq <= 250*1000*1000) sel = 0b101;
    else if (freq <= 300*1000*1000) sel = 0b110;
    else if (freq <= 380*1000*1000) sel = 0b100;
    else if (freq <= 500*1000*1000) sel = 0b000;
    else if (freq <= 1000ll*1000*1000) sel = 0b010;
    else if (freq <= 2000ll*1000*1000) sel = 0b001;
    else sel = 0b011;

    if(!Fx3->Control(TUNERTUNE, freq + IF_FREQ))
        return ERR_FX3_TRANSFER_FAILED;

    if(!Fx3->SetArgument(PRESELECTOR, sel))
        return ERR_FX3_TRANSFER_FAILED;

    freqLO_VHF = freq;
    return ERR_SUCCESS;
}

vector<float> RX999Radio::GetRFSteps_HF()
{
    return vector<float>();
}
vector<float> RX999Radio::GetRFSteps_VHF()
{
    return vector<float>();
}

vector<float> RX999Radio::GetIFSteps_HF()
{
    return this->if_steps_hf;
}
vector<float> RX999Radio::GetIFSteps_VHF()
{
    return GetIFSteps_HF();
}

sddc_err_t RX999Radio::SetIFGain_HF(int gain_index)
{
    uint8_t gain = MODE | (gain_index + 1);

    DbgPrintf("UpdateGainIF %d \n", gain);

    return Fx3->SetArgument(AD8340_VGA, gain) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

sddc_err_t RX999Radio::SetIFGain_VHF(int gain_index)
{
    return SetIFGain_HF(gain_index);
}
