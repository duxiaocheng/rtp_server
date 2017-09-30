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
// Purpose: G.729/A/B/D/E speech codec: USC functions.
//
*/
#include "usc.h"
#include "owng729.h"
#include "g729api.h"


#define  G729I_NUM_RATES  3
#define  G729A_NUM_RATES  1

static USC_Status GetInfoSize(Ipp32s *pSize);
static USC_Status GetInfo_G729I(USC_Handle handle, USC_CodecInfo *pInfo);
static USC_Status GetInfo_G729A(USC_Handle handle, USC_CodecInfo *pInfo);
static USC_Status NumAlloc(const USC_Option *options, Ipp32s *nbanks);
static USC_Status MemAlloc_G729I(const USC_Option *options, USC_MemBank *pBanks);
static USC_Status MemAlloc_G729A(const USC_Option *options, USC_MemBank *pBanks);
static USC_Status Init_G729I(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Init_G729A(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit_G729I(const USC_Modes *modes, USC_Handle handle );
static USC_Status Reinit_G729A(const USC_Modes *modes, USC_Handle handle );
static USC_Status Control_G729I(const USC_Modes *modes, USC_Handle handle );
static USC_Status Control_G729A(const USC_Modes *modes, USC_Handle handle );
static USC_Status Encode_G729I(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out);
static USC_Status Encode_G729A(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out);
static USC_Status Decode_G729I(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out);
static USC_Status Decode_G729A(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out);
static USC_Status GetOutStreamSizeI(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst);
static USC_Status GetOutStreamSizeA(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst);
static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize);

#define BITSTREAM_SIZE       15
#define G729_SPEECH_FRAME    80
#define G729_BITS_PER_SAMPLE 16
#define G729_SAMPLE_FREQUENCY 8000
#define G729_NCHANNELS 1
#define G729_SID_FRAMESIZE    2
#define G729_NUM_UNTR_FRAMES  2

typedef struct {
    USC_Direction direction;
    Ipp32s bitrate;
    Ipp32s vad;
    Ipp32s reserved; // for future extension
} G729_Handle_Header;

/* global usc vector table */
USCFUN USC_Fxns USC_G729I_Fxns=
{
    {
        USC_Codec,
        (GetInfoSize_func) GetInfoSize,
        (GetInfo_func)     GetInfo_G729I,
        (NumAlloc_func)    NumAlloc,
        (MemAlloc_func)    MemAlloc_G729I,
        (Init_func)        Init_G729I,
        (Reinit_func)      Reinit_G729I,
        (Control_func)     Control_G729I
    },
    Encode_G729I,
    Decode_G729I,
    GetOutStreamSizeI,
    SetFrameSize

};

USCFUN USC_Fxns USC_G729A_Fxns=
{
    {
        USC_Codec,
        (GetInfoSize_func) GetInfoSize,
        (GetInfo_func)     GetInfo_G729A,
        (NumAlloc_func)    NumAlloc,
        (MemAlloc_func)    MemAlloc_G729A,
        (Init_func)        Init_G729A,
        (Reinit_func)      Reinit_G729A,
        (Control_func)     Control_G729A
    },
    Encode_G729A,
    Decode_G729A,
    GetOutStreamSizeA,
    SetFrameSize

};

static __ALIGN32 CONST USC_Rates pTblRates_G729I[G729I_NUM_RATES]={
    {11800},
    {8000},
    {6400}
};

static __ALIGN32 CONST USC_Rates pTblRates_G729A[G729A_NUM_RATES]={
    {8000}
};
static __ALIGN32 CONST USC_PCMType pcmTypesTbl_G729I[1]={
   {G729_SAMPLE_FREQUENCY,G729_BITS_PER_SAMPLE,G729_NCHANNELS}
};
static Ipp32s CheckRate_G729I(Ipp32s rate_bps)
{
    Ipp32s rate;

    switch(rate_bps) {
      case 8000:  rate = 0; break;
      case 6400:  rate = 2; break;
      case 11800: rate = 3; break;
      default: rate = -1; break;
    }
    return rate;
}

static Ipp32s CheckRate_G729A(Ipp32s rate_bps)
{
    Ipp32s rate;

    switch(rate_bps) {
      case 8000:  rate = 1; break;
      default: rate = -1; break;
    }
    return rate;
}

static USC_Status GetInfoSize(Ipp32s *pSize)
{
   USC_CHECK_PTR(pSize);
   *pSize = sizeof(USC_CodecInfo);
   return USC_NoError;
}

static USC_Status GetInfo_G729I(USC_Handle handle, USC_CodecInfo *pInfo)
{

    G729_Handle_Header *g729_header;

    USC_CHECK_PTR(pInfo);

    pInfo->name = "IPP_G729I";
    pInfo->params.framesize = G729_SPEECH_FRAME*sizeof(Ipp16s);
    if (handle == NULL) {
       pInfo->params.modes.bitrate = 8000;
       pInfo->params.direction = USC_DECODE;
       pInfo->params.modes.vad = 1;
    } else {
       g729_header = (G729_Handle_Header*)handle;
       pInfo->params.modes.bitrate = g729_header->bitrate;
       pInfo->params.direction = g729_header->direction;
       pInfo->params.modes.vad = g729_header->vad;
    }
    pInfo->params.modes.truncate = 0;
    pInfo->maxbitsize = BITSTREAM_SIZE;
    pInfo->params.pcmType.sample_frequency = pcmTypesTbl_G729I[0].sample_frequency;
    pInfo->params.pcmType.nChannels = pcmTypesTbl_G729I[0].nChannels;
    pInfo->params.pcmType.bitPerSample = pcmTypesTbl_G729I[0].bitPerSample;
    pInfo->nPcmTypes = 1;
    pInfo->pPcmTypesTbl = pcmTypesTbl_G729I;
    pInfo->params.modes.hpf = 0;
    pInfo->params.modes.pf = 0;
    pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
    pInfo->params.law = 0;
    pInfo->nRates = G729I_NUM_RATES;
    pInfo->pRateTbl = (const USC_Rates *)&pTblRates_G729I;
    pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);

   return USC_NoError;
}

