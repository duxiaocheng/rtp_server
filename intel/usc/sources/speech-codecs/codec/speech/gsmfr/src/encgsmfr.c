/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2004-2012 Intel Corporation. All Rights Reserved.
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
// Purpose: GSM FR 06.10: encoder API.
//
*/

#include    <ippsc.h>
#include    "owngsmfr.h"
static __ALIGN32 CONST Ipp16s FAC[8] = { 18431, 20479, 22527,
 24575, 26623, 28671,
 30719, 32767};
static __ALIGN32 CONST Ipp16s DLB[4] = {6554,16384,
26214,32767};
static __ALIGN32 CONST Ipp16s swTab[4]={3277,11469,
21299,32767};
static void acfEnComp(GSMFREncoder_Obj *encoderObj,const Ipp32s *L_ACF);
static void acfEverag(GSMFREncoder_Obj *encoderObj,const Ipp32s *L_ACF);
static void spectComp(GSMFREncoder_Obj *encoderObj);
static void periodDetect(GSMFREncoder_Obj *encoderObj);
static void threshAdapt(GSMFREncoder_Obj *encoderObj);
static void vadDecision(GSMFREncoder_Obj *encoderObj);
static void vadHangoverAdd(GSMFREncoder_Obj *encoderObj );
static void periodUpdate(GSMFREncoder_Obj *encoderObj,Ipp16s lags[4]);
static Ipp32s  toneDetect(GSMFREncoder_Obj *encoderObj);
static void saveParm(Ipp16s *pSrc,Ipp16s *pDst,Ipp32s *index,Ipp32s len);
static void larAverage(Ipp16s *a,Ipp16s *LAR,Ipp32s len);
static void xmaxAverage(Ipp16s *a,Ipp16s *xmax);
static Ipp32s  Dtx_Encoder_Count(GSMFREncoder_Obj *encoderObj,Ipp16s *LAR,Ipp16s *xmax,uchar *dst);
static void newSIDFrame(Ipp16s *a,Ipp16s *b,uchar dst[33]);
GSMFR_CODECFUN( APIGSMFR_Status, apiGSMFREncoder_GetSize,
         (GSMFREncoder_Obj* encoderObj, Ipp32u *pCodecSize))
{
   if(NULL == encoderObj)
      return APIGSMFR_StsBadArgErr;
   if(NULL == pCodecSize)
      return APIGSMFR_StsBadArgErr;
   if(encoderObj->objPrm.key != ENC_KEY)
      return APIGSMFR_StsNotInitialized;

   *pCodecSize = encoderObj->objPrm.objSize;
   return APIGSMFR_StsNoErr;
}

