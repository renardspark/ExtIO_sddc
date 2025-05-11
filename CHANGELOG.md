# Changelog

### tag v1.4 version date 7/5/2024
- Add SoapySDR module for Linux & Windoes
- Embedded firmware into Core to avoid carrying firmware file.

Install SoapySDR on Linux via the follow command:
> export SOAPY_SDR_PLUGIN_PATH=<path of libSDDCSupport.so>

### tag  v1.3.0RC1 Version "V1.2 RC1" date 4/11/2021
- Use Ringbuffer for input and output #157
- Delegate the VHF and HF decision to radio class #194
- Debug trace via USB #195
- Arm neon port #203
- A dialogBox with SDR name and FX3 serial number allows selection of a device from many. #210

 So far the known issues:
- Al power up sometime FX3 is not enumerated as Cypress USB FX3 BootLoader Device. When it happens also Cypress USB Control Center app is not able to detect it and a hardware disconnect and reconnect maybe required to enumerate the unit.

### tag  v1.2.0 Version "V1.2.0" date 18/3/2021
- Fix the crosstalk HF <-> VHF/UHF issue #177
- When HDSDR's version >= HDSDR v2.81 beta3 (March 8, 2021) following an ADC's sampling rate change the new IF bandwidths are computed dynamically and restart of HDSDR in not required.


### tag  v1.2RC1 Version "V1.2 RC1" date 18/2/2021
- The ADC's nominal sampling frequency is user selectable from 50 MHz to 140 MHz at 1Hz step, 
- The reference calibration can be adjusted in the dialog GUI in a range of +/- 200 ppm. (#171)
- The tuner IF center frequency is moved to 4.570 MHz that is the standard for RT820T. We were using 5 MHz before when we did not have yet fine tune ability. (#159)
- Support for AVX/2/512 instructions added. This change may reduce CPU usage for some modern hardware.(#152) 
- The Kaiser-Bessel IF filter with 85% of Nyquist band are computed at initialization. It simplifies managment of IF filters (#147)
- Add automatically build verification for both master branch and PRs. This  feature of the Github environment speeds development checks(#141)

 So far the known issues:
- The ADC sampling frequency can be selected via the ExtIO dialog.  HDSDR versions <= 2.80 require to close HDSDR.exe and restart the application to have the right IF sample rates. Higher HDSDR releases will have dynamically allocated IF sample rates and they will not require the restart.
- This release does not operate correctly with HF103 and similar designs that do not use a pll to generate the ADC sampling clock.
- The accuracy is that of the SI5351a programming about 1 Hz


### tag  v1.1.0 Version "V1.1.0" date 29/12/2020
- Fix the round of rf/if gains in the UI #109
- Fix sounds like clipping, on strong stations #120
- Fix reboot of FX3 #119

 So far the known issues:
- The reference frequency correction via HDSDR -> Options -> Calibration Settings ->LO Frequency Calibration doesn't work correctly. This problem will be addressed in the next release.

### tag  v1.1RC1 Version "V1.1 RC1" date 20/12/2020
- Supports 128M ADC sampling rate selectable via the dialog GUI ( Justin Peng and Howard Su )
- Use of libsddc allows development of Linux support and Soapy integration ( Franco Venturi https://github.com/fventuri/libsdd) 
- Tune the LO with a 1Hz step everywhere (Hayati Ayguen https://github.com/hayguen/pffft).
- Move multi thread r2iq to a multithreaded pipeline model to better leverage multi cores. Remove callback in USB (adcsample) thread to HDSDR to make sure we can reach 128Msps.
- Continuous tuning LW,MW,HF and VHF.
- Dialog GUI has samplerates and gains settings for use with other SDR applications than HDSDR.
- Test harmonic R820T tuning is there (Hayati Ayguen https://github.com/librtlsdr/librtlsdr/tree/development)
- the gain correction is made via  HDSDR -> Options -> Calibration Settings ->S-Meter Calibration.

 So far the known issues:
- The reference frequency correction via HDSDR -> Options -> Calibration Settings ->LO Frequency Calibration doesn't work correctly. This problem will be addressed in the next release.
- The 128M adc rate is experimental and must be activated manually in the ExtIO dialog GUI. It works with RX888 hardware that have 60 MHz LPF and requires a quite fast PC.

### tag  v1.01 Version "V1.01 RC1" date 06/11/2020
- SDDC_FX3 directory contains ARM sources and GPIFII project to compile sddc_fx3.img
- Detects the HW type: BBRF103, BBRF103, RX888 at runtime.
- Si5351a and R820T2 driver moved to FX3 code
- Redesign of FX3 control commands
- Rename of FX3handler (ex: OpenFX3) and RadioHandler (ex: BBRF103) modules
- Simplified ExtIO GUI Antenna BiasT, Dither, and Rand.
- Reference frequency correction via software +/- 200 ppm range
- Gain adjust +/-20 dB step 1dB
- R820T2 controls RF gains via a single control from GUI
- ExtIO.dll designed for HDSDR use.
- HF103 added a tuning limit at ADC_FREQ/2.

### tag  v0.98  Version "SDDC-0.98" date  13/06/2019
   R820T2 is enabled to support VHF

### tag  v0.96  Version "SDDC-0.96" date  25/02/2018

### tag  v0.95  Version "SDDC-0.95" date 31/08/2017

Initial version