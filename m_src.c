/*****************************************************************************************/
/**
@brief Sample rate converter module for wideband AMR

The filtering is performed using a 71st order lowpass FIR filter. FIR filter is linked
from the misc_math library. Before processing, the signal is down scaled by a factor of
four to prevent overflow. After the processing, the original signal level is restored.

*/

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include "m_src.h"

#include <l_scratch.h>
#include "fir_filter_blk4.h"

#include <string.h>
#include <stdlib.h> // for malloc/free

/*****************************************************************************************/
/****    2 LOCAL CONSTANTS    ************************************************************/

/** Filter length */
#define FILTER_LENGTH_TAPS 72
/** Delay buffer length for upsampling */
#define DELAY_BUF_UP_SIZE_16 ((FILTER_LENGTH_TAPS / 2) - 1)
/** Delay buffer length for downsampling */
#define DELAY_BUF_DOWN_SIZE_16 (FILTER_LENGTH_TAPS - 1)
/** Number of PCM samples in 30 ms at 16 kHz sample rate */
#define PCM_30_MS_16_KHZ_SIZE_SAMPLES  480
/** Number of bits the signal is scaled down for the time of the processing */
#define SCALING_BITS 2
/** Filter delay line scratch size in 16-bit words */
#define DLINE_SCRATCH_SIZE_16 (PCM_30_MS_16_KHZ_SIZE_SAMPLES + FILTER_LENGTH_TAPS)
/** Filter output vector scratch size in 16-bit words */
#define OUTVEC_SCRATCH_SIZE_16 PCM_30_MS_16_KHZ_SIZE_SAMPLES

/** Filter coefficients for interpolation (upsampling), must be 64-bit aligned for
    fir_blk4(..) */
MGW_LOCAL const int16 COEFF_INT[FILTER_LENGTH_TAPS] =
  {-14, -54, -38, 56, 86, -42,
   -141,  3, 196, 67, -240, -171,
   258, 307, -234, -469, 151, 643,
   8, -810, -258, 943, 612, -1008,
   -1085, 962, 1696, -741, -2495, 233,
   3616, 842, -5551, -3624, 11544, 27472,
   27472, 11544, -3624, -5551, 842, 3616,
   233, -2495, -741, 1696, 962, -1085,
   -1008, 612, 943, -258, -810, 8,
   643, 151, -469, -234, 307, 258,
   -171, -240, 67, 196, 3, -141,
   -42, 86, 56, -38, -54, -14};

/** Filter coefficients for decimation (downsampling), must be 64-bit aligned for
    fir_blk4(..) */
MGW_LOCAL const int16 COEFF_DEC[FILTER_LENGTH_TAPS] =
  {-7, -27, -19, 28, 43, -21,
   -71, 1, 98, 33, -120, -85,
   129, 153, -117, -234, 76, 322,
   4, -405, -129, 472, 306, -504,
   -542, 481, 848, -371, -1248, 117,
   1808, 421, -2776, -1812, 5772, 13736,
   13736, 5772, -1812, -2776, 421, 1808,
   117, -1248, -371, 848, 481, -542,
   -504, 306, 472, -129, -405, 4,
   322, 76, -234, -117, 153, 129,
   -85, -120, 33, 98, 1, -71,
   -21, 43, 28, -19, -27, -7};

/*****************************************************************************************/
/****    3 LOCAL MACROS    ***************************************************************/

/** Converts 8 kHz size to 16 kHz size */
#define SIZE_8_KHZ_TO_16_KHZ(size_8_khz) (2 * (size_8_khz))

#define shl(a,b) (int16)(a<<b)
#define shr(a,b) (int16)(a>>b)

/*****************************************************************************************/
/****    4 LOCAL DATA TYPES    ***********************************************************/

/**
@brief      Downsampler scratch
*/
struct down_scratch {
  /** Filter delay line scratch buffer, must be 64-bit aligned for fir_blk4(..) */
  int16 dline[DLINE_SCRATCH_SIZE_16];
  /** Filter output vector scratch buffer */
  int16 outvec[OUTVEC_SCRATCH_SIZE_16];
};

