#include "SoapySDDC.hpp"
#include <SoapySDR/Types.hpp>
#include <SoapySDR/Time.hpp>
#include <cstdint>
#include <sys/types.h>
#include <cstring>

#define TAG "SoapySDDC_Settings"

static void _Callback(void *context, const sddc_complex_t *data, uint32_t len)
{
    SoapySDDC *sddc = (SoapySDDC *)context;
    sddc->Callback(data, len);
}

void SoapySDDC::Callback(const sddc_complex_t *data, uint32_t len)
{
    TraceExtremePrintln(TAG, "%p, %d", data, len);
    if (_buf_count == numBuffers)
    {
        _overflowEvent = true;
        return;
    }

    auto &buff = samples_buffer[samples_block_write];
    buff.resize(len * sizeof(sddc_complex_t));
    memcpy(buff.data(), data, len * sizeof(sddc_complex_t));

    samples_block_write++;
    samples_block_write %= numBuffers;

    {
        std::lock_guard<std::mutex> lock(_buf_mutex);
        _buf_count++;
    }
    _buf_cond.notify_one();

    return;
}

SoapySDDC::SoapySDDC(uint8_t dev_index): deviceId(-1),
                                        numBuffers(16)
{
    TracePrintln(TAG, "%d", dev_index);
    radio_handler = new RadioHandler();
    radio_handler->Init(dev_index);
    radio_handler->AttachIQ(_Callback, this);
}

SoapySDDC::~SoapySDDC(void)
{
    TracePrintln(TAG, "");
    radio_handler->Stop();

    delete radio_handler;
}

std::string SoapySDDC::getDriverKey(void) const
{
    TracePrintln(TAG, "");
    return "SDDC";
}

std::string SoapySDDC::getHardwareKey(void) const
{
    TracePrintln(TAG, "");
    return std::string(radio_handler->getHardwareName());
}

SoapySDR::Kwargs SoapySDDC::getHardwareInfo(void) const
{
    TracePrintln(TAG, "");
    // key/value pairs for any useful information
    // this also gets printed in --probe
    SoapySDR::Kwargs args;

    args["author"] = "RenardSpark";
    args["origin"] = "https://github.com/renardspark/SDDC_Driver";
    args["index"] = std::to_string(deviceId);

    return args;
}

/*******************************************************************
 * Channels API
 ******************************************************************/

size_t SoapySDDC::getNumChannels(const int dir) const
{
    TracePrintln(TAG, "%i", dir);
    return (dir == SOAPY_SDR_RX) ? 1 : 0;
}

SoapySDR::Kwargs SoapySDDC::getChannelInfo(const int direction, const size_t channel) const
{
    TracePrintln(TAG, "%i, %ld", direction, channel);
    // We could add infos about the channel here in the future
    return SoapySDR::Kwargs();
}

bool SoapySDDC::getFullDuplex(const int dir, const size_t chan) const
{
    TracePrintln(TAG, "%i, %ld", dir, chan);
    return false;
}

/*******************************************************************
 * Antenna API
 ******************************************************************/

std::vector<std::string> SoapySDDC::listAntennas(const int direction, const size_t) const
{
    TracePrintln(TAG, "%d", direction);
    std::vector<std::string> antennas;

    // No antennas available in TX, return early
    if (direction != SOAPY_SDR_TX)
    {
        return antennas;
    }

    antennas.push_back("HF");
    antennas.push_back("VHF");
    return antennas;
}

// set the selected antenna
void SoapySDDC::setAntenna(const int direction, const size_t, const std::string &name)
{
    TracePrintln(TAG, "%d, %s", direction, name.c_str());

    // No antennas available in TX, return early
    if (direction != SOAPY_SDR_RX)
    {
        return;
    }

    if(name == "HF")
    {
        radio_handler->SetRFMode(HFMODE);
        return;
    }

    if(name == "VHF")
    {
        radio_handler->SetRFMode(VHFMODE);
        return;
    }
}

// get the selected antenna
std::string SoapySDDC::getAntenna(const int direction, const size_t) const
{
    TracePrintln(TAG, "%d", direction);

    // No antennas available in TX, return early
    if (direction != SOAPY_SDR_RX)
    {
        return "Unavailable";
    }

    switch(radio_handler->GetRFMode())
    {
        case VHFMODE:
            return "VHF";
        case HFMODE:
            return "HF";
        default:
            return "Unknown";
    }
}

