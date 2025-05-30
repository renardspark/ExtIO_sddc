#include "r2iq.h"
#include "FX3Class.h"
#include "CppUnitTestFramework.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <inttypes.h>  // For portable 64-bit type printf codes

#include "RadioHandler.h"

using namespace std::chrono;

class fx3handler : public fx3class
{
    bool Open(uint8_t)
    {
        return true;
    }

    bool Control(FX3Command command, uint8_t data = 0)
    {
        return true;
    }

    bool Control(FX3Command command, uint32_t data)
    {
        return true;
    }

    bool Control(FX3Command command, uint64_t data)
    {
        return true;
    }

    bool SetArgument(uint16_t index, uint16_t value)
    {
        return true;
    }

    bool GetHardwareInfo(uint32_t* data) {
        const uint8_t d[4] = {
            0, FIRMWARE_VER_MAJOR, FIRMWARE_VER_MINOR, 0
        };

        *data = *(uint32_t*)d;
        return true;
    }

    bool Enumerate(unsigned char& idx, char* lbuf)
    {
        return true;
    }

    bool ReadDebugTrace(uint8_t* pdata, uint8_t len) {
        return true;
    }

    std::thread emuthread;
    bool run;
	long nxfers;
    void StartStream(ringbuffer<int16_t>& input, int numofblock)
    {
        input.setBlockSize(transferSamples);
        run = true;
        emuthread = std::thread([&input, this]{
            while(run)
            {
                auto ptr = input.getWritePtr();
                memset(ptr, 0x5A, input.getWriteCount());
                input.WriteDone();
                ++nxfers;
                std::this_thread::sleep_for(1ms);
            }
        });
    }

	void StopStream() {
        run = false;
        emuthread.join();
    }

    size_t GetDeviceListLength()
    {
        return 0;
    }

    bool GetDevice(unsigned char &a, char *b, size_t c, char *d, size_t e)
    {
        return true;
    }

    
public:
	long Xfers(bool clear) { long rv=nxfers; if (clear) nxfers=0; return rv; }


};

static uint32_t frame_count;
static uint64_t totalsize;

static void Callback(void* context, const float* data, uint32_t len)
{
    frame_count++;
    totalsize += len;
}

namespace {
    struct CoreFixture {};
}

TEST_CASE(CoreFixture, OpenTest)
{
    auto radio = new RadioHandler();

    radio = new RadioHandler();
    delete radio;
}

TEST_CASE(CoreFixture, BasicTest)
{
    auto radio = new RadioHandler();

    radio->AttachIQ(Callback);

    REQUIRE_EQUAL(radio->getHardwareModel(), NORADIO);
    REQUIRE_EQUAL(radio->getHardwareName(), "Dummy");

    REQUIRE_EQUAL(radio->GetADCSampleRate(), 64000000u);
    radio->SetADCSampleRate(128000000);
    REQUIRE_EQUAL(radio->GetADCSampleRate(), 128000000u);

    REQUIRE_EQUAL(radio->GetDither(), false);
    radio->SetDither(true);
    REQUIRE_EQUAL(radio->GetDither(), true);

    REQUIRE_EQUAL(radio->GetRand(), false);
    radio->SetRand(true);
    REQUIRE_EQUAL(radio->GetRand(), true);

    REQUIRE_EQUAL(radio->GetPGA(), false);
    radio->SetPGA(true);
    REQUIRE_EQUAL(radio->GetPGA(), true);

    REQUIRE_EQUAL(radio->GetBiasT_HF(), false);
    radio->SetBiasT_HF(true);
    REQUIRE_EQUAL(radio->GetBiasT_HF(), true);

    REQUIRE_EQUAL(radio->GetBiasT_VHF(), false);
    radio->SetBiasT_VHF(true);
    REQUIRE_EQUAL(radio->GetBiasT_VHF(), true);

    delete radio;
}

TEST_CASE(CoreFixture, R2IQTest)
{
    auto radio = new RadioHandler();
    radio->Init(0);

    radio->AttachIQ(Callback);

    for (int decimate = 0; decimate < 5; decimate++)
    {
        frame_count = 0;
        totalsize = 0;
        radio->SetDecimation(decimate);
        radio->Start(true); // full bandwidth
        std::this_thread::sleep_for(1s);
        radio->Stop();

        REQUIRE_TRUE(frame_count > 0);
        REQUIRE_TRUE(totalsize > 0);
        REQUIRE_EQUAL(totalsize / frame_count, transferSamples);
    }

    delete radio;
}

TEST_CASE(CoreFixture, TuneTest)
{
    auto radio = new RadioHandler();
    radio->Init(0);

    radio->AttachIQ(Callback);

    radio->SetDecimation(1); // full bandwidth
    radio->Start(true);

    for (uint64_t i = 1000; i < 15000000;  i+=377000)
    {
        radio->SetCenterFrequency(i);
        std::this_thread::sleep_for(0.011s);
    }

    radio->Stop();


    delete radio;
}
