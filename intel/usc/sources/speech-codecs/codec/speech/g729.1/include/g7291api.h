/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2006-2011 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives
//     USC - Unified Speech Codec interface library
//
// By downloading and installing USC codec, you hereby agree that the
// accompanying Materials are being provided to you under the terms and
// conditions of the End User License Agreement for the Intel(R) Integrated
// Performance Primitives product previously accepted by you. Please refer
// to the file ipplic.htm located in the root directory of your Intel(R) IPP
// product installation for more information.
//
// A speech coding standards promoted by ITU, ETSI, 3GPP and other
// organizations. Implementations of these standards, or the standard enabled
// platforms may require licenses from various entities, including
// Intel Corporation.
//
//
// Purpose: G.729/A/B/D/E/EV speech codec: API header file.
//
*/

#if !defined( __G7291API_H__ )
#define __G7291API_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _G729Codec_Type{
   G729_CODEC=0,
   G729A_CODEC=1,
   G729D_CODEC=2,
   G729E_CODEC=3,
   G729I_CODEC=4,
   G7291_CODEC=5
}G729Codec_Type;

typedef enum _G729Encode_Mode{
   G729Encode_VAD_Enabled=1,
   G729Encode_VAD_Disabled=0
}G729Encode_Mode;

typedef enum _G729Decode_Mode{
   G729Decode_DefaultMode=0
}G729Decode_Mode;

typedef enum{
    APIG729_StsBadCodecType     =   -5,
    APIG729_StsNotInitialized   =   -4,
    APIG729_StsBadArgErr        =   -3,
    APIG729_StsDeactivated      =   -2,
    APIG729_StsErr              =   -1,
    APIG729_StsNoErr            =    0
}APIG729_Status;

typedef enum{
    G729_Bad_Frame           = -1,
    G729_Untransmitted_Frame = 0,
    G729_SID_Frame           = 1,
    G729D_Voice_Frame        = 2,
    G729_Voice_Frame         = 3,
    G729E_Voice_Frame        = 4,
    G729_FirstSID_Frame      = 5
}G729_FrameType;

//struct _G7291Encoder_Obj;
//struct _G7291Decoder_Obj;
//typedef struct _G7291Encoder_Obj G7291Encoder_Obj;
//typedef struct _G7291Decoder_Obj G7291Decoder_Obj;


#if defined( _WIN32 ) || defined ( _WIN64 )
  #define __STDCALL  __stdcall
  #define __CDECL    __cdecl
#else
  #define __STDCALL
  #define __CDECL
#endif

#ifdef __cplusplus
}
#endif


#endif /* __G7291API_H__ */
