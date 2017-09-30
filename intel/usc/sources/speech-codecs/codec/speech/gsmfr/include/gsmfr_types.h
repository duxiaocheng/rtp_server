/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2004-2011 Intel Corporation. All Rights Reserved.
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
// Purpose: GSM FR 06.10: types declaration.
//
*/

#ifndef __GSMFR_TYPES_H__
#define __GSMFR_TYPES_H__

#include <stdio.h>

typedef Ipp8u  uchar;
#define GSMFR_EXT_FRAME_LEN  160 /* length of input to encoder output of decoder frame=160 16-bit numbers*/
#define GSMFR_INT_FRAME_LEN  76  /* length of output of encoder input to decoder frame=76  16-bit numbers*/
#define GSMFR_PACKED_FRAME_LEN  33  /* length of bitstream to decoder. 33 8-bit numbers*/
#define GSMFR_RATE 0xD /* 13 kbit/s RPE-LTP */

typedef enum {
   ULAW =0,
   ALAW ,
   LINEAR
}GSMFR_FILE_t;

typedef enum{
   LTP_OFF=0,
   LTP_ON
}GSMFR_LTP_t;

struct GSMFR_Obj;
typedef struct GSMFR_Obj *GSMFR_Handle_t;

typedef enum {
  GSMFR_VAD_OFF=0,
  GSMFR_VAD_ON ,
  GSMFR_DOWNLINK,  /*VAD on , uplink mode*/
  GSMFR_UPLINK     /*VAD on , downlink mode*/
} GSMFR_VAD_t;

/* GSMFR status */
typedef enum{
  GSMFR_OK = 0,
  GSMFR_ERROR = -1
} GSMFR_Status_t;

typedef enum {
  GSMFR_ENCODE,
  GSMFR_DECODE
} GSMFR_DIRECTION_t;

/* The instance parameter structure */
typedef struct {
  GSMFR_VAD_t       vadMode;     /* VAD_ON or VAD_OFF */
  GSMFR_VAD_t       vadDirection;/* uplink or downlink */
  GSMFR_LTP_t       ltp_f;       /* cut ltp */
  GSMFR_DIRECTION_t direction;   /* GSMFR_ENCODE or GSMFR_DECODE */
} GSMFR_Params_t;

typedef struct _frame {
   Ipp16s LARc[8];   /* 6 6 5 5 4 4 3 3 bits*/

   Ipp16s Ncr0;      /* 7 bits */
   Ipp16s bcr0;      /* 2 bits */
   Ipp16s Mcr0;      /* 2 bits */
   Ipp16s xmaxc0;    /* 6 bits */
   Ipp16s xmcr0[13]; /* 3 bits per one */

   Ipp16s Ncr1;
   Ipp16s bcr1;
   Ipp16s Mcr1;
   Ipp16s xmaxc1;
   Ipp16s xmcr1[13];

   Ipp16s Ncr2;
   Ipp16s bcr2;
   Ipp16s Mcr2;
   Ipp16s xmaxc2;
   Ipp16s xmcr2[13];

   Ipp16s Ncr3;
   Ipp16s bcr3;
   Ipp16s Mcr3;
   Ipp16s xmaxc3;
   Ipp16s xmcr3[13];
} frame;
/*************************************************************************
       GSM FR recieved and transmitted frame types
**************************************************************************/

typedef enum  {
   RX_SID = 0,
   RX_SPEECH_GOOD = 1,
   RX_BAD = 2,
   RX_N_FRAMETYPES = 3     /* frame types number*/
}RXFrameType;

typedef enum  {
   TX_SID = 0,
   TX_SPEECH_GOOD = 1,
   TX_BAD = 2,
   TX_N_FRAMETYPES = 3     /* frame types number*/
}TXFrameType;

#endif /* __GSMFR_TYPES_H__ */
