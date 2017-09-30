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
// Purpose: G.722 speech codec: USC functions.
//
*/

#include "usc.h"
#include "owng722sb.h"
#include "g722sb.h"
#include "g722sbapi.h"


#define  G722SB_NUM_RATES           3
#define  G722SB_BITS_PER_SAMPLE    16
#define  G722SB_SAMPLE_FREQ     16000
#define  G722SB_NCHANNELS           1


static USC_Status GetInfoSize(Ipp32s *pSize);
static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo);
static USC_Status NumAlloc(const USC_Option *options, Ipp32s *nbanks);
static USC_Status MemAlloc(const USC_Option *options, USC_MemBank *pBanks);
static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle );
static USC_Status Control(const USC_Modes *modes, USC_Handle handle );
static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out);
static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out);
static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst);
static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize);

#define BITSTREAM_SIZE_10MS        80
#define G722SB_SPEECH_FRAME_10MS   160
#define BITSTREAM_SIZE_20MS        160
#define G722SB_SPEECH_FRAME_20MS   320

typedef struct {
   USC_Direction direction;
   Ipp32s bitrate;
   Ipp32s hpf;
   Ipp32s pf;
   Ipp32s framesize;
   Ipp32s reserve1;
   Ipp32s reserve2;
   Ipp32s reserve3;
} G722SB_Handle_Header;

/* global usc vector table */
USCFUN USC_Fxns USC_G722SB_Fxns=
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

static __ALIGN32 CONST USC_Rates pTblRates_G722SB[G722SB_NUM_RATES]={
    {64000},
    {56000},
   {48000}
};
static __ALIGN32 CONST USC_PCMType pcmTypesTbl_G722SB[1]={
   {G722SB_SAMPLE_FREQ,G722SB_BITS_PER_SAMPLE, G722SB_NCHANNELS}
};

static Ipp32s CheckRate_G722SB(Ipp32s rate_bps)
{
   Ipp32s rate;

    switch(rate_bps) {
      case 64000:  rate = 1; break;
      case 56000:  rate = 2; break;
     case 48000:  rate = 3; break;
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

static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo)
{
   G722SB_Handle_Header *g722sb_header;

   USC_CHECK_PTR(pInfo);

   pInfo->name = "IPP_G722";
   if (handle == NULL) {
      //pInfo->params.framesize = G722SB_SPEECH_FRAME_10MS*sizeof(Ipp16s);
      //pInfo->maxbitsize = BITSTREAM_SIZE_10MS;
      pInfo->params.framesize = G722SB_SPEECH_FRAME_20MS*sizeof(Ipp16s);
      pInfo->maxbitsize = BITSTREAM_SIZE_20MS;
      pInfo->params.modes.bitrate = 64000;
      pInfo->params.direction = USC_DECODE;
      pInfo->params.modes.hpf = 1;
      pInfo->params.modes.pf = 1;
   } else {
      g722sb_header = (G722SB_Handle_Header*)handle;
      pInfo->params.framesize = g722sb_header->framesize;
      pInfo->maxbitsize = (g722sb_header->framesize/sizeof(Ipp16s))>>1;
      pInfo->params.modes.bitrate = g722sb_header->bitrate;
      pInfo->params.direction = g722sb_header->direction;
      pInfo->params.modes.hpf = g722sb_header->hpf;
      pInfo->params.modes.pf = g722sb_header->pf;
   }

   pInfo->params.modes.vad = 0;
   pInfo->params.modes.truncate = 0;
   pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
   pInfo->params.pcmType.sample_frequency = pcmTypesTbl_G722SB[0].sample_frequency;
   pInfo->params.pcmType.nChannels = pcmTypesTbl_G722SB[0].nChannels;
   pInfo->params.pcmType.bitPerSample = pcmTypesTbl_G722SB[0].bitPerSample;
   pInfo->nPcmTypes = 1;
   pInfo->pPcmTypesTbl = pcmTypesTbl_G722SB;
   pInfo->params.law = 0;
   pInfo->nRates = G722SB_NUM_RATES;
   pInfo->pRateTbl = (const USC_Rates *)&pTblRates_G722SB;
   pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);

   return USC_NoError;
}

static USC_Status NumAlloc(const USC_Option *options, Ipp32s *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);

   *nbanks = 1;
   return USC_NoError;
}

