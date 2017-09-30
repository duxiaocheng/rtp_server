/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2012 Intel Corporation. All Rights Reserved.
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
// Purpose: GSMAMR speech codec: USC funtions.
//
*/

#include "usc.h"
#include "owngsmamr.h"
#include "gsmamrapi.h"

#define GSMAMR_NUM_RATES          8
#define GSMAMR_BITS_PER_SAMPLE   16
#define GSMAMR_SAMPLE_FREQ     8000
#define GSMAMR_NCHANNELS          1


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

typedef struct {
    Ipp16s sid_update_counter;
    TXFrameType prev_ft;
} sid_syncState;

typedef struct {
    USC_Direction direction;
    Ipp32s trunc;
    Ipp32s bitrate;
    Ipp32s vad;
    Ipp32s bitrate_old;
    Ipp16s reset_flag;
    Ipp16s reset_flag_old;
    sid_syncState sid_state;
	Ipp16s EFR_flag;	
} GSMAMR_Handle_Header;

static Ipp32s sid_sync_init (sid_syncState *st);
static void sid_sync(sid_syncState *st, GSMAMR_Rate_t mode, TXFrameType *tx_frame_type);
static Ipp32s is_pcm_frame_homing (Ipp16s *input_frame);

static Ipp16s is_bitstream_frame_homing (Ipp8u* bitstream, Ipp32s mode);


/* global usc vector table */
USCFUN USC_Fxns USC_GSMAMR_Fxns=
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


static __ALIGN32 CONST Ipp32s bitsLen[N_MODES]={
   95, 103, 118, 134, 148, 159, 204, 244, 35
};

static __ALIGN32 CONST USC_Rates pTblRates_GSMAMR[GSMAMR_NUM_RATES]={
    {12200},
    {10200},
    {7950},
    {7400},
    {6700},
    {5900},
    {5150},
   {4750}
};
static __ALIGN32 CONST USC_PCMType pcmTypesTbl_GSMAMR[1]={
   {GSMAMR_SAMPLE_FREQ,GSMAMR_BITS_PER_SAMPLE,GSMAMR_NCHANNELS}
};

    /* Frame classification according to bfi-flag and ternary-valued
       SID flag. The frames between SID updates (not actually trans-
       mitted) are also classified here; they will be discarded later
       and provided with "NO TRANSMISSION"-flag */


    /*
     *          SID1 = 1    SID1 = 1    SID1 = 0    SID1 = 0
     *          SID0 = 1    SID0 = 0    SID0 = 1    SID0 = 0
     *
     * BFI = 0 Undefined   Valid SID   Invalid SID Good Speech
     * BFI = 1 Undefined   Invalid SID Invalid SID Unusable Frame
     */

static __ALIGN32 CONST Ipp8u efr_frame_types[16] = {
                       /* TAF SID BFI */
    RX_SPEECH_GOOD,    /*  0   0   0  */
    RX_SPEECH_BAD,     /*  0   0   1  */
    RX_SID_INVALID,    /*  0   1   0  */
    RX_SID_BAD,        /*  0   1   1  */
    RX_SID_UPDATE,     /*  0   2   0  */
    RX_SID_BAD,        /*  0   2   1  */
    RX_SID_INVALID,    /*  0   3   0  */
    RX_SID_BAD,        /*  0   3   1  */
    RX_SPEECH_GOOD,    /*  1   0   0  */
    RX_SPEECH_BAD_TAF, /*  1   0   1  */
    RX_SID_INVALID,    /*  1   1   0  */
    RX_SID_BAD,        /*  1   1   1  */
    RX_SID_UPDATE,     /*  1   2   0  */
    RX_SID_BAD,        /*  1   2   1  */
    RX_SID_INVALID,    /*  1   3   0  */
    RX_SID_BAD         /*  1   3   1  */
};