bool SoapySDDC::hasDCOffset(const int, const size_t) const
{
    TracePrintln(TAG, "");
    return false;
}

bool SoapySDDC::hasDCOffsetMode(const int, const size_t) const
{
    TracePrintln(TAG, "");
    return false;
}

bool SoapySDDC::hasFrequencyCorrection(const int, const size_t) const
{
    TracePrintln(TAG, "");
    return false;
}

bool SoapySDDC::hasIQBalance(const int, const size_t) const
{
    TracePrintln(TAG, "");
    return false;
}

bool SoapySDDC::hasIQBalanceMode(const int, const size_t) const
{
    TracePrintln(TAG, "");
    return false;
}

std::vector<std::string> SoapySDDC::listGains(const int, const size_t) const
{
    TracePrintln(TAG, "");
    std::vector<std::string> gains;
    gains.push_back("HF - RF");
    gains.push_back("HF - IF");

    gains.push_back("VHF - RF");
    gains.push_back("VHF - IF");
    return gains;
}

bool SoapySDDC::hasGainMode(const int direction, const size_t channel) const
{
    TracePrintln(TAG, "%i, %ld", direction, channel);
    return false;
}

double SoapySDDC::getGain(const int direction, const size_t channel, const std::string &name) const
{
    TracePrintln(TAG, "%i, %ld, %s", direction, channel, name.c_str());

    if(name == "HF - RF") {
        return radio_handler->GetRFGain(HFMODE);
    }
    else if(name == "HF - IF") {
        return radio_handler->GetIFGain(HFMODE);
    }
    else if(name == "VHF - RF") {
        return radio_handler->GetRFGain(VHFMODE);
    }
    else if(name == "VHF - IF") {
        return radio_handler->GetIFGain(VHFMODE);
    }
    return 0;
}
void SoapySDDC::setGain(const int direction, const size_t channel, const std::string &name, const double value)
{
    TracePrintln(TAG, "%i, %ld, %s, %f", direction, channel, name.c_str(), value);

    if (name == "HF - RF") {
        radio_handler->SetRFGain(value, HFMODE);
    }
    else if(name == "HF - IF") {
        radio_handler->SetIFGain(value, HFMODE);
    }
    else if(name == "VHF - RF") {
        radio_handler->SetRFGain(value, VHFMODE);
    }
    else if(name == "VHF - IF") {
        radio_handler->SetIFGain(value, VHFMODE);
    }
}

SoapySDR::Range SoapySDDC::getGainRange(const int direction, const size_t channel, const std::string &name) const
{
    TracePrintln(TAG, "%i, %ld, %s", direction, channel, name.c_str());

    if (name == "HF - RF") {
        array<float, 2> gain_range = radio_handler->GetRFGainRange(HFMODE);

        return SoapySDR::Range(
            gain_range.front(),
            gain_range.back()
        );
    }
    else if (name == "HF - IF") {
        array<float, 2> gain_range = radio_handler->GetIFGainRange(HFMODE);

        return SoapySDR::Range(
            gain_range.front(),
            gain_range.back()
        );
    }
    else if (name == "VHF - RF") {
        array<float, 2> gain_range = radio_handler->GetRFGainRange(VHFMODE);

        return SoapySDR::Range(
            gain_range.front(),
            gain_range.back()
        );
    }
    else if (name == "VHF - IF") {
        array<float, 2> gain_range = radio_handler->GetIFGainRange(VHFMODE);

        return SoapySDR::Range(
            gain_range.front(),
            gain_range.back()
        );
    }
    
    return SoapySDR::Range();
}

void SoapySDDC::setFrequency(const int direction, const size_t channel, const double frequency, const SoapySDR::Kwargs &)
{
    TracePrintf("SoapySDDC::setFrequency(%i, %ld, %f)\n", direction, channel, frequency);
    radio_handler->SetRFMode(radio_handler->GetBestRFMode(frequency));
    radio_handler->SetCenterFrequency((uint32_t)frequency);
    centerFrequency = radio_handler->GetCenterFrequency();
}

