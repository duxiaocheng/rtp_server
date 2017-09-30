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
// Purpose: G.722 speech codec: encode API.
//
*/

#include "owng722sb.h"

static Ipp32s EncoderObjSize(void){
   Ipp32s stSize;
   Ipp32s objSize=sizeof(G722SB_Encoder_Obj);
   ippsSBADPCMEncodeStateSize_G722_16s(&stSize);
   objSize += stSize; /* memory size in bytes */
   return objSize;
}

G722SB_CODECFUN( API_G722SB_Status, apiG722SBEncoder_Alloc, (Ipp32s *pSize))
{
   *pSize =  EncoderObjSize();
   return API_G722SB_StsNoErr;
}

G722SB_CODECFUN( API_G722SB_Status, apiG722SBEncoder_Init,
              (G722SB_Encoder_Obj* encoderObj, Ipp32u mode_qmf))
{
   Ipp8s* ptrMem = NULL;

   encoderObj->objPrm.objSize = EncoderObjSize();
   encoderObj->objPrm.mode = mode_qmf;
   encoderObj->objPrm.key = G722_SBAD_ENC_KEY;
   encoderObj->objPrm.rat = 1; /* 64 kbps default */

   ippsZero_16s(encoderObj->qmf_tx_delayx,  SBADPCM_G722_SIZE_QMFDELAY);
   ptrMem = (Ipp8s*)encoderObj + sizeof(G722SB_Encoder_Obj);
   encoderObj->stateEnc = (IppsEncoderState_G722_16s*)ptrMem;
   ippsSBADPCMEncodeInit_G722_16s(encoderObj->stateEnc);

   return API_G722SB_StsNoErr;
}

G722SB_CODECFUN(  API_G722SB_Status, apiG722SBEncode,
         (G722SB_Encoder_Obj* encoderObj, Ipp32s lenSamples, Ipp16s *pSrc, Ipp8s *pDst ))
{
   if(encoderObj->objPrm.mode == 1)
     g722Encode(encoderObj, pSrc, pDst, lenSamples);
   else
     g722NQMFEncode(encoderObj, pSrc, pDst, lenSamples);

   return API_G722SB_StsNoErr;
}