static Ipp32s AutoCorr1(Ipp16s *pSrcDst, Ipp32s len, Ipp32s *pDst,Ipp32s lenDst, Ipp32s *pNorm)
{
   Ipp16s tmp, sMaxVal;

   ippsMax_16s(pSrcDst,len,&sMaxVal);
   ippsMin_16s(pSrcDst,len,&tmp);
   if(-tmp > sMaxVal)
       sMaxVal = (Ipp16s)(-tmp);
   if(sMaxVal == 0)
       *pNorm = 0;
   else
      *pNorm = 4-Exp_32s_Pos( (Ipp32s)sMaxVal << 16);
   if(*pNorm > 0) {
      tmp = (Ipp16s)(16384 >>(*pNorm-1));
      ippsMulC_NR_16s_ISfs( tmp,pSrcDst,len,15 );
   }
   ippsAutoCorr_16s32s(pSrcDst,len,pDst, lenDst );
   return 0;
}
static Ipp32s AutoCorr0(Ipp16s *pSrcDst, Ipp32s len, Ipp32s *pDst,Ipp32s lenDst, Ipp32s *pNorm)
{
   AutoCorr1( pSrcDst,  len, pDst, lenDst, pNorm);
   if(*pNorm > 0)
      ippsLShiftC_16s(pSrcDst,*pNorm,pSrcDst,len);
   return 0;
}
static Ipp32s GsmLPCAnal(GSMFREncoder_Obj *encoderObj, Ipp16s *pSrcDst, Ipp16s *pDst)
{
   IPP_ALIGNED_ARRAY(16, Ipp32s, L_ACF, 9);

   AutoCorr0(pSrcDst,160,L_ACF, 9,&encoderObj->scalauto);
   ippsLShiftC_32s(L_ACF,1,L_ACF,9);
   if(encoderObj->vadMode == GSMFR_VAD_ON ){
      acfEnComp(encoderObj, L_ACF);  /* do pvad for threshAdapt() & vadDecision()*/
      acfEverag(encoderObj, L_ACF);  /* do av1 for ippspredValComp() & av0 for spectComp()*/
   }
   ippsSchur_GSMFR_32s16s(L_ACF,pDst,8);
   Ref2LAR(pDst,8);
   QuantCoding(pDst,8);
   return 0;
}
static void CutLTPParm(Ipp16s *dp,const Ipp16s *d,Ipp16s *dst1,Ipp16s *dst2) {
  Ipp32s j, maxL, powerL, resultL, maxInd, minInd;
  Ipp16s pTmp[40], NcVal, bcVal, R, S, sMaxVal, scal, tmp, wtmp;

   ippsMaxIndx_16s(d,40,&sMaxVal,&maxInd);
   ippsMinIndx_16s(d,40,&tmp,&minInd);
   if(-tmp > sMaxVal) {
      sMaxVal   = (Ipp16s)(-tmp);
      maxInd = minInd;
   }
   if(sMaxVal == 0) {
      tmp = scal = 0;
   }
   else {
      tmp = Exp_32s_Pos( (Ipp32s)sMaxVal << 16 );
   }
   if(tmp > 6)
      scal = 0;
   else
      scal = (Ipp16s)(6 - tmp);

   maxL = 0;
   NcVal    = 40;
   wtmp  = (Ipp16s)(d[maxInd] >> scal);

   for(j = 40; j <121; j++) {
      resultL = (Ipp32s)wtmp * dp[maxInd - j];
      if(resultL > maxL) {
         NcVal    = (Ipp16s)(j);
         maxL = resultL;
      }
   }
   *dst2 = NcVal;
   maxL <<= 1;

   maxL = maxL>>(6-scal);
   ippsRShiftC_16s(&dp[- NcVal],3,pTmp,40);
   ippsDotProd_16s32s_Sfs(pTmp,pTmp,40, &powerL,-1 );
   if(maxL <1) {
      *dst1 = 0;
      return;
   }
   if(maxL >= powerL) {
      *dst1 = 3;
      return;
   }
   tmp = Exp_32s_Pos( powerL );
   R = (Ipp16s)( (maxL<<tmp) >> 16 );
   S = (Ipp16s)( (powerL<<tmp) >> 16 );
   for(bcVal = 0; bcVal <3; bcVal++)
      if(R <= Mul_16s_Sfs(S, DLB[bcVal],15))
         break;
   *dst1 = bcVal;
}
static void LTPParm(Ipp16s *dp,const Ipp16s *d,Ipp16s *dst1, Ipp16s *dst2){
   Ipp32s maxL, powerL, NcVal;
   Ipp16s bcVal, R, S, sMaxVal, scal, tmp, pTmp[40];

   ippsMax_16s(d,40,&sMaxVal);
   ippsMin_16s(d,40,&tmp);
   if(tmp==IPP_MIN_16S) tmp++;
   if(tmp<0) tmp =(Ipp16s)( -tmp);
   sMaxVal = (Ipp16s)(IPP_MAX(sMaxVal,tmp));
   if(sMaxVal == 0){
      tmp = scal = 0;
   } else {
      tmp = Exp_32s_Pos( (Ipp32s)sMaxVal << 16 );
   }
   if(tmp > 6)
      scal = 0;
   else
      scal = (Ipp16s)(6 - tmp);

   ippsRShiftC_16s(d,scal,pTmp,40);

   ippsCrossCorrLagMax_16s(pTmp,dp-120,40,80,&maxL,&NcVal);
   *dst2 = (Ipp16s)(-(NcVal-120));
   maxL = maxL<<1;
   maxL >>= (6-scal);
   ippsRShiftC_16s(&dp[NcVal-120],3,pTmp,40);
   ippsDotProd_16s32s_Sfs(pTmp,pTmp,40, &powerL,-1 );
   if(maxL <1) {
      *dst1 = 0;
      return;
   }
   if(maxL >= powerL) {
      *dst1 = 3;
      return;
   }
   tmp = Exp_32s_Pos( powerL );
   R = (Ipp16s)( (maxL<<tmp) >> 16 );
   S = (Ipp16s)( (powerL<<tmp) >> 16 );
   for(bcVal = 0; bcVal <3; bcVal++)
      if(R <= Mul_16s_Sfs(S, DLB[bcVal],15))
         break;
   *dst1 = bcVal;
}

static Ipp32s GsmLTP(GSMFR_LTP_t cutLTPFlag,const Ipp16s *d,Ipp16s *dp, Ipp16s *ep, Ipp16s *bcVal,
                  Ipp16s *NcVal) {
   if(!d || !ep || !dp || !NcVal || !bcVal )
       return -1;
   if( cutLTPFlag == LTP_ON)
      CutLTPParm(dp, d, bcVal, NcVal);
   else
      LTPParm(dp, d, bcVal, NcVal);
   Ltaf(dp,d, ep,swTab[*bcVal],*NcVal);
   return 0;
}
static void GsmSTAF(Ipp16s *s,Ipp16s *LARc,Ipp16s *pLARj,Ipp16s *pLARj1,Ipp16s *pMem)
{
  Ipp16s LARp[32];
   DecCodLAR( LARc, pLARj,8 );
   Interpolate_GSMFR_16s( pLARj1, pLARj, LARp , 8);
   ippsInterpolate_G729_16s( pLARj1, pLARj, LARp+8, 8);
   Interpolate_GSMFR_16s( pLARj, pLARj1, LARp+16, 8);
   ippsCopy_16s( pLARj, LARp+24 , 8);
   LARp2RC( LARp, 32);
   ippsShortTermAnalysisFilter_GSMFR_16s_I(LARp   , s     , 13, pMem );
   ippsShortTermAnalysisFilter_GSMFR_16s_I(LARp+8 , s + 13, 14, pMem );
   ippsShortTermAnalysisFilter_GSMFR_16s_I(LARp+16, s + 27, 13, pMem );
   ippsShortTermAnalysisFilter_GSMFR_16s_I(LARp+24, s + 40,120, pMem );
}

