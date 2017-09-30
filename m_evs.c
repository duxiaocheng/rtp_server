/*****************************************************************************************/
/**
@brief EVS codec module

*/

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include "m_evs.h"
#include <type_def.h>
#include <log_print.h>
#include <l_mgw.h>

#include <usc.h>
#include <ippcore.h>
#include <ipps.h>
#include <ippsc.h>

#include <string.h>

/*****************************************************************************************/
/****    X PRAGMAS    ********************************************************************/

/*****************************************************************************************/
/****    2 LOCAL CONSTANTS    ************************************************************/

#define  EVS_ENC_NBUFS 1 /**< Number of buffers for EVS encoder */
#define  EVS_DEC_NBUFS 1 /**< Number of buffers for EVS decoder */

#define  EVS_NUM_RATES  21
#define  EVS_SAMPLE_FREQUENCY_8  8000
#define  EVS_SAMPLE_FREQUENCY_16 16000
#define  EVS_SAMPLE_FREQUENCY_32 32000
#define  EVS_SAMPLE_FREQUENCY_48 48000
#define  EVS_BITS_PER_SAMPLE 16
#define  BITS_PER_PCM_SAMPLE 16 /**< Bits per linear PCM sample */
#define  CHANNELS_MONO 1 /**< Number of channels */
#define  TRUNCATE_ENABLED 1 /**< Truncate PCM to 14-bit samples (instead of 16-bit) */

/*****************************************************************************************/
/****    3 LOCAL MACROS    ***************************************************************/

/*****************************************************************************************/
/****    4 LOCAL DATA TYPES    ***********************************************************/

/**
@brief      Encoder internal data
*/
struct m_internal_evs_enc_data {
  char *pMem;
  USC_Handle enc_handle;
  USC_CodecInfo codec_info;
};

/**
@brief      Decoder internal data
*/
struct m_internal_evs_dec_data {
  char *pMem;
  USC_Handle dec_handle;
  uint16 dec_frames_numb;     ///< number of frames after init
};

/*****************************************************************************************/
/****    X LOCAL VARIABLES   *************************************************************/

MGW_LOCAL int primary_mode_br[] = {
  0, 5900, 7200, 8000, 9600, 13200, 16400,
  24400, 32000, 48000, 64000, 96000,128000
};

MGW_LOCAL int wbio_mode_br[] = {
  0, 6600, 8850, 12650, 14250, 15850, 18250, 19850,
  23050, 23850
};
/*****************************************************************************************/
/****    X EXTERN VARIABLES   *************************************************************/

extern USC_Fxns USC_EVS_Fxns;

/*****************************************************************************************/
/****    5 LOCAL FUNCTION PROTOTYPES    **************************************************/

MGW_LOCAL int32
frame_type_upd_packet_to_ipp(upd_type_t frame_type);

MGW_LOCAL int
get_evs_bitrate(uint16 evs_mode, upd_mode_t mode);

MGW_LOCAL int
evs_primary_mode_br(upd_mode_t mode);

MGW_LOCAL int
evs_wbio_mode_br(upd_mode_t mode);

MGW_LOCAL upd_type_t
map_type_from_usc_to_upd(int16 usc_type);

/*****************************************************************************************/
/****    6 FUNCTIONS    ******************************************************************/

/*****************************************************************************************/

