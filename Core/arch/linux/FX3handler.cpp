#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "FX3handler.h"
#include "usb_device.h"
#include "ezusb.h"
#include "firmware.h"

using namespace std;

#define firmware_data ((const char *)FIRMWARE)
#define firmware_size sizeof(FIRMWARE)

#define TAG "FX3Handler"

fx3class *CreateUsbHandler()
{
    TracePrintln(TAG, "");

    return new fx3handler();
}

fx3handler::fx3handler()
{
    TracePrintln(TAG, "");

    usb_device_infos = nullptr;
    dev = nullptr;
}

fx3handler::~fx3handler()
{
    TracePrintln(TAG, "");

    Close();
}

bool fx3handler::Open(uint8_t dev_index)
{
    TracePrintln(TAG, "%d", dev_index);
    dev = usb_device_open(dev_index, firmware_data, firmware_size);
    DebugPrintln(TAG, "Open device with dev_index=%d, dev=%p", dev_index, dev);

    usleep(5000);
    Control(STOPFX3, (uint8_t)0);

    return dev != nullptr;
}

bool fx3handler::Close(void)
{
    TracePrintln(TAG, "");

    if (dev) {
        usb_device_close(dev);
        dev = nullptr;
    }

    return true;
}

bool fx3handler::Control(FX3Command command, uint8_t data)
{
    TracePrintln(TAG, "%d, %d", command, data);

    return usb_device_control(this->dev, command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::Control(FX3Command command, uint32_t data)
{
    TracePrintln(TAG, "%d, %d", command, data);

    return usb_device_control(this->dev, command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::Control(FX3Command command, uint64_t data)
{
    TracePrintln(TAG, "%d, %ld", command, data);

    return usb_device_control(this->dev, command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::SetArgument(uint16_t index, uint16_t value)
{
    TracePrintln(TAG, "%d, %d", index, value);

    uint8_t data = 0;
    return usb_device_control(this->dev, SETARGFX3, value, index, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::GetHardwareInfo(uint32_t *data)
{
    TracePrintln(TAG, "%p", data);
#ifdef _DEBUG
    uint8_t enable_debug = 1;
#else
    uint8_t enable_debug = 0;
#endif
    return usb_device_control(this->dev, TESTFX3, enable_debug, 0, (uint8_t *)data, sizeof(*data), 1) == 0;
}

void fx3handler::StartStream(ringbuffer<int16_t> &samples_buf)
{
    TracePrintln(TAG, "");

    inputbuffer = &samples_buf;

    stream = streaming_open_async(this->dev, inputbuffer->getBlockSize() * sizeof(int16_t), concurrentTransfers, PacketRead, this);
    //samples_buf.setBlockSize(streaming_framesize(stream) / sizeof(int16_t));

    DebugPrintln(TAG, "Samples buffer blocksize: %d", samples_buf.getBlockSize());

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
    TracePrintln(TAG, "");

    run = false;
    poll_thread.join();

    streaming_stop(stream);
    streaming_close(stream);
}

void fx3handler::PacketRead(uint32_t data_size, uint8_t *data, void *context)
{
    TraceExtremePrintln(TAG, "%d, %p, %p", data_size, data, context);
    fx3handler *handler = (fx3handler *)context;

    auto *ptr = handler->inputbuffer->getWritePtr();
    assert(data_size == handler->inputbuffer->getBlockSize() * sizeof(int16_t));
    memcpy(ptr, data, data_size);
    handler->inputbuffer->WriteDone();
}

bool fx3handler::ReadDebugTrace(uint8_t *pdata, uint8_t len)
{
    TracePrintln(TAG, "%p, %d", pdata, len);

    return usb_device_control(this->dev, READINFODEBUG, pdata[0], 0, (uint8_t *)pdata, len, 1) == 0;
}

bool fx3handler::Enumerate(unsigned char &idx, char *lbuf)
{
    TracePrintln(TAG, "%d, %s", idx, lbuf);

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
    TracePrintln(TAG, "");

    if (usb_device_infos != nullptr) {
        usb_device_free_device_list(usb_device_infos);
    }

    usb_device_get_device_list(&usb_device_infos);

    return ERR_SUCCESS;
}

size_t fx3handler::GetDeviceListLength()
{
    TracePrintln(TAG, "");

    return usb_device_count_devices();
}

bool fx3handler::GetDevice(
    unsigned char &idx,
    char *name,
    size_t name_len,
    char *serial,
    size_t serial_len)
{
    TracePrintln(TAG, "%d, %p, %ld, %p, %ld", idx, name, name_len, serial, serial_len);

    if (idx >= usb_device_count_devices()) return false;

    if (usb_device_infos == nullptr) {
        SearchDevices();
    }

    auto dev = &usb_device_infos[idx];

    strncpy(name, (const char*)dev->product, name_len);
    strncpy(serial, (const char*)dev->serial_number, serial_len);

    return true;
}

vector<SDDC::DeviceItem> fx3handler::GetDeviceList()
{
    TracePrintln(TAG, "");

    vector<SDDC::DeviceItem> dev_list;

    struct usb_device_info *usb_device_list;
    usb_device_get_device_list(&usb_device_list);

    for(uint8_t i = 0; i < usb_device_count_devices(); i++)
    {
        SDDC::DeviceItem dev = {
            .index = i,
            .product = string(usb_device_list[i].product),
            .serial_number = string(usb_device_list[i].serial_number)
        };
        dev_list.push_back(dev);
    }

    usb_device_free_device_list(usb_device_list);

    return dev_list;
}