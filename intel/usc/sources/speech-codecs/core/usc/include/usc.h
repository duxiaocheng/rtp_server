/*****************************************************************************
//
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 2005-2011 Intel Corporation. All Rights Reserved.
//
// Intel(R) Integrated Performance Primitives
//
//     USCI - Unified Speech Codec Interface
//
//  Purpose: USCI Speech Codec header file.
//               Structures and functions declarations.
*****************************************************************************/

#ifndef __USC_H__
#define __USC_H__

#include "usc_base.h"

#ifndef USC_MAX_NAME_LEN
#define USC_MAX_NAME_LEN 64
#endif

typedef enum {
    ENC_INP_NONE = 0, /* not available */
    ENC_INP_WAV = 1, /* IPP Wave Format */
    ENC_INP_ITU = 2,  /* ITU .INP Format (RAW_PCM) */
    ENC_INP_ITUI = 3,  /* ITU .INP Format (RAW_PCM interleaved stereo) */
	ENC_INP_ALAW = 4,  /* G.711 legacy A-law format */
	ENC_INP_MULAW = 5  /* G.711 legacy Mu-law format */
} USC_EncInputFormat;

typedef enum {
	ENC_OUTP_NONE = 0, /* not available */
	ENC_OUTP_ICW  = 1, /* IPP Compressed Wave Format */
    ENC_OUTP_ITU  = 2, /* ITU .ITU Format */
    ENC_OUTP_COD  = 3, /* ITU .COD Format */
    ENC_OUTP_AMR  = 4, /* Only for AMR IF1 Format */
	ENC_OUTP_BIT  = 5, /* G.722 raw 16-bit Format */
	ENC_OUTP_BITO = 6, /* OPUS packet Format */
    ENC_OUTP_ITUJ = 7, /* ITU .j raw byte Format */
	ENC_OUTP_711  = 8  /* G.711 legacy raw bit format */
} USC_EncOutputFormat;

typedef enum {
	DEC_INP_NONE = 0, /* not available */
	DEC_INP_ICW  = 1, /* IPP Compressed Wave Format */
    DEC_INP_ITU  = 2, /* ITU .ITU Format */
    DEC_INP_COD  = 3, /* ITU .COD Format */
    DEC_INP_AMR  = 4, /* Only for AMR IF1 Format */
	DEC_INP_BIT  = 5, /* G.722 raw bit Format */
	DEC_INP_BITO = 6, /* OPUS packet Format */
    DEC_INP_ITUJ = 7, /* ITU .j raw byte Format */
	DEC_INP_711  = 8, /* G.711 legacy raw bit format */
	DEC_INP_DEC  = 9, /* EFR decoder input raw bit format */
	DEC_INP_723  = 10, /* G.723 legacy raw bit format */
	DEC_INP_EVS_RTP = 11 /* EVS RTP format for test purpose*/
} USC_DecInputFormat;

typedef enum {
	DEC_OUTP_NONE = 0, /* not available */
    DEC_OUTP_WAV = 1, /* IPP Wave Format */
    DEC_OUTP_OUT = 2,  /* ITU .OUT Format (RAW_PCM) */	
    DEC_OUTP_OUI = 3,  /* ITU .OUI Format (RAW_PCM interleaved stereo) */
	DEC_OUTP_ALAW = 4,  /* G.711 legacy A-law format */
	DEC_OUTP_MULAW = 5  /* G.711 legacy Mu-law format */
} USC_DecOutputFormat;

typedef enum {
   USC_ENCODE = 0, /* Encoder */
   USC_DECODE = 1, /* Decoder */
   USC_DUPLEX = 2, /* Both  */
   USC_MAX_DIRECTION_TYPES /* Number of direction types */
} USC_Direction;

typedef enum  {
    USC_OUT_NO_CONTROL     =0,
    USC_OUT_MONO           =1,
    USC_OUT_STEREO         =2,
    USC_OUT_COMPATIBLE     =3,
    USC_OUT_DELAY          =4,
    USC_MAX_OUTPUT_MODES
} USC_OutputMode;

