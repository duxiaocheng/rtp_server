/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2011 Intel Corporation. All Rights Reserved.
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
//  Purpose: Echo Canceller, fullband algorithm
//
*/

#include <stdlib.h>
#include <ipps.h>
#include <ippsc.h>
#include "scratchmem.h"
#include "aux_fnxs.h"
#include "nlp_api.h"
#include "ec_api.h"
#include "ec_td.h"
#include "ownah.h"
#include "ah_api.h"
#include "ownec.h"
static Ipp32s ownNLMS_EC_32f(const Ipp32f *pSrcSpchRef,
                             const Ipp32f *pSrcSpch,
                             const Ipp32f *pStepSize,
                             Ipp32f *pDstSpch,      Ipp32s len,
                             Ipp32f *pSrcDstCoeffs,
                             Ipp32s tapLength,
                             Ipp32f *pErr,Ipp32f *pErrToWgts)
{
    Ipp32f estimatedEcho;
    Ipp32f err,errToWeight,tmp;
    Ipp32s i, j;
    err = *pErr;
    for(i = 0; i < len; i++){
        estimatedEcho = 0;
        for(j = 0; j < tapLength; j++){
            errToWeight=err;
            tmp=(pErrToWgts[j]*errToWeight);
            if (tmp>0){
                errToWeight*=2;
            }else
                if (tmp<0)
                    errToWeight= errToWeight*20/15;
            pErrToWgts[j]= errToWeight;
            pSrcDstCoeffs[j] +=pSrcSpchRef[i-j-1]*errToWeight;
            estimatedEcho += pSrcSpchRef[i-j] * pSrcDstCoeffs[j];
        }
        pDstSpch[i] = pSrcSpch[i] - estimatedEcho;
        err = pDstSpch[i] * pStepSize[i];
    }
    *pErr = err;

    return 0;
}

#define COEFF_4_3 21  //(4<<4)/3

