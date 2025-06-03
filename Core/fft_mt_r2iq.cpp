#include "license.txt"  
/*
The ADC input real stream of 16 bit samples (at Fs = 64 Msps in the example) is converted to:
- 32 Msps float Fs/2 complex stream, or
- 16 Msps float Fs/2 complex stream, or
-  8 Msps float Fs/2 complex stream, or
-  4 Msps float Fs/2 complex stream, or
-  2 Msps float Fs/2 complex stream.
The decimation factor is selectable from HDSDR GUI sampling rate selector

The name fft_mt_r2iq stands for Fast Fourier Transform, Multi-Threaded, Real to I+Q stream

*/

#include "fft_mt_r2iq.h"
#include "config.h"
#include "fftw3.h"
#include "RadioHandler.h"

#include "fir.h"

#include <assert.h>
#include <utility>

#define TAG "fft_mt_r2iq"


fft_mt_r2iq::fft_mt_r2iq() :
	filterHw(nullptr)
{
	r2iqOn = false;
	stateADCRand = false;
	useSidebandLSB = false;

	// --- Decimation --- //
	decimation = 0;
	decimation_ratio[0] = 1;  // 1,2,4,8,16
	for (int i = 1; i < NDECIDX; i++)
	{
		decimation_ratio[i] = decimation_ratio[i - 1] * 2;
	}

	fft_size_per_decimation[0] = halfFft;
	for (int i = 1; i < NDECIDX; i++)
	{
		fft_size_per_decimation[i] = fft_size_per_decimation[i - 1] / 2;
	}
	// --- //

	// Arbitrary value, defined to avoid overlapping with the end of the spectrum
	// by putting 0 or BASE_FFT_HALF_SIZE 
	center_frequency_bin = BASE_FFT_HALF_SIZE / 4;
	
	GainScale = 0.0f;

#ifndef NDEBUG
	int decimation_ratio = 1;  // 1,2,4,8,16,..
	const float Astop = 120.0f;
	const float relPass = 0.85f;  // 85% of Nyquist should be usable
	const float relStop = 1.1f;   // 'some' alias back into transition band is OK
	printf("\n***************************************************************************\n");
	printf("Filter tap estimation, Astop = %.1f dB, relPass = %.2f, relStop = %.2f\n", Astop, relPass, relStop);
	for (int d = 0; d < NDECIDX; d++)
	{
		float Bw = 64.0f / decimation_ratio;
		int ntaps = KaiserWindow(0, Astop, relPass * Bw / 128.0f, relStop * Bw / 128.0f, nullptr);
		printf("decimation %2d: KaiserWindow(Astop = %.1f dB, Fpass = %.3f,Fstop = %.3f, Bw %.3f @ %f ) => %d taps\n",
			d, Astop, relPass * Bw, relStop * Bw, Bw, 128.0f, ntaps);
		decimation_ratio = decimation_ratio * 2;
	}
	printf("***************************************************************************\n");
#endif

}

fft_mt_r2iq::~fft_mt_r2iq()
{
	if (filterHw == nullptr)
		return;

	fftwf_export_wisdom_to_filename("wisdom");

	for (int d = 0; d < NDECIDX; d++)
	{
		fftwf_free(filterHw[d]);     // 4096
	}
	fftwf_free(filterHw);

	fftwf_destroy_plan(plan_time2freq_r2c);
	for (int d = 0; d < NDECIDX; d++)
	{
		fftwf_destroy_plan(plan_freq2time_per_decimation[d]);
	}

	for (unsigned t = 0; t < processor_count; t++) {
		auto th = threadArgs[t];
		fftwf_free(th->ADCinTime);
		fftwf_free(th->ADCinFreq);
		fftwf_free(th->inFreqTmp);

		delete threadArgs[t];
	}
}

float fft_mt_r2iq::setFreqOffset(float offset)
{
	TracePrintln(TAG, "%f", offset);

	// Round to nearest multiple of 4 bins for better performance with SIMD operations
	this->center_frequency_bin = int(offset * BASE_FFT_HALF_SIZE / 4) * 4;

	float delta = ((float)this->center_frequency_bin  / BASE_FFT_HALF_SIZE) - offset;
	float ret = delta * getRatio(); // ret increases with higher decimation
	DebugPrintln(TAG, "Offset = %f/1, center_frequency_bin = %d/%d, delta = %f (%f)\n", offset, this->center_frequency_bin, BASE_FFT_HALF_SIZE, delta, ret);
	return ret;
}

