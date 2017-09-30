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
// Purpose: G711 speech codec: Packet loss concealment.
//
*/

#include <ippsc.h>
#include "owng711.h"

/*
APPENDIX I. A high quality low-complexity algorithm for packet loss concealment.

Packet Loss Concealment (PLC) algorithms, also known as frame erasure concealment
algorithms, hide transmission losses in an audio system where the input signal is
encoded and packetized at a transmitter, sent over a network, and received at
a receiver that decodes the packet and plays out the output.
*/

#define PITCHDIFF       (PITCH_MAX - PITCH_MIN)
#define NDEC            2                     /* 2:1 decimation */
#define CORRLEN         160                   /* 20 ms correlation length */
#define CORRBUFLEN      (CORRLEN + PITCH_MAX) /* correlation buffer length */
#define CORRMINPOWER    250                   /* minimum power */
#define EOVERLAPINCR    32                    /* end OLA increment per frame, 4 ms */
#define FRAMESZ         80                    /* 10 ms at 8 KHz */
#define ATTENDIV        5                     /* attenuation factor per 10 ms frame */
#define G_SHIFT         14
#define G_SCALE         (1 << (G_SHIFT + 16))
#define SHIFT0          7

#define erasecnt      (PLC->_erasecnt)
#define poverlap      (PLC->_poverlap)
#define poffset       (PLC->_poffset)
#define pitch         (PLC->_pitch)
#define pitchblen     (PLC->_pitchblen)
#define pitchbufend   (PLC->_pitchbufend)
#define pitchbufstart (PLC->_pitchbufstart)
#define pitchbuf      (PLC->_pitchbuf)
#define lastq         (PLC->_lastq)
#define history       (PLC->_history)

static void scalespeech(G711_PLC_state *PLC, Ipp16s *out);
static void getfespeech(G711_PLC_state *PLC, Ipp16s *out, Ipp32s sz);
static void savespeech(G711_PLC_state *PLC, Ipp16s *s);
static Ipp32s  findpitch(G711_PLC_state *PLC);
static void overlapadd(Ipp16s *l, Ipp16s *r, Ipp16s *o, Ipp32s cnt);
static void overlapaddatend(G711_PLC_state *PLC, Ipp16s *s, Ipp16s *f, Ipp32s cnt);

void PLC_init(G711_PLC_state *PLC)
{
  ippsZero_8u((void*)PLC, sizeof(G711_PLC_state));
  erasecnt = 0;
  pitchbufend = &pitchbuf[HISTORYLEN];
  ippsZero_16s(history, HISTORYLEN);
}

/*
* Save a frames worth of new speech in the history buffer.
* Return the output speech delayed by POVERLAPMAX.
*/
void savespeech(G711_PLC_state *PLC, Ipp16s *s)
{
  Ipp16s *p_history = history;
  /* make room for new signal */
  ippsCopy_16s(&p_history[FRAMESZ], p_history, HISTORYLEN - FRAMESZ);
  /* copy in the new frame */
  ippsCopy_16s(s, &p_history[HISTORYLEN - FRAMESZ], FRAMESZ);
  /* copy out the delayed frame */
  //if(erasecnt==1) /* do delay when first erasure occurred (different to the ref code) */
  ippsCopy_16s(&p_history[HISTORYLEN - FRAMESZ - POVERLAPMAX], s, FRAMESZ);
}
/*
* A good frame was received and decoded.
* If right after an erasure, do an overlap add with the synthetic signal.
* Add the frame to history buffer.
*/
void PLC_addtohistory(G711_PLC_state *PLC, Ipp16s *s)
{
  if (erasecnt) {
    Ipp16s overlapbuf[FRAMESZ];
    /*
    * longer erasures require longer overlaps
    * to smooth the transition between the synthetic
    * and real signal.
    */
    Ipp32s olen = poverlap + (erasecnt - 1) * EOVERLAPINCR;
    if (olen > FRAMESZ)
      olen = FRAMESZ;
    getfespeech(PLC, overlapbuf, olen);
    overlapaddatend(PLC, s, overlapbuf, olen);
    erasecnt = 0;
  }
  savespeech(PLC, s);
}

/*
* Generate the synthetic signal.
* At the beginning of an erasure determine the pitch, and extract
* one pitch period from the tail of the signal. Do an OLA for 1/4
* of the pitch to smooth the signal. Then repeat the extracted signal
* for the length of the erasure. If the erasure continues for more than
* 10 ms, increase the number of periods in the pitchbuffer. At the end
* of an erasure, do an OLA with the start of the first good frame.
* The gain decays as the erasure gets longer.
*/

