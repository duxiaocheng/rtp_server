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
// Purpose: AMRWBE speech codec own header file
//
*/

#ifndef __OWNAMRWBE_H__
#define __OWNAMRWBE_H__

#if defined(_WIN32_WCE)
#pragma warning( disable : 4505 )
#pragma warning( disable : 4305 ) //'identifier' : truncation from 'type1' to 'type2'
#endif

#include <ipps.h>
#include <ippsc.h>
#include "scratchmem.h"
#include "apiamrwbe.h"
#include "aux_fnxs.h"

#define   CODECFUN(type,name,arg)                extern type name arg

#define USE_CASE_A       0
#define USE_CASE_B       1 /* ==VAD==DTX */

#define L_FILT_OVER    12
#define L_FILT_DECIM   ((L_FILT_OVER+1)*60)
#define L_FILT_SPLIT   24
#define L_FILT_JOIN    12
#define L_MEM_DECIM_SPLIT (2*(L_FILT_SPLIT+L_FILT_DECIM))
#define L_MEM_JOIN_OVER   (2*(L_FILT_OVER+L_FILT_JOIN+L_FILT_JOIN))

#define L_FILT24k    23

/* codec constant assuming at fs=12.8kHz */

#define L_OVLP       128     /* 80ms TCX overlap size (10 ms)              */
#define L_TCX        (1024+128)  /* 80ms TCX Frame size (80ms + 10 ms overlap) */

#define L_FRAME_PLUS 1024    /* 80ms frame size (long TCX frame)           */
#define L_DIV        256     /* 20ms frame size (ACELP or Ipp16s TCX frame) */
#define NB_DIV       4       /* number of division (20ms) per 80ms frame   */
#define NB_SUBFR_WBP 16      /* number of 5ms subframe per 80ms frame      */
#define L_SUBFR      64      /* subframe size (5ms)                        */


#define L_WINDOW_WBP 448     /* 35ms window size in LP analysis            */

#define L_NEXT_HIGH_RATE       288     /* overhead in LP analysis                    */
#define L_WINDOW_HIGH_RATE     512     /* window size in LP analysis (50% overlap)   */

#define M            16      /* order of LP filter                         */
#define MHF          8       /* order of LP filter for HF band             */

#define L_FILT                12      /* Delay of up-sampling filter                */
#define DELAY_SPLIT           54      /* Additional delay of mid band signal due to split filter */
#define DELAY_PF              (L_FILT+L_SUBFR)     /* decoder delay from post-filter e.tc*/
#define DELAY_PF_HIGH_RATE    (L_FILT+(2*L_SUBFR)) /* decoder delay from post-filter e.tc*/

#define L_TOTAL_PLUS          (M+L_FRAME_PLUS+L_NEXT_WBP)   /* total size of speech buffer.  */
#define L_TOTAL_HIGH_RATE     (M+L_FRAME_PLUS+L_NEXT_HIGH_RATE)

/* AMR_WB+ mode relative to AMR-WB core */
#define MODE_9k6     0  /*IPP_SPCHBR_9600*/
#define MODE_11k2    1
#define MODE_12k8    2
#define MODE_14k4    3
#define MODE_16k     4
#define MODE_18k4    5
#define MODE_20k     6
#define MODE_23k2    7

/* number of bits for parametric bandwidth extension (BWE) */
//#define  NBITS_BWE    (4*16)     /* 4 packets x 16 bits = 0.8 kbps */
#define  NPRM_BWE_DIV  6         /* 12 on 40ms frame, 24 on 80ms frame */

/* number of packets per frame (4 packets of 20ms) */
#define  N_PACK_MAX   4

/* codec mode: 0=ACELP, 1=TCX20, 2=TCX40, 3=TCX80 */
#define  NBITS_MODE   (4*2)      /* 4 packets x 2 bits */
#define  NBITS_LPC    (46)       /* AMR-WB LPC quantizer */

#define FSCALE_DENOM_NORMS 8     /* filter into decim_split.h */

//delay of STEREO need to be ajusted to new bass_postfilter delay (+64 samples)
//before adding these new pitch limit for higher rate...

#define PIT_MIN_12k8    34      /* Minimum pitch lag with resolution 1/4      */
#define PIT_FR2_12k8    128     /* Minimum pitch lag with resolution 1/2      */
#define PIT_FR1_12k8    160     /* Minimum pitch lag with resolution 1        */
#define PIT_MAX_12k8    231     /* Maximum pitch lag                          */

/* Maximum pitch lag for highest freq. scaling factor  */
#define PIT_MAX_MAX     (PIT_MAX_12k8 + (6*((((FAC_FSCALE_MAX*PIT_MIN_12k8)+(FSCALE_DENOM/2))/FSCALE_DENOM)-PIT_MIN_12k8)))
/*  PIT_MAX_MAX = 378.5 !!! BUT BY rounding 375 only !!! */
#define PIT_MAX         231

#define OPL_DECIM    2       /* Decimation in open-loop pitch analysis     */
#define L_MEANBUF    3        /* for isf recovery */