static USC_Status GetInfo_G729A(USC_Handle handle, USC_CodecInfo *pInfo)
{

    G729_Handle_Header *g729_header;

    USC_CHECK_PTR(pInfo);

    pInfo->name = "IPP_G729A";
    pInfo->params.framesize = G729_SPEECH_FRAME*sizeof(Ipp16s);
    if (handle == NULL) {
       pInfo->params.modes.bitrate = 8000;
       pInfo->params.direction = USC_DECODE;
       pInfo->params.modes.vad = 1;
    } else {
       g729_header = (G729_Handle_Header*)handle;
       pInfo->params.modes.bitrate = g729_header->bitrate;
       pInfo->params.direction = g729_header->direction;
       pInfo->params.modes.vad = g729_header->vad;
    }
    pInfo->params.modes.truncate = 0;
    pInfo->maxbitsize = BITSTREAM_SIZE;
    pInfo->params.pcmType.sample_frequency = pcmTypesTbl_G729I[0].sample_frequency;
    pInfo->params.pcmType.nChannels = pcmTypesTbl_G729I[0].nChannels;
    pInfo->params.pcmType.bitPerSample = pcmTypesTbl_G729I[0].bitPerSample;
    pInfo->nPcmTypes = 1;
    pInfo->pPcmTypesTbl = pcmTypesTbl_G729I;
    pInfo->params.modes.hpf = 0;
    pInfo->params.modes.pf = 0;
    pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
    pInfo->params.law = 0;
    pInfo->nRates = G729A_NUM_RATES;
    pInfo->pRateTbl = (const USC_Rates *)&pTblRates_G729A;
    pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);

    return USC_NoError;
}

static USC_Status NumAlloc(const USC_Option *options, Ipp32s *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);

   *nbanks = 2;

   return USC_NoError;
}

static USC_Status MemAlloc_G729I(const USC_Option *options, USC_MemBank *pBanks)
{
    Ipp32u nbytes;
    Ipp32s bitrate_idx;
    Ipp32s srcatchMemSize = 0;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);

    pBanks[0].pMem = NULL;
    pBanks[0].align = 32;
    pBanks[0].memType = USC_OBJECT;
    pBanks[0].memSpaceType = USC_NORMAL;

    bitrate_idx = CheckRate_G729I(options->modes.bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

    if (options->direction == USC_ENCODE) /* encode only */
    {
        apiG729Encoder_Alloc(G729I_CODEC, (Ipp32s*)&nbytes);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        apiG729Decoder_Alloc(G729I_CODEC, (Ipp32s*)&nbytes);
    } else {
        return USC_NoOperation;
    }
    pBanks[0].nbytes = nbytes + sizeof(G729_Handle_Header); /* include header in handle */

    pBanks[1].pMem = NULL;
    pBanks[1].align = 32;
    pBanks[1].memType = USC_BUFFER;
    pBanks[1].memSpaceType = USC_NORMAL;

    apiG729Codec_ScratchMemoryAlloc(&srcatchMemSize);

    pBanks[1].nbytes = srcatchMemSize;

    return USC_NoError;
}


