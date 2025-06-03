#include "fft_mt_r2iq.h"
#include "config.h"
#include "fftw3.h"
#include "RadioHandler.h"

#define TAG "fft_mt_r2iq_def"

void * fft_mt_r2iq::r2iqThreadf_def(r2iqThreadArg *th)
{
    TracePrintln(TAG, "%p", th);
    DebugPrintln(TAG, "Initialization...");

    const int deci_ratio = decimation_ratio[decimation];

    const int deci_fft_scrap_size = (BASE_FFT_SCRAP_SIZE / 2) / deci_ratio;
    const int fft_output_size = this->fft_size_per_decimation[decimation];
    const int fft_output_half_size = fft_output_size / 2;
    const int fft_useful_size = fft_output_size - deci_fft_scrap_size;

    DebugPrintln(TAG, "Decimation : %d (index %d)", deci_ratio, decimation);
    DebugPrintln(TAG, "Scrap size : %d", deci_fft_scrap_size);
    DebugPrintln(TAG, "FFT output size : %d", fft_output_size);
    DebugPrintln(TAG, "FFT useful output size : %d", fft_useful_size);
    DebugPrintln(TAG, "Initialization done");

    const fftwf_complex* filter = filterHw[decimation];
    const bool lsb = this->getSideband();
    const auto filter2 = &filter[BASE_FFT_HALF_SIZE - fft_output_half_size];

    plan_freq2time = &plan_freq2time_per_decimation[decimation];
    fftwf_complex* pout = nullptr;
    int decimate_count = 0;

    while(r2iqOn)
    {
        // Pointer to the current input block
        const int16_t *input_current_block;  
        // Pointer to the end of the previous input block minus halfFft
        // (input_previous_block + inputbuffer_block_size - halfFft)
        const int16_t *last_buffer_end;

        const int _center_frequency_bin = this->center_frequency_bin;  // Update LO tune is possible during run

        {
            std::unique_lock<std::mutex> lk(mutexR2iqControl);
            input_current_block = inputbuffer->getReadPtr();

            if (!r2iqOn)
                return 0;

            this->bufIdx = (this->bufIdx + 1) % QUEUE_SIZE;

            const int16_t *input_last_block = inputbuffer->peekReadPtr(-1);
            last_buffer_end = input_last_block + inputbuffer_block_size - BASE_FFT_SCRAP_SIZE;
        }

        // @todo: move the following int16_t conversion to (32-bit) float
        // directly inside the following loop (for "k < ffts_per_blocks")
        //   just before the forward fft "fftwf_execute_dft_r2c" is called
        // idea: this should improve cache/memory locality
#if PRINT_INPUT_RANGE
        std::pair<int16_t, int16_t> blockMinMax = std::make_pair<int16_t, int16_t>(0, 0);
#endif
        if (!this->getRand())        // plain samples no ADC rand set
        {
            convert_float<false>(
                /*dest=*/th->ADCinTime,
                /*source=*/last_buffer_end,
                /*len=*/BASE_FFT_SCRAP_SIZE
            );
#if PRINT_INPUT_RANGE
            auto minmax = std::minmax_element(input_current_block, input_current_block + inputbuffer_block_size);
            blockMinMax.first = *minmax.first;
            blockMinMax.second = *minmax.second;
#endif
            convert_float<false>(
                /*dest=*/th->ADCinTime + BASE_FFT_SCRAP_SIZE,
                /*source=*/input_current_block,
                /*len=*/inputbuffer_block_size
            );
        }
        else
        {
            convert_float<true>(
                /*dest=*/th->ADCinTime,
                /*source=*/last_buffer_end,
                /*len=*/BASE_FFT_SCRAP_SIZE
            );
            convert_float<true>(
                /*dest=*/th->ADCinTime + BASE_FFT_SCRAP_SIZE,
                /*source=*/input_current_block,
                /*len=*/inputbuffer_block_size
            );
        }

#if PRINT_INPUT_RANGE
        th->MinValue = std::min(blockMinMax.first, th->MinValue);
        th->MaxValue = std::max(blockMinMax.second, th->MaxValue);
        ++th->MinMaxBlockCount;
        if (th->MinMaxBlockCount * processor_count / 3 >= DEFAULT_TRANSFERS_PER_SEC )
        {
            float minBits = (th->MinValue < 0) ? (log10f((float)(-th->MinValue)) / log10f(2.0f)) : -1.0f;
            float maxBits = (th->MaxValue > 0) ? (log10f((float)(th->MaxValue)) / log10f(2.0f)) : -1.0f;
            printf("r2iq: min = %d (%.1f bits) %.2f%%, max = %d (%.1f bits) %.2f%%\n",
                (int)th->MinValue, minBits, th->MinValue *-100.0f / 32768.0f,
                (int)th->MaxValue, maxBits, th->MaxValue * 100.0f / 32768.0f);
            th->MinValue = 0;
            th->MaxValue = 0;
            th->MinMaxBlockCount = 0;
        }
#endif
        input_current_block = nullptr;
        inputbuffer->ReadDone();
        // decimate in frequency plus tuning

        if (decimate_count == 0)
            pout = (fftwf_complex*)outputbuffer->getWritePtr();

        decimate_count = (decimate_count + 1) & (deci_ratio - 1);

        // Calculate the parameters for the first half
        // Includes all frequencies above _center_frequency_bin
        const auto upper_frequencies_source = &th->ADCinFreq[_center_frequency_bin];
        const auto upper_frequencies_len = std::min(
            BASE_FFT_HALF_SIZE - _center_frequency_bin, // Desired value
            fft_output_half_size // Overflow protection
        );

        // Calculate the parameters for the second half
        // Includes all frequencies below _center_frequency_bin
        const auto lower_frequencies_source = &th->ADCinFreq[_center_frequency_bin - (fft_output_size / 2)];
        const auto lower_frequencies_start = std::max(
            fft_output_half_size - _center_frequency_bin,
            0
        );
        
        // Main processing loop based on overlap-save method
        // It also includes filtering and decimation
        for (int k = 0; k < ffts_per_blocks; k++)
        {
            // core of fast convolution including filter and decimation
            //   main part is 'overlap-scrap' (IMHO better name for 'overlap-save'), see
            //   https://en.wikipedia.org/wiki/Overlap%E2%80%93save_method
            {
                // FFT first stage: time to frequency, real to complex
                // Input buffer: th->ADCinTime + k * (0.75 * BASE_FFT_SIZE)
                // Transformation size: BASE_FFT_SIZE
                // Output buffer: th->ADCinFreq[]
                // Output size: BASE_FFT_HALF_SIZE + 1
                fftwf_execute_dft_r2c(plan_time2freq_r2c, th->ADCinTime + k * (BASE_FFT_SIZE - BASE_FFT_SCRAP_SIZE), th->ADCinFreq);

                // circular shift (mixing in full bins) and low/bandpass filtering (complex multiplication)
                {

                    // circular shift tune fs/2 first half array into th->inFreqTmp[]
                    shift_freq(
                        /*destination=*/th->inFreqTmp,
                        /*source1=*/upper_frequencies_source,
                        /*source2=*/filter,
                        /*start=*/0,
                        /*end=*/upper_frequencies_len
                    );

                    // Pad with zeroes if needed
                    if(fft_output_half_size != upper_frequencies_len)
                        memset(th->inFreqTmp[upper_frequencies_len], 0, (fft_output_half_size - upper_frequencies_len) * sizeof(fftwf_complex));

                    // circular shift tune fs/2 second half array
                    shift_freq(
                        /*destination=*/&th->inFreqTmp[fft_output_half_size],
                        /*source1=*/lower_frequencies_source,
                        /*source2=*/filter2,
                        /*start=*/lower_frequencies_start,
                        /*end=*/fft_output_half_size
                    );

                    if (lower_frequencies_start != 0)
                        memset(th->inFreqTmp[fft_output_half_size], 0, lower_frequencies_start * sizeof(fftwf_complex));
                }
                // result now in th->inFreqTmp[]
                // Size: fft_output_size (depending on the decimation)

                // 'shorter' inverse FFT transform (decimation) -> frequency (back) to COMPLEX time domain
                // transform size: fft_output_size (depending on the decimation)
                fftwf_execute_dft(*plan_freq2time, th->inFreqTmp, th->inFreqTmp);
                // result now in th->inFreqTmp[]
            }

            // postprocessing
            // @todo: is it possible to ..
            //  1)
            //    let inverse FFT produce/save it's result directly
            //    in "this->obuffers[modx] + offset" (pout)
            //    ( obuffers[] would need to have additional space ..;
            //      need to move 'scrap' of 'ovelap-scrap'? )
            //    at least FFTW would allow so,
            //      see http://www.fftw.org/fftw3_doc/New_002darray-Execute-Functions.html
            //    attention: multithreading!
            //  2)
            //    could mirroring (lower sideband) get calculated together
            //    with fine mixer - modifying the mixer frequency? (fs - fc)/fs
            //    (this would reduce one memory pass)
            if (lsb) // lower sideband
            {
                // mirror just by negating the imaginary Q of complex I/Q
                copy<true>(pout + k * fft_useful_size, &th->inFreqTmp[deci_fft_scrap_size], fft_useful_size);
            }
            else // upper sideband
            {
                copy<false>(pout + k * fft_useful_size, &th->inFreqTmp[deci_fft_scrap_size], fft_useful_size);
            }
            // result now in this->outputbuffer[]
        }

        if (decimate_count == 0) {
            outputbuffer->WriteDone();
            pout = nullptr;
        }
        else
        {
            pout += fft_useful_size * (ffts_per_blocks);
        }
    } // while(run)
//    DbgPrintf("r2iqThreadf idx %d pthread_exit %u\n",(int)th->t, pthread_self());
    return 0;
}