/**
@brief      Internal module data structure
*/
struct m_src_up_internal {
  /** Filter state that has to be saved between the consecutive calls */
  int16 delaybuffer[DELAY_BUF_UP_SIZE_16];
  /** Filter delay line scratch buffer, must be 64-bit aligned for fir_blk4(..) */
  int16 *dline_scratch;
};

/**
@brief      Internal module data structure
*/
struct m_src_down_internal {
  /** Filter state that has to be saved between the consecutive calls */
  int16 delaybuffer[DELAY_BUF_DOWN_SIZE_16];
  /** Scratch memory */
  struct down_scratch *scratch;
};

/*****************************************************************************************/
/****    5 LOCAL FUNCTION PROTOTYPES    **************************************************/

MGW_LOCAL bool16
is_8_khz_upd_packet_ok(upd_packet_t *upd);

MGW_LOCAL bool16
is_16_khz_upd_packet_ok(upd_packet_t *upd);

MGW_LOCAL void
upscale(int16 *inbuf, int16 *outbuf, int16 length);

MGW_LOCAL void
downscale(int16 *buf, int16 length);

/*****************************************************************************************/
/****    6 FUNCTIONS    ******************************************************************/

/*****************************************************************************************/
/**
@brief      Initialises the module for upsampling

Initialises the upsampling module.

@param      master Module master structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_src_up_init(m_src_up_data *master)
{
  struct m_src_up_internal *internal = NULL;

  if (master == NULL) {
    return E_M_SRC_NULL_POINTER;
  }

  internal = (struct m_src_up_internal *)malloc(sizeof(*internal));

  internal->dline_scratch =
    l_scratch_get_buffer(L_SCRATCH_ID_SRC_C6X, DLINE_SCRATCH_SIZE_16);

  master->internal = internal;

  return EA_NO_ERRORS;
}

/*****************************************************************************************/
/**
@brief      Initialises the module for downsampling

Initialises the downsampling module.

@param      master Module master structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_src_down_init(m_src_down_data *master)
{
  struct m_src_down_internal *internal = NULL;

  if (master == NULL) {
    return E_M_SRC_NULL_POINTER;
  }

  internal = (struct m_src_down_internal *)malloc(sizeof(*internal));

  internal->scratch =
    l_scratch_get_buffer(L_SCRATCH_ID_SRC_C6X, SIZE_AU_TO_16(sizeof(struct down_scratch)));

  master->internal = internal;

  return EA_NO_ERRORS;
}

/*****************************************************************************************/
/**
@brief      Main function

Upsamples the linear PCM signal with the factor of two.

@param      master Module master structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_src_up(m_src_up_data *master)
{
  struct m_src_up_internal *internal = NULL;
  int16 *payload_out = NULL;
  int16 *payload_in = NULL;
  int16 *dline = NULL;
  int16 *delaybuffer = NULL;
  uint16 pcm_8_khz_size_samples = 0;
  uint32 i = 0;
  uint32 j = 0;

  if ((master == NULL) || (master->internal == NULL) ||
      (master->upd_in == NULL) || (master->upd_out == NULL)) {
    return E_M_SRC_NULL_POINTER;
  }

  if (is_8_khz_upd_packet_ok(master->upd_in) == FALSE) {
    master->extra_ecode = l_upd_generate_extra_ecode_32(master->upd_in);
    return E_M_SRC_UP_UPD_IN;
  }

  master->extra_ecode = 0;

  internal = (struct m_src_up_internal *) master->internal;

  pcm_8_khz_size_samples = l_upd_get_pcm_size_samples(master->upd_in->mode);
  payload_out = l_upd_get_payload(master->upd_out);
  payload_in = l_upd_get_payload(master->upd_in);
  dline = internal->dline_scratch;
  delaybuffer = internal->delaybuffer;

  downscale(payload_in, pcm_8_khz_size_samples);

  j = 0;
  for (i = 0; i < DELAY_BUF_UP_SIZE_16; i++) {
    dline[j++] = 0;
    dline[j++] = delaybuffer[i];
  }

  dline[j++] = 0;

  for (i = 0; i < pcm_8_khz_size_samples; i++) {
    dline[j++] = payload_in[i];
    dline[j++] = 0;
  }

  /*lint -e{676} "Possibly negative subscript..." */
  (void) memcpy(delaybuffer,
                &payload_in[pcm_8_khz_size_samples - DELAY_BUF_UP_SIZE_16],
                SIZE_16_TO_AU(DELAY_BUF_UP_SIZE_16));

  fir_blk4(payload_out, dline, SIZE_8_KHZ_TO_16_KHZ(pcm_8_khz_size_samples),
           FILTER_LENGTH_TAPS, COEFF_INT, 0);

  upscale(payload_out, payload_out, SIZE_8_KHZ_TO_16_KHZ(pcm_8_khz_size_samples));

  master->upd_out->codec = UPD_CODEC_PCM;
  master->upd_out->type = UPD_TYPE_SPEECH_GOOD;
  master->upd_out->mode = master->upd_in->mode;
  master->upd_out->form = UPD_FORM_PCM_LINEAR_16_KHZ;

  return EA_NO_ERRORS;
}