/* AMRWB+ core parameters constants */
#define  NPRM_LPC     7                /* number of prm for LPC */
#define  NPRM_RE8     (L_TCX+(L_TCX/8))
#define  NPRM_TCX80   (2+NPRM_RE8)           /* TCX 80ms */
#define  NPRM_TCX40   (2+(NPRM_RE8/2))       /* TCX 40ms */
#define  NPRM_TCX20   (2+(NPRM_RE8/4))       /* TCX 20ms */
#define  NPRM_DIV     (NPRM_LPC+NPRM_TCX20)  /* buffer size = NB_DIV*NPRM_DIV */
/* number of parameters on the decoder side (AVQ use 4 bits per parameter) */
#define  DEC_NPRM_DIV (((24*80)/4)/NB_DIV)   /* set for max of 24kbps (TCX) */

#define L_FILT_2k       128
#define L_DIV_2k        ((L_DIV * 5)/32)
#define L_FRAME_2k      160
#define L_NEXT_2k       80

#define L_ANA           (12800 * 30 / 1000)
#define L_ANA_2k        (2000 * 80/1000)
#define L_B             ((L_ANA - L_DIV)/2)
#define L_B_2k          ((L_ANA_2k - L_DIV_2k)/2)
#define L_A             L_B
#define L_A_2k          L_B_2k
#define L_A_MAX         (L_A > ((L_A_2k * 32)/5) ? L_A : ((L_A_2k * 32)/5)) /* 384 */

#define N_COEFF_F2K     321
#define L_FDEL_64k      ((N_COEFF_F2K * 2 - 1) - 1)/2
#define L_FDEL          (L_FDEL_64k / 5)
#define L_FDEL_2k       (L_FDEL_64k / 32)

#define L_BSP           (2 * L_FDEL)         /* 128 */
#define L_BSP_2k        ((L_BSP*5)/32)

#define L_TOTAL_ST      (M+L_FRAME_PLUS+L_A_MAX+L_BSP)            /* 1552 total size of speech buffer in stereo mode */
#define L_TOTAL_ST_2k   (L_B_2k + L_FRAME_2k + L_A_2k + L_BSP_2k)
#define L_TOTAL_ST_hi   (L_B + L_FRAME_PLUS + L_A_MAX)

#define L_OLD_SPEECH             (L_TOTAL_PLUS-L_FRAME_PLUS)
#define L_OLD_SPEECH_HIGH_RATE   (L_TOTAL_HIGH_RATE-L_FRAME_PLUS)
#define L_OLD_SPEECH_ST          (L_TOTAL_ST-L_FRAME_PLUS)           /* 528 */
#define L_OLD_SPEECH_2k          (L_TOTAL_ST_2k-L_FRAME_2k)
#define L_OLD_SPEECH_hi          (L_TOTAL_ST_hi-L_FRAME_PLUS)

#define MAX_NUMSTAGES   6 /* Maximum number of stages in MSVQ. */
#define INTENS_LO       8
#define INTENS_HI       8
#define INTENS_MAX      8 //(INTENS_LO > INTENS_HI ? INTENS_LO : INTENS_HI)

#define HI_FILT_ORDER   9

#define L_INTERPOL  (16+1)

/* High stereo codebook */
// filter quantizers
#define NSTAGES_FILT_HI_MSVQ4 1
#define SIZE_FILT_HI_MSVQ_4A 16
#define INTENS_FILT_HI_MSVQ4  1

#define NSTAGES_FILT_HI_MSVQ7 2
#define SIZE_FILT_HI_MSVQ_7A 16
#define SIZE_FILT_HI_MSVQ_7B  8
#define INTENS_FILT_HI_MSVQ7  8

// gain quantizers
#define HI_GAIN_ORDER 2

#define NSTAGES_GAIN_HI_MSVQ2 1
#define SIZE_GAIN_HI_MSVQ_2A  4
#define INTENS_GAIN_HI_MSVQ2  1

#define NSTAGES_GAIN_HI_MSVQ5 1
#define SIZE_GAIN_HI_MSVQ_5A 32
#define INTENS_GAIN_HI_MSVQ5  1

/*FIP*/
#define LG10                24660       /*  10*log10(2)  in Q13                 */
#define LG10_28             17262       /*  28*log10(2)  in Q11                 */
#define Q_MAX               8
#define Q_MAX2              6     // 4
#define Q_MAX3              2     // 4

#define PREEMPH_FAC_FX  22282      /* preemphasis factor                         */
#define GAMMA1_FX       30147      /* weighting factor (numerator)               */
#define TILT_FAC_FX     22282
#define TILT_CODE_FX    9830       /* ACELP code preemphasis factor              */
#define PIT_SHARP_FX    27853      /* pitch sharpening factor                    */
#define GAMMA_HF_FX     29491      /* weighting factor for HF weghted speech     */

#define N_MAX       1152          /* maximum fft coefficients */
#define COSOFFSET   288           /* Offset to find cosine in table */

#define WIENER_ORDER       20 /* Filter order for wiener filters used in parametric stereo encoding. (Default value)*/
#define WIENER_HI_ORDER    WIENER_ORDER /* Filter order for wiener filters in high band (must be an even number) */
#define WIENER_LO_ORDER    WIENER_ORDER /* Filter order for wiener filters in low band (must be an even number) */
#define D_BPF (DELAY_PF+20)                     /* Bass postfilter delay used for stereo */
#define D_NC ((WIENER_LO_ORDER/2 * 32) / 5)     /* Non causality delay */

