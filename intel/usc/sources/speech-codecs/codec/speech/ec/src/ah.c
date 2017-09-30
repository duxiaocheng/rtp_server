/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
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
//  Purpose: anti-howler, tested for subband algorithm
//
*/

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
#include "aux_fnxs.h"
//#define _USC_ARCH_XSC
static Ipp32f bqTaps2[]={
    0.068460f,0.13692f,0.06846f,/* b00 b01 b02 */
    1.f,-1.1500138f,0.42438f,   /* a00 a01 a02 */

    0.98531f,-1.9706f,0.06846f, /* b10 b11 b12 */
    1.f,-1.1500138f,0.42438f    /* a10 a11 a12 */

    };
Ipp32s ah_SetMode(_ahState *state, AHOpcode op){
    if((op==AH_ENABLE1) || (op==AH_ENABLE2) || (op==AH_DISABLE)){
        state->mode=op;
        return 0;
        } else
            return 1;
    }

static Ipp32f ms_en(Ipp32f *src, Ipp32s len)
    {
    Ipp64f en;
    ippsDotProd_32f64f(src,src,len,&en);
    return (Ipp32f)(en/len);
    }
static Ipp32s detectHowlingIIR(_ahState *state, Ipp32f *in, Ipp32f rms,int frameSize){
    Ipp32f arms;
    Ipp32s dstLen,phase=0,dstLen2,ret=0;
    LOCAL_ALIGN_ARRAY(32,Ipp32f,out,FRAME_SIZE_10ms,state);
    ippsSampleUp_32f(in,frameSize,state->upDownBuf,&dstLen,4,&phase);
    ippsIIR_32f(state->upDownBuf,state->upDownBuf,dstLen,state->bq2StateTest);
    ippsSampleDown_32f(state->upDownBuf,dstLen,out,&dstLen2,4,&phase);
    arms = ms_en(out,frameSize);
    if ((rms*100>arms*120)){
        ippsIIRInit_BiQuad_32f(&state->bq2StateTest,bqTaps2,2,0,
            (Ipp8u *)state->bq2StateTest);
        ret=2; /* howling started */
        }
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,out,FRAME_SIZE_10ms,state);
    return ret;
    }
#define BW_AMPL_PRECISION 2
//static Ipp32f mean_32f(const Ipp32f *src, Ipp32s len){
//    Ipp64f sum=0;
//    Ipp32s i;
//    for (i=0;i<len;i++) {
//        sum+=src[i];
//    }
//    return (Ipp32f)(sum/len);
//}
static Ipp32s detectHowlingFFT(_ahState *state,Ipp32f *in){
    Ipp32f   cmean;
    Ipp32s   per[3]={11,22,33},ret=0;
    Ipp32s   max_idx[3]={0,0,0};
    Ipp32s   i, flen=NOISE_SUBBANDS_NUM,disp[3];
    LOCAL_ALIGN_ARRAY(32,Ipp32f,curMagn,NOISE_SUBBANDS_NUM,state);
    LOCAL_ALIGN_ARRAY(32,Ipp32f,inx,(((1<<FFT_ORDER6)+2)),state);
    LOCAL_ALIGN_ARRAY(32,Ipp32fc,spectrum,ORDER10_SPECTR_LEN,state);
    LOCAL_ALIGN_ARRAY(32,Ipp8u,pBuf,state->fftWorkBufSize,state);
    ippsFFTFwd_RToCCS_32f((const Ipp32f *)in,(Ipp32f *)spectrum,state->FFTspec6,pBuf);
    ippsMagnitude_32fc(spectrum,curMagn,flen);
    ippsMean_32f(curMagn,flen,&cmean,ippAlgHintNone);
    if(fabs(cmean)<1.e-5f)
        return 0;
    for (disp[0]=i=0;i<per[0];i++) {
        if (curMagn[i]>cmean) {
            disp[0]++;
            max_idx[0]=i;
            }
        }
    for (disp[1]=0;i<per[1];i++) {
        if (curMagn[i]>cmean) {
            disp[1]++;
            max_idx[1]=i;
            }
        }
    for (disp[2]=0;i<per[2];i++) {
        if (curMagn[i]>cmean) {
            disp[2]++;
            max_idx[2]=i;
            }
        }
    if(
        (
        ((disp[0]-disp[1])>BW_AMPL_PRECISION &&
        (disp[2]-disp[1])>BW_AMPL_PRECISION) &&
        (max_idx[0]>MAX_HOWL_IDX0 &&
        max_idx[2]>MAX_HOWL_IDX2 ))
        ){
            ret= 2;
        }
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp8u,pBuf,state->fftWorkBufSize,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32fc,spectrum,ORDER10_SPECTR_LEN,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,inx,(((1<<FFT_ORDER6)+2)),state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,curMagn,NOISE_SUBBANDS_NUM,state);
    return ret;
    }
static void shiftCutFreq(_ahState *state,Ipp32fc *src, Ipp32s len){
    Ipp32s shift,i;
    Ipp32fc val={1,1};
    ippsSet_32fc(val,src,state->howlLowBand);
    src[0].im=0;
    shift = ((state->shift<<dFScale)+(state->dF-1))/state->dF;
    if(shift>0){
        i=len-state->howlHighBand;
        shift=shift>i?i:shift;
        ippsCopy_32fc(&src[state->howlLowBand+shift],&src[state->howlLowBand],
            state->howlHighBand-state->howlLowBand);
        }
    ippsSet_32fc(val,&src[state->howlHighBand],len-state->howlHighBand);
    src[len-1].im=0;
    return;
    }
static void filterHowlingFFT(_ahState *state, Ipp32f *out,int frameSize){
    LOCAL_ALIGN_ARRAY(32,Ipp32fc,spectrum,(ORDER10_SPECTR_LEN),state);
    LOCAL_ALIGN_ARRAY(32,Ipp8u,pBuf,state->fftWorkBufSize,state);
    LOCAL_ALIGN_ARRAY(32,Ipp32f,sBuf,FFT_ORDER10_SRC_LEN,state);
    ippsFFTFwd_RToCCS_32f((const Ipp32f *)state->histSin,
        (Ipp32f *)spectrum, state->FFTspec10,pBuf);
    shiftCutFreq(state,spectrum,ORDER10_SPECTR_LEN);
    ippsFFTInv_CCSToR_32f((Ipp32f *)spectrum,sBuf, state->FFTspec10,pBuf);
    ippsCopy_32f(sBuf+FFT_ORDER10_SRC_LEN-frameSize,out,frameSize);
    //ippsMulC_32f_I(12.f,out,frameSize);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,sBuf,FFT_ORDER10_SRC_LEN,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp8u,pBuf,state->fftWorkBufSize,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32fc,spectrum,(ORDER10_SPECTR_LEN),state);
    }
static void initFilter(_ahState *state){
    ippsIIRInit_BiQuad_32f(&state->bq2State,bqTaps2,2,0,
        (Ipp8u *)state->bq2State);
    }
static void filterHowlingIIR(_ahState *state, Ipp32f *in, Ipp32f *out,int frameSize){
    Ipp32s dstLen,phase=0,dstLen2;
    ippsSampleUp_32f(in,frameSize,state->upDownBuf,&dstLen,4,&phase);
    ippsIIR_32f(state->upDownBuf,state->upDownBuf,dstLen,state->bq2State);
    ippsSampleDown_32f(state->upDownBuf,dstLen,out,&dstLen2,4,&phase);
    //ippsMulC_32f_I(12.f,out,state->frameSize);
    }
AHOpcode ah_ProcessFrame(_ahState *state, Ipp32f *in, Ipp32f *out,int frameSize)
    {
    Ipp32s  detect;
    Ipp32f  rms;
    if(state->mode==AH_ENABLE1 || state->mode==AH_ENABLE2){
        if(state->histSinCurLen<FFT_ORDER6_SRC_LEN)/* too small history */
            return AH_DISABLE;
        rms = ms_en(state->histSin-FFT_ORDER6_SRC_LEN,FFT_ORDER6_SRC_LEN);
        if (rms<111){
            state->howlPeriod=0;
            return AH_DISABLE;
            }
        if (state->howlPeriod==0) {
            if(state->mode==AH_ENABLE1){
                detect=detectHowlingFFT(state,state->histSin-FFT_ORDER6_SRC_LEN);
                }else
                    detect=detectHowlingIIR(state,state->histSin-FFT_ORDER6_SRC_LEN,rms, frameSize);
                if(detect<=0)
                    return AH_DISABLE;
                if(detect==2){ /* howling started */
                    state->howlPeriod=state->howlPeriodInit;
                    initFilter(state);
                    }
            }
        --state->howlPeriod;
        if(state->prevHistExist){// full history
            filterHowlingFFT(state,out, frameSize);
            filterHowlingIIR(state,out,out, frameSize);
            }else
                filterHowlingIIR(state,in,out, frameSize);
            //ippsMulC_16s_I(12.f,out,state->frameSize);
            return state->mode;
        }
    return AH_DISABLE;
    }
