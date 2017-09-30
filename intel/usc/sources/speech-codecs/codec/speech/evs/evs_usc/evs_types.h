/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2011 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives
//     USC - Unified Speech Codec interface library
//
// By downloading and installing USC codec, you hereby agree that the
// accompanying Materials are being provided to you under the terms and
// conditions of the End User License Agreement for the Intel(R) Integrated
// Performance Primitives product previously accepted by you. Please refer
// to the file ippEULA.rtf or ippEULA.txt located in the root directory of your Intel(R) IPP
// product installation for more information.
//
// A speech coding standards promoted by ITU, ETSI, 3GPP and other
// organizations. Implementations of these standards, or the standard enabled
// platforms may require licenses from various entities, including
// Intel Corporation.
//
//
// Purpose: EVS speech codec types definition.
//
*/

#ifndef __EVS_TYPES_H__
#define __EVS_TYPES_H__

#define FIXED_SID_RATE          8			/* DTX SID rate */

#define  EVS_NUM_RATES  21
#define  EVS_SAMPLE_FREQUENCY_8  8000
#define  EVS_SAMPLE_FREQUENCY_16 16000
#define  EVS_SAMPLE_FREQUENCY_32 32000
#define  EVS_SAMPLE_FREQUENCY_48 48000
#define  EVS_BITS_PER_SAMPLE 16
#define  EVS_NCHANNELS 1

#define	 MAX_NUM_INDICES 1953
#define  MAX_AU_SIZE	 (128000/50/8)		/* Maximum frame frame size in bytes*/


/***************** FROM cnst_fx.h ******************/

#define RANDOM_INITSEED                       21845     /* Seed for random generators */
#define MAX_FRAME_COUNTER                     200
#define BITS_PER_SHORT                        16
#define BITS_PER_BYTE                         8
#define MAX_BITS_PER_FRAME                    2560
#define SYNC_GOOD_FRAME                       (UWord16) 0x6B21         /* synchronization word of a "good" frame */
#define SYNC_BAD_FRAME                        (UWord16) 0x6B20         /* synchronization word of a "bad" frame */
#define G192_BIN0                             (UWord16) 0x007F         /* binary "0" according to ITU-T G.192 */
#define G192_BIN1                             (UWord16) 0x0081         /* binary "1" according to ITU-T G.192 */

#define ENC                                   0         /* Index for "encoder" */
#define DEC                                   1         /* Index for "decoder" */

#define NB                                    0         /* Indicator of 4 kHz bandwidth */
#define WB                                    1         /* Indicator of 8 kHz bandwidth */
#define SWB                                   2         /* Indicator of 14 kHz bandwidth */
#define FB                                    3         /* Indicator of 20 kHz bandwidth */

/* Conversion of bandwidth string into numerical constant */
#define CONV_BWIDTH(bw)                       ( !strcmp(bw, "NB") ? NB : !strcmp(bw, "WB") ? WB : !strcmp(bw, "SWB") ? SWB : !strcmp(bw, "FB") ? FB : -1)

#define L_FRAME48k                            960       /* Frame size in samples at 48kHz */
#define L_FRAME32k                            640       /* Frame size in samples at 32kHz */
#define L_FRAME16k                            320       /* Frame size in samples at 16kHz */
#define L_FRAME8k                             160       /* Frame size in samples at 8kHz */

/* Conversion of ns to samples for a given sampling frequency */
#define NS2SA(fs,x)                           ((short)((((long)(fs)/100L) * ((x)/100L)) / 100000L))

/*----------------------------------------------------------------------------------*
* Delays
*----------------------------------------------------------------------------------*/

#define FRAME_SIZE_NS                         20000000L

#define ACELP_LOOK_NS                         8750000L
#define DELAY_FIR_RESAMPL_NS                  937500L
#define DELAY_CLDFB_NS                        1250000L

#define DELAY_SWB_TBE_12k8_NS                 1250000L
#define DELAY_SWB_TBE_16k_NS                  1125000L
#define MAX_DELAY_TBE_NS                      1312500L
#define DELAY_BWE_TOTAL_NS                    2312500L
#define DELAY_FD_BWE_ENC_12k8_NS             (DELAY_BWE_TOTAL_NS - (MAX_DELAY_TBE_NS - DELAY_SWB_TBE_12k8_NS))
#define DELAY_FD_BWE_ENC_16k_NS              (DELAY_BWE_TOTAL_NS - (MAX_DELAY_TBE_NS - DELAY_SWB_TBE_16k_NS))
#define DELAY_FD_BWE_ENC_NS                   2250000L

/*----------------------------------------------------------------------------------*
* EVS Bitrates
*----------------------------------------------------------------------------------*/

#define FRAME_NO_DATA                         0         /* Frame with no data */
#define SID_1k75                              1750      /* SID at 1.75 kbps               (used only in AMR-WB IO mode   */
#define SID_2k40                              2400      /* SID at 2.40 kbps                                              */
#define PPP_NELP_2k80                         2800      /* PPP and NELP at 2.80 kbps      (used only for SC-VBR)         */

