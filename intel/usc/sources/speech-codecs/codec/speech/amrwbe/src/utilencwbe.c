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
// Purpose: AMRWBE speech codec: encoder utilities.
//
*/

#include "ownamrwbe.h"

#define TWINLENSHORT (4)
#define COMPLEN2 12
#define CS 0

#define ACELP_MODE   0
#define TCX_MODE     1
#define TCX_OR_ACELP 2

#define QS        32
#define QAVE      2048
#define QLPH      64
#define QLPH_2_5  160

#define LOWLIMIT 8
#define LOWOMIT 1
#define DFTN 64
#define DFTNx2 128
#define LPC_N 16


static __ALIGN(16) CONST Ipp16s tblBandFreqs_16s[COMPLEN2] = {
   0x00C8, 0x0190, 0x0258, 0x0320, 0x04B0, 0x0640, 0x07D0, 0x0960,
   0x0C80, 0x0FA0, 0x12C0, 0x1900};

static __ALIGN(16) CONST Ipp16s tblClssWeight_16s[LPHAVELEN] = {
   0x199A, 0x1666, 0x1333, 0x1333, 0x1000, 0x1000, 0x0666, 0x0333};

void InitClassifyExcitation(NCLASSDATA_FX *stClass)
{
   Ipp32s i,j;

   stClass->prevModes[0] = ACELP_MODE;
   stClass->prevModes[1] = ACELP_MODE;
   stClass->prevModes[2] = ACELP_MODE;
   stClass->prevModes[3] = ACELP_MODE;

   stClass->vadFlag[0] = 1;
   stClass->vadFlag[1] = 1;
   stClass->vadFlag[2] = 1;
   stClass->vadFlag[3] = 1;
   stClass->vadFlag_old[0] = 0;
   stClass->vadFlag_old[1] = 0;
   stClass->vadFlag_old[2] = 0;
   stClass->vadFlag_old[3] = 0;
   for(i=0; i<10; i++) {
      stClass->LTPGain[i]= 0;
      stClass->LTPLag[i]= 0;
      stClass->NormCorr[i]= 0;
   }
   stClass->TotalEnergy[0]= 0;
   stClass->TotalEnergy[1]= 0;
   stClass->TotalEnergy[2]= 0;
   stClass->TotalEnergy[3]= 0;
   stClass->TotalEnergy[4]= 0;
   stClass->NoMtcx[0]= 0;
   stClass->NoMtcx[1]= 0;
   stClass->NbOfAcelps = 0;
   stClass->lph[0] = 0;
   stClass->lph[1] = 0;
   stClass->lph[2] = 0;
   stClass->lph[3] = 0;
   for (j=0; j<4*M; j++) {
      stClass->ApBuf[j] = 0;
   }
   for(j=0; j<TWINLEN; j++) {
      for(i=0;i<COMPLEN;i++) {
         stClass->levelHist[j][i] = 0;
      }
   }
   for(i=0; i<COMPLEN; i++) {
      stClass->averageHistTime[i] = (Ipp16s)QS;
      stClass->stdDevHistTime[i] = (Ipp16s)QAVE;
      stClass->averageHistTimeShort[i] = (Ipp16s)QS;
      stClass->stdDevHistTimeShort[i] = (Ipp16s)QAVE;
   }
   for(i=0; i<LPHLEN; i++)
      stClass->lphBuf[i] = (Ipp16s)(6*QLPH);
   for(i=0; i<LPHAVELEN; i++)
      stClass->lphAveBuf[i] = (Ipp16s)(6*QLPH);
   stClass->StatClassCount = 0;
}