/**************** _CNST_TCX_STEREO_ ****************/
#define  D_STEREO_TCX   5
#define  L_OVLP_2k      32

#define  L_TCX_LB       192
#define  NPRM_RE8_D     (L_TCX_LB+(L_TCX_LB/8))
#define  NPRM_TCX80_D   (2+NPRM_RE8_D)          /* TCX 80ms */
#define  NPRM_TCX40_D   (2+(NPRM_RE8_D/2))      /* TCX 40ms */
#define  NPRM_TCX20_D   (2+(NPRM_RE8_D/4))      /* TCX 20ms */
#define  NPRM_DIV_D     (NPRM_TCX20_D)          /* buffer size = NB_DIV*NPRM_DIV */

#define  TOT_PRM_80     L_TCX_LB
#define  TOT_PRM_40     (L_TCX_LB/2)
#define  TOT_PRM_20     (L_TCX_LB/4)

#define  NPRM_DIV_TCX_STEREO     (NPRM_TCX20_D) /* buffer size = NB_DIV*NPRM_DIV */

#define NPRM_STEREO_HI_X         (2+1)          // maximum: 2 msvq stages (filt) + 1 vq (gain)
#define NPRM_STEREO_DIV_X        (NPRM_DIV_TCX_STEREO + NPRM_STEREO_HI_X + 1)
#define MAX_NPRM_STEREO_DIV      (NPRM_STEREO_DIV_X)
#define TCX_STEREO_DELAY_2k      (((D_BPF + L_BSP + 2*D_NC)*5/32) + D_STEREO_TCX)   /* 65 + 5 = 70 which is the available look back */
#define TCX_L_FFT_2k             (L_FRAME_2k + L_OVLP_2k) /*192*/

#define L_SUBFR_2k               10
#define ECU_WIEN_ORD             8

/* HF gain quantiser constants */
#define Q_GN_ORDER        4
#define SIZE_BK_HF        128
#define MEAN_GAIN_HF_FX   719  /*Q8*/

#define MSVQ_PERDICTCOEFF  16384

typedef struct {
   const Ipp16s vdim;
   const Ipp16s *mean; /* Mean vector */
   const Ipp16s nstages;
   const Ipp16s intens;
   const Ipp16s *cbsizes;
   const Ipp16s **cbs;
} sMSPQTab;

#define TWINLEN (24-8)
#define COMPLEN 12
#define LPHLEN 4
#define LPHAVELEN 8
typedef struct
{
  Ipp16s levelHist[TWINLEN][COMPLEN];
  Ipp16s averageHistTime[COMPLEN];
  Ipp16s stdDevHistTime[COMPLEN];
  Ipp16s averageHistTimeShort[COMPLEN];
  Ipp16s stdDevHistTimeShort[COMPLEN];
  Ipp16s lphBuf[LPHLEN];
  Ipp16s lphAveBuf[LPHAVELEN];
  Ipp16s prevModes[4];
  Ipp16s vadFlag[4];
  Ipp16s vadFlag_old[4];
  Ipp16s LTPLag[10];
  Ipp16s NormCorr[10];
  Ipp16s LTPGain[10];
  Ipp32s TotalEnergy[5];
  Ipp16s NoMtcx[2];
  Ipp16s NbOfAcelps;
  Ipp16s ApBuf[4 * M];
  Ipp16s lph[4];
  Ipp16s StatClassCount;
  Ipp16s LTPlagV[8];
} NCLASSDATA_FX;

typedef struct {
    /* FIP */
    Ipp16s mem_decim[L_MEM_DECIM_SPLIT];    /* speech decimated filter memory */
    Ipp16s decim_frac;

    IppsHighPassFilterState_AMRWB_16s* memSigIn; /* {mem_sig_in[6]} hp50 filter memory */

    Ipp16s mem_preemph;                   /* speech preemph filter memory */

    Ipp16s mem_decim_hf[2*L_FILT24k];     /* HF speech decimated filter memory */
    Ipp16s old_speech_hf[L_OLD_SPEECH_ST];/* HF old speech vector at 12.8kHz */

    /* cod_hf.c */
    Ipp16s past_q_isf_hf[MHF];            /* HF past quantized isf */
    Ipp16s ispold_hf[MHF];                /* HF old isp (immittance spectral pairs) */
    Ipp16s ispold_q_hf[MHF];              /* HF quantized old isp */
    Ipp16s old_gain;                      /* HF old gain (match amplitude at 6.4kHz) */
    Ipp16s mem_hf1[MHF];                  /* HF memory for gain calculcation */
    Ipp16s mem_hf2[MHF];                  /* HF memory for gain calculcation */
    Ipp16s mem_hf3[MHF];                  /* HF memory for gain calculcation */
    Ipp16s mem_lev_hf[18];
    Ipp16s old_exc[PIT_MAX_MAX];
    Ipp16s Q_sp_hf;
    Ipp16s OldQ_sp_hf[2];
    /* Table pointers */
    //const Ipp16s *mean_isf_hf;
    //const Ipp16s *dico1_isf_hf;
} EncoderOneChanelState_AMRWBE;