static Ipp32s GsmRPEEnc(Ipp16s *ep,Ipp16s *xMaxC,
                     Ipp16s *Mc,
                     Ipp16s *xMc) {
   Ipp16s x[40], xM[13], xMp[13], mant, exponent;

   ippsWeightingFilter_GSMFR_16s(ep, x, 40);
   RPEGrid(x,xM,Mc);
   APCMQuant2(xM,xMc,&mant,&exponent,xMaxC);
   ippsRPEQuantDecode_GSMFR_16s(xMc,FAC[mant],(Ipp16s)(6-exponent),xMp );
   ippsZero_16s(ep,40);
   RPEGridPos( xMp,*Mc,  ep);
   return 0;
}
static Ipp32s GSMFREncoder(GSMFREncoder_Obj *pObj, const Ipp16s *s,Ipp16s *LARc,Ipp16s *NcVal,
                        Ipp16s *bcVal,Ipp16s *Mc, Ipp16s *xMaxC,Ipp16s *xMc) {
   IPP_ALIGNED_ARRAY( 16, Ipp16s, so,160);
   Ipp16s *dp = pObj->dp0 + 120 ;
   Ipp32s k;

   ippsHighPassFilter_GSMFR_16s( s,pObj->sof,GSMFR_EXT_FRAME_LEN,pObj->hpMem);
   ippsPreemphasize_GSMFR_16s(pObj->sof,so, GSMFR_EXT_FRAME_LEN, &pObj->mp);
   GsmLPCAnal(pObj,so, LARc);
   GsmSTAF(so,LARc,pObj->LARpp[ pObj->idx],pObj->LARpp[pObj->idx^1],pObj->u);
   pObj->idx^=1;
   for(k = 0; k <4; k++) {
      GsmLTP(pObj->objPrm.cutLTPFlag,&so[k*40],dp,&pObj->ep[5],&bcVal[k],&NcVal[k]);
      GsmRPEEnc(&pObj->ep[5],&xMaxC[k], &Mc[k], &xMc[13*k] );
      ippsAdd_16s(&pObj->ep[5],dp,dp,40);
      dp += 40;
   }
   ippsCopy_8u((Ipp8u*)(pObj->dp0 + GSMFR_EXT_FRAME_LEN),(Ipp8u*)pObj->dp0,
               120 * sizeof(*pObj->dp0));
   return 0;
}

GSMFR_CODECFUN( APIGSMFR_Status, apiGSMFREncoder_Alloc,(Ipp32s *pSizeTab)) {
   Ipp32s allocSize = sizeof(GSMFREncoder_Obj);
  *pSizeTab      = allocSize;
   return APIGSMFR_StsNoErr;
}
static __ALIGN32 CONST Ipp16s rrvad[3]={24576,-16384,4096};
static __ALIGN32 CONST Ipp16s hann[160]={
0    ,12   ,51   ,114  ,204  ,318  ,458  ,622  ,811  ,1025 ,1262 ,1523 ,1807 ,2114 ,2444 ,2795 ,3167 ,3560 ,3972 ,4405 ,
4856 ,5325 ,5811 ,6314 ,6832 ,7365 ,7913 ,8473 ,9046 ,9631 ,10226,10831,11444,12065,12693,13326,13964,14607,15251,15898,
16545,17192,17838,18482,19122,19758,20389,21014,21631,22240,22840,23430,24009,24575,25130,25670,26196,26707,27201,27679,
28193,28581,29003,29406,29789,30151,30491,30809,31105,31377,31626,31852,32053,32230,32382,32509,32611,32688,32739,32764
, 32764, 32739, 32688, 32611, 32509, 32382, 32230, 32053, 31852, 31626, 31377, 31105, 30809,
30491, 30151, 29789, 29406, 29003, 28581, 28193, 27679, 27201, 26707, 26196, 25670, 25130,
24575, 24009, 23430, 22840, 22240, 21631, 21014, 20389, 19758, 19122, 18482, 17838, 17192,
16545, 15898, 15251, 14607, 13964, 13326, 12693, 12065, 11444, 10831, 10226, 9631, 9046,
8473, 7913, 7365, 6832, 6314, 5811, 5325, 4856, 4405, 3972, 3560, 3167, 2795, 2444, 2114,
1807, 1523, 1262, 1025, 811, 622, 458, 318, 204, 114, 51, 12, 0
};
GSMFR_CODECFUN( APIGSMFR_Status, apiGSMFREncoder_Init, (GSMFREncoder_Obj* encoderObj,
               GSMFR_Params_t *params)) {
   if(!encoderObj) return APIGSMFR_StsBadArgErr;
   ippsZero_8u((Ipp8u*)encoderObj,sizeof(GSMFREncoder_Obj));
   encoderObj->objPrm.objSize = sizeof(GSMFREncoder_Obj);
   encoderObj->vadMode = params->vadMode;
   encoderObj->vadDirection = params->vadDirection;
   encoderObj->objPrm.key = ENC_KEY;
   ippsCopy_16s(rrvad,encoderObj->rvad,3);
   encoderObj->nextFrameVadMode = params->vadMode;
   encoderObj->nextFrameVadDirection = params->vadDirection;
   encoderObj->objPrm.cutLTPFlag = params->ltp_f;
   encoderObj->normrvad=7;
   encoderObj->e_thvad=20;
   encoderObj->m_thvad=31250;
   encoderObj->hangcount=-1;
   encoderObj->oldlag=40;
   encoderObj->vad=1;
   encoderObj->vvad=1;
   encoderObj->myburstcount =  DTX_ELAPSED_FRAMES;
   return APIGSMFR_StsNoErr;
}
GSMFR_CODECFUN( APIGSMFR_Status, apiGSMFREncoder_Mode, (GSMFREncoder_Obj* encoderObj,
               GSMFR_Params_t *params)) {
   if(!encoderObj) return APIGSMFR_StsBadArgErr;
   encoderObj->vadMode = params->vadMode;
   encoderObj->vadDirection = params->vadDirection;
   return APIGSMFR_StsNoErr;
}
static void newSIDFrame(Ipp16s *saveLAR,Ipp16s *saveXMax,uchar pDst[33])
{
  Ipp16s xMaxAvg, LAR[8];
   larAverage(saveLAR,LAR,8);
   xmaxAverage(saveXMax,&xMaxAvg);
   ippsSet_8u(0,pDst,33);
   /* pack to bitstream */
   pDst[0]  =(uchar)( ((LAR[0] >> 2) & 15));
   pDst[1]  =(uchar)( ((LAR[0] & 3) << 6) | (LAR[1] & 0x3F));
   pDst[2]  =(uchar)( ((LAR[2] & 0x1F) << 3)| ((LAR[3] >> 2) & 7));
   pDst[3]  =(uchar)( ((LAR[3] & 3) << 6) | ((LAR[4] & 15) << 2) | ((LAR[5] >> 2) & 3));
   pDst[4]  =(uchar)( ((LAR[5] & 3) << 6) | ((LAR[6] & 7) << 3) | (LAR[7] &7));
   pDst[6]  =(uchar)( ((xMaxAvg >> 1) & 0x1F));
   pDst[7]  =(uchar)( ((xMaxAvg & 1) << 7));
   pDst[13] =(uchar)( ((xMaxAvg >> 1) & 0x1F));
   pDst[14] =(uchar)( ((xMaxAvg & 1) << 7));
   pDst[20] =(uchar)( ((xMaxAvg >> 1) & 0x1F));
   pDst[21] =(uchar)( ((xMaxAvg & 1) << 7));
   pDst[27] =(uchar)( ((xMaxAvg >> 1) & 0x1F));
   pDst[28] =(uchar)( ((xMaxAvg & 1) << 7));
}

