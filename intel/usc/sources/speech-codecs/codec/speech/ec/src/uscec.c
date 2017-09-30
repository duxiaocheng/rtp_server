/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2006-2011 Intel Corporation. All Rights Reserved.
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
//
//
// Purpose: Echo Canceller [Ipp32f point]: USC functions.
//
*/

#include <ipps.h>
#include "usc_ec.h"
#include <ippsc.h>
#include "ec_api.h"

#define  SUBBAND_FRAME_SIZE    64
#define  ASB_MAX_LEN_TAIL     200
#define  SB_MAX_LEN_TAIL      200
#define  FB_MAX_LEN_TAIL      200
#define  MAX_BLOCK_SIZE       128

static USC_Status GetInfoSize(Ipp32s *pSize);
static USC_Status GetInfo(USC_Handle handle, USC_EC_Info *pInfo);
static USC_Status NumAlloc(const USC_EC_Option *options, Ipp32s *nbanks);
static USC_Status MemAlloc(const USC_EC_Option *options, USC_MemBank *pBanks);
static USC_Status Init(const USC_EC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit(const USC_EC_Modes *modes, USC_Handle handle );
static USC_Status Control(const USC_EC_Modes *modes, USC_Handle handle );
static USC_Status CancelEcho(USC_Handle handle, Ipp16s *pSin, Ipp16s *pRin, Ipp16s *pSout);

typedef struct {

   ec_GetFrameSize_ptr ec_GetFrameSize;
   ec_GetSize_ptr ec_GetSize;
   ec_Init_ptr ec_Init;
   ec_ModeOp_ptr ec_ModeOp;
   ec_GetInfo_ptr ec_GetInfo;
   ec_GetSendPathDelay_ptr ec_GetSendPathDelay;
   ec_ProcessFrame_ptr ec_ProcessFrame;
   ec_setNR_ptr ec_setNR;
   ec_setCNG_ptr ec_setCNG;
   ec_setNRreal_ptr ec_setNRreal;

} ecfp_fun;

typedef struct {
    IppsFilterNoiseState_EC_32f *nrSin;
    USC_EC_Algs   algType;
    USC_PCMType   pcmType;
    Ipp32s        echotail;
    USC_AdaptType adapt;
    Ipp32s        zeroCoeff;
    Ipp32s        cng;
    Ipp32s        ap;
    Ipp32s        nlp;
    Ipp32s        nr;
    Ipp32s        td;
    Ipp32s        ah;
    Ipp32s        frameSize;
    Ipp32s        nr_smooth;
    Ipp32s        dcFlag;
    Ipp32f        dcOffset;
    IppPCMFrequency        freq;
    ecfp_fun      fun;
} ECFP_Handle_Header;

/* global usc vector table */
USCFUN USC_EC_Fxns USC_ECFP_Fxns=
{
    {
        USC_EC,
        (GetInfoSize_func) GetInfoSize,
        (GetInfo_func)     GetInfo,
        (NumAlloc_func)    NumAlloc,
        (MemAlloc_func)    MemAlloc,
        (Init_func)        Init,
        (Reinit_func)      Reinit,
        (Control_func)     Control
    },
    CancelEcho

};

static USC_PCMType pcmTypesTbl_EC[2]={
   {8000,16},{16000,16}
};

static USC_Status GetInfoSize(Ipp32s *pSize)
{
    if (pSize == NULL)
        return USC_BadDataPointer;
    *pSize = sizeof(USC_EC_Info);
    return USC_NoError;
}
static USC_Status GetInfo(USC_Handle handle, USC_EC_Info *pInfo)
{
    ECFP_Handle_Header *ecfp_header;
    if (pInfo == NULL)
        return USC_BadDataPointer;
    pInfo->name = "IPP_EC_FP";
    if (handle == NULL) {
#ifdef AEC_SB_8ms_interface
        pInfo->params.framesize = SUBBAND_FRAME_SIZE*sizeof(Ipp16s);
#else
        pInfo->params.framesize = FRAME_SIZE_10ms*sizeof(Ipp16s);
#endif
        pInfo->params.algType = EC_SUBBAND;
        pInfo->params.pcmType.sample_frequency = 8000;
        pInfo->params.pcmType.bitPerSample = 16;
        pInfo->params.pcmType.nChannels = 1;
        pInfo->params.echotail = 16;
        pInfo->params.modes.adapt = AD_FULLADAPT;
        pInfo->params.modes.zeroCoeff = 0;
        pInfo->params.modes.nlp = 1;
        pInfo->params.modes.cng = 1;
        pInfo->params.modes.td = 1;
        pInfo->params.modes.ah = 0;         /* AH off*/
        pInfo->params.modes.ap = 0;         /* affine projection order */
        pInfo->params.modes.nr = 4;
    } else {
        ecfp_header = (ECFP_Handle_Header*)handle;
        if(ecfp_header->algType == EC_SUBBAND) {
            pInfo->params.algType = EC_SUBBAND;
#ifdef AEC_SB_8ms_interface
            pInfo->params.framesize = SUBBAND_FRAME_SIZE*sizeof(Ipp16s);
#else
            pInfo->params.framesize = FRAME_SIZE_10ms*sizeof(Ipp16s);
#endif
        } else if(ecfp_header->algType == EC_FULLBAND) {
            pInfo->params.algType = EC_FULLBAND;
            pInfo->params.framesize = FRAME_SIZE_10ms*sizeof(Ipp16s);
        } else {
            pInfo->params.algType = EC_FASTSUBBAND;
            pInfo->params.framesize = FRAME_SIZE_10ms*sizeof(Ipp16s);
        }
        pInfo->params.pcmType.sample_frequency = ecfp_header->pcmType.sample_frequency;
        pInfo->params.pcmType.bitPerSample = ecfp_header->pcmType.bitPerSample;
        pInfo->params.pcmType.nChannels = ecfp_header->pcmType.nChannels;
        pInfo->params.echotail = ecfp_header->echotail;
        pInfo->params.modes.adapt = ecfp_header->adapt;
        pInfo->params.modes.zeroCoeff = ecfp_header->zeroCoeff;
        pInfo->params.modes.nlp = ecfp_header->nlp;
        pInfo->params.modes.cng = ecfp_header->cng;
        pInfo->params.modes.td = ecfp_header->td;
        pInfo->params.modes.ah = ecfp_header->ah;
        pInfo->params.modes.ap = ecfp_header->ap;
        pInfo->params.nModes = sizeof(USC_EC_Modes)/sizeof(Ipp32s);
        pInfo->params.modes.nr = ecfp_header->nr;
    }
    pInfo->nPcmTypes = 2;
    pInfo->pPcmTypesTbl = pcmTypesTbl_EC;
    return USC_NoError;
}

static USC_Status NumAlloc(const USC_EC_Option *options, Ipp32s *nbanks)
{
    if(options==NULL) return USC_BadDataPointer;
    if(nbanks==NULL) return USC_BadDataPointer;
    *nbanks = 4;
    return USC_NoError;
}

#define ALIGN(s) (((s) + 15) & ~15)
static USC_Status MemAlloc(const USC_EC_Option *options, USC_MemBank *pBanks)
{
    Ipp32s nbytes,nbytes2,scratch_size,taptime_ms;
    IppPCMFrequency freq;

    if(options==NULL) return USC_BadDataPointer;
    if(pBanks==NULL) return USC_BadDataPointer;
    if(options->pcmType.bitPerSample != 16) return USC_UnsupportedPCMType;
    switch(options->pcmType.sample_frequency) {
     case 8000:  freq = IPP_PCM_FREQ_8000;  break;
     case 16000: freq = IPP_PCM_FREQ_16000; break;
     default: return USC_UnsupportedPCMType;
    }

    pBanks[0].pMem = NULL;
    pBanks[0].align = 32;
    pBanks[0].memType = USC_OBJECT;
    pBanks[0].memSpaceType = USC_NORMAL;

    pBanks[1].pMem = NULL;
    pBanks[1].align = 32;
    pBanks[1].memType = USC_OBJECT;
    pBanks[1].memSpaceType = USC_NORMAL;

    pBanks[2].pMem = NULL;
    pBanks[2].align = 32;
    pBanks[2].memType = USC_OBJECT;
    pBanks[2].memSpaceType = USC_NORMAL;

    pBanks[3].pMem = NULL;
    pBanks[3].align = 32;
    pBanks[3].memType = USC_OBJECT;
    pBanks[3].memSpaceType = USC_NORMAL;

    //if(options->algType == EC_AFFINESUBBAND) {
    //    if((options->echotail > 0) && (options->echotail <= ASB_MAX_LEN_TAIL)) taptime_ms = options->echotail;
    //    else return USC_UnsupportedEchoTail;
    //    ec_asb_GetSize(freq, taptime_ms,&nbytes,&nbytes2,&scratch_size);
    //} else
        if(options->algType == EC_SUBBAND) {
        if((options->echotail > 0) && (options->echotail <= SB_MAX_LEN_TAIL)) taptime_ms = options->echotail;
        else return USC_UnsupportedEchoTail;
        ec_sb_GetSize(freq, taptime_ms,&nbytes,&nbytes2,&scratch_size,options->modes.ap);
    } else if(options->algType == EC_FULLBAND) {
        if((options->echotail > 0) && (options->echotail <= FB_MAX_LEN_TAIL)) taptime_ms = options->echotail;
        else return USC_UnsupportedEchoTail;
        ec_fb_GetSize(freq, taptime_ms,&nbytes,&nbytes2,&scratch_size,0);
    } else {
        if((options->echotail > 0) && (options->echotail <= SB_MAX_LEN_TAIL)) taptime_ms = options->echotail;
        else return USC_UnsupportedEchoTail;
        ec_sbf_GetSize(freq, taptime_ms,&nbytes,&nbytes2,&scratch_size,0);
    }
    pBanks[0].nbytes = nbytes + ALIGN(sizeof(ECFP_Handle_Header)); /* room for USC header */
    pBanks[1].nbytes = nbytes2;
    pBanks[2].nbytes = scratch_size;
    ippsFilterNoiseGetStateSize_EC_32f((IppPCMFrequency)freq,&pBanks[3].nbytes);
    return USC_NoError;
}

static USC_Status Init(const USC_EC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
    ECFP_Handle_Header *ecfp_header;
    IppPCMFrequency freq;
    Ipp32s taptime_ms;
    void *ahObj;
    void* scratchbuff = NULL;
    USC_Handle *obj_ec;

    if(options==NULL) return USC_BadDataPointer;
    if(pBanks==NULL) return USC_BadDataPointer;
    if(pBanks[0].pMem==NULL) return USC_NotInitialized;
    if(pBanks[0].nbytes<=0) return USC_NotInitialized;
    if(pBanks[1].pMem==NULL) return USC_NotInitialized;
    if(pBanks[1].nbytes<=0) return USC_NotInitialized;
    if(pBanks[2].pMem==NULL) return USC_NotInitialized;
    if(pBanks[2].nbytes<=0) return USC_NotInitialized;
    if(pBanks[3].nbytes<=0) return USC_NotInitialized;
    if(handle==NULL) return USC_InvalidHandler;
    if(options->pcmType.bitPerSample != 16) return USC_UnsupportedPCMType;
    switch(options->pcmType.sample_frequency) {
     case 8000:  freq = IPP_PCM_FREQ_8000;  break;
     case 16000: freq = IPP_PCM_FREQ_16000; break;
     default: return USC_UnsupportedPCMType;
    }
    if(options->pcmType.nChannels<=0)
        return USC_UnsupportedPCMType;
    if(options->echotail<0)
        return USC_UnsupportedEchoTail;
    if(options->modes.zeroCoeff<0)
        return USC_BadArgument;
    if(options->modes.ah<0)
        return USC_BadArgument;
    if(options->modes.cng<0)
        return USC_BadArgument;
    if(options->modes.nlp<0)
        return USC_BadArgument;
    if(options->modes.td<0)
        return USC_BadArgument;
    if(options->modes.ap<0)
        return USC_BadArgument;
    if(options->modes.nr<0)
        return USC_BadArgument;

    *handle = (USC_Handle*)pBanks[0].pMem;
    ahObj = (USC_Handle*)pBanks[1].pMem;
    scratchbuff = (void*)pBanks[2].pMem;
    ecfp_header = (ECFP_Handle_Header*)*handle;

    ecfp_header->nrSin = (IppsFilterNoiseState_EC_32f*)pBanks[3].pMem;
    ecfp_header->freq = freq;
    ecfp_header->dcOffset = 0;
    ippsFilterNoiseInit_EC_32f(freq,ecfp_header->nrSin);
    ippsFilterNoiseLevel_EC_32f(ippsNrHigh, ecfp_header->nrSin);
    ippsFilterNoiseSetMode_EC_32f(ippsNrSmoothOff, ecfp_header->nrSin);
    obj_ec = (USC_Handle*)((Ipp8s*)*handle + ALIGN(sizeof(ECFP_Handle_Header)));

    ecfp_header->algType = options->algType;
    ecfp_header->pcmType.sample_frequency = options->pcmType.sample_frequency;
    ecfp_header->pcmType.bitPerSample = options->pcmType.bitPerSample;
    ecfp_header->pcmType.nChannels = options->pcmType.nChannels;
    if(ecfp_header->algType == EC_SUBBAND) {
        if((options->echotail > 0) && (options->echotail <= SB_MAX_LEN_TAIL)) taptime_ms = options->echotail;
        else return USC_UnsupportedEchoTail;
        ecfp_header->fun.ec_GetFrameSize = (ec_GetFrameSize_ptr)ec_sb_GetFrameSize;
        ecfp_header->fun.ec_GetSize = (ec_GetSize_ptr)ec_sb_GetSize;
        ecfp_header->fun.ec_Init = (ec_Init_ptr)ec_sb_Init;
        ecfp_header->fun.ec_ModeOp = (ec_ModeOp_ptr)ec_sb_ModeOp;
        ecfp_header->fun.ec_GetInfo = (ec_GetInfo_ptr)ec_sb_GetInfo;
        ecfp_header->fun.ec_GetSendPathDelay = (ec_GetSendPathDelay_ptr)ec_sb_GetSendPathDelay;
        ecfp_header->fun.ec_ProcessFrame = (ec_ProcessFrame_ptr)ec_sb_ProcessFrame;

        ecfp_header->fun.ec_setNR= (ec_setNR_ptr)ec_sb_setNR;
        ecfp_header->fun.ec_setCNG= (ec_setCNG_ptr)ec_sb_setCNG;
        ecfp_header->fun.ec_setNRreal= (ec_setNRreal_ptr)ec_sb_setNRreal;
    //} else if(ecfp_header->algType == EC_AFFINESUBBAND) {
    //    if((options->echotail > 0) && (options->echotail <= ASB_MAX_LEN_TAIL)) taptime_ms = options->echotail;
    //    else return USC_UnsupportedEchoTail;
    //    ecfp_header->fun.ec_GetFrameSize = (ec_GetFrameSize_ptr)ec_asb_GetFrameSize;
    //    ecfp_header->fun.ec_GetSize = (ec_GetSize_ptr)ec_asb_GetSize;
    //    ecfp_header->fun.ec_Init = (ec_Init_ptr)ec_asb_Init;
    //    ecfp_header->fun.ec_ModeOp = (ec_ModeOp_ptr)ec_asb_ModeOp;
    //    ecfp_header->fun.ec_GetInfo = (ec_GetInfo_ptr)ec_asb_GetInfo;
    //    ecfp_header->fun.ec_GetSendPathDelay = (ec_GetSendPathDelay_ptr)ec_asb_GetSendPathDelay;
    //    ecfp_header->fun.ec_ProcessFrame = (ec_ProcessFrame_ptr)ec_asb_ProcessFrame;

    //    ecfp_header->fun.ec_setNR= (ec_setNR_ptr)ec_asb_setNR;
    //    ecfp_header->fun.ec_setCNG= (ec_setCNG_ptr)ec_asb_setCNG;
    } else if(ecfp_header->algType == EC_FULLBAND) {
        if((options->echotail > 0) && (options->echotail <= FB_MAX_LEN_TAIL)) taptime_ms = options->echotail;
        else return USC_UnsupportedEchoTail;
        ecfp_header->fun.ec_GetFrameSize = (ec_GetFrameSize_ptr)ec_fb_GetFrameSize;
        ecfp_header->fun.ec_GetSize = (ec_GetSize_ptr)ec_fb_GetSize;
        ecfp_header->fun.ec_Init = (ec_Init_ptr)ec_fb_Init;
        ecfp_header->fun.ec_ModeOp = (ec_ModeOp_ptr)ec_fb_ModeOp;
        ecfp_header->fun.ec_GetInfo = (ec_GetInfo_ptr)ec_fb_GetInfo;
        ecfp_header->fun.ec_GetSendPathDelay = (ec_GetSendPathDelay_ptr)ec_fb_GetSendPathDelay;
        ecfp_header->fun.ec_ProcessFrame = (ec_ProcessFrame_ptr)ec_fb_ProcessFrame;

        ecfp_header->fun.ec_setNR= (ec_setNR_ptr)ec_fb_setNR;
        ecfp_header->fun.ec_setCNG= (ec_setCNG_ptr)ec_fb_setCNG;
        ecfp_header->fun.ec_setNRreal= (ec_setNRreal_ptr)ec_fb_setNRreal;
    } else {
        if((options->echotail > 0) && (options->echotail <= SB_MAX_LEN_TAIL)) taptime_ms = options->echotail;
        else return USC_UnsupportedEchoTail;
        ecfp_header->fun.ec_GetFrameSize = (ec_GetFrameSize_ptr)ec_sbf_GetFrameSize;
        ecfp_header->fun.ec_GetSize = (ec_GetSize_ptr)ec_sbf_GetSize;
        ecfp_header->fun.ec_Init = (ec_Init_ptr)ec_sbf_Init;
        ecfp_header->fun.ec_ModeOp = (ec_ModeOp_ptr)ec_sbf_ModeOp;
        ecfp_header->fun.ec_GetInfo = (ec_GetInfo_ptr)ec_sbf_GetInfo;
        ecfp_header->fun.ec_GetSendPathDelay = (ec_GetSendPathDelay_ptr)ec_sbf_GetSendPathDelay;
        ecfp_header->fun.ec_ProcessFrame = (ec_ProcessFrame_ptr)ec_sbf_ProcessFrame;

        ecfp_header->fun.ec_setNR= (ec_setNR_ptr)ec_sbf_setNR;
        ecfp_header->fun.ec_setCNG= (ec_setCNG_ptr)ec_sbf_setCNG;
        ecfp_header->fun.ec_setNRreal= (ec_setNRreal_ptr)ec_sbf_setNRreal;
    }
    ecfp_header->echotail = taptime_ms;
    ecfp_header->adapt = options->modes.adapt;
    ecfp_header->zeroCoeff = options->modes.zeroCoeff;
    ecfp_header->nlp = options->modes.nlp;
    ecfp_header->cng = options->modes.cng;
    ecfp_header->td = options->modes.td;
    ecfp_header->ah = options->modes.ah;
    ecfp_header->ap = options->modes.ap;
    ecfp_header->nr = options->modes.nr;
    ecfp_header->nr_smooth = options->modes.nr_smooth;
    ecfp_header->dcOffset = 0;
    ecfp_header->dcFlag = options->modes.dcFlag;

    if (ecfp_header->fun.ec_Init(obj_ec, ahObj,freq, ecfp_header->echotail,ecfp_header->ap)){
        return USC_NoOperation;
    }
    if(ecfp_header->algType == EC_SUBBAND) {
        if (ec_sb_InitBuff(obj_ec, scratchbuff)){
            return USC_NoOperation;
        }
    //} else if(ecfp_header->algType == EC_AFFINESUBBAND) {
    //    if (ec_asb_InitBuff(obj_ec, scratchbuff)){
    //        return USC_NoOperation;
    //    }
    } else if(ecfp_header->algType == EC_FULLBAND) {
        if (ec_fb_InitBuff(obj_ec, scratchbuff)){
            return USC_NoOperation;
        }
    }else{
        if (ec_sbf_InitBuff(obj_ec, scratchbuff)){
            return USC_NoOperation;
        }
    }
    if(ecfp_header->zeroCoeff) ecfp_header->fun.ec_ModeOp(obj_ec, EC_COEFFS_ZERO);
    if((ecfp_header->adapt == AD_FULLADAPT) || (ecfp_header->adapt == AD_LITEADAPT)) {
        if(ecfp_header->adapt == AD_FULLADAPT) ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_ENABLE);
        else ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_ENABLE_LITE);
    } else {
        ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_DISABLE);
        ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_DISABLE_LITE);
    }
    if(ecfp_header->nlp) ecfp_header->fun.ec_ModeOp(obj_ec, EC_NLP_ENABLE);
    else ecfp_header->fun.ec_ModeOp(obj_ec, EC_NLP_DISABLE);
    if(ecfp_header->td) ecfp_header->fun.ec_ModeOp(obj_ec, EC_TD_ENABLE);
    else ecfp_header->fun.ec_ModeOp(obj_ec, EC_TD_DISABLE);

    switch(ecfp_header->ah) {
    case 0:
        ecfp_header->fun.ec_ModeOp(obj_ec, EC_AH_DISABLE);
        break;
    case 1:
        ecfp_header->fun.ec_ModeOp(obj_ec, EC_AH_ENABLE);
        break;
        //    case 2:
        //      ecfp_header->fun.ec_ModeOp(obj_ec, EC_AH_ENABLE3);
        //    break;
    default:
        ecfp_header->fun.ec_ModeOp(obj_ec, EC_AH_DISABLE);
    }
    ecfp_header->fun.ec_GetFrameSize(IPP_PCM_FREQ_8000,0,&ecfp_header->frameSize );
    ecfp_header->fun.ec_setNR(obj_ec, ecfp_header->nlp);
    ecfp_header->fun.ec_setCNG(obj_ec, ecfp_header->cng);
    ecfp_header->fun.ec_setNRreal(obj_ec, ecfp_header->nr, ecfp_header->nr_smooth);
    return USC_NoError;
}