Ipp16s ClassifyExcitation(
    NCLASSDATA_FX *stClass,
    const Ipp16s* pLevelNew,
    Ipp32s sfIndex
)
{

   Ipp32s levH , levL, Ltmp1;
   Ipp16s tmp1, stdAve, stdAveShort, lph, exp_tmp, m_tmp, exp_hist, m_hist;
   Ipp16s exp_tmp1, m_tmp1, tmp16;
   Ipp32s i, j;
   Ipp16s headMode;
   Ipp16s NormalizedBands[COMPLEN2], bckr_est[COMPLEN2];
   Ipp16s AverFreqFiltBand, FiltBand_Thres,levRatio;
   Ipp32s Ltmp;

   levH=0; levL = 0;

   ippsZero_16s(bckr_est,COMPLEN2);
   FiltBand_Thres = 2000;
   for(i=CS;i<COMPLEN2;i++) {
      if(stClass->vadFlag[sfIndex]>0) {
         stClass->levelHist[0][i] = pLevelNew[i];
      }
   }

   if(stClass->vadFlag[sfIndex] != 0) {
      levH=0; levL = 0;

      for(i=CS; i<COMPLEN2; i++) {
         tmp16 = Sub_16s(stClass->averageHistTime[i], ShiftR_NR_16s(stClass->levelHist[TWINLEN - 1][i],4));
         stClass->averageHistTime[i] = Add_16s(tmp16, ShiftR_NR_16s(stClass->levelHist[0][i],4));

         tmp16 = Sub_16s(stClass->averageHistTimeShort[i], ShiftR_NR_16s(stClass->levelHist[TWINLENSHORT - 1][i],2));
         stClass->averageHistTimeShort[i] = Add_16s(tmp16, ShiftR_NR_16s(stClass->levelHist[0][i] ,2));

         Ltmp = 0;
         for(j=0; j<TWINLEN; j++) {
            tmp16 = Sub_16s(stClass->averageHistTime[i], stClass->levelHist[j][i]);
            Ltmp = Add_32s(Ltmp, Mul2_Low_32s(tmp16*tmp16));
         }
         exp_tmp = Norm_32s_I(&Ltmp);
         exp_tmp = (Ipp16s)(36 - exp_tmp);
         InvSqrt_32s16s_I(&Ltmp, &exp_tmp);
         m_tmp = (Ipp16s)(Ltmp>>16);

         Ltmp = Mul2_Low_32s(stClass->averageHistTime[i]*m_tmp);
         exp_hist = Norm_32s_I(&Ltmp);
         m_hist = (Ipp16s)(Ltmp>>16);
         exp_hist = (Ipp16s)(20 - exp_hist + exp_tmp);

         m_hist = Div_16s(16384, m_hist);
         exp_hist = (Ipp16s)(1 - exp_hist);
         if (4 >= exp_hist) {
            stClass->stdDevHistTime[i] = (Ipp16s)(m_hist >> (4-exp_hist));
         } else {
            stClass->stdDevHistTime[i] = ShiftL_16s(m_hist, (Ipp16u)(exp_hist-4));
         }

         Ltmp = 0;
         for(j=0; j<TWINLENSHORT; j++) {
            tmp16 = Sub_16s(stClass->averageHistTimeShort[i], stClass->levelHist[j][i]);
            Ltmp = Add_32s(Ltmp, Mul2_Low_32s(tmp16*tmp16));
         }

         exp_tmp = Norm_32s_I(&Ltmp);
         exp_tmp = (Ipp16s)(38 - exp_tmp);
         InvSqrt_32s16s_I(&Ltmp, &exp_tmp);
         m_tmp = (Ipp16s)(Ltmp>>16);

         Ltmp = Mul2_Low_32s(stClass->averageHistTimeShort[i]*m_tmp);
         exp_hist = Norm_32s_I(&Ltmp);
         m_hist = (Ipp16s)(Ltmp >> 16);
         exp_hist = (Ipp16s)(20 - exp_hist + exp_tmp);

         m_hist = Div_16s(16384, m_hist);
         exp_hist = (Ipp16s)(1 - exp_hist);
         if (4 >= exp_hist) {
            stClass->stdDevHistTimeShort[i] = (Ipp16s)(m_hist >> (4-exp_hist));
         } else {
            stClass->stdDevHistTimeShort[i] = ShiftL_16s(m_hist, (Ipp16u)(exp_hist-4));
         }

         if(i < LOWOMIT) {
            ;
         } else if(i < LOWLIMIT) {
            levL = Add_32s(levL, stClass->levelHist[0][i]);
         } else {
            levH = Add_32s(levH, stClass->levelHist[0][i]);
         }
      }
   }

   stClass->TotalEnergy[4] = stClass->TotalEnergy[3];
   stClass->TotalEnergy[3] = stClass->TotalEnergy[2];
   stClass->TotalEnergy[2] = stClass->TotalEnergy[1];
   stClass->TotalEnergy[1] = stClass->TotalEnergy[0];
   stClass->TotalEnergy[0] = 0;

   for(i=CS; i<COMPLEN2; i++) {
      if (pLevelNew[i] < bckr_est[i]) {
         bckr_est[i] = pLevelNew[i];
      }
      stClass->TotalEnergy[0] = Add_32s(stClass->TotalEnergy[0], Sub_16s(pLevelNew[i],bckr_est[i]));
   }

   for(i=CS; i<COMPLEN2; i++) {
      tmp16 = Sub_16s(pLevelNew[i],bckr_est[i]);
      exp_tmp = (Ipp16s)(Exp_16s(tmp16) - 1);
      if (exp_tmp > 0) {
         m_tmp = ShiftL_16s(tmp16, exp_tmp);
      } else {
         m_tmp = (Ipp16s)(tmp16 >> (-exp_tmp));
      }
      exp_tmp = (Ipp16s)(15 - exp_tmp);

      if(stClass->TotalEnergy[0]== 0) {
         stClass->TotalEnergy[0] = 1;
      }
      Ltmp = stClass->TotalEnergy[0];
      exp_tmp1 = (Ipp16s)(31 - Norm_32s_I(&Ltmp));
      m_tmp1 = (Ipp16s)(Ltmp >> 16);

      m_tmp = Div_16s(m_tmp, m_tmp1);
      exp_tmp = (Ipp16s)(exp_tmp - exp_tmp1);
      if (exp_tmp > 0) {
         NormalizedBands[i] = ShiftL_16s(m_tmp, exp_tmp);
      } else {
         NormalizedBands[i] = (Ipp16s)(m_tmp >> (-exp_tmp));
      }
   }

   Ltmp = 0;
   for(i=CS;i<COMPLEN2;i++) {
      Ltmp = Add_32s(Ltmp, Mul2_Low_32s(NormalizedBands[i]*tblBandFreqs_16s[i]));
   }
   AverFreqFiltBand = Cnvrt_NR_32s16s(Ltmp);

   if(levL == 0) {
      levRatio = 0;
   } else {
      if (levH == 0) {
         levH = 1;
      }
      Ltmp = levL;
      exp_tmp1 = (Ipp16s)(31 - Norm_32s_I(&Ltmp));
      tmp1 = (Ipp16s)(Ltmp >> 16);

      Ltmp = levH;
      exp_tmp = (Ipp16s)(31 - Norm_32s_I(&Ltmp));
      tmp16 = (Ipp16s)(Ltmp >> 16);


      if(tmp1 > tmp16) {
         tmp1 = (Ipp16s)(tmp1 >> 1);
         exp_tmp1 = (Ipp16s)(exp_tmp1 + 1);
      }
      tmp1 = Div_16s(tmp1, tmp16);
      exp_tmp1 = (Ipp16s)(exp_tmp1 - exp_tmp);
      levRatio = (Ipp16s)((tmp1*29789)>>15);
      if (8 >= exp_tmp1) {
         levRatio = (Ipp16s)(levRatio >> (8-exp_tmp1));
      } else {
         levRatio = ShiftL_16s(levRatio, (Ipp16u)(exp_tmp1-8));
      }
   }

   Ltmp = 0;
   Ltmp1 = 0;

   for(j=CS;j<COMPLEN2;j++) {
      Ltmp = Add_32s(Ltmp, Mul2_Low_32s(stClass->stdDevHistTime[j]*2731));
      Ltmp1 = Add_32s(Ltmp1, Mul2_Low_32s(stClass->stdDevHistTimeShort[j]*2731));
   }
   stdAve = Cnvrt_NR_32s16s(Ltmp);
   stdAveShort = Cnvrt_NR_32s16s(Ltmp1);

   if(stClass->vadFlag[sfIndex] > 0) {
      for(i=CS;i<COMPLEN2;i++) {
         for(j=TWINLEN-1; j>0; j--) {
            stClass->levelHist[j][i] = stClass->levelHist[j-1][i];
         }
      }
   }

   if(stClass->vadFlag[sfIndex] > 0) {
      for(j=LPHLEN-1;j>0;j--) {
         stClass->lphBuf[j] =  stClass->lphBuf[j-1];
      }
      stClass->lphBuf[0] = levRatio;
   }
   Ltmp = 0;
   for(j=0;j<LPHLEN;j++) {
      Ltmp = Add_32s(Ltmp, Mul2_Low_32s(stClass->lphBuf[j]*8192));
   }
   lph = Cnvrt_NR_32s16s(Ltmp);

   if( (stClass->vadFlag[sfIndex]>0) && (sfIndex==3) ) {
      for(j=LPHAVELEN-1; j>0; j--) {
         stClass->lphAveBuf[j] = stClass->lphAveBuf[j-1];
      }
   }
   stClass->lphAveBuf[0] = lph;

   Ltmp = 0;

   for(j=0;j<LPHAVELEN;j++) {
      Ltmp = Add_32s(Ltmp, Mul2_Low_32s(stClass->lphAveBuf[j]*tblClssWeight_16s[j]));
   }
   lph = Cnvrt_NR_32s16s(Ltmp);

   if (stClass->StatClassCount == 0) {
      if (stdAve <= 819) {
         headMode = TCX_MODE;
      } else if (lph > (280*QLPH) ) {
         headMode = TCX_MODE;
      } else if (stdAve > 819) {
         tmp16 = Sub_16s(stdAve,819);
         exp_tmp = Exp_16s(tmp16);
         tmp16 = ShiftL_16s(tmp16, exp_tmp);
         exp_tmp = (Ipp16s)(15 - exp_tmp);
         tmp16 = Div_16s(16384, tmp16);
         if (3 > exp_tmp) {
            tmp1 = Sub_16s(Add_16s(ShiftL_16s(tmp16, (Ipp16u)(3-exp_tmp)), 5*QLPH), lph);
         } else {
            tmp1 = Sub_16s(Add_16s((Ipp16s)(tmp16 >> (exp_tmp-3)), 5*QLPH), lph);
         }

         Ltmp1 = Add_32s(491520, Mul2_Low_32s(stdAve*(-90)));
         Ltmp1 = Sub_32s(Ltmp1, ShiftL_32s(lph, 6));

         if (tmp1>0) {
            headMode = TCX_MODE;
         } else if (Ltmp1 < 0) {
            headMode = ACELP_MODE;
         } else {
            headMode = TCX_OR_ACELP;
         }
      }
   } else {
      headMode = TCX_OR_ACELP;
   }

   if ( ((headMode==ACELP_MODE) || (headMode==TCX_OR_ACELP)) &&
        (AverFreqFiltBand>FiltBand_Thres) ) {
      headMode = TCX_MODE;
   }

   if (stClass->StatClassCount < 5) {
      if (headMode == TCX_OR_ACELP) {
         if (stdAveShort <= 410) {
            headMode = TCX_MODE;
         } else if (stdAveShort > 410) {
            tmp16 = Sub_16s(stdAveShort,410);
            exp_tmp = Exp_16s(tmp16);
            tmp16 = ShiftL_16s(tmp16, exp_tmp);
            exp_tmp = (Ipp16s)(15 - exp_tmp);
            tmp16 = Div_16s(16384, tmp16);
            if (3 > exp_tmp) {
               tmp1 = Sub_16s(Add_16s(ShiftL_16s(tmp16, (Ipp16u)(3-exp_tmp)), QLPH_2_5), lph);
            } else {
               tmp1 = Sub_16s(Add_16s((Ipp16s)(tmp16>>(exp_tmp-3)), QLPH_2_5), lph);
            }

            Ltmp1 = Add_32s(573440, Mul2_Low_32s(stdAveShort*(-90)));
            Ltmp1 = Sub_32s(Ltmp1, ShiftL_32s(lph, 11+1-6));

            if (tmp1 > 0) {
               headMode = TCX_MODE;
            } else if (Ltmp1 < 0) {
               headMode = ACELP_MODE;
            } else {
               headMode = TCX_OR_ACELP;
            }
         }
      }
   }

   if (headMode == TCX_OR_ACELP) {
      if (stClass->StatClassCount < 15) {
         exp_tmp = (Ipp16s)(Exp_32s(stClass->TotalEnergy[0]) - 1);
         tmp16 = (Ipp16s)(ShiftL_32s(stClass->TotalEnergy[0], exp_tmp)>>16);

         exp_tmp = (Ipp16s)(26 - exp_tmp);
         tmp16 = Div_16s(tmp16, 25600);
         exp_tmp = (Ipp16s)(exp_tmp - 5);
         if (10 >= exp_tmp) {
            Ltmp1 = tmp16 >> (10-exp_tmp);
         } else {
            Ltmp1 = ShiftL_32s(tmp16 , (Ipp16u)(exp_tmp-10));
         }
         Ltmp = Sub_32s(Ltmp1,stClass->TotalEnergy[1]);
         if (Ltmp > 0) {
            headMode = ACELP_MODE;
         }
      }
   }

   if ( (headMode==TCX_MODE) || (headMode==TCX_OR_ACELP) ) {
      if ( (AverFreqFiltBand>FiltBand_Thres) && (stClass->TotalEnergy[0]<1920) ) {
         headMode = ACELP_MODE;
      }
   }

   stClass->lph[3] = stClass->lph[2];
   stClass->lph[2] = stClass->lph[1];
   stClass->lph[1] = stClass->lph[0];
   stClass->lph[0] = lph;

   if(stClass->vadFlag[sfIndex]==0) {
      headMode = TCX_MODE;
   }
   return(headMode);
}