Ipp32s ah_GetSize(Ipp32s frameSize, Ipp32s *size,int freq,int *scratchMemSize)
    {
    int iirSize=0;
    int specSize,specSize1,specSize2,bufSize,bufSize1,bufSize2,initSize,initSize1,
        bufSize3, specSize3, initSize3,TDsize,
        initSize2,gsize,hsize,histSoutLen,nrSize;//histSoutSLen,
    int sbStackLen,fbStackLen,sbfStackLen,ahStackLen1,ahStackLen2,ahStackLen3,
        scrLen,fdStackLen;
    ippsIIRGetStateSize_BiQuad_32f(2,&bufSize);
    iirSize=ALIGN(bufSize);
    iirSize+=ALIGN(bufSize);
    *size = ALIGN(frameSize*4*sizeof(Ipp32f)) +/* up buf size */
        iirSize;
    //switch(frameSize) {
    //case SB_FRAME_SIZE:
    //    hsize = histSoutLen = 0;//SB_FRAME_SIZE*2*sizeof(Ipp32f)
    //    break;
    //case FB_FRAME_SIZE:
    //    hsize = histSoutLen = 0;//SB_FRAME_SIZE*2*sizeof(Ipp32f)
    //    break;
    //case SBF_FRAME_SIZE:
    //    hsize = histSoutLen = (SBF_OFF+NLP_FRAME_SIZE)*sizeof(Ipp32f);//FRAME_SIZE_10ms*2*sizeof(Ipp32f)
    //    break;
    //default:
    //    return -1;
    //}
    hsize = (SBF_OFF*2+NLP_FRAME_SIZE)*sizeof(Ipp32f);//FRAME_SIZE_10ms*2*sizeof(Ipp32f)
    histSoutLen = (SBF_OFF+NLP_FRAME_SIZE)*sizeof(Ipp32f);//FRAME_SIZE_10ms*2*sizeof(Ipp32f)
    //  histSoutSLen  = FFT_ORDER10_SRC_LEN*sizeof(Ipp32f);//histLen
    ippsFFTGetSize_R_32f(FFT_ORDER10, IPP_FFT_DIV_INV_BY_N, ippAlgHintAccurate,
        &specSize, &initSize, &bufSize);
    ippsFFTGetSize_R_32f(FFT_ORDER6, IPP_FFT_DIV_INV_BY_N, ippAlgHintAccurate,
        &specSize1, &initSize1, &bufSize1);
    ippsFFTGetSize_R_32f(FFT_ORDER7, IPP_FFT_DIV_INV_BY_N, ippAlgHintAccurate,
        &specSize2, &initSize2, &bufSize2);
    ippsFFTGetSize_R_32f(FFT_ORDER11, IPP_FFT_DIV_INV_BY_N, ippAlgHintAccurate,
        &specSize3, &initSize3, &bufSize3);
    gsize = initSize;
    if (gsize < bufSize)
        gsize = bufSize;
    if (gsize < initSize1)
        gsize = initSize1;
    if (gsize < bufSize1)
        gsize = bufSize1;
    if (gsize < initSize2)
        gsize = initSize2;
    if (gsize < bufSize2)
        gsize = bufSize2;
    if (gsize < initSize3)
        gsize = initSize3;
    if (gsize < bufSize3)
        gsize = bufSize3;
    if (gsize<(int)(frameSize*4*sizeof(Ipp32f))) {// reserve for updown2 buf
        gsize=(int)(frameSize*4*sizeof(Ipp32f));
        }
    sbfStackLen=
        ALIGN(sizeof(Ipp32fc)*SBF_SUBBANDS)+
        ALIGN(sizeof(Ipp32fc)*SBF_SUBBANDS)+
        ALIGN(sizeof(Ipp32fc)*SBF_SUBBANDS)+
        ALIGN(sizeof(Ipp32fc)*SBF_SUBBANDS)+
        ALIGN(sizeof(Ipp32fc)*SBF_SUBBANDS)+
        ALIGN(sizeof(Ipp32fc)*SBF_SUBBANDS)+
        ALIGN(sizeof(Ipp64f)*SBF_SUBBANDS);

    fbStackLen=
        ALIGN(sizeof(Ipp32f)*FB_FRAME_SIZE_MAX)+
        ALIGN(sizeof(Ipp32f)*FB_FRAME_SIZE_MAX);

    sbStackLen=
        ALIGN(sizeof(Ipp32fc)*SB_SUBBANDS)+
        ALIGN(sizeof(Ipp32fc)*SB_SUBBANDS)+
        ALIGN(sizeof(Ipp32fc)*SB_SUBBANDS)+
        ALIGN(sizeof(Ipp32fc)*SB_SUBBANDS)+
        ALIGN(sizeof(Ipp64f)*SB_SUBBANDS*2)+
        ALIGN(sizeof(Ipp32f)*SB_FFT_LEN)+
        ALIGN(sizeof(Ipp32f)*SB_FFT_LEN)+
        ALIGN(sizeof(Ipp32f)*SB_SUBBANDS * 4)+
        ALIGN(sizeof(Ipp32f)*SB_SUBBANDS * 4)+
        ALIGN(sizeof(Ipp32f)*SB_FFT_LEN
        );
    ahStackLen1=
        ALIGN(sizeof(Ipp32fc)*ORDER11_SPECTR_LEN)+
        ALIGN(sizeof(Ipp8u)*gsize)+
        ALIGN(sizeof(Ipp32f)*FFT_ORDER11_SRC_LEN
        );
    ahStackLen2=
        ALIGN(sizeof(Ipp32f)*NOISE_SUBBANDS_NUM)+
        ALIGN(sizeof(Ipp32f)*(((1<<FFT_ORDER6))+2))+
        ALIGN(sizeof(Ipp32fc)*(ORDER10_SPECTR_LEN))+
        ALIGN(sizeof(Ipp8u)*gsize);
    ahStackLen3=
        ALIGN(sizeof(Ipp32fc)*ORDER10_SPECTR_LEN)+
        ALIGN(sizeof(Ipp8u)*gsize)+
        ALIGN(sizeof(Ipp32f)*FFT_ORDER10_SRC_LEN);

    fdStackLen=
        ALIGN(sizeof(Ipp32fc)*AEC_FFT_ORDER7_LEN)+
        ALIGN(sizeof(Ipp8u)*gsize)+
        ALIGN(sizeof(Ipp64f)*AEC_FFT_ORDER7_LEN);

    scrLen=ahStackLen1;
    if (ahStackLen2>scrLen)
        scrLen=ahStackLen2;
    if (ahStackLen3>scrLen)
        scrLen=ahStackLen3;
    if (sbfStackLen>scrLen)
        scrLen=sbfStackLen;
    if (fbStackLen>scrLen)
        scrLen=fbStackLen;
    if (sbStackLen>scrLen)
        scrLen=sbStackLen;
    if (fdStackLen>scrLen)
        scrLen=sbStackLen;
    *scratchMemSize=scrLen;

    ippsToneDetectGetStateSize_EC_32f((IppPCMFrequency)freq, &TDsize);// freq don't used
    ippsFilterNoiseGetStateSize_EC_32f((IppPCMFrequency)freq,&nrSize);
    *size += ALIGN(sizeof(ahState))+
        ALIGN(specSize)+
        ALIGN(specSize1)+
        ALIGN(specSize2)+
        ALIGN(specSize3)+
        ALIGN(hsize)+//histSin
        ALIGN(histSoutLen)+//histSout
        //ALIGN(histSoutSLen)+//histSoutS
        ALIGN(TDsize)+
        ALIGN(TDsize)+
        ALIGN(hsize)+//histRin
        ALIGN(nrSize)+
        ALIGN(nrSize);
    return 0;//42112==a480
    }