static Ipp32s CheckRate_GSMAMR(Ipp32s rate_bps)
{
    Ipp32s rate;

    switch(rate_bps) {
      case 4750:  rate = 0; break;
      case 5150:  rate = 1; break;
      case 5900:  rate = 2; break;
      case 6700:  rate = 3; break;
      case 7400:  rate = 4; break;
      case 7950:  rate = 5; break;
      case 10200: rate = 6; break;
      case 12200: rate = 7; break;
      default: rate = -1; break;
    }
    return rate;
}

static Ipp32s CheckIdxRate_GSMAMR(Ipp32s idx_rate)
{
    Ipp32s rate;

    switch(idx_rate) {
      case 0: rate = 4750; break;
      case 1: rate = 5150; break;
      case 2: rate = 5900; break;
      case 3: rate = 6700; break;
      case 4: rate = 7400; break;
      case 5: rate = 7950; break;
      case 6: rate = 10200; break;
      case 7: rate = 12200; break;
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
    GSMAMR_Handle_Header *gsmamr_header;

    USC_CHECK_PTR(pInfo);

    pInfo->name = "IPP_GSMAMR";
    pInfo->params.framesize = GSMAMR_Frame*sizeof(Ipp16s);
    if (handle == NULL) {
      pInfo->params.modes.bitrate = 12200;
      pInfo->params.modes.truncate = 1;
      pInfo->params.direction = USC_DECODE;
      pInfo->params.modes.vad = 2;
    } else {
      gsmamr_header = (GSMAMR_Handle_Header*)handle;
      pInfo->params.modes.bitrate = gsmamr_header->bitrate;
      pInfo->params.modes.truncate = gsmamr_header->trunc;
      pInfo->params.direction = gsmamr_header->direction;
      pInfo->params.modes.vad = gsmamr_header->vad;
    }
    pInfo->maxbitsize = BITSTREAM_SIZE;
    pInfo->params.pcmType.sample_frequency = pcmTypesTbl_GSMAMR[0].sample_frequency;
    pInfo->params.pcmType.nChannels = pcmTypesTbl_GSMAMR[0].nChannels;
    pInfo->params.pcmType.bitPerSample = pcmTypesTbl_GSMAMR[0].bitPerSample;
    pInfo->nPcmTypes = 1;
    pInfo->pPcmTypesTbl = pcmTypesTbl_GSMAMR;
    pInfo->params.modes.hpf = 0;
    pInfo->params.modes.pf = 0;
    pInfo->params.modes.outMode = USC_OUT_NO_CONTROL;
    pInfo->params.law = 0;
    pInfo->nRates = GSMAMR_NUM_RATES;
    pInfo->pRateTbl = (const USC_Rates *)&pTblRates_GSMAMR;
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
    Ipp32u nbytes;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);

    pBanks->pMem = NULL;
    pBanks->align = 32;
    pBanks->memType = USC_OBJECT;
    pBanks->memSpaceType = USC_NORMAL;
   if (options->direction == USC_ENCODE) /* encode only */
    {
        GSMAMREnc_Params enc_params;
        enc_params.codecType = GSMAMR_CODEC;
        enc_params.mode = options->modes.vad;
        apiGSMAMREncoder_Alloc(&enc_params, &nbytes);
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        GSMAMRDec_Params dec_params;
        dec_params.codecType = GSMAMR_CODEC;
        dec_params.mode = options->modes.vad;
        apiGSMAMRDecoder_Alloc(&dec_params, &nbytes);
    } else {
        return USC_NoOperation;
    }
    pBanks->nbytes = nbytes + sizeof(GSMAMR_Handle_Header); /* room for USC header */
    return USC_NoError;
}