/* return 0 if SID frame ,1 if voice
 SID (after hangover) recalculated with every new vad=0 frame
 last update period SID = 23
 vad=0 frame considered as speech in myburstcount
 toneDetect() have to go after threshAdapt()
*/
static Ipp32s Dtx_Encoder_Count(GSMFREncoder_Obj *encoderObj,Ipp16s LAR[8],Ipp16s xmax[4],uchar *dst)
{
   if ( encoderObj->vad == 1)
   {
      ++encoderObj->myburstcount;
      if(encoderObj->SIDAveragePeriodCount>0 && encoderObj->SIDAveragePeriodCount<2)
      {
      ++encoderObj->myburstcount;
      }
      encoderObj->SIDAveragePeriodCount=0;
      if ( encoderObj->myburstcount >= DTX_ELAPSED_FRAMES )
      {
         encoderObj->myburstcount = DTX_ELAPSED_FRAMES;
      }
   }
   else
   {
      ++encoderObj->SIDAveragePeriodCount;
      if ( encoderObj->myburstcount >= DTX_ELAPSED_FRAMES &&
         encoderObj->SIDAveragePeriodCount < DTX_HANG_CONST)
      {
         saveParm(LAR ,encoderObj->saveLAR,&encoderObj->oldestLAR,8);    /* save history for SID forming */
         saveParm(xmax ,encoderObj->saveXMax,&encoderObj->oldestxmax,4);    /* save history for SID forming */
         return 1;
      }
      if ( encoderObj->SIDAveragePeriodCount >= DTX_HANG_CONST )
      {/*form new SID */
         encoderObj->myburstcount = 0;
         newSIDFrame(encoderObj->saveLAR,encoderObj->saveXMax,dst);
         saveParm(LAR ,encoderObj->saveLAR,&encoderObj->oldestLAR,8);    /* save history for SID forming */
         saveParm(xmax ,encoderObj->saveXMax,&encoderObj->oldestxmax,4);    /* save history for SID forming */
         ippsCopy_8u(dst,encoderObj->oldSID,33);
         encoderObj->SIDAveragePeriodCount = DTX_HANG_CONST;
         return 0;
      }else
      if ( encoderObj->myburstcount < (DTX_ELAPSED_FRAMES+1))
      {
            ippsCopy_8u(encoderObj->oldSID,dst,33);
            saveParm(LAR ,encoderObj->saveLAR,&encoderObj->oldestLAR,8);    /* save history for SID forming */
            saveParm(xmax ,encoderObj->saveXMax,&encoderObj->oldestxmax,4);    /* save history for SID forming */
            return 0;
      }
      saveParm(LAR ,encoderObj->saveLAR,&encoderObj->oldestLAR,8);    /* save history for SID forming */
      saveParm(xmax ,encoderObj->saveXMax,&encoderObj->oldestxmax,4);    /* save history for SID forming */
   }
   return 1;
}
GSMFR_CODECFUN( APIGSMFR_Status, apiGSMFREncode,(GSMFREncoder_Obj *encoderObj,
               const Ipp16s *source, uchar *wBuf,Ipp32s *vad,Ipp32s *frameType)) {
   Ipp16s LAR[8], NcVal[4], Mc[4], bcVal[4], xmax[4], xMc[13*4],vpar[9];
   Ipp32s   L_work[9];
   if(NULL==encoderObj || NULL==source || NULL ==wBuf )
      return APIGSMFR_StsBadArgErr;
   if(0 >= encoderObj->objPrm.objSize)
      return APIGSMFR_StsNotInitialized;
   encoderObj->vadMode = encoderObj->nextFrameVadMode;
   encoderObj->vadDirection = encoderObj->nextFrameVadDirection;

   ippsZero_16s(vpar,9);
   if(GSMFREncoder(encoderObj, source, LAR, NcVal, bcVal, Mc, xmax, xMc))
      return APIGSMFR_StsBadArgErr;
   *frameType=1;
   if(encoderObj->vadMode == GSMFR_VAD_ON ){
      /*3.3.1 clause, Schur recursion to compute reflection coefficients*/
      ippsSchur_GSMFR_32s16s(encoderObj->L_av1,&vpar[1],8);
      predValComp(vpar,L_work,9); /* do rav1 for threshAdapt() & spectComp()*/
      ippsLShiftC_32s(L_work,1,L_work,9);
      /*Keep the normrav1 for use in subclause 3.4 and 3.6.*/
      if ( L_work[0] == 0 )
         encoderObj->normrav1 =0;
      else
         encoderObj->normrav1 = Exp_32s_Pos( L_work[0] );
      Shift_32s16s(L_work,encoderObj->rav1,9,16-encoderObj->normrav1);

      spectComp(encoderObj);   /* do stat for threshAdapt()*/
      periodDetect(encoderObj);/* do ptch for threshAdapt()*/
      periodUpdate(encoderObj,NcVal);
      threshAdapt(encoderObj); /* do rvad for next call acfEnComp(L_ACF) & thvad for vadDecision()*/
      vadDecision(encoderObj); /* do vvad for vadHangoverAdd(vad) */
      vadHangoverAdd(encoderObj);
      if(encoderObj->vadDirection == GSMFR_DOWNLINK)
         toneDetect(encoderObj);/* do tone for threshAdapt()*/
      /* sid frame encoding */
      *vad      = encoderObj->vad;
      *frameType= Dtx_Encoder_Count(encoderObj,LAR,xmax,wBuf);
   }
   if(*frameType)
   {
      /*pack to bitstream*/
      wBuf[0]  =(uchar)( ((LAR[0] >> 2) & 0xF));
      wBuf[8]  =(uchar)( ((xMc[2] & 0x3) << 6) | ((xMc[3] & 0x7) << 3) | (xMc[4] & 0x7));
      wBuf[1]  =(uchar)( ((LAR[0] & 0x3) << 6) | (LAR[1] & 0x3F));
      wBuf[9]  =(uchar)( ((xMc[5] & 0x7) << 5) | ((xMc[6] & 0x7) << 2) | ((xMc[7] >> 1) & 0x3));
      wBuf[2]  =(uchar)( ((LAR[2] & 0x1F) << 3)| ((LAR[3] >> 2) & 0x7));
      wBuf[10] =(uchar)( ((xMc[7] & 0x1) << 7) | ((xMc[8] & 0x7) << 4) | ((xMc[9] & 0x7) << 1) | ((xMc[10] >> 2) & 0x1));
      wBuf[3]  =(uchar)( ((LAR[3] & 0x3) << 6) | ((LAR[4] & 0xF) << 2) | ((LAR[5] >> 2) & 0x3));
      wBuf[11] =(uchar)( ((xMc[10] & 0x3) << 6)| ((xMc[11] & 0x7) << 3)| (xMc[12] & 0x7));
      wBuf[4]  =(uchar)( ((LAR[5] & 0x3) << 6) | ((LAR[6] & 0x7) << 3) | (LAR[7] & 0x7));

      wBuf[15] =(uchar)( ((xMc[15] & 0x3) << 6)| ((xMc[16] & 0x7) << 3)| (xMc[17] & 0x7));
      wBuf[5]  =(uchar)( ((NcVal[0] & 0x7F) << 1) | ((bcVal[0] >> 1) & 0x1));
      wBuf[16] =(uchar)( ((xMc[18] & 0x7) << 5)| ((xMc[19] & 0x7) << 2)| ((xMc[20] >> 1) & 0x3));
      wBuf[12] =(uchar)( ((NcVal[1] & 0x7F) << 1)  | ((bcVal[1] >> 1) & 0x1));
      wBuf[17] =(uchar)( ((xMc[20] & 0x1) << 7)| ((xMc[21] & 0x7) << 4)| ((xMc[22] & 0x7) << 1) | ((xMc[23] >> 2) & 0x1));
      wBuf[19] =(uchar)( ((NcVal[2] & 0x7F) << 1)  | ((bcVal[2] >> 1) & 0x1));
      wBuf[18] =(uchar)( ((xMc[23] & 0x3) << 6)| ((xMc[24] & 0x7) << 3)| (xMc[25] & 0x7));
      wBuf[26] =(uchar)( ((NcVal[3] & 0x7F) << 1)  | ((bcVal[3] >> 1) & 0x1));

      wBuf[22] =(uchar)( ((xMc[28] & 0x3) << 6)| ((xMc[29] & 0x7) << 3)| (xMc[30] & 0x7));
      wBuf[6]  =(uchar)( ((bcVal[0] & 0x1) << 7)   | ((Mc[0] & 0x3) << 5)   | ((xmax[0] >> 1) & 0x1F));
      wBuf[23] =(uchar)( ((xMc[31] & 0x7) << 5)| ((xMc[32] & 0x7) << 2)| ((xMc[33] >> 1) & 0x3));
      wBuf[13] =(uchar)( ((bcVal[1] & 0x1) << 7)   | ((Mc[1] & 0x3) << 5)   | ((xmax[1] >> 1) & 0x1F));
      wBuf[24] =(uchar)( ((xMc[33] & 0x1) << 7)| ((xMc[34] & 0x7) << 4)| ((xMc[35] & 0x7) << 1)| ((xMc[36] >> 2) & 0x1));
      wBuf[20] =(uchar)( ((bcVal[2] & 0x1) << 7)   | ((Mc[2] & 0x3) << 5)   | ((xmax[2] >> 1) & 0x1F));
      wBuf[25] =(uchar)( ((xMc[36] & 0x3) << 6)| ((xMc[37] & 0x7) << 3)| (xMc[38] & 0x7));
      wBuf[27] =(uchar)( ((bcVal[3] & 0x1) << 7)   | ((Mc[3] & 0x3) << 5)   | ((xmax[3] >> 1) & 0x1F));

      wBuf[29] =(uchar)( ((xMc[41] & 0x3) << 6)| ((xMc[42] & 0x7) << 3)| (xMc[43] & 0x7));
      wBuf[7]  =(uchar)( ((xmax[0] & 0x1) << 7)| ((xMc[0] & 0x7) << 4) | ((xMc[1] & 0x7) << 1) | ((xMc[2] >> 2) & 0x1));
      wBuf[30] =(uchar)( ((xMc[44] & 0x7) << 5)| ((xMc[45] & 0x7) << 2)| ((xMc[46] >> 1) & 0x3));
      wBuf[14] =(uchar)( ((xmax[1] & 0x1) << 7)| ((xMc[13] & 0x7) << 4)| ((xMc[14] & 0x7) << 1) | ((xMc[15] >> 2) & 0x1));
      wBuf[31] =(uchar)( ((xMc[46] & 0x1) << 7)| ((xMc[47] & 0x7) << 4)| ((xMc[48] & 0x7) << 1)| ((xMc[49] >> 2) & 0x1));
      wBuf[21] =(uchar)( ((xmax[2] & 0x1) << 7)| ((xMc[26] & 0x7) << 4)| ((xMc[27] & 0x7) << 1) | ((xMc[28] >> 2) & 0x1));
      wBuf[32] =(uchar)( ((xMc[49] & 0x3) << 6)| ((xMc[50] & 0x7) << 3)| (xMc[51] & 0x7));
      wBuf[28] =(uchar)( ((xmax[3] & 0x1) << 7)| ((xMc[39] & 0x7) << 4)| ((xMc[40] & 0x7) << 1) | ((xMc[41] >> 2) & 0x1));
   }
   return APIGSMFR_StsNoErr;
}
static void saveParm(Ipp16s *pSrc,Ipp16s *pDst,Ipp32s *index,Ipp32s len)
{
   ippsCopy_16s(pSrc,&pDst[(*index)*len],len);
      ++*index;
      if(*index>3)
         *index=0;
}
/* LAR averaging */
static void larAverage(Ipp16s *saveLAR,Ipp16s *LAR,Ipp32s len)
{
 Ipp32s i,
     TmpL;
   for( i=0;i<len;++i)
   {
      TmpL = 2;
      TmpL += saveLAR[i];
      TmpL += saveLAR[i+8];
      TmpL += saveLAR[i+16];
      TmpL += saveLAR[i+24];

      TmpL = TmpL >> 2;
      LAR[i] = (Ipp16s)TmpL;
    }
}

