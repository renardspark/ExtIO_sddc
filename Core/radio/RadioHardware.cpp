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

#include "RadioHardware.h"

#include <cstdio>

sddc_rf_mode_t RadioHardware::GetRFMode()
{
    return currentRFMode;
}

uint32_t RadioHardware::GetADCSampleRate()
{
    return sampleRate;
}
sddc_err_t RadioHardware::SetADCSampleRate(uint32_t adc_rate)
{
    sampleRate = adc_rate;
    return Fx3->Control(STARTADC, adc_rate) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

// --- Gain --- //
uint16_t RadioHardware::GetRF_HF()
{
    return attenuationHFStep;
}
uint16_t RadioHardware::GetRF_VHF()
{
    return attenuationVHFStep;
}
uint16_t RadioHardware::GetIF_HF()
{
    return gainHFStep;
}
uint16_t RadioHardware::GetIF_VHF()
{
    return gainVHFStep;
}

// --- Misc --- //
bool RadioHardware::GetDither()
{
    return stateDither;
}
sddc_err_t RadioHardware::SetDither(bool new_state)
{
    stateDither = new_state;
    if (stateDither)
        return SetGPIO(DITH);
    else
        return UnsetGPIO(DITH);
}

bool RadioHardware::GetPGA()
{
    return statePGA;
}
sddc_err_t RadioHardware::SetPGA(bool new_state)
{
    statePGA = new_state;
    if (statePGA)
        return SetGPIO(PGA_EN);
    else
        return UnsetGPIO(PGA_EN);
}

bool RadioHardware::GetRand()
{
    return stateRand;
}
sddc_err_t RadioHardware::SetRand(bool new_state)
{
    stateRand = new_state;
    if (stateRand)
        return SetGPIO(RANDO);
    else
        return UnsetGPIO(RANDO);
}

// ----- Bias T ----- //
bool RadioHardware::GetBiasT_HF()
{
    return stateBiasT_HF;
}
sddc_err_t RadioHardware::SetBiasT_HF(bool new_state) 
{
    stateBiasT_HF = new_state;

    if (stateBiasT_HF)
        return SetGPIO(BIAS_HF);
    else
        return UnsetGPIO(BIAS_HF);
}

bool RadioHardware::GetBiasT_VHF()
{
    return stateBiasT_VHF;
}
sddc_err_t RadioHardware::SetBiasT_VHF(bool new_state)
{
    stateBiasT_VHF = new_state;
    if (stateBiasT_VHF)
        return SetGPIO(BIAS_VHF);
    else
        return UnsetGPIO(BIAS_VHF);
}
// ----- //

// ----- Tuner ----- //
uint32_t RadioHardware::GetCenterFrequency_HF()
{
    return freqLO_HF;
}
uint32_t RadioHardware::GetCenterFrequency_VHF()
{
    return freqLO_VHF;
}

// ----- GPIOs ----- //
sddc_err_t RadioHardware::SetGPIO(uint32_t mask)
{
    gpios |= mask;

    return Fx3->Control(GPIOFX3, gpios) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

sddc_err_t RadioHardware::UnsetGPIO(uint32_t mask)
{
    gpios &= ~mask;

    return Fx3->Control(GPIOFX3, gpios) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED;
}

/**
 * @brief Change the state of an LED
 * 
 * @param[in] led The LED to change
 * @param[in] on  The new LED state
 */
sddc_err_t RadioHardware::SetLED(sddc_leds_t led, bool on)
{
    int pin;
    switch(led)
    {
        case sddc_leds_t::SDDC_LED_YELLOW:
            pin = LED_YELLOW;
            break;
        case sddc_leds_t::SDDC_LED_RED:
            pin = LED_RED;
            break;
        case sddc_leds_t::SDDC_LED_BLUE:
            pin = LED_BLUE;
            break;
        default:
            return ERR_NOT_LED;
    }

    if (on)
        return SetGPIO(pin);
    else
        return UnsetGPIO(pin);
}
// ----- //

RadioHardware::~RadioHardware()
{
    if (Fx3) {
        SetGPIO(SHDWN);
    }
}