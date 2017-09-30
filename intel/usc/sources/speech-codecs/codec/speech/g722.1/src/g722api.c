/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2012 Intel Corporation. All Rights Reserved.
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
// Purpose: G.722.1 speech codec: API functions.
//
*/

#include "owng722.h"
#include "g722api.h"


G722_CODECFUN( APIG722_Status, apiG722Encoder_Alloc,(Ipp32u* objSize)){
   *objSize = sizeof(G722Encoder_Obj);
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Encoder_Free,(G722Encoder_Obj* pEncoderObj)){
   if(pEncoderObj == NULL) return APIG722_StsBadArgErr;
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Encoder_Init,(G722Encoder_Obj* pEncoderObj)){
   Ipp32s i;
   for (i=0;i<G722_FRAMESIZE_MAX;i++) pEncoderObj->history[i] = 0;
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Encode, (G722Encoder_Obj* pEncoderObj, Ipp32s bitFrameSize,
              Ipp32s frameSize, Ipp32s nRegions, Ipp16s *pSrc, Ipp16s *pDst))
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, vecMlt, G722_FRAMESIZE_MAX);
   IPP_ALIGNED_ARRAY(16, Ipp16s, vecWndData, G722_DCT_LENGTH_MAX);
   Ipp16s scale;
   /* Convert input samples to rmlt coefs */
   ippsDecomposeMLTToDCT_G7221_16s(pSrc, pEncoderObj->history, vecWndData, frameSize);
   NormalizeWndData(vecWndData, &scale, frameSize);
   ippsDCTFwd_G7221_16s(vecWndData, vecMlt, frameSize);
   /* Encode the mlt coefs */
   EncodeFrame(bitFrameSize, nRegions, vecMlt, scale, pDst);
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Decoder_Alloc,(Ipp32u* objSize)){
   *objSize = sizeof(G722Decoder_Obj);
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Decoder_Free,(G722Decoder_Obj* pDecoderObj)){
   if(pDecoderObj == NULL) return APIG722_StsBadArgErr;
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Decoder_Init,(G722Decoder_Obj* pDecoderObj)){
   Ipp32s i;
   /* initialize the coefs history */
   for (i=0; i<G722_DCT_LENGTH_MAX; i++)
      pDecoderObj->vecOldMlt[i] = 0;
   for (i=0; i<(G722_DCT_LENGTH_MAX>> 1); i++)
      pDecoderObj->vecOldSmpls[i] = 0;
   pDecoderObj->prevScale= 0;
   /* initialize the random number generator */
   pDecoderObj->randVec[0] = 1;
   pDecoderObj->randVec[1] = 1;
   pDecoderObj->randVec[2] = 1;
   pDecoderObj->randVec[3] = 1;
   return APIG722_StsNoErr;
}

G722_CODECFUN( APIG722_Status, apiG722Decode, (G722Decoder_Obj* pDecoderObj, Ipp16s errFlag,
      Ipp32s nBitsPerFrame, Ipp32s frameSize, Ipp32s nRegions, Ipp16s *pSrc, Ipp16s *pDst))
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, vecNewSmpls, G722_DCT_LENGTH_MAX);
   IPP_ALIGNED_ARRAY(16, Ipp16s, vecMlt, G722_DCT_LENGTH_MAX);
   Ipp16s scale;
   /* reinit the current word to point to the start of the buffer */
   pDecoderObj->bitObj.pBitstream = pSrc;
   pDecoderObj->bitObj.curWord = *pSrc;
   pDecoderObj->bitObj.bitCount = 0;
   pDecoderObj->bitObj.curBitsNumber = (Ipp16s)nBitsPerFrame;
   /* process the input samples into decoder Mlt coefs */
   DecodeFrame(&pDecoderObj->bitObj, pDecoderObj->randVec, nRegions, vecMlt, &scale,
      &pDecoderObj->prevScale, pDecoderObj->vecOldMlt, errFlag);
   /* Convert the decoder mlt coefs  to samples (inverse DCT)*/
   ippsDCTInv_G7221_16s(vecMlt, vecNewSmpls, frameSize);
   if (scale > 0)
      ippsRShiftC_16s(vecNewSmpls, scale, vecNewSmpls, frameSize);
   else if (scale < 0)
      ippsLShiftC_16s(vecNewSmpls, -scale, vecNewSmpls, frameSize);
   ippsDecomposeDCTToMLT_G7221_16s(vecNewSmpls, pDecoderObj->vecOldSmpls, pDst, frameSize);
   return APIG722_StsNoErr;
}

G722_CODECFUN(APIG722_Status, apiG722GetObjSize, (void* pObj, Ipp32u *pObjSize)){
   own_G722_Obj_t *pG722CodecObj = (own_G722_Obj_t*)pObj;
   if (pG722CodecObj == NULL) return APIG722_StsBadArgErr;
   *pObjSize = pG722CodecObj->objSize;
   return APIG722_StsNoErr;
}