void PLC_dofe(G711_PLC_state *PLC, Ipp16s *out)
{
  if (erasecnt == 0) {
    ippsCopy_16s(history, pitchbuf, HISTORYLEN); /* get history */
    pitch = findpitch(PLC);                     /* find pitch */
    poverlap = pitch >> 2;                     /* OLA 1/4 wavelength */
    /* save original last poverlap samples */
    ippsCopy_16s(pitchbufend - poverlap, lastq, poverlap);
    poffset = 0;                     /* create pitch buffer with 1 period */
    pitchblen = pitch;
    pitchbufstart = pitchbufend - pitchblen;
    overlapadd(lastq, pitchbufstart - poverlap,
      pitchbufend - poverlap, poverlap);
    /* update last 1/4 wavelength in history buffer */
    ippsCopy_16s(pitchbufend - poverlap, &history[HISTORYLEN-poverlap],
      poverlap);
    getfespeech(PLC, out, FRAMESZ);               /* get synthesized speech */
  } else if (erasecnt == 1 || erasecnt == 2) {
    /* tail of previous pitch estimate */
    Ipp16s tmp[POVERLAPMAX];
    Ipp32s saveoffset = poffset;              /* save offset for OLA */
    getfespeech(PLC, tmp, poverlap);       /* continue with old pitchbuf */
    /* add periods to the pitch buffer */
    poffset = saveoffset;
    while (poffset > pitch)
      poffset -= pitch;
    pitchblen += pitch;                       /* add a period */
    pitchbufstart = pitchbufend - pitchblen;
    overlapadd(lastq, pitchbufstart - poverlap,
      pitchbufend - poverlap, poverlap);
    /* overlap add old pitchbuffer with new */
    getfespeech(PLC, out, FRAMESZ);
    overlapadd(tmp, out, out, poverlap);
    scalespeech(PLC, out);
  } else if (erasecnt > 5) {
    ippsZero_16s(out, FRAMESZ);
  } else {
    getfespeech(PLC, out, FRAMESZ);
    scalespeech(PLC, out);
  }
  erasecnt++;
  savespeech(PLC, out);
}

/*
* Estimate the pitch.
* j0 - first index of search
* j0 - last  index of search
* ndec - step of search
* l - pointer to first sample in last 20 ms of speech.
* r - points to the sample PITCH_MAX before l
*/
static Ipp32s findpitch_int(G711_PLC_state *PLC, Ipp32s j0, Ipp32s j1, Ipp32s ndec)
{
  Ipp32s  i, j, bestmatch;
  Ipp32s  bestcorr, bestcorr2, best_scl;
  Ipp32s  corr, corr2, energy, scale;
  Ipp32s  energy1, energy2;
  Ipp32s  e, e0, e1, e2;
  Ipp32s  t0, t1, minpower;
  Ipp32s  ec, es, ec0, es0;
  Ipp16s  *r = pitchbufend - CORRBUFLEN;
  Ipp16s  *l = r + PITCH_MAX;
  Ipp16s  *rp;

  if (j0 < 0) {
    j0 = 0;
  }
  if (j1 > PITCHDIFF) {
    j1 = PITCHDIFF;
  }

  /* estimate energy */
  energy1 = (1 << 16);
  energy2 = (1 << 16);
  for (i = 0; i < CORRLEN; i += ndec) {
    energy1 += (l[i] * l[i]) >> SHIFT0;
  }
  for (i = j0; i <= j1 + CORRLEN; i += ndec) {
    energy2 += (r[i] * r[i]) >> SHIFT0;
  }
  e2 = SHIFT0 - Exp_32s_Pos(energy2);
  e = 2*SHIFT0 + 1 - Exp_32s_Pos((energy1 >> 16)*(energy2 >> 16));
  if (e < 0) e = 0;
  e = (e + 1)/2;
  if (e < e2) e = e2;

  rp = r + j0;
  minpower = CORRMINPOWER >> e;
  energy = 0;
  corr = 0;
  for (i = 0; i < CORRLEN; i += ndec) {
    energy += (rp[i] * rp[i]) >> e;
    corr += (rp[i] * l[i]) >> e;
  }
  scale = energy;
  if (scale < minpower)
    scale = minpower;
  bestcorr = corr;
  best_scl = scale;
  ec0 = Norm_32s_I(&bestcorr);
  es0 = Norm_32s_I(&best_scl);
  bestcorr2 = (((bestcorr >> 16)*(bestcorr >> 16)) >> 16);
  if (bestcorr < 0) bestcorr2 = -bestcorr2;
  bestmatch = j0;
  for (j = j0 + ndec; j <= j1; j += ndec) {
    energy -= (rp[0] * rp[0]) >> e;
    energy += (rp[CORRLEN] * rp[CORRLEN]) >> e;
    rp += ndec;
    corr = 0;
    for (i = 0; i < CORRLEN; i += ndec)
      corr += (rp[i] * l[i]) >> e;
    scale = energy;
    if (scale < minpower)
      scale = minpower;
    ec = Norm_32s_I(&corr);
    es = Norm_32s_I(&scale);
    corr2 = (((corr >> 16)*(corr >> 16)) >> 16);
    if (corr < 0) corr2 = -corr2;
    t0 = corr2*(best_scl >> 16);
    e0 = 2*ec + es0;
    t1 = bestcorr2*(scale >> 16);
    e1 = 2*ec0 + es;
    if (e0 < e1) {
      t1 >>= (e1 - e0);
    } else {
      t0 >>= (e0 - e1);
    }
    if (t0 >= t1) {
    /*if (fabs(corr)*corr*best_scl >= fabs(bestcorr)*bestcorr*scale) {*/
        bestcorr2 = corr2;
        best_scl = scale;
        ec0 = ec;
        es0 = es;
        bestmatch = j;
    }
  }
  return bestmatch;
}

