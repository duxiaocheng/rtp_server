/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2004-2011 Intel Corporation. All Rights Reserved.
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
// Purpose: G711 speech codec: decoder API.
//
*/

#include <ippsc.h>
#include "owng711.h"
#include "g729api.h"

/* PF coefficients */
static Ipp32s DecoderObjSize(void) {
    Ipp32s objSize = sizeof(G711Decoder_Obj);
    Ipp32s g729_size;
    apiG729Decoder_Alloc(G729A_CODEC, &g729_size);
    objSize += g729_size;
    return objSize;
}

G711_CODECFUN( APIG711_Status, apiG711Decoder_Alloc,
               (Ipp32s *pCodecSize)) {
   if (NULL==pCodecSize)
      return APIG711_StsBadArgErr;
   *pCodecSize = DecoderObjSize();
   return APIG711_StsNoErr;
}
#define G711_PST_OFF 0
#define G711_PST_ON  1

G711_CODECFUN( APIG711_Status, apiG711Decoder_Init,
               (G711Decoder_Obj* decoderObj, G711Codec_Type codecType)) {
    Ipp8s* oldMemBuff;

    if((codecType != G711_ALAW_CODEC)&&(codecType != G711_MULAW_CODEC)) {
        return APIG711_StsBadCodecType;
    }

    oldMemBuff = decoderObj->Mem.base; /* if Reinit */

    ippsZero_8u((Ipp8u*)decoderObj,sizeof(G711Decoder_Obj)) ;

    decoderObj->objPrm.objSize = DecoderObjSize();
    decoderObj->objPrm.key = DEC_KEY;
    decoderObj->objPrm.codecType=codecType;

    decoderObj->codecType=codecType;

    decoderObj->g729_obj = (Ipp8s*)decoderObj + sizeof(G711Decoder_Obj);

    PLC_init(&decoderObj->PLCstate);
    decoderObj->lastGoodFrame = G711_Voice_Frame;

    apiG729Decoder_InitBuff(decoderObj->g729_obj,oldMemBuff);

    apiG729Decoder_Init(decoderObj->g729_obj, G729A_CODEC);
    apiG729Decoder_Mode(decoderObj->g729_obj,G711_PST_OFF);

    return APIG711_StsNoErr;
}

G711_CODECFUN( APIG711_Status, apiG711Decoder_Mode,
               (G711Decoder_Obj* decoderObj, Ipp32s mode))
{
   if(NULL==decoderObj)
      return APIG711_StsBadArgErr;
   if((mode<G711_PST_OFF)&&(mode>G711_PST_ON))
      return APIG711_StsBadArgErr;

   apiG729Decoder_Mode(decoderObj->g729_obj,mode);
   return APIG711_StsNoErr;
}

G711_CODECFUN( APIG711_Status, apiG711Decoder_InitBuff,
               (G711Decoder_Obj* decoderObj, Ipp8s *buff)) {
   APIG729_Status sts729;
   if(NULL==decoderObj || NULL==buff)
      return APIG711_StsBadArgErr;

   decoderObj->Mem.base = buff;

   sts729 = apiG729Decoder_InitBuff(decoderObj->g729_obj, buff);
   if(APIG729_StsBadArgErr==sts729) return APIG711_StsBadArgErr;
   return APIG711_StsNoErr;
}

G711_CODECFUN( APIG711_Status, apiG711Decode,
              (G711Decoder_Obj* decoderObj,const Ipp8u* src, Ipp32s frametype, Ipp16s* dst)) {
  Ipp32s lastGoodFrame;

  if(NULL==decoderObj || NULL==src || NULL ==dst)
    return APIG711_StsBadArgErr;
  if(decoderObj->objPrm.objSize <= 0)
    return APIG711_StsNotInitialized;
  if(DEC_KEY != decoderObj->objPrm.key)
    return APIG711_StsBadCodecType;

  lastGoodFrame = decoderObj->lastGoodFrame;

  if (frametype == G711_Bad_Frame) {
     if (lastGoodFrame == G711_SID_Frame) {
        frametype = G711_Untransmitted_Frame; /* CNG on last SID information */
     } else {
        PLC_dofe(&decoderObj->PLCstate, dst); /* Packet Loss Concealment */
        return APIG711_StsNoErr;
     }
  }

  if (frametype == G711_Voice_Frame) {
    if (decoderObj->codecType == G711_ALAW_CODEC) {
      ippsALawToLin_8u16s(src, dst, LP_FRAME_DIM);
    } else { /* G711_MULAW_CODEC */
      ippsMuLawToLin_8u16s(src, dst, LP_FRAME_DIM);
    }
  } else {
    Ipp32s ftype;
    if (frametype == G711_SID_Frame) {
      ftype = G729_SID_Frame;
    } else if (frametype == G711_Untransmitted_Frame) {
      ftype = G729_Untransmitted_Frame;
    } else {
      return APIG711_StsBadArgErr;
    }
    if (decoderObj->lastGoodFrame == G711_Voice_Frame) { /* starting of silence period */
      if (frametype == G711_SID_Frame) {
        ftype = G729_FirstSID_Frame; /* first SID of silence period */
      } else { /* G711_Untransmitted_Frame */
          PLC_dofe(&decoderObj->PLCstate, dst); /* Packet Loss Concealment */
          return APIG711_StsNoErr;
      }
    }
    apiG729Decode(decoderObj->g729_obj,src,ftype,dst);
  }

  PLC_addtohistory(&decoderObj->PLCstate, dst); /* delay by 30 samples is here */

  if (frametype != G711_Untransmitted_Frame) {
    decoderObj->lastGoodFrame = frametype; /* save last frame type */
  }

  return APIG711_StsNoErr;
}