static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
    GSMAMR_Handle_Header *gsmamr_header;
    Ipp32s bitrate_idx;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);
    USC_CHECK_PTR(pBanks->pMem);
    USC_BADARG(pBanks->nbytes<=0, USC_NotInitialized);
    USC_CHECK_HANDLE(handle);
    USC_BADARG(options->modes.vad > 2, USC_UnsupportedVADType);
    USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);

    USC_BADARG(options->pcmType.bitPerSample!=GSMAMR_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.sample_frequency!=GSMAMR_SAMPLE_FREQ, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.nChannels!=GSMAMR_NCHANNELS, USC_UnsupportedPCMType);

    bitrate_idx = CheckRate_GSMAMR(options->modes.bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

    *handle = (USC_Handle*)pBanks->pMem;
    gsmamr_header = (GSMAMR_Handle_Header*)*handle;

    gsmamr_header->direction = options->direction;
    gsmamr_header->trunc = options->modes.truncate;
    gsmamr_header->vad = options->modes.vad;
    gsmamr_header->bitrate = options->modes.bitrate;
    gsmamr_header->bitrate_old = options->modes.bitrate;

	gsmamr_header->EFR_flag = options->modes.EFR_flag;
	
    if (options->direction == USC_ENCODE) /* encode only */
    {
        GSMAMREncoder_Obj *EncObj = (GSMAMREncoder_Obj *)((Ipp8s*)*handle + sizeof(GSMAMR_Handle_Header));
		apiGSMAMREncoder_Init((GSMAMREncoder_Obj*)EncObj, gsmamr_header->vad, gsmamr_header->EFR_flag, 0);
		gsmamr_header->reset_flag = 0;
        gsmamr_header->reset_flag_old = 1;
    }
    else if (options->direction == USC_DECODE) /* decode only */
    {
        GSMAMRDecoder_Obj *DecObj = (GSMAMRDecoder_Obj *)((Ipp8s*)*handle + sizeof(GSMAMR_Handle_Header));
        apiGSMAMRDecoder_Init((GSMAMRDecoder_Obj*)DecObj, gsmamr_header->vad,gsmamr_header->EFR_flag, 0);
        gsmamr_header->reset_flag = 0;
        gsmamr_header->reset_flag_old = 1;
    } else {
        *handle = NULL;
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle )
{
    GSMAMR_Handle_Header *gsmamr_header;
    Ipp32s bitrate_idx;

    USC_CHECK_PTR(modes);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(modes->vad > 2, USC_UnsupportedVADType);
    USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

    gsmamr_header = (GSMAMR_Handle_Header*)handle;

    bitrate_idx = CheckRate_GSMAMR(modes->bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
    gsmamr_header->vad = modes->vad;
    gsmamr_header->bitrate = modes->bitrate;
    gsmamr_header->bitrate_old = modes->bitrate;
    gsmamr_header->EFR_flag = modes->EFR_flag;
	
    if (gsmamr_header->direction == USC_ENCODE) /* encode only */
    {
        GSMAMREncoder_Obj *EncObj = (GSMAMREncoder_Obj *)((Ipp8s*)handle + sizeof(GSMAMR_Handle_Header));
        gsmamr_header->reset_flag = 0;
        sid_sync_init (&gsmamr_header->sid_state);		
		apiGSMAMREncoder_Init((GSMAMREncoder_Obj*)EncObj, modes->vad, gsmamr_header->EFR_flag,EncObj->stEncState.dtx.ef_tx.taf_count);		
    }
    else if (gsmamr_header->direction == USC_DECODE) /* decode only */
    {
        GSMAMRDecoder_Obj *DecObj = (GSMAMRDecoder_Obj *)((Ipp8s*)handle + sizeof(GSMAMR_Handle_Header));
		apiGSMAMRDecoder_Init((GSMAMRDecoder_Obj*)DecObj, modes->vad, gsmamr_header->EFR_flag,DecObj->stDecState.dtx.ef_rx.taf_count);
		gsmamr_header->reset_flag = 0;
        gsmamr_header->reset_flag_old = 1;
    } else {
        return USC_NoOperation;
    }
    return USC_NoError;
}

static USC_Status Control(const USC_Modes *modes, USC_Handle handle )
{
   GSMAMR_Handle_Header *gsmamr_header;
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(modes->vad > 2, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   gsmamr_header = (GSMAMR_Handle_Header*)handle;

   bitrate_idx = CheckRate_GSMAMR(modes->bitrate);
   USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
   gsmamr_header->vad = modes->vad;
   gsmamr_header->bitrate = modes->bitrate;
   gsmamr_header->trunc = modes->truncate;

   if (gsmamr_header->direction == USC_ENCODE) /* encode only */
   {
      GSMAMREncoder_Obj *EncObj = (GSMAMREncoder_Obj *)((Ipp8s*)handle + sizeof(GSMAMR_Handle_Header));
      apiGSMAMREncoder_Mode((GSMAMREncoder_Obj*)EncObj, modes->vad);
   }
   else if (gsmamr_header->direction == USC_DECODE) /* decode only */
   {
      GSMAMRDecoder_Obj *DecObj = (GSMAMRDecoder_Obj *)((Ipp8s*)handle + sizeof(GSMAMR_Handle_Header));
      apiGSMAMRDecoder_Mode((GSMAMRDecoder_Obj*)DecObj, modes->vad);
   } else {
      return USC_NoOperation;
   }
    return USC_NoError;
}

#define IO_MASK (Ipp16u)0xfff8     /* 13-bit input/output                      */
static __ALIGN32 CONST GSMAMR_Rate_t usc2amr[8]={
    GSMAMR_RATE_4750,
    GSMAMR_RATE_5150,
    GSMAMR_RATE_5900,
    GSMAMR_RATE_6700,
    GSMAMR_RATE_7400,
    GSMAMR_RATE_7950,
    GSMAMR_RATE_10200,
    GSMAMR_RATE_12200
};


static Ipp32s getBitstreamSize(Ipp32s rate, Ipp32s frametype, Ipp32s *outRate)
{
   Ipp32s nbytes;
   Ipp32s usedRate = rate;

   if (frametype == 1 || frametype == 2 || frametype == 3||frametype == 6){
        usedRate = GSMAMR_RATE_DTX;
   }
    nbytes = ((bitsLen[usedRate] + 7) >> 3);
   //*outRate = usedRate;
   *outRate = rate;

   return nbytes;

}

static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
    GSMAMR_Handle_Header *gsmamr_header;
    GSMAMREncoder_Obj *EncObj;
    Ipp32s bitrate_idx, out_WrkRate;
    IPP_ALIGNED_ARRAY(16, Ipp16u, work_buf, GSMAMR_Frame);
    Ipp32s pVad = 0;

    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(in->pcmType.bitPerSample!=GSMAMR_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.sample_frequency!=GSMAMR_SAMPLE_FREQ, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.nChannels!=GSMAMR_NCHANNELS, USC_UnsupportedPCMType);

    USC_BADARG(in->nbytes<GSMAMR_Frame*sizeof(Ipp16s), USC_NoOperation);

    USC_BADARG(NULL == in->pBuffer, USC_NoOperation);

    gsmamr_header = (GSMAMR_Handle_Header*)handle;

    USC_BADARG(USC_ENCODE != gsmamr_header->direction, USC_NoOperation);

    bitrate_idx = CheckRate_GSMAMR(in->bitrate);
    USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);
    gsmamr_header->bitrate = in->bitrate;
    EncObj = (GSMAMREncoder_Obj *)((Ipp8s*)handle + sizeof(GSMAMR_Handle_Header));

    /* check for homing frame */
    gsmamr_header->reset_flag = (Ipp16s)is_pcm_frame_homing((Ipp16s*)in->pBuffer);

    if (gsmamr_header->trunc) {
        /* Delete the LSBs */
        ippsAndC_16u((Ipp16u*)in->pBuffer, IO_MASK, work_buf, GSMAMR_Frame);
    } else {
        ippsCopy_16s((const Ipp16s*)in->pBuffer, (Ipp16s*)work_buf, GSMAMR_Frame);
    }

    /* do a check before the encoding */
    {
        Ipp32u codeSize;
		if (apiGSMAMREncoder_GetSize(EncObj, &codeSize) != APIGSMAMR_StsNoErr)
			return USC_NoOperation;
	}

    EncObj->reset_flag = gsmamr_header->reset_flag;
	EncObj->reset_flag_old = gsmamr_header->reset_flag_old;
	 
    if(apiGSMAMREncode(EncObj,(Ipp16s*)work_buf,usc2amr[bitrate_idx&7],(Ipp8u*)out->pBuffer,&pVad) != APIGSMAMR_StsNoErr){
       return USC_NoOperation;
    }

    out->ef_txdtx_ctrl = EncObj->stEncState.dtx.ef_tx.txdtx_ctrl;

	if ((EncObj->stEncState.dtx.ef_tx.txdtx_ctrl & EF_TX_ACTIVE) == 0)
	{
		/* include frame type and mode information in serial bitstream */
	    if(!pVad) {
	        sid_sync (&gsmamr_header->sid_state, GSMAMR_RATE_DTX, (TXFrameType *)&out->frametype);
	    } else {
	        sid_sync (&gsmamr_header->sid_state, (GSMAMR_Rate_t)bitrate_idx, (TXFrameType *)&out->frametype);
	    }

	}
	else
	{
	    out->frametype = EncObj->frame_type;
	}

	if (gsmamr_header->EFR_flag ==1)
	{
		out->nbytes = 31;
		out->bitrate = 12200;
	}
	else
	{

	    out->nbytes = getBitstreamSize(bitrate_idx, out->frametype, &out_WrkRate);
	    out->bitrate = CheckIdxRate_GSMAMR(out_WrkRate);
	}

    in->nbytes = GSMAMR_Frame*sizeof(Ipp16s);

    if (gsmamr_header->reset_flag != 0) {
        sid_sync_init (&gsmamr_header->sid_state);
		apiGSMAMREncoder_Init(EncObj, EncObj->objPrm.mode,gsmamr_header->EFR_flag,EncObj->stEncState.dtx.ef_tx.taf_count);			
    }
	gsmamr_header->reset_flag_old = gsmamr_header->reset_flag;
	
    return USC_NoError;
}

static RXFrameType tx2rx (TXFrameType tx_type)
{
    switch (tx_type) {
      case TX_SPEECH_GOOD:      return RX_SPEECH_GOOD;
      case TX_SID_FIRST:        return RX_SID_FIRST;
      case TX_SID_UPDATE:       return RX_SID_UPDATE;
      case TX_NO_DATA:          return RX_NO_DATA;
      case TX_SPEECH_DEGRADED:  return RX_SPEECH_DEGRADED;
      case TX_SPEECH_BAD:       return RX_SPEECH_BAD;
      case TX_SID_BAD:          return RX_SID_BAD;
      case TX_ONSET:            return RX_ONSET;
      default:
         return RX_NON_SPEECH;
    }
}

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
    GSMAMR_Handle_Header *gsmamr_header;
    Ipp32s bitrate_idx;
    GSMAMRDecoder_Obj *DecObj;
    Ipp32s PLC = 0;
	RXFrameType frame_type;


    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    gsmamr_header = (GSMAMR_Handle_Header*)handle;

    USC_BADARG(USC_DECODE != gsmamr_header->direction, USC_NoOperation);

    DecObj = (GSMAMRDecoder_Obj *)((Ipp8s*)handle + sizeof(GSMAMR_Handle_Header));

    if(in == NULL) {
      PLC = 1;
    } else if(in->pBuffer == NULL) {
      PLC = 1;
    }
	
    if(PLC) {
       /* Lost frame */
      bitrate_idx = CheckRate_GSMAMR(gsmamr_header->bitrate_old);
      USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

      gsmamr_header->bitrate = gsmamr_header->bitrate_old;
	  
	  {
	      Ipp32u codeSize;
	      if (apiGSMAMRDecoder_GetSize(DecObj, &codeSize) != APIGSMAMR_StsNoErr)
			return USC_NoOperation;
	  }

      if(apiGSMAMRDecode(DecObj,(const Ipp8u*)LostFrame,(GSMAMR_Rate_t)bitrate_idx,
           RX_SPEECH_BAD,(Ipp16s*)out->pBuffer) != APIGSMAMR_StsNoErr){
            return USC_NoOperation;
      }
      if(gsmamr_header->trunc) {
         /* Truncate LSBs */
         ippsAndC_16u((Ipp16u*)out->pBuffer, IO_MASK, (Ipp16u*)out->pBuffer, GSMAMR_Frame);
      }
      gsmamr_header->reset_flag = 0;
      gsmamr_header->reset_flag_old = 1;

      out->nbytes = GSMAMR_Frame*sizeof(Ipp16s);
    } else {
      bitrate_idx = CheckRate_GSMAMR(in->bitrate);
      USC_BADARG(bitrate_idx < 0, USC_UnsupportedBitRate);

	  if (gsmamr_header->EFR_flag == 1)
	  {
		  frame_type = (RXFrameType)efr_frame_types[in->frametype];
	  }
	  else
	  {
		  frame_type = tx2rx((TXFrameType)in->frametype);
	  }

      gsmamr_header->bitrate = in->bitrate;

      gsmamr_header->bitrate_old = in->bitrate;


      if (gsmamr_header->EFR_flag == 1)
      {
      	gsmamr_header->reset_flag = is_bitstream_frame_homing((Ipp8u *)in->pBuffer, 8); 

	  }
	  else
	  {
        gsmamr_header->reset_flag = is_bitstream_frame_homing((Ipp8u *)in->pBuffer, bitrate_idx); 
	  }


      /* produce encoder homing frame if homed & input=decoder homing frame */
      if ((gsmamr_header->reset_flag != 0) && (gsmamr_header->reset_flag_old != 0))
      {
         ippsSet_16s(EHF_MASK, (Ipp16s*)out->pBuffer, GSMAMR_Frame);
      }
      else
      {

	  	/* do a check before the encoding */
    	  {
	        Ipp32u codeSize;
			if (apiGSMAMRDecoder_GetSize(DecObj, &codeSize) != APIGSMAMR_StsNoErr)
				return USC_NoOperation;
		  }

		 if(apiGSMAMRDecode(DecObj,(const Ipp8u*)in->pBuffer,(GSMAMR_Rate_t)bitrate_idx,
             frame_type,(Ipp16s*)out->pBuffer) != APIGSMAMR_StsNoErr){
             return USC_NoOperation;
          }
          if (gsmamr_header->trunc) {
             /* Truncate LSBs */
             ippsAndC_16u((Ipp16u*)out->pBuffer, IO_MASK, (Ipp16u*)out->pBuffer, GSMAMR_Frame);
          }
      }
      /* reset decoder if current frame is a homing frame */
      if (gsmamr_header->reset_flag != 0)
      {
      	  apiGSMAMRDecoder_Init(DecObj, DecObj->objPrm.mode, gsmamr_header->EFR_flag, DecObj->stDecState.dtx.ef_rx.taf_count);
	  }
      gsmamr_header->reset_flag_old = gsmamr_header->reset_flag;

       out->nbytes = GSMAMR_Frame*sizeof(Ipp16s);

	    if (gsmamr_header->EFR_flag == 1)
		{
			in->nbytes = 31;
		}
		else
		{
			Ipp32s foo;		  
            in->nbytes = getBitstreamSize(bitrate_idx, in->frametype, &foo);
		  
		}

    }

    out->bitrate = gsmamr_header->bitrate;
    out->pcmType.bitPerSample = GSMAMR_BITS_PER_SAMPLE;
    out->pcmType.sample_frequency = GSMAMR_SAMPLE_FREQ;
    out->pcmType.nChannels = GSMAMR_NCHANNELS;

    return USC_NoError;
}

