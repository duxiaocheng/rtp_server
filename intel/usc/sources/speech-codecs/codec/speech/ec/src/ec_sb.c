/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2012 Intel Corporation. All Rights Reserved.
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
//  Purpose: Echo Canceller, subband algorithm
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

typedef struct _SubbandControllerState_EC_32f {
    Ipp32s numSubbands;
    Ipp32s frameSize;
    Ipp32s numSegments;
    Ipp32s sampleFreq;
    Ipp32f *adaptive_pwr;
    Ipp32f *fixed_pwr;
    Ipp32f *rin_pwr;
    /*    Ipp32f *rin_pwr_n;*/
    Ipp32f *cf_pwr;
    /*    Ipp32f pwr_b;*/
    Ipp32f sin_pwr;
    Ipp32f coef_werr;
    Ipp32f coef_wcoef;
    Ipp32f coef_nlp0, coef_nlp1;
    Ipp32f sgain, rgain, nlp_val;
    Ipp32f rin_pwr_min, stepSize_max;
    Ipp32s nlp_count;
    Ipp32s sub_use;
    } SubbandControllerState_EC_32f;

static Ipp32s ownSubbandController_EC_32f (const Ipp32fc *pSrcAdaptiveFilterErr,
                                           const Ipp32fc *pSrcFixedFilterErr,
                                           Ipp32fc **ppDstAdaptiveCoefs, Ipp32fc **ppDstFixedCoefs,
                                           Ipp32f *pDstSGain, SubbandControllerState_EC_32f *pState,Ipp32s dtFlag)
    {
    Ipp32s i, j;
    Ipp32f powa = 0, powc = 0, powd = 0, uval, add;
    Ipp32f *cf_pwr, *fixed_pwr, *adaptive_pwr;
    Ipp32f coef_wcoef, coef_werr;


    pState = IPP_ALIGNED_PTR(pState, 16);
    cf_pwr = pState->cf_pwr;
    fixed_pwr = pState->fixed_pwr;
    adaptive_pwr = pState->adaptive_pwr;
    coef_wcoef = pState->coef_wcoef;
    coef_werr = pState->coef_werr;

    for (i = 0; i < pState->numSubbands; i++) {
        Ipp32f fpwr, apwr, cpwr, cdlt;

        fpwr = pSrcFixedFilterErr[i].re * pSrcFixedFilterErr[i].re +
            pSrcFixedFilterErr[i].im * pSrcFixedFilterErr[i].im;
        apwr = pSrcAdaptiveFilterErr[i].re * pSrcAdaptiveFilterErr[i].re +
            pSrcAdaptiveFilterErr[i].im * pSrcAdaptiveFilterErr[i].im;
        cpwr = 0;

        for (j = 0; j < pState->numSegments; j++)
            cpwr += ppDstAdaptiveCoefs[j][i].re * ppDstAdaptiveCoefs[j][i].re +
            ppDstAdaptiveCoefs[j][i].im * ppDstAdaptiveCoefs[j][i].im;

        cdlt = (Ipp32f)fabs(cf_pwr[i] - cpwr);

        ADDWEIGHTED(cf_pwr[i], cpwr, coef_wcoef);
        ADDWEIGHTED(fixed_pwr[i], fpwr, coef_werr);
        ADDWEIGHTED(adaptive_pwr[i], apwr, coef_werr);

        if (0.1 * cf_pwr[i] > cdlt &&//0.2736 > 8.08e-5
            //            0.35 * fixed_pwr[i] > adaptive_pwr[i]) {
            fixed_pwr[i] > adaptive_pwr[i] && (!dtFlag)) {
                for (j = 0; j < pState->numSegments; j++) {// adaptation converged
                    ppDstFixedCoefs[j][i] = ppDstAdaptiveCoefs[j][i];
                    }
                fixed_pwr[i] = adaptive_pwr[i];
                powc += fixed_pwr[i];
            } else if (1.5 * fixed_pwr[i] < adaptive_pwr[i]) {
                for (j = 0; j < pState->numSegments; j++) {
                    ppDstAdaptiveCoefs[j][i] = ppDstFixedCoefs[j][i];
                    }
                adaptive_pwr[i] = fixed_pwr[i];
                powd += fixed_pwr[i];
            }
        powa += fixed_pwr[i];
        }

    if (pState->sub_use) {
        if (powc > 0.1 * powa && pState->sin_pwr * 0.2 > powa) {
            add = 0.f;
            ADDWEIGHTED(pState->nlp_val, add, pState->coef_nlp0);
            } else if (powd > 0.1 * powa) {
                add = 1.f;
                ADDWEIGHTED(pState->nlp_val, add, pState->coef_nlp0);
            }
        } else {
            if (powc > 0.1 * powa) {
                add = 0.f;
                ADDWEIGHTED(pState->nlp_val, add, pState->coef_nlp0);
                } else if (powd > 0.1 * powa) {
                    add = 1.f;
                    ADDWEIGHTED(pState->nlp_val, add, pState->coef_nlp0);
                }
        }

    uval = 0;
    if (pState->nlp_val > 0.5)
        uval = 1;

    ADDWEIGHTED(pState->sgain, uval, pState->coef_nlp1);

    *pDstSGain = pState->sgain;

    return 0;
    }

int ec_sb_setNRreal(void *stat, int level,int smooth){
    int status;
    _sbECState *state=(_sbECState *)stat;
    status=aecSetNlpThresholds(state->ah,level,smooth,23);
    return status;
    }

int ec_sb_setTDThreshold(void *stat,int val)
    {
    _sbECState *state=(_sbECState *)stat;
    if (val>-30) {
        return 1;
        }
    if (val<-89) {
        val=-89;
        }
    state->ah->td.td_thres=(Ipp64f)(IPP_AEC_dBtbl[-val-10]*IPP_AEC_dBtbl[-val-10]);
    return 0;
    }
int ec_sb_setTDSilenceTime(void *stat,int val)
    {
    _sbECState *state=(_sbECState *)stat;
    if (val<100) {
        return 1;
        }
    state->ah->td.td_sil_thresh=(Ipp16s)val;
    return 0;
    }