static USC_Status MemAlloc(const USC_Option *options, USC_MemBank *pBanks)
{
    Ipp32s nbytes;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);

    pBanks->pMem = NULL;
    pBanks->align = 32;
    pBanks->memType = USC_OBJECT;
    pBanks->memSpaceType = USC_NORMAL;

   if (options->direction == USC_ENCODE) /* encode only */
    {
        apiG722SBEncoder_Alloc(&nbytes);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        apiG722SBDecoder_Alloc(&nbytes);
    } else {
        return USC_NoOperation;
    }
    pBanks->nbytes = nbytes+sizeof(G722SB_Handle_Header); /* include direction in handle */
    return USC_NoError;
}

static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
    Ipp32s bitrate_idx;
    G722SB_Handle_Header *g722sb_header;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);
    USC_CHECK_PTR(pBanks->pMem);
    USC_BADARG(pBanks->nbytes<=0, USC_NotInitialized);
    USC_CHECK_HANDLE(handle);
    USC_BADARG(options->pcmType.bitPerSample!=G722SB_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.sample_frequency!=G722SB_SAMPLE_FREQ, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.nChannels!=G722SB_NCHANNELS, USC_UnsupportedPCMType);

    if((options->framesize != G722SB_SPEECH_FRAME_10MS*sizeof(Ipp16s)) &&
       (options->framesize != G722SB_SPEECH_FRAME_20MS*sizeof(Ipp16s))) return USC_UnsupportedFrameSize;

    bitrate_idx = CheckRate_G722SB(options->modes.bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

    *handle = (USC_Handle*)pBanks->pMem;
    g722sb_header = (G722SB_Handle_Header*)*handle;

    g722sb_header->hpf = options->modes.hpf;
    g722sb_header->pf = options->modes.pf;
    g722sb_header->bitrate = options->modes.bitrate;
    g722sb_header->direction = options->direction;
    g722sb_header->framesize = options->framesize;

    if (options->direction == USC_ENCODE) /* encode only */
    {
      G722SB_Encoder_Obj *EncObj = (G722SB_Encoder_Obj *)((Ipp8s*)*handle + sizeof(G722SB_Handle_Header));
      apiG722SBEncoder_Init(EncObj, g722sb_header->hpf);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
      G722SB_Decoder_Obj *DecObj = (G722SB_Decoder_Obj *)((Ipp8s*)*handle + sizeof(G722SB_Handle_Header));
      apiG722SBDecoder_Init(DecObj, g722sb_header->pf, g722sb_header->framesize);

    } else {
      *handle = NULL;
      return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle )
{
    Ipp32s bitrate_idx;
    G722SB_Handle_Header *g722sb_header;

    USC_CHECK_PTR(modes);
    USC_CHECK_HANDLE(handle);

    g722sb_header = (G722SB_Handle_Header*)handle;

    bitrate_idx = CheckRate_G722SB(modes->bitrate);
    if(bitrate_idx < 0) return USC_UnsupportedBitRate;
    g722sb_header->hpf = modes->hpf;
    g722sb_header->pf = modes->pf;
    g722sb_header->bitrate = modes->bitrate;

    if (g722sb_header->direction == USC_ENCODE) /* encode only */
    {
      G722SB_Encoder_Obj *EncObj = (G722SB_Encoder_Obj *)((Ipp8s*)handle + sizeof(G722SB_Handle_Header));
      apiG722SBEncoder_Init(EncObj, g722sb_header->hpf);
    }
    else if (g722sb_header->direction == USC_DECODE) /* decode only */
    {
      G722SB_Decoder_Obj *DecObj = (G722SB_Decoder_Obj *)((Ipp8s*)handle + sizeof(G722SB_Handle_Header));
      apiG722SBDecoder_Init(DecObj, g722sb_header->pf, g722sb_header->framesize);
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(const USC_Modes *modes, USC_Handle handle )
{
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   bitrate_idx = CheckRate_G722SB(modes->bitrate);
   if(bitrate_idx < 0) return USC_UnsupportedBitRate;

   return USC_NoError;
}

static Ipp32s getBitstreamSize(const USC_PCMStream *in, Ipp32s hpf)
{
   Ipp32s size, nSamples;

   nSamples = in->nbytes/sizeof(Ipp16s);
   if(hpf) {
     size = nSamples/2;
   } else {
     size = nSamples;
   }

   return size;

}

static Ipp32s getPCMStreamSize(const USC_Bitstream *in, Ipp32s pf)
{
   Ipp32s size;

   if(pf) {
     size = in->nbytes*sizeof(Ipp16s)*2;
   } else {
     size = in->nbytes*sizeof(Ipp16s);
   }

   return size;

}

static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
    Ipp32s bitrate_idx, lenSamples, i;
    Ipp8s pOutBuff[2];
    Ipp16s *in_buff, *out_buff;
    G722SB_Handle_Header *g722sb_header;
    G722SB_Encoder_Obj *EncObj;

    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(in->pcmType.bitPerSample!=G722SB_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.sample_frequency!=G722SB_SAMPLE_FREQ, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.nChannels!=G722SB_NCHANNELS, USC_UnsupportedPCMType);

    USC_BADARG( NULL==in->pBuffer, USC_NoOperation );
    USC_BADARG( NULL==out->pBuffer, USC_NoOperation );

    g722sb_header = (G722SB_Handle_Header*)handle;

    if(g722sb_header->direction != USC_ENCODE) return USC_NoOperation;

    bitrate_idx = CheckRate_G722SB(in->bitrate);
    USC_BADARG( bitrate_idx < 0, USC_UnsupportedBitRate );
    USC_BADARG( in->nbytes <= 0, USC_NoOperation );

    g722sb_header->bitrate = in->bitrate;
    EncObj = (G722SB_Encoder_Obj *)((Ipp8s*)handle + sizeof(G722SB_Handle_Header));

    if(g722sb_header->hpf) {
      if(in->nbytes < g722sb_header->framesize) lenSamples = in->nbytes/sizeof(Ipp16s);
      else lenSamples = g722sb_header->framesize/sizeof(Ipp16s);

      if(apiG722SBEncode(EncObj, lenSamples, (Ipp16s*)in->pBuffer, out->pBuffer) != API_G722SB_StsNoErr){
        return USC_NoOperation;
      }
      in->nbytes = lenSamples*sizeof(Ipp16s);
      out->nbytes = getBitstreamSize(in, g722sb_header->hpf);
    } else {

      lenSamples = in->nbytes/sizeof(Ipp16s);

      in_buff = (Ipp16s*)in->pBuffer;
      out_buff = (Ipp16s*)out->pBuffer;
      for(i=0; i<lenSamples; i++) {
        if((in_buff[i] & 1) == 1) {
          apiG722SBEncoder_Init(EncObj, g722sb_header->hpf);
          out_buff[i] = 1;
        } else {
          if(apiG722SBEncode(EncObj, 1, (Ipp16s*)&in_buff[i], (Ipp8s *)&pOutBuff[0]) != API_G722SB_StsNoErr){
            return USC_NoOperation;
          }
          out_buff[i] = (Ipp16s)((Ipp16s)(pOutBuff[0] << 8) & 0xFF00);
        }
      }
      out->nbytes = in->nbytes;
    }

    out->frametype = 0;
    out->bitrate = in->bitrate;

    return USC_NoError;
}

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
    G722SB_Handle_Header *g722sb_header;
    G722SB_Decoder_Obj *DecObj;
    Ipp32s bitrate_idx, length, i, j;
    Ipp8s bit[2];
    Ipp16s *in_buff, *out_buff;
    Ipp32s PLC = 0;

    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);
    USC_BADARG( NULL==out->pBuffer, USC_NoOperation );

    g722sb_header = (G722SB_Handle_Header*)handle;

    if(g722sb_header->direction != USC_DECODE) return USC_NoOperation;

    DecObj = (G722SB_Decoder_Obj *)((Ipp8s*)handle + sizeof(G722SB_Handle_Header));

    if(in == NULL) {
      PLC = 1;
    } else if(in->pBuffer == NULL) {
      PLC = 1;
    }

    if(PLC) {
       /* Lost frame */
       bitrate_idx = CheckRate_G722SB(g722sb_header->bitrate);
       if(bitrate_idx < 0) return USC_UnsupportedBitRate;

      if(g722sb_header->pf) {
        if(apiG722SBDecode(DecObj, 0, (Ipp16s)bitrate_idx, NULL, (Ipp16s*)out->pBuffer) != API_G722SB_StsNoErr){
          return USC_NoOperation;
        }
        out->nbytes = g722sb_header->framesize;
      } else {
        if(g722sb_header->framesize == (G722SB_SPEECH_FRAME_10MS*sizeof(short))) length = BITSTREAM_SIZE_10MS;
        else length = BITSTREAM_SIZE_20MS;
        in_buff = (Ipp16s*)LostFrame;
        out_buff = (Ipp16s*)out->pBuffer;
        for(i=0,j=0; i<length; i++) {
          bit[0] = (Ipp8s)((in_buff[i] >> 8) & 0x00FF);
          if(apiG722SBDecode(DecObj, 1, (Ipp16s)bitrate_idx, (Ipp8s *)&bit[0], (Ipp16s*)&out_buff[j]) != API_G722SB_StsNoErr){
            return USC_NoOperation;
          }
          j+=2;
        }
        out->nbytes = j*sizeof(Ipp16s);
      }

    } else {
      bitrate_idx = CheckRate_G722SB(in->bitrate);
       if(bitrate_idx < 0) return USC_UnsupportedBitRate;
      if(in->nbytes <= 0) return USC_NoOperation;
       g722sb_header->bitrate = in->bitrate;

      if(g722sb_header->pf) {
        length = (g722sb_header->framesize/(G722SB_BITS_PER_SAMPLE >> 3))>>1;
        if(in->nbytes < length) return USC_NoOperation;

        if(apiG722SBDecode(DecObj, length, (Ipp16s)bitrate_idx, (Ipp8s *)in->pBuffer, (Ipp16s*)out->pBuffer) != API_G722SB_StsNoErr){
          return USC_NoOperation;
        }
        in->nbytes = length;
        out->nbytes = getPCMStreamSize(in, g722sb_header->pf);
      } else {
        if(in->nbytes < (g722sb_header->framesize/sizeof(short))) length = in->nbytes/sizeof(Ipp16s);
        else length = (g722sb_header->framesize/sizeof(short)) >> 1;

        in_buff = (Ipp16s*)in->pBuffer;
        out_buff = (Ipp16s*)out->pBuffer;
        for(i=0,j=0; i<length; i++) {
          if((in_buff[i] & 1) == 1) {
            apiG722SBDecoder_Init(DecObj, g722sb_header->pf, g722sb_header->framesize);
            out_buff[j++] = 1;
            out_buff[j++] = 1;
          } else {
            bit[0] = (Ipp8s)((in_buff[i] >> 8) & 0x00FF);
            if(apiG722SBDecode(DecObj, 1, (Ipp16s)bitrate_idx, (Ipp8s *)&bit[0], (Ipp16s*)&out_buff[j]) != API_G722SB_StsNoErr){
              return USC_NoOperation;
            }
            j+=2;
          }
        }
        out->nbytes = j*sizeof(Ipp16s);
      }
    }
    out->pcmType.bitPerSample = G722SB_BITS_PER_SAMPLE;
    out->pcmType.sample_frequency = G722SB_SAMPLE_FREQ;
    out->pcmType.nChannels = G722SB_NCHANNELS;

    return USC_NoError;
}

