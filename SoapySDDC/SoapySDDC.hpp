#pragma once
#include "../Core/config.h"
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Types.h>
#include <atomic>
#include <cstddef>
#include <sys/types.h>
#include "FX3Class.h"
#include "RadioHandler.h"

class SoapySDDC : public SoapySDR::Device
{
public:
    explicit SoapySDDC(uint8_t dev_index);
    ~SoapySDDC(void);

    // ----- Metadata ----- //
    std::string getDriverKey(void) const;
    std::string getHardwareKey(void) const;
    SoapySDR::Kwargs getHardwareInfo(void) const;

    size_t getNumChannels(const int) const;
    SoapySDR::Kwargs getChannelInfo(const int direction, const size_t channel) const;
    bool getFullDuplex(const int direction, const size_t channel) const;
    // ----- //

    // ----- Stream ----- //
    // --- Metadata --- //
    std::vector<std::string> getStreamFormats(const int direction, const size_t channel) const;
    std::string getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const;
    SoapySDR::ArgInfoList getStreamArgsInfo(const int direction, const size_t channel) const;

    // --- 
    SoapySDR::Stream *setupStream(const int direction, const std::string &format, const std::vector<size_t> &channels = std::vector<size_t>(), const SoapySDR::Kwargs &args = SoapySDR::Kwargs());

    void closeStream(SoapySDR::Stream *stream);

    size_t getStreamMTU(SoapySDR::Stream *stream) const;

    int activateStream(SoapySDR::Stream *stream, const int flags = 0, const long long timeNs = 0, const size_t numElems = 0);

    int deactivateStream(SoapySDR::Stream *stream, const int flags = 0, const long long timeNs = 0);

    int readStream(SoapySDR::Stream *stream, void *const *buffs, const size_t numElems, int &flags, long long &timeNs, const long timeoutUs = 100000);

    // size_t getNumDirectAccessBuffers(SoapySDR::Stream *stream);

    // int getDirectAccessBufferAddrs(SoapySDR::Stream *stream, const size_t handle, void **buffs);

    int acquireReadBuffer(SoapySDR::Stream *stream, size_t &handle, const void **buffs, int &flags, long long &timeNs, const long timeoutUs = 100000);

    void releaseReadBuffer(SoapySDR::Stream *stream, const size_t handle);
    // ----- //

    // ----- Antennas ----- //
    std::vector<std::string> listAntennas(const int direction, const size_t channel) const;
    void                     setAntenna(const int direction, const size_t channel, const std::string &name);
    std::string              getAntenna(const int direction, const size_t channel) const;
    // ----- //

    // ----- Additional features ----- //
    bool hasDCOffset(const int direction, const size_t channel) const;
    bool hasDCOffsetMode(const int direction, const size_t channel) const;

    bool hasIQBalance(const int, const size_t) const;
    bool hasIQBalanceMode(const int, const size_t) const;

    bool hasFrequencyCorrection(const int direction, const size_t channel) const;
    // ----- //

    // ----- Gains --- //
    std::vector<std::string> listGains(const int direction, const size_t channel) const;
    bool                     hasGainMode(const int direction, const size_t channel) const;
    void                     setGain(const int direction, const size_t channel, const std::string &name, const double value);
    double                   getGain(const int direction, const size_t channel, const std::string &name) const;
    SoapySDR::Range          getGainRange(const int direction, const size_t channel, const std::string &name) const;
    // ----- //

    // ----- Frequency ----- //
    // --- Metadata --- //
    std::vector<std::string> listFrequencies(const int direction, const size_t channel) const;
    SoapySDR::RangeList      getFrequencyRange(const int direction, const size_t channel) const;
    SoapySDR::RangeList      getFrequencyRange(const int direction, const size_t channel, const std::string &name) const;
    SoapySDR::ArgInfoList    getFrequencyArgsInfo(const int direction, const size_t channel) const;
    // --- //

    void   setFrequency(const int direction, const size_t channel, const double frequency, const SoapySDR::Kwargs &args = SoapySDR::Kwargs());
    void   setFrequency(const int direction, const size_t channel, const std::string &name, const double frequency, const SoapySDR::Kwargs &args = SoapySDR::Kwargs());
    double getFrequency(const int direction, const size_t channel) const;
    double getFrequency(const int direction, const size_t channel, const std::string &name) const;
    // ----- //

    // ----- Sample rate ----- //
    SoapySDR::RangeList getSampleRateRange(const int direction, const size_t channel) const;
    void                setSampleRate(const int direction, const size_t channel, const double rate);
    double              getSampleRate(const int direction, const size_t channel) const;
    // ----- //

    // ----- Settings ----- //
    SoapySDR::ArgInfoList getSettingInfo();

    // ----- Sensors ----- //
    vector<string> listSensors();
    string readSensor(const string key);
    // ----- //

    

    SoapySDR::ArgInfoList getSettingInfo(void) const;

    void writeSetting(const std::string &key, const std::string &value);

    // void setMasterClockRate(const double rate);

    // double getMasterClockRate(void) const;

    // std::vector<std::string> listTimeSources(void) const;

    // std::string getTimeSource(void) const;

    // bool hasHardwareTime(const std::string &what = "") const;

    // long long getHardwareTime(const std::string &what = "") const;

    // void setHardwareTime(const long long timeNs, const std::string &what = "");

private:
    int deviceId;
    int bytesPerSample;

    uint64_t centerFrequency;
    size_t numBuffers, bufferLength, asyncBuffs;
    std::atomic<long long> ticks;

    fx3class *Fx3;
    RadioHandler *radio_handler;

public:
    int Callback(const float *data, uint32_t len);

    std::mutex _buf_mutex;
    std::condition_variable _buf_cond;