int ec_sb_setNR(void *stat, int n){
    int status=0;
    _sbECState *state=(_sbECState *)stat;
    state->nlp = (Ipp16s)n;
    return status;
    }
int ec_sb_setCNG(void *stat, int n){
    int status=0;
    _sbECState *state=(_sbECState *)stat;
    state->cng = (Ipp16s)n;
    return status;
    }
/* Returns size of frame */
/* init scratch memory buffer */
int ec_sb_InitBuff(void * stat, char *buff) {
    _sbECState *state=(_sbECState *)stat;
    if (state==NULL) {
        return 1;
        }
    if(NULL != buff)
        state->Mem.base = buff;
    else
        if (NULL == state->Mem.base)
            return 1;
    state->Mem.CurPtr = state->Mem.base;
    state->Mem.VecPtr = (int *)(state->Mem.base+state->scratch_mem_size);
    return 0;
    }
int ec_sb_GetInfo(void *stat,ECOpcode *isNLP,ECOpcode *isTD,
                  ECOpcode *isAdapt,ECOpcode *isAdaptLite, ECOpcode *isAH)
    {
    _sbECState *state=(_sbECState *)stat;
    if(state->mode & 2)
        *isNLP = EC_NLP_ENABLE;
    else
        *isNLP = EC_NLP_DISABLE;

    if(state->mode & 16)
        *isAH = EC_AH_ENABLE;
    //*isAH = EC_AH_ENABLE1;
    //else
    //    *isAH = EC_AH_DISABLE;

    //if(state->mode & 32)
    //    *isAH = EC_AH_ENABLE2;
    //else
    //    *isAH = EC_AH_DISABLE;

    if(state->mode & 4)
        *isTD = EC_TD_ENABLE;
    else
        *isTD = EC_TD_DISABLE;

    *isAdaptLite = EC_ADAPTATION_DISABLE_LITE;
    if(state->mode & 1){
        *isAdapt = EC_ADAPTATION_ENABLE;
        if(state->mode & 8)
            *isAdaptLite = EC_ADAPTATION_ENABLE_LITE;
        } else
            *isAdapt = EC_ADAPTATION_DISABLE;

        return 0;
    }

#ifdef Aec_sb_8ms_interface
/* Returns size of frame */
int ec_sb_GetFrameSize(IppPCMFrequency freq, int taptime_ms, int *s)
    {
    *s = SB_FRAME_SIZE+(int)freq-(int)freq+taptime_ms-taptime_ms;
    return 0;
    }
#else
int ec_sb_GetFrameSize(IppPCMFrequency freq, int taptime_ms, int *s)
    {
    *s = FRAME_SIZE_10ms+(int)freq-(int)freq+taptime_ms-taptime_ms;
    return 0;
    }
#endif

int ec_sb_GetSegNum(void * stat)
    {
    _sbECState *state=(_sbECState *)stat;
    return state->numSegments;
    }
int ec_sb_GetSubbandNum(void * stat)
    {
    _sbECState *state=(_sbECState *)stat;
    return state->numSubbands;
    }

/* Returns size of buffer for state structure */
Ipp32s ec_sb_GetSize(IppPCMFrequency freq, int taptime_ms, int *s, int *s1, int *scratch_size,int order)
    {
    int apOrder;
    Ipp32s size = 0, numSegments;
    Ipp32s pSizeSpec, pSizeInit, pSizeBuf;
    Ipp32s pSizeInit1, pSizeBuf1, csize;

    if(order<2)
        apOrder=0;
    else
        apOrder=order;
    size += ALIGN(sizeof(_sbECState));

    ippsFFTGetSize_R_32f(SB_FFT_ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintAccurate,
        &pSizeSpec, &pSizeInit, &pSizeBuf);

    size += ALIGN(pSizeSpec);

    ippsFFTGetSize_C_32fc(SB_FFT_ORDER, IPP_FFT_DIV_FWD_BY_N, ippAlgHintAccurate,
        &pSizeSpec, &pSizeInit1, &pSizeBuf1);

    size += ALIGN(pSizeSpec);

    csize = pSizeInit;
    if (csize < pSizeInit1)
        csize = pSizeInit1;
    if (csize < pSizeBuf)
        csize = pSizeBuf;
    if (csize < pSizeBuf1)
        csize = pSizeBuf1;

    size += ALIGN(csize);

    if (freq == IPP_PCM_FREQ_8000) {
        numSegments = (taptime_ms * 8 - 1) / SB_FRAME_SIZE + 1;
        } else if (freq == IPP_PCM_FREQ_16000) {
            numSegments = (taptime_ms * 16 - 1) / SB_FRAME_SIZE + 1;
        } else {
            return 1;
            }

        size += ALIGN((3 * numSegments+apOrder) * sizeof(Ipp32fc *));

        size += ALIGN((3 * numSegments+apOrder) * SB_SUBBANDS * sizeof(Ipp32fc));

        ippsSubbandControllerGetSize_EC_32f(SB_SUBBANDS, SB_FRAME_SIZE, numSegments, freq, &csize);

        size += ALIGN(csize);
        if (apOrder>1) {
            /************************************************************************/
            size += ALIGN((apOrder)*sizeof(Ipp32fc*));// for pPrevError
            size += ALIGN((apOrder)*SB_SUBBANDS* sizeof(Ipp32fc));// for pPrevError

            /************************************************************************/
            /************************************************************************/
            size += ALIGN((apOrder)*sizeof(Ipp64f*));// for pMu
            size += ALIGN((apOrder)*SB_SUBBANDS* sizeof(Ipp64f));// for pMu

            /************************************************************************/
            }
        *s = ALIGN(size+31);

        ah_GetSize(SB_FRAME_SIZE, &csize,freq,scratch_size);
        //shiftupCos_GetSize(FRAME_SIZE_10ms, &csize1);
        //csize += ALIGN(csize1);
        *s1 = ALIGN(csize+31);
        return 0;
    }

/* Returns delay introduced to send-path */
int ec_sb_GetSendPathDelay(int *delay)
    {
    *delay = SB_OFF;
    return 0;
    }

