#include "SoapySDDC.hpp"
#include <SoapySDR/Registry.hpp>
#include <cstdint>
#include <string>

SoapySDR::KwargsList findSDDC(const SoapySDR::Kwargs &args)
{
    DbgPrintf("soapySDDC::findSDDC\n");

    std::vector<SoapySDR::Kwargs> results;

    uint8_t count = RadioHandler::GetDeviceListLength();

    /* get device info */
    struct sddc_device_t sddc_device_infos;
    for(int i = 0; i < count; ++i)
    {
        RadioHandler::GetDevice(i, &sddc_device_infos);

        SoapySDR::Kwargs devInfo;
        devInfo["index"] = std::to_string(i);
        devInfo["label"] = std::string(sddc_device_infos.product);
        devInfo["serial"] = std::string(sddc_device_infos.serial_number);
        results.push_back(devInfo);
    }

    return results;
}

SoapySDR::Device *makeSDDC(const SoapySDR::Kwargs &args)
{
    // I don't know how it works, but here I need to choose the right device
    DbgPrintf("soapySDDC::makeSDDC\n");
    return new SoapySDDC(0);
}

static SoapySDR::Registry registerSDDC("SDDC", &findSDDC, &makeSDDC, SOAPY_SDR_ABI_VERSION);
