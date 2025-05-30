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

HF103Radio::HF103Radio(fx3class* fx3)
    : RadioHardware(fx3)
{
    // initialize steps
    for (uint8_t i = 0 ; i < step_size; i++) {
        this->rf_steps_hf[step_size - i - 1] = -(
            ((i & 0x01) != 0) * 0.5f +
            ((i & 0x02) != 0) * 1.0f +
            ((i & 0x04) != 0) * 2.0f +
            ((i & 0x08) != 0) * 4.0f +
            ((i & 0x010) != 0) * 8.0f +
            ((i & 0x020) != 0) * 16.0f
        );
    }
}

sddc_rf_mode_t HF103Radio::GetBestRFMode(uint64_t freq)
{
    if(freq > 32 * 1000 * 1000) return NOMODE;

    return HFMODE;
}

sddc_err_t HF103Radio::SetRFMode(sddc_rf_mode_t mode)
{
    if (mode != HFMODE)
        return ERR_NOT_COMPATIBLE;

    currentRFMode = mode;
    return ERR_SUCCESS;
}

sddc_err_t HF103Radio::SetLOFreq_HF(uint32_t)
{
    return ERR_NOT_COMPATIBLE;
}
uint32_t HF103Radio::GetTunerCarrier_HF()
{
    return 0;
}
sddc_err_t HF103Radio::SetLOFreq_VHF(uint32_t)
{
    return ERR_NOT_COMPATIBLE;
}
uint32_t HF103Radio::GetTunerCarrier_VHF()
{
    return 0;
}

sddc_err_t HF103Radio::SetRFAttenuation_HF(int att)
{
    if (att > step_size - 1) att = step_size - 1;
    if (att < 0) att = 0;
    uint8_t d = step_size - att - 1;

    DbgPrintf("UpdateattRF %f \n", this->rf_steps_hf[att]);

    return Fx3->SetArgument(DAT31_ATT, d) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}
sddc_err_t HF103Radio::SetRFAttenuation_VHF(uint16_t)
{
    return ERR_NOT_COMPATIBLE;
}

vector<float> HF103Radio::GetRFSteps_HF()
{
    return this->rf_steps_hf;
}
vector<float> HF103Radio::GetRFSteps_VHF()
{
    return vector<float>();
}

vector<float> HF103Radio::GetIFSteps_HF ()
{
    return vector<float>();
}
vector<float> HF103Radio::GetIFSteps_VHF()
{
    return vector<float>();
}

sddc_err_t HF103Radio::SetIFGain_HF  (int attIndex)
{
    return ERR_NOT_COMPATIBLE;
}
sddc_err_t HF103Radio::SetIFGain_VHF (int attIndex)
{
    return ERR_NOT_COMPATIBLE;
}