void ClassifyExcitationRef(NCLASSDATA_FX *stClass, Ipp16s *ISPs_, Ipp16s *headMode)
{
   Ipp32s sfIndex, i, j, sf2;
   Ipp32s SDminInd = 0, SDmaxInd = 0;
   Ipp16s tmp1 = 0, tmp2 = 0, e_tmp;
   Ipp16s SD_[4] = {0}, tmp16, mag_[DFTN];
   Ipp32s Ltmp1;

   Ipp16s ip_[LPC_N]={0};
   Ipp16s cos_t_[DFTNx2], sin_t_[DFTNx2];
   Ipp32s x_ = 0, y_ = 0;

   for (sfIndex=0; sfIndex<4; sfIndex++) {
      tmp16 = 0;
      for (i=0; i<4; i++) {
         tmp16 = Add_16s(tmp16, Abs_16s(Sub_16s(ISPs_[(sfIndex+1)*M+i], ISPs_[sfIndex*M+i])));
      }
      SD_[sfIndex] =  tmp16;

      if (SD_[sfIndex] < SD_[SDminInd]) {
         SDminInd = sfIndex;
      } else if (SD_[sfIndex] > SD_[SDmaxInd]) {
         SDmaxInd = sfIndex;
      }
   }

   if (stClass->StatClassCount == 15) {
      sf2 = 2;
      stClass->LTPLag[sf2-2] = stClass->LTPLag[sf2];
      stClass->LTPLag[sf2-1] = stClass->LTPLag[sf2+1];
      stClass->LTPGain[sf2-2] = stClass->LTPGain[sf2];
      stClass->LTPGain[sf2-1] = stClass->LTPGain[sf2+1];
      stClass->NormCorr[sf2-2] = stClass->NormCorr[sf2];
      stClass->NormCorr[sf2-1] = stClass->NormCorr[sf2+1];
   }

   for (sfIndex=0; sfIndex<4; sfIndex++) {
      sf2 = (sfIndex<<1) + 2;

      if( (stClass->vadFlag[sfIndex]!=0) && (headMode[sfIndex]==TCX_OR_ACELP) ) {
         if (SD_[sfIndex] > 6554) {
            headMode[sfIndex] = ACELP_MODE;
         } else {
            tmp1 = Abs_16s(Sub_16s(stClass->LTPLag[sf2],stClass->LTPLag[sf2+1]));
            tmp2 = Abs_16s(Sub_16s(stClass->LTPLag[sf2-2],stClass->LTPLag[sf2-1]));

            if ( (tmp1<2) && (tmp2<2) &&
               (Abs_16s(Sub_16s(stClass->LTPLag[sf2-1],stClass->LTPLag[(sfIndex*2+2)]))<2) ) {

               if ( ((stClass->LTPLag[sf2-2]==18) && (stClass->LTPLag[sf2+1]==18)) ||
                  ((stClass->LTPLag[sf2-2]==115) && (stClass->LTPLag[sf2+1]==115)) ) {

                  if ( (Abs_16s(Sub_16s(stClass->LTPGain[sf2],stClass->NormCorr[sf2]))<3277) &&
                     (Abs_16s(Sub_16s(stClass->LTPGain[sf2+1],stClass->NormCorr[sf2+1]))<3277) &&
                     (stClass->NormCorr[sf2] > 29491) && (stClass->NormCorr[sf2+1] > 29491) &&
                     (stClass->NormCorr[sf2-1] > 29491) && (stClass->NormCorr[sf2-2] > 29491) ) {
                        headMode[sfIndex] = ACELP_MODE;
                  } else {
                     headMode[sfIndex] = TCX_MODE;
                  }
               } else if ( (Abs_16s(Sub_16s(stClass->LTPGain[sf2],stClass->NormCorr[sf2]))<3277) &&
                  (Abs_16s(Sub_16s(stClass->LTPGain[sf2+1],stClass->NormCorr[sf2+1]))<3277) &&
                  (stClass->NormCorr[sf2]>28836) && (stClass->NormCorr[sf2+1]>28836) ) {
                     headMode[sfIndex] = ACELP_MODE;
               } else if ( (Abs_16s(Sub_16s(stClass->LTPGain[sf2],stClass->NormCorr[sf2]))>6554) &&
                  (Abs_16s(Sub_16s(stClass->LTPGain[sf2+1],stClass->NormCorr[sf2+1]))>6554) ) {
                     headMode[sfIndex] = TCX_MODE;
               } else {
                  if (sfIndex < 2) {
                     stClass->NoMtcx[0] = Add_16s(stClass->NoMtcx[0],1);
                  } else {
                     stClass->NoMtcx[1] = Add_16s(stClass->NoMtcx[1],1);
                  }
               }
            }

            if (headMode[sfIndex] == TCX_OR_ACELP) {
               Ltmp1 = stClass->TotalEnergy[4];
               for(i=0;i<4;i++) {
                  if(stClass->TotalEnergy[i] > Ltmp1) {
                     Ltmp1=stClass->TotalEnergy[i];
                  }
               }
               if (Ltmp1 < (60*QS) ) {
                  if (SD_[sfIndex] > 4915) {
                     headMode[sfIndex] = ACELP_MODE;
                  } else {
                     if (sfIndex < 2) {
                        stClass->NoMtcx[0] = Add_16s(stClass->NoMtcx[0],1);
                     } else {
                        stClass->NoMtcx[1] = Add_16s(stClass->NoMtcx[1],1);
                     }
                  }
               }
            }
         }
      } else if( (stClass->vadFlag[sfIndex]!=0) && (headMode[sfIndex]==ACELP_MODE) ) {
         if ( (Abs_16s(Sub_16s(stClass->LTPLag[sf2],stClass->LTPLag[sf2+1]))<2) &&
            (Abs_16s(Sub_16s(stClass->LTPLag[sf2-2],stClass->LTPLag[sf2-1]))<2)) {

            if ( (stClass->NormCorr[sf2]<26214) && (stClass->NormCorr[sf2+1]<26214) &&
               (SD_[SDmaxInd]<3277) ) {
                  headMode[sfIndex] = TCX_MODE;
            }
         }
         if ( (stClass->lph[sfIndex]>(200*QLPH)) && (SD_[SDmaxInd]<3277)) {
            headMode[sfIndex] = TCX_MODE;
         }
      }
   }

   for(sfIndex=0;sfIndex<4;sfIndex++) {
      sf2 = (sfIndex<<1) + 2;
      if ( (stClass->vadFlag_old[sfIndex] == 0) &&
          (stClass->vadFlag[sfIndex] == 1) &&
          (headMode[sfIndex]==TCX_MODE)) {

         if (sfIndex < 2) {
            stClass->NoMtcx[0] = Add_16s(stClass->NoMtcx[0],1);
         } else {
            stClass->NoMtcx[1] = Add_16s(stClass->NoMtcx[1],1);
         }
      }

      if (headMode[sfIndex] != ACELP_MODE) {
         Ltmp1 = Abs_32s(Sub_32s(stClass->LTPGain[sf2-2], stClass->NormCorr[sf2-2]));
         Ltmp1 = Add_32s(Ltmp1, Abs_32s(Sub_32s(stClass->LTPGain[sf2-1], stClass->NormCorr[sf2-1])));
         Ltmp1 = Add_32s(Ltmp1, Abs_32s(Sub_32s(stClass->LTPGain[sf2], stClass->NormCorr[sf2])));
         Ltmp1 = Add_32s(Ltmp1, Abs_32s(Sub_32s(stClass->LTPGain[sf2+1], stClass->NormCorr[sf2+1])));

         if ( (Ltmp1 < 786) &&
            (stClass->NormCorr[sf2]>30147) && (stClass->NormCorr[sf2+1]>30147) &&
            (stClass->LTPLag[sf2]>21) && (stClass->LTPLag[sf2+1]>21) ) {

            for (i=0; i<DFTNx2; i++) {
               cos_t_[i] = tblSinCos_16s[i*N_MAX/DFTNx2 + COSOFFSET];
               sin_t_[i] = tblSinCos_16s[i*N_MAX/DFTNx2];
            }
            for (i=0; i<LPC_N; i++) {
               ip_[i] = stClass->ApBuf[sfIndex*M+i];
            }
            mag_[0] = 0;
            for (i=0; i<DFTN; i++) {
               x_ = y_ = 0;
               for (j=0; j<LPC_N; j++) {
                  x_ = Add_32s(x_, Mul2_Low_32s(ip_[j] * (cos_t_[(i*j)&(DFTNx2-1)])));
                  y_ = Add_32s(y_, Mul2_Low_32s(ip_[j] * (sin_t_[(i*j)&(DFTNx2-1)])));
               }
               tmp16= Cnvrt_NR_32s16s(x_);
               Ltmp1 = Mul2_Low_32s(tmp16*tmp16);
               tmp16 = Cnvrt_NR_32s16s(y_);
               Ltmp1 = Add_32s(Ltmp1, Mul2_Low_32s(tmp16*tmp16));
               e_tmp = (Ipp16s)(10 - Norm_32s_I(&Ltmp1));

               InvSqrt_32s16s_I(&Ltmp1, &e_tmp);
               if (25 >= e_tmp) {
                  mag_[i] = (Ipp16s)(Ltmp1 >> (25-e_tmp));
               } else {
                  mag_[i] = (Ipp16s)(ShiftL_32s(Ltmp1, (Ipp16u)(e_tmp-25)));
               }
            }
            Ltmp1 = 0;
            for (i=1; i<40; i++) {
               Ltmp1 = Add_32s(mag_[i], Ltmp1);
            }
            if ((Ltmp1>6080) && (mag_[0] < 320) ) {
               headMode[sfIndex] = TCX_MODE;
            } else {
               headMode[sfIndex] = ACELP_MODE;
               if (sfIndex < 2) {
                  stClass->NoMtcx[0] = Add_16s(stClass->NoMtcx[0],1);
               } else {
                  stClass->NoMtcx[1] = Add_16s(stClass->NoMtcx[1],1);
               }
            }
         }
      }

      if (stClass->StatClassCount < 12) {
         if (headMode[sfIndex] == TCX_OR_ACELP) {
            tmp1 = 0;
            tmp2 = 0;
            for(i=0;i<4;i++) {
               if ( ( (stClass->prevModes[i]==3) || (stClass->prevModes[i]==2) ) &&
                  (stClass->vadFlag_old[i]==1) && (stClass->TotalEnergy[i]>(60*QS)) ) {
                     tmp1 = Add_16s(tmp1,1);
               }
               if (stClass->prevModes[i] == ACELP_MODE) {
                  tmp2 = Add_16s(tmp2,1);
               }
               if (sfIndex != i) {
                  if (headMode[i] == ACELP_MODE) {
                     tmp2 = Add_16s(tmp2,1);
                  }
               }
            }
            if (tmp1 > 3) {
               headMode[sfIndex] = TCX_MODE;
            } else if (tmp2 > 1) {
               headMode[sfIndex] = ACELP_MODE;
            } else {
               headMode[sfIndex] = TCX_MODE;
            }
         }
      } else {
         if (headMode[sfIndex] == TCX_OR_ACELP) {
            headMode[sfIndex] = TCX_MODE;
         }
      }
   }
   stClass->NbOfAcelps = 0;

   for(sfIndex=0;sfIndex<4;sfIndex++) {
      if (headMode[sfIndex] == ACELP_MODE) {
         stClass->NbOfAcelps  = Add_16s(stClass->NbOfAcelps,1);
      }
   }

   stClass->LTPGain[0] = stClass->LTPGain[8];
   stClass->LTPGain[1] = stClass->LTPGain[9];
   stClass->LTPLag[0] = stClass->LTPLag[8];
   stClass->LTPLag[1] = stClass->LTPLag[9];
   stClass->NormCorr[0] = stClass->NormCorr[8];
   stClass->NormCorr[1] = stClass->NormCorr[9];

   for(i=0;i<4;i++) {
      stClass->vadFlag_old[i] = stClass->vadFlag[i];
      if (stClass->StatClassCount > 0) {
         stClass->StatClassCount = Sub_16s(stClass->StatClassCount,1);
      }
   }
   return;
}