Ipp32s ah_Init(_ahState *state,Ipp32s samplingRate,Ipp32s frameSize,Ipp32s coef)
    {
    int bufSize, specSize, initSize,bufSize1, specSize1, initSize1,
        bufSize2, specSize2, initSize2,
        bufSize3, specSize3, initSize3,
        gsize, hlen,nrSize,j;//,hSoutSLen
    IppStatus status=ippStsNoErr;
    Ipp8u *pInit0,*pInit = (Ipp8u *)state,*pInit1,*pInit2,*pInit3;//;
    pInit += ALIGN(sizeof(_ahState));//0x9c0080+a480=9ca500

    state->bq2State = (IppsIIRState_32f *)pInit;
    ippsIIRGetStateSize_BiQuad_32f(2,&bufSize);
    pInit += ALIGN(bufSize);
    state->bq2StateTest = (IppsIIRState_32f *)pInit;
    ippsIIRGetStateSize_BiQuad_32f(2,&bufSize);
    pInit += ALIGN(bufSize);
    ippsIIRInit_BiQuad_32f(&state->bq2State,bqTaps2,2,0,
        (Ipp8u *)state->bq2State);
    ippsIIRInit_BiQuad_32f(&state->bq2StateTest,bqTaps2,2,0,
        (Ipp8u *)state->bq2StateTest);

    ippsZero_32f(state->pDly2Test,6*2);
    ippsZero_32f(state->pDly1Test,6*1);
    ippsZero_32f(state->pFDdly2,6*2);
    ippsZero_32f(state->iirDly,6);
    ippsZero_32f(state->iirDly2,6);
    ippsZero_32f(state->pDly4Test,6*4);
    for (j=0;j<AEC_NLP_OBJ_NUM;++j)
        initNLP(&state->nlpObj[j],11,11,15,12,30,coef);
    state->wbNoisePwr=16;
    state->rinNoisePwr=state->wbNoisePwr;
    //switch(frameSize) {
    //case SB_FRAME_SIZE:
    //    state->histLen = state->histSoutLen = 0;//SB_FRAME_SIZE*2*sizeof(Ipp32f)
    //    break;
    //case FB_FRAME_SIZE:
    //    state->histLen = state->histSoutLen = 0;//SB_FRAME_SIZE*2*sizeof(Ipp32f)
    //    break;
    //case SBF_FRAME_SIZE:
    //    break;
    //default:
    //    return -1;
    //}
    state->histLen = (SBF_OFF*2+NLP_FRAME_SIZE);//FRAME_SIZE_10ms*2*sizeof(Ipp32f)
    state->histSoutLen = (SBF_OFF+NLP_FRAME_SIZE);//FRAME_SIZE_10ms*2*sizeof(Ipp32f)
    state->histSinCurLen = (SBF_OFF*2+NLP_FRAME_SIZE);
    state->histRinCurLen = (SBF_OFF*2+NLP_FRAME_SIZE);
    state->histSoutCurLen = (SBF_OFF+NLP_FRAME_SIZE);
    //state->histSoutSLen = FFT_ORDER10_SRC_LEN;
    hlen = ALIGN(state->histLen*sizeof(Ipp32f));
    //hSoutLen = ALIGN(state->histSoutLen*sizeof(Ipp32f));
    //hSoutSLen = ALIGN(state->histSoutSLen*sizeof(Ipp32f));
    state->upDownBuf = (Ipp32f *)pInit;
    pInit += ALIGN(frameSize*4*sizeof(Ipp32f));
    ippsFFTGetSize_R_32f(FFT_ORDER10, IPP_FFT_DIV_INV_BY_N, ippAlgHintAccurate,
        &specSize, &initSize, &bufSize);
    ippsFFTGetSize_R_32f(FFT_ORDER6, IPP_FFT_DIV_INV_BY_N, ippAlgHintAccurate,
        &specSize1, &initSize1, &bufSize1);
    ippsFFTGetSize_R_32f(FFT_ORDER7, IPP_FFT_DIV_INV_BY_N, ippAlgHintAccurate,
        &specSize2, &initSize2, &bufSize2);
    ippsFFTGetSize_R_32f(FFT_ORDER11, IPP_FFT_DIV_INV_BY_N, ippAlgHintAccurate,
        &specSize3, &initSize3, &bufSize3);
    pInit0 = pInit;
    pInit += ALIGN(specSize);
    pInit1 = pInit;
    pInit += ALIGN(specSize1);
    pInit2 = pInit;
    pInit += ALIGN(specSize2);
    pInit3 = pInit;
    pInit += ALIGN(specSize3);
    ippsFFTInit_R_32f(&(state->FFTspec10), FFT_ORDER10, IPP_FFT_DIV_INV_BY_N,
        ippAlgHintAccurate, pInit0, pInit);
    ippsFFTInit_R_32f(&(state->FFTspec6), FFT_ORDER6, IPP_FFT_DIV_INV_BY_N,
        ippAlgHintAccurate, pInit1, pInit);
    ippsFFTInit_R_32f(&(state->FFTspec7), FFT_ORDER7, IPP_FFT_DIV_INV_BY_N,
        ippAlgHintAccurate, pInit2, pInit);
    ippsFFTInit_R_32f(&(state->FFTspec11), FFT_ORDER11, IPP_FFT_DIV_INV_BY_N,
        ippAlgHintAccurate, pInit3, pInit);
    gsize = initSize;
    if (gsize < bufSize)
        gsize = bufSize;
    if (gsize < initSize1)
        gsize = initSize1;
    if (gsize < bufSize1)
        gsize = bufSize1;
    if (gsize < initSize2)
        gsize = initSize2;
    if (gsize < bufSize2)
        gsize = bufSize2;
    if (gsize < initSize3)
        gsize = initSize3;
    if (gsize < bufSize3)
        gsize = bufSize3;
    if (gsize<(int)(frameSize*4*sizeof(Ipp32f))) {// reserve for updown2 buf
        gsize=(int)(frameSize*4*sizeof(Ipp32f));
        }
    state->fftWorkBufSize=(Ipp16s)gsize;
    /* Initialize tone detection  */
    ippsToneDetectGetStateSize_EC_32f((IppPCMFrequency)samplingRate, &gsize);

    state->td.tdr = (IppsToneDetectState_EC_32f *)pInit;
    pInit += ALIGN(gsize);
    state->td.tds = (IppsToneDetectState_EC_32f *)pInit;
    pInit += ALIGN(gsize);
    ippsToneDetectInit_EC_32f(state->td.tdr, (IppPCMFrequency)samplingRate);
    ippsToneDetectInit_EC_32f(state->td.tds, (IppPCMFrequency)samplingRate);
    state->td.td_thres = (Ipp64f)(IPP_AEC_dBtbl[35-10]*IPP_AEC_dBtbl[35-10]);
    state->td.td_resr = state->td.td_ress = 0;
    state->td.td_sil_time=0;
    state->td.td_sil_thresh=250;
    ippsFilterNoiseGetStateSize_EC_32f((IppPCMFrequency)samplingRate,&nrSize);
    state->nr = (IppsFilterNoiseState_EC_32f *)pInit;
    pInit += ALIGN(nrSize);
    state->nrSin = (IppsFilterNoiseState_EC_32f *)pInit;
    pInit += ALIGN(nrSize);
    state->histRin=(Ipp32f *)pInit;
    pInit += hlen;
    state->histSin=(Ipp32f *)pInit;
    pInit += hlen;
    state->histSout=(Ipp32f *)pInit;
    //pInit += hSoutLen;
    //state->histSoutS=(Ipp32f *)pInit;
    //pInit += hSoutSLen;
    state->samplingRate = (Ipp16s)samplingRate;
    state->shift = 0;
    state->mode = AH_DISABLE;
    //state->histSoutSCurLen = 0;
    state->howlPeriodInit=(int)((state->samplingRate/frameSize));/* number of
                                                                 frames to be filtered after the howling has been detected*/
    state->howlPeriod=0;
    state->prevHistExist=0;
    state->dF=((samplingRate<<dFScale)>>FFT_ORDER10);    // fft spectrum freq step
    //dF7=((samplingRate*10)>>FFT_ORDER7);    // fft spectrum freq step
    state->howlLowBand=(LOW_BAND<<dFScale)/state->dF;
    state->howlHighBand=((HIGH_BAND<<dFScale)/state->dF);
    state->hd_mode=HD_1;
    //state->nAttn=0;
    state->nCatches=0;

    state->seed=1;
    state->dtFlag=0;

    // state->coefc = COEF_ONE - COEF_ONE * frameSize  * 20 / samplingRate;
    state->so_pwr=0;
    state->sol_pwr=0;
    state->sl_pwr=0;
    state->nsi_pwr=0;
    state->sqrtNoisePwr=(Ipp32f)sqrt(state->wbNoisePwr);
    ippsZero_32f(state->histRin,state->histLen);
    ippsZero_32f(state->histSin,state->histLen);
    ippsZero_32f(state->histSout,state->histSoutLen);
    //    ippsZero_32f(state->histSoutS,FFT_ORDER10_SRC_LEN);
    ippsZero_64f(state->noiseCurMagn,AEC_FFT_ORDER7_LEN);

    state->noiseMax=0;
    ippsFilterNoiseInit_EC_32f((IppPCMFrequency)samplingRate,state->nr);
    ippsFilterNoiseInit_EC_32f((IppPCMFrequency)samplingRate,state->nrSin);
    state->initHowl=(Ipp16s)samplingRate*2/5;
    state->sinNoisePwr=state->rinNoisePwr=0;
    state->frameNum=0;
    ippsFilterNoiseLevel_EC_32f(ippsNrHigh, state->nr);
    ippsFilterNoiseLevel_EC_32f(ippsNrHigh, state->nrSin);
    state->nrMode=ippsNrHigh;
    state->nNLP=AEC_NLP_OBJ_NUM;
    state->dcOffset=0;
    return status;
}