/*****************************************************************************************/
/**
@brief      EVS encoder  module initialisation function

@param      evs_enc Generic encoder structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_evs_enc_init(codec_enc_data_t *evs_enc)
{
  struct m_internal_evs_enc_data *internal = NULL;

  USC_Fxns *fxns = &USC_EVS_Fxns;
  USC_Status ret_val = USC_NoError;
  USC_CodecInfo enc_params = { 0 };
  USC_MemBank mem_bank[EVS_ENC_NBUFS] = { {0} };
  int32 nbufs = 0;

  if (evs_enc == NULL) {
    return E_M_EVS_UNEXPECTED_NULL_POINTER;
  }
  evs_enc->extra_ecode = EA_NO_ERRORS;

  ret_val = fxns->std.GetInfo(NULL, &enc_params);
  if (ret_val != USC_NoError) {
    evs_enc->extra_ecode = (uint32) ret_val;
    return E_M_EVS_ENC_ALG_ALLOC_FAIL;
  }
  enc_params.params.direction = USC_ENCODE;
  //enc_params.params.modes.bitrate = 24400;

  ret_val = fxns->std.NumAlloc(&enc_params.params, &nbufs);
  if ((ret_val != USC_NoError) && (nbufs != EVS_ENC_NBUFS)) {
    evs_enc->extra_ecode = (uint32) ret_val;
    return E_M_EVS_ENC_CREATE_FAIL;
  }

  ret_val = fxns->std.MemAlloc(&enc_params.params, mem_bank);
  if (ret_val != USC_NoError) {
    evs_enc->extra_ecode = (uint32) ret_val;
    return E_M_EVS_ENC_ALG_ALLOC_FAIL;
  }

  internal = (struct m_internal_evs_enc_data *)ippsMalloc_8u(sizeof(internal));
  evs_enc->internal = internal;
  internal->pMem = (char *)ippsMalloc_8u(mem_bank[0].nbytes);

  mem_bank[0].pMem = internal->pMem;

  ret_val = fxns->std.Init(&enc_params.params, &mem_bank[0], &internal->enc_handle);
  if (ret_val != USC_NoError) {
    evs_enc->extra_ecode = (uint32) ret_val;
    return E_M_EVS_ENC_CREATE_FAIL;
  }

  (void)memcpy(&internal->codec_info, &enc_params, sizeof(enc_params));
  return EA_NO_ERRORS;
}

/*****************************************************************************************/
/**
@brief      EVS decoder module initialisation function

@param      evs_dec Generic decoder structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_evs_dec_init(codec_dec_data_t *evs_dec)
{
  struct m_internal_evs_dec_data*internal = NULL;

  USC_Fxns *fxns = &USC_EVS_Fxns;
  USC_Status ret_val = USC_NoError;
  USC_CodecInfo dec_params = { 0 };
  USC_MemBank mem_bank[EVS_DEC_NBUFS] = { {0} };
  int32 nbufs = 0;

  if (evs_dec == NULL) {
    return E_M_EVS_UNEXPECTED_NULL_POINTER;
  }

  evs_dec->extra_ecode = EA_NO_ERRORS;
  dec_params.params.direction = USC_DECODE;

  ret_val = fxns->std.GetInfo(NULL, &dec_params);
  if (ret_val != USC_NoError) {
    evs_dec->extra_ecode = (uint32) ret_val;
    return E_M_EVS_DEC_ALG_ALLOC_FAIL;
  }

  /* bitstramformat is 0, when data is raw format */
  dec_params.params.modes.bitstreamformat = 0;
  dec_params.params.modes.bitrate = 128000;
  //dec_params.params.pcmType.sample_frequency = EVS_SAMPLE_FREQUENCY_8;

  ret_val = fxns->std.NumAlloc(&dec_params.params, &nbufs);
  if ((ret_val != USC_NoError) && (nbufs != EVS_DEC_NBUFS)) {
    evs_dec->extra_ecode = (uint32) ret_val;
    return E_M_EVS_DEC_CREATE_FAIL;
  }

  ret_val = fxns->std.MemAlloc(&dec_params.params, mem_bank);
  if (ret_val != USC_NoError) {
    evs_dec->extra_ecode = (uint32) ret_val;
    return E_M_EVS_DEC_ALG_ALLOC_FAIL;
  }

  internal= (struct m_internal_evs_dec_data *)ippsMalloc_8u(sizeof(internal));
  evs_dec->internal = internal;
  internal->pMem = (char *)ippsMalloc_8u(mem_bank[0].nbytes);
  mem_bank[0].pMem = internal->pMem;

  ret_val = fxns->std.Init(&dec_params.params, &mem_bank[0], &internal->dec_handle);

  if (ret_val != USC_NoError) {
    evs_dec->extra_ecode = (uint32) ret_val;
    return E_M_EVS_DEC_INIT_FAILED;
  }

  internal->dec_frames_numb = 0;
  infof("m_evs_dec_init ok.\n");
  return EA_NO_ERRORS;
}