void fft_mt_r2iq::TurnOn() {
	this->r2iqOn = true;
	this->bufIdx = 0;
	this->lastThread = threadArgs[0];

	inputbuffer->Start();
	outputbuffer->Start();

	for (unsigned t = 0; t < processor_count; t++) {
		r2iq_thread[t] = std::thread(
			[this] (void* arg)
				{ return this->r2iqThreadf((r2iqThreadArg*)arg); }, (void*)threadArgs[t]);
	}
}

void fft_mt_r2iq::TurnOff(void) {
	this->r2iqOn = false;

	inputbuffer->Stop();
	outputbuffer->Stop();
	for (unsigned t = 0; t < processor_count; t++) {
		r2iq_thread[t].join();
	}
}

bool fft_mt_r2iq::IsOn(void) { return(this->r2iqOn); }

void fft_mt_r2iq::Init(float gain, ringbuffer<int16_t> *input, ringbuffer<sddc_complex_t>* obuffers)
{
	TracePrintln(TAG, "%f, %p, %p", gain, input, obuffers);
	DebugPrintln(TAG, "Initialization...");

	DebugPrintln(TAG, "Full FFT size : %d", BASE_FFT_SIZE);
	DebugPrintln(TAG, "FFT size without scrap : %d", BASE_FFT_SIZE - BASE_FFT_SCRAP_SIZE);
	DebugPrintln(TAG, "FFT scrap size : %d", BASE_FFT_SCRAP_SIZE);

	this->inputbuffer = input;
	this->inputbuffer_block_size = input->getBlockSize();
	DebugPrintln(TAG, "Input block size: %ld", inputbuffer_block_size);

	this->outputbuffer = obuffers;
	DebugPrintln(TAG, "Output block size: %d", obuffers->getBlockSize());

	this->GainScale = gain;

	fft_scrap_proportion = 0.25;
	fft_save_proportion = 1-fft_scrap_proportion;


	// number of ffts needed to process one full buffer block
	// including an overlap with the previous samples (required by the overlap-save method)
	// Historically there was a "+ 1" here, but it triggers a rather catastrophic memory leak
	ffts_per_blocks = inputbuffer_block_size / (BASE_FFT_SIZE - BASE_FFT_SCRAP_SIZE);
	DebugPrintln(TAG, "Number of FFTs per blocks : %d", ffts_per_blocks);
	DebugPrintln(TAG, "Effective FFT conversion : %ld", ffts_per_blocks * (BASE_FFT_SIZE - BASE_FFT_SCRAP_SIZE));



	fftwf_import_wisdom_from_filename("wisdom");

	// Get the processor count
	processor_count = std::thread::hardware_concurrency();
	DebugPrintln(TAG, "Maximum available threads: %d", processor_count);

	if (processor_count > N_MAX_R2IQ_THREADS)
		processor_count = N_MAX_R2IQ_THREADS;

	DebugPrintln(TAG, "Usable threads: %d", processor_count);

	{
		fftwf_plan filterplan_t2f_c2c; // time to frequency fft

		


		DbgPrintf("RandTable generated");

		   // filters
		fftwf_complex *pfilterht;       // time filter ht
		pfilterht = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*halfFft);     // halfFft
		filterHw = (fftwf_complex**)fftwf_malloc(sizeof(fftwf_complex*)*NDECIDX);
		for (int d = 0; d < NDECIDX; d++)
		{
			filterHw[d] = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*halfFft);     // halfFft
		}

		filterplan_t2f_c2c = fftwf_plan_dft_1d(halfFft, pfilterht, filterHw[0], FFTW_FORWARD, FFTW_MEASURE);
		float *pht = new float[halfFft / 4 + 1];
		const float Astop = 120.0f;
		const float relPass = 0.85f;  // 85% of Nyquist should be usable
		const float relStop = 1.1f;   // 'some' alias back into transition band is OK
		for (int d = 0; d < NDECIDX; d++)	// @todo when increasing NDECIDX
		{
			// @todo: have dynamic bandpass filter size - depending on decimation
			//   to allow same stopband-attenuation for all decimations
			float Bw = 64.0f / decimation_ratio[d];
			// Bw *= 0.8f;  // easily visualize Kaiser filter's response
			KaiserWindow(halfFft / 4 + 1, Astop, relPass * Bw / 128.0f, relStop * Bw / 128.0f, pht);

			float gainadj = gain * 2048.0f / (float)FFTN_R_ADC; // reference is FFTN_R_ADC == 2048

			for (int t = 0; t < halfFft; t++)
			{
				pfilterht[t][0] = pfilterht[t][1]= 0.0F;
			}
		
			for (int t = 0; t < (halfFft/4+1); t++)
			{
				pfilterht[halfFft-1-t][0] = gainadj * pht[t];
			}

			fftwf_execute_dft(filterplan_t2f_c2c, pfilterht, filterHw[d]);
		}
		delete[] pht;
		fftwf_destroy_plan(filterplan_t2f_c2c);
		fftwf_free(pfilterht);

		for (unsigned t = 0; t < processor_count; t++) {
			r2iqThreadArg *th = new r2iqThreadArg();
			threadArgs[t] = th;

			// Buffer containing real samples of one block converted to float
			// plus a scrap portion from the previous block for the overlap-save
			th->ADCinTime = (float*)fftwf_malloc((inputbuffer_block_size + (BASE_FFT_SIZE * fft_scrap_proportion)) * sizeof(float));

			th->ADCinFreq = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*(BASE_FFT_HALF_SIZE + 1)); // 1024+1
			th->inFreqTmp = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*(halfFft));    // 1024
		}
		DebugPrintln(TAG, "Generated arguments sets for the threads");

		plan_time2freq_r2c = fftwf_plan_dft_r2c_1d(/*real_length=*/BASE_FFT_SIZE, /*in=*/threadArgs[0]->ADCinTime, /*out=*/threadArgs[0]->ADCinFreq, /*flags=*/FFTW_MEASURE);
		DebugPrintln(TAG, "Generated FFTW real to IQ plan");

		for (int d = 0; d < NDECIDX; d++)
		{
			// Generate inverse FFT plans for each decimation steps
			plan_freq2time_per_decimation[d] = fftwf_plan_dft_1d(fft_size_per_decimation[d], threadArgs[0]->inFreqTmp, threadArgs[0]->inFreqTmp, FFTW_BACKWARD, FFTW_MEASURE);
		}
		DebugPrintln(TAG, "Generated %d IFFT plans", NDECIDX);
	}

	DebugPrintln(TAG, "Initialization done");
}

