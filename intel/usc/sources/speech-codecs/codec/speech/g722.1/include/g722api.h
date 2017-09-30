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
// Purpose: G.722.1 speech codec: API header file.
//
*/

#ifndef __G722API_H__
#define __G722API_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    APIG722_StsBadCodecType     =   -5,
    APIG722_StsNotInitialized   =   -4,
    APIG722_StsBadArgErr        =   -3,
    APIG722_StsDeactivated      =   -2,
    APIG722_StsErr              =   -1,
    APIG722_StsNoErr            =    0
}APIG722_Status;

struct _G722Encoder_Obj;
struct _G722Decoder_Obj;
typedef struct _G722Encoder_Obj G722Encoder_Obj;
typedef struct _G722Decoder_Obj G722Decoder_Obj;

#define   G722_CODECAPI( type,name,arg )        extern type name arg;

G722_CODECAPI( APIG722_Status, apiG722Encoder_Alloc, (Ipp32u* objSize))
G722_CODECAPI( APIG722_Status, apiG722Encoder_Init, (G722Encoder_Obj* pEncoderObj))
G722_CODECAPI( APIG722_Status, apiG722Encoder_Free, (G722Encoder_Obj* pEncoderObj))
G722_CODECAPI( APIG722_Status, apiG722Encode, (G722Encoder_Obj* pEncoderObj,
   Ipp32s nBitsPerFrame, Ipp32s frameSize, Ipp32s nRegions, Ipp16s *pSrc, Ipp16s *pDst))

G722_CODECAPI( APIG722_Status, apiG722Decoder_Alloc, (Ipp32u* objSize))
G722_CODECAPI( APIG722_Status, apiG722Decoder_Init, (G722Decoder_Obj* pDecoderObj))
G722_CODECAPI( APIG722_Status, apiG722Decoder_Free, (G722Decoder_Obj* pDecoderObj))
G722_CODECAPI( APIG722_Status, apiG722Decode, (G722Decoder_Obj* pDecoderObj, Ipp16s errFlag,
   Ipp32s nBitsPerFrame, Ipp32s frameSize, Ipp32s nRegions, Ipp16s *pSrc, Ipp16s *pDst))

G722_CODECAPI(APIG722_Status, apiG722GetObjSize, (void* pObj,
              Ipp32u *pObjSize))

#ifdef __cplusplus
}
#endif

#endif /* __G722API_H__ */