/*****************************************************************************************/
/**
@brief      EVS encoding function

Encodes an incoming UPD packet.

@param      evs_enc Generic encoder structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_evs_enc(codec_enc_data_t *evs_enc)
{
  struct m_internal_evs_enc_data *internal = NULL;
  upd_packet_t *lin_pcm_packet = NULL;
  upd_packet_t *out_codec_packet = NULL;
  uint8 *lin_pcm_payload = NULL;
  uint8 *upd_params_payload = NULL;
  int enc_bitrate = 0;
  uint8 evs_primary_mode = UPD_EVS_UNKNOW_MODE;
  upd_mode_t evs_bitrate_mode = UPD_MODE_EVS_PRIMARY_NOT_APPLICABLE;

  USC_Fxns *fxns = &USC_EVS_Fxns;
  USC_Status ret_val = USC_NoError;
  USC_PCMStream inBuf = { 0 };
  USC_Bitstream outBuf = { 0 };
  USC_Modes dynamic_params = { 0 };

  if ((evs_enc == NULL) || (evs_enc->internal == NULL) ||
      (evs_enc->data_io.upd_lin_pcm == NULL) ||
      (evs_enc->data_io.upd_out_codec == NULL)) {
    return E_M_EVS_UNEXPECTED_NULL_POINTER;
  }
  internal = evs_enc->internal;

  evs_primary_mode = evs_enc->ctrl_io.codec_ctrl_io.evs_enc_ctrl_io.evs_mode;
  if ((evs_primary_mode != UPD_EVS_PRIMARY_MODE)
    && (evs_primary_mode != UPD_EVS_WBIO_MODE)) {
    evs_enc->extra_ecode = evs_primary_mode;
    return E_M_EVS_ENC_UNKNOWN_CODEC_MODE;
  }

  evs_bitrate_mode = evs_enc->ctrl_io.mode;
  if (evs_primary_mode == UPD_EVS_PRIMARY_MODE) {
    if (evs_bitrate_mode >= UPD_NUMBER_OF_EVS_PRIMARY_MODES) {
      evs_enc->extra_ecode = evs_bitrate_mode;
      return E_M_EVS_INCORRECT_UPD_CODEC_TYPE;
    }
  } else {
    if (evs_bitrate_mode >= UPD_NUMBER_OF_EVS_WBIO_MODES) {
      evs_enc->extra_ecode = evs_bitrate_mode;
      return E_M_EVS_INCORRECT_UPD_CODEC_TYPE;
    }
  }

  /* get the real bitrate with the enum number */
  enc_bitrate = get_evs_bitrate(evs_primary_mode, evs_bitrate_mode);

  lin_pcm_packet = evs_enc->data_io.upd_lin_pcm;
  out_codec_packet = evs_enc->data_io.upd_out_codec;
  lin_pcm_payload = l_upd_get_payload(lin_pcm_packet);
  upd_params_payload = l_upd_get_payload(out_codec_packet);

  debugf("Encode, codec:%d,type:%d,mode_bitrate:%d,form:%d\n",
    out_codec_packet->codec, out_codec_packet->type,
    out_codec_packet->mode, out_codec_packet->form);

  debugf("Encode, dtx:%d, vad:%d,%d\n",
    evs_enc->ctrl_io.dtx_enabled, evs_enc->ctrl_io.va_detected,
    evs_enc->ctrl_io.enhanced_vad_enabled);

  dynamic_params.vad = (int32) evs_enc->ctrl_io.dtx_enabled;
  dynamic_params.truncate = TRUNCATE_ENABLED;
  dynamic_params.bitrate = enc_bitrate;
  dynamic_params.rf_fec_indicator = evs_enc->ctrl_io.codec_ctrl_io.evs_enc_ctrl_io.evs_fec_enable;
  dynamic_params.rf_fec_offset = evs_enc->ctrl_io.codec_ctrl_io.evs_enc_ctrl_io.evs_fec_offset;
  dynamic_params.max_bwidth_fx = enc_bitrate; //internal->codec_info.params.modes.max_bwidth_fx;
  dynamic_params.Opt_RF_ON = internal->codec_info.params.modes.Opt_RF_ON;
  dynamic_params.update_brate_on = 1; //internal->codec_info.params.modes.update_brate_on;
  dynamic_params.update_bwidth_on = 1; //internal->codec_info.params.modes.update_bwidth_on;
  dynamic_params.interval_SID_fx = 1;

  debugf("Encode, bitrate:%d,rf_fec:%d,%d,max_bwidth_fx:%d\n",
    dynamic_params.bitrate, dynamic_params.rf_fec_indicator,
    dynamic_params.rf_fec_offset, dynamic_params.max_bwidth_fx);
  debugf("Encode, Opt_RF_ON:%d, update:%d,%d\n",
    dynamic_params.Opt_RF_ON, dynamic_params.update_brate_on,
    dynamic_params.update_bwidth_on);

  ret_val = fxns->std.Control(&dynamic_params, internal->enc_handle);
  if (ret_val != USC_NoError) {
    evs_enc->extra_ecode = (uint32) ret_val;
    return E_M_EVS_ENC_CONTROL_FAILED;
  }

  inBuf.pBuffer = (char8 *) lin_pcm_payload;
  inBuf.nbytes = SIZE_1_TO_8(l_upd_get_payload_size_1(lin_pcm_packet));
  inBuf.bitrate = enc_bitrate;
  inBuf.pcmType.bitPerSample = EVS_BITS_PER_SAMPLE;
  inBuf.pcmType.nChannels = CHANNELS_MONO;
  inBuf.pcmType.sample_frequency = EVS_SAMPLE_FREQUENCY_16;
  outBuf.pBuffer = (char8 *) upd_params_payload;

  ret_val = fxns->Encode(internal->enc_handle, &inBuf, &outBuf);
  if (ret_val != USC_NoError) {
    evs_enc->extra_ecode = (uint32) ret_val;
    return E_M_EVS_ENC_PROCESS_FAILED;
  }
  debugf("Encode, outBuf:%d,%d,%d\n",
    outBuf.nbytes, outBuf.bitrate, outBuf.frametype);

  switch (outBuf.bitrate) {
    case 2400  :
      outBuf.frametype = (int)TX_SID_FIRST;
      evs_bitrate_mode = UPD_MODE_EVS_PRIMARY_SID;
      break;
    case 0     :
      outBuf.frametype = (int)TX_SID_UPDATE;
      evs_bitrate_mode = UPD_MODE_EVS_PRIMARY_SID;
      break;
    default    :
      outBuf.frametype = (int)TX_SPEECH;
      break;
  }

  /* Generate UPD header */
  out_codec_packet->codec = UPD_CODEC_EVS;
  out_codec_packet->type = map_type_from_usc_to_upd((int16) outBuf.frametype);
  out_codec_packet->mode = evs_bitrate_mode;
  out_codec_packet->form = evs_primary_mode;

  debugf("Encode, codec:%d,type:%d,mode_bitrate:%d,PRIMARY_MODE:%d,%d\n",
    out_codec_packet->codec, out_codec_packet->type,
    out_codec_packet->mode, out_codec_packet->form, evs_primary_mode);

  return EA_NO_ERRORS;
}