Ipp16s ownScaleTwoChannels_16s(Ipp16s* pChannelLeft, Ipp16s* pChannelRight, Ipp32s len,
      Ipp16s *scaleCurr, Ipp16s *scalePrev, Ipp32s BitToRemove)
{
   Ipp16s maxLeft, maxRight, scaleTmp, scaleNew;

   ippsMaxAbs_16s(pChannelLeft, len, &maxLeft);
   ippsMaxAbs_16s(pChannelRight, len, &maxRight);
   maxRight = IPP_MAX(1, maxRight);
   maxRight = (Ipp16s)(IPP_MAX(maxRight, maxLeft));

   scaleNew = (Ipp16s)(Exp_16s(maxRight) - BitToRemove);
   scaleNew = (Ipp16s)(IPP_MIN(scaleNew, Q_MAX2));

   scaleTmp = scaleNew;
   scaleNew = (Ipp16s)(IPP_MIN(scaleNew, scalePrev[1]));
   scalePrev[1] = scalePrev[0];
   scaleNew = (Ipp16s)(IPP_MIN(scaleNew, scalePrev[0]));
   scalePrev[0] = scaleTmp;

   ownScaleSignal_AMRWB_16s_ISfs(pChannelRight, len, scaleNew);
   ownScaleSignal_AMRWB_16s_ISfs(pChannelLeft, len, scaleNew);

   scaleTmp = *scaleCurr;
   *scaleCurr = scaleNew;

   return (Ipp16s)(scaleNew - scaleTmp);
}

Ipp16s ownScaleOneChannel_16s(Ipp16s* pChannel, Ipp32s len,
      Ipp16s *scaleCurr, Ipp16s *scalePrev, Ipp32s BitToRemove)
{
   Ipp16s maxVal=1, scaleNew, scaleTmp;

   ippsMaxAbs_16s(pChannel, len, &maxVal);
   maxVal = IPP_MAX(1, maxVal);

   scaleNew = (Ipp16s)(Exp_16s(maxVal) - BitToRemove);
   scaleNew = (Ipp16s)(IPP_MIN(scaleNew, Q_MAX2));

   scaleTmp = scaleNew;
   scaleNew = (Ipp16s)(IPP_MIN(scaleNew, scalePrev[1]));
   scalePrev[1] = scalePrev[0];
   scaleNew = (Ipp16s)(IPP_MIN(scaleNew, scalePrev[0]));
   scalePrev[0] = scaleTmp;

   ownScaleSignal_AMRWB_16s_ISfs(pChannel, len, scaleNew);

   scaleTmp = *scaleCurr;
   *scaleCurr = scaleNew;
   return (Ipp16s)(scaleNew - scaleTmp);
}


void ownRescaleEncoderStereo(EncoderObj_AMRWBE *pObj, Ipp16s scaleSpch, Ipp16s scalePrmph)
{
   if (scaleSpch != 0) {
      Ipp16s memHPFilter[6];
      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.mem_decim,L_MEM_DECIM_SPLIT, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->left.mem_decim,L_MEM_DECIM_SPLIT, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(&(pObj->right.mem_preemph),1, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(&(pObj->left.mem_preemph),1, scaleSpch);

      ippsHighPassFilterGetDlyLine_AMRWB_16s(pObj->right.memSigIn, memHPFilter, 2);
      ownScaleHPFilterState(memHPFilter, scaleSpch, 2);
      ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, pObj->right.memSigIn, 2);
      ippsHighPassFilterGetDlyLine_AMRWB_16s(pObj->left.memSigIn, memHPFilter, 2);
      ownScaleHPFilterState(memHPFilter, scaleSpch, 2);
      ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, pObj->left.memSigIn, 2);

      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.mem_decim_hf, 2*L_FILT24k, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->left.mem_decim_hf, 2*L_FILT24k, scaleSpch);

      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.old_speech_hf, L_OLD_SPEECH_ST, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->left.old_speech_hf, L_OLD_SPEECH_ST, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.mem_hf3, MHF, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->left.mem_hf3, MHF, scaleSpch);

      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_chan, L_OLD_SPEECH_ST, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_chan_2k, L_OLD_SPEECH_2k, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_speech_hi, L_OLD_SPEECH_hi, scaleSpch);

      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_speech, L_OLD_SPEECH_ST, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_speech_2k, L_OLD_SPEECH_2k, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_chan_hi, L_OLD_SPEECH_hi, scaleSpch);

      ownScaleSignal_AMRWB_16s_ISfs(pObj->mem_stereo_ovlp, L_OVLP_2k, scaleSpch);
   }

   if(scalePrmph != 0) {
      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.mem_hf1, MHF, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->left.mem_hf1, MHF, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.mem_hf2, MHF, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->left.mem_hf2, MHF, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_speech_pe, L_OLD_SPEECH_ST, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_synth, M, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(&(pObj->mem_wsp), 1, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->mem_lp_decim2, 3, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_wovlp, 128, scalePrmph);
   }
}
void ownRescaleEncoderMono(EncoderObj_AMRWBE *pObj, Ipp16s scaleSpch, Ipp16s scalePrmph)
{
   if (scaleSpch != 0) {
      Ipp16s memHPFilter[6];
      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.mem_decim,L_MEM_DECIM_SPLIT, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(&(pObj->right.mem_preemph),1, scaleSpch);

      ippsHighPassFilterGetDlyLine_AMRWB_16s(pObj->right.memSigIn, memHPFilter, 2);
      ownScaleHPFilterState(memHPFilter, scaleSpch, 2);
      ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, pObj->right.memSigIn, 2);

      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.mem_decim_hf, 2*L_FILT24k, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.old_speech_hf, L_OLD_SPEECH_ST, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.mem_hf3, MHF, scaleSpch);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_exc_mono, HI_FILT_ORDER, scaleSpch);
   }

   if(scalePrmph != 0) {
      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.mem_hf1, MHF, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->right.mem_hf2, MHF, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_speech_pe, L_OLD_SPEECH_ST, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_synth, M, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(&(pObj->mem_wsp), 1, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->mem_lp_decim2, 3, scalePrmph);
      ownScaleSignal_AMRWB_16s_ISfs(pObj->old_wovlp, 128, scalePrmph);
   }
}

void ownMixChennels_16s(Ipp16s *pChennelLeft, Ipp16s *pChennelRight, Ipp16s *pChennelMix,
      Ipp32s len, Ipp16s wghtLeft, Ipp16s wghtRight)
{
   Ipp32s i;
   for (i=0; i<len; i++) {
     pChennelMix[i] = Cnvrt_NR_32s16s( ((wghtLeft*pChennelLeft[i])+(wghtRight*pChennelRight[i])) << 1 );
   }
}

void Preemph_scaled(Ipp16s* new_speech, Ipp16s *Q_new, Ipp16s *exp, Ipp16s *mem_preemph,
      Ipp16s *Q_max, Ipp16s *Q_old, Ipp16s Preemph_factor, Ipp16s bits, Ipp32s Lframe)
{
   Ipp16s mu, shift, QVal, tmp_fixed;
   Ipp32s i, L_tmp, L_max;

   QVal = (Ipp16s)(1 << (15-bits));
   mu = (Ipp16s)(Preemph_factor >> bits);

   L_tmp = Mul2_Low_32s(new_speech[0]*QVal);
   L_tmp = Sub_32s(L_tmp, Mul2_Low_32s((*mem_preemph)* mu));
   L_max = Abs_32s(L_tmp);

   for (i = 1; i < Lframe; i++) {
      L_tmp = Mul2_Low_32s(new_speech[i]*QVal);
      L_tmp = Sub_32s(L_tmp, Mul2_Low_32s(new_speech[i - 1]*mu));
      L_tmp = Abs_32s(L_tmp);
      if (L_tmp > L_max) {
            L_max = L_tmp;
      }
   }

   tmp_fixed = (Ipp16s)(L_max>>16);

   if (tmp_fixed == 0) {
        shift = Q_MAX3;
   } else {
      shift = (Ipp16s)(Exp_16s(tmp_fixed) - bits);
      if (shift < 0) {
         shift = 0;
      }
      if (shift > Q_MAX3) {
         shift = Q_MAX3;
      }
   }
   *Q_new = shift;

   if ((*Q_new) > Q_max[0]) {
      *Q_new = Q_max[0];
   }

   if ((*Q_new) > Q_max[1]) {
      *Q_new = Q_max[1];
   }

   *exp = (Ipp16s)((*Q_new) - (*Q_old));
   *Q_old   = *Q_new;
   Q_max[1] = Q_max[0];
   Q_max[0] = shift;

   tmp_fixed = new_speech[Lframe - 1];

   for (i=(Lframe-1); i>0; i--) {
      L_tmp = Mul2_Low_32s(new_speech[i]*QVal);
      L_tmp = Sub_32s(L_tmp, Mul2_Low_32s(new_speech[i - 1]*mu));
      L_tmp = ShiftL_32s(L_tmp, *Q_new);
      new_speech[i] = Cnvrt_NR_32s16s(L_tmp);
   }

   L_tmp = Mul2_Low_32s(new_speech[0]*QVal);
   L_tmp = Sub_32s(L_tmp, Mul2_Low_32s((*mem_preemph)*mu));
   L_tmp = ShiftL_32s(L_tmp, *Q_new);
   new_speech[0] = Cnvrt_NR_32s16s(L_tmp);

   *mem_preemph = tmp_fixed;
}

Ipp32s Unpack4bits(Ipp32s nbits, Ipp16s *prm, Ipp16s *ptr)
{
   Ipp32s i=0;

   while (nbits > 4) {
      Int2bin(prm[i], 4, ptr);
      ptr += 4;
      nbits = nbits - 4;
      i++;
   }
   Int2bin(prm[i], nbits, ptr);
   i++;
   return(i);
}