static USC_Status Reinit(const USC_EC_Modes *modes, USC_Handle handle )
{
    ECFP_Handle_Header *ecfp_header;
    USC_Handle *obj_ec;

    if(modes==NULL) return USC_BadDataPointer;
    if(handle==NULL) return USC_InvalidHandler;

    ecfp_header = (ECFP_Handle_Header*)handle;
    obj_ec = (USC_Handle*)((Ipp8s*)handle + ALIGN(sizeof(ECFP_Handle_Header)));
    if(modes->zeroCoeff<0)
        return USC_BadArgument;
    if(modes->ah<0)
        return USC_BadArgument;
    if(modes->cng<0)
        return USC_BadArgument;
    if(modes->nlp<0)
        return USC_BadArgument;
    if(modes->td<0)
        return USC_BadArgument;
    if(modes->nr<0)
        return USC_BadArgument;

    ippsFilterNoiseInit_EC_32f(ecfp_header->freq,ecfp_header->nrSin);
    ippsFilterNoiseLevel_EC_32f(ippsNrHigh, ecfp_header->nrSin);
    ippsFilterNoiseSetMode_EC_32f(ippsNrSmoothOff, ecfp_header->nrSin);
    ecfp_header->adapt = modes->adapt;
    ecfp_header->zeroCoeff = modes->zeroCoeff;
    ecfp_header->nlp = modes->nlp;
    ecfp_header->cng = modes->cng;
    ecfp_header->td = modes->td;
    ecfp_header->ah = modes->ah;
    ecfp_header->nr = modes->nr;
    ecfp_header->nr_smooth = modes->nr_smooth;
    ecfp_header->dcOffset = 0;

    if(ecfp_header->zeroCoeff) ecfp_header->fun.ec_ModeOp(obj_ec, EC_COEFFS_ZERO);
    if((ecfp_header->adapt == AD_FULLADAPT) || (ecfp_header->adapt == AD_LITEADAPT)) {
        if(ecfp_header->adapt == AD_FULLADAPT) ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_ENABLE);
        else ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_ENABLE_LITE);
    } else {
        ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_DISABLE);
        ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_DISABLE_LITE);
    }
    if(ecfp_header->nlp>0) ecfp_header->fun.ec_ModeOp(obj_ec, EC_NLP_ENABLE);
    else ecfp_header->fun.ec_ModeOp(obj_ec, EC_NLP_DISABLE);
    if(ecfp_header->td) ecfp_header->fun.ec_ModeOp(obj_ec, EC_TD_ENABLE);
    else ecfp_header->fun.ec_ModeOp(obj_ec, EC_TD_DISABLE);

    switch(ecfp_header->ah) {
    case 0:
        ecfp_header->fun.ec_ModeOp(obj_ec, EC_AH_DISABLE);
        break;
    case 1:
        ecfp_header->fun.ec_ModeOp(obj_ec, EC_AH_ENABLE);
        break;
    default:
        ecfp_header->fun.ec_ModeOp(obj_ec, EC_AH_DISABLE);
    }
    ecfp_header->fun.ec_setNR(obj_ec, ecfp_header->nlp);
    ecfp_header->fun.ec_setCNG(obj_ec, ecfp_header->cng);
    ecfp_header->fun.ec_setNRreal(obj_ec, ecfp_header->nr, ecfp_header->nr_smooth);

    return USC_NoError;
}