struct _EncoderObj_AMRWBE {
    /* memory for both channels */
    EncoderOneChanelState_AMRWBE right;
    EncoderOneChanelState_AMRWBE left;

    /* memory for the  stereo */

    /*FIP */
    Ipp16s  old_chan[L_OLD_SPEECH_ST];
    Ipp16s  old_chan_2k[L_OLD_SPEECH_2k];
    Ipp16s  old_chan_hi[L_OLD_SPEECH_hi];

    Ipp16s  old_speech_2k[L_OLD_SPEECH_2k];
    Ipp16s  old_speech_hi[L_OLD_SPEECH_hi];

    Ipp16s  old_speech_pe[L_OLD_SPEECH_ST];
    // NMBC
    Ipp16s old_wh[HI_FILT_ORDER];
    Ipp16s old_wh_q[HI_FILT_ORDER];
    Ipp16s old_gm_gain[2];
    Ipp16s old_exc_mono[HI_FILT_ORDER];
    Ipp16s filt_energy_threshold;
    Ipp16s w_window[L_SUBFR];
    const sMSPQTab *filt_hi_pmsvq;
    const sMSPQTab *gain_hi_pmsvq;
    // E_STEREO_TCX
    Ipp16s mem_stereo_ovlp_size;
    Ipp16s mem_stereo_ovlp[L_OVLP_2k];
    /* cod_main.c */
    Ipp16s old_ovlp_size;                   /* last tcx overlap size */
    // Memory of past gain for WB+ -> WB switching
    Ipp16s SwitchFlagPlusToWB;
    Ipp16s prev_mod;

    /* cod_main.c */
    NCLASSDATA_FX *_stClass;
    //VadVars *_vadSt;
    //VADStateVars* pVADState;
    IppsVADState_AMRWB_16s* pVADState;
    Ipp16s tone_flag;                      /* form VadVars */
    Ipp16s vad_hist;

    Ipp16s old_speech[L_OLD_SPEECH_ST];   /* old speech vector at 12.8kHz */
    Ipp16s old_synth[M];                  /* synthesis memory */

    /* cod_lf.c */
    Ipp16s past_isfq[M];                  /* past isf quantizer */
    Ipp16s old_wovlp[128];                /* last tcx overlap synthesis */
    Ipp16s old_d_wsp[PIT_MAX_MAX/OPL_DECIM];  /* Weighted speech vector */
    Ipp16s old_exc[PIT_MAX_MAX+L_INTERPOL];   /* old excitation vector */

    Ipp16s old_mem_wsyn;                  /* weighted synthesis memory */
    Ipp16s old_mem_w0;                    /* weighted speech memory */
    Ipp16s old_mem_xnq;                   /* quantized target memory */

    Ipp16s isfold[M];                     /* old isf (frequency domain) */
    Ipp16s ispold[M];                     /* old isp (immittance spectral pairs) */
    Ipp16s ispold_q[M];                   /* quantized old isp */
    Ipp16s mem_wsp;                       /* wsp vector memory */
    Ipp16s mem_lp_decim2[3];              /* wsp decimation filter memory */

    /* memory of open-loop LTP */
    Ipp16s ada_w;
    Ipp16s ol_gain;
    Ipp16s ol_wght_flg;

    Ipp16s old_ol_lag[5];
    Ipp16s old_T0_med;
    Ipp16s hp_old_wsp[L_FRAME_PLUS/OPL_DECIM+(PIT_MAX_MAX/OPL_DECIM)];

   //Ipp16s hp_ol_ltp_mem[9/* HP_ORDER*2 *//* 3*2+1*/];
   IppsHighPassFilterState_AMRWB_16s* memHPOLLTP;
   Ipp16s scaleHPOLLTP;                             /* scale for memHPOLLTP filter coefficients */
    // Mem_OL mem_ol;
    /* LP analysis window */
    Ipp16s window[L_WINDOW_HIGH_RATE];
    // Memory of past gain for WB+ -> WB switching
    Ipp16s mem_gain_code[4];
    // Ipp16s wprev_mod;
    Ipp16s Q_sp;
    Ipp16s OldQ_sp;
    Ipp16s i_offset;
    Ipp16s pit_max;
    Ipp16s lev_mem[18];
    Ipp16s old_wsp_max[4];
    Ipp16s old_wsp_shift;
    Ipp16s scale_fac;
    Ipp16s Q_new;           /* First scaling */
    Ipp16s Q_max[2];
    Ipp16s OldQ_sp_deci[2];
    Ipp16s Q_exc;
    Ipp16s Old_Qexc;
    Ipp16s LastQMode;
};