/*****************************************************************************************/
/**
@brief      EVS decoding function

Decodes an incoming UPD packet.

@param      evs_dec Generic decoder structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_evs_dec(codec_dec_data_t *evs_dec)
{
  struct m_internal_evs_dec_data *internal = NULL;
  upd_packet_t *codec_packet = NULL;
  upd_packet_t *lin_pcm_packet = NULL;
  uint8 *upd_params_payload = NULL;
  uint8 *lin_pcm_payload = NULL;
  int dec_bitrate = 0;
  upd_type_t frame_type = UPD_TYPE_NOT_APPLICABLE;
  uint8 evs_mode = UPD_EVS_UNKNOW_MODE;

  USC_Fxns *fxns = &USC_EVS_Fxns;
  USC_Status ret_val = USC_NoError;
  USC_Bitstream inBuf = { 0 };
  USC_PCMStream outBuf = { 0 };
  USC_Modes dynamic_params = { 0 };

  if ((evs_dec == NULL)
      || (evs_dec->internal == NULL)
      || (evs_dec->data_io.upd_lin_pcm == NULL)
      || (evs_dec->data_io.upd_input_codec == NULL)) {
    return E_M_EVS_UNEXPECTED_NULL_POINTER;
  }

  evs_dec->extra_ecode = EA_NO_ERRORS;
  codec_packet = evs_dec->data_io.upd_input_codec;
  lin_pcm_packet = evs_dec->data_io.upd_lin_pcm;
  upd_params_payload = l_upd_get_payload(codec_packet);
  lin_pcm_payload = l_upd_get_payload(lin_pcm_packet);
  lin_pcm_packet->type = UPD_TYPE_NO_DATA;

  evs_mode = codec_packet->form;
  if ((evs_mode != UPD_EVS_PRIMARY_MODE) && (evs_mode != UPD_EVS_WBIO_MODE)) {
    evs_dec->extra_ecode = evs_mode;
    return E_M_EVS_DEC_UNKNOWN_CODEC_MODE;
  }

  dec_bitrate= get_evs_bitrate(codec_packet->form, codec_packet->mode);

  internal = (struct m_internal_evs_dec_data *)evs_dec->internal;

  dynamic_params.truncate = 1;
  dynamic_params.bitrate = dec_bitrate;
  dynamic_params.use_parital_copy = 0;
  dynamic_params.next_coder_type = 0;
  dynamic_params.bitstreamformat = 0; /* without JBM */
  dynamic_params.vad = 1;

  debugf("Decode, codec:%d,type:%d,mode_bitrate:%d,PRIMARY_MODE:%d\n",
    codec_packet->codec, codec_packet->type,
    codec_packet->mode, codec_packet->form);

  ret_val = fxns->std.Control(&dynamic_params, internal->dec_handle);
  if (ret_val != USC_NoError) {
    evs_dec->extra_ecode = (uint32) dec_bitrate;
    return E_M_EVS_DEC_CONTROL_PROCESS_FAILED;
  }

  if ((codec_packet->type == UPD_TYPE_SPEECH_NO_DATA) ||
      (codec_packet->type == UPD_TYPE_SID_NO_DATA)) {
    frame_type = UPD_TYPE_NO_DATA;
  } else {
    frame_type = codec_packet->type;
  }

  inBuf.nbytes = SIZE_1_TO_8(l_upd_get_payload_size_1(codec_packet));
  inBuf.pBuffer = (char8 *)upd_params_payload;
  inBuf.frametype = frame_type_upd_packet_to_ipp(frame_type);// lost: frametype = -1