#define ACELP_5k90                            5900      /* ACELP core layer at average bitrate of 5.90 kbps (used only in SC-VBR mode)      */
#define ACELP_6k60                            6600      /* ACELP core layer at 6.60  kbps (used only in AMR-WB IO mode)  */
#define ACELP_7k20                            7200      /* ACELP core layer at 7.20  kbps                                */
#define ACELP_8k00                            8000      /* ACELP core layer at 8     kbps                                */
#define ACELP_8k85                            8850      /* ACELP core layer at 8.85  kbps (used only in AMR-WB IO mode)  */
#define ACELP_9k60                            9600      /* ACELP core layer at 9.60  kbps                                */
#define ACELP_11k60                           11600     /* ACELP core layer at 11.60 kbps (used for SWB TBE)             */
#define ACELP_12k15                           12150     /* ACELP core layer at 12.15 kbps (used for WB TBE)              */
#define ACELP_12k65                           12650     /* ACELP core layer at 12.65 kbps (used only in AMR-WB IO mode)  */
#define ACELP_12k85                           12850     /* ACELP core layer at 12.85 kbps (used for WB BWE)              */
#define ACELP_13k20                           13200     /* ACELP core layer at 13.20 kbps                                */
#define ACELP_14k25                           14250     /* ACELP core layer at 14.25 kbps (used only in AMR-WB IO mode)  */
#define ACELP_14k80                           14800     /* ACELP core layer at 14.80 kbps (used for core switching only) */
#define ACELP_15k85                           15850     /* ACELP core layer at 15.85 kbps (used only in AMR-WB IO mode)  */
#define ACELP_16k40                           16400     /* ACELP core layer at 16.40 kbps                                */
#define ACELP_18k25                           18250     /* ACELP core layer at 18.25 kbps (used only in AMR-WB IO mode)  */
#define ACELP_19k85                           19850     /* ACELP core layer at 19.85 kbps (used only in AMR-WB IO mode)  */
#define ACELP_22k60                           22600     /* ACELP core layer at 22.60 kbps (used for core switching only) */
#define ACELP_23k05                           23050     /* ACELP core layer at 23.05 kbps (used only in AMR-WB IO mode)  */
#define ACELP_23k85                           23850     /* ACELP core layer at 23.85 kbps (used only in AMR-WB IO mode)  */
#define ACELP_24k40                           24400     /* ACELP core layer at 24.40 kbps                                */
#define ACELP_29k00                           29000     /* ACELP core layer at 29.00 kbps (used for FB + SWB TBE)        */
#define ACELP_29k20                           29200     /* ACELP core layer at 29.20 kbps (used for SWB TBE)             */
#define ACELP_30k20                           30200     /* ACELP core layer at 30.20 kbps (used for FB + SWB BWE)        */
#define ACELP_30k40                           30400     /* ACELP core layer at 30.40 kbps (used for SWB BWE)             */
#define ACELP_32k                             32000     /* ACELP core layer at 32    kbps                                */
#define ACELP_48k                             48000     /* ACELP core layer at 48    kbps                                */
#define ACELP_64k                             64000     /* ACELP core layer at 64    kbps                                */

#define HQ_16k40                              16400     /* HQ core at 16.4 kbps   */
#define HQ_13k20                              13200     /* HQ core at 13.2 kbps */
#define HQ_24k40                              24400     /* HQ core at 24.4 kbps */
#define HQ_32k                                32000     /* HQ core at 32 kbps */
#define HQ_48k                                48000     /* HQ core at 48 kbps */
#define HQ_64k                                64000     /* HQ core at 64 kbps */
#define HQ_96k                                96000     /* HQ core at 96 kbps */
#define HQ_128k                               128000    /* HQ core at 128 kbps */

/* EVS Rate mode */
enum
{
	PRIMARY_2800,
	PRIMARY_7200,
	PRIMARY_8000,
	PRIMARY_9600,
	PRIMARY_13200,
	PRIMARY_16400,
	PRIMARY_24400,
	PRIMARY_32000,
	PRIMARY_48000,
	PRIMARY_64000,
	PRIMARY_96000,
	PRIMARY_128000,
	PRIMARY_SID,
	PRIMARY_FUT1,
	SPEECH_LOST,
	NO_DATA_TYPE
};

/* AMRWB-io rate mode */
enum
{
	AMRWB_IO_6600,
	AMRWB_IO_8850,
	AMRWB_IO_1265,
	AMRWB_IO_1425,
	AMRWB_IO_1585,
	AMRWB_IO_1825,
	AMRWB_IO_1985,
	AMRWB_IO_2305,
	AMRWB_IO_2385,
	AMRWB_IO_SID
	/*, AMRWB_IO_FUT1,
	AMRWB_IO_FUT2,
	AMRWB_IO_FUT3,
	AMRWB_IO_FUT4,
	SPEECH_LOST,
	NO_DATA_TYPE */
};

/* Input file type */
enum
{
	G192,
	MIME,
	RTP
};


#endif /* __EVS_TYPES_H__ */