typedef struct {
   /* dec_main.c */
   Ipp16s wmem_oversamp[L_MEM_JOIN_OVER];     /* memory for core oversampling */
   Ipp16s wover_frac;
   Ipp16s wmem_oversamp_hf[2*L_FILT];     /* memory for HF oversampling */

   Ipp16s wpast_q_isf_hf[MHF];            /* HF past quantized isf */
   Ipp16s wpast_q_isf_hf_other[MHF];      /* HF past quantized isf for the other channel when mono decoding stereo */
   Ipp16s wpast_q_gain_hf;                /* HF past quantized gain */
   Ipp16s wpast_q_gain_hf_other;          /* HF past quantized gain for the other channel when mono decoding stereo */
   Ipp16s wold_gain;                      /* HF old gain (match amplitude at 6.4kHz) */
   Ipp16s wispold_hf[MHF];                /* HF old isp (immittance spectral pairs) */
   Ipp32s Lp_amp;                         /* HF memory for soft exc */
   Ipp32s Lthreshold;                     /* HF memory for smooth ener */
   Ipp16s wmem_syn_hf[MHF];               /* HF synthesis memory */

   Ipp16s wold_synth_hf[D_BPF +L_SUBFR+ L_BSP + 2*D_NC + L_FDEL + 32*D_STEREO_TCX/5];        /* HF memory for synchronisation */
   // NMBC
   Ipp16s wmem_d_nonc[D_NC];
   //Ipp16s wmem_sig_out[6];                /* hp50 filter memory for synthesis */
   IppsHighPassFilterState_AMRWB_16s* wmemSigOut;   /* hp50 filter memory for synthesis {wmem_sig_out[6]}*/

   Ipp16s Q_synHF;
   //const Ipp16s *Mean_isf_hf;
   //const Ipp16s *Dico1_isf_hf;

   /* stereo */
   Ipp16s mem_synth_hi_fx[M];
   Ipp16s mem_d_tcx_fx[D_NC+(D_STEREO_TCX*32/5)];
} DecoderOneChanelState_AMRWBE;

/* Memory structure for parametric stereo decoding states. */

struct _DecoderObj_AMRWBE {
   /* memory for both channels */
   DecoderOneChanelState_AMRWBE left;
   DecoderOneChanelState_AMRWBE right;

   /* memory for parametric stereo */
   Ipp16s mem_left_2k_fx[2*L_FDEL_2k];
   Ipp16s mem_right_2k_fx[2*L_FDEL_2k];
   Ipp16s mem_left_hi_fx[L_FDEL];
   Ipp16s mem_right_hi_fx[L_FDEL];

   Ipp16s my_old_synth_2k_fx[L_FDEL_2k + D_STEREO_TCX + 2*(D_NC*5)/32];
   Ipp16s my_old_synth_hi_fx[2*L_FDEL];
   Ipp16s my_old_synth_fx[2*L_FDEL+20];

   Ipp16s old_AqLF_fx[5*(M+1)];
   Ipp16s old_wh_fx[HI_FILT_ORDER];
   Ipp16s old_wh2_fx[HI_FILT_ORDER];
   Ipp16s old_exc_mono_fx[HI_FILT_ORDER];
   Ipp16s old_gain_left_fx[4];
   Ipp16s old_gain_right_fx[4];
   Ipp16s old_wh_q_fx[HI_FILT_ORDER];
   Ipp16s old_gm_gain_fx[2];
   Ipp16s W_window[L_SUBFR];
   const sMSPQTab *Filt_hi_pmsvq_fx;
   const sMSPQTab *Gain_hi_pmsvq_fx;

   Ipp16s mem_stereo_ovlp_size_fx;
   Ipp16s mem_stereo_ovlp_fx[L_OVLP_2k];
   Ipp16s last_stereo_mode;
   Ipp16s side_rms_fx;
   Ipp16s h_fx[ECU_WIEN_ORD+1];
   Ipp16s mem_balance_fx;

   Ipp16s wold_xri[L_TCX];
   /* memory for lower band (mono) */
   Ipp16s last_mode;                       /* last mode in previous 80ms frame */

   //Ipp16s wmem_sig_out[6];                /* hp50 filter memory for synthesis */
   IppsHighPassFilterState_AMRWB_16s* wmemSigOut;   /* hp50 filter memory for synthesis {wmem_sig_out[6]}*/

   Ipp16s wmem_deemph;                    /* speech deemph filter memory      */

   Ipp16s prev_lpc_lost;                   /* previous lpc is lost when = 1 */
   Ipp16s wold_synth[M];                  /* synthesis memory */
   Ipp16s wold_exc[PIT_MAX_MAX+L_INTERPOL];                /* old excitation vector (20ms) */

   Ipp16s wisfold[M];                     /* old isf (frequency domain) */
   Ipp16s wispold[M];                     /* old isp (immittance spectral pairs) */
   Ipp16s wpast_isfq[M];                  /* past isf quantizer */
   Ipp16s wwovlp[128];                    /* last weighted synthesis for overlap */
   Ipp16s ovlp_size;                       /* overlap size */
   Ipp16s wisf_buf[L_MEANBUF*(M+1)];      /* old isf (for frame recovery) */
   Ipp16s seed_ace;                      /* seed memory (for random function) */
   Ipp16s wmem_wsyn;                      /* TCX synthesis memory */
   Ipp16s seed_tcx;                      /* seed memory (for random function) */
   Ipp16s wwsyn_rms;                      /* rms value of weighted synthesis */
   Ipp16s wpast_gpit;                     /* past gain of pitch (for frame recovery) */
   Ipp32s Lpast_gcode;                    /* past gain of code (for frame recovery) */
   Ipp16s pitch_tcx;                       /* for bfi on tcx20 */
   Ipp32s L_gc_thres;

   Ipp16s siOldPitchLag;                         /* {wold_T0} old pitch value (for frame recovery) */
   Ipp16s siOldPitchLagFrac;                     /* {wold_T0_frac} old pitch value (for frame recovery) */