/* Initializes state structure */
Ipp32s ec_sb_Init(void * stat,void *ahObj, IppPCMFrequency freq, int taptime_ms,int apOrder)
    {
    _sbECState *state=(_sbECState *)stat;
    Ipp32s i, numSegments,ret;
    Ipp32s pSizeSpec, pSizeInit, pSizeBuf;
    Ipp32s pSizeInit1, pSizeBuf1, csize;
    Ipp8u  *ptr, *ptr0, *ptr1, *buf;
    if (state==0 || ahObj==0) {
        return 1;
        }
    ptr = (Ipp8u *)state;
    ptr += ALIGN(sizeof(_sbECState));

    /* Initialize FFTs */
    ippsFFTGetSize_R_32f(SB_FFT_ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintAccurate,
        &pSizeSpec, &pSizeInit, &pSizeBuf);
    ptr0 = ptr;
    ptr += ALIGN(pSizeSpec);

    ippsFFTGetSize_C_32fc(SB_FFT_ORDER, IPP_FFT_DIV_FWD_BY_N, ippAlgHintAccurate,
        &pSizeSpec, &pSizeInit1, &pSizeBuf1);
    ptr1 = ptr;
    ptr += ALIGN(pSizeSpec);

    ippsFFTInit_R_32f(&(state->spec_fft), SB_FFT_ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintAccurate,
        ptr0, ptr);
    ippsFFTInit_C_32fc(&(state->spec_fftC), SB_FFT_ORDER, IPP_FFT_DIV_FWD_BY_N, ippAlgHintAccurate,
        ptr1, ptr);

    /* Work buffer for FFT */
    state->pBuf = ptr;

    csize = pSizeInit;
    if (csize < pSizeInit1)
        csize = pSizeInit1;
    if (csize < pSizeBuf)
        csize = pSizeBuf;
    if (csize < pSizeBuf1)
        csize = pSizeBuf1;

    ptr += ALIGN(csize);

    state->FFTSize = (1<<SB_FFT_ORDER);
    state->numSubbands = SB_SUBBANDS;
    state->frameSize = SB_FRAME_SIZE;
    state->windowLen = SB_WIN_LEN;

    if (freq == IPP_PCM_FREQ_8000) {
        numSegments = (taptime_ms * 8 - 1) / state->frameSize + 1;
        } else if (freq == IPP_PCM_FREQ_16000) {
            numSegments = (taptime_ms * 16 - 1) / state->frameSize + 1;
        } else {
            return 1;
            }

        state->numSegments = numSegments;
        if (apOrder<2)
            state->ap_order = 0;
        else
            state->ap_order = apOrder;

        /* receive-in subband history */
        state->ppRinSubbandHistory = (Ipp32fc **)ptr;
        ptr += ALIGN((3 * numSegments+state->ap_order) * sizeof(Ipp32fc *));
        /* adaptive coeffs */
        state->ppAdaptiveCoefs = state->ppRinSubbandHistory + numSegments+state->ap_order;
        /* fixed coeffs */
        state->ppFixedCoefs = state->ppRinSubbandHistory + numSegments * 2+state->ap_order;

        buf = ptr;
        ptr += ALIGN((3 * numSegments+state->ap_order) * SB_SUBBANDS * sizeof(Ipp32fc));

        /* Zero coeffs buffer and history buffer */
        ippsZero_32fc((Ipp32fc *)buf, (3 * numSegments+state->ap_order) * SB_SUBBANDS);
        if (state->ap_order>1) {
            /************************************************************************/
            state->pPrevError = (Ipp32fc**)ptr;
            ptr += ALIGN((state->ap_order)*sizeof(Ipp32fc*));
            ippsZero_32fc((Ipp32fc*)ptr, (state->ap_order)*state->numSubbands);
            for (i = 0; i < state->ap_order; i++) {
                (state->pPrevError)[i] = (Ipp32fc*)ptr+i*state->numSubbands;
                }
            ptr += ALIGN((state->ap_order)*SB_SUBBANDS* sizeof(Ipp32fc));
            /************************************************************************/
            /************************************************************************/
            state->pMu = (Ipp64f**)ptr;
            ptr += ALIGN((state->ap_order)*sizeof(Ipp64f*));
            ippsZero_64f((Ipp64f*)ptr, (state->ap_order)*state->numSubbands);
            for (i = 0; i < state->ap_order; i++) {
                (state->pMu)[i] = (Ipp64f*)ptr+i*state->numSubbands;
                }
            ptr += ALIGN((state->ap_order)*SB_SUBBANDS* sizeof(Ipp64f));
            /************************************************************************/
            }
        /* Initialize receive-in subband history array */
        for (i = 0; i < numSegments+state->ap_order; i++) {
            (state->ppRinSubbandHistory)[i] = (Ipp32fc *)buf + i * (state->numSubbands);
            }
        buf += (numSegments+state->ap_order) * state->numSubbands * sizeof(Ipp32fc);

        /* Initialize adaptive coeffs array */
        for (i = 0; i < numSegments; i++) {
            (state->ppAdaptiveCoefs)[i] = (Ipp32fc *)buf + i * (state->numSubbands);
            }
        buf += numSegments * state->numSubbands * sizeof(Ipp32fc);

        /* Initialize fixed coeffs array */
        for (i = 0; i < numSegments; i++) {
            (state->ppFixedCoefs)[i] = (Ipp32fc *)buf + i * (state->numSubbands);
            }

        /* Initialize subband controller */
        state->controller = (IppsSubbandControllerState_EC_32f *)ptr;
        if (ippsSubbandControllerInit_EC_32f(state->controller, state->numSubbands, state->frameSize, numSegments, freq) != ippStsOk)
            return 1;

        ippsSubbandControllerGetSize_EC_32f(SB_SUBBANDS, SB_FRAME_SIZE, numSegments, freq, &csize);

        /* ptr += ALIGN(csize); */
        state->ah = (ahState *)IPP_ALIGNED_PTR(ahObj,16);
        ret=ah_Init(state->ah, freq, SB_FRAME_SIZE,23);
        if (ret) {
            return ret;
            }
        ah_GetSize(SB_FRAME_SIZE, &csize, freq,&state->scratch_mem_size);

        /* zero receive-in history buffer */
        ippsZero_32f(state->pRinHistory, state->windowLen);

        /* enable adaptation */
        state->mode = 1;
        state->nlp=1;
        state->ff=0;
        state->ffa=0;

#ifndef AEC_SB_8ms_interface
        state->instackLenSout=state->instackLenRin=state->instackLenSin=FRAME_SIZE_10ms+FRAME_SIZE_6ms;
        state->instackCurLenSout=0;
        state->instackCurLenRin=state->instackCurLenSin=FRAME_SIZE_6ms;
        ippsZero_32f(state->instackRin, FRAME_SIZE_10ms+FRAME_SIZE_6ms);
        ippsZero_32f(state->instackSin, FRAME_SIZE_10ms+FRAME_SIZE_6ms);
        ippsZero_32f(state->instackSout, FRAME_SIZE_10ms+FRAME_SIZE_6ms);
#endif
        return 0;
    }