#if 0
  if (frame_type == UPD_TYPE_SID_FIRST) {
    internal->dec_frames_numb = 1;
  }
  if ( (internal->dec_frames_numb > 0) /*&& (internal->dec_frames_numb <= 3)*/ ) {
    internal->dec_frames_numb++;
    inBuf.frametype = -1;
    inBuf.pBuffer = NULL;
    inBuf.nbytes = 0;
  }
#endif
  inBuf.bitrate = dec_bitrate;
  if (inBuf.bitrate == 5900) {
    inBuf.bitrate = 2400; // WA: decode does not support 5.9kbps SID package
  }
  //set_rtp_info(&(inBuf.rtp_info), &evs_dec->data_io.rtp_info);  JBM Only

  outBuf.pBuffer = (char8 *) lin_pcm_payload;
  outBuf.nbytes = SIZE_1_TO_8(UPD_PAYLOAD_PCM_20_MS_LINEAR_16_KHZ_BITS);
  //outBuf.pcmType.bitPerSample = BITS_PER_PCM_SAMPLE;
  //outBuf.pcmType.nChannels = CHANNELS_MONO;

  ret_val = fxns->Decode(internal->dec_handle, &inBuf, &outBuf);
  if (ret_val != USC_NoError) {
    evs_dec->extra_ecode = (uint32) ret_val;
    return E_M_EVS_DEC_DECODE_PROCESS_FAILED;
  }

  debugf("Decode, output_sample_rate_hz:%d, actual:%d\n", 
    evs_dec->ctrl_io.output_sample_rate_hz, outBuf.pcmType.sample_frequency);

  lin_pcm_packet->codec = UPD_CODEC_PCM;
  lin_pcm_packet->type = UPD_TYPE_SPEECH_GOOD;
  lin_pcm_packet->mode = UPD_MODE_PCM_20_MS;
  lin_pcm_packet->form = UPD_FORM_PCM_LINEAR_16_KHZ;

  return EA_NO_ERRORS;
}

/*****************************************************************************************/
/**
@brief      EV encoder module release function

Calls the codec memory handler to release the memories allocated in the initialisation
phase (internal struct and instance memories).

@param      evs_enc Generic encoder structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_evs_enc_release(codec_enc_data_t *evs_enc)
{
  struct m_internal_evs_enc_data *internal = NULL;

  if ((evs_enc == NULL) || (evs_enc->internal == NULL)) {
    return E_M_EVS_UNEXPECTED_NULL_POINTER;
  }

  internal = evs_enc->internal;
  internal->enc_handle = NULL;

  if (internal->pMem == NULL){
    return E_M_EVS_MEM_NULL_POINTER;
  }

  ippsFree(internal->pMem);
  internal->pMem = NULL;
  ippsFree(internal);
  evs_enc->internal = NULL;

  return EA_NO_ERRORS;
}

/*****************************************************************************************/
/**
@brief      EVS decoder DAF module release function

Calls the codec memory handler to release the memories allocated in the initialisation
phase (internal struct and instance memories).

@param      evs_dec Generic decoder structure
@return     Error code
*/
MGW_GLOBAL ecode_t
m_evs_dec_release(codec_dec_data_t *evs_dec)
{
 struct m_internal_evs_dec_data *internal = NULL;

  if ((evs_dec == NULL) || (evs_dec->internal == NULL)) {
    return E_M_EVS_UNEXPECTED_NULL_POINTER;
  }

  internal = evs_dec->internal;
  internal->dec_handle = NULL;

  if (internal->pMem == NULL){
    return E_M_EVS_MEM_NULL_POINTER;
  }

  ippsFree(internal->pMem);
  internal->pMem = NULL;
  ippsFree(internal);
  evs_dec->internal = NULL;

  return EA_NO_ERRORS;
}