static Ipp32s is_pcm_frame_homing (Ipp16s *input_frame)
{
    Ipp32s i, j=0;

    /* check 160 input samples for matching EHF_MASK: defined in e_homing.h */
    for (i = 0; i < GSMAMR_Frame; i++)
    {
        j = input_frame[i] ^ EHF_MASK;

        if (j)
            break;
    }

    return !j;
}

static Ipp32s sid_sync_init (sid_syncState *state)
{
    state->sid_update_counter = 3;
    state->prev_ft = TX_SPEECH_GOOD;
    return 0;
}

static void sid_sync (sid_syncState *st, GSMAMR_Rate_t mode, TXFrameType *tx_frame_type)
{
    if ( mode == GSMAMR_RATE_DTX){
       st->sid_update_counter--;
        if (st->prev_ft == TX_SPEECH_GOOD)
        {
           *tx_frame_type = TX_SID_FIRST;
           st->sid_update_counter = 3;
        }
        else
        {
           if (st->sid_update_counter == 0)
           {
              *tx_frame_type = TX_SID_UPDATE;
              st->sid_update_counter = 8;
           } else {
              *tx_frame_type = TX_NO_DATA;
           }
        }
    }
    else
    {
       st->sid_update_counter = 8 ;
       *tx_frame_type = TX_SPEECH_GOOD;
    }
    st->prev_ft = *tx_frame_type;
    return;
}