void SoapySDDC::setFrequency(const int direction, const size_t channel, const string &name, const double frequency, const SoapySDR::Kwargs &args)
{
    TracePrintf("SoapySDDC::setFrequency(%i, %ld, %s, %f)\n", direction, channel, name.c_str(), frequency);
    setFrequency(direction, channel, frequency, args);
}

double SoapySDDC::getFrequency(const int direction, const size_t channel) const
{
    TracePrintf("SoapySDDC::getFrequency(%i, %ld)\n", direction, channel);

    return (double)centerFrequency;
}

double SoapySDDC::getFrequency(const int direction, const size_t channel, const std::string &name) const
{
    TracePrintf("SoapySDDC::getFrequency(%i, %ld, %s)\n", direction, channel, name.c_str());
    return (double)centerFrequency;
}

std::vector<std::string> SoapySDDC::listFrequencies(const int direction, const size_t channel) const
{
    TracePrintln(TAG, "%i, %ld", direction, channel);
    std::vector<std::string> ret;

    if(direction != SOAPY_SDR_RX)
    {
        return ret;
    }

    if (channel == 0) {
        ret.push_back("HF");
    }

    return ret;
}

SoapySDR::RangeList SoapySDDC::getFrequencyRange(const int direction, const size_t channel) const
{
    TracePrintln(TAG, "%i, %ld", direction, channel);

    SoapySDR::RangeList ranges;

    ranges.push_back(SoapySDR::Range(10000, 1800000000));

    return ranges;
}

SoapySDR::RangeList SoapySDDC::getFrequencyRange(const int direction, const size_t channel, const std::string &name) const
{
    TracePrintln(TAG, "%i, %ld, %s", direction, channel, name.c_str());

    SoapySDR::RangeList ranges;

    if(name == "HF")
    {
        ranges.push_back(SoapySDR::Range(10000, 1800000000));
    }
    /*else if(name == "VHF")
    {
        ranges.push_back(SoapySDR::Range(64000000, 1800000000));
    }*/

    return ranges;
}

SoapySDR::ArgInfoList SoapySDDC::getFrequencyArgsInfo(const int direction, const size_t channel) const
{
    TracePrintln(TAG, "%i, %ld", direction, channel);
    return SoapySDR::ArgInfoList();
}

void SoapySDDC::setSampleRate(const int direction, const size_t channel, const double rate)
{
    TracePrintln(TAG, "%i, %ld, %f", direction, channel, rate);
    
    radio_handler->SetADCSampleRate(rate*2);
}

double SoapySDDC::getSampleRate(const int direction, const size_t channel) const
{
    TracePrintln(TAG, "%i, %ld", direction, channel);
    return radio_handler->GetADCSampleRate()/2;
}

SoapySDR::RangeList SoapySDDC::getSampleRateRange(const int direction, const size_t channel) const
{
    TracePrintln(TAG, "%i, %ld", direction, channel);

    SoapySDR::RangeList ranges;

    ranges.push_back(SoapySDR::Range(1000000/2, 160000000/2));

    return ranges;
}


vector<string> SoapySDDC::listSensors() const 
{
    TracePrintln(TAG, "");

    return vector<string>{
        "RFMode"
    };
}

SoapySDR::ArgInfo SoapySDDC::getSensorInfo(const string &key) const
{
    TracePrintln(TAG, "%s", key.c_str());

    if(key == "RFMode")
    {
        SoapySDR::ArgInfo arg;
        arg.key = "RFMode";
        arg.value = readSensor(key);
        arg.name = "RF Mode";
        arg.description = "Current RF mode";
        arg.type = SoapySDR::ArgInfo::STRING;
        return arg;
    }

    return SoapySDR::ArgInfo();
}

string SoapySDDC::readSensor(const string &key) const
{
    TracePrintln(TAG, "%s", key.c_str());

    if(key == "RF mode")
    {
        return radio_handler->GetRFMode() == VHFMODE ? "VHF" : "HF";
    }
    return "";
}