void Enc_prm(Ipp16s* mod, Ipp32s codec_mode, Ipp16s* sparam, Ipp16s* serial, Ipp32s nbits_pack)
{
   Ipp16s sfr, mode, *sprm, parity, bit;
   Ipp32s k, n, j, nbits;
   Ipp32s nbits_AVQ[N_PACK_MAX];
   Ipp16s prm_AVQ[(NBITS_MAX/4)+N_PACK_MAX];
   Ipp16s *ptr;

   nbits = (tblNBitsMonoCore_16s[codec_mode]>>2) - 2;

   k = 0;
   while (k < NB_DIV) {
      mode = mod[k];
      sprm = sparam + (k*NPRM_DIV);
      if ( (mode == 0) || (mode==1) ) {
         ptr = serial + (2 + (k*nbits_pack));
         Int2bin(sprm[0], 8, ptr);      ptr += 8;
         Int2bin(sprm[1], 8, ptr);      ptr += 8;
         Int2bin(sprm[2], 6, ptr);      ptr += 6;
         Int2bin(sprm[3], 7, ptr);      ptr += 7;
         Int2bin(sprm[4], 7, ptr);      ptr += 7;
         Int2bin(sprm[5], 5, ptr);      ptr += 5;
         Int2bin(sprm[6], 5, ptr);      ptr += 5;

         if (mode == 0) {
            j = 7;
            Int2bin(sprm[j], 2, ptr);       ptr += 2;    j++;
            for (sfr=0; sfr<4; sfr++) {
               if ((sfr == 0) || (sfr==2) ) {
                  n=9;
               } else {
                  n=6;
               }

               Int2bin(sprm[j], n, ptr);       ptr += n;    j++;
               Int2bin(sprm[j], 1, ptr);       ptr += 1;    j++;

               if (codec_mode == MODE_9k6) {
                  Int2bin(sprm[j], 5, ptr);       ptr += 5;     j++;
                  Int2bin(sprm[j], 5, ptr);       ptr += 5;     j++;
                  Int2bin(sprm[j], 5, ptr);       ptr += 5;     j++;
                  Int2bin(sprm[j], 5, ptr);       ptr += 5;     j++;
               } else if (codec_mode == MODE_11k2) {
                  Int2bin(sprm[j], 9, ptr);       ptr += 9;     j++;
                  Int2bin(sprm[j], 9, ptr);       ptr += 9;     j++;
                  Int2bin(sprm[j], 5, ptr);       ptr += 5;     j++;
                  Int2bin(sprm[j], 5, ptr);       ptr += 5;     j++;
               } else if (codec_mode == MODE_12k8) {
                  Int2bin(sprm[j], 9, ptr);       ptr += 9;     j++;
                  Int2bin(sprm[j], 9, ptr);       ptr += 9;     j++;
                  Int2bin(sprm[j], 9, ptr);       ptr += 9;     j++;
                  Int2bin(sprm[j], 9, ptr);       ptr += 9;     j++;
               } else if (codec_mode == MODE_14k4) {
                  Int2bin(sprm[j], 13, ptr);      ptr += 13;    j++;
                  Int2bin(sprm[j], 13, ptr);      ptr += 13;    j++;
                  Int2bin(sprm[j], 9, ptr);       ptr += 9;     j++;
                  Int2bin(sprm[j], 9, ptr);       ptr += 9;     j++;
               } else if (codec_mode == MODE_16k) {
                  Int2bin(sprm[j], 13, ptr);      ptr += 13;    j++;
                  Int2bin(sprm[j], 13, ptr);      ptr += 13;    j++;
                  Int2bin(sprm[j], 13, ptr);      ptr += 13;    j++;
                  Int2bin(sprm[j], 13, ptr);      ptr += 13;    j++;
               } else if (codec_mode == MODE_18k4) {
                  Int2bin(sprm[j], 2, ptr);       ptr += 2;     j++;
                  Int2bin(sprm[j], 2, ptr);       ptr += 2;     j++;
                  Int2bin(sprm[j], 2, ptr);       ptr += 2;     j++;
                  Int2bin(sprm[j], 2, ptr);       ptr += 2;     j++;
                  Int2bin(sprm[j], 14, ptr);      ptr += 14;    j++;
                  Int2bin(sprm[j], 14, ptr);      ptr += 14;    j++;
                  Int2bin(sprm[j], 14, ptr);      ptr += 14;    j++;
                  Int2bin(sprm[j], 14, ptr);      ptr += 14;    j++;
               } else if (codec_mode == MODE_20k) {
                  Int2bin(sprm[j], 10, ptr);      ptr += 10;    j++;
                  Int2bin(sprm[j], 10, ptr);      ptr += 10;    j++;
                  Int2bin(sprm[j], 2,  ptr);      ptr += 2;     j++;
                  Int2bin(sprm[j], 2,  ptr);      ptr += 2;     j++;
                  Int2bin(sprm[j], 10, ptr);      ptr += 10;    j++;
                  Int2bin(sprm[j], 10, ptr);      ptr += 10;    j++;
                  Int2bin(sprm[j], 14, ptr);      ptr += 14;    j++;
                  Int2bin(sprm[j], 14, ptr);      ptr += 14;    j++;
               } else if (codec_mode == MODE_23k2) {
                  Int2bin(sprm[j], 11, ptr);      ptr += 11;    j++;
                  Int2bin(sprm[j], 11, ptr);      ptr += 11;    j++;
                  Int2bin(sprm[j], 11, ptr);      ptr += 11;    j++;
                  Int2bin(sprm[j], 11, ptr);      ptr += 11;    j++;
                  Int2bin(sprm[j], 11, ptr);      ptr += 11;    j++;
                  Int2bin(sprm[j], 11, ptr);      ptr += 11;    j++;
                  Int2bin(sprm[j], 11, ptr);      ptr += 11;    j++;
                  Int2bin(sprm[j], 11, ptr);      ptr += 11;    j++;
               }
               Int2bin(sprm[j], 7, ptr);       ptr += 7;    j++;
            }
         } else {
            nbits_AVQ[0] = nbits - 56;
            ippsEncodeMux_AMRWBE_16s(sprm+9, 288/8, nbits_AVQ, prm_AVQ, 1);

            Int2bin(sprm[7], 3, ptr);     ptr += 3;
            Int2bin(sprm[8], 7, ptr);     ptr += 7;

            Unpack4bits(nbits_AVQ[0], prm_AVQ, ptr);
         }
         k ++;
      } else if (mode == 2) {
         nbits_AVQ[0] = nbits - 26;
         nbits_AVQ[1] = nbits - (30+6);

         ippsEncodeMux_AMRWBE_16s(sprm+9, 576/8, nbits_AVQ, prm_AVQ, 2);

         ptr = serial + (2 + (k*nbits_pack));

         Int2bin(sprm[0], 8, ptr);      ptr += 8;
         Int2bin(sprm[1], 8, ptr);      ptr += 8;
         Int2bin(sprm[7], 3, ptr);      ptr += 3;
         Int2bin(sprm[8], 7, ptr);      ptr += 7;

         j = Unpack4bits(nbits_AVQ[0], prm_AVQ, ptr);
         k ++;

         ptr = serial + (2 + (k*nbits_pack));
         Int2bin(sprm[2], 6, ptr);      ptr += 6;
         Int2bin(sprm[3], 7, ptr);      ptr += 7;
         Int2bin(sprm[4], 7, ptr);      ptr += 7;
         Int2bin(sprm[5], 5, ptr);      ptr += 5;
         Int2bin(sprm[6], 5, ptr);      ptr += 5;
         Int2bin((Ipp16s)(sprm[8]>>1), 6, ptr);   ptr += 6;
         Unpack4bits(nbits_AVQ[1], prm_AVQ+j, ptr);
         k ++;
      } else if (mode == 3) {
         nbits_AVQ[0] = nbits - 23;
         nbits_AVQ[1] = nbits - (9+3);
         nbits_AVQ[2] = nbits - (12+3);
         nbits_AVQ[3] = nbits - (12+3);

         ippsEncodeMux_AMRWBE_16s(sprm+9, 1152/8, nbits_AVQ, prm_AVQ, 4);

         ptr = serial + (2 + (k*nbits_pack));
         Int2bin(sprm[0], 8, ptr);      ptr += 8;
         Int2bin(sprm[1], 8, ptr);      ptr += 8;
         Int2bin(sprm[8], 7, ptr);      ptr += 7;

         j = Unpack4bits(nbits_AVQ[0], prm_AVQ, ptr);
         k ++;

         ptr = serial + (2 + (k*nbits_pack));

         Int2bin(sprm[2], 6, ptr);      ptr += 6;
         Int2bin(sprm[7], 3, ptr);      ptr += 3;

         bit = (Ipp16s)( (((sprm[8]>>6)) & 0x01) ^ (((sprm[8]>>3)) & 0x01) );
         parity = (Ipp16s)(bit << 2);
         bit = (Ipp16s)( (((sprm[8]>>5)) & 0x01) ^ (((sprm[8]>>2)) & 0x01) );
         parity = (Ipp16s)(parity + (bit<<1));
         bit = (Ipp16s)( (((sprm[8]>>4)) & 0x01) ^ (((sprm[8]>>1)) & 0x01));
         parity = (Ipp16s)(parity + bit);
         Int2bin(parity, 3, ptr);      ptr += 3;

         j = j + Unpack4bits(nbits_AVQ[1], &prm_AVQ[j], ptr);
         k ++;

         ptr = serial + (2 + (k*nbits_pack));
         Int2bin(sprm[3], 7, ptr);      ptr += 7;
         Int2bin(sprm[5], 5, ptr);      ptr += 5;
         Int2bin((Ipp16s)(sprm[8]>>4), 3, ptr);      ptr += 3;
         j = j + Unpack4bits(nbits_AVQ[2], &prm_AVQ[j], ptr);
         k ++;

         ptr = serial + (2 + (k*nbits_pack));
         Int2bin(sprm[4], 7, ptr);      ptr += 7;
         Int2bin(sprm[6], 5, ptr);      ptr += 5;
         Int2bin((Ipp16s)(((sprm[8]>>1)) & 0x07), 3, ptr);      ptr += 3;
         Unpack4bits(nbits_AVQ[3], &prm_AVQ[j], ptr);
         k++;
      }
   }
   return;
}