static Ipp32s ibq2Cfs[]= {
    73508365,   147016731,    73508365, /* b00 b01 b02 */
    1073741824, -1234817915,   455674555, /* a00 a01 a02 */

    1057968557, -2115915638,    73508365, /* b10 b11 b12 */
    1073741824, -1234817915,   455674555, /* a10 a11 a12 */
    };
#if AH_FIR_TAPS == 64
static Ipp32s firTaps[]= {
    -17988,   33588858,  -20872581,  -21977277,
    25997889,    3148237,   -2514792,  -17906252,
    -5048429,   48128369,  -27564475,  -38187884,
    47303481,    1662761,  -13100879,  -13866776,
    -13185841,   72239313,  -36112350,  -75948528,
    93006755,    6090086,  -48016687,   -2296032,
    -29080901,  140435448,  -60474300, -239274319,
    321014236,   70193130, -467466716,  281372249,
    281372249, -467466716,   70193130,  321014236,
    -239274319,  -60474300,  140435448,  -29080901,
    -2296032,  -48016687,    6090086,   93006755,
    -75948528,  -36112350,   72239313,  -13185841,
    -13866776,  -13100879,    1662761,   47303481,
    -38187884,  -27564475,   48128369,   -5048429,
    -17906252,   -2514792,    3148237,   25997889,
    -21977277,  -20872581,   33588858,     -17988
    };
#elif (AH_FIR_TAPS == 128)
static Ipp32s firTaps[]= {
    1756015,    -997170,    1316744,  -14197986,
    -133828,    6184919,   -5599905,   -1730733,
    1987130,     335179,    3289091,   -6726224,
    -1408823,   11531957,   -6516370,   -6549650,
    7733016,    -232276,    1035938,   -4618104,
    -5224528,   16270553,   -4830032,  -16269422,
    14841711,    3506423,   -7425106,    -258235,
    -6535353,   16515651,    1491810,  -28852737,
    19156716,   15920438,  -22816747,    1860083,
    880890,    9804774,    9911257,  -39465116,
    15801814,   40169762,  -42786732,   -7154181,
    25044512,   -2498970,   12078017,  -41617745,
    1318520,   79595934,  -63625399,  -45050762,
    82099995,  -12154627,  -14480067,  -27492113,
    -27846267,  163497858,  -93756612, -217661896,
    326750225,   40517835, -434421764,  267279911,
    267279911, -434421764,   40517835,  326750225,
    -217661896,  -93756612,  163497858,  -27846267,
    -27492113,  -14480067,  -12154627,   82099995,
    -45050762,  -63625399,   79595934,    1318520,
    -41617745,   12078017,   -2498970,   25044512,
    -7154181,  -42786732,   40169762,   15801814,
    -39465116,    9911257,    9804774,     880890,
    1860083,  -22816747,   15920438,   19156716,
    -28852737,    1491810,   16515651,   -6535353,
    -258235,   -7425106,    3506423,   14841711,
    -16269422,   -4830032,   16270553,   -5224528,
    -4618104,    1035938,    -232276,    7733016,
    -6549650,   -6516370,   11531957,   -1408823,
    -6726224,    3289091,     335179,    1987130,
    -1730733,   -5599905,    6184919,    -133828,
    -14197986,    1316744,    -997170,    1756015
    };
#endif

//#define HZ4_SHIFT
#define HZ8_SHIFT