//#ifndef IPP_OLD_QNLMS
static int nl (const Ipp16s *prevInput,
      const Ipp16s *pSrcSpch,
      const Ipp32s *pStepSize,
      Ipp16s *pDstSpch,
      int len,
      Ipp16s *pWeights,
      int tapLength,
      Ipp16s *pErr,Ipp32s *pErrToWgts){

    Ipp32s errToWeight=0;
    Ipp64s estimatedEcho,tmp2;
    int i, j,terr,err;
    err = *pErr;
    terr = (*pErr)<<11;
    for(i = 0; i < len; i++){
        estimatedEcho = 0;
        for(j = 0; j < tapLength; j++) {
            errToWeight=terr;
            tmp2=(Ipp64s)pErrToWgts[j]*terr;
            if (tmp2>0) {
                errToWeight=Cnvrt_64s32s((Ipp64s)errToWeight<<1);
            }else
                if (tmp2<0)
                   errToWeight= Cnvrt_64s32s(((Ipp64s)errToWeight*COEFF_4_3)>>4);
            pErrToWgts[j]= (Ipp32s)(((Ipp64s)errToWeight+ (1<<10)) >> 11);
            tmp2=(((Ipp64s)errToWeight * prevInput[i-j-1]+ (1<<23)) >> 24);
            tmp2+= pWeights[j];
            pWeights[j] = (Ipp16s)tmp2;
            estimatedEcho += prevInput[i-j]*pWeights[j];
        }
        estimatedEcho >>= 15;
        pDstSpch[i] = Cnvrt_32s16s((Ipp32s)((Ipp64s)pSrcSpch[i] - estimatedEcho));
        terr = pDstSpch[i] * pStepSize[i];//~27-28 order
        err = (terr+ (1<<10)) >> 11;//38-11=27
    }
    *pErr = Cnvrt_32s16s(err);
   return 0;
}
static Ipp16s initfc_l512[512]={
34,
8,178,30,-232,-115,43,55,-31,-72,62,-43,-60,71,16,-84,-73,-58,111,49,-161,
81,31,-68,-36,129,-84,-138,258,-68,-262,280,33,-358,46,196,-205,13,362,-490,284,
990,-1562,-2960,1726,4812,472,-2088,-96,-848,-3493,-1692,1652,1184,1278,3432,2740,-255,-1588,-2074,-3172,
-2197,-116,770,1078,1872,2078,826,286,51,-1229,-1684,-880,-502,-311,328,731,412,628,968,482,
-250,-739,-1037,-797,-534,-45,265,600,548,468,270,39,-4,-51,-300,-578,-514,-105,179,222,
31,106,263,169,-87,-81,95,-56,-93,-29,40,-55,-27,152,96,98,313,282,-80,-156,
-5,-110,-307,-152,-45,120,348,395,-22,-123,60,-156,-416,40,216,33,-49,15,-368,-122,
244,-139,-188,339,143,-365,-20,200,-327,-279,288,206,-144,125,121,-384,-275,37,-14,-160,
175,236,-18,6,60,-146,-160,234,119,-74,-25,122,-243,-212,2,110,13,90,149,-28,
-3,28,73,-5,-14,-63,-11,108,-57,-29,36,90,-4,50,94,-96,-125,12,-41,-16,
-78,69,14,-82,-11,47,75,-68,-38,-20,25,-92,87,50,-159,-114,40,1,11,-56,
-16,43,31,74,-24,-10,-183,-94,-87,-51,-40,82,56,-32,21,16,-8,-117,-58,-76,
-75,86,42,143,62,-48,-102,-13,61,27,20,100,133,-114,-52,132,-102,-90,20,94,
119,16,-45,106,132,14,-147,13,36,118,-48,56,61,32,49,124,34,-2,-36,25,
-34,-28,-41,-132,54,59,44,-11,17,-31,-72,-102,22,-40,-53,-48,5,49,-36,-8,
120,147,-93,34,99,-27,-36,16,-75,-49,41,24,-2,166,44,4,84,55,-66,-44,
50,78,-29,-56,-16,27,-74,12,-37,72,42,-39,-56,71,-51,9,-12,60,51,-23,
-13,12,89,-3,53,13,-40,-106,-35,-57,-62,-22,-18,8,-13,-32,24,49,-62,-29,
59,-66,-27,55,10,-81,9,52,-93,-41,42,-44,-35,10,93,25,-51,-57,-14,-63,
-62,83,34,-72,-21,148,9,-92,78,53,49,13,44,90,-66,-30,6,54,-29,39,
3,-23,-20,-48,-58,-24,11,-52,-35,-26,67,54,-34,53,41,-39,-27,-51,8,2,
29,4,-65,16,-108,-20,7,14,-62,-70,97,-29,-36,9,54,-24,14,62,80,-53,
-50,12,25,-11,-20,66,-46,-68,38,72,-79,-74,31,30,-64,75,68,-58,-13,48,
35,-43,87,-20,-62,-39,12,-34,-17,0,17,-8,43,79,-48,8,-22,-52,38,-18,
30,106,40,-41,16,16,-83,-60,27,-49,-97,-37,-18,31,-27,23,44,1,7,54,
-80,-14,21,-131,125,36,-31,53,43,-107,113,};
/* iter 1 rin up 6dB*/
static Ipp16s initfc_l1312[1312]={
154,
-318,-143,236,-25,61,-216,-53,-174,-123,-132,-24,-1,-19,-34,-220,119,41,-266,127,65,
-109,268,-182,38,-164,266,-222,168,-73,290,-279,220,-63,124,72,312,52,214,221,-268,
135,-88,26,67,-233,42,-197,-49,-69,99,43,55,84,29,154,189,-44,311,41,-191,
23,-106,-95,120,-229,184,-203,103,-166,363,-92,7,-87,193,-94,92,-138,218,60,143,
-107,132,221,-177,-17,111,185,-165,123,-284,104,5,178,-187,142,0,154,118,40,-124,
-143,-49,-58,87,14,80,-235,138,-183,-165,88,-57,-77,318,-290,219,-266,106,-295,302,
-108,356,2,-49,-167,223,71,41,25,40,57,-215,525,-291,230,37,514,-111,231,-254,
59,-19,-133,-53,-249,-256,16,-4,90,329,1295,28,-6536,-1531,16828,499,-11450,535,906,-2650,
-8786,7515,7536,-2466,7283,-166,-58,-5052,-5018,-4072,-5094,8285,-627,2608,7873,-4611,1493,1458,-3541,-1438,
-1493,-1239,-2411,4913,218,262,4937,-6277,3983,148,-6291,5780,-2748,-1408,2922,-371,-366,357,47,289,
-855,372,1155,-947,1423,464,-1702,466,-1232,-1979,1956,-178,-1103,3059,-1452,665,2846,-3454,2513,-1650,
-2299,2632,-1783,-95,1596,-934,325,571,-1184,486,1186,-1840,955,-679,-1854,1465,-1504,375,2199,-1480,
1266,1296,-1750,-43,516,-2150,548,301,-288,2398,-794,21,1908,-1819,134,-157,-946,913,-552,-30,
1148,-775,-266,256,-1048,213,361,-940,809,134,206,737,-459,537,701,-698,303,-522,-737,-78,
29,-107,270,-479,-116,117,-264,-98,88,-686,343,-397,-260,828,-476,876,198,-291,597,52,
-755,371,-494,-478,-44,104,144,479,93,105,162,-213,-113,-269,-368,-202,-19,16,332,-225,
75,170,-321,146,182,-169,342,93,440,-59,412,5,96,202,32,-49,-170,-159,81,-403,
-667,-103,-240,139,-77,-12,-75,-95,-31,382,-357,-15,51,-180,450,-83,565,-282,304,146,
-94,-68,-139,-282,203,104,-197,224,143,-89,-243,215,-384,-197,-66,7,-165,73,447,-174,
-5,-279,-156,-95,-16,-180,173,261,44,-119,504,-251,382,-159,239,334,56,160,-430,116,
-410,-303,210,-462,20,408,-3,537,227,243,238,133,-231,-324,8,-11,13,165,102,137,
-350,474,-201,-102,-228,65,-4,-3,-128,187,-240,150,5,-254,-52,358,-237,-335,296,160,
-346,521,367,-142,-127,162,91,-462,512,-35,-36,509,21,45,85,-245,111,-359,15,-177,
-103,-80,69,252,-133,120,19,261,-345,192,218,344,-572,702,-27,235,-311,-155,267,-209,
131,198,58,-128,-127,352,-65,195,276,127,-216,-174,-26,-129,-76,245,-196,187,47,-135,
54,-96,-38,-212,-84,-215,59,-350,-115,222,-153,-65,284,136,-35,316,193,-138,351,-385,
-193,-124,-134,39,-269,-104,182,-59,-52,94,124,22,263,-12,186,52,-106,355,51,3,
72,-37,-134,-1,35,-76,108,-230,188,187,283,25,-43,-13,67,-210,89,-243,151,-133,
-42,195,91,-119,115,-146,-14,-78,163,120,-218,-18,-313,243,35,-99,250,-119,-42,80,
242,-89,470,84,-323,86,22,68,-253,15,295,122,204,-6,-152,207,-217,-32,-21,97,
-159,58,-38,-233,-175,177,-22,174,-170,57,266,-6,12,-51,-49,-73,72,-277,8,-45,
-126,-200,219,-160,-93,83,209,-376,73,-396,167,63,151,-232,24,-179,-11,-154,-42,-39,
-53,335,272,-38,474,21,139,56,123,-71,-274,154,95,-163,103,-21,-40,-215,-59,-60,
192,-37,138,-85,-114,54,-326,115,-40,-1,35,-166,225,-148,-74,195,-86,110,-35,107,
84,-155,72,82,29,-202,-49,165,-76,235,-48,97,61,251,-69,96,317,135,114,-165,
-566,117,-310,17,79,-115,-154,32,182,-248,186,260,151,167,214,39,-170,-131,-90,-108,
96,10,29,293,-59,154,-54,-168,50,224,-151,-399,-114,108,-93,156,102,-20,273,99,
-100,95,-52,56,-5,166,-340,-131,-183,-150,-190,277,-191,74,-71,270,173,-121,47,298,
285,51,33,79,83,89,91,-98,-225,-36,-10,-191,-429,128,-328,40,6,212,-120,241,
246,258,-178,284,-390,-165,-124,-174,-145,137,59,-32,29,396,112,174,-116,-56,-251,-112,
-384,95,-88,-160,31,223,383,6,102,174,-350,164,46,160,-187,141,108,-109,287,-186,
120,80,-103,-33,111,88,19,-18,42,-304,93,-69,-186,363,-134,64,80,250,-107,149,
-281,-259,-239,-114,-213,169,-81,-180,-90,68,161,14,-53,-207,16,44,50,452,341,193,
-110,9,-158,200,-130,-80,17,-43,31,169,174,64,204,52,-110,90,-54,-129,4,-314,
-12,-57,13,282,115,78,19,37,-21,-96,-219,135,-61,23,139,74,-62,97,26,127,
-28,-51,-81,-88,-62,-128,-19,-26,-106,112,22,107,-14,114,89,89,53,-197,-220,39,
-77,-237,65,-49,51,161,81,374,225,228,166,123,-96,11,127,-167,9,-239,285,-252,
-133,-51,-233,-76,118,-76,37,78,168,-38,189,44,-40,-76,156,-4,-182,142,70,61,
113,-309,113,96,212,-178,9,-50,251,-187,-52,-346,-197,-191,-13,-20,276,116,120,44,
63,25,161,-171,101,-36,306,57,479,13,37,-56,-166,-62,38,90,-189,-119,18,-87,
-42,125,1,-39,46,14,105,-375,242,-115,152,-49,-30,-269,-52,-156,-67,-126,145,91,
147,-50,234,-134,152,-7,-148,147,-94,85,152,91,11,400,-59,28,-7,-209,-245,106,
-353,116,49,9,144,166,-57,-204,141,-136,11,21,-32,-145,46,240,232,168,53,134,
54,-315,-335,-89,-128,-324,-109,-81,-104,-115,183,175,173,29,225,34,109,325,-1,48,
49,-77,15,121,6,55,120,-119,151,57,31,176,-148,71,13,-31,-305,-14,156,163,
-31,177,-22,-45,245,256,-183,-56,60,11,112,-24,14,-162,50,-211,164,-152,0,-66,
103,49,215,-33,223,26,207,170,41,-142,69,-244,177,-390,168,-113,-38,-88,-104,43,
146,6,-129,-179,66,-45,174,31,27,99,-27,32,173,149,86,39,94,-133,122,-74,
306,-407,105,-192,231,-14,26,23,-117,19,-247,219,58,164,199,96,-100,360,173,-130,
-142,-60,101,-100,33,30,-55,-15,-249,230,-78,36,-95,-74,286,-37,307,-31,-72,15,
-9,302,168,136,-77,51,-49,64,-142,-51,38,-133,-310,-47,35,128,95,267,-81,181,
301,107,243,-15,121,61,55,58,-179,-168,-40,227,128,50,-59,-80,17,-167,-46,275,
-140,203,-19,114,-199,11,-42,253,-42,-117,75,321,-131,-17,-32,51,11,-216,197,263,
53,55,-171,-166,-13,-133,-73,163,71,-154,339,36,79,32,150,14,278,-75,27,-336,
-321,187,115,67,96,-149,-137,187,6,13,-21,-25,-348,-45,-242,-136,63,90,229,92,
32,-267,-151,-131,-52,-83,-253,-159,-158,-26,-422,};
int ec_fb_setNRreal(void *stat, int level,int smooth){
    int status;
    _fbECState *state=(_fbECState *)stat;
    status=aecSetNlpThresholds(state->ah,level,smooth,25);
    return status;
}
int ec_fb_setTDThreshold(void *stat,int val)
{
    _fbECState *state=(_fbECState *)stat;
    if (val>-30) {
        return 1;
    }
    if (val<-89) {
        val=-89;
    }
    state->ah->td.td_thres=(Ipp64f)(IPP_AEC_dBtbl[-val-10]*IPP_AEC_dBtbl[-val-10]);
   // state->td_thres = (Ipp32s)((state->frameSize*t*t)>>COEF_SF);
    return 0;
}
int ec_fb_setTDSilenceTime(void *stat,int val)
{
    _fbECState *state=(_fbECState *)stat;
    if (val<100) {
        return 1;
    }
    state->ah->td.td_sil_thresh=(Ipp16s)val;
    return 0;
}
int ec_fb_setNR(void *stat, int n){
    _fbECState *state=(_fbECState *)stat;
    if (state == NULL)
        return 1;
    state->nlp = (Ipp16s)n;
    return 0;
}
int ec_fb_setCNG(void *stat, int n){
    int status=0;
    _fbECState *state=(_fbECState *)stat;
    state->cng = (Ipp16s)n;
    return status;
}


