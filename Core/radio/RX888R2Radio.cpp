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

#include "RadioHardware.h"

#define R828D_FREQ (16000000) // R820T reference frequency
#define R828D_IF_CARRIER (4570000)

#define HIGH_MODE 0x80
#define LOW_MODE 0x00

#define GAIN_SWEET_POINT 18
#define HIGH_GAIN_RATIO (0.409f)
#define LOW_GAIN_RATIO (0.059f)

const vector<float> RX888R2Radio::rf_steps_vhf = {
    0.0f, 0.9f, 1.4f, 2.7f, 3.7f, 7.7f, 8.7f, 12.5f, 14.4f, 15.7f,
    16.6f, 19.7f, 20.7f, 22.9f, 25.4f, 28.0f, 29.7f, 32.8f,
    33.8f, 36.4f, 37.2f, 38.6f, 40.2f, 42.1f, 43.4f, 43.9f,
    44.5f, 48.0f, 49.6f};

const vector<float> RX888R2Radio::if_steps_vhf = {
    -4.7f, -2.1f, 0.5f, 3.5f, 7.7f, 11.2f, 13.6f, 14.9f, 16.3f, 19.5f, 23.1f, 26.5f, 30.0f, 33.7f, 37.2f, 40.8f};

RX888R2Radio::RX888R2Radio(fx3class *fx3)
    : RadioHardware(fx3)
{
    for (auto it = this->rf_steps_hf.rbegin(); it != this->rf_steps_hf.rend(); it++)
    {
        int index = it-this->rf_steps_hf.rbegin();
        *it = -index / 2.0f;
    }

    for (auto it = this->if_steps_hf.begin(); it != this->if_steps_hf.end(); it++)
    {
        int index = it-this->if_steps_hf.begin();
        if (index > GAIN_SWEET_POINT)
            *it = 20.0f * log10f(HIGH_GAIN_RATIO * (index - GAIN_SWEET_POINT + 3));
        else
            *it = 20.0f * log10f(LOW_GAIN_RATIO * (index + 1));
    }
}

sddc_rf_mode_t RX888R2Radio::GetBestRFMode(uint64_t freq)
{
    if (freq < 10 * 1000) return NOMODE;
    if (freq > 1750 * 1000 * 1000) return NOMODE;

    if ( freq >= this->sampleRate / 2)
        return VHFMODE;
    else
        return HFMODE;
}

sddc_err_t RX888R2Radio::SetRFMode(sddc_rf_mode_t mode)
{
    if(mode == VHFMODE)
    {
        currentRFMode = mode;

        // disable HF by setting max ATT
        sddc_err_t ret = SetRFAttenuation_HF(0); // max att 0 -> -31.5 dB
        if(ret != ERR_SUCCESS) return ret;

        // switch to VHF Attenna
        ret = SetGPIO(VHF_EN);
        if(ret != ERR_SUCCESS) return ret;

        // high gain, 0db
        uint8_t gain = 0x80 | 3;
        if(!Fx3->SetArgument(AD8340_VGA, gain))
            return ERR_FX3_TRANSFER_FAILED;

        // Enable Tuner reference clock
        uint32_t ref = R828D_FREQ;
        return Fx3->Control(TUNERINIT, ref) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED; // Initialize Tuner
    }
    else if(mode == HFMODE)
    {
        currentRFMode = mode;

        if(!Fx3->Control(TUNERSTDBY))
            return ERR_FX3_TRANSFER_FAILED;

        return UnsetGPIO(VHF_EN);                // switch to HF Attenna
    }

    return ERR_NOT_COMPATIBLE;
}

sddc_err_t RX888R2Radio::SetRFAttenuation_HF(int att)
{
    // hf mode
    if (att >= rf_steps_hf.size())
        att = rf_steps_hf.size() - 1;
    if (att < 0)
        att = 0;
    uint8_t d = rf_steps_hf.size() - att - 1;

    attenuationHFStep = att;

    return Fx3->SetArgument(DAT31_ATT, d) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

sddc_err_t RX888R2Radio::SetRFAttenuation_VHF(uint16_t att)
{
    attenuationVHFStep = att;
    return Fx3->SetArgument(R82XX_ATTENUATOR, att) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

uint32_t RX888R2Radio::GetTunerCarrier_HF()
{
    return 0;
}
sddc_err_t RX888R2Radio::SetLOFreq_HF(uint32_t)
{
    return ERR_NOT_COMPATIBLE;
}


uint32_t RX888R2Radio::GetTunerCarrier_VHF()
{
    return R828D_IF_CARRIER;
}

sddc_err_t RX888R2Radio::SetLOFreq_VHF(uint32_t freq)
{
    if(!Fx3->Control(TUNERTUNE, freq))
        return ERR_FX3_TRANSFER_FAILED;

    freqLO_VHF = freq;

    return ERR_SUCCESS;
}

vector<float> RX888R2Radio::GetRFSteps_HF()
{
    return this->rf_steps_hf;
}

vector<float> RX888R2Radio::GetRFSteps_VHF()
{
    return this->rf_steps_vhf;
}

vector<float> RX888R2Radio::GetIFSteps_HF()
{
    return this->if_steps_hf;

}
vector<float> RX888R2Radio::GetIFSteps_VHF()
{
    return this->if_steps_vhf;
}

sddc_err_t RX888R2Radio::SetIFGain_HF(int gain_index)
{
    uint8_t gain;
    if (gain_index > GAIN_SWEET_POINT)
        gain = HIGH_MODE | (gain_index - GAIN_SWEET_POINT + 3);
    else
        gain = LOW_MODE | (gain_index + 1);

    gainHFStep = gain_index;

    return Fx3->SetArgument(AD8340_VGA, gain) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

sddc_err_t RX888R2Radio::SetIFGain_VHF(int gain_index)
{
    gainVHFStep = gain_index;
    return Fx3->SetArgument(R82XX_VGA, (uint16_t)gain_index) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}