#ifdef _WIN32
	//  Windows, assumed MSVC
	#include <intrin.h>
	#define cpuid(info, x)    __cpuidex(info, x, 0)
	#define DETECT_AVX
#elif defined(__x86_64__)
	//  GCC Intrinsics, x86 only
	#include <cpuid.h>
	#define cpuid(info, x)  __cpuid_count(x, 0, info[0], info[1], info[2], info[3])
	#define DETECT_AVX
#elif defined(__arm__) || defined(__aarch64__)
	#define DETECT_NEON
	#if defined(__linux__)
	#include <sys/auxv.h>
	#include <asm/hwcap.h>
	static bool detect_neon()
	{
		unsigned long caps = getauxval(AT_HWCAP);
		return (caps & HWCAP_NEON);
	}
    #elif defined(__APPLE__)
        #include <sys/sysctl.h>
        static bool detect_neon()
        {
            int hasNeon = 0;
            size_t len = sizeof(hasNeon);
            sysctlbyname("hw.optional.neon", &hasNeon, &len, NULL, 0);
            return hasNeon;
        }
    #endif
#else
#error Compiler does not identify an x86 or ARM core..
#endif

void * fft_mt_r2iq::r2iqThreadf(r2iqThreadArg *th)
{
#ifdef NO_SIMD_OPTIM
	DebugPrintln(TAG, "Hardware Capability: all SIMD features (AVX, AVX2, AVX512) deactivated\n");
	return r2iqThreadf_def(th);
#else
#if defined(DETECT_AVX)
	int info[4];
	bool HW_AVX = false;
	bool HW_AVX2 = false;
	bool HW_AVX512F = false;

	cpuid(info, 0);
	int nIds = info[0];

	if (nIds >= 0x00000001){
		cpuid(info,0x00000001);
		HW_AVX    = (info[2] & ((int)1 << 28)) != 0;
	}
	if (nIds >= 0x00000007){
		cpuid(info,0x00000007);
		HW_AVX2   = (info[1] & ((int)1 <<  5)) != 0;

		HW_AVX512F     = (info[1] & ((int)1 << 16)) != 0;
	}

	DebugPrintln(TAG, "Hardware Capability: AVX:%s AVX2:%s AVX512:%s\n", HW_AVX ? "yes" : "no", HW_AVX2 ? "yes" : "no", HW_AVX512F ? "yes" : "no");

	return r2iqThreadf_def(th);
#elif defined(DETECT_NEON)
	bool NEON = detect_neon();
	DebugPrintln(TAG, "Hardware Capability: NEON:%d\n", NEON);
	return r2iqThreadf_def(th);
#endif
#endif
}
