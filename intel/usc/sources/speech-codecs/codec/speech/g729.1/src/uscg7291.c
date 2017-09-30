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
// to the file ipplic.htm located in the root directory of your Intel(R) IPP
// product installation for more information.
//
// A speech coding standards promoted by ITU, ETSI, 3GPP and other
// organizations. Implementations of these standards, or the standard enabled
// platforms may require licenses from various entities, including
// Intel Corporation.
//
//
// Purpose: G.729/A/B/D/E/EV speech codec: USC functions.
//
*/
#include "usc.h"
#include "owng7291.h"
#include "g7291api.h"


#define G7291_NUM_RATES        12
#define G7291_NUM_PCM_TYPES     2
#define G7291_SPEECH_FRAME    320
#define G7291_8k_SPEECH_FRAME 160
#define G7291_BITSTREAM_SIZE   80
#define G7291_BITS_PER_SAMPLE  16
#define G7291_NCHANNELS         1

static USC_Status GetInfoSize(int *pSize);
static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo);
static USC_Status NumAlloc(const USC_Option *options, int *nbanks);
static USC_Status MemAlloc(const USC_Option *options, USC_MemBank *pBanks);
static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle );
static USC_Status Control(const USC_Modes *modes, USC_Handle handle );
static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out);
static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out);
static USC_Status GetOutStreamSize(const USC_Option *options, int bitrate, int nbytesSrc, int *nbytesDst);
static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, int frameSize);

typedef struct {
    USC_Direction direction;
    int bitrate;
    int frequency;
    USC_OutputMode outMode;
} G7291_Handle_Header;

/* global usc vector table */
USCFUN USC_Fxns USC_G7291_Fxns=
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

static __ALIGN32 CONST USC_Rates pTblRates[G7291_NUM_RATES]={
    {32000}, {30000}, {28000}, {26000}, {24000}, {22000}, {20000},
    {18000}, {16000}, {14000}, {12000}, {8000}
};

static __ALIGN32 CONST USC_PCMType pcmTypesTbl[G7291_NUM_PCM_TYPES]={
    {16000,G7291_BITS_PER_SAMPLE,G7291_NCHANNELS},
    {8000,G7291_BITS_PER_SAMPLE,G7291_NCHANNELS}
};
static int CheckRate(int rate_bps)
{
    int rate;

    switch(rate_bps) {
      case 8000:  rate = 0;  break;
      case 12000: rate = 1;  break;
      case 14000: rate = 2;  break;
      case 16000: rate = 3;  break;
      case 18000: rate = 4;  break;
      case 20000: rate = 5;  break;
      case 22000: rate = 6;  break;
      case 24000: rate = 7;  break;
      case 26000: rate = 8;  break;
      case 28000: rate = 9;  break;
      case 30000: rate = 10; break;
      case 32000: rate = 11; break;
      default:    rate = -1; break;
    }
    return rate;
}

static USC_Status GetInfoSize(int *pSize)
{
    USC_CHECK_PTR(pSize);
    *pSize = sizeof(USC_CodecInfo);
    return USC_NoError;
}