/* USC codec modes, (may be alternated) */
typedef struct {
   int            bitrate;    /* in bps */
   int            truncate;   /* 0 - no truncate */
   int            vad;        /* 0 - disabled, otherwise VAD type (1,2, more if any) */
   int            plc;        /* 0 - disabled, otherwise enabled */
   int            level_adj;  /* Level adjust (G.722). -6dB on encoder input, +6dB on decoder output. */
   int            hpf;        /* high pass filter: 1- on, 0- off */
   int            pf;         /* post filter / AGC for AMRWB+ : 1- on, 0- off */
   USC_OutputMode outMode;    /* codec output mode */
   /* following parameters are for OPUS*/
   int      sample_frequency;
   int      samplesPerFrame;
   short	nChannels;
   short	application;	/* 0=AUDIO, 1=VOIP, 2=RESTRICTED_LOWDELAY */		   
   short	cbr_flag;			   
   short	cvbr_flag;			   
   short	inbandfec;			   
   short	forcemono_flag;			   
   short	complexity;			   
   short	max_payload;			   
   short	pcLost;			        /* % lost frames Packet Loss Resilience */
   /* following parameters are for OPUS and EVS */
   short	max_bwidth_fx;        /* maximum encoded bandwidth */
   /* following parameters are for EVS*/
   short	Opt_RF_ON;			   /* RF on or off*/
   short	rf_fec_offset;         /* RF FEC offset in num of frames */
   short	rf_fec_indicator;	   /* RF FEC indicator: HI or LO */
   short	interval_SID_fx;      /* CNG and DTX - interval of SID update, default 8 */
   short	update_brate_on;	  /* bit rate update flag. Set to one when new rate is set */
   short	update_bwidth_on;	  /* bandwidh update flag. Set to one when new bandwidth is set */
   short    bitstreamformat;      /* input bitstream format for EVS decoder*/
   short    use_parital_copy;     /* a flag that indicates if partial frame is used, EVS RTP only*/
   short    next_coder_type;	  /* next frame coder type information, for EVS RTP format only*/
   short	jbm_enabled;		  /* Internal jitter buffer module enabled flag */
   int		system_time_ms;		  /* For internal EVS jitter buffer*/
   short    jbIsEmpty;            /* A flag to indicate if the internal jitter buffer is empty */
   /* following parameters are for GSMEFR*/
   short          EFR_flag;   /* flag for using the EFR mode (integrated with AMR) */
   short          efr_txdtx_ctrl; /*efr control flag used for generating output */   
}USC_Modes;

typedef struct {
  int bitrate;         /* in bps */
} USC_Rates;

/* USC codec option */
typedef struct {
   USC_Direction    direction;  /* 0 - encode only, 1 - decode only, 2 - both */
   int              law;        /* 0 - pcm, 1 - aLaw, 2 -muLaw */
   int              framesize;  /* a codec frame size (in bytes) */
   USC_PCMType      pcmType;    /* PCM type to support */
   int              nModes;
   USC_Modes        modes;
}USC_Option;


/* USC codec information */
typedef struct {
   const char        *name;          /* codec name */
   int                maxbitsize;    /* bitstream max frame size in bytes */
   int                nPcmTypes;     /* PCM Types tbl entries number */
   USC_PCMType       *pPcmTypesTbl;  /* supported PCMs lookup table */
   int                nRates;        /* Rate tbl entries number */
   const USC_Rates   *pRateTbl;      /* supported bitrates lookup table */
   USC_Option         params;        /* what is supported */
}USC_CodecInfo;

typedef struct{
	short      rtpSequenceNumber;
	int        rtpTimeStamp;
	int 	   RcvTime_ms;
}RTP_Info;

/* USC compressed bits */
typedef struct {
   char      *pBuffer;    /* bitstream data buffer pointer */
   int        nbytes;     /* bitstream size in byte */
   int        frametype;  /* codec specific frame type (transmission frame type) */
   int        bitrate;    /* in bps */
   short      ef_txdtx_ctrl; 
   /* Used only by RTP frames with JBM for EVS decoder*/
   RTP_Info   rtp_info;
}USC_Bitstream;

/* USC functions table.
   Each codec should supply a function table structure, which is derived from this base table.
   Codec table is to be specified for each codec by name as follows:
       USC_<codec-name>_Fxns, for example USC_g729_Fxns.
   The typical usage model:
    - Questing a codec about memory requirement using  MemAlloc() function
      which returns a memory banks description table with required bank sizes.
    - Use Init() function to create codec instance according to modes (vad, rate etc) requested.
      Codec handle is returned. Thus different instances of particular codec may be created
      and used in parallel.
    - Encode(), Decode() - compression/decompression functions.
    - GetInfo() - inquire codec state or codec requirement.
*/

typedef struct {

    USC_baseFxns std;


   /*   Encode()
        in - input audio stream (pcm) pointer,
        out - output bitstream pointer ,
    */
    USC_Status (*Encode)(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out);

    /*  Decode()
        in -  input bitstream pointer,
        out - output audio stream pointer,
    */
    USC_Status (*Decode)(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out);

   /*   GetOutStreamSize()
        bitrate -  input bitrate in kbit/s,
        nBytesSrc - lenght in bytes of the input stream,
        nBytesDst - output lenght in bytes of the output stream,
    */
   USC_Status (*GetOutStreamSize)(const USC_Option *options, int bitrate, int nbytesSrc, int *nbytesDst);
   /*   SetFrameSize()
        frameSize -  Desired frame size in bytes,
    */
   USC_Status (*SetFrameSize)(const USC_Option *options, USC_Handle handle, int frameSize);

   /*  FeedJBMFrame()
   in -  input bitstream pointer,
   */
   USC_Status(*FeedJBMFrame)(USC_Handle handle, USC_MemBank *pBanks, USC_Bitstream *in);

   /*  GetJBMFrame()
   out - output audio stream pointer,
   */
   USC_Status(*GetJBMFrame)(USC_Handle handle, USC_MemBank *pBanks, USC_PCMStream *out, USC_Modes *pModes);

} USC_Fxns;

#endif /* __USC_H__ */