/*
* Estimate the pitch.
*/
Ipp32s findpitch(G711_PLC_state *PLC)
{
  Ipp32s i;
  i = findpitch_int(PLC, 0, PITCHDIFF, 2);
  i = findpitch_int(PLC, i - 1, i + 1, 1);
  return PITCH_MAX - i;
}

/*
* Get samples from the circular pitch buffer. Update poffset so
* when subsequent frames are erased the signal continues.
*/
void getfespeech(G711_PLC_state *PLC, Ipp16s *out, Ipp32s sz)
{
  while (sz) {
    Ipp32s cnt = pitchblen - poffset;
    if (cnt > sz)
      cnt = sz;
    ippsCopy_16s(&pitchbufstart[poffset], out, cnt);
    poffset += cnt;
    if (poffset == pitchblen)
      poffset = 0;
    out += cnt;
    sz -= cnt;
  }
}

void scalespeech(G711_PLC_state *PLC, Ipp16s *out)
{
  Ipp32s g = G_SCALE - (erasecnt - 1)*(G_SCALE/ATTENDIV);
  Ipp32s i;
  for (i=0; i < FRAMESZ; i++) {
    out[i] = (Ipp16s)(((Ipp32s)out[i] * (g >> G_SHIFT)) >> 16);
    g -= G_SCALE/(ATTENDIV*FRAMESZ);
  }
}

/*
* Overlap add left and right sides
*/
void overlapadd(Ipp16s *l, Ipp16s *r, Ipp16s *o, Ipp32s cnt)
{
  Ipp32s incr = G_SCALE / cnt;
  Ipp32s lw = G_SCALE - incr;
  Ipp32s rw = incr;
  Ipp32s i;
  for (i=0; i<cnt; i++) {
    Ipp32s t = (lw >> 16) * l[i] + (rw >> 16) * r[i];
    t = (t + (1 << (G_SHIFT - 1))) >> G_SHIFT;
    if (t > 32767)
      t = 32767;
    else if (t < -32768)
      t = -32768;
    o[i] = (Ipp16s)t;
    lw -= incr;
    rw += incr;
  }
}

/*
* Overlap add the end of the erasure with the start of the first good frame
* Scale the synthetic speech by the gain factor before the OLA.
*/
void overlapaddatend(G711_PLC_state *PLC, Ipp16s *s, Ipp16s *f, Ipp32s cnt)
{
  Ipp32s incr = G_SCALE / cnt;
  Ipp32s gain = (1 << 16) - (erasecnt - 1)*(1 << 16)/ATTENDIV;
  Ipp32s incrg, lw, rw;
  Ipp32s i;

  if (gain < 0)
    gain = 0;
  incrg = (incr >> 16) * gain;
  lw = (gain << G_SHIFT) - incrg;
  rw = incr;
  for (i=0; i<cnt; i++) {
    Ipp32s t = (lw >> 16) * f[i] + (rw >> 16) * s[i];
    t = (t + (1 << (G_SHIFT - 1))) >> G_SHIFT;
    if (t > 32767)
      t = 32767;
    else if (t < -32768)
      t = -32768;
    s[i] = (Ipp16s)t;
    lw -= incrg;
    rw += incr;
  }
}
