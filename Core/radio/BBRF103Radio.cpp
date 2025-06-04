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

#include "../RadioHandler.h"

#define R820T_FREQ (32000000)	// R820T reference frequency
#define R820T2_IF_CARRIER (4570000)

const vector<float> BBRF103Radio::rf_steps_vhf =  {
    0.0f, 0.9f, 1.4f, 2.7f, 3.7f, 7.7f, 8.7f, 12.5f, 14.4f, 15.7f,
    16.6f, 19.7f, 20.7f, 22.9f, 25.4f, 28.0f, 29.7f, 32.8f,
    33.8f, 36.4f, 37.2f, 38.6f, 40.2f, 42.1f, 43.4f, 43.9f,
    44.5f, 48.0f, 49.6f
};

const vector<float> BBRF103Radio::if_steps_vhf =  {
    -4.7f, -2.1f, 0.5f, 3.5f, 7.7f, 11.2f, 13.6f, 14.9f, 16.3f, 19.5f, 23.1f, 26.5f, 30.0f, 33.7f, 37.2f, 40.8f
};

const vector<float> BBRF103Radio::rf_steps_hf = {
    -20.0f, -10.0f, 0.0f
};

BBRF103Radio::BBRF103Radio(fx3class* fx3):
    RadioHardware(fx3)
{
    
}

sddc_rf_mode_t BBRF103Radio::GetBestRFMode(uint64_t freq)
{
    if (freq < 10 * 1000) return NOMODE;
    if (freq > 1750 * 1000 * 1000) return NOMODE;

    if ( freq >= this->sampleRate / 2)
        return VHFMODE;
    else
        return HFMODE;
}

sddc_err_t BBRF103Radio::SetRFMode(sddc_rf_mode_t mode)
{
    if (mode == VHFMODE)
    {
        // switch to VHF Attenna
        UnsetGPIO(ATT_SEL0 | ATT_SEL1);

        // Initialize Tuner
        return Fx3->Control(TUNERINIT, (uint32_t)R820T_FREQ) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
    }

    else if (mode == HFMODE )   // (mode == HFMODE || mode == VLFMODE) no more VLFMODE
    {
        // Stop Tuner
        Fx3->Control(TUNERSTDBY);

        // switch to HF Attenna
        return SetGPIO(ATT_SEL0 | ATT_SEL1) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
    }

    return ERR_NOT_COMPATIBLE;
}

sddc_err_t BBRF103Radio::SetRFAttenuation_HF(int att)
{
    if (att > 2) att = 2;
    if (att < 0) att = 0;
    switch (att)
    {
    case 1: //11
        gpios |= ATT_SEL0 | ATT_SEL1;
        break;
    case 0: //01
        gpios |=  ATT_SEL0;
        gpios &=  ~ATT_SEL1;
        break;
    case 2:   //10
    default:
        gpios |=  ATT_SEL1;
        gpios &=  ~ATT_SEL0;
        break;
    }
    return Fx3->Control(GPIOFX3, gpios) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}
sddc_err_t BBRF103Radio::SetRFAttenuation_VHF(uint16_t att)
{
    return Fx3->SetArgument(R82XX_ATTENUATOR, att) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

sddc_err_t BBRF103Radio::SetCenterFrequency_HF(uint32_t freq)
{
    return ERR_NOT_COMPATIBLE;
}

uint32_t BBRF103Radio::GetTunerFrequency_HF()
{
    return 0;
}
uint32_t BBRF103Radio::GetTunerFrequency_VHF()
{
    return R820T2_IF_CARRIER;
}
sddc_err_t BBRF103Radio::SetCenterFrequency_VHF(uint32_t freq)
{
    if(!Fx3->Control(TUNERTUNE, freq))
        return ERR_FX3_TRANSFER_FAILED;

    freqLO_VHF = freq;

    return ERR_SUCCESS;
}

vector<float> BBRF103Radio::GetRFSteps_HF()
{
    return this->rf_steps_hf;
}
vector<float> BBRF103Radio::GetRFSteps_VHF()
{
    return this->rf_steps_vhf;
}

vector<float> BBRF103Radio::GetIFSteps_HF()
{
    return vector<float>();
}
vector<float> BBRF103Radio::GetIFSteps_VHF()
{
    return this->if_steps_vhf;
}

sddc_err_t BBRF103Radio::SetIFGain_HF(int attIndex)
{
    return ERR_NOT_COMPATIBLE;
}

sddc_err_t BBRF103Radio::SetIFGain_VHF(int attIndex)
{
    return Fx3->SetArgument(R82XX_VGA, (uint16_t)attIndex) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}