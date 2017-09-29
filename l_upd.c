/*****************************************************************************************/
/**
 * @brief Function library for UPD
 *
 * Remember to name the author of the function when adding one.
 *
 * */

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include "l_upd.h"
#include <l_string.h>

/*****************************************************************************************/
/****    2 LOCAL CONSTANTS    ************************************************************/

/*****************************************************************************************/
/****    3 LOCAL MACROS    ***************************************************************/

/*****************************************************************************************/
/****    4 LOCAL DATA TYPES    ***********************************************************/

/*****************************************************************************************/
/****    5 LOCAL FUNCTION PROTOTYPES    **************************************************/

/*****************************************************************************************/
/****    6 FUNCTIONS    ******************************************************************/

/*****************************************************************************************/
/**
 * @brief      Returns pointer to upd packet payload
 *
 * @param      upd_packet UPD packet
 * @return     Pointer to upd packet payload
 * */
MGW_GLOBAL void *
l_upd_get_payload(const upd_packet_t *upd_packet)
{
    return (void *) upd_packet->payload_8;
}

/*****************************************************************************************/
/**
@brief      Gets the size of the speech EVS payload in bits header_full_primary

@param      mode AMR mode of the payload
@return     The size of the AMR payload in bits, or 0 if codec or mode is invalid
*/
MGW_GLOBAL uint16
l_upd_get_evs_primary_payload_size(upd_mode_evs_t mode)
{
	/** EVS PRIMARY codec parameter bit order table address in easy to use array format */
	MGW_LOCAL const uint16
	evs_primary_payload_size_in_bits[UPD_NUMBER_OF_EVS_PRIMARY_MODES] = {
		UPD_PAYLOAD_EVS_56,
		UPD_PAYLOAD_EVS_144,
		UPD_PAYLOAD_EVS_160,
		UPD_PAYLOAD_EVS_192,
		UPD_PAYLOAD_EVS_264,
		UPD_PAYLOAD_EVS_328,
		UPD_PAYLOAD_EVS_488,
		UPD_PAYLOAD_EVS_640,
		UPD_PAYLOAD_EVS_960,
		UPD_PAYLOAD_EVS_1280,
		UPD_PAYLOAD_EVS_1920,
		UPD_PAYLOAD_EVS_2560
	};
	if (mode <= UPD_MODE_EVS_PRIMARY_128000) {
    return evs_primary_payload_size_in_bits[mode];
  }

  return 0;
}

/*****************************************************************************************/
/**
@brief      Gets the size of the speech EVS payload in bits header_full_wbio

@param      mode AMR mode of the payload
@return     The size of the AMR payload in bits, or 0 if codec or mode is invalid
*/
MGW_GLOBAL uint16
l_upd_get_evs_wbio_payload_size(upd_mode_evs_t mode)
{
	/** EVS WBIO codec parameter bit order table address in easy to use array format */
	MGW_LOCAL const uint16
	evs_wbio_payload_size_in_bits[UPD_NUMBER_OF_EVS_WBIO_MODES] = {
		UPD_PAYLOAD_EVS_132,
		UPD_PAYLOAD_EVS_177,
		UPD_PAYLOAD_EVS_253,
		UPD_PAYLOAD_EVS_285,
		UPD_PAYLOAD_EVS_317,
		UPD_PAYLOAD_EVS_365,
		UPD_PAYLOAD_EVS_397,
		UPD_PAYLOAD_EVS_461,
		UPD_PAYLOAD_EVS_477
	};
  if (mode <= UPD_MODE_EVS_WBIO_23850) {
    return evs_wbio_payload_size_in_bits[mode];
  }

  return 0;
}

/*****************************************************************************************/
/**
@brief      Reads EVS SID UPD type from SID payload bits


@param      EVS Payload of EVS UPD packet
@return     EVS SID UPD type
*/
MGW_GLOBAL upd_type_t
l_upd_get_evs_wbio_sid_type(uint16 *evs_payload)
{
  upd_type_t upd_type;
  /* evs wbio just like amrwb_payload[2] = payload bits 33-40 */
  if ((evs_payload[2] & 0x1000) == 0) { /* bit 36 (STI) */
    upd_type = UPD_TYPE_SID_FIRST;
  } else {
    upd_type = UPD_TYPE_SID_UPDATE;
  }
  return upd_type;
}