/*****************************************************************************************/
/**
@brief      Checks whether 8 kHz linear PCM UPD packet is OK

@param      upd UPD packet
@return     TRUE if UPD packet is OK, otherwise FALSE
*/
MGW_LOCAL bool16
is_8_khz_upd_packet_ok(upd_packet_t *upd)
{
  return ((upd->codec == UPD_CODEC_PCM) &&
          (upd->type == UPD_TYPE_SPEECH_GOOD) &&
          ((upd->mode == UPD_MODE_PCM_05_MS) ||
           (upd->mode == UPD_MODE_PCM_10_MS) ||
           (upd->mode == UPD_MODE_PCM_20_MS) ||
           (upd->mode == UPD_MODE_PCM_30_MS)) &&
          (upd->form == UPD_FORM_PCM_LINEAR_08_KHZ));
}

/*****************************************************************************************/
/**
@brief      Main function for downsampling

Downsamples the linear PCM signal with the factor of two.

@param      master Module master structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_src_down(m_src_down_data *master)
{
  struct m_src_down_internal *internal = NULL;
  int16 *payload_out = NULL;
  int16 *payload_in = NULL;
  int16 *outvec = NULL;
  int16 *dline = NULL;
  uint16 pcm_8_khz_size_samples = 0;
  uint32 i = 0;
  uint32 j = 0;

  if ((master == NULL) || (master->internal == NULL) ||
      (master->upd_in == NULL) || (master->upd_out == NULL)) {
    return E_M_SRC_NULL_POINTER;
  }

  if (is_16_khz_upd_packet_ok(master->upd_in) == FALSE) {
    master->extra_ecode = l_upd_generate_extra_ecode_32(master->upd_in);
    return E_M_SRC_DOWN_UPD_IN;
  }

  master->extra_ecode = 0;

  internal = (struct m_src_down_internal *) master->internal;

  pcm_8_khz_size_samples = l_upd_get_pcm_size_samples(master->upd_in->mode);
  payload_out = l_upd_get_payload(master->upd_out);
  payload_in = l_upd_get_payload(master->upd_in);
  outvec = internal->scratch->outvec;
  dline = internal->scratch->dline;

  (void) memcpy(dline,
                internal->delaybuffer,
                SIZE_16_TO_AU(DELAY_BUF_DOWN_SIZE_16));

  (void) memcpy(&dline[DELAY_BUF_DOWN_SIZE_16],
                payload_in,
                SIZE_16_TO_AU(SIZE_8_KHZ_TO_16_KHZ(pcm_8_khz_size_samples)));

  (void) memcpy(internal->delaybuffer,
                &payload_in[SIZE_8_KHZ_TO_16_KHZ(pcm_8_khz_size_samples) -
                            DELAY_BUF_DOWN_SIZE_16],
                SIZE_16_TO_AU(DELAY_BUF_DOWN_SIZE_16));

  downscale(dline, DELAY_BUF_DOWN_SIZE_16 + SIZE_8_KHZ_TO_16_KHZ(pcm_8_khz_size_samples));

  fir_blk4(outvec, dline, SIZE_8_KHZ_TO_16_KHZ(pcm_8_khz_size_samples), FILTER_LENGTH_TAPS,
           COEFF_DEC, 0);

  for ((i = 0), (j = 0); i < pcm_8_khz_size_samples; (i++), (j += 2)) {
    payload_out[i] = outvec[j];
  }

  upscale(payload_out, payload_out, pcm_8_khz_size_samples);

  master->upd_out->codec = UPD_CODEC_PCM;
  master->upd_out->type = UPD_TYPE_SPEECH_GOOD;
  master->upd_out->mode = master->upd_in->mode;
  master->upd_out->form = UPD_FORM_PCM_LINEAR_08_KHZ;

  return EA_NO_ERRORS;
}

/*****************************************************************************************/
/**
@brief      Checks whether 16 kHz linear PCM UPD packet is OK

@param      upd UPD packet
@return     TRUE if UPD packet is OK, otherwise FALSE
*/
MGW_LOCAL bool16
is_16_khz_upd_packet_ok(upd_packet_t *upd)
{
  return ((upd->codec == UPD_CODEC_PCM) &&
          (upd->type == UPD_TYPE_SPEECH_GOOD) &&
          ((upd->mode == UPD_MODE_PCM_05_MS) ||
           (upd->mode == UPD_MODE_PCM_10_MS) ||
           (upd->mode == UPD_MODE_PCM_20_MS) ||
           (upd->mode == UPD_MODE_PCM_30_MS)) &&
          (upd->form == UPD_FORM_PCM_LINEAR_16_KHZ));
}

