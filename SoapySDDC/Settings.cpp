#include "SoapySDDC.hpp"
#include <SoapySDR/Types.hpp>
#include <SoapySDR/Time.hpp>
#include <cstdint>
#include <sys/types.h>
#include <cstring>

static void _Callback(void *context, const float *data, uint32_t len)
{
    SoapySDDC *sddc = (SoapySDDC *)context;
    sddc->Callback(data, len);
}

int SoapySDDC::Callback(const float *data, uint32_t len)
{
    TraceExtremePrintf("SoapySDDC::Callback(%p, %d)\n", data, len);
    if (_buf_count == numBuffers)
    {
        _overflowEvent = true;
        return 0;
    }

    auto &buff = _buffs[_buf_tail];
    buff.resize(len * bytesPerSample);
    memcpy(buff.data(), data, len * bytesPerSample);
    _buf_tail = (_buf_tail + 1) % numBuffers;

    {
        std::lock_guard<std::mutex> lock(_buf_mutex);
        _buf_count++;
    }
    _buf_cond.notify_one();

    return 0;
}

SoapySDDC::SoapySDDC(uint8_t dev_index): deviceId(-1),
                                        numBuffers(16)
{
    TracePrintf("SoapySDDC::SoapySDDC(%d)\n", dev_index);
    radio_handler = new RadioHandler();
    radio_handler->Init(dev_index);
    radio_handler->AttachIQ(_Callback, this);
}

SoapySDDC::~SoapySDDC(void)
{
    TracePrintf("SoapySDDC::~SoapySDDC()\n");
    radio_handler->Stop();

    delete radio_handler;
}

std::string SoapySDDC::getDriverKey(void) const
{
    TracePrintf("SoapySDDC::getDriverKey()\n");
    return "SDDC";
}

std::string SoapySDDC::getHardwareKey(void) const
{
    TracePrintf("SoapySDDC::getHardwareKey()\n");
    return std::string(radio_handler->getHardwareName());
}

SoapySDR::Kwargs SoapySDDC::getHardwareInfo(void) const
{
    TracePrintf("SoapySDDC::getHardwareInfo()\n");
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
    TracePrintf("SoapySDDC::getNumChannels(%i)\n", dir);
    return (dir == SOAPY_SDR_RX) ? 1 : 0;
}

SoapySDR::Kwargs SoapySDDC::getChannelInfo(const int direction, const size_t channel) const
{
    TracePrintf("SoapySDDC::getChannelInfo(%i, %ld)\n", direction, channel);
    // We could add infos about the channel here in the future
    return SoapySDR::Kwargs();
}

bool SoapySDDC::getFullDuplex(const int dir, const size_t chan) const
{
    TracePrintf("SoapySDDC::getFullDuplex(%i, %ld)\n", dir, chan);
    return false;
}

/*******************************************************************
 * Antenna API
 ******************************************************************/

std::vector<std::string> SoapySDDC::listAntennas(const int direction, const size_t) const
{
    TracePrintf("SoapySDDC::listAntennas(%d)\n", direction);
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
    TracePrintf("SoapySDDC::setAntenna(%d, %s)\n", direction, name.c_str());

    // No antennas available in TX, return early
    if (direction != SOAPY_SDR_RX)
    {
        return;
    }

    if(name == "HF")
    {
        radio_handler->SetRFMode(HFMODE);
        radio_handler->SetBiasT_HF(true);
        radio_handler->SetBiasT_VHF(false);
        return;
    }

    if(name == "VHF")
    {
        radio_handler->SetRFMode(VHFMODE);
        radio_handler->SetBiasT_VHF(true);
        radio_handler->SetBiasT_HF(false);
    }

    
    radio_handler->SetBiasT_HF(false);
    radio_handler->SetBiasT_VHF(false);
}

// get the selected antenna
std::string SoapySDDC::getAntenna(const int direction, const size_t) const
{
    TracePrintf("SoapySDDC::getAntenna(%d)\n", direction);

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
    TracePrintf("SoapySDDC::hasDCOffset()\n");
    return false;
}

bool SoapySDDC::hasDCOffsetMode(const int, const size_t) const
{
    TracePrintf("SoapySDDC::hasDCOffsetMode()\n");
    return false;
}

bool SoapySDDC::hasFrequencyCorrection(const int, const size_t) const
{
    TracePrintf("SoapySDDC::hasFrequencyCorrection()\n");
    return false;
}

bool SoapySDDC::hasIQBalance(const int, const size_t) const
{
    TracePrintf("SoapySDDC::hasIQBalance()\n");
    return false;
}