/* Do operation or set mode */
int ec_sb_ModeOp(void * stat, ECOpcode op)
    {
    int i, j;
    _sbECState *state=(_sbECState *)stat;
    if (state == NULL)
        return 1;
    switch (op) {
    case (EC_COEFFS_ZERO):      /* Zero coeffs of filters */
        {
        for (i = 0; i < state->numSegments; i++)
            for (j = 0; j < state->numSubbands; j++)
                state->ppAdaptiveCoefs[i][j].re = state->ppAdaptiveCoefs[i][j].im =
                state->ppFixedCoefs[i][j].re = state->ppFixedCoefs[i][j].im = 0.f;
        }
        break;
    case(EC_ADAPTATION_ENABLE): /* Enable adaptation */
        if (!(state->mode & 1))
            ippsSubbandControllerReset_EC_32f(state->controller);
        state->mode |= 1;
        break;
    case(EC_ADAPTATION_ENABLE_LITE): /* Enable adaptation */
        if (!(state->mode & 1))
            ippsSubbandControllerReset_EC_32f(state->controller);
        state->mode |= 9; /* Enable adaptation + no constrain */
        break;
    case(EC_ADAPTATION_DISABLE): /* Disable adaptation */
        state->mode &= ~1;
        break;
    case(EC_ADAPTATION_DISABLE_LITE): /* Disable adaptation */
        state->mode &= ~8;
        break;
    case(EC_NLP_ENABLE):    /* Enable NLP */
        state->mode |= 2;
        ec_sb_setNR(state,1);
        ec_sb_setNRreal(state,4,0);
        break;
    case(EC_NLP_DISABLE):   /* Disable NLP */
        state->mode &= ~2;
        ec_sb_setNR(state,0);
        //        ec_sb_setNRreal(state,0);
        break;
    case(EC_TD_ENABLE):     /* Enable ToneDisabler */
        state->mode |= 4;
        break;
    case(EC_TD_DISABLE):    /* Disable ToneDisabler */
        state->mode &= ~4;
        break;
    case(EC_AH_ENABLE):     /* Enable howling control */
        //case(EC_AH_ENABLE1):     /* Enable howling control */
        ah_SetMode(state->ah,AH_ENABLE1);// set anti-howling on
        state->mode |= 16;
        break;
        //case(EC_AH_ENABLE2):     /* Enable howling control */
        //    ah_SetMode (state->ah,AH_ENABLE2);// set anti-howling on
        //    state->mode |= 32;
        //    break;
    case(EC_AH_DISABLE):    /* Disable howling control */
        ah_SetMode(state->ah,AH_DISABLE);// set anti-howling off
        state->mode &= ~16;
        state->mode &= ~32;
        break;
    default:
        break;
        }

    return 0;
    }
#define UpdCoefs_MAXLEN            65
int _ippsMeanColSqrtDev_32fc(const Ipp32fc** pSrc, int len, Ipp32fc* pStdDev,int height)
    {
    int    i;
    Ipp32fc mean[UpdCoefs_MAXLEN],diff[UpdCoefs_MAXLEN];
    /* reference code could be strongly optimized calculating inside one cycle
    and using "sigmas" previously calculated */
    ippsZero_32fc(mean,len);
    for ( i=0; i < height; i++ ) {
        //ippsCopy_32fc(pSrc[i],diff,len);
        //ippsSet_32fc(one,pSrc[i],len);
        ippsAdd_32fc(pSrc[i],mean,mean,len);
        }
    ippsDivC_32f((Ipp32f*)mean,(Ipp32f)height,(Ipp32f*)mean,len*2);// calc mean val
    ippsZero_32fc(diff,len);
    ippsZero_32fc(pStdDev,len);
    for ( i=0; i < height; i++ ) {
        ippsSub_32fc(pSrc[i],mean,diff,len);/* (u[i]-mean) */
        ippsMul_32fc(diff,diff,diff,len);           /* (u[i]-mean)*(u[i]-mean) */
        ippsAdd_32fc(diff,pStdDev,pStdDev,len);
        }
    ippsDivC_32f((Ipp32f*)pStdDev,(Ipp32f)height,(Ipp32f*)pStdDev,len*2);
    return 0;
    }
static void _ippsMagn_32fc64f(const Ipp32fc *src,Ipp64f *dst,int len){
    int j;
    for (j = 0; j < len; j++) {
        dst[j] = (Ipp64f)src[j].re*src[j].re+(Ipp64f)src[j].im*src[j].im;
        }
    //ippsSqrt_64f_I(dst,len);           /* (u[i]-mean)*(u[i]-mean) */
    return;
    }
//n = 0
//    sum = 0
//    sum_sqr = 0

//    foreach x in data:
//n = n + 1
//    sum = sum + x
//    sum_sqr = sum_sqr + x*x
//    end for