#ifdef HZ4_SHIFT
#define NCOS 2000
static short shCos[NCOS+1] = {
    16384,    13, -16384,   -39, 16384,    64, -16384,   -90,
    16384,   116, -16383,  -142, 16383,   167, -16383,  -193,
    16383,   219, -16382,  -244, 16382,   270, -16382,  -296,
    16381,   322, -16381,  -347, 16380,   373, -16379,  -399,
    16379,   425, -16378,  -450, 16377,   476, -16377,  -502,
    16376,   527, -16375,  -553, 16374,   579, -16373,  -605,
    16372,   630, -16371,  -656, 16370,   682, -16369,  -708,
    16368,   733, -16367,  -759, 16366,   785, -16365,  -810,
    16363,   836, -16362,  -862, 16361,   887, -16359,  -913,
    16358,   939, -16356,  -965, 16355,   990, -16353, -1016,
    16352,  1042, -16350, -1067, 16348,  1093, -16347, -1119,
    16345,  1144, -16343, -1170, 16341,  1196, -16339, -1221,
    16337,  1247, -16335, -1273, 16333,  1298, -16331, -1324,
    16329,  1350, -16327, -1375, 16325,  1401, -16323, -1427,
    16321,  1452, -16318, -1478, 16316,  1503, -16314, -1529,
    16311,  1555, -16309, -1580, 16306,  1606, -16304, -1632,
    16301,  1657, -16299, -1683, 16296,  1708, -16293, -1734,
    16291,  1760, -16288, -1785, 16285,  1811, -16282, -1836,
    16279,  1862, -16276, -1887, 16273,  1913, -16270, -1939,
    16267,  1964, -16264, -1990, 16261,  2015, -16258, -2041,
    16255,  2066, -16252, -2092, 16248,  2117, -16245, -2143,
    16242,  2168, -16238, -2194, 16235,  2219, -16231, -2245,
    16228,  2270, -16224, -2296, 16221,  2321, -16217, -2347,
    16213,  2372, -16209, -2398, 16206,  2423, -16202, -2449,
    16198,  2474, -16194, -2499, 16190,  2525, -16186, -2550,
    16182,  2576, -16178, -2601, 16174,  2627, -16170, -2652,
    16166,  2677, -16162, -2703, 16157,  2728, -16153, -2753,
    16149,  2779, -16144, -2804, 16140,  2830, -16136, -2855,
    16131,  2880, -16127, -2906, 16122,  2931, -16117, -2956,
    16113,  2982, -16108, -3007, 16103,  3032, -16099, -3057,
    16094,  3083, -16089, -3108, 16084,  3133, -16079, -3158,
    16074,  3184, -16069, -3209, 16064,  3234, -16059, -3259,
    16054,  3285, -16049, -3310, 16044,  3335, -16038, -3360,
    16033,  3385, -16028, -3411, 16022,  3436, -16017, -3461,
    16012,  3486, -16006, -3511, 16001,  3536, -15995, -3561,
    15989,  3587, -15984, -3612, 15978,  3637, -15972, -3662,
    15967,  3687, -15961, -3712, 15955,  3737, -15949, -3762,
    15943,  3787, -15937, -3812, 15931,  3837, -15925, -3862,
    15919,  3887, -15913, -3912, 15907,  3937, -15901, -3962,
    15895,  3987, -15888, -4012, 15882,  4037, -15876, -4062,
    15869,  4087, -15863, -4112, 15856,  4137, -15850, -4162,
    15843,  4187, -15837, -4211, 15830,  4236, -15824, -4261,
    15817,  4286, -15810, -4311, 15803,  4336, -15796, -4361,
    15790,  4385, -15783, -4410, 15776,  4435, -15769, -4460,
    15762,  4484, -15755, -4509, 15748,  4534, -15741, -4559,
    15733,  4583, -15726, -4608, 15719,  4633, -15712, -4657,
    15704,  4682, -15697, -4707, 15690,  4731, -15682, -4756,
    15675,  4781, -15667, -4805, 15660,  4830, -15652, -4854,
    15645,  4879, -15637, -4904, 15629,  4928, -15621, -4953,
    15614,  4977, -15606, -5002, 15598,  5026, -15590, -5051,
    15582,  5075, -15574, -5100, 15566,  5124, -15558, -5149,
    15550,  5173, -15542, -5197, 15534,  5222, -15525, -5246,
    15517,  5271, -15509, -5295, 15501,  5319, -15492, -5344,
    15484,  5368, -15475, -5392, 15467,  5416, -15459, -5441,
    15450,  5465, -15441, -5489, 15433,  5514, -15424, -5538,
    15415,  5562, -15407, -5586, 15398,  5610, -15389, -5635,
    15380,  5659, -15371, -5683, 15362,  5707, -15353, -5731,
    15344,  5755, -15335, -5779, 15326,  5803, -15317, -5827,
    15308,  5851, -15299, -5876, 15290,  5900, -15280, -5924,
    15271,  5948, -15262, -5971, 15252,  5995, -15243, -6019,
    15233,  6043, -15224, -6067, 15214,  6091, -15205, -6115,
    15195,  6139, -15186, -6163, 15176,  6187, -15166, -6210,
    15156,  6234, -15147, -6258, 15137,  6282, -15127, -6306,
    15117,  6329, -15107, -6353, 15097,  6377, -15087, -6400,
    15077,  6424, -15067, -6448, 15057,  6471, -15047, -6495,
    15036,  6519, -15026, -6542, 15016,  6566, -15006, -6589,
    14995,  6613, -14985, -6637, 14974,  6660, -14964, -6684,
    14954,  6707, -14943, -6731, 14932,  6754, -14922, -6777,
    14911,  6801, -14900, -6824, 14890,  6848, -14879, -6871,
    14868,  6894, -14857, -6918, 14847,  6941, -14836, -6964,
    14825,  6988, -14814, -7011, 14803,  7034, -14792, -7057,
    14781,  7081, -14769, -7104, 14758,  7127, -14747, -7150,
    14736,  7173, -14725, -7196, 14713,  7220, -14702, -7243,
    14691,  7266, -14679, -7289, 14668,  7312, -14656, -7335,
    14645,  7358, -14633, -7381, 14622,  7404, -14610, -7427,
    14598,  7450, -14587, -7473, 14575,  7495, -14563, -7518,
    14551,  7541, -14539, -7564, 14528,  7587, -14516, -7610,
    14504,  7632, -14492, -7655, 14480,  7678, -14468, -7701,
    14455,  7723, -14443, -7746, 14431,  7769, -14419, -7791,
    14407,  7814, -14394, -7837, 14382,  7859, -14370, -7882,
    14357,  7904, -14345, -7927, 14333,  7949, -14320, -7972,
    14308,  7994, -14295, -8017, 14282,  8039, -14270, -8062,
    14257,  8084, -14244, -8106, 14232,  8129, -14219, -8151,
    14206,  8173, -14193, -8196, 14180,  8218, -14167, -8240,
    14155,  8262, -14142, -8285, 14129,  8307, -14115, -8329,
    14102,  8351, -14089, -8373, 14076,  8395, -14063, -8418,
    14050,  8440, -14036, -8462, 14023,  8484, -14010, -8506,
    13996,  8528, -13983, -8550, 13970,  8572, -13956, -8594,
    13943,  8615, -13929, -8637, 13916,  8659, -13902, -8681,
    13888,  8703, -13875, -8725, 13861,  8746, -13847, -8768,
    13833,  8790, -13820, -8812, 13806,  8833, -13792, -8855,
    13778,  8877, -13764, -8898, 13750,  8920, -13736, -8941,
    13722,  8963, -13708, -8984, 13694,  9006, -13680, -9027,
    13666,  9049, -13651, -9070, 13637,  9092, -13623, -9113,
    13608,  9135, -13594, -9156, 13580,  9177, -13565, -9199,
    13551,  9220, -13536, -9241, 13522,  9262, -13507, -9284,
    13493,  9305, -13478, -9326, 13463,  9347, -13449, -9368,
    13434,  9389, -13419, -9410, 13405,  9431, -13390, -9452,
    13375,  9473, -13360, -9494, 13345,  9515, -13330, -9536,
    13315,  9557, -13300, -9578, 13285,  9599, -13270, -9620,
    13255,  9641, -13240, -9661, 13225,  9682, -13209, -9703,
    13194,  9724, -13179, -9744, 13164,  9765, -13148, -9786,
    13133,  9806, -13117, -9827, 13102,  9848, -13087, -9868,
    13071,  9889, -13056, -9909, 13040,  9930, -13024, -9950,
    13009,  9971, -12993, -9991, 12977, 10011, -12962, -10032,
    12946, 10052, -12930, -10072, 12914, 10093, -12898, -10113,
    12883, 10133, -12867, -10153, 12851, 10174, -12835, -10194,
    12819, 10214, -12803, -10234, 12787, 10254, -12770, -10274,
    12754, 10294, -12738, -10314, 12722, 10334, -12706, -10354,
    12689, 10374, -12673, -10394, 12657, 10414, -12640, -10434,
    12624, 10453, -12608, -10473, 12591, 10493, -12575, -10513,
    12558, 10533, -12542, -10552, 12525, 10572, -12508, -10592,
    12492, 10611, -12475, -10631, 12458, 10650, -12442, -10670,
    12425, 10689, -12408, -10709, 12391, 10728, -12375, -10748,
    12358, 10767, -12341, -10787, 12324, 10806, -12307, -10825,
    12290, 10845, -12273, -10864, 12256, 10883, -12239, -10902,
    12221, 10922, -12204, -10941, 12187, 10960, -12170, -10979,
    12153, 10998, -12135, -11017, 12118, 11036, -12101, -11055,
    12083, 11074, -12066, -11093, 12049, 11112, -12031, -11131,
    12014, 11150, -11996, -11169, 11979, 11187, -11961, -11206,
    11943, 11225, -11926, -11244, 11908, 11262, -11890, -11281,
    11873, 11300, -11855, -11318, 11837, 11337, -11819, -11356,
    11802, 11374, -11784, -11393, 11766, 11411, -11748, -11430,
    11730, 11448, -11712, -11466, 11694, 11485, -11676, -11503,
    11658, 11521, -11640, -11540, 11622, 11558, -11603, -11576,
    11585, 11594, -11567, -11613, 11549, 11631, -11531, -11649,
    11512, 11667, -11494, -11685, 11476, 11703, -11457, -11721,
    11439, 11739, -11420, -11757, 11402, 11775, -11383, -11793,
    11365, 11810, -11346, -11828, 11328, 11846, -11309, -11864,
    11290, 11882, -11272, -11899, 11253, 11917, -11234, -11935,
    11216, 11952, -11197, -11970, 11178, 11987, -11159, -12005,
    11140, 12022, -11121, -12040, 11103, 12057, -11084, -12075,
    11065, 12092, -11046, -12109, 11027, 12127, -11008, -12144,
    10989, 12161, -10969, -12179, 10950, 12196, -10931, -12213,
    10912, 12230, -10893, -12247, 10873, 12264, -10854, -12281,
    10835, 12298, -10816, -12315, 10796, 12332, -10777, -12349,
    10758, 12366, -10738, -12383, 10719, 12400, -10699, -12417,
    10680, 12433, -10660, -12450, 10641, 12467, -10621, -12484,
    10601, 12500, -10582, -12517, 10562, 12533, -10542, -12550,
    10523, 12566, -10503, -12583, 10483, 12599, -10463, -12616,
    10444, 12632, -10424, -12649, 10404, 12665, -10384, -12681,
    10364, 12698, -10344, -12714, 10324, 12730, -10304, -12746,
    10284, 12762, -10264, -12779, 10244, 12795, -10224, -12811,
    10204, 12827, -10184, -12843, 10163, 12859, -10143, -12875,
    10123, 12890, -10103, -12906, 10082, 12922, -10062, -12938,
    10042, 12954, -10022, -12970, 10001, 12985, -9981, -13001,
    9960, 13017, -9940, -13032,  9919, 13048, -9899, -13063,
    9878, 13079, -9858, -13094,  9837, 13110, -9817, -13125,
    9796, 13141, -9775, -13156,  9755, 13171, -9734, -13187,
    9713, 13202, -9693, -13217,  9672, 13232, -9651, -13247,
    9630, 13262, -9609, -13278,  9589, 13293, -9568, -13308,
    9547, 13323, -9526, -13338,  9505, 13353, -9484, -13367,
    9463, 13382, -9442, -13397,  9421, 13412, -9400, -13427,
    9379, 13441, -9358, -13456,  9336, 13471, -9315, -13485,
    9294, 13500, -9273, -13515,  9252, 13529, -9230, -13544,
    9209, 13558, -9188, -13573,  9167, 13587, -9145, -13601,
    9124, 13616, -9102, -13630,  9081, 13644, -9060, -13658,
    9038, 13673, -9017, -13687,  8995, 13701, -8974, -13715,
    8952, 13729, -8931, -13743,  8909, 13757, -8887, -13771,
    8866, 13785, -8844, -13799,  8822, 13813, -8801, -13827,
    8779, 13840, -8757, -13854,  8735, 13868, -8714, -13882,
    8692, 13895, -8670, -13909,  8648, 13922, -8626, -13936,
    8604, 13949, -8583, -13963,  8561, 13976, -8539, -13990,
    8517, 14003, -8495, -14017,  8473, 14030, -8451, -14043,
    8429, 14056, -8406, -14070,  8384, 14083, -8362, -14096,
    8340, 14109, -8318, -14122,  8296, 14135, -8274, -14148,
    8251, 14161, -8229, -14174,  8207, 14187, -8185, -14200,
    8162, 14212, -8140, -14225,  8118, 14238, -8095, -14251,
    8073, 14263, -8050, -14276,  8028, 14289, -8006, -14301,
    7983, 14314, -7961, -14326,  7938, 14339, -7916, -14351,
    7893, 14364, -7870, -14376,  7848, 14388, -7825, -14401,
    7803, 14413, -7780, -14425,  7757, 14437, -7735, -14449,
    7712, 14462, -7689, -14474,  7667, 14486, -7644, -14498,
    7621, 14510, -7598, -14522,  7575, 14533, -7553, -14545,
    7530, 14557, -7507, -14569,  7484, 14581, -7461, -14592,
    7438, 14604, -7415, -14616,  7392, 14627, -7369, -14639,
    7346, 14650, -7323, -14662,  7300, 14673, -7277, -14685,
    7254, 14696, -7231, -14708,  7208, 14719, -7185, -14730,
    7162, 14741, -7139, -14753,  7115, 14764, -7092, -14775,
    7069, 14786, -7046, -14797,  7023, 14808, -6999, -14819,
    6976, 14830, -6953, -14841,  6929, 14852, -6906, -14863,
    6883, 14874, -6859, -14884,  6836, 14895, -6813, -14906,
    6789, 14917, -6766, -14927,  6742, 14938, -6719, -14948,
    6695, 14959, -6672, -14969,  6648, 14980, -6625, -14990,
    6601, 15000, -6578, -15011,  6554, 15021, -6530, -15031,
    6507, 15042, -6483, -15052,  6460, 15062, -6436, -15072,
    6412, 15082, -6389, -15092,  6365, 15102, -6341, -15112,
    6317, 15122, -6294, -15132,  6270, 15142, -6246, -15152,
    6222, 15161, -6198, -15171,  6175, 15181, -6151, -15190,
    6127, 15200, -6103, -15210,  6079, 15219, -6055, -15229,
    6031, 15238, -6007, -15248,  5983, 15257, -5959, -15266,
    5936, 15276, -5912, -15285,  5888, 15294, -5863, -15303,
    5839, 15313, -5815, -15322,  5791, 15331, -5767, -15340,
    5743, 15349, -5719, -15358,  5695, 15367, -5671, -15376,
    5647, 15385, -5622, -15393,  5598, 15402, -5574, -15411,
    5550, 15420, -5526, -15428,  5501, 15437, -5477, -15446,
    5453, 15454, -5429, -15463,  5404, 15471, -5380, -15480,
    5356, 15488, -5331, -15496,  5307, 15505, -5283, -15513,
    5258, 15521, -5234, -15530,  5210, 15538, -5185, -15546,
    5161, 15554, -5136, -15562,  5112, 15570, -5087, -15578,
    5063, 15586, -5038, -15594,  5014, 15602, -4989, -15610,
    4965, 15618, -4940, -15625,  4916, 15633, -4891, -15641,
    4867, 15648, -4842, -15656,  4818, 15663, -4793, -15671,
    4768, 15679, -4744, -15686,  4719, 15693, -4694, -15701,
    4670, 15708, -4645, -15715,  4620, 15723, -4596, -15730,
    4571, 15737, -4546, -15744,  4522, 15751, -4497, -15758,
    4472, 15765, -4447, -15772,  4423, 15779, -4398, -15786,
    4373, 15793, -4348, -15800,  4323, 15807, -4298, -15813,
    4274, 15820, -4249, -15827,  4224, 15833, -4199, -15840,
    4174, 15847, -4149, -15853,  4124, 15860, -4099, -15866,
    4075, 15872, -4050, -15879,  4025, 15885, -4000, -15891,
    3975, 15898, -3950, -15904,  3925, 15910, -3900, -15916,
    3875, 15922, -3850, -15928,  3825, 15934, -3800, -15940,
    3775, 15946, -3750, -15952,  3725, 15958, -3700, -15964,
    3674, 15970, -3649, -15975,  3624, 15981, -3599, -15987,
    3574, 15992, -3549, -15998,  3524, 16003, -3499, -16009,
    3474, 16014, -3448, -16020,  3423, 16025, -3398, -16030,
    3373, 16036, -3348, -16041,  3322, 16046, -3297, -16051,
    3272, 16057, -3247, -16062,  3222, 16067, -3196, -16072,
    3171, 16077, -3146, -16082,  3121, 16087, -3095, -16091,
    3070, 16096, -3045, -16101,  3019, 16106, -2994, -16110,
    2969, 16115, -2944, -16120,  2918, 16124, -2893, -16129,
    2868, 16133, -2842, -16138,  2817, 16142, -2792, -16147,
    2766, 16151, -2741, -16155,  2715, 16160, -2690, -16164,
    2665, 16168, -2639, -16172,  2614, 16176, -2588, -16180,
    2563, 16184, -2538, -16188,  2512, 16192, -2487, -16196,
    2461, 16200, -2436, -16204,  2410, 16208, -2385, -16211,
    2359, 16215, -2334, -16219,  2309, 16222, -2283, -16226,
    2258, 16229, -2232, -16233,  2207, 16236, -2181, -16240,
    2156, 16243, -2130, -16247,  2105, 16250, -2079, -16253,
    2053, 16256, -2028, -16260,  2002, 16263, -1977, -16266,
    1951, 16269, -1926, -16272,  1900, 16275, -1875, -16278,
    1849, 16281, -1823, -16284,  1798, 16286, -1772, -16289,
    1747, 16292, -1721, -16295,  1696, 16297, -1670, -16300,
    1644, 16303, -1619, -16305,  1593, 16308, -1567, -16310,
    1542, 16312, -1516, -16315,  1491, 16317, -1465, -16320,
    1439, 16322, -1414, -16324,  1388, 16326, -1362, -16328,
    1337, 16330, -1311, -16332,  1285, 16334, -1260, -16336,
    1234, 16338, -1208, -16340,  1183, 16342, -1157, -16344,
    1131, 16346, -1106, -16348,  1080, 16349, -1054, -16351,
    1029, 16352, -1003, -16354,   977, 16356,  -952, -16357,
    926, 16359,  -900, -16360,   875, 16361,  -849, -16363,
    823, 16364,  -797, -16365,   772, 16366,  -746, -16368,
    720, 16369,  -695, -16370,   669, 16371,  -643, -16372,
    618, 16373,  -592, -16374,   566, 16375,  -540, -16376,
    515, 16376,  -489, -16377,   463, 16378,  -437, -16378,
    412, 16379,  -386, -16380,   360, 16380,  -335, -16381,
    309, 16381,  -283, -16382,   257, 16382,  -232, -16383,
    206, 16383,  -180, -16383,   154, 16383,  -129, -16384,
    103, 16384,   -77, -16384,    51, 16384,   -26, -16384, 0
    };