static USC_Status MemAlloc_G729A(const USC_Option *options, USC_MemBank *pBanks)
{
    Ipp32u nbytes;
    Ipp32s bitrate_idx;
    Ipp32s srcatchMemSize = 0;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);

    pBanks[0].pMem = NULL;
    pBanks[0].align = 32;
    pBanks[0].memType = USC_OBJECT;
    pBanks[0].memSpaceType = USC_NORMAL;

    bitrate_idx = CheckRate_G729A(options->modes.bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

    if (options->direction == USC_ENCODE) /* encode only */
    {
        apiG729Encoder_Alloc((G729Codec_Type)bitrate_idx, (Ipp32s*)&nbytes);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        apiG729Decoder_Alloc((G729Codec_Type)bitrate_idx, (Ipp32s*)&nbytes);
    } else {
        return USC_NoOperation;
    }
    pBanks[0].nbytes = nbytes + sizeof(G729_Handle_Header); /* include header in handle */

    pBanks[1].pMem = NULL;
    pBanks[1].align = 32;
    pBanks[1].memType = USC_BUFFER;
    pBanks[1].memSpaceType = USC_NORMAL;

    apiG729Codec_ScratchMemoryAlloc(&srcatchMemSize);

    pBanks[1].nbytes = srcatchMemSize;

    return USC_NoError;
}

