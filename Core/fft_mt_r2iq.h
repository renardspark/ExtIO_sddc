#pragma once

#include "fftw3.h"
#include "config.h"
#include <algorithm>
#include <string.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "dsp/ringbuffer.h"

// use up to this many threads
#define N_MAX_R2IQ_THREADS 1
#define PRINT_INPUT_RANGE  0
#define NDECIDX 7  //number of srate

static const int BASE_FFT_SCRAP_SIZE = 1024;
static const int BASE_FFT_SIZE = FFTN_R_ADC;
static const int BASE_FFT_HALF_SIZE = BASE_FFT_SIZE / 2;

struct r2iqThreadArg;

class fft_mt_r2iq
{
public:
    fft_mt_r2iq();
    virtual ~fft_mt_r2iq();

    void Init(float gain, ringbuffer<int16_t>* buffers, ringbuffer<sddc_complex_t>* obuffers);

    void TurnOn();
    void TurnOff(void);
    bool IsOn(void);

    // --- Decimation --- //
    int getRatio()
    {
        return decimation_ratio[decimation];
    }
    bool setDecimate(uint8_t dec)
    {
        if(dec >= NDECIDX) return false;
        this->decimation = dec;
        return true;
    }
    // --- //

    void SetRand(bool v) { this->stateADCRand = v; }
    bool getRand() const { return this->stateADCRand; }

    void setSideband(bool lsb) { this->useSidebandLSB = lsb; }
    bool getSideband() const { return this->useSidebandLSB; }

    float setFreqOffset(float offset);

protected:

    template<bool rand> void convert_float(float* output, const int16_t *input, int size)
    {
        for(int m = 0; m < size; m++)
        {
            int16_t val;
            if (rand && (input[m] & 1))
            {
                val = input[m] ^ (-2);
            }
            else
            {
                val = input[m];
            }
            output[m] = float(val);
        }
    }

    void shift_freq(fftwf_complex* dest, const fftwf_complex* source1, const fftwf_complex* source2, int start, int end)
    {
        for (int m = start; m < end; m++)
        {
            // besides circular shift, do complex multiplication with the lowpass filter's spectrum
            // (a+ib)(c+id) = (ac - bd) + i(ad + bc)
            dest[m][0] = source1[m][0] * source2[m][0] - source1[m][1] * source2[m][1];
            dest[m][1] = source1[m][1] * source2[m][0] + source1[m][0] * source2[m][1];
        }
    }

    template<bool flip> void copy(fftwf_complex* dest, const fftwf_complex* source, int count)
    {
        if (flip)
        {
            for (int i = 0; i < count; i++)
            {
                dest[i][0] = source[i][0];
                dest[i][1] = -source[i][1];
            }
        }
        else
        {
            for (int i = 0; i < count; i++)
            {
                dest[i][0] = source[i][0];
                dest[i][1] = source[i][1];
            }
        }
    }

private:
    bool r2iqOn;        // r2iq on flag

    ringbuffer<int16_t>* inputbuffer;    // pointer to input buffers
    size_t inputbuffer_block_size = 0;

    ringbuffer<sddc_complex_t>* outputbuffer;    // pointer to output buffers

    // --- Decimation --- //
    int decimation = 0;   // selected decimation ratio
      // 64 Msps:               0 => 32Msps, 1=> 16Msps, 2 = 8Msps, 3 = 4Msps, 4 = 2Msps
      // 128 Msps: 0 => 64Msps, 1 => 32Msps, 2=> 16Msps, 3 = 8Msps, 4 = 4Msps, 5 = 2Msps
    int decimation_ratio[NDECIDX];  // ratio

    // Lookup table linking the decimation level to the size of the resulting FFT
    // Each step divides the fft size by 2
    // Definition : fft_size_per_decimation[x] = FFT_HALF_SIZE / 2^k
    int fft_size_per_decimation[NDECIDX];
    // --- //

    bool stateADCRand;       // randomized ADC output
    bool useSidebandLSB;

    // number of ffts needed to process one block of the input buffer
    int ffts_per_blocks = 0;

    int bufIdx;         // index to next buffer to be processed
    r2iqThreadArg* lastThread;

    float GainScale;

    // The bin (the portion of the FFT result) in which
    // the desired center frequency is located
    int center_frequency_bin = 0;

    void *r2iqThreadf(r2iqThreadArg *th);   // thread function

    void * r2iqThreadf_def(r2iqThreadArg *th);

    fftwf_complex **filterHw;       // Hw complex to each decimation ratio

	fftwf_plan  plan_time2freq_r2c;      // fftw plan buffers Freq to Time complex to complex per decimation ratio
	fftwf_plan *plan_freq2time;          // fftw plan buffers Time to Freq real to complex per buffer
	fftwf_plan  plan_freq2time_per_decimation[NDECIDX];

    uint32_t processor_count;
    r2iqThreadArg* threadArgs[N_MAX_R2IQ_THREADS];
    std::mutex mutexR2iqControl;                   // r2iq control lock
    std::thread r2iq_thread[N_MAX_R2IQ_THREADS]; // thread pointers
};

// assure, that ADC is not oversteered?
struct r2iqThreadArg {

	r2iqThreadArg()
	{
#if PRINT_INPUT_RANGE
		MinMaxBlockCount = 0;
		MinValue = 0;
		MaxValue = 0;
#endif
	}

	float *ADCinTime;                // point to each threads input buffers [nftt][n]
	fftwf_complex *ADCinFreq;         // buffers in frequency
	fftwf_complex *inFreqTmp;         // tmp decimation output buffers (after tune shift)
#if PRINT_INPUT_RANGE
	int MinMaxBlockCount;
	int16_t MinValue;
	int16_t MaxValue;
#endif
};
