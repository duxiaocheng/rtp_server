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
// Purpose: GSM FR 06.10: USC functions.
//
*/
#include "usc.h"
#include "owngsmfr.h"
#include "gsmfrapi.h"

#define INIT_SEED          11111
#define GSMFR_NUM_RATES        1
#define GSMFR_EXT_FRAME_LEN  160
#define GSMFR_BITS_PER_SAMPLE 16
#define GSMFR_SAMPLE_FREQ     8000
#define GSMFR_NCHANNELS     1

static USC_Status GetInfoSize(Ipp32s *pSize);
static USC_Status GetInfo(USC_Handle handle, void *pInfo);
static USC_Status NumAlloc(const void *options, Ipp32s *nbanks);
static USC_Status MemAlloc(const void *options, USC_MemBank *pBanks);
static USC_Status Init(const void *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit(const void *modes, USC_Handle handle );
static USC_Status Control(const void *modes, USC_Handle handle );
static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out);
static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out);
static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst);
static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize);


typedef struct {
    USC_Direction direction;
    GSMFR_Params_t params;
    Ipp16s seed;
    Ipp32s fReset;
    Ipp32s fReset_old;
    Ipp32s lostFrames;
    Ipp32s validFrames;
    Ipp8u validFrame[GSMFR_PACKED_FRAME_LEN];
    Ipp32s reserved; // for future extension
} GSMFR_Handle_Header;

/* global usc vector table */
USCFUN USC_Fxns USC_GSMFR_Fxns=
{
    {
        USC_Codec,
        (GetInfoSize_func) GetInfoSize,
        (GetInfo_func)     GetInfo,
        (NumAlloc_func)    NumAlloc,
        (MemAlloc_func)    MemAlloc,
        (Init_func)        Init,
        (Reinit_func)      Reinit,
        (Control_func)     Control
    },
    Encode,
    Decode,
    GetOutStreamSize,
    SetFrameSize

};

static __ALIGN32 CONST USC_Rates pTblRates_GSMFR[GSMFR_NUM_RATES]={
    {13000}
};
static __ALIGN32 CONST USC_PCMType pcmTypesTbl_GSMFR[1]={
   {GSMFR_SAMPLE_FREQ,GSMFR_BITS_PER_SAMPLE,GSMFR_NCHANNELS}
};
static USC_Status GetInfoSize(Ipp32s *pSize)
{
    USC_CHECK_PTR(pSize);
    *pSize = sizeof(USC_CodecInfo);
    return USC_NoError;
}
static USC_Status GetInfo(USC_Handle handle, void *pInf)
{
   USC_CodecInfo *pInfo=pInf;
   GSMFR_Handle_Header *gsmfr_header;
   USC_CHECK_PTR(pInf);

   pInfo->name = "IPP_GSMFR";
   pInfo->params.framesize = GSMFR_EXT_FRAME_LEN*sizeof(Ipp16s);
   if (handle == NULL) {
      pInfo->params.direction = USC_DECODE;
      pInfo->params.modes.vad = 2;
    } else {
      gsmfr_header = (GSMFR_Handle_Header*)handle;
      pInfo->params.direction = gsmfr_header->direction;
      switch(gsmfr_header->params.vadDirection) {
        case GSMFR_VAD_OFF:  pInfo->params.modes.vad = 0; break;
        case GSMFR_DOWNLINK: pInfo->params.modes.vad = 1; break;
        case GSMFR_UPLINK:   pInfo->params.modes.vad = 2; break;
        default:             pInfo->params.modes.vad = 0; break;
      }
    }
   pInfo->params.modes.bitrate = 13000;
   pInfo->params.modes.truncate = 0;
   pInfo->params.pcmType.sample_frequency = pcmTypesTbl_GSMFR[0].sample_frequency;
   pInfo->params.pcmType.nChannels = pcmTypesTbl_GSMFR[0].nChannels;
   pInfo->params.pcmType.bitPerSample = pcmTypesTbl_GSMFR[0].bitPerSample;
   pInfo->nPcmTypes = 1;
   pInfo->pPcmTypesTbl = pcmTypesTbl_GSMFR;
   pInfo->maxbitsize = GSMFR_PACKED_FRAME_LEN;
   pInfo->params.modes.hpf = 0;
   pInfo->params.modes.pf = 0;
   pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
   pInfo->params.law = 0;
   pInfo->nRates = GSMFR_NUM_RATES;
   pInfo->pRateTbl = (const USC_Rates *)&pTblRates_GSMFR;
   pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);

   return USC_NoError;
}

static USC_Status NumAlloc(const void *opt, Ipp32s *nbanks)
{
   USC_CHECK_PTR(opt);
   USC_CHECK_PTR(nbanks);

   *nbanks = 1;
   return USC_NoError;
}

static USC_Status MemAlloc(const void *opt, USC_MemBank *pBanks)
{
    const USC_Option *options=opt;
    Ipp32s nbytes;

    USC_CHECK_PTR(opt);
    USC_CHECK_PTR(pBanks);

    pBanks->pMem = NULL;
    pBanks->align = 32;
    pBanks->memType = USC_OBJECT;
    pBanks->memSpaceType = USC_NORMAL;

   if (options->direction == USC_ENCODE) /* encode only */
    {
        apiGSMFREncoder_Alloc((Ipp32s *)&nbytes);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        apiGSMFRDecoder_Alloc((Ipp32s *)&nbytes);
    } else {
        return USC_NoOperation;
    }
    pBanks->nbytes = nbytes + sizeof(GSMFR_Handle_Header); /* include GSMFR_Handle_Header */
    return USC_NoError;
}

static Ipp32s CheckRate_GSMFR(Ipp32s rate_bps)
{
    Ipp32s rate;

    switch(rate_bps) {
      case 13000:  rate = 0; break;
      default: rate = -1; break;
    }
    return rate;
}

