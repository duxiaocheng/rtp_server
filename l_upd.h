/*****************************************************************************************/
/**
@brief Function library for UPD
*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/

#ifndef L_UPD_H
#define L_UPD_H

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include "upd.h"

/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

 /** Sample rate for narrowband */
#define L_UPD_NB_SAMPLE_RATE_HZ  8000
 /** Sample rate for mediumband */
#define L_UPD_MB_SAMPLE_RATE_HZ  12000
 /** Sample rate for wideband */
#define L_UPD_WB_SAMPLE_RATE_HZ  16000
 /** Sample rate for super-wideband */
#define L_UPD_SWB_SAMPLE_RATE_HZ 24000
 /** Sample rate for fullband */
#define L_UPD_FB_SAMPLE_RATE_HZ  48000

/** Optimal target average bitrate for Opus narrowband encoder is 8000 ... 12000 bit/s,
    see RFC6716 */
#define L_UPD_OPUS_NB_DEFAULT_BITRATE 12000
/** Optimal target average bitrate for Opus wideband encoder is 16000 ... 20000 bit/s,
    see RFC6716 */
#define L_UPD_OPUS_WB_DEFAULT_BITRATE 20000
/** Minimum bitrate for Opus encoder is 6000 bit/s, see RFC6716 */
#define L_UPD_OPUS_MINIMUM_BITRATE 6000

/*Bandwidth Option for EVS */

    /**< 4 kHz bandpass @hideinitializer*/
#define L_UPD_EVS_CIT_BW_NARROWBAND 0
    /**< 8 kHz bandpass @hideinitializer*/
#define L_UPD_EVS_CIT_BW_WIDEBAND 1
    /**<16 kHz bandpass @hideinitializer*/
#define L_UPD_EVS_CIT_BW_SUPERWIDEBAND 2
    /**<20 kHz bandpass @hideinitializer*/
#define L_UPD_EVS_CIT_BW_FULLBAND 3

#define PCM_01_MS_SIZE_SAMPLES  8    /**<  1 ms PCM frame size in samples */
#define PCM_05_MS_SIZE_SAMPLES  40   /**<  5 ms PCM frame size in samples */
#define PCM_10_MS_SIZE_SAMPLES  80   /**< 10 ms PCM frame size in samples */
#define PCM_20_MS_SIZE_SAMPLES  160  /**< 20 ms PCM frame size in samples */
#define PCM_30_MS_SIZE_SAMPLES  240  /**< 30 ms PCM frame size in samples */


/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

/** Returns TRUE if t is of type speech, FALSE otherwise */
#define L_UPD_IS_TYPE_SPEECH(t) (((t) == UPD_TYPE_SPEECH_GOOD) ||\
((t) == UPD_TYPE_SPEECH_DEGRADED) || ((t) == UPD_TYPE_SPEECH_BAD) ||\
((t) == UPD_TYPE_SPEECH_LOST) || ((t) == UPD_TYPE_SPEECH_NO_DATA))

/** Returns TRUE if t is of type SID, FALSE otherwise */
#define L_UPD_IS_TYPE_SID(t) (((t) == UPD_TYPE_SID_FIRST) ||\
((t) == UPD_TYPE_SID_UPDATE) || ((t) == UPD_TYPE_SID_BAD) ||\
((t) == UPD_TYPE_SID_UNRELIABLE) || ((t) == UPD_TYPE_SID_NO_DATA))

/** Returns TRUE if t is of type bad, FALSE otherwise */
#define L_UPD_IS_TYPE_BAD(t) (((t) == UPD_TYPE_SPEECH_BAD) || ((t) == UPD_TYPE_SID_BAD))

/** Returns TRUE if type is in types, FALSE otherwise */
#define L_UPD_IS_TYPE_IN_TYPES(type, types) (((type) < UPD_TYPE_NOT_APPLICABLE) &&\
(((types) >> (type)) & 0x1))

/** Form an error extra code from UDP header */
#ifdef ARCH_BIG_ENDIAN
#define L_UPD_HEADER_TO_EXTRA_ECODE(x) *(uint32 *)(x)
#else
#define L_UPD_HEADER_TO_EXTRA_ECODE(x) (((*(uint32 *)(x) & 0xFFFF0000) >> 16) | \
                                         ((*(uint32 *)(x) & 0x0000FFFF) << 16))
#endif

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

/*****************************************************************************************/
/****    5 FUNCTION PROTOTYPES    ********************************************************/

MGW_GLOBAL uint16
l_upd_get_t38_payload_size_8(const upd_packet_t *upd_packet);

MGW_GLOBAL void
l_upd_set_t38_payload_size_8(upd_packet_t *upd_packet, uint16 size_8);

MGW_GLOBAL uint16
l_upd_get_opus_payload_size_8(const upd_packet_t *upd_packet);

MGW_GLOBAL void
l_upd_set_opus_payload_size_8(upd_packet_t *upd_packet, uint16 size_8);

MGW_GLOBAL uint16
l_upd_get_packet_size_1(upd_packet_t *upd_packet);

MGW_GLOBAL uint16
l_upd_get_payload_size_1(const upd_packet_t *upd_packet);

MGW_GLOBAL uint16
l_upd_get_amr_payload_size_1(upd_codec_t codec, upd_mode_amr_t mode);

MGW_GLOBAL uint16
l_upd_get_speech_duration_ms(upd_codec_t codec, upd_mode_t mode);

MGW_GLOBAL upd_mode_amrnb_t
l_upd_get_amrnb_sid_mode(uint16 *amrnb_payload);