void Enc_prm_stereo_x(Ipp16s* sparam, Ipp16s* sserial, Ipp32s nbits_pack, Ipp32s nbits_bwe, Ipp32s brMode)
{
   Ipp32s j, k;
   Ipp16s mod[NB_DIV], mode;
   Ipp32s nbits, hf_bits=0;
   Ipp32s hiband_mode;
   Ipp16s *prm, *ptr;
   Ipp32s nbits_AVQ[NB_DIV];
   Ipp16s prm_AVQ[(NBITS_MAX/4)+N_PACK_MAX];

   nbits = (tblNBitsStereoCore_16s[brMode] + (nbits_bwe<<1)) >> 2;

   hiband_mode = 0;
   if (tblNBitsStereoCore_16s[brMode] > (300+4) ) {
      hiband_mode = 1;
   }

   for(k=0;k<NB_DIV;k++) {
      prm = sparam + k*NPRM_STEREO_HI_X;

      ptr = sserial + ((Ipp16s)(Mul2_Low_32s((k+1)*nbits_pack)>>1) - nbits);

      if(hiband_mode == 0) {
         Int2bin(prm[0],4, ptr); ptr += 4;
         Int2bin(prm[1],2, ptr);
      } else {
         Int2bin(prm[0],4, ptr); ptr += 4 ;
         Int2bin(prm[1],3, ptr); ptr += 3 ;
         Int2bin(prm[2],5, ptr);
      }
   }

   if (hiband_mode == 0) {
      hf_bits = 4+2;
   } else {
      hf_bits = 7+5;
   }

   mod[0] = sparam[0+NPRM_STEREO_HI_X*NB_DIV];
   mod[1] = sparam[1+NPRM_STEREO_HI_X*NB_DIV];
   mod[2] = sparam[2+NPRM_STEREO_HI_X*NB_DIV];
   mod[3] = sparam[3+NPRM_STEREO_HI_X*NB_DIV];

   k = 0;
   while (k < NB_DIV) {
      mode = mod[k];
      prm = sparam + ((4+NPRM_STEREO_HI_X*NB_DIV)+ (k*NPRM_DIV_TCX_STEREO));
      if ((mode==1) || (mode == 0)) {
         nbits_AVQ[0] = ((tblNBitsStereoCore_16s[brMode]-4)>>2) - (7+2+7+hf_bits);
         ippsEncodeMux_AMRWBE_16s(&prm[2], TOT_PRM_20/8, nbits_AVQ, prm_AVQ, 1);
         ptr = sserial + ((k+1)*nbits_pack - nbits + hf_bits);
         *ptr = 0;  ptr++;
         Int2bin(mode, 2, ptr);       ptr += 2;
         Int2bin(prm[0], 7, ptr);     ptr += 7;
         Int2bin(prm[1], 7, ptr);     ptr += 7;
         Unpack4bits(nbits_AVQ[0], prm_AVQ, ptr);
         k ++;
      } else if (mode == 2) {
         nbits_AVQ[0] = ((tblNBitsStereoCore_16s[brMode]-4)>>2) - (2+7+hf_bits);
         nbits_AVQ[1] = nbits_AVQ[0];
         ippsEncodeMux_AMRWBE_16s(&prm[2], TOT_PRM_40/8, nbits_AVQ, prm_AVQ, 2);
         ptr = sserial + ((k+1)*nbits_pack - nbits + hf_bits);
         *ptr = 0;  ptr++;
         Int2bin(mode, 2, ptr);     ptr += 2;
         Int2bin(prm[0], 7, ptr);     ptr += 7;
         j = Unpack4bits(nbits_AVQ[0], prm_AVQ, ptr);
         ptr = sserial + ((k+2)*nbits_pack - nbits + hf_bits);
         *ptr = 0;  ptr++;
         Int2bin(mode, 2, ptr);     ptr += 2;
         Int2bin(prm[1], 7, ptr);     ptr += 7;
         Unpack4bits(nbits_AVQ[1], &prm_AVQ[j], ptr);
         k += 2;
      } else if (mode == 3) {
         nbits_AVQ[0] = ((tblNBitsStereoCore_16s[brMode]-4)>>2) - (2+7+hf_bits);
         nbits_AVQ[1] = ((tblNBitsStereoCore_16s[brMode]-4)>>2) - (2+hf_bits);
         nbits_AVQ[2] = nbits_AVQ[0];
         nbits_AVQ[3] = nbits_AVQ[1];

         ippsEncodeMux_AMRWBE_16s(&prm[2], TOT_PRM_80/8, nbits_AVQ, prm_AVQ, 4);

         ptr = sserial + ((k+1)*nbits_pack - nbits + hf_bits);
         *ptr = 0;  ptr++;
         Int2bin(mode, 2, ptr);     ptr += 2;
         Int2bin(prm[0], 7, ptr);     ptr += 7;
         j = Unpack4bits(nbits_AVQ[0], prm_AVQ, ptr);

         ptr = sserial + ((k+2)*nbits_pack - nbits + hf_bits);
         *ptr = 0;  ptr++;
         Int2bin(mode, 2, ptr);     ptr += 2;
         j = j + Unpack4bits(nbits_AVQ[1], &prm_AVQ[j], ptr);
         ptr = sserial + ((k+3)*nbits_pack - nbits + hf_bits);
         *ptr = 0;  ptr++;
         Int2bin(mode, 2, ptr);     ptr += 2;
         Int2bin(prm[1], 7, ptr);     ptr += 7;
         j = j + Unpack4bits(nbits_AVQ[2], &prm_AVQ[j], ptr);
         ptr = sserial + ((k+4)*nbits_pack - nbits + hf_bits);
         *ptr = 0;  ptr++;
         Int2bin(mode, 2, ptr);     ptr += 2;
         Unpack4bits(nbits_AVQ[3], &prm_AVQ[j], ptr);
         k +=4;
      }
   }
}


void Enc_prm_hf(Ipp16s* mod, Ipp16s* param, Ipp16s* serial, Ipp32s nbits_pack)
{
   Ipp32s i, k, mode, nbits;
   Ipp16s *ptr, *prm;

   nbits = NBITS_BWE/4;
   k = 0;
   while (k < NB_DIV) {
      mode = mod[k];
      prm = param + (k*NPRM_BWE_DIV);

      ptr = serial + ((k+1)*nbits_pack - nbits);
      Int2bin(prm[0], 2, ptr);      ptr += 2;
      Int2bin(prm[1], 7, ptr);      ptr += 7;
      Int2bin(prm[2], 7, ptr);
      k++;

      if (mode == 2) {
         ptr = serial + ((k+1)*nbits_pack - nbits);
         for (i=3; i<=10; i++) {
            Int2bin(prm[i], 2, ptr);    ptr += 2;
         }
         k++;
      } else if (mode == 3) {
         ptr = serial + ((k+1)*nbits_pack - nbits);
         for (i=3; i<=10; i++) {
            Int2bin((Ipp16s)(prm[i]>>1), 2, ptr);   ptr += 2;
         }
         k++;
         ptr = serial + ((k+1)*nbits_pack - nbits);
         for (i=11; i<=18; i++) {
            Int2bin((Ipp16s)(prm[i]>>1), 2, ptr);   ptr += 2;
         }
         k++;
         ptr = serial + ((k+1)*nbits_pack - nbits);
         for (i=3; i<=18; i++) {
            Int2bin(prm[i], 1, ptr);      ptr += 1;
         }
         k++;
      }
   }
}

#define L2SCL 10
#define SCL3 0
#define MAX_VECT_DIM 16

void Compute_exc_side(Ipp16s* exc_side ,Ipp16s* exc_mono)
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, pTmpBuf, (L_FRAME_PLUS+L_SUBFR));
   ippsSub_16s(exc_side, exc_mono, pTmpBuf, (L_FRAME_PLUS+L_SUBFR));
   ippsCopy_16s(pTmpBuf, exc_side, (L_FRAME_PLUS+L_SUBFR));
}

static void M_cbcod(
    Ipp16s mpfade[INTENS_MAX][MAX_NUMSTAGES],
    Ipp32s *dist,
    Ipp16s *x,
    Ipp16s *pfad,
    Ipp32s stage,
    const Ipp16s *cb,
    Ipp32s cbsize,
    Ipp32s m,
    Ipp32s *m_best,
    Ipp32s *m_dist,
    Ipp32s vdim
)

{
   Ipp32s i,j;
   const Ipp16s *y;
   Ipp32s Ldist, endloop;
   Ipp16s d;

   for (i=0; i<cbsize; i++) {
      y = &cb[i*vdim];
      Ldist = 0;
      endloop = 0;
      for (j=0; j<vdim; j++) {
         d = Cnvrt_NR_32s16s(Sub_32s(Mul2_Low_32s(x[j]*16384),Mul2_Low_32s(y[j]*8192)));
         Ldist = Add_32s(Ldist, Mul2_Low_32s(d*d));
         if (Ldist >= (*m_dist) ) {
            endloop = 1;
         }
      }

      if(endloop == 0) {
         *m_dist = Ldist;
         dist[*m_best] = Ldist;

         for (j=0; j<stage; j++) {
              mpfade[*m_best][j] = pfad[j];
         }
         mpfade[*m_best][stage] = (Ipp16s)i;

         for (j=0; j<m; j++) {
            if (dist[j] > *m_dist) {
               *m_dist = dist[j];
               *m_best = j;
            }
         }
      }
   }
}