int ec_fb_GetSubbandNum(void* stat)
{
    _fbECState* state = (_fbECState*)stat;

    return (state->freq - state->freq);
}


/* init scratch memory buffer */
int ec_fb_InitBuff(void *stat, char *buff) {
    _fbECState *state=(_fbECState *)stat;
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
    state->Mem.offset = 0;
    return 0;
}

/* Returns size of frame */
int ec_fb_GetFrameSize(IppPCMFrequency freq, int taptime_ms, int *s)
{
    *s = FRAME_SIZE_10ms+taptime_ms-taptime_ms+(int)freq-(int)freq;
    return 0;
}

/* Returns size of buffer for state structure */
Ipp32s ec_fb_GetSize(IppPCMFrequency freq, int taptime_ms, int *s, int *s1, int *scratch_size,int ap)
{
    int size = 0, tap, tap_m, csize, csize1;

    if (freq == IPP_PCM_FREQ_8000) {
        tap = taptime_ms * 8;
        tap_m = tap + 2000;
    } else if (freq == IPP_PCM_FREQ_16000) {
        tap = taptime_ms * 16;
        tap_m = tap + 4000;
    } else {
        return 1;
    }

    size += ALIGN(sizeof(_fbECState));//

    size += ALIGN(tap * sizeof(Ipp32f));//firf_coef ALIGN(tap * sizeof(Ipp16s))
    size += ALIGN(tap * sizeof(Ipp32f));//fira_coef
    /************************************************************************/
    size += ALIGN(tap * sizeof(Ipp32f));//pErrToWeights
    /************************************************************************/

    size += ALIGN(tap_m * sizeof(Ipp32f));//hist buf

    ippsFullbandControllerGetSize_EC_32f(FB_FRAME_SIZE, tap, freq, &csize);

    size += ALIGN(csize);

    *s = ALIGN(size+31);//5664

    ah_GetSize(FB_FRAME_SIZE, &csize,freq,scratch_size);
    shiftupCos_GetSize(FRAME_SIZE_10ms, &csize1);
    csize += ALIGN(csize1);
    *s1 = ALIGN(csize+31);//16672

    return 0;
}
/* Returns delay introduced to send-path */
int ec_fb_GetSendPathDelay(int *delay)
{
    *delay = 0;
    return 0;
}

