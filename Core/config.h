/*
 * This file is part of SDDC_Driver.
 *
 * Copyright (C) 2020 - Oscar Steila
 * Copyright (C) 2020 - Howard Su
 * Copyright (C) 2021 - Hayati Ayguen
 * Copyright (C) 2025 - RenardSpark
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "types.h"
#include "types_cpp.h"
#include "../Interface.h"
#include <math.h>      // atan => PI
#include <stdbool.h>

#define _DEBUG  // defined in VS configuration
//#define VERBOSE_TRACE
//#define VERBOSE_TRACEEXTREME

// macro to call callback function with just status extHWstatusT
#define EXTIO_STATUS_CHANGE( CB, STATUS )   \
	do { \
	  SendMessage(h_dialog, WM_USER + 1, STATUS, 0); \
	  if (CB) { \
		  DbgPrintf("<==CALLBACK: %s\n", #STATUS); \
		  CB( -1, STATUS, 0, NULL );\
	  }\
	}while(0)

#ifdef VERBOSE_DEBUG
	#define EnterFunction() \
	DbgPrintf("==>%s\n", __FUNCDNAME__)

	#define EnterFunction1(v1) \
	DbgPrintf("==>%s(%d)\n", __FUNCDNAME__, (v1))
#else
	#define EnterFunction()
	#define EnterFunction1(v1)
#endif

#ifdef _DEBUG
	#include <cstdio>
	#define DbgPrintf(fmt, ...) printf("[SDDC] " fmt "\n", ##__VA_ARGS__)
	#define DebugPrintf(fmt, ...) printf("[SDDC] DEBUG - %s - " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

	#define DebugPrint(tag, fmt, ...)   printf("[SDDC] DEBUG - %s: " fmt,      tag, ##__VA_ARGS__)
	#define DebugPrintln(tag, fmt, ...) printf("[SDDC] DEBUG - %s: " fmt "\n", tag, ##__VA_ARGS__)
#else
	#define DbgPrintf(fmt, ...) do {} while(0)
	#define DebugPrintf(fmt, ...) do {} while(0)
#endif

#ifdef VERBOSE_TRACE
	#define TracePrint(tag, fmt, ...)   printf("[SDDC] TRACE - %s: %d-%s(" fmt ")",   tag, __LINE__, __FUNCTION__, ##__VA_ARGS__)
	#define TracePrintln(tag, fmt, ...) printf("[SDDC] TRACE - %s: %d-%s(" fmt ")\n", tag, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
	#define TracePrint(tag, fmt, ...)
	#define TracePrintln(tag, fmt, ...)
#endif

#ifdef VERBOSE_TRACEEXTREME
	#define TraceExtremePrint(tag, fmt, ...)   TracePrint(tag, fmt, ##__VA_ARGS__)
	#define TraceExtremePrintln(tag, fmt, ...) TracePrintln(tag, fmt, ##__VA_ARGS__)
#else
	#define TraceExtremePrint(tag, fmt, ...)
	#define TraceExtremePrintln(tag, fmt, ...)
#endif

#define SWVERSION           "1.3.0 RC1"
#define SETTINGS_IDENTIFIER	"sddc_1.06"
#define SWNAME				"ExtIO_sddc.dll"

#define	QUEUE_SIZE 32
#define WIDEFFTN  // test FFTN 8192

#define FFTN_R_ADC (8192)       // FFTN used for ADC real stream DDC  tested at  2048, 8192, 32768, 131072

// GAINFACTORS to be adjusted with lab reference source measured with HDSDR Smeter rms mode  
#define BBRF103_GAINFACTOR 	(7.8e-8f)       // BBRF103
#define HF103_GAINFACTOR   	(1.14e-8f)      // HF103
#define RX888_GAINFACTOR   	(0.695e-8f)     // RX888
#define RX888mk2_GAINFACTOR (1.08e-8f)      // RX888mk2



#define HF_HIGH (32000000)    // 32M
#define MW_HIGH ( 2000000)

#define EXT_BLOCKLEN		512	* 64	/* 32768 only multiples of 512 */

// URL definitions
#define URL1B               "16bit SDR Receiver"
#define URL1                "<a>http://www.hdsdr.de/</a>"
#define URL_HDSR            "http://www.hdsdr.de/"
#define URL_HDSDRA          "<a>http://www.hdsdr.de/</a>"

#define MAXNDEV (4)  // max number of SDR device connected to PC
#define MAXDEVSTRLEN (64)  //max char len of SDR device description

extern bool saveADCsamplesflag;

// transferSize must be a multiple of 16 (maxBurst) * 1024 (SS packet size) = 16384
const uint32_t transferSize = 131072;
const uint32_t transferSamples = transferSize / sizeof(int16_t);
const uint32_t concurrentTransfers = 16;  // used to be 96, but I think it is too high

const uint32_t DEFAULT_ADC_FREQ = 64000000;	// ADC sampling frequency

const uint32_t DEFAULT_TRANSFERS_PER_SEC = DEFAULT_ADC_FREQ / transferSamples;



#define MIN_ADC_FREQ 50000000	   // ADC sampling frequency minimum
#define MAX_ADC_FREQ 140000000	// ADC sampling frequency minimum
#define N2_BANDSWITCH 80000000    // threshold 5 or 6 SR bandwidths


#endif // _CONFIG_H_
