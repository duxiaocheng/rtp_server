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
// Purpose: G.728 speech codec: API header file.
//
*/

#ifndef __G728API_H__
#define __G728API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <ippsc.h>
#include "g728.h"

typedef enum{
    APIG728_StsBadCodecType     =   -5,
    APIG728_StsNotInitialized   =   -4,
    APIG728_StsBadArgErr        =   -3,
    APIG728_StsDeactivated      =   -2,
    APIG728_StsErr              =   -1,
    APIG728_StsNoErr            =    0
}APIG728_Status;

struct _G728Encoder_Obj;
struct _G728Decoder_Obj;
typedef struct _G728Encoder_Obj G728Encoder_Obj;
typedef struct _G728Decoder_Obj G728Decoder_Obj;

#define   G728_CODECAPI( type,name,arg )        extern type name arg;

G728_CODECAPI( APIG728_Status, apiG728Encoder_Alloc,(Ipp32u* objSize))
G728_CODECAPI( APIG728_Status, apiG728Encoder_Init,(G728Encoder_Obj* encoderObj, G728_Rate rate))
G728_CODECAPI( APIG728_Status, apiG728Encoder_GetSize,
         (G728Encoder_Obj* encoderObj, Ipp32u *pCodecSize))

G728_CODECAPI( APIG728_Status, apiG728Encode,
         (G728Encoder_Obj* encoderObj, Ipp16s *src, Ipp8u *dst))

G728_CODECAPI( APIG728_Status, apiG728Decoder_Alloc,(Ipp32u* objSize))
G728_CODECAPI( APIG728_Status, apiG728Decoder_Init,
              (G728Decoder_Obj* decoderObj, G728_Type type, G728_Rate rate, Ipp32s pst))
G728_CODECAPI( APIG728_Status, apiG728Decoder_GetSize,
         (G728Decoder_Obj* decoderObj, Ipp32u *pCodecSize))

G728_CODECAPI( APIG728_Status, apiG728Decode, (G728Decoder_Obj* decoderObj,
              Ipp8u *src, Ipp16s *dst))

#ifdef __cplusplus
}
#endif

#endif /* __G728API_H__ */
