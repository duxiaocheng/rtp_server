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
// Purpose: AMRWBE speech codec API header.
//
*/

#ifndef __APIAMRWBE_H__
#define __APIAMRWBE_H__

#include <ipps.h>
#include <ippsc.h>
#include "amrwbapi.h"
#include "typesamrwbe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _CodecType_AMRWBE{
   AMRWBE_CODEC=0
}CodecType_AMRWBE;

typedef enum _EncodeMode_AMRWBE{
   EncodeDefaultMode_AMRWBE=0,
   EncodeVADEnabled_AMRWBE
}EncodeMode_AMRWBE;


typedef struct _EncParams_AMRWBE{
   CodecType_AMRWBE codecType;
   Ipp32s mode;
}EncParams_AMRWBE;


struct _EncoderObj_AMRWBE;
typedef struct _EncoderObj_AMRWBE EncoderObj_AMRWBE;
struct _DecoderObj_AMRWBE;
typedef struct _DecoderObj_AMRWBE DecoderObj_AMRWBE;

/*
                   Functions declarations
*/
/* encoder */
AMRWB_CODECAPI( APIAMRWB_Status, apiEncoderGetObjSize_AMRWBE, (Ipp32u *pCodecSize))
AMRWB_CODECAPI( APIAMRWB_Status, apiEncoderInit_AMRWBE,
            (EncoderObj_AMRWBE* encoderObj, Ipp32s isf, Ipp32s mode, Ipp32s fullReset))
AMRWB_CODECAPI( APIAMRWB_Status, apiEncoderProceedFirst_AMRWBE,
   (EncoderObj_AMRWBE* pEncObj, Ipp16s* channel_right, Ipp16s* channel_left,
   Ipp16s n_channel, Ipp32s frameSize, Ipp16s fscale, Ipp32s* pCountSamplesProceeded))
AMRWB_CODECAPI( APIAMRWB_Status, apiEncoderStereo_AMRWBE,
   (EncoderObj_AMRWBE *st, Ipp16s* channel_right, Ipp16s* channel_left, Ipp32s codec_mode,
   Ipp32s frameSize, Ipp16s* serial, Ipp32s use_case_mode, Ipp32s fscale, Ipp32s StbrMode,
   Ipp32s* pCountSamplesProceeded))
AMRWB_CODECAPI( APIAMRWB_Status, apiEncoderMono_AMRWBE,
   (EncoderObj_AMRWBE *st, Ipp16s *channel_right, Ipp32s codec_mode, Ipp32s frameSize,
   Ipp16s *serial, Ipp32s use_case_mode, Ipp32s fscale, Ipp32s *pCountSamplesProceeded))
AMRWB_CODECAPI( APIAMRWB_Status, apiCopyEncoderState_AMRWBE,
   (EncoderObj_AMRWBE* pObjWBE, AMRWBEncoder_Obj* pObjWB, Ipp32s vad, Ipp32s direction))
/* decoder */
AMRWB_CODECAPI( APIAMRWB_Status, apiDecoderGetObjSize_AMRWBE, (Ipp32u *pCodecSize))
AMRWB_CODECAPI( APIAMRWB_Status, apiDecoderInit_AMRWBE,
   (DecoderObj_AMRWBE* decoderObj, Ipp32s isf, Ipp32s nChannels, Ipp32s fullReset))
AMRWB_CODECAPI( APIAMRWB_Status, apiDecoder_AMRWBE,
   (DecoderObj_AMRWBE *st, Ipp16s* serial, Ipp32s *bad_frame, Ipp32s frameSize, Ipp32s n_channel,
   Ipp16s *channel_right, Ipp16s *channel_left, Ipp32s codec_mode, Ipp32s StbrMode,
   Ipp16s fscale, Ipp16s upsamp_fscale, Ipp32s *pSamplesCount, Ipp32s* pFrameScale))
AMRWB_CODECAPI( APIAMRWB_Status, apiCopyDecoderState_AMRWBE,
         (DecoderObj_AMRWBE* pObjWBE, AMRWBDecoder_Obj* pObjWB, Ipp32s direction))

#ifdef __cplusplus
}
#endif

#endif /* __APIAMRWBE_H__ */