static void xmaxAverage(Ipp16s *saveXMax,Ipp16s *xmax)
{
  Ipp32s  sum;
   ippsSum_16s32s_Sfs(saveXMax, 16,&sum,0);
   *xmax = (Ipp16s)((sum+8) >> 4);
}

static void acfEnComp(GSMFREncoder_Obj *encoderObj,const Ipp32s *L_ACF){
  Ipp16s normacf,
        normprod;
  Ipp16s sacf[9];
  Ipp32s   i,L_tmp;
    if ( encoderObj->scalauto < 0 )
      encoderObj->scalvad = 0;
   else
      encoderObj->scalvad = (Ipp16s)encoderObj->scalauto;  /* keep scalvad for( use in subclause 3.2 */

   if ( L_ACF[0] == 0 )
   {
      encoderObj->e_pvad = -32768;
      encoderObj->m_pvad = 0;
      encoderObj->e_acf0 = -32768;
      encoderObj->m_acf0 = 0;
      return;
   }

   normacf = Exp_32s_Pos( L_ACF[0] );

   Shift_32s16s(L_ACF,sacf,9,19-normacf);
   encoderObj->e_acf0 = (Ipp16s)(32+(Ipp16s)(encoderObj->scalvad << 1 ));
   encoderObj->e_acf0 =(Ipp16s)( encoderObj->e_acf0-normacf);
   encoderObj->m_acf0 = (Ipp16s)(sacf[0] << 3);

   encoderObj->e_pvad = (Ipp16s)(encoderObj->e_acf0+14);
   encoderObj->e_pvad = (Ipp16s)(encoderObj->e_pvad -encoderObj->normrvad);
   L_tmp = 0;
   for( i = 1;i<9;++i){
      L_tmp +=  2*sacf[i]*encoderObj->rvad[i];
   }
   L_tmp += sacf[0]*encoderObj->rvad[0];
   if ( L_tmp <1 )
      L_tmp = 1;
   normprod = Exp_32s_Pos( L_tmp );
   encoderObj->e_pvad =(Ipp16s)(encoderObj->e_pvad- normprod);
   encoderObj->m_pvad = (Ipp16s)((( L_tmp<< normprod )) >> 16);
}
static void acfEverag(GSMFREncoder_Obj *encoderObj,const Ipp32s *L_ACF)
{
  Ipp16s scal;
  Ipp32s i,L_tmp;
   scal = (Ipp16s)(10 - (Ipp16s)(encoderObj->scalvad << 1));
   for(i = 0;i<9;++i)
   {
      L_tmp = L_ACF[i] >> scal;
      encoderObj->L_av0[i] = encoderObj->L_sacf[i] + L_tmp;
      encoderObj->L_av0[i] = encoderObj->L_sacf[i+9] + encoderObj->L_av0[i];
      encoderObj->L_av0[i] = encoderObj->L_sacf[i+18] + encoderObj->L_av0[i];
      encoderObj->L_sacf[ encoderObj->pt_sacf + i ] = L_tmp;
      encoderObj->L_av1[i] = encoderObj->L_sav0[ encoderObj->pt_sav0 + i ];
      encoderObj->L_sav0[ encoderObj->pt_sav0 + i] = encoderObj->L_av0[i];
   }

   if ( encoderObj->pt_sacf == 18 )
      encoderObj->pt_sacf = 0;
   else
      encoderObj->pt_sacf =  (Ipp16s)(encoderObj->pt_sacf+9);

   if ( encoderObj->pt_sav0 == 27 )
      encoderObj->pt_sav0 = 0;
   else
      encoderObj->pt_sav0 += 9;
}
static void spectComp(GSMFREncoder_Obj *encoderObj){
  Ipp32s   i,L_SUMp,L_tmp,L_dm;
  Ipp16s tmp,divshift,shift,sav0[9];
   if ( encoderObj->L_av0[0] == 0 ){
      for( i = 0 ;i< 9;++i){
         sav0[i] = 4095;
      }
   }else{
      shift = Exp_32s_Pos( encoderObj->L_av0[0] );
      Shift_32s16s(encoderObj->L_av0,sav0,9,19-shift);
   }
   L_SUMp = 0;
   for( i = 1 ;i< 9;++i){
      L_SUMp += 2* encoderObj->rav1[i]* sav0[i];
   }

   if ( L_SUMp < 0 )
      L_tmp = -L_SUMp;
   else
      L_tmp = L_SUMp;

   if ( L_tmp == 0 ){
      L_dm  = 0;
      shift = 0;
   }
   else
   {
      sav0[0] = (Ipp16s)(sav0[0]<< 3);
      shift = Exp_32s_Pos( L_tmp );
      tmp  = (Ipp16s)((L_tmp<< shift )>>16);
      if ( sav0[0] >= tmp ){
         divshift = 0;
         tmp = (Ipp16s)(((Ipp32s)tmp<<15)/ sav0[0]);
      }
      else
      {
         divshift = 1;
         tmp =(Ipp16s)(tmp - sav0[0]);
         tmp = (Ipp16s)(((Ipp32s)tmp<<15)/sav0[0]);
      }
      if( divshift == 1 )
         L_dm = 32768;
      else
         L_dm = 0;

      L_dm = ( L_dm + tmp)<<1;
      if( L_SUMp < 0 )
         L_dm = -L_dm;
   }

   L_dm = L_dm<<14;
   L_dm = L_dm>> shift;
   L_dm +=  encoderObj->rav1[0] << 11;
   L_dm = L_dm>> encoderObj->normrav1;

   L_tmp   = L_dm - encoderObj->L_lastdm;
   encoderObj->L_lastdm = L_dm;

   if ( L_tmp < 0 )
      L_tmp = -L_tmp;

   L_tmp -= 3277;


   if ( L_tmp < 0 )
      encoderObj->stat = 1;
   else
      encoderObj->stat = 0;
}

