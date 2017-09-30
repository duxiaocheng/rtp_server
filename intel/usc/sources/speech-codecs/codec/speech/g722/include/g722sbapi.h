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
// Purpose: G.722 speech codec: API header file.
//
*/

#ifndef __G722SBAPI_H__
#define __G722SBAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#define G722_SBAD_ENC_KEY 0xecd722
#define G722_SBAD_DEC_KEY 0xdec722

typedef struct {
   Ipp32s        objSize;
   Ipp32s        key;
   Ipp32u        mode;       /* mode's */
   Ipp32u        rat: 1;     /* decode rate: 1-64kbit/s, 2-56kbit/s, 3-48kbit/s other bits reserved.*/
}G722SB_Obj_t;

typedef enum{
    API_G722SB_StsBadCodecType     =   -5,
    API_G722SB_StsNotInitialized   =   -4,
    API_G722SB_StsBadArgErr        =   -3,
    API_G722SB_StsDeactivated      =   -2,
    API_G722SB_StsErr              =   -1,
    API_G722SB_StsNoErr            =    0
}API_G722SB_Status;

struct _G722SB_Encoder_Obj;
struct _G722SB_Decoder_Obj;
typedef struct _G722SB_Encoder_Obj G722SB_Encoder_Obj;
typedef struct _G722SB_Decoder_Obj G722SB_Decoder_Obj;

#define   G722SB_CODECAPI( type,name,arg )        extern type name arg;

G722SB_CODECAPI( API_G722SB_Status, apiG722SBEncoder_Alloc, (Ipp32s *pSize))
G722SB_CODECAPI( API_G722SB_Status, apiG722SBEncoder_Init, (G722SB_Encoder_Obj* encoderObj, Ipp32u mode_qmf))
G722SB_CODECAPI( API_G722SB_Status, apiG722SBEncode, (G722SB_Encoder_Obj* encoderObj,
                  Ipp32s lenSamples, Ipp16s *pSrc, Ipp8s *pDst ))

G722SB_CODECAPI( API_G722SB_Status, apiG722SBDecoder_Alloc, (Ipp32s *pSize))
G722SB_CODECAPI( API_G722SB_Status, apiG722SBDecoder_Init, (G722SB_Decoder_Obj* decoderObj, Ipp32u mode_qmf, Ipp32s inFrameSize))
G722SB_CODECAPI( API_G722SB_Status, apiG722SBDecode, (G722SB_Decoder_Obj* decoderObj,
                  Ipp32s lenBitstream, Ipp16s mode, Ipp8s *pSrc, Ipp16s *pDst ))

#ifdef __cplusplus
}
#endif

#endif /* __G722SBAPI_H__ */