/*****************************************************************************************/
/**
@brief      Releases the upsampling module

Release module internal data and set module state to A_INACTIVE.

@param      master Module master structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_src_up_release(m_src_up_data *master)
{
  ecode_t ecode = EA_NO_ERRORS;

  if ((master == NULL) || (master->internal == NULL)) {
    ecode = E_M_SRC_NULL_POINTER;
  } else {
    free(master->internal);
    master->internal = NULL;
  }

  return ecode;
}

/*****************************************************************************************/
/**
@brief      Releases the downsampling module

@param      master Module master structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_src_down_release(m_src_down_data *master)
{
  ecode_t ecode = EA_NO_ERRORS;

  if ((master == NULL) || (master->internal == NULL)) {
    ecode = E_M_SRC_NULL_POINTER;
  } else {
    free(master->internal);
    master->internal = NULL;
  }

  return ecode;
}

/*****************************************************************************************/
/**
@brief      Shifts the signal SCALING_BITS to left

Shifts the signal SCALING_BITS to left.

@param      inbuf Input signal
@param      outbuf Output signal
@param      length Length of the signal in samples
*/
MGW_LOCAL void
upscale(int16 *inbuf, int16 *outbuf, int16 length)
{
  int32 i = 0;

  for (i = 0; i < length; i++) {
    outbuf[i] = shl(inbuf[i], SCALING_BITS);
  }
}

/*****************************************************************************************/
/**
@brief      Shifts the signal SCALING_BITS to right

Shifts the signal SCALING_BITS to right.

@param      buf input signal
@param      length length of the signal in samples
*/
MGW_LOCAL void
downscale(int16 *buf, int16 length)
{
  int32 i = 0;

  for (i = 0; i < length; i++) {
    buf[i] = shr(buf[i], SCALING_BITS);
  }
}

