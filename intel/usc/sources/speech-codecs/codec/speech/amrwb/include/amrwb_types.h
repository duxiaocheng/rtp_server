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
// Purpose: AMRWB speech codec types definition.
//
*/

#ifndef __AMRWB_TYPES_H__
#define __AMRWB_TYPES_H__

/*************************************************************************
       AMRWB recieved and transmitted frame types
**************************************************************************/

typedef enum  {
   RX_SPEECH_GOOD=0,
   RX_SPEECH_PROBABLY_DEGRADED,
   RX_SPEECH_LOST,
   RX_SPEECH_BAD,
   RX_SID_FIRST,
   RX_SID_UPDATE,
   RX_SID_BAD,
   RX_NO_DATA
} RXFrameType;

typedef enum  {
   TX_SPEECH=0,
   TX_SID_FIRST,
   TX_SID_UPDATE,
   TX_NO_DATA,
   TX_SPEECH_PROBABLY_DEGRADED,
   TX_SPEECH_LOST,
   TX_SPEECH_BAD,
   TX_SID_BAD
} TXFrameType;

#define EHF_MASK (Ipp16s)0x0008            /* homing frame pattern                       */
#define L_FRAME16k   320                   /* Frame size at 16kHz                        */
#define IO_MASK (Ipp16u)0xfffC             /* 14-bit input/output                        */
#define AMRWB_Frame   L_FRAME16k           /* Frame size at 16kHz                        */

#define NUMBITS6600     132                  /* 6.60k  */
#define NUMBITS8850     177                  /* 8.85k  */
#define NUMBITS12650    253                  /* 12.65k */
#define NUMBITS14250    285                  /* 14.25k */
#define NUMBITS15850    317                  /* 15.85k */
#define NUMBITS18250    365                  /* 18.25k */
#define NUMBITS19850    397                  /* 19.85k */
#define NUMBITS23050    461                  /* 23.05k */
#define NUMBITS23850    477                  /* 23.85k */
#define NUMBITS_SID      35

/* number of parameters per modes (values must be <= MAX_PRM_SIZE!) */
enum _PRMNO_MR {
    PRMNO_MR660   = 18,
    PRMNO_MR8850  = 32,
    PRMNO_MR12650 = 36,
    PRMNO_MR14250 = 36,
    PRMNO_MR15850 = 36,
    PRMNO_MR18250 = 52,
    PRMNO_MR19850 = 52,
    PRMNO_MR23050 = 52,
    PRMNO_MR23850 = 56,
    PRMNO_MRDTX   =  7
};

extern Ipp16s NumberPrmsTbl[];

#define NUM_OF_MODES  10                   /* see bits.h for bits definition             */

/* AMR WB has nine possible bit rates... */

typedef enum {
    AMRWB_RATE_UNDEFINED = -1,
    AMRWB_RATE_6600 = 0,   /* 6.60 kbps   */
    AMRWB_RATE_8850,       /* 8.85 kbps   */
    AMRWB_RATE_12650,      /* 12.65  kbps */
    AMRWB_RATE_14250,      /* 14.25  kbps */
    AMRWB_RATE_15850,      /* 15.85  kbps */
    AMRWB_RATE_18250,      /* 18.25 kbps  */
    AMRWB_RATE_19850,      /* 19.85  kbps */
    AMRWB_RATE_23050,      /* 23.05  kbps */
    AMRWB_RATE_23850,      /* 23.85  kbps */
    AMRWB_RATE_DTX = 9,    /* DTX Discontinuous TX mode */
    AMRWB_RATE_LOST_FRAME = 14,
    AMRWB_RATE_NO_DATA = 15
} AMRWB_Rate_t;

#define NB_BITS_MAX   NUMBITS23850

#define SERIAL_FRAMESIZE (6+NB_BITS_MAX)   /* serial size max           */
#define BITSTREAM_SIZE  64                 /* max. num. of serial bytes */
#define TX_FRAME_TYPE (Ipp16s)0x6b21
#define RX_FRAME_TYPE (Ipp16s)0x6b20

/* for AMRWB+ compatibility */
#define WBP_OFFSET      (460)
#define HR_WINDOW_SIZE  512

#endif /* __AMRWB_TYPES_H__ */