MGW_GLOBAL upd_type_t
l_upd_get_amrnb_sid_type(uint16 *amrnb_payload);

MGW_GLOBAL upd_mode_amrwb_t
l_upd_get_amrwb_sid_mode(uint16 *amrwb_payload);

MGW_GLOBAL upd_type_t
l_upd_get_amrwb_sid_type(uint16 *amrwb_payload);

MGW_GLOBAL upd_mode_amrnb_t
l_upd_map_bits_to_amrnb_mode(uint16 bits);

MGW_GLOBAL upd_mode_amrwb_t
l_upd_map_bits_to_amrwb_mode(uint16 bits);

MGW_GLOBAL uint16
l_upd_get_max_amr_size_bits(upd_codec_t codec);

MGW_GLOBAL upd_mode_amr_t
l_upd_get_max_amr_mode(upd_codec_t codec);

MGW_GLOBAL uint16
l_upd_get_payload_padding_mask(uint16 size_in_bits);

MGW_GLOBAL upd_type_t
l_upd_get_gsm_fr_efr_type(uint16 bfi, uint16 sid, uint16 taf);

MGW_GLOBAL uint16
l_upd_get_bitrate(upd_codec_t codec, upd_mode_t mode);

MGW_GLOBAL void
l_upd_map_gsm_fr_sid_long_to_short(uint16 *payload);

MGW_GLOBAL void
l_upd_map_gsm_efr_sid_long_to_short(uint16 *payload);

MGW_GLOBAL void
l_upd_map_gsm_fr_sid_short_to_long(uint16 *payload);

MGW_GLOBAL void
l_upd_map_gsm_efr_sid_short_to_long(uint16 *payload);

MGW_GLOBAL void
l_upd_map_gsm_hr_sid_short_to_long(uint16 *payload);

MGW_GLOBAL void
l_upd_shuffle_amrnb_payload(const uint8 *payload_in, uint8 *payload_out,
                            upd_mode_amrnb_t mode);

MGW_GLOBAL void
l_upd_de_shuffle_amrnb_payload(const uint8 *payload_in, uint8 *payload_out,
                               upd_mode_amrnb_t mode);

MGW_GLOBAL uint16
l_upd_get_pcm_size_samples(upd_mode_pcm_t mode);

MGW_GLOBAL upd_mode_pcm_t
l_upd_get_pcm_mode(uint16 pcm_size_samples);

MGW_GLOBAL bool16
l_upd_is_type_valid(upd_codec_t codec, upd_type_t type);

MGW_GLOBAL uint16
l_upd_get_number_of_modes(upd_codec_t codec, uint16 mode);

MGW_GLOBAL void
l_upd_nat_to_be(upd_packet_t *upd_packet);

MGW_GLOBAL void
l_upd_be_to_nat(upd_packet_t *upd_packet);

MGW_GLOBAL uint32
l_upd_generate_extra_ecode_32(upd_packet_t *upd_packet);

MGW_GLOBAL void
l_upd_header_be_to_nat(upd_packet_t *upd_packet);

MGW_GLOBAL void
l_upd_header_nat_to_be(upd_packet_t *upd_packet);

MGW_GLOBAL void *
l_upd_get_payload(const upd_packet_t *upd_packet);

MGW_GLOBAL upd_type_t
l_upd_get_type(const upd_packet_t *upd_packet);

MGW_GLOBAL uint16
l_upd_get_pcm_sample_rate_hz(const upd_packet_t *upd_packet);

MGW_GLOBAL upd_form_t
l_upd_get_lin_pcm_form(uint16 sample_rate_hz);

MGW_GLOBAL uint32
l_upd_get_opus_speech_duration_us(upd_packet_t *upd_packet);

MGW_GLOBAL uint16
l_upd_get_num_frames_in_opus_packet(upd_packet_t *upd_packet);

MGW_GLOBAL uint16
l_upd_get_opus_speech_duration_ms(upd_packet_t *upd_packet);

MGW_GLOBAL uint16
l_upd_get_target_average_bitrate_for_enc(upd_codec_t codec, upd_mode_t mode,
                                         uint32 opus_max_average_bitrate_request,
                                         uint16 sample_rate_hz);

MGW_GLOBAL uint16
l_upd_get_sample_rate_hz(const upd_packet_t *upd_packet);

MGW_GLOBAL uint16
get_evs_sample_rate_hz(const uint16 ingress_bandwidth);


MGW_GLOBAL uint16
l_upd_get_evs_primary_payload_size(upd_mode_amr_t mode);

MGW_GLOBAL uint16
l_upd_get_evs_wbio_payload_size(upd_mode_amr_t mode);

MGW_GLOBAL upd_type_t
l_upd_get_evs_wbio_sid_type(uint16 *evs_payload);

MGW_GLOBAL upd_type_t
l_upd_get_evs_primary_sid_type(uint16 *evs_payload);

MGW_GLOBAL upd_mode_evs_t
l_upd_get_evs_wbio_sid_mode(uint16 *evs_payload);

MGW_GLOBAL upd_mode_evs_t
l_upd_get_evs_primary_sid_mode(uint16 *evs_payload);

MGW_GLOBAL upd_mode_amr_t
l_upd_get_max_evs_mode(uint16 ft_mode);

MGW_GLOBAL uint32
l_upd_get_evs_bitrate(uint16 evs_mode, upd_mode_t mode);

MGW_GLOBAL const char *
str_upd_codec(uint16 upd_codec);

MGW_GLOBAL const char *
str_upd_type(uint16 upd_type);


/*****************************************************************************************/
#endif /* UPD_H */

