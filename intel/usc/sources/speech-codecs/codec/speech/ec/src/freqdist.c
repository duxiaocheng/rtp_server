/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the tems of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the tems of that agreement.
//          Copyright(c) 2006-2012 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives EC Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel(R) Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ippEULA.rtf or ippEULA.txt located in the root directory of your Intel(R) IPP
//  product installation for more information.
//
//
//  Purpose: NLP, NR
//
*/

#include <stdio.h>
#include <stdlib.h>
#include <ipps.h>
#include <ippsc.h>
#include "scratchmem.h"
#include "nlp_api.h"
#include "ec_api.h"
#include "ec_td.h"
#include "ownah.h"
#include "ah_api.h"
#include "ownec.h"
#include "math.h"
#define tapsFactor 14
#define COEFF_15 15
static void saveHist(Ipp32f *histSout,int histSoutLen,int *histSoutCurLen,
                     Ipp32f *sout,int len){
                         if(!histSoutLen) return;
                         if((*histSoutCurLen) < histSoutLen){
                             ippsCopy_32f(sout,histSout+(*histSoutCurLen),len);
                             (*histSoutCurLen)=((*histSoutCurLen)+len);
                             return;
                             }
                         if(histSoutLen>len){
                             ippsMove_32f(histSout+len,histSout,histSoutLen-len);
                             }
                         ippsCopy_32f(sout,histSout+histSoutLen-len,len);
                         return;
    }
static int nna(ahState *state,Ipp32f range,Ipp32f *sout,int len){
    ippsRandUniform_Direct_32f(sout,len,-range,range,&state->seed);
    return 1;
    }
static int na(ahState *state,Ipp32f range,Ipp32f *sout,int len){
    LOCAL_ALIGN_ARRAY(32,Ipp32f,pRnd,NLP_FRAME_SIZE,state);
    ippsRandUniform_Direct_32f(pRnd,len,-range,range,&state->seed);
    ippsAdd_32f(pRnd, sout, sout, len);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,pRnd,NLP_FRAME_SIZE,state);
    return 1;
    }
int cnr_32f(ahState *state,Ipp32f *rin,Ipp32f *sin,Ipp32f *sout,int nlp,int cng,int off){
    Ipp32f   sqrtNoisePwr=((11*11)>>5);
    //Ipp64f   longIn_pwr,nAttn,outNoisePwr;
    //Ipp64f   outNoisePwr;
    int      j;
    int sinNoiseFlag;
    IppsNrMode nMode;
    Ipp16s   soutcur[NLP_FRAME_SIZE];
    Ipp16s   soutt[NLP_FRAME_SIZE];
    Ipp16s   hrin[NLP_FRAME_SIZE];
    Ipp16s   hsin[NLP_FRAME_SIZE];
    for (j=0;j<NLP_FRAME_SIZE;++j) {
        soutcur[j]=(Ipp16s)sout[j];
        }
    saveHist(state->histSin,state->histLen,&state->histSinCurLen,sin,NLP_FRAME_SIZE);
    saveHist(state->histRin,state->histLen,&state->histRinCurLen,rin,NLP_FRAME_SIZE);
    if ( off ) {
        //saveHist(state->histSout,state->histSoutLen,&state->histSoutCurLen,sout,NLP_FRAME_SIZE);
        // if ( state->histSoutCurLen-off>=NLP_FRAME_SIZE) {
        ippsFilterNoiseDetectModerate_EC_32f64f(state->histSin+off,state->histRin+off,&state->sinNoisePwr,&state->dcOffset,&sinNoiseFlag,state->nrSin);
        nMode=sinNoiseFlag?ippsNrUpdateAll:ippsNrUpdate;
        //ippsFilterNoise_EC_32f(state->histSout+off,sout,nMode,state->nr);
        ippsFilterNoise_EC_32f_I(sout,nMode,state->nr);
        // }
        }else{
            ippsFilterNoiseDetectModerate_EC_32f64f(sin,rin,&state->sinNoisePwr,&state->dcOffset,&sinNoiseFlag,state->nrSin);
            nMode=sinNoiseFlag?ippsNrUpdateAll:ippsNrUpdate;
            ippsFilterNoise_EC_32f_I(sout,nMode,state->nr);
        }
    //if (state->nr->trueNoise==1) {
    //    ippsZero_32f(sout,len);
    //}
    //if (sinNoiseFlag) {
    //    ippsDotProd_32f64f(sin,sin,len,
    //        &outNoisePwr);outNoisePwr/=len;
    //    ippsDotProd_32f64f(sout,sout,len,
    //        &longIn_pwr);longIn_pwr/=len;
    //    nAttn=10*log10(outNoisePwr/(longIn_pwr+0.1f));
    //    if (nAttn>0) {
    //        state->nAttn=0.9*state->nAttn+0.1*nAttn;
    //    }
    //    ++state->nCatches;
    //}
    //nAttn=10*log10(0.1);
    //if (sinNoiseFlag) {
    //    ippsDotProd_32f64f(sout,sout,NLP_FRAME_SIZE,&outNoisePwr);
    //    state->nAttn=(Ipp32f)(outNoisePwr+1);
    //}
    if ((nlp==1) || (nlp>2)) {
        if (state->initHowl>0) {
            if (cng==1)
                nna(state,sqrtNoisePwr,sout,NLP_FRAME_SIZE);
            else
                ippsZero_32f(sout, NLP_FRAME_SIZE);
            state->initHowl-=NLP_FRAME_SIZE;
            }else{
                //if (off){//initHowl>off
                //    for (j=0;j<NLP_FRAME_SIZE;++j) {
                //        soutcur[j]=(Ipp16s)state->histSout[off+j];
                //        soutt[j]=(Ipp16s)sout[j];
                //        hrin[j]=(Ipp16s)state->histRin[j];
                //        hsin[j]=(Ipp16s)state->histSin[j];
                //    }
                //    soutProc(&state->nlpObj,soutcur,soutt,hrin,hsin,coeff,(Ipp16s)state->sinNoisePwr);
                //    for (j=0;j<NLP_FRAME_SIZE;++j) {
                //        sout[j]=(Ipp32f)soutt[j];
                //    }
                //}else{
                for (j=0;j<NLP_FRAME_SIZE;++j) {
                    soutt[j]=(Ipp16s)sout[j];
                    hrin[j]=(Ipp16s)state->histRin[j+SBF_OFF-off];
                    hsin[j]=(Ipp16s)state->histSin[j+SBF_OFF-off];
                    }
                for (j=0;j<state->nNLP;++j)
                    soutProc(&state->nlpObj[j],soutcur,soutt,hrin,hsin,(Ipp16s)state->sinNoisePwr);
                for (j=0;j<NLP_FRAME_SIZE;++j) {
                    sout[j]=(Ipp32f)soutt[j];
                }
                //}
            }
        }
    if (cng==1) {
        na(state,sqrtNoisePwr,sout,NLP_FRAME_SIZE);
        }
    return 0;
    }
