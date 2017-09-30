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
// Purpose: G.722 speech codec: decode API.
//
*/

#include "owng722sb.h"

static Ipp32s DecoderObjSize(void){
   Ipp32s stSize;
   Ipp32s objSize=sizeof(G722SB_Decoder_Obj);
   ippsSBADPCMDecodeStateSize_G722_16s(&stSize);
   objSize += stSize; /* memory size in bytes */
   return objSize;
}

G722SB_CODECFUN( API_G722SB_Status, apiG722SBDecoder_Alloc, (Ipp32s *pSize))
{
   *pSize =  DecoderObjSize();
   return API_G722SB_StsNoErr;
}

G722SB_CODECFUN( API_G722SB_Status, apiG722SBDecoder_Init,
              (G722SB_Decoder_Obj* decoderObj, Ipp32u mode_qmf, Ipp32s inFrameSize))
{
   short i, frameSize;
   Ipp8s* ptrMem = NULL;

   decoderObj->objPrm.objSize = DecoderObjSize();
   decoderObj->objPrm.mode = mode_qmf;
   decoderObj->objPrm.key = G722_SBAD_DEC_KEY;
   decoderObj->objPrm.rat = 1; /* 64 kbps default */


   /*************************** For PLC Mode *************************************/
   frameSize = (short)((inFrameSize/sizeof(short)) >> 1);
   /* constants */
   decoderObj->frameSize = frameSize;
   /* bad frame indicator */
   decoderObj->prev_bfi = 0;
   for(i = 0; i < ORD_LPC; i++) { decoderObj->mem_syn[i] = 0; decoderObj->lpc[i] = 0; }
   decoderObj->lpc[ORD_LPC] = 0;
   decoderObj->t0 = 0;
   decoderObj->frameClass = IPP_G722_WEAKLY_VOICED;

   ippsZero_16s(decoderObj->mem_speech_lb, (MAXPIT2+16));
   ippsZero_16s(decoderObj->mem_speech_hb, LEN_HB_MEM);
   ippsZero_16s(decoderObj->mem_exc_lb, (MAXPIT2+8));

   decoderObj->good1stFrame = 1;
   /* higher-band hig-pass filtering */
   decoderObj->mem_hpf[0] = 0;
   decoderObj->mem_hpf[1] = 0;
   decoderObj->count_hpf = 32767;  /*max value*/

   /* adaptive muting */
   decoderObj->inc_att = 0;
   decoderObj->classWeight = (short*)tblWeight_V;
   decoderObj->count_lb = 0;
   decoderObj->count_hb = 0;
   /*******************************************************************/
   ippsZero_16s(decoderObj->qmf_rx_delayx,  SBADPCM_G722_SIZE_QMFDELAY);
   ptrMem = (Ipp8s*)decoderObj + sizeof(G722SB_Decoder_Obj);
   decoderObj->stateDec = (IppsDecoderState_G722_16s*)ptrMem;
   ippsSBADPCMDecodeInit_G722_16s(decoderObj->stateDec);
   /*******************************************************************/

   return API_G722SB_StsNoErr;
}


G722SB_CODECFUN(  API_G722SB_Status, apiG722SBDecode,
         (G722SB_Decoder_Obj* decoderObj, Ipp32s lenBitstream, Ipp16s mode, Ipp8s *pSrc, Ipp16s *pDst ))
{
   Ipp32s          plc_mode;

   if(mode < 1 || mode > 3) mode = (Ipp16s)decoderObj->objPrm.rat;
   else decoderObj->objPrm.rat = mode;

   if(pSrc == NULL) plc_mode = 1;
   else plc_mode = 0;

   if(decoderObj->objPrm.mode == 1) {
     if(plc_mode) {
       g722PLCConceal(decoderObj, pDst);
       decoderObj->prev_bfi = 1;
     } else {
       g722PLCDecode(decoderObj, pSrc, pDst, lenBitstream, mode);
       decoderObj->prev_bfi = 0;
     }
   } else {
     if(plc_mode)
       g722NQMFDecode(decoderObj, NULL, pDst, lenBitstream, mode);
     else
       g722NQMFDecode(decoderObj, pSrc, pDst, lenBitstream, mode);
   }
   return API_G722SB_StsNoErr;
}