static void Msvq2(Ipp16s *y, Ipp16s *inds, Ipp16s *x, const sMSPQTab* msvq_tab_fx)
{
   Ipp32s i, j, k, l, pfadanz_max;
   Ipp32s m_best, m_dist, dwTmp;
   Ipp16s e[MAX_VECT_DIM];
   Ipp16s mpfade_mem1[INTENS_MAX][MAX_NUMSTAGES];
   Ipp16s mpfade_mem2[INTENS_MAX][MAX_NUMSTAGES];
   Ipp16s (*mpfade)[INTENS_MAX][MAX_NUMSTAGES];
   Ipp16s (*alt_mpfade)[INTENS_MAX][MAX_NUMSTAGES];
   Ipp16s (*tmp_pfad)[INTENS_MAX][MAX_NUMSTAGES];
   Ipp32s dist[MAX_NUMSTAGES][INTENS_MAX];

   const Ipp16s *cbsize = msvq_tab_fx->cbsizes;
   const Ipp16s **wcb = msvq_tab_fx->cbs;
   Ipp32s vdim = msvq_tab_fx->vdim;
   Ipp32s stages = msvq_tab_fx->nstages;
   Ipp32s m = msvq_tab_fx->intens;



   mpfade = &mpfade_mem1;
   alt_mpfade = &mpfade_mem2;
   for (j=0; j<m; j++) {
      dist[0][j] = IPP_MAX_32S;
   }
   m_best = 0;
   m_dist = IPP_MAX_32S;

   pfadanz_max = cbsize[0];
   M_cbcod(*mpfade, dist[0], x, NULL, 0, wcb[0], cbsize[0], m, &m_best, &m_dist, vdim);
   for (i=1; i<stages; i++) {
      for (j=0; j<m; j++) {
         dist[i][j] = IPP_MAX_32S;
      }
      m_best = 0;
      m_dist = IPP_MAX_32S;

      tmp_pfad = mpfade;
      mpfade = alt_mpfade;
      alt_mpfade = tmp_pfad;

      dwTmp = m;
      if (dwTmp > pfadanz_max) {
         dwTmp = pfadanz_max;
      }

      for (k=0; k<dwTmp; k++) {
         for (j = 0; j < vdim; j++) {
            e[j] = x[j];
            for (l = 0; l < i; l++) {
               e[j] = Sub_16s(e[j], (Ipp16s)(wcb[l][(*alt_mpfade)[k][l] * vdim + j] >> 1));
            }
         }
         M_cbcod(*mpfade, dist[i], e, (*alt_mpfade)[k], i, wcb[i], cbsize[i],
            (i == stages - 1 ? 1 : m), &m_best, &m_dist, vdim);
      }
      pfadanz_max = (Ipp16s)(Mul2_Low_32s(pfadanz_max*cbsize[i]) >> 1);
   }
   for (j=0; j<stages; j++) {
      inds[j] = (*mpfade)[0][j];
   }

   for (j=0; j<vdim; j++) {
      y[j] = (Ipp16s)(wcb[0][inds[0]*vdim+j] >> 1);
      for (l=1; l<stages; l++) {
         y[j] = Add_16s(y[j], (Ipp16s)(wcb[l][inds[l]*vdim+j] >> 1));
      }
   }
}

static void Pmsvq2(Ipp16s *y, Ipp16s **prm, Ipp16s *x, Ipp16s *old_x,const sMSPQTab* filt_hi_pmsvq_fx )
{
   Ipp16s e[MAX_VECT_DIM];
   Ipp16s eq[MAX_VECT_DIM];
   Ipp32s Ltmp;
   Ipp32s i;
   const  Ipp16s *cbm = filt_hi_pmsvq_fx->mean;
   Ipp32s n = filt_hi_pmsvq_fx->vdim;
   Ipp16s *inds = *prm;

   *prm += filt_hi_pmsvq_fx->nstages;

   for (i = 0; i < n; i++) {
      Ltmp = Sub_32s(Mul2_Low_32s(x[i]*32767), Mul2_Low_32s(cbm[i]*16384));
      e[i] = Cnvrt_NR_32s16s(Sub_32s(Ltmp, Mul2_Low_32s(MSVQ_PERDICTCOEFF*old_x[i])));
   }

   Msvq2(eq, inds, e, filt_hi_pmsvq_fx);

   for (i=0; i<n; i++) {
      Ltmp = Add_32s(Mul2_Low_32s(eq[i]*32767), Mul2_Low_32s(MSVQ_PERDICTCOEFF*old_x[i]));
      old_x[i] = Cnvrt_NR_32s16s(Ltmp);
      y[i] = Cnvrt_NR_32s16s(Add_32s(Ltmp, Mul2_Low_32s(cbm[i]*16384)));
   }
}


void Smooth_ener_filter(Ipp16s filter[], Ipp16s *threshold)
{
  Ipp16s tmp;
  Ipp32s Lener;
  Ipp16s ener,old_ener;
  Ipp32s i;
  Ipp16s norm_tmp, norm_old_ener;
  Ipp32s Ltmp;

   Lener = 0;
   for (i=0; i<HI_FILT_ORDER; i++) {
      tmp = MulHR_16s(filter[i],16384);
      Lener = Add_32s(Lener, Mul2_Low_32s(tmp*tmp));
   }
   ener = Cnvrt_NR_32s16s(ShiftL_32s(Lener, 2+2));

   old_ener = ener;
   tmp = ener;

   if (tmp < *threshold) {
      tmp = ShiftL_16s((Ipp16s)((tmp*23170)>>15), 1);
      if (tmp > *threshold) {
         tmp = *threshold;
      }
   } else {
      tmp = (Ipp16s)((tmp*23170)>>15);
      if (tmp < *threshold) {
         tmp = *threshold;
      }
   }

   if (tmp == 0) {
      tmp = 1;
   }
   *threshold = tmp;

   norm_tmp = Exp_16s(tmp);
   tmp = ShiftL_16s(tmp,norm_tmp);
   norm_old_ener = (Ipp16s)(Exp_16s(old_ener) - 1);
   if (norm_old_ener > 0) {
      old_ener = ShiftL_16s(old_ener,norm_old_ener);
   } else {
      old_ener = (Ipp16s)(old_ener >> (-norm_old_ener));
   }
   tmp = Div_16s(old_ener,tmp);
   if (norm_tmp > (norm_old_ener-1)) {
      Ltmp = ShiftL_32s((Ipp32s)tmp, (Ipp16u)(norm_tmp-norm_old_ener+1));
   } else {
      Ltmp = ((Ipp32s)tmp) >> (-(norm_tmp-norm_old_ener+1));
   }
   Ltmp = _Isqrt(Ltmp);
   tmp = Cnvrt_NR_32s16s(ShiftL_32s(Ltmp,7));

   for (i=0; i<HI_FILT_ORDER; i++) {
      filter[i] = ShiftL_16s(MulHR_16s(tmp,filter[i]),1);
   }
   return;
}

void Quant_filt(Ipp16s* h, Ipp16s* old_h, Ipp16s **prm, const sMSPQTab* filt_hi_pmsvq)
{
   Pmsvq2(h, prm, h, old_h, filt_hi_pmsvq);
}

