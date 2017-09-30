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
// Purpose: G.728 speech codec: common definition.
//
*/

#ifndef __G728_H__
#define __G728_H__

#define G728_VEC_PER_FRAME  4
#define G728_VECTOR  5
#define G728_FRAME  (G728_VEC_PER_FRAME * G728_VECTOR)

typedef enum _G728_Rate{
   G728_Rate_16000=0,
   G728_Rate_12800=1,
   G728_Rate_9600 =2,
   G728_Rate_40000 =3
}G728_Rate;

typedef enum _G728_Type{
   G728_Pure=0,
   G728_Annex_I=1
}G728_Type;

typedef enum {
  G728_PST_OFF = 0,
  G728_PST_ON
} G728_PST;

typedef enum {
  G728_PCM_MuLaw = 0,
  G728_PCM_ALaw,
  G728_PCM_Linear
} G728_PCM_Law;

#endif /* __G728_H__ */