   Ipp16s wold_synth_pf[PIT_MAX_MAX+(2*L_SUBFR)];      /* bass post-filter: old synthesis  */
   Ipp16s wold_noise_pf[2*L_FILT];        /* bass post-filter: noise memory   */
   Ipp16s wold_T_pf[2];                        /* bass post-filter: old pitch      */
   Ipp16s wold_gain_pf[2];                   /* bass post-filter: old pitch gain */
   Ipp16s Old_bpf_scale;

   // For WB <-> WB+ switching
   Ipp16s wmem_gain_code[4];
   Ipp16s wmem_lpc_hf[MHF+1];
   Ipp16s wmem_gain_hf;
   Ipp16s wramp_state;
   Ipp16s cp_old_synth[M];

   /* FIP static */
   Ipp16s Q_old;
   Ipp16s Q_exc;
   Ipp16s Q_syn;
   Ipp16s Old_Q_syn;
   Ipp16s Old_Q_exc;
   Ipp16s prev_Q_syn;
   Ipp16s mem_syn2[M];
   Ipp16s Old_Qxnq;
   Ipp16s Old_QxnqMax;
   Ipp16s Old_Qxri;

   Ipp16s mem_subfr_q[7];
   Ipp16s old_subfr_q[16];
   Ipp16s old_syn_q[16];
   Ipp16s i_offset;
};

extern __ALIGN(16) CONST IppSpchBitRate tblMonoMode2MonoRate_WBE[8];

extern __ALIGN(16) CONST Ipp16s tblACoeffHP50[4];
extern __ALIGN(16) CONST Ipp16s tblBCoeffHP50[4];
extern __ALIGN(16) CONST Ipp16s tblACoeffHP50_DENORM[4];
extern __ALIGN(16) CONST Ipp16s tblBCoeffHP50_DENORM[4];
extern __ALIGN(16) CONST Ipp16s tblACoeff[4];
extern __ALIGN(16) CONST Ipp16s tblBCoeff[4];

extern __ALIGN(16) CONST Ipp16s tblISP_16s[M];
extern __ALIGN(16) CONST Ipp16s tblISPhf_16s[MHF];
extern __ALIGN(16) CONST Ipp16s tblISF_16s[M];

extern __ALIGN(16) CONST Ipp16s tblCosHF_16s[L_OVLP];
extern __ALIGN(16) CONST Ipp16s tblCosLF_16s[L_WINDOW_WBP/2];

extern __ALIGN(16) CONST Ipp16s tblInterpolTCX20_16s[NB_DIV];
extern __ALIGN(16) CONST Ipp16s tblInterpolTCX40_16s[2*NB_DIV];
extern __ALIGN(16) CONST Ipp16s tblInterpolTCX80_16s[4*NB_DIV];

extern __ALIGN(16) CONST Ipp16s tblSinCos_16s[N_MAX+COSOFFSET];

extern __ALIGN(16) CONST sMSPQTab mspqFiltLowBand;
extern __ALIGN(16) CONST sMSPQTab mspqFiltHighBand;
extern __ALIGN(16) CONST sMSPQTab mspqGainLowBand;
extern __ALIGN(16) CONST sMSPQTab mspqGainHighBand;

extern __ALIGN(16) CONST Ipp16s tblOverlap5ms_16s[63];
extern __ALIGN(16) CONST Ipp16s tblGainQuantWBE_16s[128*2];
extern __ALIGN(16) CONST Ipp16s tblTXV_16s[31];
extern __ALIGN(16) CONST Ipp16s tblRandPh16_16s[20];
extern __ALIGN(16) CONST Ipp16s tblLen_16s[6];

/* utils common */
void ownScaleHPFilterState(Ipp16s* pMemHPFilter, Ipp32s scale, Ipp32s order);
void ownIntLPC(const Ipp16s* pIspOld, const Ipp16s* pIspNew, const Ipp16s* pIntWnd,
   Ipp16s* pAz, Ipp32s nSubfr, Ipp32s m);
void ownFindWSP(const Ipp16s* Az, const Ipp16s* speech_ns, Ipp16s *wsp, Ipp16s *mem_wsp, Ipp32s lg);
Ipp16s Match_gain_6k4(Ipp16s *AqLF, Ipp16s *AqHF);
void Int_gain(Ipp16s old_gain, Ipp16s new_gain, const Ipp16s *Int_wind, Ipp16s *gain, Ipp32s nb_subfr);
void Q_gain_hf(Ipp16s *gain, Ipp16s *gain_q, Ipp16s *indice);
void D_gain_hf(Ipp16s indice, Ipp16s *gain_q, Ipp16s *past_q, Ipp32s bfi);
void Cos_window(Ipp16s *fh, Ipp32s n1, Ipp32s n2, Ipp32s inc2);
void Adap_low_freq_emph(Ipp16s* xri, Ipp32s lg);
void Adap_low_freq_deemph(Ipp16s* xri, Ipp32s lg);
void Scale_tcx_ifft(Ipp16s* exc, Ipp32s len, Ipp16s *Q_exc);
Ipp16s Bin2int(Ipp32s no_of_bits, Ipp16s *bitstream);
void Int2bin(Ipp16s value, Ipp32s no_of_bits, Ipp16s *bitstream);
void ownWindowing(const Ipp16s *pWindow, Ipp16s *pSrcDstVec, Ipp32s length);
void Fir_filt(Ipp16s* a, Ipp32s m, Ipp16s* x, Ipp16s* y, Ipp32s len);
void Apply_tcx_overlap(Ipp16s* xnq_fx, Ipp16s* wovlp_fx, Ipp32s lext, Ipp32s L_frame);
Ipp32s SpPeak1k6(Ipp16s* xri, Ipp16s *exp_m, Ipp32s len);