SoapySDR::ArgInfoList SoapySDDC::getSettingInfo(void) const
{
    TracePrintln(TAG, "");

    SoapySDR::ArgInfoList setArgs;

    SoapySDR::ArgInfo BiasTHFArg;
    BiasTHFArg.key = "SetBiasT_HF";
    BiasTHFArg.value = "false";
    BiasTHFArg.name = "HF Bias Tee";
    BiasTHFArg.description = "Enabe Bias Tee on HF antenna port";
    BiasTHFArg.type = SoapySDR::ArgInfo::BOOL;
    setArgs.push_back(BiasTHFArg);

    SoapySDR::ArgInfo BiasTVHFArg;
    BiasTVHFArg.key = "SetBiasT_VHF";
    BiasTVHFArg.value = "false";
    BiasTVHFArg.name = "VHF Bias Tee";
    BiasTVHFArg.description = "Enabe Bias Tee on VHF antenna port";
    BiasTVHFArg.type = SoapySDR::ArgInfo::BOOL;
    setArgs.push_back(BiasTVHFArg);

    SoapySDR::ArgInfo dither;
    dither.key = "SetDither";
    dither.value = "false";
    dither.name = "Dither";
    dither.description = "Enable dither";
    dither.type = SoapySDR::ArgInfo::BOOL;
    setArgs.push_back(dither);

    SoapySDR::ArgInfo pga;
    pga.key = "SetPGA";
    pga.value = "false";
    pga.name = "PGA";
    pga.description = "Enable Programmable Gain Amplifier";
    pga.type = SoapySDR::ArgInfo::BOOL;
    setArgs.push_back(pga);

    SoapySDR::ArgInfo rand;
    rand.key = "SetRand";
    rand.value = "false";
    rand.name = "ADC random";
    rand.description = "Enable ADC random generation";
    rand.type = SoapySDR::ArgInfo::BOOL;
    setArgs.push_back(rand);

    return setArgs;
}

void SoapySDDC::writeSetting(const std::string &key, const std::string &value)
{
    TracePrintln(TAG, "%s, %s", key.c_str(), value.c_str());

    if(key == "SetBiasT_HF")
    {
        bool biasTee = (value == "true") ? true : false;
        radio_handler->SetBiasT_HF(biasTee);
    }
    else if(key == "SetBiasT_VHF")
    {
        bool biasTee = (value == "true") ? true : false;
        radio_handler->SetBiasT_VHF(biasTee);
    }

    else if(key == "SetDither")
    {
        bool dither = (value == "true") ? true : false;
        radio_handler->SetDither(dither);
    }

    else if(key == "SetPGA")
    {
        bool pga = (value == "true") ? true : false;
        radio_handler->SetPGA(pga);
    }

    else if(key == "SetRand")
    {
        bool rand = (value == "true") ? true : false;
        radio_handler->SetRand(rand);
    }
}


// void SoapySDDC::setMasterClockRate(const double rate)
// {
//     DbgPrintf("SoapySDDC::setMasterClockRate %f\n", rate);
//     masterClockRate = rate;
// }

// double SoapySDDC::getMasterClockRate(void) const
// {
//     DbgPrintf("SoapySDDC::getMasterClockRate %f\n", masterClockRate);
//     return masterClockRate;
// }

// std::vector<std::string> SoapySDDC::listTimeSources(void) const
// {
//     DbgPrintf("SoapySDDC::listTimeSources\n");
//     std::vector<std::string> sources;
//     sources.push_back("sw_ticks");
//     return sources;
// }

// std::string SoapySDDC::getTimeSource(void) const
// {
//     DbgPrintf("SoapySDDC::getTimeSource\n");
//     return "sw_ticks";
// }

// bool SoapySDDC::hasHardwareTime(const std::string &what) const
// {
//     DbgPrintf("SoapySDDC::hasHardwareTime\n");
//     return what == "" || what == "sw_ticks";
// }

// long long SoapySDDC::getHardwareTime(const std::string &what) const
// {
//     DbgPrintf("SoapySDDC::getHardwareTime\n");
//     return SoapySDR::ticksToTimeNs(ticks, sampleRate);
// }

// void SoapySDDC::setHardwareTime(const long long timeNs, const std::string &what)
// {
//     DbgPrintf("SoapySDDC::setHardwareTime\n");
//     ticks = SoapySDR::timeNsToTicks(timeNs, sampleRate);
// }
