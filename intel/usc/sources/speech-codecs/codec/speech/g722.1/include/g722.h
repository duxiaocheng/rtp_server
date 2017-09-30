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
// Purpose: G.722.1 speech codec: common definition.
//
*/

#ifndef __G722_H__
#define __G722_H__

/* common constants */
#include <ipps.h>

#define G722_SAMPLE_RATE_16kHz   16000
#define G722_SAMPLE_RATE_32kHz   32000
#define G722_SAMPLE_RATE_MAX     G722_SAMPLE_RATE_32kHz

#define G722_FRAMESIZE           (G722_SAMPLE_RATE_16kHz/50)   // 7kHz bandwidth
#define G722_FRAMESIZE_32kHz     (G722_SAMPLE_RATE_32kHz/50)   // 14kHz bandwidth
#define G722_FRAMESIZE_MAX       (G722_SAMPLE_RATE_MAX/50)

#define G722_MAX_BITS_PER_FRAME  960

#define G722_DCT_LENGTH_7kHz     320
#define G722_DCT_LENGTH_14kHz    640
#define G722_DCT_LENGTH_MAX      G722_DCT_LENGTH_14kHz

#endif /* __G722_H__ */