/* Initializes state structure */
Ipp32s ec_fb_Init(void *stat ,void *ahObj, IppPCMFrequency freq, int taptime_ms,int ap)
{
    int  tap, tap_m, csize,ret;
    Ipp8u *ptr;
    _fbECState *state=(_fbECState *)stat;
    if (state==0 || ahObj==0) {
        return 1;
    }
    ptr = (Ipp8u *)state;
    ptr += ALIGN(sizeof(_fbECState));
    /* number of filter coeffs */
    if (freq == IPP_PCM_FREQ_8000) {
        tap = taptime_ms * 8;
        tap_m = tap + 2000;
        //state->td_coeff = COEF_ONE - COEF_ONE * 4 * state->frameSize / freq;
    } else if (freq == IPP_PCM_FREQ_16000) {
        tap = taptime_ms * 16;
        tap_m = tap + 4000;
        //state->td_coeff = COEF_ONE - COEF_ONE * 4 * state->frameSize / freq;
    } else {
        return 1;
    }
    //state->td_coeff = COEF_ONE - COEF_ONE * 4 * state->frameSize / freq;
    //state->td_coeff = COEF_ONE - COEF_ONE * 4 / state->frameSize / freq;

    state->tap = tap;
    /* size of history buffer */
    state->tap_m = tap_m;
    /* size of frame */
    state->frameSize = FB_FRAME_SIZE;

    /* fixed filter coeffs */
    state->firf_coef = (Ipp32f *)ptr;
    ptr += ALIGN(tap * sizeof(Ipp32f));//
    /* adaptive filter coeffs */
    state->fira_coef = (Ipp32f *)ptr;//
    ptr += ALIGN(tap * sizeof(Ipp32f));
    /* history buffer */
    state->rin_hist = (Ipp32f *)ptr;//
    ptr += ALIGN(state->tap_m * sizeof(Ipp32f));

    /* enable adaptation */
    state->mode = 1;

    /* current position in history buffer */
    state->pos = tap + 1;

    /* error of NLMS */
    state->err = 0;
    /* Initialize fullband controller */
    state->controller = (IppsFullbandControllerState_EC_32f *)ptr;

    ippsFullbandControllerGetSize_EC_32f((state->frameSize), tap, freq, &csize);
    ippsFullbandControllerInit_EC_32f(state->controller, (state->frameSize), state->tap, freq);

    ptr += ALIGN(csize);

    state->ah = (ahState *)IPP_ALIGNED_PTR(ahObj, 16);
    ret=ah_Init(state->ah, freq, FB_FRAME_SIZE,25);
    if (ret) {
        return ret;
    }
    ah_GetSize(FB_FRAME_SIZE, &csize, freq,&state->scratch_mem_size);

    state->shiftupCos = (_shiftupCosState_int *)IPP_ALIGNED_PTR((Ipp8u*)ahObj + ALIGN(csize), 16);
    shiftupCos_Init(state->shiftupCos, FRAME_SIZE_10ms);
    /* Zero coeffs and history buffer */
    ippsZero_32f(state->fira_coef,tap);
    ippsZero_32f(state->firf_coef,tap);
    ippsZero_32f(state->rin_hist,state->tap_m);
    state->pErrToWeights=(Ipp32f *)ptr;
    ippsZero_32f(state->pErrToWeights,tap);
    state->nlp=1;
    //mprintf("Intel(R) IPP AEC. FULL BAND algorithm. Version %d\n",IPP_AEC_VER );
    return 0;
}
int ec_fb_GetInfo(void *stat,ECOpcode *isNLP,ECOpcode *isTD,
                   ECOpcode *isAdapt,ECOpcode *isAdaptLite, AHOpcode *isAH)
{
    _fbECState *state=(_fbECState *)stat;
    if (state == NULL)
        return 1;
    if(state->mode & 2)
        *isNLP = EC_NLP_ENABLE;
    else
        *isNLP = EC_NLP_DISABLE;
    if(state->mode & 16)
        *isAH = AH_ENABLE1;
    else
        if(state->mode & 32)
                *isAH = AH_ENABLE2;
            else
                *isAH = AH_DISABLE;

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
/* Do operation or set mode */
int ec_fb_ModeOp(void *stat, ECOpcode op)
{
    _fbECState *state=(_fbECState *)stat;
    if (state == NULL)
        return 1;
    switch (op) {
    case (EC_COEFFS_ZERO):      /* Zero coeffs of filters */
        ippsZero_32f(state->fira_coef,state->tap);
        ippsZero_32f(state->firf_coef,state->tap);
        break;
    case(EC_ADAPTATION_ENABLE): /* Enable adaptation */
    case(EC_ADAPTATION_ENABLE_LITE): /* Enable lite adaptation */
        if (!(state->mode & 1))
            ippsFullbandControllerReset_EC_32f(state->controller);
        state->mode |= 1;
        break;
    case(EC_ADAPTATION_DISABLE): /* Disable adaptation */
        state->mode &= ~1;
        break;
    case(EC_NLP_ENABLE):    /* Enable NLP */
        state->mode |= 2;
        ec_fb_setNR(state,1);
        ec_fb_setNRreal(state,4,0);
        break;
    case(EC_NLP_DISABLE):   /* Disable NLP */
        state->mode &= ~2;
        ec_fb_setNR(state,0);
        //ec_fb_setNRreal(state,0);
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
        //    ah_SetMode(state->ah,AH_ENABLE2);// set anti-howling on
        //    state->mode |= 32;
        //    break;
    case(EC_AH_DISABLE):    /* Disable howling control */
        ah_SetMode(state->ah,AH_DISABLE);// set anti-howling off
        state->mode &= ~16;
        break;
    default:
        break;
    }
    return 0;
}

static int processFrame(void *stat, Ipp32f *r_in, Ipp32f *s_in, Ipp32f *s_out)
{
    _fbECState *state=(_fbECState *)stat;
    Ipp32s i;
    Ipp32s tap;
    Ipp32s pos;
    Ipp32s frameSize;
    Ipp32f *rin_hist,sgain;
    LOCAL_ALIGN_ARRAY(32,Ipp32f,ss,FB_FRAME_SIZE_MAX,state);
    LOCAL_ALIGN_ARRAY(32,Ipp32f,s_out_a_buf,FB_FRAME_SIZE_MAX,state);
    if ((state == NULL) || (r_in==NULL) || (s_in==NULL) || (s_out==NULL))
        return 1;
    tap = state->tap;
    pos = state->pos;
    frameSize = state->frameSize;
    rin_hist = state->rin_hist;

    /* Update history buffer */
    for (i = 0; i < frameSize; i++) {
        rin_hist[pos + i] = r_in[i];
    }

    /* Do filtering with fixed coeffs */
    ippsFIR_EC_32f(state->rin_hist + pos, s_in, s_out, frameSize, state->firf_coef, tap);

    if (ADAPTATION_ENABLED) {
        /* Update fullband controller */
        ippsFullbandControllerUpdate_EC_32f(rin_hist + pos, s_in, ss, state->controller);
        /* Do NLMS filtering */
        //        ippsNLMS_EC_32f(state->rin_hist + pos, s_in, ss, s_out_a_buf, frameSize, state->fira_coef, tap, &(state->err));
        ownNLMS_EC_32f(state->rin_hist + pos, s_in, ss, s_out_a_buf, frameSize,
            state->fira_coef, tap, &(state->err),state->pErrToWeights);
        /* Apply fullband controller */
        ippsFullbandController_EC_32f(s_out_a_buf, s_out, state->fira_coef,\
            state->firf_coef, &sgain, state->controller);
    }
    /* Update position in history buffer */
    pos += frameSize;
    if (pos + frameSize < state->tap_m) {
        state->pos = pos;
    } else {
        for (i = 0; i < tap + 1; i++)
            state->rin_hist[i] = state->rin_hist[i + pos - tap - 1];
        state->pos = tap + 1;
    }
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,s_out_a_buf,FB_FRAME_SIZE_MAX,state);
    LOCAL_ALIGN_ARRAY_FREE(32,Ipp32f,ss,FB_FRAME_SIZE_MAX,state);
    return 0;
}
int ec_fb_ProcessFrame(void *stat, Ipp32f *frin, Ipp32f *fsin, Ipp32f *fsout)
{
    _fbECState *state=(_fbECState *)stat;
    int     ret=0,nlpOff=0,i;
    //Ipp16s rin[FRAME_SIZE_10ms]; Ipp16s sin[FRAME_SIZE_10ms];
    Ipp16s sout[FRAME_SIZE_10ms];
    //state->ah->Mem=state->Mem;
    state->ah->Mem.base=state->Mem.base;
    state->ah->Mem.CurPtr=state->Mem.CurPtr;
    state->ah->Mem.VecPtr=state->Mem.VecPtr;
    state->ah->Mem.offset=state->Mem.offset;
    //for (i=0;i<FRAME_SIZE_10ms;i++) {
    //    rin[i]=(Ipp16s)frin[i];
    //    sin[i]=(Ipp16s)fsin[i];
    //}
    if (TD_ENABLED){
        nlpOff=toneDisabler_32f(frin, fsin, state->ah->samplingRate, FRAME_SIZE_10ms,
            &state->ah->td);
    }
    if(nlpOff){
        ippsCopy_32f(fsin,fsout,FRAME_SIZE_10ms);
    }else{
        for (i=0;i<FRAME_SIZE_10ms;i+=FB_FRAME_SIZE) {
                ret=processFrame(state, &frin[i], &fsin[i], &fsout[i]);
            if (ret)
                return ret;
        }
        //cnr_32f(state->ah,frin,fsin,fsout,FRAME_SIZE_10ms,state->nlp,state->cng,25,FB_OFF);
        for (i=0;i<FRAME_SIZE_10ms;i+=NLP_FRAME_SIZE) {
            cnr_32f(state->ah,&frin[i],&fsin[i],&fsout[i],state->nlp,state->cng,FB_OFF);
        }
        if (AH_ENABLED || AH_ENABLED3) {
            for (i=0;i<FRAME_SIZE_10ms;i++) {
                sout[i]=(Ipp16s)fsout[i];
            }
            shiftupCos(state->shiftupCos, sout, sout);
            for (i=0;i<FRAME_SIZE_10ms;i++) {
                fsout[i]=(Ipp32f)sout[i];
            }
        }
    }
    return ret;
}