/*****************************************************************************************/
/**
@brief      Initializes intel ipp
*/
MGW_GLOBAL void
init_intel_ipp(void)
{
  IppStatus ret = ippStsNoErr;
  const IppLibraryVersion *verIppSC = NULL;
  const IppLibraryVersion *verIppSP = NULL;

  ret = ippInit();
  if (ret != ippStsNoErr) {
    errorf("IppInit failed:%d\n", ret);
  }

  verIppSC = ippscGetLibVersion();
  verIppSP = ippsGetLibVersion();
  infof("IPPSP: %s %s %s\n",
    verIppSP->Name, verIppSP->BuildDate, verIppSP->Version);
  infof("IPPSC: %s %s %s\n",
    verIppSC->Name, verIppSC->BuildDate, verIppSC->Version);
}

/*****************************************************************************************/
/**
@brief      Maps type from Intel USC to UPD format

@param      usc_type USC frame mode
@return     UPD type
*/
MGW_LOCAL upd_type_t
map_type_from_usc_to_upd(int16 usc_type)
{
  switch (usc_type) {
    case TX_SPEECH:
      return UPD_TYPE_SPEECH_GOOD;
    case TX_SID_FIRST:
      return UPD_TYPE_SID_UPDATE;
    case TX_SID_UPDATE:
      return UPD_TYPE_SID_UPDATE;
    case TX_NO_DATA:
      return UPD_TYPE_NO_DATA;
    default:
      return UPD_TYPE_NOT_APPLICABLE;
  }
}

/*****************************************************************************************/
/**
@brief      Frame type from upd packet format to Intel IPP USC format

Convert upd packet frame type to Intel IPP USC frame type.

@param      frame_type UPD packet frame type
@return     Intel IPP frame mode
*/
MGW_LOCAL int32
frame_type_upd_packet_to_ipp(upd_type_t frame_type)
{
  switch (frame_type) {
    case UPD_TYPE_NO_DATA:
    case UPD_TYPE_SPEECH_LOST:
    case UPD_TYPE_NOT_APPLICABLE:
    case UPD_TYPE_SID_FIRST:
      return -1;
    default:
      return (int32) frame_type;
  }
}

/*****************************************************************************************/
/**
@brief      EVS get the bitrate

@param      evs mode and mode
@return     bitrate
*/
MGW_LOCAL int 
get_evs_bitrate(uint16 evs_mode, upd_mode_t mode)
{
  int bitrate = 0;

  if (evs_mode == UPD_EVS_PRIMARY_MODE) {
    bitrate = evs_primary_mode_br(mode);
  } else if (evs_mode == UPD_EVS_WBIO_MODE) {
    bitrate = evs_wbio_mode_br(mode);
  }

  return bitrate;
}
/*****************************************************************************************/
/**
@brief      EVS get primary bitrate

@param      mode
@return     bitrate
*/
MGW_LOCAL int
evs_primary_mode_br(upd_mode_t mode)
{
  int br = 0;
  uint16 i = mode + 1;

  if (mode > UPD_MODE_EVS_PRIMARY_128000) {
    i = 0;
  }

  br = primary_mode_br[i];

  return br;
}
/*****************************************************************************************/
/**
@brief      EVS get amrwb io bitrate

@param      mode
@return     bitrate
*/
MGW_LOCAL int 
evs_wbio_mode_br(upd_mode_t mode)
{
  int br = 0;
  uint16 i = mode + 1;

  if (mode > UPD_MODE_EVS_WBIO_23850) {
    i = 0;
  }

  br = wbio_mode_br[i];

  return br;
}


