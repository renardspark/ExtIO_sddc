#include "SoapySDDC.hpp"
#include <SoapySDR/Registry.hpp>
#include <cstdint>
#include <string>

using namespace std;

#define TAG "SoapySDDC_Registration"

SoapySDR::KwargsList findSDDC(const SoapySDR::Kwargs &args)
{
    TracePrintln(TAG, "");

    vector<SoapySDR::Kwargs> results;

    vector<SDDC::DeviceItem> device_list = RadioHandler::GetDeviceList();
    for(auto sddc_device: device_list)
    {
        SoapySDR::Kwargs soapy_device;
        soapy_device["index"] = to_string(sddc_device.index);
        soapy_device["label"] = string(sddc_device.product);
        soapy_device["serial"] = string(sddc_device.serial_number);
        results.push_back(soapy_device);
    }

    return results;
}

SoapySDR::Device *makeSDDC(const SoapySDR::Kwargs &args)
{
    // I don't know how it works, but here I need to choose the right device
    TracePrintln(TAG, "");

    if(args.find("index") == args.end())
        return nullptr;

    return new SoapySDDC(stoul(args.at("index")));
}

static SoapySDR::Registry registerSDDC("SDDC", &findSDDC, &makeSDDC, SOAPY_SDR_ABI_VERSION);