static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo)
{
    G7291_Handle_Header *g729_header;

    USC_CHECK_PTR(pInfo);

    pInfo->name = "IPP_G7291";
    if (handle == NULL) {
       pInfo->params.framesize = G7291_SPEECH_FRAME*sizeof(short);
       pInfo->params.modes.bitrate = 32000;
       pInfo->params.direction = USC_DECODE;
       pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
       pInfo->params.pcmType.sample_frequency = pcmTypesTbl[0].sample_frequency;
       pInfo->params.pcmType.bitPerSample = pcmTypesTbl[0].bitPerSample;
       pInfo->params.pcmType.nChannels = pcmTypesTbl[0].nChannels;
    } else {
       g729_header = (G7291_Handle_Header*)handle;
       pInfo->params.modes.bitrate = g729_header->bitrate;
       pInfo->params.direction = g729_header->direction;
       pInfo->params.modes.outMode = g729_header->outMode;
       pInfo->params.pcmType.sample_frequency = g729_header->frequency;
       pInfo->params.pcmType.bitPerSample = 16;
       pInfo->params.pcmType.nChannels = 1;
       if(g729_header->frequency==16000) {
           pInfo->params.framesize = G7291_SPEECH_FRAME*sizeof(short);
       } else {
           pInfo->params.framesize = G7291_8k_SPEECH_FRAME*sizeof(short);
       }
    }
    pInfo->params.modes.truncate = 0;
    pInfo->params.modes.vad = 0;
    pInfo->maxbitsize = G7291_BITSTREAM_SIZE;
    pInfo->nPcmTypes = G7291_NUM_PCM_TYPES;
    pInfo->pPcmTypesTbl = pcmTypesTbl;
    pInfo->params.modes.hpf = 0;
    pInfo->params.modes.pf = 0;
    pInfo->params.law = 0;
    pInfo->nRates = G7291_NUM_RATES;
    pInfo->pRateTbl = (const USC_Rates *)&pTblRates;
    pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);

    return USC_NoError;
}

static USC_Status NumAlloc(const USC_Option *options, int *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);

   *nbanks = 1;
   return USC_NoError;
}

static USC_Status MemAlloc(const USC_Option *options, USC_MemBank *pBanks)
{
    unsigned int nbytes = 0;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);

    pBanks->pMem = NULL;
    pBanks->align = 32;
    pBanks->memType = USC_OBJECT;
    pBanks->memSpaceType = USC_NORMAL;

    USC_BADARG(CheckRate(options->modes.bitrate) < 0, USC_UnsupportedBitRate);

    if (options->direction == USC_ENCODE) /* encode only */
    {
        apiG7291Encoder_Alloc(G7291_CODEC, (int*)&nbytes);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        apiG7291Decoder_Alloc(G7291_CODEC, (int*)&nbytes);
    } else {
        return USC_NoOperation;
    }
    pBanks->nbytes = nbytes + sizeof(G7291_Handle_Header); /* include header in handle */
    return USC_NoError;
}


