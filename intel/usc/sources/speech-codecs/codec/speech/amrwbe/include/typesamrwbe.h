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
// Purpose: AMRWBE speech codec types definition.
//
*/

#ifndef __TYPESAMRWBE_H__
#define __TYPESAMRWBE_H__

#include "scratchmem.h"


#define FSCALE_DENOM    96     /* filter into decim_split.h */
#define FAC_FSCALE_MAX  (FSCALE_DENOM*41/24)  /* 164 */
#define FAC_FSCALE_MIN  (FSCALE_DENOM/2)       /* 48 */

#define L_FRAME_PLUS    1024    /* 80ms frame size (long TCX frame)           */

#define L_FRAME48k      ((L_FRAME_PLUS/4)*15)      /* 3840 */
#define L_FRAME44k      ((L_FRAME_PLUS/128)*441)   /* 3528 */
#define L_FRAME32k      ((L_FRAME_PLUS/2)*5)       /* 2560 */
#define L_FRAME24k      ((L_FRAME_PLUS/8)*15)      /* 1920 */
#define L_FRAME22k      ((L_FRAME_PLUS/256)*441)   /* 1764 */
#define L_FRAME16kPLUS  ((L_FRAME_PLUS/4)*5)       /* 1280 */
#define L_FRAME8k       ((L_FRAME_PLUS/8)*5)       /*  640 */

#define L_NEXT_WBP   256
#define L_NEXT24k   ((L_NEXT_WBP/8)*15)   /* 480 */
#define L_NEXT16k   ((L_NEXT_WBP/4)*5)    /* 320 */
#define L_NEXT8k    ((L_NEXT_WBP/8)*5)    /* 160 */

#define L_NEXT_ST       512//(L_A_MAX + L_BSP)    /* 512 */
#define L_NEXT_ST24k   ((L_NEXT_ST/8)*15)    /* 960 */
#define L_NEXT_ST16k   ((L_NEXT_ST/4)*5)     /* 640 */
#define L_NEXT_ST8k    ((L_NEXT_ST/8)*5)     /* 320 */


#define L_FRAME_MAX_FS    (2*L_FRAME48k)
#define L_FILT_OVER_FS    12
#define L_FILT_DECIM_FS   (L_FILT_OVER_FS*6)

/* maximum number of bits (to set buffer size of bitstream vector) */
#define  NBITS_MAX    (48*80)    /* define the buffer size at 32kbps */
/* number of bits for parametric bandwidth extension (BWE) */
#define  NBITS_BWE    (4*16)     /* 4 packets x 16 bits = 0.8 kbps */

extern __ALIGN(16) CONST Ipp16s tblNBitsAMRWBCore_16s[10];
extern __ALIGN(16) CONST Ipp16s tblNBitsMonoCore_16s[8];
extern __ALIGN(16) CONST Ipp16s tblNBitsStereoCore_16s[16];

extern __ALIGN(16) CONST Ipp32s tblPacketSizeMMS_WB[16];

#endif /* __TYPESAMRWBE_H__ */