/*
  homing decoder frames by modes
*/
static __ALIGN32 CONST Ipp8u home_bits_efr[]={
   0x08,0x5e,0xb4,0x90,
   0xfa,0xad,0x60,0x3e,
   0x3a,0x18,0x60,0x7b,
   0x0c,0x42,0xc0,0x80,
   0x48,0x05,0x58,0x00,
   0x00,0x00,0x00,0x00,
   0x36,0xb0,0x00,0x00,
   0x00,0x00,0x00
};


/*
  homing decoder frames by modes
*/
static __ALIGN32 CONST Ipp8u home_bits_122[]={
   0x08,0x54,0xdb,0x96,
   0xaa,0xad,0x60,0x00,
   0x00,0x00,0x00,0x1b,
   0x58,0x7f,0x66,0x83,
   0x79,0x40,0x90,0x04,
   0x15,0x85,0x4f,0x10,
   0xf6,0xb0,0x24,0x03,
   0xc7,0xea,0x00
};
static __ALIGN32 CONST Ipp8u home_bits_102[]={
   0xf8,0x71,0x8b,0xd1,
   0x40,0x00,0x00,0x00,
   0x00,0xda,0xe4,0xc6,
   0x77,0xea,0x2c,0x40,
   0xad,0x6b,0x3d,0x80,
   0x6c,0x17,0xc8,0x55,
   0xc3,0x00
};
static __ALIGN32 CONST Ipp8u home_bits_795[]={
   0x61,0x38,0xc5,0xf7,
   0xa0,0x06,0xfa,0x07,
   0x3c,0x08,0x7a,0x5b,
   0x1c,0x69,0xbc,0x41,
   0xca,0x68,0x3c,0x82
};
static __ALIGN32 CONST Ipp8u home_bits_74[]={
   0xf8,0x71,0x8b,0xef,
   0x40,0x0d,0xe0,0x36,
   0x20,0x8f,0xc4,0xc1,
   0xba,0x6f,0x01,0xb0,
   0x03,0x78,0x00
};
static __ALIGN32 CONST Ipp8u home_bits_67[]={
   0xf8,0x71,0x8b,0xef,
   0x40,0x17,0x01,0xe2,
   0x63,0xe1,0x60,0xb8,
   0xbc,0x07,0xb1,0x8e,
   0x00
};
static __ALIGN32 CONST Ipp8u home_bits_59[]={
   0xf8,0x71,0x8b,0xef,
   0x40,0x1e,0xfe,0x01,
   0xcf,0x60,0x7c,0xfb,
   0xf8,0x03,0xdc
};
static __ALIGN32 CONST Ipp8u home_bits_515[]={
   0xf8,0x9d,0x38,0xcc,
   0x03,0xdf,0xc0,0x62,
   0xfb,0x7f,0x7f,0x47,
   0xbe
};
static __ALIGN32 CONST Ipp8u home_bits_475[]={
   0xf8,0x9d,0x38,0xcc,
   0x03,0x28,0xf7,0x0f,
   0xb1,0x82,0x3d,0x36
};

