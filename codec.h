/*****************************************************************************************/
/**
@brief Generic codec interface for MGW
*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/

#ifndef CODEC_H
#define CODEC_H

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include <type_def.h>
#include "l_upd.h"

/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

struct evs_codec_enc_ctrl_io {
  bf8 evs_fec_enable       : 1; /**< FEC indicator */
  bf8 evs_fec_offset       : 3; /**< FEC offset */
  bf8 evs_mode             : 4; /**< evs primary mode or amr-wb io mode*/
};

union codec_enc_ctrl_io_t {
  struct evs_codec_enc_ctrl_io  evs_enc_ctrl_io;
};

/**
@brief      Encoder input/output control parameters

Requested mode for the next encoded frame is set to mode parameter.

dtx_enabled and no_data_enabled parameters control discontinuos transmission (DTX).
no_data_enabled parameter controls the usage of 'no data' frames in GSM EFR and GSM FR
encoders i.e. if disabled encoder produces only either speech or SID frames.

va_detected parameter holds external voice activity detection (vad) decission. If voice
activity has been detected parameter is set to TRUE, otherwise parameter is set to FALSE.
If no external VAD mechanism is available parameter is set to TRUE.

tone_detected parameter is set to TRUE, if encoder detects a tone signal, otherwise it is
set to FALSE. If parameter is not valid for the encoder, parameter is always set to
FALSE.

codec_ctrl_io holds the code specific encoder input/output control parameters.
*/
struct codec_enc_ctrl_io {
  union codec_enc_ctrl_io_t codec_ctrl_io; /**< Codec specific control input/output */
  uint16 mode;                   /**< Input: requested mode (#upd_mode_t) */
  bf16 dtx_enabled          : 1; /**< Input: TRUE if DTX operation enabled */
  bf16 enhanced_vad_enabled : 1; /**< Input: TRUE if enhanced VAD used */
  bf16 va_detected          : 1; /**< Input: TRUE if voice activity detected */
  bf16 tone_detected        : 1; /**< Output: TRUE if tone signal detected */
  bf16 spare                : 12;/**< Spare (align to uint8) */
};

/**
@brief      Encoder input/output data parameters
*/
struct codec_enc_data_io {
  upd_packet_t *upd_lin_pcm;     /**< Input: UPD packet for linear PCM */
  upd_packet_t *upd_out_codec;   /**< Output: UPD packet for codec parameters */
};

/**
@brief      Decoder input/output control parameters

output_sample_rate_hz Requested sample rate of the decoder's output linear PCM given in Hz
*/
struct codec_dec_ctrl_io {
  uint16 output_sample_rate_hz; /**< Output: Sample rate */
};

/**
@brief      Decoder input/output data parameters
*/
struct codec_dec_data_io {
  upd_packet_t *upd_input_codec;   /**< Input: UPD packet for codec parameters */
  upd_packet_t *upd_lin_pcm;       /**< Output: UPD packet for linear PCM */
};

typedef struct {
  void                        *internal;     /**< Module internal data */
  uint32                      extra_ecode;   /**< Extra error code */
  struct codec_enc_ctrl_io    ctrl_io;       /**< Control input/output */
  struct codec_enc_data_io    data_io;       /**< Data input/output */
} codec_enc_data_t;

typedef struct {
  void                        *internal;     /**< Module internal data */
  uint32                      extra_ecode;   /**< Extra error code */
  struct codec_dec_ctrl_io    ctrl_io;       /**< Control input/output */
  struct codec_dec_data_io    data_io;       /**< Data input/output */
} codec_dec_data_t;

/*****************************************************************************************/
/****    5 FUNCTION PROTOTYPES    ********************************************************/

/*****************************************************************************************/
#endif /* CODEC_H */