//    mean = sum/n
//    variance = (sum_sqr - sum*mean)/(n - 1)
int _ippsMeanColSqrtMagnDev_32fc_1(const Ipp32fc** pSrc, int len, Ipp64f* pStdDev,int height)
    {
    int    i;
    Ipp64f tmp[UpdCoefs_MAXLEN],mean[UpdCoefs_MAXLEN],diff[UpdCoefs_MAXLEN];
    /* reference code could be optimized calculating inside one cycle
    and using "sigmas" previously calculated */
    ippsZero_64f(mean,len);
    for ( i=0; i < height; i++ ) {
        _ippsMagn_32fc64f(pSrc[i],tmp,len);
        ippsAdd_64f(tmp,mean,mean,len);
        }
    ippsDivC_64f(mean,(Ipp64f)height,mean,len);// calc mean val
    ippsZero_64f(diff,len);
    ippsZero_64f(pStdDev,len);
    for ( i=0; i < height; i++ ) {
        _ippsMagn_32fc64f(pSrc[i],tmp,len);
        ippsSub_64f(tmp,mean,diff,len);/* (u[i]-mean) */
        ippsMul_64f(diff,diff,diff,len);           /* (u[i]-mean)*(u[i]-mean) */
        ippsSqrt_64f(diff,diff,len);           /* (u[i]-mean)*(u[i]-mean) */
        ippsAdd_64f(diff,pStdDev,pStdDev,len);
        }
    ippsDivC_64f(pStdDev,(Ipp64f)height,pStdDev,len);
    return 0;
    }
int _ippsMeanColSqrtMagnDev_32fcxx(const Ipp32fc** pSrc, int len, Ipp64f* pStdDev,int height)
    {
    int    i,j;
    Ipp64f tmp[UpdCoefs_MAXLEN],mean[UpdCoefs_MAXLEN],diff[UpdCoefs_MAXLEN];
    /* reference code could be optimized calculating inside one cycle
    and using "sigmas" previously calculated */
    ippsZero_64f(mean,len);
    for ( i=0; i < height; i++ ) {
        _ippsMagn_32fc64f(pSrc[i],tmp,len);
        ippsAdd_64f(tmp,mean,mean,len);
        }
    ippsDivC_64f(mean,(Ipp64f)height,mean,len);// calc mean val
    ippsZero_64f(pStdDev,len);
    for ( i=0; i < height; i++ ) {
        _ippsMagn_32fc64f(pSrc[i],tmp,len);
        ippsSub_64f(tmp,mean,diff,len);/* (u[i]-mean) */
        for ( j=0; j < len; j++ ) {
            diff[j]=sqrt(fabs(diff[j]));
            }
        ippsAdd_64f(diff,pStdDev,pStdDev,len);
        }
    //ippsDivC_64f_I((Ipp64f)height,pStdDev,len);
    return 0;
    }

int _ippsMeanColNormSubDev_32fcx(const Ipp32fc** pSrc, int len, Ipp64f* pStdDev,int height)
    {
    int    i,j;
    Ipp64fc tmp,mean[UpdCoefs_MAXLEN];
    Ipp64f  diff[UpdCoefs_MAXLEN];
    ippsZero_64fc(mean,len);
    for ( i=0; i < height; i++ ) {
        for ( j=0; j < len; j++ ) {
            mean[j].re+=pSrc[i][j].re;
            mean[j].im+=pSrc[i][j].im;
            }
        }
    ippsDivC_64f((Ipp64f*)mean,height,(Ipp64f*)mean,len*2);
    ippsZero_64f(pStdDev,len);
    for ( i=0; i < height; i++ ) {
        for ( j=0; j < len; j++ ) {
            tmp.re=pSrc[i][j].re-mean[j].re;
            tmp.im=pSrc[i][j].im-mean[j].im;
            diff[j]=(tmp.re*tmp.re+tmp.im*tmp.im);//sqrt
            }
        ippsAdd_64f(diff,pStdDev,pStdDev,len);
        }
    //ippsSqrt_64f_I(pStdDev,len);
    return 0;
    }
int _ippsMeanColNormSubDev_32fc(const Ipp32fc** pSrc, int len, Ipp64f* pStdDev,int height)
    {
    int    i,j;
    Ipp64fc tmp;//,mean[UpdCoefs_MAXLEN]
    Ipp64f  diff[UpdCoefs_MAXLEN];
    //ippsZero_64fc(mean,len);
    //for ( i=0; i < height; i++ ) {
    //    for ( j=0; j < len; j++ ) {
    //        mean[j].re+=pSrc[i][j].re;
    //        mean[j].im+=pSrc[i][j].im;
    //    }
    //}
    //ippsDivC_64f_I(height,(Ipp64f*)mean,len*2);
    ippsZero_64f(pStdDev,len);
    for ( i=0; i < height; i++ ) {
        for ( j=0; j < len; j++ ) {
            tmp.re=pSrc[i][j].re;
            tmp.im=pSrc[i][j].im;
            diff[j]=(tmp.re*tmp.re+tmp.im*tmp.im);
            }
        ippsAdd_64f(diff,pStdDev,pStdDev,len);
        }
    return 0;
    }
static IppStatus ownFIRSubbandCoeffUpdate_EC_32fc_I(
    const double   *mu,
    const Ipp32fc **u,
    const Ipp32fc  *err,
    Ipp32fc       **s,
    int             nSeg,
    int             len, int height)
    {
    int     k,i,l,j;
    Ipp64fc tmp2[UpdCoefs_MAXLEN];
    Ipp32fc sigma[UpdCoefs_MAXLEN];
    Ipp32fc tmp4[UpdCoefs_MAXLEN];
    Ipp32f  tmp5[UpdCoefs_MAXLEN];
    Ipp32fc sum[UpdCoefs_MAXLEN];
    Ipp32fc tmp6[UpdCoefs_MAXLEN];

    for(i = 0; i < nSeg; i++) {
        /* calc sigmas */
        //_ippsMeanColSqrtMagnDev_32fcxx(&u[i],len,sigma,height);
        for(k=0; k < len; k ++) {// calc sigma conjugate for divide
            tmp2[k].re = sigma[k].re;
            tmp2[k].im = -sigma[k].im;
            }
        for(k=0; k < len; k++) {// sigma mult by conjugate
            tmp5[k]=sigma[k].re*sigma[k].re+sigma[k].im*sigma[k].im;
            }
        ippsZero_32fc(sum,len);
        for(l=0; l < height; l++) {
            for(j=0; j < len; j++) {
                tmp6[j].re = u[i+l][j].re;
                tmp6[j].im = -u[i+l][j].im;
                }
            ippsMul_32fc(tmp6,&err[i+l],tmp4,len); /* (u*err) */
            ippsMul_32fc((const Ipp32fc*)tmp2,tmp4,tmp4,len);       /* (u*err*conj(sigma)) */
            for(k=0; k < len; k ++) {               /* divide by (sigma*conj(sigma)) */
                tmp4[k].re/=tmp5[k];
                tmp4[k].im/=tmp5[k];
                }
            ippsAdd_32fc(tmp4,sum,sum,len);
            }
        for(k=0; k < len; k++) {
            s[i][k].re =(float)(s[i][k].re +sum[k].re*mu[k]);//
            s[i][k].im =(float)(s[i][k].im +sum[k].im*mu[k]);//
            }
        }
    return(ippStsNoErr);
    }