static __ALIGN32 CONST Ipp8u *d_homes[] =
{
   home_bits_475,
   home_bits_515,
   home_bits_59,
   home_bits_67,
   home_bits_74,
   home_bits_795,
   home_bits_102,
   home_bits_122
  ,home_bits_efr   
};

static Ipp16s is_bitstream_frame_homing (Ipp8u* bitstream, Ipp32s mode)
{
   Ipp32s i;
   Ipp16s j=0;
   Ipp32s bitstremlen = (bitsLen[mode]+7)>>3;
   for (i = 0; i < bitstremlen; i++)
   {
       j = (Ipp16s)(bitstream[i] ^ d_homes[mode][i]);

       if (j)
           break;
   }
    return (Ipp16s)(!j);

}

static USC_Status CalsOutStreamSize(const USC_Option *options, Ipp32s bitrate_idx, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s nBlocks, nSamples, foo;

   if(options->direction==USC_ENCODE) { /*Encode: src - PCMStream, dst - bitstream*/
      USC_BADARG(options->modes.vad>2, USC_UnsupportedVADType);

      nSamples = nbytesSrc / (GSMAMR_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / GSMAMR_Frame;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % GSMAMR_Frame) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }

      *nbytesDst = nBlocks * getBitstreamSize(bitrate_idx, TX_SPEECH_GOOD, &foo);
   } 
   else if(options->direction==USC_DECODE) {/*Decode: src - bitstream, dst - PCMStream*/
      if(options->modes.vad==0) { /*VAD off*/
         nBlocks = nbytesSrc / getBitstreamSize(bitrate_idx, TX_SPEECH_GOOD, &foo);
      } else if((options->modes.vad==1)||(options->modes.vad==2)) { /*VAD on*/
         nBlocks = nbytesSrc / getBitstreamSize(bitrate_idx, TX_SID_FIRST, &foo);
      } else return USC_UnsupportedVADType;

      if (0 == nBlocks) return USC_NoOperation;

      nSamples = nBlocks * GSMAMR_Frame;
      *nbytesDst = nSamples * (GSMAMR_BITS_PER_SAMPLE >> 3);

   } 
#if(0)
   else if(options->direction==USC_DUPLEX) {/* Both: src - PCMStream, dst - PCMStream*/
      nSamples = nbytesSrc / (GSMAMR_BITS_PER_SAMPLE >> 3);
      nBlocks = nSamples / GSMAMR_Frame;

      if (0 == nBlocks) return USC_NoOperation;

      if (0 != nSamples % GSMAMR_Frame) {
         /* Add another block to hold the last compressed fragment*/
         nBlocks++;
      }
      *nbytesDst = nBlocks * GSMAMR_Frame * (GSMAMR_BITS_PER_SAMPLE >> 3);
   }
#endif
   else return USC_NoOperation;

   return USC_NoError;
}
static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s bitrate_idx;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   bitrate_idx = CheckRate_GSMAMR(bitrate);
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