#else
#ifdef HZ8_SHIFT

#define NCOS 1000
static short shCos[NCOS+1] = {
    16384,    26, -16384,   -77, 16384,   129, -16383,  -180,
    16383,   232, -16382,  -283, 16381,   335, -16380,  -386,
    16379,   437, -16377,  -489, 16376,   540, -16374,  -592,
    16372,   643, -16370,  -695, 16368,   746, -16366,  -797,
    16363,   849, -16361,  -900, 16358,   952, -16355, -1003,
    16352,  1054, -16348, -1106, 16345,  1157, -16341, -1208,
    16337,  1260, -16333, -1311, 16329,  1362, -16325, -1414,
    16321,  1465, -16316, -1516, 16311,  1567, -16306, -1619,
    16301,  1670, -16296, -1721, 16291,  1772, -16285, -1823,
    16279,  1875, -16273, -1926, 16267,  1977, -16261, -2028,
    16255,  2079, -16248, -2130, 16242,  2181, -16235, -2232,
    16228,  2283, -16221, -2334, 16213,  2385, -16206, -2436,
    16198,  2487, -16190, -2538, 16182,  2588, -16174, -2639,
    16166,  2690, -16157, -2741, 16149,  2792, -16140, -2842,
    16131,  2893, -16122, -2944, 16113,  2994, -16103, -3045,
    16094,  3095, -16084, -3146, 16074,  3196, -16064, -3247,
    16054,  3297, -16044, -3348, 16033,  3398, -16022, -3448,
    16012,  3499, -16001, -3549, 15989,  3599, -15978, -3649,
    15967,  3700, -15955, -3750, 15943,  3800, -15931, -3850,
    15919,  3900, -15907, -3950, 15895,  4000, -15882, -4050,
    15869,  4099, -15856, -4149, 15843,  4199, -15830, -4249,
    15817,  4298, -15803, -4348, 15790,  4398, -15776, -4447,
    15762,  4497, -15748, -4546, 15733,  4596, -15719, -4645,
    15704,  4694, -15690, -4744, 15675,  4793, -15660, -4842,
    15645,  4891, -15629, -4940, 15614,  4989, -15598, -5038,
    15582,  5087, -15566, -5136, 15550,  5185, -15534, -5234,
    15517,  5283, -15501, -5331, 15484,  5380, -15467, -5429,
    15450,  5477, -15433, -5526, 15415,  5574, -15398, -5622,
    15380,  5671, -15362, -5719, 15344,  5767, -15326, -5815,
    15308,  5863, -15290, -5912, 15271,  5959, -15252, -6007,
    15233,  6055, -15214, -6103, 15195,  6151, -15176, -6198,
    15156,  6246, -15137, -6294, 15117,  6341, -15097, -6389,
    15077,  6436, -15057, -6483, 15036,  6530, -15016, -6578,
    14995,  6625, -14974, -6672, 14954,  6719, -14932, -6766,
    14911,  6813, -14890, -6859, 14868,  6906, -14847, -6953,
    14825,  6999, -14803, -7046, 14781,  7092, -14758, -7139,
    14736,  7185, -14713, -7231, 14691,  7277, -14668, -7323,
    14645,  7369, -14622, -7415, 14598,  7461, -14575, -7507,
    14551,  7553, -14528, -7598, 14504,  7644, -14480, -7689,
    14455,  7735, -14431, -7780, 14407,  7825, -14382, -7870,
    14357,  7916, -14333, -7961, 14308,  8006, -14282, -8050,
    14257,  8095, -14232, -8140, 14206,  8185, -14180, -8229,
    14155,  8274, -14129, -8318, 14102,  8362, -14076, -8406,
    14050,  8451, -14023, -8495, 13996,  8539, -13970, -8583,
    13943,  8626, -13916, -8670, 13888,  8714, -13861, -8757,
    13833,  8801, -13806, -8844, 13778,  8887, -13750, -8931,
    13722,  8974, -13694, -9017, 13666,  9060, -13637, -9102,
    13608,  9145, -13580, -9188, 13551,  9230, -13522, -9273,
    13493,  9315, -13463, -9358, 13434,  9400, -13405, -9442,
    13375,  9484, -13345, -9526, 13315,  9568, -13285, -9609,
    13255,  9651, -13225, -9693, 13194,  9734, -13164, -9775,
    13133,  9817, -13102, -9858, 13071,  9899, -13040, -9940,
    13009,  9981, -12977, -10022, 12946, 10062, -12914, -10103,
    12883, 10143, -12851, -10184, 12819, 10224, -12787, -10264,
    12754, 10304, -12722, -10344, 12689, 10384, -12657, -10424,
    12624, 10463, -12591, -10503, 12558, 10542, -12525, -10582,
    12492, 10621, -12458, -10660, 12425, 10699, -12391, -10738,
    12358, 10777, -12324, -10816, 12290, 10854, -12256, -10893,
    12221, 10931, -12187, -10969, 12153, 11008, -12118, -11046,
    12083, 11084, -12049, -11121, 12014, 11159, -11979, -11197,
    11943, 11234, -11908, -11272, 11873, 11309, -11837, -11346,
    11802, 11383, -11766, -11420, 11730, 11457, -11694, -11494,
    11658, 11531, -11622, -11567, 11585, 11603, -11549, -11640,
    11512, 11676, -11476, -11712, 11439, 11748, -11402, -11784,
    11365, 11819, -11328, -11855, 11290, 11890, -11253, -11926,
    11216, 11961, -11178, -11996, 11140, 12031, -11103, -12066,
    11065, 12101, -11027, -12135, 10989, 12170, -10950, -12204,
    10912, 12239, -10873, -12273, 10835, 12307, -10796, -12341,
    10758, 12375, -10719, -12408, 10680, 12442, -10641, -12475,
    10601, 12508, -10562, -12542, 10523, 12575, -10483, -12608,
    10444, 12640, -10404, -12673, 10364, 12706, -10324, -12738,
    10284, 12770, -10244, -12803, 10204, 12835, -10163, -12867,
    10123, 12898, -10082, -12930, 10042, 12962, -10001, -12993,
    9960, 13024, -9919, -13056,  9878, 13087, -9837, -13117,
    9796, 13148, -9755, -13179,  9713, 13209, -9672, -13240,
    9630, 13270, -9589, -13300,  9547, 13330, -9505, -13360,
    9463, 13390, -9421, -13419,  9379, 13449, -9336, -13478,
    9294, 13507, -9252, -13536,  9209, 13565, -9167, -13594,
    9124, 13623, -9081, -13651,  9038, 13680, -8995, -13708,
    8952, 13736, -8909, -13764,  8866, 13792, -8822, -13820,
    8779, 13847, -8735, -13875,  8692, 13902, -8648, -13929,
    8604, 13956, -8561, -13983,  8517, 14010, -8473, -14036,
    8429, 14063, -8384, -14089,  8340, 14115, -8296, -14142,
    8251, 14167, -8207, -14193,  8162, 14219, -8118, -14244,
    8073, 14270, -8028, -14295,  7983, 14320, -7938, -14345,
    7893, 14370, -7848, -14394,  7803, 14419, -7757, -14443,
    7712, 14468, -7667, -14492,  7621, 14516, -7575, -14539,
    7530, 14563, -7484, -14587,  7438, 14610, -7392, -14633,
    7346, 14656, -7300, -14679,  7254, 14702, -7208, -14725,
    7162, 14747, -7115, -14769,  7069, 14792, -7023, -14814,
    6976, 14836, -6929, -14857,  6883, 14879, -6836, -14900,
    6789, 14922, -6742, -14943,  6695, 14964, -6648, -14985,
    6601, 15006, -6554, -15026,  6507, 15047, -6460, -15067,
    6412, 15087, -6365, -15107,  6317, 15127, -6270, -15147,
    6222, 15166, -6175, -15186,  6127, 15205, -6079, -15224,
    6031, 15243, -5983, -15262,  5936, 15280, -5888, -15299,
    5839, 15317, -5791, -15335,  5743, 15353, -5695, -15371,
    5647, 15389, -5598, -15407,  5550, 15424, -5501, -15441,
    5453, 15459, -5404, -15475,  5356, 15492, -5307, -15509,
    5258, 15525, -5210, -15542,  5161, 15558, -5112, -15574,
    5063, 15590, -5014, -15606,  4965, 15621, -4916, -15637,
    4867, 15652, -4818, -15667,  4768, 15682, -4719, -15697,
    4670, 15712, -4620, -15726,  4571, 15741, -4522, -15755,
    4472, 15769, -4423, -15783,  4373, 15796, -4323, -15810,
    4274, 15824, -4224, -15837,  4174, 15850, -4124, -15863,
    4075, 15876, -4025, -15888,  3975, 15901, -3925, -15913,
    3875, 15925, -3825, -15937,  3775, 15949, -3725, -15961,
    3674, 15972, -3624, -15984,  3574, 15995, -3524, -16006,
    3474, 16017, -3423, -16028,  3373, 16038, -3322, -16049,
    3272, 16059, -3222, -16069,  3171, 16079, -3121, -16089,
    3070, 16099, -3019, -16108,  2969, 16117, -2918, -16127,
    2868, 16136, -2817, -16144,  2766, 16153, -2715, -16162,
    2665, 16170, -2614, -16178,  2563, 16186, -2512, -16194,
    2461, 16202, -2410, -16209,  2359, 16217, -2309, -16224,
    2258, 16231, -2207, -16238,  2156, 16245, -2105, -16252,
    2053, 16258, -2002, -16264,  1951, 16270, -1900, -16276,
    1849, 16282, -1798, -16288,  1747, 16293, -1696, -16299,
    1644, 16304, -1593, -16309,  1542, 16314, -1491, -16318,
    1439, 16323, -1388, -16327,  1337, 16331, -1285, -16335,
    1234, 16339, -1183, -16343,  1131, 16347, -1080, -16350,
    1029, 16353,  -977, -16356,   926, 16359,  -875, -16362,
    823, 16365,  -772, -16367,   720, 16369,  -669, -16371,
    618, 16373,  -566, -16375,   515, 16377,  -463, -16378,
    412, 16379,  -360, -16381,   309, 16382,  -257, -16382,
    206, 16383,  -154, -16383,   103, 16384,   -51, -16384, 0
    };