static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
    G7291_Handle_Header *g729_header;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);
    USC_CHECK_HANDLE(handle);
    USC_CHECK_PTR(pBanks[0].pMem);
    USC_BADARG(pBanks[0].nbytes<=0, USC_NotInitialized);
    USC_BADARG(options->modes.vad > 0, USC_UnsupportedVADType);
    USC_BADARG((options->pcmType.sample_frequency !=16000) &&
               (options->pcmType.sample_frequency != 8000), USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.bitPerSample != G7291_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.nChannels!=G7291_NCHANNELS, USC_UnsupportedPCMType);
    USC_BADARG(CheckRate(options->modes.bitrate) < 0, USC_UnsupportedBitRate);

    *handle = (USC_Handle*)pBanks->pMem;
    g729_header = (G7291_Handle_Header*)*handle;

    g729_header->bitrate = options->modes.bitrate;
    g729_header->direction = options->direction;
    g729_header->outMode = options->modes.outMode;
    g729_header->frequency = options->pcmType.sample_frequency;

    if (options->direction == USC_ENCODE) /* encode only */
    {
        G7291Encoder_Obj *EncObj = (G7291Encoder_Obj *)((char*)*handle + sizeof(G7291_Handle_Header));
        apiG7291Encoder_Init((G7291Encoder_Obj*)EncObj, g729_header->bitrate, g729_header->frequency);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        int g729b = 0, lowDelay = 0;
        G7291Decoder_Obj *DecObj = (G7291Decoder_Obj *)((char*)*handle + sizeof(G7291_Handle_Header));
        if (g729_header->outMode == USC_OUT_COMPATIBLE) {
            g729b = 1;
            lowDelay = 1;
        }else if (g729_header->outMode == USC_OUT_DELAY) {
            lowDelay = 1;
        }
        apiG7291Decoder_Init((G7291Decoder_Obj*)DecObj, g729_header->frequency, g729b, lowDelay);
    } else {
        *handle = NULL;
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle )
{
    G7291_Handle_Header *g729_header;

    USC_CHECK_PTR(modes);
    USC_CHECK_HANDLE(handle);

    g729_header = (G7291_Handle_Header*)handle;

    USC_BADARG(CheckRate(modes->bitrate) < 0, USC_UnsupportedBitRate);

    g729_header->bitrate = modes->bitrate;

    if (g729_header->direction == USC_ENCODE) /* encode only */
    {
        G7291Encoder_Obj *EncObj = (G7291Encoder_Obj *)((char*)handle + sizeof(G7291_Handle_Header));
        apiG7291Encoder_Init((G7291Encoder_Obj*)EncObj, g729_header->bitrate, g729_header->frequency);
    }
    else if (g729_header->direction == USC_DECODE) /* decode only */
    {
        int g729b = 0, lowDelay = 0;
        G7291Decoder_Obj *DecObj = (G7291Decoder_Obj *)((char*)handle + sizeof(G7291_Handle_Header));
        if (g729_header->outMode == USC_OUT_COMPATIBLE) {
            g729b = 1;
            lowDelay = 1;
        }else if (g729_header->outMode == USC_OUT_DELAY) {
            lowDelay = 1;
        }
        apiG7291Decoder_Init((G7291Decoder_Obj*)DecObj, g729_header->frequency, g729b, lowDelay);
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(const USC_Modes *modes, USC_Handle handle )
{
   G7291_Handle_Header *g729_header;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   g729_header = (G7291_Handle_Header*)handle;

   USC_BADARG(CheckRate(modes->bitrate) < 0, USC_UnsupportedBitRate);

   g729_header->bitrate = modes->bitrate;

   return USC_NoError;
}

static int getBitstreamSize(int rate)
{
    switch (rate) {
      case  8000: return 20;
      case 12000: return 30;
      case 14000: return 35;
      case 16000: return 40;
      case 18000: return 45;
      case 20000: return 50;
      case 22000: return 55;
      case 24000: return 60;
      case 26000: return 65;
      case 28000: return 70;
      case 30000: return 75;
      case 32000: return 80;
      default: return 0;
    }
}

static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
    G7291_Handle_Header *g729_header;
    G7291Encoder_Obj *EncObj;

    USC_CHECK_HANDLE(handle);
    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_BADARG(in->pcmType.bitPerSample!=G7291_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.sample_frequency!=8000 &&
        in->pcmType.sample_frequency!=16000, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.nChannels!=G7291_NCHANNELS, USC_UnsupportedPCMType);

    g729_header = (G7291_Handle_Header*)handle;
    USC_BADARG(USC_ENCODE != g729_header->direction, USC_NoOperation);
    if(g729_header->frequency==16000) {
        USC_BADARG(in->nbytes<G7291_SPEECH_FRAME*sizeof(short), USC_NoOperation);
    } else {
        USC_BADARG(in->nbytes<G7291_8k_SPEECH_FRAME*sizeof(short), USC_NoOperation);
    }

    USC_BADARG(CheckRate(in->bitrate) < 0, USC_UnsupportedBitRate);

    g729_header->bitrate = in->bitrate;
    EncObj = (G7291Encoder_Obj *)((char*)handle + sizeof(G7291_Handle_Header));

    if (apiG7291Encode(EncObj,(const short*)in->pBuffer,(unsigned char*)out->pBuffer,
        g729_header->bitrate) != APIG729_StsNoErr)
    {
       return USC_NoOperation;
    }
    out->bitrate = in->bitrate;
    out->nbytes = getBitstreamSize(out->bitrate);

    if(g729_header->frequency==16000) {
      in->nbytes = G7291_SPEECH_FRAME*sizeof(short);
    } else {
      in->nbytes = G7291_8k_SPEECH_FRAME*sizeof(short);
    }
    if(g729_header->outMode == USC_OUT_COMPATIBLE) out->frametype = 5;
    else out->frametype = 0;

    return USC_NoError;
}

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
    G7291_Handle_Header *g729_header;
    G7291Decoder_Obj *DecObj;
    Ipp32s PLC = 0;

    USC_CHECK_HANDLE(handle);
    USC_CHECK_PTR(out);

    g729_header = (G7291_Handle_Header*)handle;

    USC_BADARG(USC_DECODE != g729_header->direction, USC_NoOperation);

    DecObj = (G7291Decoder_Obj *)((char*)handle + sizeof(G7291_Handle_Header));

    if(in == NULL) {
      PLC = 1;
    } else if(in->pBuffer == NULL) {
      PLC = 1;
    }

    if (PLC) {
       /* Lost frame */
       USC_BADARG(CheckRate(g729_header->bitrate) < 0, USC_UnsupportedBitRate);
       if (apiG7291Decode(DecObj,(const short*)LostFrame,(short*)out->pBuffer, g729_header->bitrate, 1) != APIG729_StsNoErr)
       {
         return USC_NoOperation;
       }
    } else {
       USC_BADARG(CheckRate(in->bitrate) < 0, USC_UnsupportedBitRate);
       USC_BADARG(in->nbytes < 0, USC_BadDataPointer);

       g729_header->bitrate = in->bitrate;
       if (apiG7291Decode(DecObj, (const short*)in->pBuffer, (short*)out->pBuffer, in->bitrate, in->frametype) != APIG729_StsNoErr)
       {
         return USC_NoOperation;
       }
    }
    out->bitrate = g729_header->bitrate;
    if (in != NULL) in->nbytes = getBitstreamSize(out->bitrate);
    out->pcmType.sample_frequency = g729_header->frequency;
    out->pcmType.bitPerSample = G7291_BITS_PER_SAMPLE;
    out->pcmType.nChannels = G7291_NCHANNELS;
    if(g729_header->frequency==16000) {
      out->nbytes = G7291_SPEECH_FRAME*sizeof(short);
    } else {
      out->nbytes = G7291_8k_SPEECH_FRAME*sizeof(short);
    }
    return USC_NoError;
}