void ownLog2_AMRWBE(Ipp32s L_x, Ipp16s *logExponent, Ipp16s *logFraction);
void _Log2_norm (Ipp32s L_x, Ipp16s exp, Ipp16s *exponent, Ipp16s *fraction);
void _Isqrt_n(Ipp32s* frac, Ipp16s* exp);
Ipp32s _Isqrt(Ipp32s L_x);

/* utils encoder  */
Ipp16s ownScaleTwoChannels_16s(Ipp16s* pChannelLeft, Ipp16s* pChannelRight, Ipp32s len,
   Ipp16s *scaleCurr, Ipp16s *scalePrev, Ipp32s BitToRemove);
Ipp16s ownScaleOneChannel_16s(Ipp16s* pChannel, Ipp32s len,
   Ipp16s *scaleCurr, Ipp16s *scalePrev, Ipp32s BitToRemove);
void ownRescaleEncoderStereo(EncoderObj_AMRWBE *pObj, Ipp16s scaleSpch, Ipp16s scalePrmph);
void ownRescaleEncoderMono(EncoderObj_AMRWBE *pObj, Ipp16s scaleSpch, Ipp16s scalePrmph);
void ownMixChennels_16s(Ipp16s *pChennelLeft, Ipp16s *pChennelRight, Ipp16s *pChennelMix,
   Ipp32s len, Ipp16s wghtLeft, Ipp16s wghtRight);
void Preemph_scaled(Ipp16s* new_speech, Ipp16s *Q_new, Ipp16s *exp, Ipp16s *mem_preemph, Ipp16s *Q_max,
   Ipp16s *Q_old, Ipp16s Preemph_factor, Ipp16s bits, Ipp32s Lframe);
void InitClassifyExcitation(NCLASSDATA_FX *stClass);
Ipp16s ClassifyExcitation(NCLASSDATA_FX *stClass, const Ipp16s* pLevelNew, Ipp32s sfIndex);
void ClassifyExcitationRef(NCLASSDATA_FX *stClass, Ipp16s *ISPs_, Ipp16s *headMode);
void Enc_prm(Ipp16s* mod, Ipp32s codec_mode, Ipp16s* sparam, Ipp16s* serial, Ipp32s nbits_pack);
void Enc_prm_stereo_x(Ipp16s* sparam, Ipp16s* sserial, Ipp32s nbits_pack, Ipp32s nbits_bwe, Ipp32s brMode);
void Enc_prm_hf(Ipp16s* mod, Ipp16s* param, Ipp16s* serial, Ipp32s nbits_pack);
void Compute_exc_side(Ipp16s* exc_side ,Ipp16s* exc_mono);

void Smooth_ener_filter(Ipp16s filter[], Ipp16s *threshold);
void Quant_filt(Ipp16s h[], Ipp16s old_h[], Ipp16s **prm, const sMSPQTab *filt_hi_pmsvq);
void Compute_gain_match(Ipp16s *gain_left, Ipp16s *gain_right, Ipp16s x[], Ipp16s y[], Ipp16s buf[]);
void Quant_gain(Ipp16s gain_left, Ipp16s gain_right, Ipp16s old_gain[], Ipp16s **prm, const sMSPQTab *gain_hi_pmsvq);

void Comp_gain_shap_cod(Ipp16s* wm, Ipp16s* gain_shap_cod, Ipp16s* gain_shap_dec, Ipp32s len, Ipp16s Qwm);
void Apply_gain_shap(Ipp32s lg, Ipp16s* xnq_fx, Ipp16s* gain_shap_fx);
Ipp16s Q_gain_pan(Ipp16s *gain);
Ipp16s Compute_xn_target(Ipp16s *xn, Ipp16s *wm, Ipp16s gain_pan, Ipp32s len);
/* decoder */
Ipp32s Pack4bits(Ipp32s nbits, Ipp16s *ptr, Ipp16s *prm);
void Scale_mem2(Ipp16s Signal[], Ipp16s* Old_q, DecoderObj_AMRWBE *st, Ipp16s Syn);
void Delay(Ipp16s* pSignal, Ipp32s len, Ipp32s lenDelay, Ipp16s* pMem);
void Updt_mem_q(Ipp16s* old_sub_q, Ipp16s n, Ipp16s new_Q);
Ipp16s D_gain_chan(Ipp16s * gain_hf, Ipp16s *mem_gain, Ipp16s *prm, Ipp32s bfi, Ipp32s mode,
   Ipp32s *bad_frame, Ipp32s *Lgain, Ipp32s mono_dec_stereo);