static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{

   Ipp32s lenSamples, lenBitstream, bitrate_idx;
   Ipp32s nBlocks, nSamples;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   bitrate_idx = CheckRate_G722SB(bitrate);

   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
   USC_BADARG(options->modes.vad>0, USC_UnsupportedVADType);

   lenSamples = options->framesize/(G722SB_BITS_PER_SAMPLE >> 3);
   lenBitstream = lenSamples >> 1;

   if(options->direction==USC_ENCODE) { /*Encode: src - PCMStream, dst - bitstream*/

      nSamples = nbytesSrc / (G722SB_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / lenSamples;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % lenSamples) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * lenBitstream;

   } else if(options->direction==USC_DECODE) {/*Decode: src - bitstream, dst - PCMStream*/
      nBlocks = nbytesSrc / lenBitstream;

      if (0 == nBlocks) return USC_NoOperation;

      nSamples = nBlocks * lenSamples;
      *nbytesDst = nSamples * (G722SB_BITS_PER_SAMPLE >> 3);

   } else if(options->direction==USC_DUPLEX) {/* Both: src - PCMStream, dst - PCMStream*/
      nSamples = nbytesSrc / (G722SB_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / lenSamples;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % lenSamples) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * lenSamples * (G722SB_BITS_PER_SAMPLE >> 3);
   } else return USC_NoOperation;
   return USC_NoError;
}

static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize)
{
   USC_CHECK_PTR(options);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(frameSize <= 0, USC_BadArgument);

   return USC_NoOperation;
}