#endif
#endif

static void cosShift_16s_32s(_shiftupCosState_int *state)
    {
    Ipp32s *Buf = state->upDownBuf32;
    Ipp16s *BufIn = state->upDownBuf16;
    int     len = 4*state->frameSize;
    int i, counter0, counter1;

    if (state->phase < NCOS+1) {
        counter0 = len;
        if ((counter0 + state->phase) > NCOS+1) {
            counter0 = NCOS+1 - state->phase;
            }

        for (i = 0; i < counter0; i++) {
            Buf[i] = (Ipp32s)((BufIn[i] * (Ipp32s)shCos[i + state->phase]) >> 2);
            }

        counter1 = len - counter0;

        for (i = 0; i < counter1; i++) {
            Buf[i+counter0] = -(Ipp32s)((BufIn[i+counter0] * (Ipp32s)shCos[NCOS-1 - i]) >> 2);
            }
        } else if (state->phase < 2*NCOS) {
            counter0 = len;
            if ((counter0 + state->phase) > 2*NCOS) {
                counter0 = 2*NCOS - state->phase;
                }

            for (i = 0; i < counter0; i++) {
                Buf[i] = -(Ipp32s)((BufIn[i] * (Ipp32s)shCos[2*NCOS - i - state->phase]) >> 2);
                }

            counter1 = len - counter0;

            for (i = 0; i < counter1; i++) {
                Buf[i+counter0] = -(Ipp32s)((BufIn[i+counter0] * (Ipp32s)shCos[i]) >> 2);
                }
        } else if (state->phase < 3*NCOS+1) {
            counter0 = len;
            if ((counter0 + state->phase) > 3*NCOS+1) {
                counter0 = 3*NCOS+1 - state->phase;
                }

            for (i = 0; i < counter0; i++) {
                Buf[i] = -(Ipp32s)((BufIn[i] * (Ipp32s)shCos[i + state->phase - 2*NCOS]) >> 2);
                }

            counter1 = len - counter0;

            for (i = 0; i < counter1; i++) {
                Buf[i+counter0] = (Ipp32s)((BufIn[i+counter0] * (Ipp32s)shCos[NCOS - 1 - i]) >> 2);
                }
            } else {
                counter0 = len;
                if ((counter0 + state->phase) > 4*NCOS) {
                    counter0 = 4*NCOS - state->phase;
                    }

                for (i = 0; i < counter0; i++) {
                    Buf[i] = (Ipp32s)((BufIn[i] * (Ipp32s)shCos[4*NCOS - i - state->phase]) >> 2);
                    }

                counter1 = len - counter0;

                for (i = 0; i < counter1; i++) {
                    Buf[i+counter0] = (Ipp32s)((BufIn[i+counter0] * (Ipp32s)shCos[i]) >> 2);
                    }
            }

        state->phase += len;
        if (state->phase > 4*NCOS) state->phase -= 4*NCOS;
    }