    std::vector<std::vector<char>> _buffs;
    size_t _buf_head;
    size_t _buf_tail;
    std::atomic<size_t> _buf_count;
    char *_currentBuff;
    std::atomic<bool> _overflowEvent;
    size_t bufferedElems;
    size_t _currentHandle;
    bool resetBuffer;

    int samplerateidx;

    double masterClockRate;
};





 
 /*
 
 
virtual int     writeStream (Stream *stream, const void *const *buffs, const size_t numElems, int &flags, const long long timeNs=0, const long timeoutUs=100000)
 
virtual int     readStreamStatus (Stream *stream, size_t &chanMask, int &flags, long long &timeNs, const long timeoutUs=100000)
 
virtual size_t  getNumDirectAccessBuffers (Stream *stream)
 
virtual int     getDirectAccessBufferAddrs (Stream *stream, const size_t handle, void **buffs)
 
virtual int     acquireReadBuffer (Stream *stream, size_t &handle, const void **buffs, int &flags, long long &timeNs, const long timeoutUs=100000)
 
virtual void    releaseReadBuffer (Stream *stream, const size_t handle)
 
virtual int     acquireWriteBuffer (Stream *stream, size_t &handle, void **buffs, const long timeoutUs=100000)
 
virtual void    releaseWriteBuffer (Stream *stream, const size_t handle, const size_t numElems, int &flags, const long long timeNs=0)
 
 
 
 



virtual void    setBandwidth (const int direction, const size_t channel, const double bw)
 
virtual double  getBandwidth (const int direction, const size_t channel) const
 
virtual std::vector< double >   listBandwidths (const int direction, const size_t channel) const
 
virtual RangeList   getBandwidthRange (const int direction, const size_t channel) const
 
virtual void    setMasterClockRate (const double rate)
 
virtual double  getMasterClockRate (void) const
 
virtual RangeList   getMasterClockRates (void) const
 
virtual void    setReferenceClockRate (const double rate)
 
virtual double  getReferenceClockRate (void) const
 
virtual RangeList   getReferenceClockRates (void) const
 
virtual std::vector< std::string >  listClockSources (void) const
 
virtual void    setClockSource (const std::string &source)
 
virtual std::string     getClockSource (void) const
 
virtual std::vector< std::string >  listTimeSources (void) const
 
virtual void    setTimeSource (const std::string &source)
 
virtual std::string     getTimeSource (void) const
 
virtual bool    hasHardwareTime (const std::string &what="") const
 
virtual long long   getHardwareTime (const std::string &what="") const
 
virtual void    setHardwareTime (const long long timeNs, const std::string &what="")
 
virtual void    setCommandTime (const long long timeNs, const std::string &what="")
 
virtual std::vector< std::string >  listSensors (void) const
 
virtual ArgInfo     getSensorInfo (const std::string &key) const
 
virtual std::string     readSensor (const std::string &key) const
 
template<typename Type >
Type    readSensor (const std::string &key) const
 
virtual std::vector< std::string >  listSensors (const int direction, const size_t channel) const
 
virtual ArgInfo     getSensorInfo (const int direction, const size_t channel, const std::string &key) const
 
virtual std::string     readSensor (const int direction, const size_t channel, const std::string &key) const
 
template<typename Type >
Type    readSensor (const int direction, const size_t channel, const std::string &key) const
 
virtual std::vector< std::string >  listRegisterInterfaces (void) const
 
virtual void    writeRegister (const std::string &name, const unsigned addr, const unsigned value)
 
virtual unsigned    readRegister (const std::string &name, const unsigned addr) const
 
virtual void    writeRegister (const unsigned addr, const unsigned value)
 
virtual unsigned    readRegister (const unsigned addr) const
 
virtual void    writeRegisters (const std::string &name, const unsigned addr, const std::vector< unsigned > &value)
 
virtual std::vector< unsigned >     readRegisters (const std::string &name, const unsigned addr, const size_t length) const
 
virtual ArgInfoList     getSettingInfo (void) const
 
virtual void    writeSetting (const std::string &key, const std::string &value)
 
template<typename Type >
void    writeSetting (const std::string &key, const Type &value)
 
virtual std::string     readSetting (const std::string &key) const
 
template<typename Type >
Type    readSetting (const std::string &key)
 
virtual ArgInfoList     getSettingInfo (const int direction, const size_t channel) const
 
virtual void    writeSetting (const int direction, const size_t channel, const std::string &key, const std::string &value)
 
template<typename Type >
void    writeSetting (const int direction, const size_t channel, const std::string &key, const Type &value)
 
virtual std::string     readSetting (const int direction, const size_t channel, const std::string &key) const
 
template<typename Type >
Type    readSetting (const int direction, const size_t channel, const std::string &key)
 
virtual std::vector< std::string >  listGPIOBanks (void) const
 
virtual void    writeGPIO (const std::string &bank, const unsigned value)
 
virtual void    writeGPIO (const std::string &bank, const unsigned value, const unsigned mask)
 
virtual unsigned    readGPIO (const std::string &bank) const
 
virtual void    writeGPIODir (const std::string &bank, const unsigned dir)
 
virtual void    writeGPIODir (const std::string &bank, const unsigned dir, const unsigned mask)
 
virtual unsigned    readGPIODir (const std::string &bank) const
 
virtual void    writeI2C (const int addr, const std::string &data)
 
virtual std::string     readI2C (const int addr, const size_t numBytes)
 
virtual unsigned    transactSPI (const int addr, const unsigned data, const size_t numBits)
 
virtual std::vector< std::string >  listUARTs (void) const
 
virtual void    writeUART (const std::string &which, const std::string &data)
 
virtual std::string     readUART (const std::string &which, const long timeoutUs=100000) const
 
virtual void *  getNativeDeviceHandle (void) const
*/