static void periodDetect(GSMFREncoder_Obj *encoderObj)
{
  Ipp32s   tmp;
   tmp = encoderObj->oldlagcount+encoderObj->veryoldlagcount;

   if ( tmp >= 4 )
      encoderObj->ptch = 1;
   else
      encoderObj->ptch = 0;
}
static void threshAdapt(GSMFREncoder_Obj *encoderObj){
  Ipp16s E_PTH=19,M_PTH=18750,
         E_MARGIN=27,M_MARGIN=19531,
         E_PLEV=20,M_PLEV=25000,
         e_tmp;
  Ipp32s comp, L_tmp, m_tmp,comp2,tmp;
   comp=0;
   if ( ( encoderObj->e_acf0 < E_PTH )||
       ((encoderObj->e_acf0 == E_PTH)&& ( encoderObj->m_acf0 < M_PTH ))){
           comp=1;
       }
   if ( comp == 1 ){
      encoderObj->e_thvad = E_PLEV;
      encoderObj->m_thvad = M_PLEV;
      return;
   }

   comp = 0;
   if ( encoderObj->ptch == 1 )  comp = 1;
   if ( encoderObj->stat == 0 )  comp = 1;
   if ( encoderObj->tone == 1 )  comp = 1;
   if ( comp == 1 )
   {
      encoderObj->adaptcount = 0;
      return;
   }

   encoderObj->adaptcount++;
   if ( encoderObj->adaptcount < 9 ){
       return;
   }

   encoderObj->m_thvad = (Ipp16s)(encoderObj->m_thvad - (Ipp16s)(encoderObj->m_thvad >> 5 ));
   if ( encoderObj->m_thvad < 16384)
   {
      encoderObj->m_thvad = (Ipp16s)(encoderObj->m_thvad<<1);
      --encoderObj->e_thvad;
   }

   L_tmp =  encoderObj->m_pvad + encoderObj->m_pvad;
   L_tmp += encoderObj->m_pvad;
   L_tmp = L_tmp>>1;
   e_tmp = (Ipp16s)(encoderObj->e_pvad + 1);
   if ( L_tmp > 32767 )
   {
      L_tmp = L_tmp>> 1;
      e_tmp += 1;
   }
   m_tmp = L_tmp;

   comp = 0;
   if ( (encoderObj->e_thvad < e_tmp) ||
      ((encoderObj->e_thvad == e_tmp)&&
      (encoderObj->m_thvad < m_tmp))){
          comp =1;
      }


   if ( comp == 1 ){
      L_tmp = encoderObj->m_thvad + (encoderObj->m_thvad>> 4);
      if ( L_tmp > 32767 )
      {
         encoderObj->m_thvad = (Ipp16s)(L_tmp >> 1);
         encoderObj->e_thvad += 1;
      }
      else
         encoderObj->m_thvad =(Ipp16s)(L_tmp);
      comp2 = 0;
      if ( (e_tmp < encoderObj->e_thvad) ||
        ((e_tmp == encoderObj->e_thvad)&&
        (m_tmp<encoderObj->m_thvad))){
            comp2 = 1;
        }
      if ( comp2 == 1 ){
         encoderObj->e_thvad = e_tmp;
         encoderObj->m_thvad = (Ipp16s)m_tmp;
      }
   }

   if ( encoderObj->e_pvad == E_MARGIN ){
      L_tmp = encoderObj->m_pvad + M_MARGIN;
      m_tmp = L_tmp >> 1;
      e_tmp = (Ipp16s)(encoderObj->e_pvad + 1);
   }
   else
   {
      if ( encoderObj->e_pvad > E_MARGIN )
      {
         tmp = encoderObj->e_pvad - E_MARGIN;
         tmp = M_MARGIN >> tmp;
         L_tmp = encoderObj->m_pvad + tmp;
         if ( L_tmp > 32767)
         {
            e_tmp = (Ipp16s)(encoderObj->e_pvad + 1);
            m_tmp = L_tmp >> 1;
         }
         else
         {
            e_tmp = encoderObj->e_pvad;
            m_tmp = L_tmp;
         }
      }
      else
      {
         tmp = E_MARGIN - encoderObj->e_pvad;
         tmp = encoderObj->m_pvad >> tmp;
         L_tmp = M_MARGIN + tmp;
         if (L_tmp > 32767)
         {
            e_tmp = (Ipp16s)(E_MARGIN + 1);
            m_tmp = L_tmp >> 1;
         }
         else
         {
            e_tmp = E_MARGIN;
            m_tmp = L_tmp;
         }
      }
   }

   comp = 0;
   if ( encoderObj->e_thvad > e_tmp)     comp = 1;
   if (encoderObj->e_thvad == e_tmp)
      if (encoderObj->m_thvad > m_tmp)   comp =1;

   if ( comp == 1 ){
      encoderObj->e_thvad = e_tmp;
      encoderObj->m_thvad = (Ipp16s)m_tmp;
   }

   encoderObj->normrvad  = encoderObj->normrav1;
   ippsCopy_16s(encoderObj->rav1,encoderObj->rvad,9);
   encoderObj->adaptcount = 9;
}