static USC_Status CalsOutStreamSize(const USC_Option *options, int bitrate, int nbytesSrc, int *nbytesDst)
{
   int nBlocks, nSamples, frameSize;

   if (options->pcmType.sample_frequency == 16000)
      frameSize = G7291_SPEECH_FRAME;
   else
      frameSize = G7291_8k_SPEECH_FRAME;

   if(options->direction==USC_ENCODE)
   { /*Encode: src - PCMStream, dst - bitstream*/
      nSamples = nbytesSrc / (G7291_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / frameSize;
      if (0 == nBlocks) return USC_NoOperation;
      if (0 != nSamples % frameSize) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * getBitstreamSize(bitrate);
   } else if(options->direction==USC_DECODE)
   {/*Decode: src - bitstream, dst - PCMStream*/
      nBlocks = nbytesSrc / getBitstreamSize(bitrate);
      if (0 == nBlocks) return USC_NoOperation;
      nSamples = nBlocks * frameSize;
      *nbytesDst = nSamples * (G7291_BITS_PER_SAMPLE >> 3);
   } else if(options->direction==USC_DUPLEX)
   {/* Both: src - PCMStream, dst - PCMStream*/
      nSamples = nbytesSrc / (G7291_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / frameSize;
      if (0 == nBlocks) return USC_NoOperation;
      if (0 != nSamples % frameSize) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * frameSize * (G7291_BITS_PER_SAMPLE >> 3);
   } else return USC_NoOperation;

   return USC_NoError;
}

static USC_Status GetOutStreamSize(const USC_Option *options, int bitrate, int nbytesSrc, int *nbytesDst)
{
   USC_CHECK_PTR(options);
   USC_BADARG(CheckRate(bitrate) < 0, USC_UnsupportedBitRate);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(CheckRate(options->modes.bitrate) < 0, USC_UnsupportedBitRate);

   return CalsOutStreamSize(options, bitrate, nbytesSrc, nbytesDst);
}

static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, int frameSize)
{
   USC_CHECK_PTR(options);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(frameSize <= 0, USC_BadArgument);

   return USC_NoOperation;
}