static USC_Status Init(const void *opt, const USC_MemBank *pBanks, USC_Handle *handle)
{
    const USC_Option *options=opt;
    GSMFR_Handle_Header *gsmfr_header;

    USC_CHECK_PTR(opt);
    USC_CHECK_PTR(pBanks);
    USC_CHECK_PTR(pBanks->pMem);
    USC_BADARG(pBanks->nbytes<=0, USC_NotInitialized);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(options->modes.vad > 2, USC_UnsupportedVADType);
    USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);

    USC_BADARG(CheckRate_GSMFR(options->modes.bitrate) < 0, USC_UnsupportedBitRate);

    USC_BADARG(options->pcmType.bitPerSample!=GSMFR_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.sample_frequency!=GSMFR_SAMPLE_FREQ, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.nChannels!=GSMFR_NCHANNELS, USC_UnsupportedPCMType);

    *handle = (USC_Handle*)pBanks->pMem;
    gsmfr_header = (GSMFR_Handle_Header*)*handle;
    gsmfr_header->direction = options->direction;

    switch(options->modes.vad) {
      case 1: gsmfr_header->params.vadDirection = GSMFR_DOWNLINK; break;
      case 2: gsmfr_header->params.vadDirection = GSMFR_UPLINK; break;
      default: gsmfr_header->params.vadDirection = GSMFR_VAD_OFF; break;
    }
     if(gsmfr_header->params.vadDirection>0)
        gsmfr_header->params.vadMode=GSMFR_VAD_ON;
     else
        gsmfr_header->params.vadMode=GSMFR_VAD_OFF;
     gsmfr_header->params.ltp_f=LTP_OFF;//options->modes.truncate;
     gsmfr_header->params.direction=(GSMFR_DIRECTION_t)options->direction;
     gsmfr_header->fReset=0;
     gsmfr_header->fReset_old = 1;
     gsmfr_header->seed = INIT_SEED;

    if (options->direction == USC_ENCODE) /* encode only */
    {
        GSMFREncoder_Obj *EncObj = (GSMFREncoder_Obj *)((Ipp8s*)*handle + sizeof(GSMFR_Handle_Header));
        apiGSMFREncoder_Init(EncObj, &gsmfr_header->params);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        GSMFRDecoder_Obj *DecObj = (GSMFRDecoder_Obj *)((Ipp8s*)*handle + sizeof(GSMFR_Handle_Header));
        apiGSMFRDecoder_Init(DecObj, &gsmfr_header->params);

    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Reinit(const void *mode, USC_Handle handle )
{
    const USC_Modes *modes=mode;
    GSMFR_Handle_Header *gsmfr_header=(GSMFR_Handle_Header*)handle;

    USC_CHECK_HANDLE(handle);
    USC_CHECK_PTR(mode);

    USC_BADARG(modes->vad > 2, USC_UnsupportedVADType);
    USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

    USC_BADARG(CheckRate_GSMFR(modes->bitrate) < 0, USC_UnsupportedBitRate);

    if (gsmfr_header->direction == USC_ENCODE) /* encode only */
    {
        GSMFREncoder_Obj *EncObj = (GSMFREncoder_Obj *)((Ipp8s*)handle + sizeof(GSMFR_Handle_Header));
        apiGSMFREncoder_Init(EncObj, &gsmfr_header->params);
    }
    else if (gsmfr_header->direction == USC_DECODE) /* decode only */
    {
        GSMFRDecoder_Obj *DecObj = (GSMFRDecoder_Obj *)((Ipp8s*)handle + sizeof(GSMFR_Handle_Header));
        gsmfr_header->fReset=0;
        gsmfr_header->fReset_old = 1;
        gsmfr_header->seed = INIT_SEED;

        apiGSMFRDecoder_Init(DecObj, &gsmfr_header->params);
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(const void *mode, USC_Handle handle )
{
   const USC_Modes *modes=mode;
   GSMFR_Handle_Header *gsmfr_header;

   USC_CHECK_HANDLE(handle);
   USC_CHECK_PTR(mode);

   USC_BADARG(modes->vad > 2, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   USC_BADARG(CheckRate_GSMFR(modes->bitrate) < 0, USC_UnsupportedBitRate);

   gsmfr_header = (GSMFR_Handle_Header*)handle;
   switch(modes->vad) {
      case 1:  gsmfr_header->params.vadDirection = GSMFR_DOWNLINK; break;
      case 2:  gsmfr_header->params.vadDirection = GSMFR_UPLINK; break;
      default: gsmfr_header->params.vadDirection = GSMFR_VAD_OFF; break;
   }
   if(gsmfr_header->params.vadDirection>0)
      gsmfr_header->params.vadMode=GSMFR_VAD_ON;
   else
      gsmfr_header->params.vadMode=GSMFR_VAD_OFF;

   if (gsmfr_header->direction == USC_ENCODE) /* encode only */
   {
      GSMFREncoder_Obj *EncObj = (GSMFREncoder_Obj *)((Ipp8s*)handle + sizeof(GSMFR_Handle_Header));
      apiGSMFREncoder_Init(EncObj, &gsmfr_header->params);
   }
   return USC_NoError;
}

#define EHF_MASK 0x0008

static Ipp32s is_pcm_frame_homing (const USC_PCMStream *in) {
    Ipp32s i, k=0;
    Ipp16s *pSrc = (Ipp16s *)in->pBuffer;
    for(i = 0; i < (Ipp32s)(in->nbytes/sizeof(Ipp16s)); i++) {
        k = pSrc[i] ^ EHF_MASK;
        if(k)
            break;
    }
    return !k;
}

static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
    GSMFR_Handle_Header *gsmfr_header;
    GSMFREncoder_Obj *EncObj;
    Ipp32s vad, hangover,reset_flag;

    USC_CHECK_PTR(in);
    USC_BADARG(NULL == in->pBuffer, USC_NoOperation);
    USC_CHECK_PTR(out);
    USC_BADARG(NULL == out->pBuffer, USC_NoOperation);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(in->pcmType.bitPerSample!=GSMFR_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.sample_frequency!=GSMFR_SAMPLE_FREQ, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.nChannels!=GSMFR_NCHANNELS, USC_UnsupportedPCMType);

    USC_BADARG(CheckRate_GSMFR(in->bitrate) < 0, USC_UnsupportedBitRate);

    if(in->nbytes < GSMFR_EXT_FRAME_LEN*sizeof(Ipp16s)) {
       return USC_NoOperation;
    }

    gsmfr_header = (GSMFR_Handle_Header*)handle;

    if(handle == NULL) return USC_InvalidHandler;

    USC_BADARG(USC_ENCODE != gsmfr_header->direction, USC_NoOperation);

    EncObj = (GSMFREncoder_Obj *)((Ipp8s*)handle + sizeof(GSMFR_Handle_Header));

    /* check for homing frame */
    reset_flag = is_pcm_frame_homing(in);

    if(apiGSMFREncode(EncObj,(const Ipp16s *)in->pBuffer,(Ipp8u *)out->pBuffer,&vad,&hangover ) != APIGSMFR_StsNoErr){
        return USC_NoOperation;
    }
    if(reset_flag != 0) {
        apiGSMFREncoder_Init(EncObj, &gsmfr_header->params);
    }
    in->nbytes = GSMFR_EXT_FRAME_LEN*sizeof(Ipp16s);
    out->bitrate=in->bitrate;
    out->nbytes=GSMFR_PACKED_FRAME_LEN;
    if(gsmfr_header->params.vadMode == GSMFR_VAD_ON ){
        if(vad){/* voice activity detected */
            out->frametype=1;/* speech frame */
        }else{
            if(hangover) {
                out->frametype=2;/* silence frame as speech frame, SP=1 */
            }else{
                out->frametype=0;/* SID frame, SP=0   */
            }
        }
    }else{
        out->frametype=-1;
    }

    return USC_NoError;
}

static void B2R(const Ipp8u *pSrc, Ipp16s *out_buff_cur)
{
   frame *pf = (frame*)out_buff_cur;
   pf->LARc[0]  = (Ipp16s)((pSrc[0] & 0xF) << 2);
   pf->LARc[0] |= (pSrc[1] >> 6) & 0x3;
   pf->LARc[1]  = (Ipp16s)(pSrc[1] & 0x3F);
   pf->LARc[2]  =(Ipp16s)( (pSrc[2] >> 3) & 0x1F);
   pf->LARc[3]  =(Ipp16s)( (pSrc[2] & 0x7) << 2);
   pf->LARc[3] |=(Ipp16s)( (pSrc[3] >> 6) & 0x3);
   pf->LARc[4]  =(Ipp16s)( (pSrc[3] >> 2) & 0xF);
   pf->LARc[5]  =(Ipp16s)( (pSrc[3] & 0x3) << 2);
   pf->LARc[5] |=(Ipp16s)( (pSrc[4] >> 6) & 0x3);
   pf->LARc[6]  =(Ipp16s)( (pSrc[4] >> 3) & 0x7);
   pf->LARc[7]  =(Ipp16s)( pSrc[4] & 0x7);
   pf->Ncr0     =(Ipp16s)( (pSrc[5] >> 1) & 0x7F);
   pf->bcr0     =(Ipp16s)( (pSrc[5] & 0x1) << 1);
   pf->bcr0    |=(Ipp16s)( (pSrc[6] >> 7) & 0x1);
   pf->Mcr0     =(Ipp16s)( (pSrc[6] >> 5) & 0x3);
   pf->xmaxc0   =(Ipp16s)( (pSrc[6] & 0x1F) << 1);
   pf->xmaxc0  |=(Ipp16s)( (pSrc[7] >> 7) & 0x1);
   pf->xmcr0[0]  =(Ipp16s)( (pSrc[7] >> 4) & 0x7);
   pf->xmcr0[1]  =(Ipp16s)( (pSrc[7] >> 1) & 0x7);
   pf->xmcr0[2]  =(Ipp16s)( (pSrc[7] & 0x1) << 2);
   pf->xmcr0[2] |=(Ipp16s)( (pSrc[8] >> 6) & 0x3);
   pf->xmcr0[3]  =(Ipp16s)( (pSrc[8] >> 3) & 0x7);
   pf->xmcr0[4]  =(Ipp16s)( pSrc[8] & 0x7);
   pf->xmcr0[5]  =(Ipp16s)( (pSrc[9] >> 5) & 0x7);
   pf->xmcr0[6]  =(Ipp16s)( (pSrc[9] >> 2) & 0x7);
   pf->xmcr0[7]  =(Ipp16s)( (pSrc[9] & 0x3) << 1);
   pf->xmcr0[7] |=(Ipp16s)( (pSrc[10] >> 7) & 0x1);
   pf->xmcr0[8]  =(Ipp16s)( (pSrc[10] >> 4) & 0x7);
   pf->xmcr0[9]  =(Ipp16s)( (pSrc[10] >> 1) & 0x7);
   pf->xmcr0[10] =(Ipp16s)( (pSrc[10] & 0x1) << 2);
   pf->xmcr0[10]|=(Ipp16s)( (pSrc[11] >> 6) & 0x3);
   pf->xmcr0[11] =(Ipp16s)( (pSrc[11] >> 3) & 0x7);
   pf->xmcr0[12] =(Ipp16s)( pSrc[11] & 0x7);
   pf->Ncr1      =(Ipp16s)( (pSrc[12] >> 1) & 0x7F);
   pf->bcr1      =(Ipp16s)( (pSrc[12] & 0x1) << 1);
   pf->bcr1     |=(Ipp16s)( (pSrc[13] >> 7) & 0x1);
   pf->Mcr1      =(Ipp16s)( (pSrc[13] >> 5) & 0x3);
   pf->xmaxc1    =(Ipp16s)( (pSrc[13] & 0x1F) << 1);
   pf->xmaxc1   |=(Ipp16s)( (pSrc[14] >> 7) & 0x1);
   pf->xmcr1[0]  =(Ipp16s)( (pSrc[14] >> 4) & 0x7);
   pf->xmcr1[1]  =(Ipp16s)( (pSrc[14] >> 1) & 0x7);
   pf->xmcr1[2]  =(Ipp16s)( (pSrc[14] & 0x1) << 2);
   pf->xmcr1[2] |=(Ipp16s)( (pSrc[15] >> 6) & 0x3);
   pf->xmcr1[3]  =(Ipp16s)( (pSrc[15] >> 3) & 0x7);
   pf->xmcr1[4]  =(Ipp16s)( pSrc[15] & 0x7);
   pf->xmcr1[5]  =(Ipp16s)( (pSrc[16] >> 5) & 0x7);
   pf->xmcr1[6]  =(Ipp16s)( (pSrc[16] >> 2) & 0x7);
   pf->xmcr1[7]  =(Ipp16s)( (pSrc[16] & 0x3) << 1);
   pf->xmcr1[7] |=(Ipp16s)( (pSrc[17] >> 7) & 0x1);
   pf->xmcr1[8]  =(Ipp16s)( (pSrc[17] >> 4) & 0x7);
   pf->xmcr1[9]  =(Ipp16s)( (pSrc[17] >> 1) & 0x7);
   pf->xmcr1[10] =(Ipp16s)( (pSrc[17] & 0x1) << 2);
   pf->xmcr1[10]|=(Ipp16s)( (pSrc[18] >> 6) & 0x3);
   pf->xmcr1[11] =(Ipp16s)( (pSrc[18] >> 3) & 0x7);
   pf->xmcr1[12] =(Ipp16s)( pSrc[18] & 0x7);
   pf->Ncr2      =(Ipp16s)( (pSrc[19] >> 1) & 0x7F);
   pf->bcr2      =(Ipp16s)( (pSrc[19] & 0x1) << 1);
   pf->bcr2     |=(Ipp16s)( (pSrc[20] >> 7) & 0x1);
   pf->Mcr2      =(Ipp16s)( (pSrc[20] >> 5) & 0x3);
   pf->xmaxc2    =(Ipp16s)( (pSrc[20] & 0x1F) << 1);
   pf->xmaxc2   |=(Ipp16s)( (pSrc[21] >> 7) & 0x1);
   pf->xmcr2[0]  =(Ipp16s)( (pSrc[21] >> 4) & 0x7);
   pf->xmcr2[1]  =(Ipp16s)( (pSrc[21] >> 1) & 0x7);
   pf->xmcr2[2]  =(Ipp16s)( (pSrc[21] & 0x1) << 2);
   pf->xmcr2[2] |=(Ipp16s)( (pSrc[22] >> 6) & 0x3);
   pf->xmcr2[3]  =(Ipp16s)( (pSrc[22] >> 3) & 0x7);
   pf->xmcr2[4]  =(Ipp16s)( pSrc[22] & 0x7);
   pf->xmcr2[5]  =(Ipp16s)( (pSrc[23] >> 5) & 0x7);
   pf->xmcr2[6]  =(Ipp16s)( (pSrc[23] >> 2) & 0x7);
   pf->xmcr2[7]  =(Ipp16s)( (pSrc[23] & 0x3) << 1);
   pf->xmcr2[7] |=(Ipp16s)( (pSrc[24] >> 7) & 0x1);
   pf->xmcr2[8]  =(Ipp16s)( (pSrc[24] >> 4) & 0x7);
   pf->xmcr2[9]  =(Ipp16s)( (pSrc[24] >> 1) & 0x7);
   pf->xmcr2[10] =(Ipp16s)( (pSrc[24] & 0x1) << 2);
   pf->xmcr2[10] |=(Ipp16s)( (pSrc[25] >> 6) & 0x3);
   pf->xmcr2[11]  =(Ipp16s)( (pSrc[25] >> 3) & 0x7);
   pf->xmcr2[12]  =(Ipp16s)( pSrc[25] & 0x7);
   pf->Ncr3       =(Ipp16s)( (pSrc[26] >> 1) & 0x7F);
   pf->bcr3       =(Ipp16s)( (pSrc[26] & 0x1) << 1);
   pf->bcr3      |=(Ipp16s)( (pSrc[27] >> 7) & 0x1);
   pf->Mcr3       =(Ipp16s)( (pSrc[27] >> 5) & 0x3);
   pf->xmaxc3     =(Ipp16s)( (pSrc[27] & 0x1F) << 1);
   pf->xmaxc3    |=(Ipp16s)( (pSrc[28] >> 7) & 0x1);
   pf->xmcr3[0]   =(Ipp16s)( (pSrc[28] >> 4) & 0x7);
   pf->xmcr3[1]   =(Ipp16s)( (pSrc[28] >> 1) & 0x7);
   pf->xmcr3[2]   =(Ipp16s)( (pSrc[28] & 0x1) << 2);
   pf->xmcr3[2]  |=(Ipp16s)( (pSrc[29] >> 6) & 0x3);
   pf->xmcr3[3]   =(Ipp16s)( (pSrc[29] >> 3) & 0x7);
   pf->xmcr3[4]   =(Ipp16s)( pSrc[29] & 0x7);
   pf->xmcr3[5]   =(Ipp16s)( (pSrc[30] >> 5) & 0x7);
   pf->xmcr3[6]   =(Ipp16s)( (pSrc[30] >> 2) & 0x7);
   pf->xmcr3[7]   =(Ipp16s)( (pSrc[30] & 0x3) << 1);
   pf->xmcr3[7]  |=(Ipp16s)( (pSrc[31] >> 7) & 0x1);
   pf->xmcr3[8]   =(Ipp16s)( (pSrc[31] >> 4) & 0x7);
   pf->xmcr3[9]   =(Ipp16s)( (pSrc[31] >> 1) & 0x7);
   pf->xmcr3[10]  =(Ipp16s)( (pSrc[31] & 0x1) << 2);
   pf->xmcr3[10] |=(Ipp16s)( (pSrc[32] >> 6) & 0x3);
   pf->xmcr3[11]  =(Ipp16s)( (pSrc[32] >> 3) & 0x7);
   pf->xmcr3[12]  =(Ipp16s)( pSrc[32] & 0x7);

   return;
}

static Ipp32s Rand_1_6(Ipp16s *pseed)/* generate random number from 1..6*/
{
  Ipp16s tmp;
   tmp = Rand_16s(pseed);/*1<=x<=6*/
   tmp = (Ipp16s)((((tmp&0x7fff) *6)>>14)+1);
   if(tmp>=6) {
      tmp=6;
   }
   return tmp;
}
static Ipp32s Rand_0_3(Ipp16s *pseed)/* generate random number from 1..6*/
{
  Ipp16s tmp;
   tmp = Rand_16s(pseed);/*0<=x<=3*/
   tmp = (Ipp16s)((((tmp&0x7fff) *3)>>14)&0x0003);
   return tmp;
}
static void generateNoise(Ipp16s *pseed,Ipp16s *source) /*  ref vector unpacked bitstream */
{
Ipp16s i;
  frame *fptr = (frame*)source;/* to input SID frame */
/*  for(i=0;i<8;i++)
    LARc[i] =  LARc[i]; used as is. gsm06.12 clause 6.1 */
  for(i=0;i<13;i++)
      fptr-> xmcr0[i] = (Ipp16s)Rand_1_6(pseed);/*1<=x<=6*/
   for(i=0;i<13;i++)
      fptr-> xmcr1[i] = (Ipp16s)Rand_1_6(pseed);/*1<=x<=6*/
   for(i=0;i<13;i++)
      fptr-> xmcr2[i] = (Ipp16s)Rand_1_6(pseed);/*1<=x<=6*/
   for(i=0;i<13;i++)
      fptr-> xmcr3[i] = (Ipp16s)Rand_1_6(pseed);/*1<=x<=6*/
   fptr-> Mcr0 = (Ipp16s)Rand_0_3(pseed);/*0<=x<=3*/
   fptr-> Mcr1 = (Ipp16s)Rand_0_3(pseed);
   fptr-> Mcr2 = (Ipp16s)Rand_0_3(pseed);
   fptr-> Mcr3 = (Ipp16s)Rand_0_3(pseed);

   fptr-> bcr0 = 0;
   fptr-> bcr1 = 0;
   fptr-> bcr2 = 0;
   fptr-> bcr3 = 0;

   fptr-> Ncr0 = 40;
   fptr-> Ncr1 = 120;
   fptr-> Ncr2 = 40;
   fptr-> Ncr3 = 120;

/*   fptr-> xmaxc0 =  xmaxc0;
   fptr-> xmaxc1 =  xmaxc1;
   fptr-> xmaxc2 =  xmaxc2;
   fptr-> xmaxc3 =  xmaxc3; used as is*/
}
static void mute(Ipp16s *pseed,Ipp16s *pSrc){
   frame *fptr = (frame*)pSrc;
   fptr-> Mcr0 = (Ipp16s)Rand_0_3(pseed);/*0<=x<=3*/
   fptr-> Mcr1 = (Ipp16s)Rand_0_3(pseed);
   fptr-> Mcr2 = (Ipp16s)Rand_0_3(pseed);
   fptr-> Mcr3 = (Ipp16s)Rand_0_3(pseed);

   fptr-> xmaxc0 -= 4;/* down to zero*/
   if(fptr-> xmaxc0<0)
       fptr-> xmaxc0 = 0;
   fptr-> xmaxc1 -= 4;
   if(fptr-> xmaxc1<0)
       fptr-> xmaxc1 = 0;
   fptr-> xmaxc2 -= 4;
   if(fptr-> xmaxc2<0)
       fptr-> xmaxc2 = 0;
   fptr-> xmaxc3 -= 4;
   if(fptr-> xmaxc3<0)
       fptr-> xmaxc3 = 0;
}

static void R2B(const Ipp16s *pSrc, /*  ref vector unpacked bitstream */
              Ipp8u *pDst)/*  packed bitstream  */
{
const frame *pf = (frame*)pSrc;
   pDst[0]  =(uchar)( (pf->LARc[0] >> 2) & 0xF);
   pDst[1]  =(uchar)( ((pf->LARc[0] & 0x3) << 6) | (pf->LARc[1] & 0x3F));
   pDst[2]  =(uchar)( ((pf->LARc[2] & 0x1F) << 3)| ((pf->LARc[3] >> 2) & 0x7));
   pDst[3]  =(uchar)( ((pf->LARc[3] & 0x3) << 6) | ((pf->LARc[4] & 0xF) << 2) | ((pf->LARc[5] >> 2) & 0x3));
   pDst[4]  =(uchar)( ((pf->LARc[5] & 0x3) << 6) | ((pf->LARc[6] & 0x7) << 3) | (pf->LARc[7] & 0x7));
   pDst[5]  =(uchar)( ((pf->Ncr0 & 0x7F) << 1)  | ((pf->bcr0 >> 1) & 0x1));
   pDst[6]  =(uchar)( ((pf->bcr0 & 0x1) << 7)   | ((pf->Mcr0 & 0x3) << 5) | ((pf->xmaxc0 >> 1) & 0x1F));
   pDst[7]  =(uchar)( ((pf->xmaxc0 & 0x1) << 7)| ((pf->xmcr0[0] & 0x7) << 4)| ((pf->xmcr0[1] & 0x7) << 1)|
              ((pf->xmcr0[2] >> 2) & 0x1));
   pDst[8]  =(uchar)( ((pf->xmcr0[2] & 0x3) << 6)| ((pf->xmcr0[3] & 0x7) << 3)| (pf->xmcr0[4] & 0x7));
   pDst[9]  =(uchar)( ((pf->xmcr0[5] & 0x7) << 5)| ((pf->xmcr0[6] & 0x7) << 2)| ((pf->xmcr0[7] >> 1) & 0x3));
   pDst[10] =(uchar)( ((pf->xmcr0[7] & 0x1) << 7)| ((pf->xmcr0[8] & 0x7) << 4)| ((pf->xmcr0[9] & 0x7) << 1)|
              ((pf->xmcr0[10] >> 2) & 0x1));
   pDst[11] =(uchar)( ((pf->xmcr0[10] & 0x3) << 6)| ((pf->xmcr0[11] & 0x7) << 3)| (pf->xmcr0[12] & 0x7));
   pDst[12] =(uchar)( ((pf->Ncr1 & 0x7F) << 1)  | ((pf->bcr1 >> 1) & 0x1));
   pDst[13] =(uchar)( ((pf->bcr1 & 0x1) << 7)   | ((pf->Mcr1 & 0x3) << 5)| ((pf->xmaxc1 >> 1) & 0x1F));
   pDst[14] =(uchar)( ((pf->xmaxc1 & 0x1) << 7)| ((pf->xmcr1[0] & 0x7) << 4)| ((pf->xmcr1[1] & 0x7) << 1)|
              ((pf->xmcr1[2] >> 2) & 0x1));
   pDst[15] =(uchar)( ((pf->xmcr1[2] & 0x3) << 6)| ((pf->xmcr1[3] & 0x7) << 3)| (pf->xmcr1[4] & 0x7));
   pDst[16] =(uchar)( ((pf->xmcr1[5] & 0x7) << 5)| ((pf->xmcr1[6] & 0x7) << 2)| ((pf->xmcr1[7] >> 1) & 0x3));
   pDst[17] =(uchar)( ((pf->xmcr1[7] & 0x1) << 7)| ((pf->xmcr1[8] & 0x7) << 4)|
              ((pf->xmcr1[9] & 0x7) << 1)| ((pf->xmcr1[10] >> 2) & 0x1));
   pDst[18] =(uchar)( ((pf->xmcr1[10] & 0x3) << 6)| ((pf->xmcr1[11] & 0x7) << 3)| (pf->xmcr1[12] & 0x7));
   pDst[19] =(uchar)( ((pf->Ncr2 & 0x7F) << 1)  | ((pf->bcr2 >> 1) & 0x1));
   pDst[20] =(uchar)( ((pf->bcr2 & 0x1) << 7)   | ((pf->Mcr2 & 0x3) << 5)| ((pf->xmaxc2 >> 1) & 0x1F));
   pDst[21] =(uchar)( ((pf->xmaxc2 & 0x1) << 7)| ((pf->xmcr2[0] & 0x7) << 4)| ((pf->xmcr2[1] & 0x7) << 1)|
              ((pf->xmcr2[2] >> 2) & 0x1));
   pDst[22] =(uchar)( ((pf->xmcr2[2] & 0x3) << 6)| ((pf->xmcr2[3] & 0x7) << 3)| (pf->xmcr2[4] & 0x7));
   pDst[23] =(uchar)( ((pf->xmcr2[5] & 0x7) << 5)| ((pf->xmcr2[6] & 0x7) << 2)| ((pf->xmcr2[7] >> 1) & 0x3));
   pDst[24] =(uchar)( ((pf->xmcr2[7] & 0x1) << 7)| ((pf->xmcr2[8] & 0x7) << 4)| ((pf->xmcr2[9] & 0x7) << 1)|
              ((pf->xmcr2[10] >> 2) & 0x1));
   pDst[25] =(uchar)( ((pf->xmcr2[10] & 0x3) << 6) | ((pf->xmcr2[11] & 0x7) << 3)| (pf->xmcr2[12] & 0x7));
   pDst[26] =(uchar)( ((pf->Ncr3 & 0x7F) << 1)   | ((pf->bcr3 >> 1) & 0x1));
   pDst[27] =(uchar)( ((pf->bcr3 & 0x1) << 7)    | ((pf->Mcr3 & 0x3) << 5) | ((pf->xmaxc3 >> 1) & 0x1F));
   pDst[28] =(uchar)( ((pf->xmaxc3 & 0x1) << 7) | ((pf->xmcr3[0] & 0x7) << 4) | ((pf->xmcr3[1] & 0x7) << 1)|
              ((pf->xmcr3[2] >> 2) & 0x1));
   pDst[29] =(uchar)( ((pf->xmcr3[2] & 0x3) << 6) | ((pf->xmcr3[3] & 0x7) << 3) | (pf->xmcr3[4] & 0x7));
   pDst[30] =(uchar)( ((pf->xmcr3[5] & 0x7) << 5) | ((pf->xmcr3[6] & 0x7) << 2) | ((pf->xmcr3[7] >> 1) & 0x3));
   pDst[31] =(uchar)( ((pf->xmcr3[7] & 0x1) << 7) | ((pf->xmcr3[8] & 0x7) << 4) | ((pf->xmcr3[9] & 0x7) << 1)|
              ((pf->xmcr3[10] >> 2) & 0x1));
   pDst[32] =(uchar)( ((pf->xmcr3[10] & 0x3) << 6) | ((pf->xmcr3[11] & 0x7) << 3) | (pf->xmcr3[12] & 0x7));
}

static Ipp32s is_bitstream_frame_homing(Ipp16s *src, Ipp32s len)
{
  Ipp32s i,k=0;
  static __ALIGN32 CONST Ipp32s maskaTable[76] = {6, 6, 5, 5, 4, 4, 3, 3,7, 2, 2, 6, 3, 3, 3, 3,3, 3, 3, 3, 3, 3, 3, 3,
                          3,7, 2, 2, 6, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,7, 2, 2, 6, 3, 3,
                          3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,7, 2, 2, 6, 3, 3, 3,3, 3, 3, 3, 3, 3,
                          3, 3, 3, 3};
  static __ALIGN32 CONST Ipp16s maska[] ={
        0x0009,  /* LARc[1]  */
        0x0017,  /* LARc[2]  */
        0x000f,  /* LARc[3]  */
        0x0008,  /* LARc[4]  */
        0x0007,  /* LARc[5]  */
        0x0003,  /* LARc[6]  */
        0x0003,  /* LARc[7]  */
        0x0002,  /* LARc[8]  */
        0x0028,  /* LTP lag  */
        0x0000,  /* LTP gain */
        0x0000,  /* RPE grid */
        0x0000,  /* block amplitude */
        0x0004,  /* RPE pulse 0 */
        0x0004,  /* RPE pulse 1 */
        0x0004,  /* RPE pulse 2 */
        0x0004,  /* RPE pulse 3 */
        0x0004,  /* RPE pulse 4 */
        0x0004,  /* RPE pulse 5 */
        0x0004,  /* RPE pulse 6 */
        0x0004,  /* RPE pulse 7 */
        0x0004,  /* RPE pulse 8 */
        0x0004,  /* RPE pulse 9 */
        0x0004,  /* RPE pulse 10 */
        0x0004,  /* RPE pulse 11 */
        0x0004,  /* RPE pulse 12 */
        0x0028,  /* LTP lag  */
        0x0000,  /* LTP gain */
        0x0000,  /* RPE grid */
        0x0000,  /* block amplitude */
        0x0004,  /* RPE pulse 0 */
        0x0004,  /* RPE pulse 1 */
        0x0004,  /* RPE pulse 2 */
        0x0004,  /* RPE pulse 3 */
        0x0004,  /* RPE pulse 4 */
        0x0004,  /* RPE pulse 5 */
        0x0004,  /* RPE pulse 6 */
        0x0004,  /* RPE pulse 7 */
        0x0004,  /* RPE pulse 8 */
        0x0004,  /* RPE pulse 9 */
        0x0004,  /* RPE pulse 10 */
        0x0004,  /* RPE pulse 11 */
        0x0004,  /* RPE pulse 12 */
        0x0028,  /* LTP lag  */
        0x0000,  /* LTP gain */
        0x0000,  /* RPE grid */
        0x0000,  /* block amplitude */
        0x0004,  /* RPE pulse 0 */
        0x0004,  /* RPE pulse 1 */
        0x0004,  /* RPE pulse 2 */
        0x0004,  /* RPE pulse 3 */
        0x0004,  /* RPE pulse 4 */
        0x0004,  /* RPE pulse 5 */
        0x0004,  /* RPE pulse 6 */
        0x0004,  /* RPE pulse 7 */
        0x0004,  /* RPE pulse 8 */
        0x0004,  /* RPE pulse 9 */
        0x0004,  /* RPE pulse 10 */
        0x0004,  /* RPE pulse 11 */
        0x0004,  /* RPE pulse 12 */
        0x0028,  /* LTP lag  */
        0x0000,  /* LTP gain */
        0x0000,  /* RPE grid */
        0x0000,  /* block amplitude */
        0x0004,  /* RPE pulse 0 */
        0x0004,  /* RPE pulse 1 */
        0x0004,  /* RPE pulse 2 */
        0x0004,  /* RPE pulse 3 */
        0x0003,  /* RPE pulse 4 */
        0x0004,  /* RPE pulse 5 */
        0x0004,  /* RPE pulse 6 */
        0x0004,  /* RPE pulse 7 */
        0x0004,  /* RPE pulse 8 */
        0x0004,  /* RPE pulse 9 */
        0x0004,  /* RPE pulse 10 */
        0x0004,  /* RPE pulse 11 */
        0x0004}; /* RPE pulse 12 */
   for(i = 0; i < len; i++) {
      k = ((src[i] & ~(~0 << maskaTable[i])) ^ maska[i]);
      if(k) break;
   }
   return !k;
}
static __ALIGN32 CONST frame silenceFrame = {
  42,39,21,10,9,4,3,2,
  0,
  40,
  1,
  0,
  3,4,3,4,4,3,3,3,3,4,4,3,3,

  0,
  40,
  1,
  0,
  3,4,3,4,4,3,3,3,3,4,4,3,3,

  0,
  40,
  1,
  0,
  3,4,3,4,4,3,3,3,3,4,4,3,3,

  0,
  40,
  1,
  0,
  3,4,3,4,4,3,3,3,3,4,4,3,3
};

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
    GSMFR_Handle_Header *gsmfr_header;
    GSMFRDecoder_Obj *DecObj;
    Ipp8u *srcFrame;
    Ipp32s bfi;
    IPP_ALIGNED_ARRAY(16, Ipp16s, workBuff, GSMFR_INT_FRAME_LEN);
    IPP_ALIGNED_ARRAY(16, Ipp8u, workBuff2, GSMFR_PACKED_FRAME_LEN);

    USC_CHECK_PTR(out);
    USC_BADARG(NULL == out->pBuffer, USC_NoOperation);
    USC_CHECK_HANDLE(handle);

    gsmfr_header = (GSMFR_Handle_Header*)handle;

    USC_BADARG(USC_DECODE != gsmfr_header->direction, USC_NoOperation);

    DecObj = (GSMFRDecoder_Obj *)((Ipp8s*)handle + sizeof(GSMFR_Handle_Header));

    if(!in){
        bfi=1;
        srcFrame = workBuff2;
    }else{
        out->bitrate = in->bitrate;
        if(!in->pBuffer){
            bfi=1;
            srcFrame = workBuff2;
        }else{
            if(in->frametype > 2){/* it's the bad frame, BFI=1*/
                bfi=1;
                srcFrame = workBuff2;
                in->nbytes=0;
            }else{
                bfi=0;
                srcFrame = (Ipp8u *)in->pBuffer;
                in->nbytes=GSMFR_PACKED_FRAME_LEN;
            }
        }
    }

    if(!bfi){/* store good frame for the subsequent lost SID or speech frame case*/
        ippsCopy_8u(srcFrame,gsmfr_header->validFrame,GSMFR_PACKED_FRAME_LEN);
        gsmfr_header->lostFrames = 0;
        gsmfr_header->validFrames = 1;
    }else{/* the lost frame case */
        ++gsmfr_header->lostFrames;
        if(gsmfr_header->lostFrames==1){
           if(gsmfr_header->validFrames){/* repeat the first lost speech or SID frame */
              ippsCopy_8u(gsmfr_header->validFrame,srcFrame,GSMFR_PACKED_FRAME_LEN);
           }else{
              R2B((const Ipp16s *)&silenceFrame, srcFrame );
/*              printf( "Bad first input frame\n");
              return 0;*/
           }
        }else{/* second and next lost speech or SID frame, mute procedure to be applied  */
           if(gsmfr_header->lostFrames>16){/* 320ms passed since first bad frame */
              R2B((const Ipp16s *)&silenceFrame, srcFrame );
           }else{
              B2R(gsmfr_header->validFrame, workBuff );
              mute(&gsmfr_header->seed,workBuff);
              R2B(workBuff, gsmfr_header->validFrame );
              srcFrame = gsmfr_header->validFrame;
           }
        }
    }
    B2R(srcFrame, workBuff );
    if(in){
        if(!in->frametype && in->pBuffer){/* SID frame */
            generateNoise(&gsmfr_header->seed,workBuff);/* comfort noise generation on current SID*/
        }else{
            gsmfr_header->seed=INIT_SEED;
        }
    }else{
        gsmfr_header->seed=INIT_SEED;
    }
    if(gsmfr_header->fReset_old == 1) {
        gsmfr_header->fReset=is_bitstream_frame_homing(workBuff, 25);
    }
    if(gsmfr_header->fReset && gsmfr_header->fReset_old) {
        ippsSet_16s( EHF_MASK, (Ipp16s*) out->pBuffer, (GSMFR_EXT_FRAME_LEN*sizeof(Ipp16s))>>1 );
    } else {
        R2B( workBuff, srcFrame );
        if(apiGSMFRDecode(DecObj,srcFrame,(Ipp16s*)out->pBuffer) != APIGSMFR_StsNoErr){
            return USC_NoOperation;
        }
    }
    if(!gsmfr_header->fReset_old) {
        gsmfr_header->fReset=is_bitstream_frame_homing(workBuff, 76);
    }

    if(gsmfr_header->fReset) {
        apiGSMFRDecoder_Init(DecObj, &gsmfr_header->params);
        gsmfr_header->seed=INIT_SEED;
    }
    gsmfr_header->fReset_old = gsmfr_header->fReset;
    out->nbytes = GSMFR_EXT_FRAME_LEN*sizeof(Ipp16s);
    out->pcmType.sample_frequency = GSMFR_SAMPLE_FREQ;
    out->pcmType.bitPerSample = GSMFR_BITS_PER_SAMPLE;
    out->pcmType.nChannels = GSMFR_NCHANNELS;

    return USC_NoError;
}

static USC_Status CalsOutStreamSize(const USC_Option *options, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s nBlocks, nSamples;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);
   USC_BADARG(options->modes.vad>2, USC_UnsupportedVADType);

   if(options->direction==USC_ENCODE) { /*Encode: src - PCMStream, dst - bitstream*/
      nSamples = nbytesSrc / (GSMFR_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / GSMFR_EXT_FRAME_LEN;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % GSMFR_EXT_FRAME_LEN) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }

      *nbytesDst = nBlocks * (GSMFR_PACKED_FRAME_LEN);
   } else if(options->direction==USC_DECODE) {/*Decode: src - bitstream, dst - PCMStream*/
      nBlocks = nbytesSrc / (GSMFR_PACKED_FRAME_LEN);

      if (0 == nBlocks) return USC_NoOperation;

      nSamples = nBlocks * GSMFR_EXT_FRAME_LEN;
      *nbytesDst = nSamples * (GSMFR_BITS_PER_SAMPLE >> 3);
   } else if(options->direction==USC_DUPLEX) {/* Both: src - PCMStream, dst - PCMStream*/
      nSamples = nbytesSrc / (GSMFR_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / GSMFR_EXT_FRAME_LEN;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % GSMFR_EXT_FRAME_LEN) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * GSMFR_EXT_FRAME_LEN * (GSMFR_BITS_PER_SAMPLE >> 3);
   } else return USC_NoOperation;

   return USC_NoError;
}
static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   bitrate_idx = CheckRate_GSMFR(bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

   return CalsOutStreamSize(options, nbytesSrc, nbytesDst);
}

static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize)
{
   USC_CHECK_PTR(options);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(frameSize <= 0, USC_BadArgument);

   return USC_NoOperation;
}
