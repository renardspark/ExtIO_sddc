#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "FX3handler.h"
#include "usb_device.h"
#include "ezusb.h"
#include "firmware.h"

#define firmware_data ((const char *)FIRMWARE)
#define firmware_size sizeof(FIRMWARE)

fx3class *CreateUsbHandler()
{
    return new fx3handler();
}

fx3handler::fx3handler()
{
    usb_device_infos = nullptr;
    dev = nullptr;
}

fx3handler::~fx3handler()
{
    Close();
}

bool fx3handler::Open(uint8_t dev_index)
{
    TracePrintf("fx3handler::Open(%d)\n", dev_index);
    dev = usb_device_open(dev_index, firmware_data, firmware_size);
    DebugPrintf("fx3handler - Open device with dev_index=%d, dev=%p\n", devidx, dev);

    usleep(5000);
    Control(STOPFX3, (uint8_t)0);

    return dev != nullptr;
}

bool fx3handler::Close(void)
{
    TracePrintf("fx3handler::Close()\n");

    if (dev) {
        usb_device_close(dev);
        dev = nullptr;
    }

    return true;
}

bool fx3handler::Control(FX3Command command, uint8_t data)
{
    return usb_device_control(this->dev, command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::Control(FX3Command command, uint32_t data)
{
    return usb_device_control(this->dev, command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::Control(FX3Command command, uint64_t data)
{
    return usb_device_control(this->dev, command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::SetArgument(uint16_t index, uint16_t value)
{
    uint8_t data = 0;
    return usb_device_control(this->dev, SETARGFX3, value, index, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::GetHardwareInfo(uint32_t *data)
{
#ifdef _DEBUG
    uint8_t enable_debug = 1;
#else
    uint8_t enable_debug = 0;
#endif
    return usb_device_control(this->dev, TESTFX3, enable_debug, 0, (uint8_t *)data, sizeof(*data), 1) == 0;
}

void fx3handler::StartStream(ringbuffer<int16_t> &samples_buf)
{
    TracePrintf("fx3handler::StartStream()\n");

    inputbuffer = &samples_buf;

    stream = streaming_open_async(this->dev, transferSize, concurrentTransfers, PacketRead, this);
    samples_buf.setBlockSize(streaming_framesize(stream) / sizeof(int16_t));

    DebugPrintf("fx3handler - Samples buffer blocksize: %d\n", samples_buf.getBlockSize());

    // Start background thread to poll the events
    run = true;
    if (stream)
    {
        streaming_start(stream);
    }

    poll_thread = std::thread(
        [this]()
        {
            while (run)
            {
                usb_device_handle_events(this->dev);
            }
        }
    );
}

void fx3handler::StopStream()
{
    run = false;
    poll_thread.join();

    streaming_stop(stream);
    streaming_close(stream);
}

void fx3handler::PacketRead(uint32_t data_size, uint8_t *data, void *context)
{
    TraceExtremePrintf("fx3handler::PacketRead(%d, %p, %p)\n", data_size, data, context);
    fx3handler *handler = (fx3handler *)context;

    auto *ptr = handler->inputbuffer->getWritePtr();
    assert(data_size == handler->inputbuffer->getBlockSize() * sizeof(int16_t));
    memcpy(ptr, data, data_size);
    handler->inputbuffer->WriteDone();
}

bool fx3handler::ReadDebugTrace(uint8_t *pdata, uint8_t len)
{
    //TODO : This doesn't work
    return true;
    return usb_device_control(this->dev, READINFODEBUG, pdata[0], 0, (uint8_t *)pdata, len, 1) == 0;
}

bool fx3handler::Enumerate(unsigned char &idx, char *lbuf)
{
    if (idx >= usb_device_count_devices()) return false;

    if (usb_device_infos == nullptr) {
        usb_device_get_device_list(&usb_device_infos);
    }

    auto dev = &usb_device_infos[idx];

    strcpy (lbuf, (const char*)dev->product);
    while (strlen(lbuf) < 18) strcat(lbuf, " ");
    strcat(lbuf, "sn:");
    strcat(lbuf, (const char*)dev->serial_number);

    return true;
}



sddc_err_t fx3handler::SearchDevices()
{
    if (usb_device_infos != nullptr) {
        usb_device_free_device_list(usb_device_infos);
    }

    usb_device_get_device_list(&usb_device_infos);

    return ERR_SUCCESS;
}

size_t fx3handler::GetDeviceListLength()
{
    return usb_device_count_devices();
}

bool fx3handler::GetDevice(
    unsigned char &idx,
    char *name,
    size_t name_len,
    char *serial,
    size_t serial_len)
{
    if (idx >= usb_device_count_devices()) return false;

    if (usb_device_infos == nullptr) {
        SearchDevices();
    }

    auto dev = &usb_device_infos[idx];

    strncpy(name, (const char*)dev->product, name_len);
    strncpy(serial, (const char*)dev->serial_number, serial_len);

    return true;
}