/*
 * This file is part of SDDC_Driver.
 *
 * Copyright (C) 2020 - Howard Su
 * Copyright (C) 2021 - Oscar Steila
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

#ifndef _H_RADIOHARDWARE
#define _H_RADIOHARDWARE

#include "../config.h"
#include "../FX3Class.h"

using namespace std;

class RadioHardware {
    public:
        RadioHardware(fx3class* fx3): Fx3(fx3) {}
        virtual ~RadioHardware();

        // ----- Common methods ----- //
        sddc_rf_mode_t GetRFMode();
        virtual sddc_rf_mode_t GetBestRFMode(uint64_t freq) = 0;
        virtual sddc_err_t SetRFMode(sddc_rf_mode_t mode) = 0;

        // --- ADC --- //
        uint32_t    GetADCSampleRate();
        sddc_err_t  SetADCSampleRate(uint32_t samplefreq);

        // --- Bias T --- //
        bool        GetBiasT_HF ();
        sddc_err_t  SetBiasT_HF (bool new_state);
        bool        GetBiasT_VHF();
        sddc_err_t  SetBiasT_VHF(bool new_state);

        // --- Gain --- //
        uint16_t GetRF_HF();
        uint16_t GetRF_VHF();
        uint16_t GetIF_HF();
        uint16_t GetIF_VHF();

        // --- Tuner --- //
        uint32_t    GetLOFreq_HF ();
        uint32_t    GetLOFreq_VHF();

        // --- Misc --- //
        bool        GetDither();
        sddc_err_t  SetDither(bool new_state);
        bool        GetPGA ();
        sddc_err_t  SetPGA (bool new_state);
        bool        GetRand();
        sddc_err_t  SetRand(bool new_state);

        // --- Data stream --- //
        sddc_err_t StartStream() { return Fx3->Control(STARTFX3) ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED; }
        sddc_err_t StopStream () { return Fx3->Control(STOPFX3)  ? ERR_SUCCESS : ERR_FX3_TRANSFER_FAILED; }

        // --- GPIOs --- //
        sddc_err_t SetGPIO  (uint32_t mask);
        sddc_err_t UnsetGPIO(uint32_t mask);
        sddc_err_t SetLED   (sddc_leds_t led, bool on);

        bool ReadDebugTrace(uint8_t* pdata, uint8_t len) { return Fx3->ReadDebugTrace(pdata, len); }
        // ----- //

        // ----- Custom methods ----- //
        virtual const char* GetName() = 0;
        virtual float getGain() = 0;

        // --- Tuner --- //
        virtual sddc_err_t  SetLOFreq_HF (uint32_t freq) = 0;
        virtual sddc_err_t  SetLOFreq_VHF(uint32_t freq) = 0;
        virtual uint32_t    GetTunerCarrier_HF() = 0;
        virtual uint32_t    GetTunerCarrier_VHF() = 0;

        // --- RF settings --- //
        virtual vector<float> GetRFSteps_HF () = 0;
        virtual vector<float> GetRFSteps_VHF() = 0;
        virtual sddc_err_t SetRFAttenuation_HF (int attIndex) = 0;
        virtual sddc_err_t SetRFAttenuation_VHF(uint16_t attIndex) = 0;

        // --- IF settings --- //
        virtual vector<float> GetIFSteps_HF () = 0;
        virtual vector<float> GetIFSteps_VHF() = 0;
        virtual sddc_err_t SetIFGain_HF  (int attIndex) = 0;
        virtual sddc_err_t SetIFGain_VHF (int attIndex) = 0;

        
    protected:
        fx3class* Fx3;
        uint32_t gpios = 0;

        uint32_t sampleRate;
        sddc_rf_mode_t currentRFMode = NOMODE;

        bool stateDither    = false;
        bool statePGA       = false;
        bool stateRand      = false;
        bool stateBiasT_HF  = false;
        bool stateBiasT_VHF = false;

        uint16_t attenuationHFStep = 0;
        uint16_t attenuationVHFStep = 0;
        uint16_t gainHFStep = 0;
        uint16_t gainVHFStep = 0;

        uint32_t freqLO_HF  = 0;
        uint32_t freqLO_VHF = 0;
};

class BBRF103Radio : public RadioHardware {
    public:
        BBRF103Radio(fx3class* fx3);

        const char* GetName() override { return "BBRF103"; }
        float getGain() override { return BBRF103_GAINFACTOR; }

        // --- Tuner --- //
        sddc_err_t  SetLOFreq_HF (uint32_t freq) override;
        sddc_err_t  SetLOFreq_VHF(uint32_t freq) override;
        uint32_t    GetTunerCarrier_HF() override;
        uint32_t    GetTunerCarrier_VHF() override;

        // --- RF settings --- //
        sddc_rf_mode_t GetBestRFMode(uint64_t freq) override;
        sddc_err_t SetRFMode(sddc_rf_mode_t mode) override;
        vector<float> GetRFSteps_HF () override;
        vector<float> GetRFSteps_VHF() override;
        sddc_err_t SetRFAttenuation_HF (int attIndex) override;
        sddc_err_t SetRFAttenuation_VHF(uint16_t attIndex) override;

        // --- IF settings --- //
        vector<float> GetIFSteps_HF () override;
        sddc_err_t SetIFGain_HF (int attIndex) override;
        vector<float> GetIFSteps_VHF() override;
        sddc_err_t SetIFGain_VHF (int attIndex) override;

    private:
        static const vector<float> rf_steps_vhf;
        static const vector<float> rf_steps_hf;
        static const vector<float> if_steps_vhf;
};

class RX888Radio : public BBRF103Radio {
public:
    RX888Radio(fx3class* fx3) : BBRF103Radio(fx3) {}
    const char* GetName() override { return "RX888"; }
    float getGain() override { return RX888_GAINFACTOR; }
};

class RX888R2Radio : public RadioHardware {
    public:
        RX888R2Radio(fx3class* fx3);
        const char* GetName() override { return "RX888 mkII"; }
        float getGain() override { return RX888mk2_GAINFACTOR; }
        
        sddc_err_t  SetLOFreq_HF (uint32_t freq) override;
        sddc_err_t  SetLOFreq_VHF(uint32_t freq) override;
        uint32_t    GetTunerCarrier_HF() override;
        uint32_t    GetTunerCarrier_VHF() override;

        // --- RF settings --- //
        sddc_rf_mode_t GetBestRFMode(uint64_t freq) override;
        sddc_err_t SetRFMode(sddc_rf_mode_t mode) override;
        vector<float> GetRFSteps_HF() override;
        vector<float> GetRFSteps_VHF() override;
        sddc_err_t SetRFAttenuation_HF(int attIndex) override;
        sddc_err_t SetRFAttenuation_VHF(uint16_t attIndex) override;

        // --- IF settings --- //
        vector<float> GetIFSteps_HF () override;
        vector<float> GetIFSteps_VHF() override;
        sddc_err_t SetIFGain_HF  (int attIndex) override;
        sddc_err_t SetIFGain_VHF (int attIndex) override;

    private:
        vector<float> rf_steps_hf = vector<float>(64, 0);
        vector<float> if_steps_hf = vector<float>(127, 0);
        static const vector<float> rf_steps_vhf;
        static const vector<float> if_steps_vhf;
};

class RX888R3Radio : public RadioHardware {
    public:
        RX888R3Radio(fx3class* fx3);
        const char* GetName() override { return "RX888 mkIII"; }
        float getGain() override { return RX888mk2_GAINFACTOR; }

        // --- Tuner --- //
        sddc_err_t  SetLOFreq_HF (uint32_t freq) override;
        sddc_err_t  SetLOFreq_VHF(uint32_t freq) override;
        uint32_t    GetTunerCarrier_HF() override;
        uint32_t    GetTunerCarrier_VHF() override;

        // --- RF settings --- //
        sddc_rf_mode_t GetBestRFMode(uint64_t freq) override;
        sddc_err_t SetRFMode(sddc_rf_mode_t mode) override;
        vector<float> GetRFSteps_HF () override;
        vector<float> GetRFSteps_VHF() override;
        sddc_err_t SetRFAttenuation_HF (int attIndex) override;
        sddc_err_t SetRFAttenuation_VHF(uint16_t attIndex) override;

        // --- IF settings --- //
        vector<float> GetIFSteps_HF () override;
        sddc_err_t SetIFGain_HF  (int attIndex) override;
        vector<float> GetIFSteps_VHF() override;
        sddc_err_t SetIFGain_VHF (int attIndex) override;

    private:

        vector<float>  rf_steps_hf;
        vector<float>  if_steps_hf;
        static const vector<float> rf_steps_vhf;
        static const vector<float> if_steps_vhf;

        uint32_t freqLO_VHF_offset = 0;
};

class RX999Radio : public RadioHardware {
    public:
        RX999Radio(fx3class* fx3);
        const char* GetName() override { return "RX999"; }
        float getGain() override { return RX888_GAINFACTOR; }

        // --- Tuner --- //
        sddc_err_t  SetLOFreq_HF (uint32_t freq) override;
        sddc_err_t  SetLOFreq_VHF(uint32_t freq) override;
        uint32_t    GetTunerCarrier_HF() override;
        uint32_t    GetTunerCarrier_VHF() override;

        // --- RF settings --- //
        sddc_rf_mode_t GetBestRFMode(uint64_t freq) override;
        sddc_err_t SetRFMode(sddc_rf_mode_t mode) override;
        vector<float> GetRFSteps_HF () override;
        vector<float> GetRFSteps_VHF() override;
        sddc_err_t SetRFAttenuation_HF (int attIndex) override;
        sddc_err_t SetRFAttenuation_VHF(uint16_t attIndex) override;

        // --- IF settings --- //
        vector<float> GetIFSteps_HF () override;
        sddc_err_t SetIFGain_HF  (int attIndex) override;
        vector<float> GetIFSteps_VHF() override;
        sddc_err_t SetIFGain_VHF (int attIndex) override;

    private:
        vector<float> if_steps_hf;
};

class HF103Radio : public RadioHardware {
    public:
        HF103Radio(fx3class* fx3);
        const char* GetName() override { return "HF103"; }
        float getGain() override { return HF103_GAINFACTOR; }

        // --- Tuner --- //
        sddc_err_t  SetLOFreq_HF (uint32_t freq) override;
        sddc_err_t  SetLOFreq_VHF(uint32_t freq) override;
        uint32_t    GetTunerCarrier_HF() override;
        uint32_t    GetTunerCarrier_VHF() override;

        // --- RF settings --- //
        sddc_rf_mode_t GetBestRFMode(uint64_t freq) override;
        sddc_err_t SetRFMode(sddc_rf_mode_t mode) override;
        vector<float> GetRFSteps_HF () override;
        vector<float> GetRFSteps_VHF() override;
        sddc_err_t SetRFAttenuation_HF (int attIndex) override;
        sddc_err_t SetRFAttenuation_VHF(uint16_t attIndex) override;

        // --- IF settings --- //
        vector<float> GetIFSteps_HF () override;
        sddc_err_t SetIFGain_HF  (int attIndex) override;
        vector<float> GetIFSteps_VHF() override;
        sddc_err_t SetIFGain_VHF (int attIndex) override;

    private:
        static const int step_size = 64;
        vector<float> rf_steps_hf;
};

class RXLucyRadio : public RadioHardware {
    public:
        RXLucyRadio(fx3class* fx3);
        const char* GetName() override { return "Lucy"; }
        float getGain() override { return HF103_GAINFACTOR; }

        // --- Tuner --- //
        sddc_err_t  SetLOFreq_HF (uint32_t freq) override;
        sddc_err_t  SetLOFreq_VHF(uint32_t freq) override;
        uint32_t    GetTunerCarrier_HF() override;
        uint32_t    GetTunerCarrier_VHF() override;

        // --- RF settings --- //
        sddc_rf_mode_t GetBestRFMode(uint64_t freq) override;
        sddc_err_t SetRFMode(sddc_rf_mode_t mode) override;
        vector<float> GetRFSteps_HF () override;
        vector<float> GetRFSteps_VHF() override;
        sddc_err_t SetRFAttenuation_HF (int attIndex) override;
        sddc_err_t SetRFAttenuation_VHF(uint16_t attIndex) override;

        // --- IF settings --- //
        vector<float> GetIFSteps_HF () override;
        sddc_err_t SetIFGain_HF  (int attIndex) override;
        vector<float> GetIFSteps_VHF() override;
        sddc_err_t SetIFGain_VHF (int attIndex) override;

    private:
        vector<float> rf_steps_hf;
        vector<float> if_steps_hf;
};

class DummyRadio : public RadioHardware {
public:
    DummyRadio(fx3class* fx3) : RadioHardware(fx3) {}
    const char* GetName() override { return "Dummy"; }
    float getGain() override { return 0; }

    sddc_rf_mode_t GetBestRFMode(uint64_t) override { return HFMODE; }
    sddc_err_t SetRFMode(sddc_rf_mode_t) override { return ERR_SUCCESS; }

    sddc_err_t  SetLOFreq_HF (uint32_t) override { return ERR_SUCCESS; };
    sddc_err_t  SetLOFreq_VHF(uint32_t) override { return ERR_SUCCESS; };
    uint32_t    GetTunerCarrier_HF() override { return 0; };
    uint32_t    GetTunerCarrier_VHF() override { return 0; };

    vector<float> GetRFSteps_HF () override { return vector<float>(); };
    vector<float> GetRFSteps_VHF() override { return vector<float>(); };
    sddc_err_t SetRFAttenuation_HF (int) override { return ERR_SUCCESS; };
    sddc_err_t SetRFAttenuation_VHF(uint16_t) override { return ERR_SUCCESS; };

    // --- IF settings --- //
    vector<float> GetIFSteps_HF () override { return vector<float>(); };
    sddc_err_t SetIFGain_HF  (int) override { return ERR_SUCCESS; };
    vector<float> GetIFSteps_VHF() override { return vector<float>(); };
    sddc_err_t SetIFGain_VHF (int) override { return ERR_SUCCESS; };
};

#endif