MGW_GLOBAL upd_type_t
l_upd_get_evs_primary_sid_type(uint16 *evs_payload)
{
  upd_type_t upd_type;
  /* evs wbio just like amrwb_payload[2] = payload bits 33-40 */
  if ((evs_payload[2] & 0x1000) == 0) { /* bit 36 (STI) */
    upd_type = UPD_TYPE_SID_FIRST;
  } else {
    upd_type = UPD_TYPE_SID_UPDATE;
  }
  return upd_type;
}
/*****************************************************************************************/
/**
@brief      Reads EVS WBIO SID UPD mode from SID payload bits

EVS WBIO SID payload is defined in 3GPP 26.201 Chapter 4.2.3. Mode is indicated in payload
bits 37,38,39,40 (37=MSB, 40=LSB).

@param      EVS WBIO Payload of EVS WBIO UPD packet
@return     EVS WBIO SID UPD mode
*/
MGW_GLOBAL upd_mode_evs_t
l_upd_get_evs_wbio_sid_mode(uint16 *evs_payload)
{
  /* evs_payload[2] = payload bits 33-40 */
  uint16 mode = ((evs_payload[2] & 0x0F00) >> 8); /* bits 37-40 */
  upd_mode_evs_t upd_evs_mode;
    
  if (mode <= UPD_MODE_EVS_WBIO_23850) {
    upd_evs_mode = mode;
  } else {
    upd_evs_mode = UPD_MODE_EVS_WBIO_NOT_APPLICABLE;
  }
  return upd_evs_mode;
}

/*****************************************************************************************/
/**
@brief      Reads EVS primary SID UPD mode from SID payload bits

EVS primary SID payload is defined in 3GPP 26.201 Chapter 4.2.3. Mode is indicated in payload
bits 37,38,39,40 (37=MSB, 40=LSB).

@param      EVS PRIMARY Payload of EVS WBIO UPD packet
@return     EVS PRIMARY SID UPD mode
*/
MGW_GLOBAL upd_mode_evs_t
l_upd_get_evs_primary_sid_mode(uint16 *evs_payload)
{
  /* evs_payload[2] = payload bits 33-40 */
  uint16 mode = ((evs_payload[2] & 0x0F00) >> 8); /* bits 37-40 */
  upd_mode_evs_t upd_evs_mode;

  if (mode <= UPD_MODE_EVS_PRIMARY_128000) {
    upd_evs_mode = mode;
  } else {
    upd_evs_mode = UPD_MODE_EVS_PRIMARY_NOT_APPLICABLE;
  }
  return upd_evs_mode;
}

/*****************************************************************************************/
/**
@brief      Gets the maximum EVS mode

Gets the maximum (highest bitrate) EVS mode.

@param      codec Codec whose maximum mode is queried
@return     Maximum EVS mode
*/
MGW_GLOBAL upd_mode_amr_t
l_upd_get_max_evs_mode(uint16 ft_mode)
{
  if (ft_mode == UPD_EVS_WBIO_MODE) {
    return UPD_MODE_EVS_WBIO_23850;
  } else if (ft_mode == UPD_EVS_PRIMARY_MODE) {
    return UPD_MODE_EVS_PRIMARY_128000;
  } else {
    return UPD_MODE_EVS_NOT_APPLICABLE;
  }
}