void Compute_gain_match(Ipp16s *gain_left, Ipp16s *gain_right, Ipp16s *x, Ipp16s *y, Ipp16s *buf)
{
   Ipp16s tmp16;
   Ipp32s t, j;
   Ipp32s Lenergy_right;
   Ipp32s Lenergy_right_q;
   Ipp32s Lenergy_left;
   Ipp32s Lenergy_mono, Ltmp;
   Ipp32s Lenergy_left_q;
   Ipp32s Lcorr_left_right;

   Ipp32s Lright;
   Ipp32s Lright_q;
   Ipp32s Lleft;
   Ipp32s Lmono;
   Ipp32s Left_q;
   Ipp32s Lcorr_lr;

   Ipp16s gain_fact;
   Ipp16s x_p_y;
   Ipp16s x_m_y;
   Ipp16s x_p_b;
   Ipp16s x_m_b;
   Ipp16s tmp,tmp_fraq, tmp_exp;

   Lenergy_left     = 1;
   Lenergy_right    = 1;

   Lenergy_left_q   = 1;
   Lenergy_right_q  = 1;
   Lenergy_mono     = 1;
   Lcorr_left_right = 1;

   for (t=0; t<(L_DIV+L_SUBFR); t+= L_SUBFR) {
      Lright      = 0;
      Lright_q    = 0;
      Lleft       = 0;
      Lmono       = 0;
      Left_q      = 0;
      Lcorr_lr    = 0;

      for(j=0; j<L_SUBFR; j++) {
         Ltmp  = Mul2_Low_32s(x[j+t]*8192);
         x_p_y = Cnvrt_NR_32s16s(Add_32s(Ltmp, Mul2_Low_32s(y[j+t]*8192)));
         x_m_y = Cnvrt_NR_32s16s(Sub_32s(Ltmp, Mul2_Low_32s(y[j+t]*8192)));
         x_p_b = Cnvrt_NR_32s16s(Add_32s(Ltmp, Mul2_Low_32s(buf[j+t]*8192)));
         x_m_b = Cnvrt_NR_32s16s(Sub_32s(Ltmp, Mul2_Low_32s(buf[j+t]*8192)));
         Lleft     = Add_32s(Lleft, Mul2_Low_32s(x_p_y*x_p_y));
         Left_q    = Add_32s(Left_q, Mul2_Low_32s(x_p_b*x_p_b));
         tmp16     = Cnvrt_NR_32s16s(Ltmp);
         Lmono     = Add_32s(Lmono, Mul2_Low_32s(tmp16*tmp16));
         Lright    = Add_32s(Lright, Mul2_Low_32s(x_m_y*x_m_y));
         Lright_q  = Add_32s(Lright_q, Mul2_Low_32s(x_m_b*x_m_b));
         Lcorr_lr  = Add_32s(Lcorr_lr, Mul2_Low_32s(x_m_y*x_m_y));
      }
      Lenergy_right    = Add_32s(Lenergy_right, (Lright>>1));
      Lenergy_left     = Add_32s(Lenergy_left, (Lleft>>1));
      Lenergy_mono     = Add_32s(Lenergy_mono, (Lmono>>1));
      Lenergy_right_q  = Add_32s(Lenergy_right_q, (Lright_q>>1));
      Lenergy_left_q   = Add_32s(Lenergy_left_q, (Left_q>>1));
      Lcorr_left_right = Add_32s(Lcorr_left_right, (Lcorr_lr>>1));
   }

   ownLog2_AMRWBE(Lenergy_left,&tmp_exp,&tmp_fraq);
   tmp = ShiftL_16s(tmp_exp,L2SCL);
   *gain_left = Add_16s(tmp, (Ipp16s)(tmp_fraq>>5));
   ownLog2_AMRWBE(Lenergy_left_q,&tmp_exp,&tmp_fraq);
   tmp = ShiftL_16s(tmp_exp,L2SCL);
   *gain_left = Sub_16s(*gain_left,Add_16s(tmp, (Ipp16s)(tmp_fraq>>5)));

   ownLog2_AMRWBE(Lenergy_right,&tmp_exp,&tmp_fraq);
   tmp = ShiftL_16s(tmp_exp,L2SCL);
   *gain_right = Add_16s(tmp, (Ipp16s)(tmp_fraq>>5));
   ownLog2_AMRWBE(Lenergy_right_q,&tmp_exp,&tmp_fraq);
   tmp = ShiftL_16s(tmp_exp,L2SCL);
   *gain_right = Sub_16s(*gain_right,Add_16s(tmp, (Ipp16s)(tmp_fraq>>5)));

   ownLog2_AMRWBE(Lenergy_mono,&tmp_exp,&tmp_fraq);
   tmp = ShiftL_16s(Add_16s(tmp_exp,2),L2SCL);
   gain_fact = Add_16s(tmp, (Ipp16s)(tmp_fraq>>5));
   ownLog2_AMRWBE(Add_32s(Lenergy_left,Lenergy_right),&tmp_exp,&tmp_fraq);
   tmp = ShiftL_16s(tmp_exp,L2SCL);
   gain_fact = Sub_16s(gain_fact,Add_16s(tmp, (Ipp16s)(tmp_fraq>>5)));

   if (gain_fact < 0) {
      if (*gain_right > 0) {
         *gain_right = Add_16s(*gain_right,gain_fact);
         if (*gain_right < 0) {
            *gain_right = 0;
         }
      }
      if (*gain_left > 0) {
         *gain_left = Add_16s(*gain_left,gain_fact);
         if (*gain_left < 0) {
            *gain_left = 0;
         }
      }
   }
   *gain_left = (Ipp16s)((ShiftL_16s(*gain_left,2) * 24717)>>15);
   *gain_right = (Ipp16s)((ShiftL_16s(*gain_right,2) * 24717)>>15);
}
void Quant_gain(Ipp16s gain_left, Ipp16s gain_right, Ipp16s *old_gain,
      Ipp16s **prm, const sMSPQTab* gain_hi_pmsvq)
{
   Ipp16s tmp[2];
   Ipp16s e[2];
   Ipp32s i, j, bestI;
   Ipp16s d;
   Ipp32s Ldist,Lbest, Ltmp;

   const Ipp16s *cbsize = gain_hi_pmsvq->cbsizes;
   const Ipp16s **cb = gain_hi_pmsvq->cbs;
   const Ipp16s *cbm = gain_hi_pmsvq->mean;
   const Ipp16s *y;

   Ipp16s *inds = *prm;


   tmp[0] = gain_left;
   tmp[1] = gain_right ;
   *prm += gain_hi_pmsvq->nstages;
   for (i=0;i<2;i++) {
      Ltmp = Mul2_Low_32s(Sub_16s(tmp[i], cbm[i]) * 32767);
      e[i] = Cnvrt_NR_32s16s(Sub_32s(Ltmp, Mul2_Low_32s(MSVQ_PERDICTCOEFF*old_gain[i])));
   }

   bestI = 0;
   Lbest = 0x7FFFFFFF;

   for (j = 0; j < cbsize[0]; j++) {
      y = &cb[0][j*2];
      Ldist = 0;
      for(i=0;i<2;i++) {
         d = Sub_16s(e[i], y[i]);
         Ldist = Add_32s(Ldist, Mul2_Low_32s(d*d));
      }
      if (Ldist < Lbest) {
         Lbest = Ldist;
         bestI = j;
      }
   }
   y = &cb[0][bestI*2];
   *inds = (Ipp16s)bestI;
   tmp[0] = y[0];
   tmp[1] = y[1];

   for(i=0; i<2; i++) {
      Ltmp = Mul2_Low_32s(tmp[i]*32767);
      Ltmp = Add_32s(Ltmp , Mul2_Low_32s(MSVQ_PERDICTCOEFF*old_gain[i]));
      old_gain[i] = Cnvrt_NR_32s16s(Ltmp);
      tmp[i] = Cnvrt_NR_32s16s(Add_32s(Ltmp, Mul2_Low_32s(cbm[i]*32767)));
   }
}

void Comp_gain_shap_cod(Ipp16s* wm, Ipp16s *gain_shap_cod, Ipp16s *gain_shap_dec,
      Ipp32s len, Ipp16s Qwm)
{
   Ipp32s i,j,k;
   Ipp16s tmp, expt, expt2, frac, log_sum, inv_lg16;
   Ipp32s L_tmp;

   log_sum = 0;

   if (len == 192) {
      inv_lg16 = 1365;
   } else if (len == 96) {
      inv_lg16 = 2731;
   } else {
      inv_lg16 = 5461;
   }

   j = 0;
   for(i=0; i<len; i+=16) {
      L_tmp = 1;
      for(k=0; k<16; k++) {
         tmp = (Ipp16s)(wm[i+k] >> 2);
         L_tmp = Add_32s(L_tmp, Mul2_Low_32s(tmp*tmp));
      }
      ownLog2_AMRWBE(L_tmp,&expt,&frac);
      expt = (Ipp16s)(expt - ((Qwm<<1) - 3));
      gain_shap_dec[j] = Add_16s(ShiftL_16s(expt,8), (Ipp16s)(frac>>7));
      log_sum = Add_16s(log_sum,gain_shap_dec[j]);
      gain_shap_dec[j] = MulHR_16s(gain_shap_dec[j],16384);
      j++;
   }
   log_sum = MulHR_16s(log_sum,inv_lg16);

   expt = (Ipp16s)(log_sum >> 8);
   frac = ShiftL_16s(Sub_16s(log_sum, ShiftL_16s(expt,8)), 7);

   ownPow2_AMRWB(expt,frac);
   log_sum = (Ipp16s)(ownPow2_AMRWB(30,frac)>>16);
   log_sum = Div_16s(16384,log_sum);
   j = 0;
   for(i=0; i<len; i+=16) {
      tmp = gain_shap_dec[j];
      expt2 = (Ipp16s)(tmp>>8);
      frac = ShiftL_16s(Sub_16s(tmp, ShiftL_16s(expt2,8)), 7);
      tmp = (Ipp16s)(ownPow2_AMRWB(30,frac)>>16);
      tmp = (Ipp16s)((tmp*log_sum)>>15);
      if (expt2 > expt) {
         gain_shap_dec[j] = ShiftL_16s(tmp, (Ipp16u)(expt2-expt));
         tmp = Div_16s(8192,tmp);
         gain_shap_cod[j] = (Ipp16s)(tmp >> (expt2-expt));
      } else {
         gain_shap_dec[j] = (Ipp16s)(tmp >> (expt-expt2));
         tmp = Div_16s(8192,tmp);
         gain_shap_cod[j] = ShiftL_16s(tmp, (Ipp16u)(expt-expt2));
      }

      if (gain_shap_dec[j] < 8192) {
         gain_shap_dec[j] = 8192;
      }
      j++;
   }
}

void Apply_gain_shap(Ipp32s len, Ipp16s* xnq_fx, Ipp16s* gain_shap_fx)
{
   Ipp32s i, k, is4;
   Ipp32s Ltmp;

   for(i=0; i<len; i+=16) {
      is4 = i>>4;
      for(k=0; k<16; k++) {
         Ltmp = Mul2_Low_32s(gain_shap_fx[is4]*xnq_fx[i+k]);
         Ltmp = ShiftL_32s(Ltmp,1);
         xnq_fx[i+k]= Cnvrt_NR_32s16s(Ltmp);
      }
  }
  return;
}

Ipp16s Q_gain_pan(Ipp16s *gain)
{
   Ipp16s index, tmp16;

   tmp16 = (Ipp16s)((*gain) >> 2);
   tmp16 = Add_16s(tmp16,8192);
   tmp16 = Add_16s(tmp16,64);
   index = (Ipp16s)(tmp16 >> 7);

   if (index > 127) {
      index = 127;
   }
   tmp16 = ShiftL_16s(index,8);
   *gain = ShiftL_16s(Sub_16s(tmp16,16384),1);
   return index;
}

Ipp16s Compute_xn_target(Ipp16s *xn, Ipp16s *wm, Ipp16s gain_pan, Ipp32s len)
{
  Ipp32s Ltmp,Lmax;
  Ipp32s Lxn[L_TCX_LB];
  Ipp16s tmp;
  Ipp32s i;

  Lmax = 1;
  for(i = 0; i<len; i++) {
     Lxn[i] = Sub_32s(Mul2_Low_32s(xn[i]*16384), Mul2_Low_32s(gain_pan*wm[i]));
     Ltmp = Abs_32s(Lxn[i]);
     if (Ltmp > Lmax) {
        Lmax = Ltmp;
     }
  }
  tmp = (Ipp16s)(Exp_32s(Lmax) - 1);
  if (tmp > 0) {
     for(i=0; i<len; i++) {
        xn[i] = Cnvrt_NR_32s16s(ShiftL_32s(Lxn[i],tmp));
     }
  } else {
     for(i=0; i<len; i++) {
        xn[i] = Cnvrt_NR_32s16s(Lxn[i] >> (-tmp));
     }
  }
  return (Ipp16s)(tmp-1);
}