IppStatus _ippsFIRSubbandAPCoeffUpdate_EC_32fc_I(const Ipp64f** ppSrcStepSize, 
                                                 const Ipp32fc** ppSrcFilterInput, 
                                                 const Ipp32fc** ppSrcError,
                                                 Ipp32fc** ppSrcDstCoefs,
                                                 Ipp32u numSegments,
                                                 Ipp32u len,
                                                 Ipp32u apOrder)
    {

    Ipp32u  i,j,l,k;
    Ipp64fc conj,tmp2[UpdCoefs_MAXLEN];
    Ipp32f  invOrder;
    invOrder=1.f/apOrder;
    for(i = 0; i < numSegments; i ++) {
        ippsZero_64fc(tmp2,len);
        for(k=i,l=0; l < apOrder; k++,l++) {
            for(j=0; j < len; j ++) {
                conj.re = ppSrcFilterInput[k][j].re;
                conj.im = -ppSrcFilterInput[k][j].im;
                tmp2[j].re += ((Ipp64f)ppSrcError[l][j].re*conj.re -(Ipp64f)ppSrcError[l][j].im*conj.im)*ppSrcStepSize[l][j];
                tmp2[j].im += ((Ipp64f)ppSrcError[l][j].re*conj.im +(Ipp64f)ppSrcError[l][j].im*conj.re)*ppSrcStepSize[l][j];
                }
            }
        for(j = 0; j < len; j++){
            ppSrcDstCoefs[i][j].re +=(Ipp32f)(tmp2[j].re*invOrder);
            ppSrcDstCoefs[i][j].im +=(Ipp32f)(tmp2[j].im*invOrder);
            }
        }
    return(ippStsNoErr);
    }
IppStatus _ippsSubbandAPControllerUpdate_EC_32f(const Ipp32fc **ppSrcRinSubbandsHistory, const Ipp32fc *pSrcSinSubbands,
                                                double *pDstStepSize, Ipp32f learningRate, SubbandControllerState_EC_32f *pState)
    {
    int i;
    const Ipp32fc *pSegment;
    Ipp32f pwr, t;
    Ipp32f *rin_pwr;/*, *rin_pwr_n;*/
    Ipp32f rin_pwr_min;
    Ipp32f stepSize_max;
    int numSubbands;

    pState = IPP_ALIGNED_PTR(pState, 16);

    rin_pwr_min = pState->rin_pwr_min;
    stepSize_max = pState->stepSize_max;
    rin_pwr = pState->rin_pwr;
    /*    rin_pwr_n = pState->rin_pwr_n;*/
    numSubbands = pState->numSubbands;

    pSegment = ppSrcRinSubbandsHistory[0];

    for (i = 0; i < numSubbands; i++) {
        rin_pwr[i] +=
            (pSegment[i].re * pSegment[i].re + pSegment[i].im * pSegment[i].im);
        /*        pwr += rin_pwr[i];*/
        }

    /*    pwr *= pState->pwr_b;*/

    for (i = 0; i < numSubbands; i++) {
        t = rin_pwr[i];
        /*        if (t < 0.01 * pwr)
        t = pwr;*/
        if (t > rin_pwr_min)
            pDstStepSize[i] = learningRate / t;
        else
            pDstStepSize[i] = stepSize_max;
        }

    pSegment = ppSrcRinSubbandsHistory[pState->numSegments - 1];

    for (i = 0; i < numSubbands; i++) {
        rin_pwr[i] -=
            (pSegment[i].re * pSegment[i].re + pSegment[i].im * pSegment[i].im);
        if (pState->rin_pwr[i] < 0) pState->rin_pwr[i] = 0;
        }

    pwr = 0;
    if (pSrcSinSubbands) {
        pState->sub_use = 1;
        for (i = 0; i < numSubbands; i++)  {
            pwr += pSrcSinSubbands[i].re * pSrcSinSubbands[i].re + pSrcSinSubbands[i].im * pSrcSinSubbands[i].im;
            }
        }

    ADDWEIGHTED(pState->sin_pwr, pwr, pState->coef_werr);

    return ippStsOk;
    }