static USC_Status Init_G729I(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
    G729_Handle_Header *g729_header;
    Ipp32s bitrate_idx;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);
    USC_CHECK_HANDLE(handle);
    USC_CHECK_PTR(pBanks[0].pMem);
    USC_BADARG(pBanks[0].nbytes<=0, USC_NotInitialized);
    USC_CHECK_PTR(pBanks[1].pMem);
    USC_BADARG(pBanks[1].nbytes<=0, USC_NotInitialized);
    USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);
    USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);

    USC_BADARG(options->pcmType.bitPerSample!=G729_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.sample_frequency!=G729_SAMPLE_FREQUENCY, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.nChannels!=G729_NCHANNELS, USC_UnsupportedPCMType);

    bitrate_idx = CheckRate_G729I(options->modes.bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

    *handle = (USC_Handle*)pBanks[0].pMem;
    g729_header = (G729_Handle_Header*)*handle;

    g729_header->vad = options->modes.vad;
    g729_header->bitrate = options->modes.bitrate;
    g729_header->direction = options->direction;

    if (options->direction == USC_ENCODE) /* encode only */
    {
        G729Encoder_Obj *EncObj = (G729Encoder_Obj *)((Ipp8s*)*handle + sizeof(G729_Handle_Header));
        apiG729Encoder_InitBuff(EncObj, pBanks[1].pMem);
        apiG729Encoder_Init((G729Encoder_Obj*)EncObj,
            (G729Codec_Type)G729I_CODEC,(G729Encode_Mode)g729_header->vad);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        G729Decoder_Obj *DecObj = (G729Decoder_Obj *)((Ipp8s*)*handle + sizeof(G729_Handle_Header));
        apiG729Decoder_InitBuff(DecObj, pBanks[1].pMem);
        apiG729Decoder_Init((G729Decoder_Obj*)DecObj, (G729Codec_Type)G729I_CODEC);
    } else {
        *handle = NULL;
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Init_G729A(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
    G729_Handle_Header *g729_header;
    Ipp32s bitrate_idx;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);
    USC_CHECK_HANDLE(handle);
    USC_CHECK_PTR(pBanks[0].pMem);
    USC_BADARG(pBanks[0].nbytes<=0, USC_NotInitialized);
    USC_CHECK_PTR(pBanks[1].pMem);
    USC_BADARG(pBanks[1].nbytes<=0, USC_NotInitialized);
    USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);
    USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);

    USC_BADARG(options->pcmType.bitPerSample!=G729_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.sample_frequency!=G729_SAMPLE_FREQUENCY, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.nChannels!=G729_NCHANNELS, USC_UnsupportedPCMType);

    bitrate_idx = CheckRate_G729A(options->modes.bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

    *handle = (USC_Handle*)pBanks[0].pMem;
    g729_header = (G729_Handle_Header*)*handle;

    g729_header->vad = options->modes.vad;
    g729_header->bitrate = options->modes.bitrate;
    g729_header->direction = options->direction;

    if (options->direction == USC_ENCODE) /* encode only */
    {
        G729Encoder_Obj *EncObj = (G729Encoder_Obj *)((Ipp8s*)*handle + sizeof(G729_Handle_Header));
        apiG729Encoder_InitBuff(EncObj, pBanks[1].pMem);
        apiG729Encoder_Init((G729Encoder_Obj*)EncObj,
            (G729Codec_Type)bitrate_idx,(G729Encode_Mode)g729_header->vad);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        G729Decoder_Obj *DecObj = (G729Decoder_Obj *)((Ipp8s*)*handle + sizeof(G729_Handle_Header));
        apiG729Decoder_InitBuff(DecObj, pBanks[1].pMem);
        apiG729Decoder_Init((G729Decoder_Obj*)DecObj, (G729Codec_Type)bitrate_idx);
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}


static USC_Status Reinit_G729I(const USC_Modes *modes, USC_Handle handle )
{
    G729_Handle_Header *g729_header;
    Ipp32s bitrate_idx;

    USC_CHECK_PTR(modes);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
    USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

    g729_header = (G729_Handle_Header*)handle;

    bitrate_idx = CheckRate_G729I(modes->bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

    g729_header->vad = modes->vad;
    g729_header->bitrate = modes->bitrate;

    if (g729_header->direction == USC_ENCODE) /* encode only */
    {
        G729Encoder_Obj *EncObj = (G729Encoder_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));
        apiG729Encoder_Init((G729Encoder_Obj*)EncObj, G729I_CODEC, (G729Encode_Mode)modes->vad);
    }
    else if (g729_header->direction == USC_DECODE) /* decode only */
    {
        G729Decoder_Obj *DecObj = (G729Decoder_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));
        apiG729Decoder_Init((G729Decoder_Obj*)DecObj, G729I_CODEC);
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Reinit_G729A(const USC_Modes *modes, USC_Handle handle )
{
    G729_Handle_Header *g729_header;
    Ipp32s bitrate_idx;

    USC_CHECK_PTR(modes);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
    USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

    g729_header = (G729_Handle_Header*)handle;

    bitrate_idx = CheckRate_G729A(modes->bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

     g729_header->vad = modes->vad;
     g729_header->bitrate = modes->bitrate;

    if (g729_header->direction == USC_ENCODE) /* encode only */
    {
        G729Encoder_Obj *EncObj = (G729Encoder_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));
        apiG729Encoder_Init((G729Encoder_Obj*)EncObj, (G729Codec_Type)bitrate_idx, (G729Encode_Mode)modes->vad);
    }
    else if (g729_header->direction == USC_DECODE) /* decode only */
    {
        G729Decoder_Obj *DecObj = (G729Decoder_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));
        apiG729Decoder_Init((G729Decoder_Obj*)DecObj, (G729Codec_Type)bitrate_idx);
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control_G729I(const USC_Modes *modes, USC_Handle handle )
{
   G729_Handle_Header *g729_header;
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   g729_header = (G729_Handle_Header*)handle;

   bitrate_idx = CheckRate_G729I(modes->bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

   g729_header->vad = modes->vad;
   g729_header->bitrate = modes->bitrate;

   if (g729_header->direction == USC_ENCODE) /* encode only */
   {
        G729Encoder_Obj *EncObj = (G729Encoder_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));
        apiG729Encoder_Mode((G729Encoder_Obj*)EncObj, (G729Encode_Mode)modes->vad);
   }
   return USC_NoError;
}

static USC_Status Control_G729A(const USC_Modes *modes, USC_Handle handle )
{
   G729_Handle_Header *g729_header;
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   g729_header = (G729_Handle_Header*)handle;

   bitrate_idx = CheckRate_G729A(modes->bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

   g729_header->vad = modes->vad;
   g729_header->bitrate = modes->bitrate;

   if (g729_header->direction == USC_ENCODE) /* encode only */
   {
        G729Encoder_Obj *EncObj = (G729Encoder_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));
        apiG729Encoder_Mode((G729Encoder_Obj*)EncObj, (G729Encode_Mode)modes->vad);
   }
   return USC_NoError;
}

static Ipp32s getBitstreamSize(Ipp32s frametype)
{

    switch (frametype) {
      case 0: return 0;
      case 1: return 2;
      case 2: return 8;
      case 3: return 10;
      case 4: return 15;
      default: return 0;
    }
}

static USC_Status Encode_G729I(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
    G729_Handle_Header *g729_header;
    G729Encoder_Obj *EncObj;
    Ipp32s bitrate_idx;

    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(in->pcmType.bitPerSample!=G729_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.sample_frequency!=G729_SAMPLE_FREQUENCY, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.nChannels!=G729_NCHANNELS, USC_UnsupportedPCMType);

    USC_BADARG(in->nbytes<G729_SPEECH_FRAME*sizeof(Ipp16s), USC_NoOperation);

    g729_header = (G729_Handle_Header*)handle;

    USC_BADARG(USC_ENCODE != g729_header->direction, USC_BadDataPointer);

    bitrate_idx = CheckRate_G729I(in->bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

    g729_header->bitrate = in->bitrate;
    EncObj = (G729Encoder_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));

    if(apiG729Encode(EncObj,(const Ipp16s*)in->pBuffer,(Ipp8u*)out->pBuffer,(G729Codec_Type)bitrate_idx,&out->frametype) != APIG729_StsNoErr){
       return USC_NoOperation;
    }
     out->nbytes = getBitstreamSize(out->frametype);
     out->bitrate = in->bitrate;

     in->nbytes = G729_SPEECH_FRAME*sizeof(Ipp16s);

    return USC_NoError;
}

static USC_Status Encode_G729A(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
    G729_Handle_Header *g729_header;
    G729Encoder_Obj *EncObj;
    Ipp32s bitrate_idx;

    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(in->pcmType.bitPerSample!=G729_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.sample_frequency!=G729_SAMPLE_FREQUENCY, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.nChannels!=G729_NCHANNELS, USC_UnsupportedPCMType);

    USC_BADARG(in->nbytes<G729_SPEECH_FRAME*sizeof(Ipp16s), USC_NoOperation);

    g729_header = (G729_Handle_Header*)handle;

    USC_BADARG(USC_ENCODE != g729_header->direction, USC_BadDataPointer);

    bitrate_idx = CheckRate_G729A(in->bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

    g729_header->bitrate = in->bitrate;
    EncObj = (G729Encoder_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));

    if(apiG729Encode(EncObj,(const Ipp16s*)in->pBuffer,(Ipp8u*)out->pBuffer,(G729Codec_Type)bitrate_idx,&out->frametype) != APIG729_StsNoErr){
       return USC_NoOperation;
    }
    out->nbytes = getBitstreamSize(out->frametype);
    out->bitrate = in->bitrate;

    in->nbytes = G729_SPEECH_FRAME*sizeof(Ipp16s);

    return USC_NoError;
}


static USC_Status Decode_G729I(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
    G729_Handle_Header *g729_header;
    G729Decoder_Obj *DecObj;
    Ipp32s bitrate_idx;
    Ipp32s PLC = 0;

    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    g729_header = (G729_Handle_Header*)handle;

    USC_BADARG(USC_DECODE != g729_header->direction, USC_NoOperation);

    DecObj = (G729Decoder_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));

    if(in == NULL) {
      PLC = 1;
    } else if(in->pBuffer == NULL) {
      PLC = 1;
    }

    if (PLC) {
       /* Lost frame */
       if(apiG729Decode(DecObj,(const Ipp8u*)LostFrame,(-1),(Ipp16s*)out->pBuffer) != APIG729_StsNoErr){
         return USC_NoOperation;
      }
      out->bitrate = g729_header->bitrate;
    } else {
      bitrate_idx = CheckRate_G729I(in->bitrate);
      USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

      g729_header->bitrate = in->bitrate;
      if(apiG729Decode(DecObj,(const Ipp8u*)in->pBuffer,in->frametype,(Ipp16s*)out->pBuffer) != APIG729_StsNoErr){
         return USC_NoOperation;
      }
      in->nbytes = getBitstreamSize(in->frametype);
      in->bitrate = g729_header->bitrate;
    }
    out->nbytes = G729_SPEECH_FRAME*sizeof(Ipp16s);
    out->pcmType.sample_frequency = G729_SAMPLE_FREQUENCY;
    out->pcmType.bitPerSample = G729_BITS_PER_SAMPLE;
    out->pcmType.nChannels = G729_NCHANNELS;

    return USC_NoError;
}

static USC_Status Decode_G729A(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
    G729_Handle_Header *g729_header;
    G729Decoder_Obj *DecObj;
    Ipp32s bitrate_idx;
    Ipp32s PLC = 0;

    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    g729_header = (G729_Handle_Header*)handle;

    USC_BADARG(USC_DECODE != g729_header->direction, USC_NoOperation);

    DecObj = (G729Decoder_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));

    if(in == NULL) {
      PLC = 1;
    } else if(in->pBuffer == NULL) {
      PLC = 1;
    }

    if (PLC) {
      /* Lost frame */
      if(apiG729Decode(DecObj,(const Ipp8u*)LostFrame,(-1),(Ipp16s*)out->pBuffer) != APIG729_StsNoErr){
        return USC_NoOperation;
      }
      out->bitrate = g729_header->bitrate;
    } else {
      bitrate_idx = CheckRate_G729A(in->bitrate);
      USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
      g729_header->bitrate = in->bitrate;
      if(apiG729Decode(DecObj,(const Ipp8u*)in->pBuffer,in->frametype,(Ipp16s*)out->pBuffer) != APIG729_StsNoErr){
        return USC_NoOperation;
      }
      in->nbytes = getBitstreamSize(in->frametype);
      out->bitrate = in->bitrate;
    }
    out->nbytes = G729_SPEECH_FRAME*sizeof(Ipp16s);
    out->pcmType.sample_frequency = G729_SAMPLE_FREQUENCY;
    out->pcmType.bitPerSample = G729_BITS_PER_SAMPLE;
    out->pcmType.nChannels = G729_NCHANNELS;

    return USC_NoError;
}

static __ALIGN32 CONST Ipp32s pFrameSize_G729I[G729I_NUM_RATES]={
    15,
    10,
   8
};
static Ipp32s RateToIndex_G729I(Ipp32s rate_bps)
{
    Ipp32s idx;

    switch(rate_bps) {
     case 11800:  idx = 0; break;
      case 8000:  idx = 1; break;
      case 6400: idx = 2; break;
      default: idx = -1; break;
    }
    return idx;
}

static Ipp32s RateToIndex_G729A(Ipp32s rate_bps)
{
    Ipp32s idx;

    switch(rate_bps) {
     case 8000:  idx = 1; break;
      default: idx = -1; break;
    }
    return idx;
}

static USC_Status CalsOutStreamSize(const USC_Option *options, Ipp32s bitrate_idx, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s nBlocks, nSamples;

   if(options->direction==USC_ENCODE) { /*Encode: src - PCMStream, dst - bitstream*/
      USC_BADARG(options->modes.vad>1, USC_UnsupportedVADType);

      nSamples = nbytesSrc / (G729_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / G729_SPEECH_FRAME;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % G729_SPEECH_FRAME) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }

      *nbytesDst = nBlocks * pFrameSize_G729I[bitrate_idx];
   } else if(options->direction==USC_DECODE) {/*Decode: src - bitstream, dst - PCMStream*/
      if(options->modes.vad==0) { /*VAD off*/
         nBlocks = nbytesSrc / pFrameSize_G729I[bitrate_idx];
      } else if(options->modes.vad==1) { /*VAD on*/
         nBlocks = nbytesSrc / G729_SID_FRAMESIZE;
         nBlocks += nBlocks*G729_NUM_UNTR_FRAMES; /*Len of Untr frame  in bitstream==0, there are 2 untr frames after SID frame*/
      } else return USC_UnsupportedVADType;

      if (0 == nBlocks) return USC_NoOperation;

      nSamples = nBlocks * G729_SPEECH_FRAME;
      *nbytesDst = nSamples * (G729_BITS_PER_SAMPLE >> 3);
   } else if(options->direction==USC_DUPLEX) {/* Both: src - PCMStream, dst - PCMStream*/
      nSamples = nbytesSrc / (G729_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / G729_SPEECH_FRAME;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % G729_SPEECH_FRAME) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * G729_SPEECH_FRAME * (G729_BITS_PER_SAMPLE >> 3);
   } else return USC_NoOperation;

   return USC_NoError;
}
static USC_Status GetOutStreamSizeI(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   bitrate_idx = RateToIndex_G729I(bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

   return CalsOutStreamSize(options, bitrate_idx, nbytesSrc, nbytesDst);
}

static USC_Status GetOutStreamSizeA(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   bitrate_idx = RateToIndex_G729A(bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

   return CalsOutStreamSize(options, bitrate_idx, nbytesSrc, nbytesDst);
}

static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize)
{
   USC_CHECK_PTR(options);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(frameSize <= 0, USC_BadArgument);

   return USC_NoOperation;
}
