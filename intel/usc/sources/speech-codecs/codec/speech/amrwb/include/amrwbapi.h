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
// Purpose: AMRWB speech codec API header.
//
*/

#ifndef __AMRWB_API_H__
#define __AMRWB_API_H__

#include <ipps.h>
#include <ippsc.h>
#include "amrwb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _AMRWBCodec_Type{
   AMRWB_CODEC=0
} AMRWBCodec_Type;

typedef enum _AMRWBEncode_Mode{
   AMRWBEncode_DefaultMode=0,
   AMRWBEncode_VAD_Enabled
} AMRWBEncode_Mode;

typedef enum _AMRWBDecode_Mode{
   AMRWBDecode_DefaultMode=0
} AMRWBDecode_Mode;

typedef struct _AMRWBEnc_Params{
   AMRWBCodec_Type codecType;
   Ipp32s mode;
} AMRWBEnc_Params;

typedef struct _AMRWBDec_Params{
   AMRWBCodec_Type codecType;
   Ipp32s mode;
} AMRWBDec_Params;

typedef enum{
    APIAMRWB_StsBadCodecType     =   -5,
    APIAMRWB_StsNotInitialized   =   -4,
    APIAMRWB_StsBadArgErr        =   -3,
    APIAMRWB_StsDeactivated      =   -2,
    APIAMRWB_StsErr              =   -1,
    APIAMRWB_StsNoErr            =    0
} APIAMRWB_Status;

struct _AMRWBEncoder_Obj;
struct _AMRWBDecoder_Obj;
typedef struct _AMRWBEncoder_Obj AMRWBEncoder_Obj;
typedef struct _AMRWBDecoder_Obj AMRWBDecoder_Obj;

#if defined( _WIN32 ) || defined ( _WIN64 )
  #define __STDCALL  __stdcall
  #define __CDECL    __cdecl
#else
  #define __STDCALL
  #define __CDECL
#endif

#define   AMRWB_CODECAPI(type,name,arg)                extern type name arg;

/*
                   Functions declarations
*/
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBEncoder_Alloc,
         (const AMRWBEnc_Params *amrwb_Params, Ipp32u *pCodecSize))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBDecoder_Alloc,
         (const AMRWBDec_Params *amrwb_Params, Ipp32u *pCodecSize))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBEncoder_Init,
         (AMRWBEncoder_Obj* encoderObj, Ipp32s mode))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBEncoderProceedFirst,
            (AMRWBEncoder_Obj* pEncoderObj, const Ipp16s* pSrc))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBDecoder_Init,
         (AMRWBDecoder_Obj* decoderObj))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBDecoder_GetSize,
         (AMRWBDecoder_Obj* decoderObj, Ipp32u *pCodecSize))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBEncoder_GetSize,
         (AMRWBEncoder_Obj* encoderObj, Ipp32u *pCodecSize))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBEncode,
         (AMRWBEncoder_Obj* encoderObj, const Ipp16u* src, Ipp8u* dst,
         AMRWB_Rate_t *rate, Ipp32s Vad, Ipp32s offsetWBE ))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBDecode,
         (AMRWBDecoder_Obj* decoderObj,const Ipp8u* src, AMRWB_Rate_t rate,
          RXFrameType rx_type, Ipp16u* dst, Ipp32s offsetWBE))
AMRWB_CODECAPI( APIAMRWB_Status, apiAMRWBEncoder_Mode,
               (AMRWBEncoder_Obj* encoderObj, Ipp32s mode))

#ifdef __cplusplus
}
#endif

#endif /* __AMRWB_API_H__ */