static int processFrame(void *stat, Ipp32f *rin, Ipp32f *sin, Ipp32f *sout)//,Ipp32f mu
    {
    _sbECState *state=(_sbECState *)stat;
    Ipp32s i, k, s;
    Ipp32fc* pSegment,*pErr; /* Pointer to segment of input history */
    Ipp64f * pMu;
    Ipp32s iSegment;
    Ipp32f *pRinHistory,sgain;
    Ipp32s numSegments;
    Ipp32s windowLen;
    Ipp32s frameSize;
    LOCAL_ALIGN_ARRAY(32,Ipp32fc,fira_err,SB_SUBBANDS,state);
    LOCAL_ALIGN_ARRAY(32,Ipp32fc,firf_err,SB_SUBBANDS,state);
    LOCAL_ALIGN_ARRAY(32,Ipp32fc,fira_out,SB_SUBBANDS,state);
    LOCAL_ALIGN_ARRAY(32,Ipp32fc,firf_out,SB_SUBBANDS,state);
    LOCAL_ALIGN_ARRAY(32,Ipp64f,pStepSize,SB_SUBBANDS*2,state);
    LOCAL_ALIGN_ARRAY(32,Ipp32f,pTimeFiltOut,SB_FFT_LEN,state);
    LOCAL_ALIGN_ARRAY(32,Ipp32f,pTimeError,SB_FFT_LEN,state);

    pRinHistory = state->pRinHistory;
    numSegments = state->numSegments;
    windowLen = state->windowLen;
    frameSize = state->frameSize;

    /* update receive-in subband history buffer */
    pSegment = state->ppRinSubbandHistory[state->numSegments-1+state->ap_order];
    for(iSegment = state->numSegments-1+state->ap_order; iSegment > 0; iSegment--)
        {
        state->ppRinSubbandHistory[iSegment] = state->ppRinSubbandHistory[iSegment-1];
        }
    state->ppRinSubbandHistory[0] = pSegment;

    /* update error subband history buffer */
    if (state->ap_order>1) {
        pErr = state->pPrevError[state->ap_order-1];
        pMu = state->pMu[state->ap_order-1];
        for(i = state->ap_order-1; i > 0; i--)
            {
            state->pPrevError[i] = state->pPrevError[i-1];
            state->pMu[i] = state->pMu[i-1];
            }
        state->pPrevError[0] = pErr;
        state->pMu[0] = pMu;
        }
    /* update receive-in history buffer */
    ippsMove_32f(&pRinHistory[frameSize],pRinHistory,windowLen - frameSize);
    ippsMove_32f(rin,&pRinHistory[windowLen - frameSize],frameSize);
    /* Apply FFT to receive-in signal */
    ippsFFTFwd_RToCCS_32f(pRinHistory, (Ipp32f *)(state->ppRinSubbandHistory[0]), state->spec_fft, state->pBuf);

    /* Do filtering with fixed coeffs */
    ippsFIRSubband_EC_32fc(state->ppRinSubbandHistory, state->ppFixedCoefs, firf_out,
        numSegments, state->numSubbands);

    /* Get fixed filter output in time domain: apply inverse FFT */
    ippsFFTInv_CCSToR_32f((Ipp32f *)firf_out, pTimeFiltOut, state->spec_fft, state->pBuf);

    /* First half of error is set to zero. */
    ippsZero_32f(pTimeError, SB_FRAME_SIZE);

    /* Subtract microphone signal from fixed filter output. */
    if ((state->ah->dcOffset<100) && (state->ah->dcOffset>-100))
    {
        for (i = 0; i < SB_FRAME_SIZE; i++) {
            state->ff=state->ff*0.5f+0.5f*pTimeFiltOut[SB_FRAME_SIZE + i];
            pTimeError[SB_FRAME_SIZE + i] = sin[i] - pTimeFiltOut[SB_FRAME_SIZE + i];
            sout[i] = pTimeError[SB_FRAME_SIZE + i];
        }
    } 
    else
    {
        for (i = 0; i < SB_FRAME_SIZE; i++) {
            state->ff=state->ff*0.5f+0.5f*pTimeFiltOut[SB_FRAME_SIZE + i];
            pTimeError[SB_FRAME_SIZE + i] = sin[i] - state->ff;
            sout[i] = pTimeError[SB_FRAME_SIZE + i];
        }
    }
    if (ADAPTATION_ENABLED) // Adaptation Enabled
        {
        /* Get fixed filter error in frequency domain */
        ippsFFTFwd_RToCCS_32f(pTimeError, (Ipp32f *)firf_err, state->spec_fft, state->pBuf);

        /* Do filtering with adaptive coeffs */
        ippsFIRSubband_EC_32fc(state->ppRinSubbandHistory, state->ppAdaptiveCoefs, fira_out,
            numSegments, state->numSubbands);
        /* Get adaptive filter output in time domain */
        ippsFFTInv_CCSToR_32f((Ipp32f *)fira_out, pTimeFiltOut, state->spec_fft, state->pBuf);

        /* Subtract microphone signal from adaptive filter output. */
        for (i = 0; i < SB_FRAME_SIZE; i++) {
            pTimeError[SB_FRAME_SIZE + i] = sin[i] - pTimeFiltOut[SB_FRAME_SIZE + i];
            }

        ippsFFTFwd_RToCCS_32f(pTimeError, (Ipp32f *)fira_err, state->spec_fft, state->pBuf);

        if (state->ap_order>1) {
            ippsCopy_32fc(fira_err,state->pPrevError[0],SB_SUBBANDS);
            /* Update subband controller */
            _ippsSubbandAPControllerUpdate_EC_32f((const Ipp32fc **)(state->ppRinSubbandHistory),
                0, pStepSize,0.75, state->controller);
            ippsCopy_64f(pStepSize,state->pMu[0],SB_SUBBANDS);
            /* Update adaptive coeffs */
            _ippsFIRSubbandAPCoeffUpdate_EC_32fc_I(state->pMu, (const Ipp32fc **)(state->ppRinSubbandHistory),
                state->pPrevError,
                state->ppAdaptiveCoefs, numSegments, state->numSubbands,state->ap_order);//,mu
            }else{
                /* Update subband controller */
                ippsSubbandControllerUpdate_EC_32f(rin, sin, (const Ipp32fc **)(state->ppRinSubbandHistory),
                    0, pStepSize, state->controller);
                ippsFIRSubbandCoeffUpdate_EC_32fc_I(pStepSize, (const Ipp32fc **)(state->ppRinSubbandHistory), fira_err,
                    state->ppAdaptiveCoefs, numSegments, state->numSubbands);
            }
        /* Constrain coeffs. */
        for (i = 0; i < numSegments; i++) {
            Ipp32f bsrc[SB_SUBBANDS * 4];
            Ipp32f bsrc1[SB_SUBBANDS * 4];
            Ipp32f bdst[SB_FFT_LEN];

            ippsFFTInv_CCSToR_32f((Ipp32f *)(state->ppAdaptiveCoefs[i]), bdst, state->spec_fft, state->pBuf);

            ippsZero_32f(bsrc1,SB_SUBBANDS * 4);
            for (k = 0; k < SB_FRAME_SIZE; k++) {
                bsrc1[2 * k] = SB_FFT_LEN * bdst[k];
                }

            ippsFFTFwd_CToC_32fc((const Ipp32fc *)bsrc1, (Ipp32fc *)bsrc, state->spec_fftC, state->pBuf);

            for (s = 0; s < SB_SUBBANDS; s++) {
                state->ppAdaptiveCoefs[i][s].re = bsrc[2 * s];
                state->ppAdaptiveCoefs[i][s].im = bsrc[2 * s + 1];
                }
            }
        /* Apply subband controller */
        //ownSubbandController_EC_32f(fira_err, firf_err, state->ppAdaptiveCoefs, state->ppFixedCoefs,
        //    &state->sgain,state->controller,dtFlag);
        ippsSubbandController_EC_32f(fira_err, firf_err, state->ppAdaptiveCoefs, state->ppFixedCoefs,
            &sgain, state->controller);
        }
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,pTimeError,SB_FFT_LEN,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,pTimeFiltOut,SB_FFT_LEN,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp64f,pStepSize,SB_SUBBANDS*2,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32fc,firf_out,SB_SUBBANDS,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32fc,fira_out,SB_SUBBANDS,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32fc,firf_err,SB_SUBBANDS,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32fc,fira_err,SB_SUBBANDS,state);
    return 0;
    }