static void vadDecision(GSMFREncoder_Obj *encoderObj){
   encoderObj->vvad = 0;
   if (encoderObj->e_pvad >  encoderObj->e_thvad){
      encoderObj->vvad = 1;
   }
   if (encoderObj->e_pvad == encoderObj->e_thvad)
      if (encoderObj->m_pvad > encoderObj->m_thvad)
         encoderObj->vvad =1;
}

static void vadHangoverAdd(GSMFREncoder_Obj *encoderObj){
   if ( encoderObj->vvad == 1 )
      encoderObj->burstcount++;
   else{
      encoderObj->burstcount = 0;
   }
   if ( encoderObj->burstcount >= 3 ){
      encoderObj->hangcount =  5;
      encoderObj->burstcount = 3;
   }
   encoderObj->vad = encoderObj->vvad;
   if ( encoderObj->hangcount >= 0 ){
      encoderObj->vad = 1;
      --encoderObj->hangcount;
   }
}

static void periodUpdate(GSMFREncoder_Obj *encoderObj,Ipp16s lags[4])
{
  Ipp32s i,j;
  Ipp16s minlag,maxlag,smallag,tmp;
   encoderObj->lagcount = 0;

   for( i = 0;i<4;++i)
   {
      if ( encoderObj->oldlag > lags[i] )
      {
         minlag = lags[i];
         maxlag = encoderObj->oldlag;
      }
      else
      {
         minlag = encoderObj->oldlag;
         maxlag = lags[i] ;
      }
      smallag = maxlag;
      for( j = 0;j<3;++j)
      {
         if (smallag >= minlag)
            smallag = (Ipp16s)(smallag -minlag);
      }
      tmp = (Ipp16s)(minlag - smallag);
      if ( tmp < smallag )
         smallag = tmp;
      if ( smallag < 2 )
         encoderObj->lagcount++;
      encoderObj->oldlag = lags[i];
   }

   encoderObj->veryoldlagcount = encoderObj->oldlagcount;
   encoderObj->oldlagcount     = encoderObj->lagcount;
}
static Ipp32s toneDetect(GSMFREncoder_Obj *encoderObj)
{
  Ipp32s L_acfh[5],autoScale;
  Ipp32s i, L_tmp,L_den,L_num;
  Ipp16s sofh[160],a[3],rc[5] ,tmp,prederr ;
   ippsMul_NR_16s_Sfs( encoderObj->sof,hann,sofh,160,15);
   AutoCorr1(sofh,160,L_acfh, 5,&autoScale);
   ippsLShiftC_32s(L_acfh,1,L_acfh,5);
   ippsSchur_GSMFR_32s16s(L_acfh,&rc[1],4 );
   tmp = (Ipp16s)(rc[1] >> 2);
   a[1] = (Ipp16s)(tmp + (Ipp16s)(((Ipp32s)rc[2]* (Ipp32s)tmp + 16384)>>15));
   a[2] = (Ipp16s)(rc[2] >> 2);

   L_den = 2*a[1]*a[1];

   L_tmp = a[2]<< 16;
   L_num = L_tmp - L_den;
   if ( L_num <1 )
   {
      encoderObj->tone = 0;
      return 0;
   }

   if ( a[1] < 0)
   {
      tmp = (Ipp16s)(L_den >> 16);
      L_den = tmp*6378;
      L_tmp = L_num - L_den;
      if ( L_tmp < 0 )
      {
         encoderObj->tone = 0;
         return 0;
      }
   }
   prederr = 32767;
   for( i=1;i<5;++i)
   {
      tmp = Mul_16s_Sfs ( rc[i], rc[i],15 );
      tmp = (Ipp16s)(32767 - tmp);
      prederr = Mul_16s_Sfs( prederr, tmp,15 );
   }
   tmp = (Ipp16s)(prederr - 1464);
   if ( tmp < 0 )
      encoderObj->tone = 1;
   else
      encoderObj->tone = 0;
   return 0;
}