static void ownIIR_BQ_32s_I(Ipp32s* pSrcDst,
                            int len,
                            Ipp32s* pCoeff1,
                            Ipp32s* pDlyLine1,
                            int numBq)
    {
    int i;

    for (i = 0; i < len; i++) {
        Ipp32s *pCoeff = pCoeff1;
        Ipp32s *pDlyLine = pDlyLine1;
        Ipp32s src = pSrcDst[i];
        int n;

        for (n = 0; n < numBq; n++) {
            pDlyLine[0] = src << 2;
            src = (Ipp32s)(((Ipp64s)pCoeff[0]*pDlyLine[0] +
                (Ipp64s)pCoeff[1]*pDlyLine[1] +
                (Ipp64s)pCoeff[2]*pDlyLine[2] -
                (Ipp64s)pCoeff[4]*pDlyLine[4] -
                (Ipp64s)pCoeff[5]*pDlyLine[5]) >> 32);

            pDlyLine[5] = pDlyLine[4]; pDlyLine[4] = (src << 2);
            pDlyLine[2] = pDlyLine[1]; pDlyLine[1] = pDlyLine[0];
            pCoeff += 6; pDlyLine += 6;
            }
        pSrcDst[i] = src;
        }
    }

IppStatus shiftupCos(_shiftupCosState_int *state,
                     Ipp16s *in,
                     Ipp16s *out)
    {
    int i;
    for (i = 0; i < state->frameSize; i++) {
        state->upDownBuf16[4*i+0] = in[i];
        state->upDownBuf16[4*i+1] = 0;
        state->upDownBuf16[4*i+2] = 0;
        state->upDownBuf16[4*i+3] = 0;
        }
    ippsFIR32s_16s_Sfs(state->upDownBuf16, state->upDownBuf16, 4*state->frameSize, state->fir64State, 0);
    cosShift_16s_32s(state);
    ownIIR_BQ_32s_I(state->upDownBuf32, 4*state->frameSize, state->ibq2Cfs, state->ibq2Dly, 2);

    for (i = 0; i < state->frameSize; i++) {
        state->upDownBuf32[4*i+0] >>= 7;
        out[i] = Cnvrt_32s16s((state->upDownBuf32[4*i+0] * 75) >> 10);
        }

    return ippStsNoErr;
    }

int shiftupCos_GetSize(int frameSize,
                       int *size)
    {
    IppStatus status;
    int bufSize;
    status = ippsFIRGetStateSize32s_16s(AH_FIR_TAPS, &bufSize);
    *size = ALIGN(frameSize*sizeof(Ipp16s)*4) +
        frameSize*sizeof(Ipp32s)*4 + /* up buf size */
        ALIGN(sizeof(_shiftupCosState_int) + ALIGN(bufSize));
    return status;
    }

int shiftupCos_Init(_shiftupCosState_int *state,
                    int frameSize)
    {
    IppStatus status;
    Ipp8u *pInit = (Ipp8u *)state;
    pInit += ALIGN(sizeof(_shiftupCosState_int));
    state->upDownBuf16 = (Ipp16s *)pInit;
    pInit += ALIGN(frameSize*sizeof(Ipp16s)*4);
    state->upDownBuf32 = (Ipp32s *)pInit;
    pInit += ALIGN(frameSize*sizeof(Ipp32s)*4);

    ippsZero_32s(state->ibq2Dly,2*6);
    ippsZero_16s(state->fir64Dly,AH_FIR_TAPS);

    ippsCopy_32s(ibq2Cfs, state->ibq2Cfs, 2*6);

    state->frameSize = frameSize;
    state->phase = 0;
    status = ippsFIRInit32s_16s(&state->fir64State, firTaps, AH_FIR_TAPS, -31, state->fir64Dly, pInit);
    return status;
    }
int aecSetNlpThresholds(_ahState *state,Ipp32s n,Ipp32s smooth,Ipp32s coef){
    int status,j;
    switch(n) {
    case (0):
        ippsFilterNoiseLevel_EC_32f(ippsNrNone, state->nr);
        ippsFilterNoiseLevel_EC_32f(ippsNrNone, state->nrSin);
        state->nrMode=ippsNrNone;
        for (j=0;j<state->nNLP;++j)
            initNLP(&state->nlpObj[j],11,11,15,12,30,coef);
        break;
    case (1):
        ippsFilterNoiseLevel_EC_32f(ippsNrLow, state->nr);
        ippsFilterNoiseLevel_EC_32f(ippsNrLow, state->nrSin);
        state->nrMode=ippsNrLow;
        for (j=0;j<state->nNLP;++j)
            initNLP(&state->nlpObj[j],11,11,15,12,30,coef);
        break;
    case (2):
        ippsFilterNoiseLevel_EC_32f(ippsNrMedium, state->nr);
        ippsFilterNoiseLevel_EC_32f(ippsNrMedium, state->nrSin);
        state->nrMode=ippsNrMedium;
        for (j=0;j<state->nNLP;++j)
            initNLP(&state->nlpObj[j],11,11,15,12,30,coef);
        break;
    case (3):
        ippsFilterNoiseLevel_EC_32f(ippsNrNormal, state->nr);
        ippsFilterNoiseLevel_EC_32f(ippsNrNormal, state->nrSin);
        state->nrMode=ippsNrNormal;
        for (j=0;j<state->nNLP;++j)
            initNLP(&state->nlpObj[j],11,11,15,12,30,coef);
        break;
    case (4):
        ippsFilterNoiseLevel_EC_32f(ippsNrHigh, state->nr);
        ippsFilterNoiseLevel_EC_32f(ippsNrHigh, state->nrSin);
        state->nrMode=ippsNrHigh;
        for (j=0;j<state->nNLP;++j)
            initNLP(&state->nlpObj[j],11,11,15,12,30,coef);
        break;
    default:
        return (-1);
        }
    switch(smooth) {
    case (0):
        ippsFilterNoiseSetMode_EC_32f(ippsNrSmoothOff, state->nr);
        status=ippsFilterNoiseSetMode_EC_32f(ippsNrSmoothOff, state->nrSin);
        break;
    case (1):
        ippsFilterNoiseSetMode_EC_32f(ippsNrSmoothStatic, state->nr);
        status=ippsFilterNoiseSetMode_EC_32f(ippsNrSmoothStatic, state->nrSin);
        break;
    case (2):
        ippsFilterNoiseSetMode_EC_32f(ippsNrSmoothDynamic, state->nr);
        status=ippsFilterNoiseSetMode_EC_32f(ippsNrSmoothDynamic, state->nrSin);
        break;
    default:
        return (-1);
        }
    return status;
    }
