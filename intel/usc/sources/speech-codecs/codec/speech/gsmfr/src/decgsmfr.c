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
// Purpose: GSM FR 06.10: decoder API.
//
*/

#include    <ippsc.h>
#include    "owngsmfr.h"

/* Table GSM 06.10 5.5 Normalized inverse mantissa used to compute xM/xmax */
static __ALIGN32 CONST Ipp16s FAC[8] = { 18431, 20479, 22527, 24575, 26623, 28671, 30719, 32767};

/*Ipp16s term residual signal == ST RS*/
GSMFR_CODECFUN( APIGSMFR_Status, apiGSMFRDecoder_GetSize,
         (GSMFRDecoder_Obj* decoderObj, Ipp32u *pCodecSize))
{
   if(NULL == decoderObj)
      return APIGSMFR_StsBadArgErr;
   if(NULL == pCodecSize)
      return APIGSMFR_StsBadArgErr;
   if(decoderObj->objPrm.key != DEC_KEY)
      return APIGSMFR_StsNotInitialized;

   *pCodecSize = decoderObj->objPrm.objSize;
   return APIGSMFR_StsNoErr;
}
GSMFR_CODECFUN( APIGSMFR_Status, apiGSMFRDecoder_Alloc,
                (Ipp32s *pSizeTab)) {
   Ipp32s allocSize=sizeof(GSMFRDecoder_Obj);
   pSizeTab[0] =  allocSize;
   return APIGSMFR_StsNoErr;
}
GSMFR_CODECFUN( APIGSMFR_Status, apiGSMFRDecoder_Init, (GSMFRDecoder_Obj *decoderObj,
                                                        GSMFR_Params_t *params )) {
   if(!decoderObj) return APIGSMFR_StsBadArgErr;
   ippsZero_8u((Ipp8u*)decoderObj,sizeof(GSMFRDecoder_Obj));
   decoderObj->objPrm.cutLTPFlag = params->ltp_f;
   decoderObj->objPrm.key = DEC_KEY;

   return APIGSMFR_StsNoErr;
}
/* Table 5.3b Quantization levels of the LTP gain quantizer
* bc 0 1 2 3
* GSM 06.10 5.3.2. bcr and Ncr parameters were used for
* LT synthesis filtering.  The decoding of bcr needs
* table 5.3b.
* Compute reconstructed ST RS drp
*/
static __ALIGN32 CONST Ipp16s gsm_QLB[4] = {  3277,    11469,    21299,     32767};
static void GsmLongTSF(GSMFRDecoder_Obj *pObj, Ipp16s Ncr, Ipp16s idx, Ipp16s *erp, /* 40 in */
                Ipp16s *drp) /*[-120..-1] in, [-120..40] out */
{
  Ipp16s tmp[40], Nr;
   Nr = (Ipp16s)(Ncr < 40 || Ncr > 120 ? pObj->nrp : Ncr);
   pObj->nrp = Nr;
   ippsMulC_NR_16s_Sfs( &drp[-Nr],gsm_QLB[idx],tmp,40,15);
   ippsAdd_16s_Sfs(erp,tmp,drp,40,0);
   ippsMove_16s(&drp[ -80], &drp[ -120],120);
}
static void GsmShortTSF(Ipp16s *LARpp_j ,
                       const Ipp16s *LARpp_j_1,
                       const Ipp16s *LARcr, /* 8 in */
                       const Ipp16s *wt,    /* 160 in  */
                       Ipp16s *s,           /* 160 out */
                       Ipp16s *pMem)
{
   Ipp16s LARp[4*8];
   DecCodLAR( LARcr, LARpp_j,8 );
   /* 5.2.9.1 */
   Interpolate_GSMFR_16s( LARpp_j_1, LARpp_j, LARp,8 );
   ippsInterpolate_G729_16s (LARpp_j_1,LARpp_j,LARp+8,8);
   Interpolate_GSMFR_16s( LARpp_j,LARpp_j_1,  LARp+16,8);
   ippsCopy_16s( LARpp_j, LARp+24,8 );

   LARp2RC( LARp, 32 );
   ippsShortTermSynthesisFilter_GSMFR_16s( LARp   , wt     , s     , 13, pMem);
   ippsShortTermSynthesisFilter_GSMFR_16s( LARp+8 , wt + 13, s + 13, 14, pMem);
   ippsShortTermSynthesisFilter_GSMFR_16s( LARp+16, wt + 27, s + 27, 13, pMem);
   ippsShortTermSynthesisFilter_GSMFR_16s( LARp+24, wt + 40, s + 40,120, pMem);
}
static Ipp32s Gsm_Decoder(GSMFRDecoder_Obj *pObj,
                        Ipp16s *LARcr,  /*8 in*/
                        Ipp16s *Ncr,    /*4 in*/
                        Ipp16s *bcr,    /*4 in*/
                        Ipp16s *Mcr,    /*4 in*/
                        Ipp16s *xmaxcr, /*4 in*/
                        Ipp16s *xMcr,   /*[14*4]in*/
                        Ipp16s *s) {    /*160 out*/
   Ipp32s  j;
   Ipp16s  erp[40], wt[GSMFR_EXT_FRAME_LEN];
   Ipp16s *drp = pObj->dp0 + 120;
   for(j=0; j <4; j++, xMcr += 13) {
      GsmRPEDec(xmaxcr[j], Mcr[j], xMcr, erp );
      GsmLongTSF( pObj, Ncr[j], bcr[j], erp, drp );
      ippsCopy_16s(drp,&wt[j*40],40);
   }
   GsmShortTSF(pObj->LARpp[ pObj->idx],pObj->LARpp[ pObj->idx^1],LARcr, wt, s, pObj->v );
   pObj->idx^= 1;
   ippsDeemphasize_GSMFR_16s_I( s,GSMFR_EXT_FRAME_LEN,&pObj->msr);
   return 0;
}
GSMFR_CODECFUN( APIGSMFR_Status, apiGSMFRDecode,
                ( GSMFRDecoder_Obj *pObj,const uchar *c, Ipp16s *dst)) {
   Ipp16s LLARc[8], NNc[4], MMc[4], Bbc[4], Xxmaxc[4], XxMMc[13*4];
   LLARc[0]  =(Ipp16s)( (c[0] & 0xF) << 2);
   LLARc[0] |=(Ipp16s)( (c[1] >> 6) & 0x3);
   LLARc[1]  =(Ipp16s)( c[1] & 0x3F);
   LLARc[2]  =(Ipp16s)( (c[2] >> 3) & 0x1F);
   LLARc[3]  =(Ipp16s)( (c[2] & 0x7) << 2);
   LLARc[3] |=(Ipp16s)( (c[3] >> 6) & 0x3);
   LLARc[4]  =(Ipp16s)( (c[3] >> 2) & 0xF);
   LLARc[5]  =(Ipp16s)( (c[3] & 0x3) << 2);
   LLARc[5] |=(Ipp16s)( (c[4] >> 6) & 0x3);
   LLARc[6]  =(Ipp16s)( (c[4] >> 3) & 0x7);
   LLARc[7]  =(Ipp16s)( c[4] & 0x7);
   NNc[0]    =(Ipp16s)( (c[5] >> 1) & 0x7F);
   NNc[1]     =(Ipp16s)( (c[12] >> 1) & 0x7F);
   NNc[2]     =(Ipp16s)( (c[19] >> 1) & 0x7F);
   NNc[3]     =(Ipp16s)( (c[26] >> 1) & 0x7F);
   Bbc[0]    =(Ipp16s)( (c[5] & 0x1) << 1);
   Bbc[0]   |=(Ipp16s)( (c[6] >> 7) & 0x1);
   MMc[0]    =(Ipp16s)( (c[6] >> 5) & 0x3);
   MMc[1]     =(Ipp16s)( (c[13] >> 5) & 0x3);
   MMc[2]     =(Ipp16s)( (c[20] >> 5) & 0x3);
   MMc[3]     =(Ipp16s)( (c[27] >> 5) & 0x3);
   Xxmaxc[0] =(Ipp16s)( (c[6] & 0x1F) << 1);
   Xxmaxc[0]|=(Ipp16s)( (c[7] >> 7) & 0x1);
   XxMMc[0]  =(Ipp16s)( (c[7] >> 4) & 0x7);
   XxMMc[1]  =(Ipp16s)( (c[7] >> 1) & 0x7);
   XxMMc[2]  =(Ipp16s)( (c[7] & 0x1) << 2);
   XxMMc[2] |=(Ipp16s)( (c[8] >> 6) & 0x3);
   XxMMc[3]  =(Ipp16s)( (c[8] >> 3) & 0x7);
   XxMMc[4]  =(Ipp16s)( c[8] & 0x7);
   XxMMc[5]  =(Ipp16s)( (c[9] >> 5) & 0x7);
   XxMMc[6]  =(Ipp16s)( (c[9] >> 2) & 0x7);
   XxMMc[7]  =(Ipp16s)( (c[9] & 0x3) << 1);
   XxMMc[7] |=(Ipp16s)( (c[10] >> 7) & 0x1);
   XxMMc[8]  =(Ipp16s)( (c[10] >> 4) & 0x7);
   XxMMc[9]  =(Ipp16s)( (c[10] >> 1) & 0x7);
   XxMMc[10]  =(Ipp16s)( (c[10] & 0x1) << 2);
   XxMMc[10] |=(Ipp16s)( (c[11] >> 6) & 0x3);
   XxMMc[11]  =(Ipp16s)( (c[11] >> 3) & 0x7);
   XxMMc[12]  =(Ipp16s)( c[11] & 0x7);
   Bbc[1]     =(Ipp16s)( (c[12] & 0x1) << 1);
   Bbc[1]    |=(Ipp16s)( (c[13] >> 7) & 0x1);
   Xxmaxc[1]  =(Ipp16s)( (c[13] & 0x1F) << 1);
   Xxmaxc[1] |=(Ipp16s)( (c[14] >> 7) & 0x1);
   XxMMc[13]  =(Ipp16s)( (c[14] >> 4) & 0x7);
   XxMMc[14]  =(Ipp16s)( (c[14] >> 1) & 0x7);
   XxMMc[15]  =(Ipp16s)( (c[14] & 0x1) << 2);
   XxMMc[15] |=(Ipp16s)( (c[15] >> 6) & 0x3);
   XxMMc[16]  =(Ipp16s)( (c[15] >> 3) & 0x7);
   XxMMc[17]  =(Ipp16s)( c[15] & 0x7);
   XxMMc[18]  =(Ipp16s)( (c[16] >> 5) & 0x7);
   XxMMc[19]  =(Ipp16s)( (c[16] >> 2) & 0x7);
   XxMMc[20]  =(Ipp16s)( (c[16] & 0x3) << 1);
   XxMMc[20] |=(Ipp16s)( (c[17] >> 7) & 0x1);
   XxMMc[21]  =(Ipp16s)( (c[17] >> 4) & 0x7);
   XxMMc[22]  =(Ipp16s)( (c[17] >> 1) & 0x7);
   XxMMc[23]  =(Ipp16s)( (c[17] & 0x1) << 2);
   XxMMc[23] |=(Ipp16s)( (c[18] >> 6) & 0x3);
   XxMMc[24]  =(Ipp16s)( (c[18] >> 3) & 0x7);
   XxMMc[25]  =(Ipp16s)( c[18] & 0x7);
   Bbc[2]     =(Ipp16s)( (c[19] & 0x1) << 1);
   Bbc[2]    |=(Ipp16s)( (c[20] >> 7) & 0x1);
   Xxmaxc[2]  =(Ipp16s)( (c[20] & 0x1F) << 1);
   Xxmaxc[2] |=(Ipp16s)( (c[21] >> 7) & 0x1);
   XxMMc[26]  =(Ipp16s)( (c[21] >> 4) & 0x7);
   XxMMc[27]  =(Ipp16s)( (c[21] >> 1) & 0x7);
   XxMMc[28]  =(Ipp16s)( (c[21] & 0x1) << 2);
   XxMMc[28] |=(Ipp16s)( (c[22] >> 6) & 0x3);
   XxMMc[29]  =(Ipp16s)( (c[22] >> 3) & 0x7);
   XxMMc[30]  =(Ipp16s)( c[22] & 0x7);
   XxMMc[31]  =(Ipp16s)( (c[23] >> 5) & 0x7);
   XxMMc[32]  =(Ipp16s)( (c[23] >> 2) & 0x7);
   XxMMc[33]  =(Ipp16s)( (c[23] & 0x3) << 1);
   XxMMc[33] |=(Ipp16s)( (c[24] >> 7) & 0x1);
   XxMMc[34]  =(Ipp16s)( (c[24] >> 4) & 0x7);
   XxMMc[35]  =(Ipp16s)( (c[24] >> 1) & 0x7);
   XxMMc[36]  =(Ipp16s)( (c[24] & 0x1) << 2);
   XxMMc[36] |=(Ipp16s)( (c[25] >> 6) & 0x3);
   XxMMc[37]  =(Ipp16s)( (c[25] >> 3) & 0x7);
   XxMMc[38]  =(Ipp16s)( c[25] & 0x7);
   Bbc[3]     =(Ipp16s)( (c[26] & 0x1) << 1);
   Bbc[3]    |=(Ipp16s)( (c[27] >> 7) & 0x1);
   Xxmaxc[3]  =(Ipp16s)( (c[27] & 0x1F) << 1);
   Xxmaxc[3] |=(Ipp16s)( (c[28] >> 7) & 0x1);
   XxMMc[39]  =(Ipp16s)( (c[28] >> 4) & 0x7);
   XxMMc[40]  =(Ipp16s)( (c[28] >> 1) & 0x7);
   XxMMc[41]  =(Ipp16s)( (c[28] & 0x1) << 2);
   XxMMc[41] |=(Ipp16s)( (c[29] >> 6) & 0x3);
   XxMMc[42]  =(Ipp16s)( (c[29] >> 3) & 0x7);
   XxMMc[43]  =(Ipp16s)( c[29] & 0x7);
   XxMMc[44]  =(Ipp16s)( (c[30] >> 5) & 0x7);
   XxMMc[45]  =(Ipp16s)( (c[30] >> 2) & 0x7);
   XxMMc[46]  =(Ipp16s)( (c[30] & 0x3) << 1);
   XxMMc[46] |=(Ipp16s)( (c[31] >> 7) & 0x1);
   XxMMc[47]  =(Ipp16s)( (c[31] >> 4) & 0x7);
   XxMMc[48]  =(Ipp16s)( (c[31] >> 1) & 0x7);
   XxMMc[49]  =(Ipp16s)( (c[31] & 0x1) << 2);
   XxMMc[49] |=(Ipp16s)( (c[32] >> 6) & 0x3);
   XxMMc[50]  =(Ipp16s)( (c[32] >> 3) & 0x7);
   XxMMc[51]  =(Ipp16s)( c[32] & 0x7);
   Gsm_Decoder(pObj, LLARc, NNc, Bbc, MMc, Xxmaxc, XxMMc, dst);
   return APIGSMFR_StsNoErr;
}

Ipp32s GsmRPEDec(Ipp16s xmaxcr, Ipp16s Mcr, Ipp16s *xMcr,  /*13 (3 bits) in*/
               Ipp16s *erp)/*40 out */
{
   Ipp16s exp, mant;
   Ipp16s xMp[ 13 ];

    APCMQuant( xmaxcr, &exp, &mant );
    ippsRPEQuantDecode_GSMFR_16s( xMcr, FAC[mant], (Ipp16s)(6-exp), xMp );
    ippsZero_16s(erp,40);
    RPEGridPos( xMp, Mcr, erp );
    return 0;
}