#ifdef AEC_SB_8ms_interface
int ec_sb_ProcessFrame(void * stat, Ipp32f *frin, Ipp32f *fsin, Ipp32f *fsout)
    {
    _sbECState *state=(_sbECState *)stat;
    int     ret=0,nlpOff=0;
    //state->ah->Mem=state->Mem;
    state->ah->Mem.base=state->Mem.base;
    state->ah->Mem.CurPtr=state->Mem.CurPtr;
    state->ah->Mem.VecPtr=state->Mem.VecPtr;
    state->ah->Mem.offset=state->Mem.offset;
    if (TD_ENABLED){
        nlpOff=toneDisabler_32f(frin, fsin, state->ah->samplingRate, SB_FRAME_SIZE,
            &state->ah->td);
        }
    if(nlpOff){
        ippsCopy_32f(fsin,fsout,SB_FRAME_SIZE);
        }else{
            ret=processFrame(state, frin, fsin, fsout);
            if (ret)
                return ret;
            cnr_32f(state->ah,frin,fsin,fsout,SB_FRAME_SIZE,state->nlp,state->cng,23,SB_OFF);
        }
    return ret;
    }
#else
static void put_frame(Ipp32f *histSout,int histSoutLen,int *histSoutCurLen, Ipp32f *sout,int len){
    if(((*histSoutCurLen)+len) <= histSoutLen){
      ippsCopy_32f(sout,histSout+(*histSoutCurLen),len);
      (*histSoutCurLen)=((*histSoutCurLen)+len);
      return;
      }else{
          if((*histSoutCurLen) < histSoutLen){
              ippsMove_32f(histSout+(len-(histSoutLen-(*histSoutCurLen))),histSout,
                  (*histSoutCurLen)-(len-(histSoutLen-(*histSoutCurLen))));
              ippsCopy_32f(sout,histSout+histSoutLen-len,len);
              (*histSoutCurLen)=histSoutLen;
              return;
              }
      }
    if(histSoutLen>len){
      ippsMove_32f(histSout+len,histSout,
          histSoutLen-len);
      }
    ippsCopy_32f(sout,histSout+histSoutLen-len,
      len);
    return;
}
static int get_frame(Ipp32f *histSout,int *histSoutCurLen, Ipp32f *sout,int len){
    if((*histSoutCurLen) >= len){
     ippsCopy_32f(histSout,sout,len);
     (*histSoutCurLen)=((*histSoutCurLen)-len);
     if (*histSoutCurLen) {
         ippsMove_32f(histSout+len,histSout,*histSoutCurLen);
         }
     return 1;
     }
    return 0;
}
int ec_sb_ProcessFrame(void * stat, Ipp32f *rin, Ipp32f *sin, Ipp32f *sout)//,Ipp32f mu
    {
    _sbECState *state=(_sbECState *)stat;
    int     ret=0,nlpOff=0,i;
    LOCAL_ALIGN_ARRAY(32,Ipp32f,rin_sb,SB_FRAME_SIZE,state);
    LOCAL_ALIGN_ARRAY(32,Ipp32f,sin_sb,SB_FRAME_SIZE,state);
    LOCAL_ALIGN_ARRAY(32,Ipp32f,sout_sb,SB_FRAME_SIZE,state);
    //state->ah->Mem=state->Mem;
    state->ah->Mem.base=state->Mem.base;
    state->ah->Mem.CurPtr=state->Mem.CurPtr;
    state->ah->Mem.VecPtr=state->Mem.VecPtr;
    state->ah->Mem.offset=state->Mem.offset;
    put_frame(state->instackRin,state->instackLenRin,&state->instackCurLenRin,
        rin,FRAME_SIZE_10ms);
    put_frame(state->instackSin,state->instackLenSin,&state->instackCurLenSin,
        sin,FRAME_SIZE_10ms);
    while (get_frame(state->instackRin,&state->instackCurLenRin,rin_sb,SB_FRAME_SIZE)) {
        get_frame(state->instackSin,&state->instackCurLenSin,sin_sb,SB_FRAME_SIZE);
        if (TD_ENABLED){
            nlpOff=toneDisabler_32f(rin_sb, sin_sb, state->ah->samplingRate, SB_FRAME_SIZE,
                &state->ah->td);
            }
        state->ah->frameNum++;
        if(state->ah->frameNum*state->frameSize*1000/state->ah->samplingRate>10150){//43307
            //if(state->ah->frameNum==2136){
            //if(state->ah->frameNum==982){
            state->ah->frameNum++;
            state->ah->frameNum-=1;
            }
        if(nlpOff){
            ippsCopy_32f(sin_sb,sout_sb,SB_FRAME_SIZE);
            }else{
                ret=processFrame(state, rin_sb, sin_sb, sout_sb);//,0 mu
                if (ret)
                    break;
                for (i=0;i<SB_FRAME_SIZE;i+=NLP_FRAME_SIZE) {
                    cnr_32f(state->ah,&rin_sb[i],&sin_sb[i],&sout_sb[i],state->nlp,state->cng,0);
                    }
            }
        put_frame(state->instackSout,state->instackLenSout,&state->instackCurLenSout,
            sout_sb,SB_FRAME_SIZE);
        }
    get_frame(state->instackSout,&state->instackCurLenSout,sout,FRAME_SIZE_10ms);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,sout_sb,SB_FRAME_SIZE,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,sin_sb,SB_FRAME_SIZE,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,rin_sb,SB_FRAME_SIZE,state);
    return ret;
    }
#endif