void Comp_gain_shap(Ipp16s* wm, Ipp16s* gain_shap, Ipp32s lg, Ipp16s Qwm);
Ipp16s Balance(Ipp32s bad_frame, Ipp16s* mem_balance_fx,Ipp16s prm);
void Apply_xnq_gain2(Ipp32s len, Ipp16s wbalance, Ipp32s Lgain, Ipp16s* xnq_fx, Ipp16s* wm_fx, Ipp16s Q_syn);
void Apply_wien_filt(Ipp32s len, Ipp16s* xnq_fx, Ipp16s* h_fx, Ipp16s* wm_fx);
void Crosscorr_2(Ipp16s* vec1_fx,  Ipp16s* vec2_fx, Ipp16s* result_fx_h, Ipp16s* result_fx_l,
   Ipp32s length, Ipp32s minlag, Ipp32s maxlag, Ipp16s *shift_fx, Ipp32s crosscorr_type);
Ipp16s Glev_s(Ipp16s *b_fx, Ipp16s *Rh, Ipp16s *Rl, Ipp16s *Zh, Ipp16s *Zl, Ipp32s  m, Ipp16s shift);
void Dec_filt(Ipp16s *prm, Ipp16s wh[], Ipp16s old_wh[],
   Ipp32s bfi,const sMSPQTab *filt_hi_pmsvq);
void Dec_gain(Ipp16s *prm, Ipp16s *gain_left, Ipp16s *gain_right, Ipp16s *old_gain,
   Ipp32s bfi, const sMSPQTab* Gain_hi_pmsvq_fx);
Ipp16s Comp_hi_gain(Ipp16s gcode0);
void Get_exc_win(Ipp16s *exc_ch, Ipp16s *buf, Ipp16s *exc_mono, Ipp16s *side_buf, Ipp16s *win,
   Ipp16s* gain, Ipp32s N, Ipp32s doSum);
void Get_exc(Ipp16s *exc_ch, Ipp16s *exc_mono, Ipp16s *side_buf, Ipp16s gain, Ipp32s len, Ipp32s doSum);
Ipp16s D_gain2_plus(Ipp16s index, Ipp16s code[], Ipp16s lcode, Ipp16s *gain_pit, Ipp32s  *gain_code,
   Ipp32s bfi, Ipp16s mean_ener, Ipp16s *past_gpit, Ipp32s  *past_gcode);
Ipp16s Scale_exc(Ipp16s* exc, Ipp32s lenSubframe, Ipp32s L_gain_code, Ipp16s *Q_exc, Ipp16s *Q_exsub, Ipp32s len);
void Rescale_mem(Ipp16s *mem_syn2, DecoderObj_AMRWBE* st);
void Adapt_low_freq_deemph_ecu(Ipp16s* xri, Ipp32s len, Ipp16s Q_ifft, DecoderObj_AMRWBE* st);
void Reconst_spect(Ipp16s* xri, Ipp16s* old_xri, Ipp32s n_pack, Ipp32s* bfi, Ipp32s len,
   Ipp32s last_mode, Ipp16s Q_xri);
void NoiseFill(Ipp16s *xri, Ipp16s *buf, Ipp16s *seed_tcx, Ipp16s fac_ns, Ipp16s Q_ifft, Ipp32s len);
Ipp16s Find_mpitch(Ipp16s* xri, Ipp32s len);
void Scale_mem_tcx(Ipp16s* xnq, Ipp32s len, Ipp32s Lgain, Ipp16s* mem_syn, DecoderObj_AMRWBE *st);

/* Funs from AMMRWB modul */
extern void ownSoftExcitationHF(Ipp16s *exc_hf, Ipp32s *mem);
extern void ownSmoothEnergyHF(Ipp16s *HF, Ipp32s *threshold);
extern void ownScaleSignal_AMRWB_16s_ISfs(Ipp16s *pSrcDstSignal, Ipp32s   len, Ipp16s ScaleFactor);
extern Ipp32s ownPow2_AMRWB(Ipp16s valExp, Ipp16s valFrac);
extern void ownAutoCorr_AMRWB_16s32s(Ipp16s *pSrcSignal, Ipp32s valLPCOrder, Ipp32s *pDst,
   const Ipp16s* tblWindow, Ipp32s windowLen);
extern void ownLagWindow_AMRWB_32s_I(Ipp32s *pSrcDst, Ipp32s len);
extern void ownLPDecim2(Ipp16s* pSig, Ipp32s len, Ipp16s* pMem);



#define   AMRWB_CODECFUN(type,name,arg)                extern type name arg

__INLINE Ipp32s Mul2_Low_32s(Ipp32s x) { // x=Ipp16s*Ipp16s
   if(x < 0x40000000) {
      return (x << 1);
   } else {
      return IPP_MAX_32S;
   }
}
__INLINE Ipp32s Mpy_16s32s(Ipp16s wHi1, Ipp16s wLo1, Ipp16s wHi2, Ipp16s wLo2) {
   Ipp32s dwRes;
   dwRes = Mul2_Low_32s(wHi1*wHi2);
   dwRes = Add_32s(dwRes, Mul2_Low_32s((wHi1*wLo2)>>15));
   dwRes = Add_32s(dwRes, Mul2_Low_32s((wLo1*wHi2)>>15));
   return (dwRes);
}

#endif /* __OWNAMRWBE_H__ */
