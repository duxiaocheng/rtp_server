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
// Purpose: GSMAMR speech codec: API header file.
//
*/

#ifndef __GSMAMRAPI_H__
#define __GSMAMRAPI_H__

#include "gsmamr.h"
#include "gsmamr_types.h"

#ifdef __cplusplus
extern "C" {
/* for C++ */
#endif

typedef enum _GSMAMRCodec_Type{
   GSMAMR_CODEC=0
}GSMAMRCodec_Type;

typedef enum _GSMAMREncode_Mode{
   GSMAMREncode_DefaultMode=0,
   GSMAMREncode_VAD1_Enabled,
   GSMAMREncode_VAD2_Enabled
}GSMAMREncode_Mode;

typedef enum _GSMAMRDecode_Mode{
   GSMAMRDecode_DefaultMode=0
}GSMAMRDecode_Mode;

typedef struct _GSMAMREnc_Params{
   GSMAMRCodec_Type codecType;
   Ipp32s mode;
}GSMAMREnc_Params;

typedef struct _GSMAMRDec_Params{
   GSMAMRCodec_Type codecType;
   Ipp32s mode;
}GSMAMRDec_Params;

typedef enum{
    APIGSMAMR_StsBadCodecType     =   -5,
    APIGSMAMR_StsNotInitialized   =   -4,
    APIGSMAMR_StsBadArgErr        =   -3,
    APIGSMAMR_StsDeactivated      =   -2,
    APIGSMAMR_StsErr              =   -1,
    APIGSMAMR_StsNoErr            =    0
}APIGSMAMR_Status;

struct _GSMAMREncoder_Obj;
struct _GSMAMRDecoder_Obj;
typedef struct _GSMAMREncoder_Obj GSMAMREncoder_Obj;
typedef struct _GSMAMRDecoder_Obj GSMAMRDecoder_Obj;

#if defined( _WIN32 ) || defined ( _WIN64 )
  #define __STDCALL  __stdcall
  #define __CDECL    __cdecl
#else
  #define __STDCALL
  #define __CDECL
#endif

#define   GSMAMR_CODECAPI(type,name,arg)                extern type name arg;

/*
                   Functions declarations
*/
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMREncoder_Alloc,
         (const GSMAMREnc_Params *gsm_Params, Ipp32u *pCodecSize))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMRDecoder_Alloc,
         (const GSMAMRDec_Params *gsm_Params, Ipp32u *pCodecSize))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMREncoder_Init,
         (GSMAMREncoder_Obj* encoderObj, Ipp32u mode, Ipp16s EFR_flag, Ipp16s taf_count))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMRDecoder_Init,
         (GSMAMRDecoder_Obj* decoderObj, Ipp32u mode, Ipp16s EFR_flag, Ipp16s taf_count))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMREncoder_Mode,
         (GSMAMREncoder_Obj* encoderObj, Ipp32u mode))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMRDecoder_Mode,
         (GSMAMRDecoder_Obj* decoderObj, Ipp32u mode))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMRDecoder_GetSize,
         (GSMAMRDecoder_Obj* decoderObj, Ipp32u *pCodecSize))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMREncoder_GetSize,
         (GSMAMREncoder_Obj* encoderObj, Ipp32u *pCodecSize))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMREncode,
         (GSMAMREncoder_Obj* encoderObj,const Ipp16s* src, GSMAMR_Rate_t rate,
          Ipp8u* dst, Ipp32s *pVad  ))
GSMAMR_CODECAPI( APIGSMAMR_Status, apiGSMAMRDecode,
         (GSMAMRDecoder_Obj* decoderObj,const Ipp8u* src, GSMAMR_Rate_t rate,
          RXFrameType rx_type, Ipp16s* dst))

#ifdef __cplusplus
}
#endif

#endif /* __GSMAMRAPI_H__ */