static USC_Status Control(const USC_EC_Modes *modes, USC_Handle handle )
{

    ECFP_Handle_Header *ecfp_header;
    USC_Handle *obj_ec;

    if(modes==NULL) return USC_BadDataPointer;
    if(handle==NULL) return USC_InvalidHandler;

    ecfp_header = (ECFP_Handle_Header*)handle;
    ippsFilterNoiseInit_EC_32f(ecfp_header->freq,ecfp_header->nrSin);
    ippsFilterNoiseLevel_EC_32f(ippsNrHigh, ecfp_header->nrSin);
    ippsFilterNoiseSetMode_EC_32f(ippsNrSmoothOff, ecfp_header->nrSin);
    obj_ec = (USC_Handle*)((Ipp8s*)handle + ALIGN(sizeof(ECFP_Handle_Header)));
    if(modes->zeroCoeff<0)
        return USC_BadArgument;
    if(modes->ah<0)
        return USC_BadArgument;
    if(modes->cng<0)
        return USC_BadArgument;
    if(modes->nlp<0)
        return USC_BadArgument;
    if(modes->td<0)
        return USC_BadArgument;
    if(modes->nr<0)
        return USC_BadArgument;

    ecfp_header->zeroCoeff = modes->zeroCoeff;
    if(ecfp_header->zeroCoeff) ecfp_header->fun.ec_ModeOp(obj_ec, EC_COEFFS_ZERO);
    if(ecfp_header->adapt != modes->adapt) {
        ecfp_header->adapt = modes->adapt;
        if((ecfp_header->adapt == AD_FULLADAPT) || (ecfp_header->adapt == AD_LITEADAPT)) {
            if(ecfp_header->adapt == AD_FULLADAPT) ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_ENABLE);
            else ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_ENABLE_LITE);
        } else {
            ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_DISABLE);
            ecfp_header->fun.ec_ModeOp(obj_ec, EC_ADAPTATION_DISABLE_LITE);
        }
    }
    if(ecfp_header->nlp != modes->nlp) {
        ecfp_header->nlp = modes->nlp;
        if(ecfp_header->nlp) ecfp_header->fun.ec_ModeOp(obj_ec, EC_NLP_ENABLE);
        else ecfp_header->fun.ec_ModeOp(obj_ec, EC_NLP_DISABLE);
    }
    if(ecfp_header->td != modes->td) {
        ecfp_header->td = modes->td;
        if(ecfp_header->td) ecfp_header->fun.ec_ModeOp(obj_ec, EC_TD_ENABLE);
        else ecfp_header->fun.ec_ModeOp(obj_ec, EC_TD_DISABLE);
    }
    if(ecfp_header->ah != modes->ah) {
        ecfp_header->ah = modes->ah;
        switch(ecfp_header->ah) {
        case 0:
            ecfp_header->fun.ec_ModeOp(obj_ec, EC_AH_DISABLE);
            break;
        case 1:
            ecfp_header->fun.ec_ModeOp(obj_ec, EC_AH_ENABLE);
            break;
        default:
            ecfp_header->fun.ec_ModeOp(obj_ec, EC_AH_DISABLE);
        }
    }
    if(ecfp_header->nlp != modes->nlp)
        ecfp_header->nlp = modes->nlp;
    ecfp_header->fun.ec_setNR(obj_ec, modes->nlp);
    if(ecfp_header->cng != modes->cng)
        ecfp_header->cng = modes->cng;
    ecfp_header->fun.ec_setCNG(obj_ec, modes->cng);
    ecfp_header->fun.ec_setNRreal(obj_ec, ecfp_header->nr, ecfp_header->nr_smooth);
    return USC_NoError;
}
static USC_Status CancelEcho(USC_Handle handle, Ipp16s *pSin, Ipp16s *pRin, Ipp16s *pSout)
{
   ECFP_Handle_Header *ecfp_header;
   USC_Handle *obj_ec;
   Ipp32s framesize;
   Ipp32f r_in_32f_cur[MAX_BLOCK_SIZE];
   Ipp32f s_in_32f_cur[MAX_BLOCK_SIZE];
   Ipp32f s_out_32f_cur[MAX_BLOCK_SIZE];
   //Ipp64f noisePwr;
   //int    dstFlag,j;
   if(handle==NULL) return USC_InvalidHandler;
   if(pSin==NULL) return USC_BadDataPointer;
   if(pRin==NULL) return USC_BadDataPointer;
   if(pSout==NULL) return USC_BadDataPointer;

   ecfp_header = (ECFP_Handle_Header*)handle;
   obj_ec = (USC_Handle*)((Ipp8s*)handle + ALIGN(sizeof(ECFP_Handle_Header)));
   framesize = FRAME_SIZE_10ms;
   ippsConvert_16s32f_Sfs((Ipp16s *)pRin, r_in_32f_cur, framesize, 0);
   ippsConvert_16s32f_Sfs((Ipp16s *)pSin, s_in_32f_cur, framesize, 0);
   //if (ecfp_header->dcFlag)
   //{
   //    ippsSubC_32f_I(ecfp_header->dcOffset,s_in_32f_cur, framesize);
   //    for (j=0;j<framesize;j+=16){
   //        ippsFilterNoiseDetect_EC_32f64f((const Ipp32f  *)&s_in_32f_cur[j],&noisePwr,
   //            &ecfp_header->dcOffset,&dstFlag,ecfp_header->nrSin);
   //    }
   //}
   ecfp_header->fun.ec_ProcessFrame(obj_ec, r_in_32f_cur, s_in_32f_cur, s_out_32f_cur);
   ippsConvert_32f16s_Sfs(s_out_32f_cur, (Ipp16s *)pSout, framesize, ippRndZero, 0);

   return USC_NoError;

}