bool SoapySDDC::hasIQBalanceMode(const int, const size_t) const
{
    TracePrintf("SoapySDDC::hasIQBalanceMode()\n");
    return false;
}

std::vector<std::string> SoapySDDC::listGains(const int, const size_t) const
{
    TracePrintf("SoapySDDC::listGains()\n");
    std::vector<std::string> gains;
    gains.push_back("RF Attenuator");
    gains.push_back("IF Gain");
    return gains;
}

bool SoapySDDC::hasGainMode(const int direction, const size_t channel) const
{
    TracePrintf("SoapySDDC::hasGainMode(%i, %ld)\n", direction, channel);
    return false;
}

double SoapySDDC::getGain(const int direction, const size_t channel, const std::string &name) const
{
    TracePrintf("SoapySDDC::getGain(%i, %ld, %s)\n", direction, channel, name.c_str());

    if (name == "RF Attenuator") {
        return radio_handler->GetAttenuation();
    }
    else if (name == "IF Gain") {
        return radio_handler->GetGain();
    }
    return 0;
}
void SoapySDDC::setGain(const int direction, const size_t channel, const std::string &name, const double value)
{
    TracePrintf("SoapySDDC::setGain(%i, %ld, %s, %f)\n", direction, channel, name.c_str(), value);

    if (name == "RF Attenuator") {
        radio_handler->SetAttenuation(value);
    }
    else if (name == "IF Gain") {
        radio_handler->SetGain(value);
    }
}

SoapySDR::Range SoapySDDC::getGainRange(const int direction, const size_t channel, const std::string &name) const
{
    TracePrintf("SoapySDDC::getGainRange(%i, %ld, %s)\n", direction, channel, name.c_str());

    if (name == "RF Attenuator") {
        array<float, 2> gain_range = radio_handler->GetAttenuationRange();

        return SoapySDR::Range(
            gain_range.front(),
            gain_range.back()
        );
    }
    else if (name == "IF Gain") {
        array<float, 2> gain_range = radio_handler->GetGainRange();

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
    TracePrintf("SoapySDDC::listFrequencies(%i, %ld)\n", direction, channel);
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
    TracePrintf("SoapySDDC::getFrequencyRange(%i, %ld)\n", direction, channel);

    SoapySDR::RangeList ranges;

    ranges.push_back(SoapySDR::Range(10000, 1800000000));

    return ranges;
}

SoapySDR::RangeList SoapySDDC::getFrequencyRange(const int direction, const size_t channel, const std::string &name) const
{
    TracePrintf("SoapySDDC::getFrequencyRange(%i, %ld, %s)\n", direction, channel, name.c_str());

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

SoapySDR::ArgInfoList SoapySDDC::getFrequencyArgsInfo(const int, const size_t) const
{
    DbgPrintf("SoapySDDC::getFrequencyArgsInfo\n");
    return SoapySDR::ArgInfoList();
}

void SoapySDDC::setSampleRate(const int, const size_t, const double rate)
{
    TracePrintf("SoapySDDC::setSampleRate(%f)\n", rate);
    
    radio_handler->SetADCSampleRate(rate*2);
}

double SoapySDDC::getSampleRate(const int, const size_t) const
{
    TracePrintf("SoapySDDC::getSampleRate()\n");
    return radio_handler->GetADCSampleRate()/2;
}

SoapySDR::RangeList SoapySDDC::getSampleRateRange(const int direction, const size_t channel) const
{
    DbgPrintf("SoapySDDC::getSampleRateRange(%d, %ld)\n", direction, channel);

    SoapySDR::RangeList ranges;

    ranges.push_back(SoapySDR::Range(1000000/2, 160000000/2));

    return ranges;
}


SoapySDR::ArgInfoList SoapySDDC::getSettingInfo()
{
    TracePrintf("SoapySDDC::getSettingInfo()\n");
    SoapySDR::ArgInfoList list;
    SoapySDR::ArgInfo arg;

    arg.key = "rfmode";
    arg.value = "X";
    arg.name = "RF mode";
    arg.type = SoapySDR::ArgInfo::Type::STRING;
    list.push_back(arg);

    return list;
}

vector<string> SoapySDDC::listSensors()
{
    return vector<string>{
        "RF mode"
    };
}

string SoapySDDC::readSensor(const string key)
{
    if(key == "RF mode")
    {
        return radio_handler->GetRFMode() == VHFMODE ? "VHF" : "HF";
    }
    return "";
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