/*****************************************************************************************/
/**
@brief      Gets EVS bitrate (bit/s) correspoding to UPD codec and mode


@return     Bitrate (bit/s), 0 for error situations
*/
MGW_GLOBAL uint32
l_upd_get_evs_bitrate(uint16 evs_mode, upd_mode_t mode)
{
  uint32 bitrate = 0;
  if (evs_mode == UPD_EVS_PRIMARY_MODE) {
    switch (mode){
      case UPD_MODE_EVS_PRIMARY_2800:
        bitrate = 2800;
        break;
      case UPD_MODE_EVS_PRIMARY_7200:
        bitrate = 7200;
        break;
      case UPD_MODE_EVS_PRIMARY_8000:
        bitrate = 8000;
        break;
      case UPD_MODE_EVS_PRIMARY_9600:
        bitrate = 9600;
        break;
      case UPD_MODE_EVS_PRIMARY_13200:
        bitrate = 13200;
        break;
      case UPD_MODE_EVS_PRIMARY_16400:
        bitrate = 16400;
        break;
      case UPD_MODE_EVS_PRIMARY_24400:
        bitrate = 24400;
        break;
      case UPD_MODE_EVS_PRIMARY_32000:
        bitrate = 32000;
        break;
      case UPD_MODE_EVS_PRIMARY_48000:
        bitrate = 48000;
        break;
      case UPD_MODE_EVS_PRIMARY_64000:
        bitrate = 64000;
        break;
      case UPD_MODE_EVS_PRIMARY_96000:
        bitrate = 96000;
        break;
      case UPD_MODE_EVS_PRIMARY_128000:
        bitrate = 128000;
        break;
      default:
        bitrate = 0;
        break;
    }
  } else if (evs_mode == UPD_EVS_WBIO_MODE) {
    switch (mode){
      case UPD_MODE_EVS_WBIO_6600:
        bitrate = 6600;
        break;
      case UPD_MODE_EVS_WBIO_8850:
        bitrate = 8850;
        break;
      case UPD_MODE_EVS_WBIO_12650:
        bitrate = 12650;
        break;
      case UPD_MODE_EVS_WBIO_14250:
        bitrate = 14250;
        break;
      case UPD_MODE_EVS_WBIO_15850:
        bitrate = 15850;
        break;
      case UPD_MODE_EVS_WBIO_18250:
        bitrate = 18250;
        break;
      case UPD_MODE_EVS_WBIO_19850:
        bitrate = 19850;
        break;
      case UPD_MODE_EVS_WBIO_23050:
        bitrate = 23050;
        break;
      case UPD_MODE_EVS_WBIO_23850:
        bitrate = 23850;
        break;
      default:
        bitrate = 0;
        break;
    }
  }
  return bitrate;
}

MGW_GLOBAL const char *
str_upd_type(uint16 upd_type)
{
  const str_table_t upd_type_tab[] = { 
    STR_TABLE_LINE(UPD_TYPE_NO_DATA),
    STR_TABLE_LINE(UPD_TYPE_SPEECH_GOOD),
    STR_TABLE_LINE(UPD_TYPE_SPEECH_BAD),
    STR_TABLE_LINE(UPD_TYPE_SPEECH_DEGRADED),
    STR_TABLE_LINE(UPD_TYPE_SPEECH_LOST),
    STR_TABLE_LINE(UPD_TYPE_SID_FIRST),
    STR_TABLE_LINE(UPD_TYPE_SID_UPDATE),
    STR_TABLE_LINE(UPD_TYPE_SID_BAD),
    STR_TABLE_LINE(UPD_TYPE_SID_UNRELIABLE),
    STR_TABLE_LINE(UPD_TYPE_ONSET),
    STR_TABLE_LINE(UPD_TYPE_SPEECH_NO_DATA),
    STR_TABLE_LINE(UPD_TYPE_SID_NO_DATA),
    STR_TABLE_LINE(UPD_TYPE_SID_FOR_FEATURE),
    STR_TABLE_LINE(UPD_TYPE_NOT_APPLICABLE),
    STR_TABLE_END
  };  
  return value_to_str(upd_type, upd_type_tab);
}

MGW_GLOBAL const char *
str_upd_codec(uint16 upd_codec)
{
  const str_table_t upd_code_tab[] = { 
    STR_TABLE_LINE(UPD_CODEC_UNDEFINED),
    STR_TABLE_LINE(UPD_CODEC_PCM),
    STR_TABLE_LINE(UPD_CODEC_GSM_FR),
    STR_TABLE_LINE(UPD_CODEC_GSM_EFR),
    STR_TABLE_LINE(UPD_CODEC_GSM_HR),
    STR_TABLE_LINE(UPD_CODEC_AMRNB),
    STR_TABLE_LINE(UPD_CODEC_AMRWB),
    STR_TABLE_LINE(UPD_CODEC_BFP),
    STR_TABLE_LINE(UPD_CODEC_OPUS),
    STR_TABLE_LINE(UPD_CODEC_ILBC),
    STR_TABLE_LINE(UPD_CODEC_G723),
    STR_TABLE_LINE(UPD_CODEC_G729),
    STR_TABLE_LINE(UPD_CODEC_EVRC),
    STR_TABLE_LINE(UPD_CODEC_DATA),
    STR_TABLE_LINE(UPD_CODEC_EVS),
    STR_TABLE_END
  };
  return value_to_str(upd_codec, upd_code_tab);
}




/*****************************************************************************************/


