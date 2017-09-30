/*****************************************************************************************/
/**
@brief EVS functions
*/

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include <type_def.h>
#include <log_print.h>
#include <l_mgw.h>
#include <l_scratch.h>
#include <l_string.h>
#include "l_upd.h"
#include "m_evs.h"
#include "m_src.h"

#include <string.h>
#include <stdlib.h>

/*****************************************************************************************/
/****    2 LOCAL CONSTANTS    ************************************************************/

/** Scratch linear 8 kHz PCM UPD packet alignment size in 16-bit words */
#define SCRATCH_LIN_08_KHZ_PCM_UPD_PACKET_ALIGNMENT_SIZE_16 \
  (SIZE_1_TO_16(64) - UPD_HEADER_SIZE_16)
/** Scratch linear 16 kHz PCM UPD packet alignment size in 16-bit words */
#define SCRATCH_LIN_16_KHZ_PCM_UPD_PACKET_ALIGNMENT_SIZE_16 \
  SCRATCH_LIN_08_KHZ_PCM_UPD_PACKET_ALIGNMENT_SIZE_16

/** Scratch linear 8 kHz PCM UPD packet size in 16-bit words */
#define SCRATCH_LIN_08_KHZ_PCM_UPD_PACKET_SIZE_16 \
  (UPD_HEADER_SIZE_16 + SIZE_1_TO_16(UPD_PAYLOAD_PCM_30_MS_LINEAR_08_KHZ_BITS))

/** Scratch linear 16 kHz PCM UPD packet size in 16-bit words */
#define SCRATCH_LIN_16_KHZ_PCM_UPD_PACKET_SIZE_16 \
  (UPD_HEADER_SIZE_16 + SIZE_1_TO_16(UPD_PAYLOAD_PCM_30_MS_LINEAR_16_KHZ_BITS))

/*****************************************************************************************/
/****    3 LOCAL MACROS    ***************************************************************/

#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_48_SID            48                     /**Primary 2.4 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_56                56                     /**Primary 2.8 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_144               144                    /**Primary 7.2 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_160               160                    /**Primary 8.0 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_192               192                    /**Primary 9.6 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_264               264                    /**Primary 13.2 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_328               328                    /**Primary 16.4 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_488               488                    /**Primary 24.4 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_640               640                    /**Primary 32.0 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_960               960                    /**Primary 48.0 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_1280              1280                   /**Primary 64.0 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_1920              1920                   /**Primary 96.0 kbps*/
#define EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_2560              2560                   /**Primary 128.0 kbps*/

#define EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_136               136                    /**AMR-WB IO 6.6 kbps*/
#define EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_184               184                    /**AMR-WB IO 8.85 kbps*/
#define EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_256               256                    /**AMR-WB IO 12.65 kbps*/
#define EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_288               288                    /**AMR-WB IO 14.25 kbps*/
#define EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_320               320                    /**AMR-WB IO 15.85 kbps*/
#define EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_368               368                    /**AMR-WB IO 18.25 kbps*/
#define EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_400               400                    /**AMR-WB IO 19.85 kbps*/
#define EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_464               464                    /**AMR-WB IO 23.05 kbps*/
#define EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_480               480                    /**AMR-WB IO 23.85 kbps*/


/** CMR no_data  same like EVS_FT_NO_DATA and AMR_FT_NO_DATA*/
#define CMR_FT_NO_DATA 15

#define CMR_T_000 0
#define CMR_T_001 1
#define CMR_T_010 2
#define CMR_T_011 3
#define CMR_T_100 4
#define CMR_T_101 5
#define CMR_T_110 6
#define CMR_T_111 7

#define CMR_COMPACT_660   0
#define CMR_COMPACT_885   1
#define CMR_COMPACT_1265  2
#define CMR_COMPACT_1585  3
#define CMR_COMPACT_1825  4
#define CMR_COMPACT_2305  5
#define CMR_COMPACT_2385  6
#define CMR_COMPACT_NONE  7

#define CMR_CHANNEL_AWARE_MODE_0000 0
#define CMR_CHANNEL_AWARE_MODE_0001 1
#define CMR_CHANNEL_AWARE_MODE_0010 2
#define CMR_CHANNEL_AWARE_MODE_0011 3
#define CMR_CHANNEL_AWARE_MODE_0100 4
#define CMR_CHANNEL_AWARE_MODE_0101 5
#define CMR_CHANNEL_AWARE_MODE_0110 6
#define CMR_CHANNEL_AWARE_MODE_0111 7

#define CHANNEL_AWARE_MODE_OFFSET2  2
#define CHANNEL_AWARE_MODE_OFFSET3  3
#define CHANNEL_AWARE_MODE_OFFSET5  5
#define CHANNEL_AWARE_MODE_OFFSET7  7
#define CHANNEL_AWARE_MODE_OFFSET_INVALID  0xFF


/** Maximum number of EVS frames per RTP packet */
#define M_MB_IPT_MAX_EVS_FRAMES_PER_PACKET 4
/** Maximum size of payload in EVS Primary 128.0 kbps single frame size*/
#define M_MB_IPT_MAX_SINGLE_EVS_PAYLOAD_SIZE_16   160
/** Egress offset (= internal extra header + uncompressed RTP header) */
#define M_MB_IPT_EGRESS_OFFSET_SIZE_32   4
#define M_MB_IPT_MAX_SINGLE_EVS_DATA_OUTPUT_SIZE_16 \
      (SIZE_32_TO_16(M_MB_IPT_EGRESS_OFFSET_SIZE_32) + M_MB_IPT_MAX_SINGLE_EVS_PAYLOAD_SIZE_16)/*32BIT for UPD_PACKET header and padding*/
/** Maximum size of payload in EVS Primary 128.0 kbps header full */
#define M_MB_IPT_MAX_EVS_PAYLOAD_SIZE_16   (M_MB_IPT_MAX_SINGLE_EVS_PAYLOAD_SIZE_16*M_MB_IPT_MAX_EVS_FRAMES_PER_PACKET)
/** Maximum size of payload head with CMR and TOCs( 8bits CMR +4*8bits TOC*/
#define M_MB_IPT_MAX_EVS_PAYLOAD_HEAD_WITH_CMR_TOCS_SIZE_16 SIZE_1_TO_16(8+(M_MB_IPT_MAX_EVS_FRAMES_PER_PACKET*8))
/** Maximum size of payload zero padding( 8bits *4packet*2byte zero padding*/
#define M_MB_IPT_MAX_EVS_PAYLOAD_ZERO_PADDING_SIZE_16 SIZE_1_TO_16(M_MB_IPT_MAX_EVS_FRAMES_PER_PACKET*8*2)
/** Maximum size of payload in EVS Primary 128.0 kbps header full */
#define M_MB_IPT_MAX_EVS_PAYLOAD_WITH_CMR_TOCS_SIZE_16 \
      (M_MB_IPT_MAX_EVS_PAYLOAD_SIZE_16 + M_MB_IPT_MAX_EVS_PAYLOAD_HEAD_WITH_CMR_TOCS_SIZE_16 + M_MB_IPT_MAX_EVS_PAYLOAD_ZERO_PADDING_SIZE_16 )
/** Size of an element in EVS frame buffer */
#define M_MB_IPT_EVS_FRAME_BUFFER_ELEMENT_SIZE_16 (SIZE_1_TO_16(UPD_PAYLOAD_EVS_2560))
/** Maximum evs size of data output in octets (= offset + payload) */
#define M_MB_IPT_MAX_EVS_DATA_OUTPUT_SIZE_16 \
  (SIZE_32_TO_16(M_MB_IPT_EGRESS_OFFSET_SIZE_32) + M_MB_IPT_MAX_EVS_PAYLOAD_WITH_CMR_TOCS_SIZE_16)
/** Maximum evs size of data input in octets (upd_head * 4 +  payload * 4) */
#define M_MB_IPT_MAX_EVS_DATA_INPUT_SIZE_16 \
  ((UPD_HEADER_SIZE_16*M_MB_IPT_MAX_EVS_FRAMES_PER_PACKET) + (M_MB_IPT_EVS_FRAME_BUFFER_ELEMENT_SIZE_16*M_MB_IPT_MAX_EVS_FRAMES_PER_PACKET))

#define EVS_COMPACT_CMR_ENTRY_SIZE                  3
#define EVS_HEADER_FULL_CMR_ENTRY_SIZE              8
#define EVS_TOC_ENTRY_SIZE                          8

#define CHECK_ERROR(func, ret, extra) \
        if (ret != EA_NO_ERRORS) { \
          errorf("%s error:0x%x, extra:0x%x\n", func, ret, extra); \
        }

#define EVS_PRIMARY_MODE        UPD_EVS_PRIMARY_MODE /* 0 */
#define EVS_WBIO_MODE           UPD_EVS_WBIO_MODE    /* 1 */
#define EVS_UNKNOW_MODE         UPD_EVS_UNKNOW_MODE  /* 3 */


/*****************************************************************************************/
/****    4 LOCAL DATA TYPES    ***********************************************************/

/**
@brief      Module parameters
*/
struct params_t {
  bf16 interface            : 4; /**< Type of external interface (#m_mb_ipt_interface_t) */
  bf16 codec                : 4; /**< UPD codec (#upd_codec_t) */
  bf16 mode_change_period   : 4; /**< Must be 1 or 2 (AMR) */
  bf16 up_is_amrwb          : 1; /**< Is the UP amrwb while the CP is EVS amrwb io */
  bf16 octet_align          : 1; /**< Bandwidth efficient = 0, octet aligned = 1 (AMR) */
  bf16 mode_change_neighbor : 1; /**< Change to any mode = 0, only to neighbor = 1 (AMR) */
  bf16 evs_format           : 1; /**< EVS format 0: compact 1: header full */
  bf16 evs_mode             : 1; /**< EVS mode 0: evs primary 1: evs amrwb io */
  bf16 padding              : 15; /**< Bit-field padding */

  /* bitrate */
  uint16 br;
  // uint16 cam_offset_rcv; /**< ch_aw_rcv, if -1, disable it in receive direction */
  /** LSB bit is the lowest mode etc. (AMR, G.723) */
  mode_set_t mode_set;
  /** PCM: 10/20/30, AMR: 20/40/60/80, ILBC: 20/30, G.723: 30, G.729: 10/20 */
  uint16 ptime;
  /** UPD form */
  // upd_form_t form;
};

/**
@brief      Codec Mode Request data

Data needed to perform rate control.
*/
struct rtp_cmr_data_t {
  bf16 is_ok_to_change : 1; /**< Output control event is allowed to be generated if TRUE */
  bf16 is_cmr_received : 1; /**< CMR has been received in ingress direction if TRUE */
  bf16 spare           : 14; /**< Bit-field padding */

  upd_mode_amr_t previous; /**< Mode in previous control output event */

  bf16 previous_ingress_mode : 4;  /**< Previous mode in ingress (#upd_mode_amr_t) */
  bf16 is_urgent_cmr_sent    : 1;  /**< Urgent CMR is sent if TRUE, not sent if FALSE */
  bf16 urgent_cmr_counter    : 11; /**< Counter for re-sending urgent CMR */

  uint8 egress;  /**< CMR value used in the AMR RTP header in egress direction */
  uint8 ingress; /**< CMR value of the previous AMR RTP header in ingress direction */
  uint8 ingress_t;

  bf8 ingress_cam_offset_previous : 4;
  bf8 ingress_cam_offset : 4;
};

struct context_t {
	struct params_t           params;
	struct rtp_cmr_data_t     rtp_cmr_data;
}; 

/*****************************************************************************************/
/****    5 LOCAL FUNCTION PROTOTYPES    **************************************************/

MGW_LOCAL uint16
get_evs_hdr_format(uint16 rtp_payload_size_1, uint16 evs_mode);

MGW_LOCAL uint16
get_evs_wbio_format(uint16 rtp_payload_size_1);

MGW_LOCAL uint16
get_evs_primary_format(uint16 rtp_payload_size_1);

MGW_LOCAL bool16
is_wbio_hf_sid(uint16 *rtp_payload);

MGW_LOCAL uint16
get_compact_evs_mode(uint16 rtp_payload_size);

MGW_LOCAL void
create_evs_upd_packet_compact(uint16 *rtp_payload,
                              uint16 rtp_payload_size, upd_packet_t **upd_packet);

MGW_LOCAL uint16
get_evs_compact_io_mode_from_payload_size(uint16 rtp_payload_size);

MGW_LOCAL uint16
get_evs_compact_primary_mode_from_payload_size(uint16 rtp_payload_size, uint16 *upd_payload);

MGW_LOCAL uint16
get_evs_amr_wb_io_payload_size_from_mode(uint16 mode);

MGW_LOCAL upd_type_t
get_evs_compact_primary_upd_type(uint16 rtp_payload_size, uint16 *upd_payload);

MGW_LOCAL uint8
map_cam_offset(uint8 cmr_t, uint8 cmr_d);

MGW_LOCAL uint16
map_upd_mode(uint16 cmr_mode, uint16 upd_mode_in);

MGW_LOCAL uint16
l_mgw_get_highest_evs_mode(uint16 ft_mode, mode_set_t mode_set);

MGW_LOCAL bool16
check_if_cmr(uint16 *rtp_payload);

MGW_LOCAL uint16
get_egress_evs_cmr(upd_packet_t *upd_packet, uint16 old_cmr, struct params_t *params, uint16 evs_mode);

MGW_LOCAL void
update_evs_cmr_data(struct rtp_cmr_data_t *rtp_cmr_data, struct params_t *params,
                    uint16 *rtp_payload, upd_packet_t *upd_packet,
                    uint16 evs_mode, uint16 evs_hdr_fmt);

MGW_LOCAL const char *
str_evs_mode(uint16 evs_mode);

MGW_LOCAL const char *
str_evs_format_wbio(uint16 evs_format);

MGW_LOCAL const char *
str_evs_format_primary(uint16 evs_format);

MGW_LOCAL const char *
str_evs_format(uint16 evs_format, uint16 evs_mode);

MGW_LOCAL upd_packet_t *
l_uph_get_scratch_lin_08_khz_pcm_upd_packet(void);

MGW_LOCAL upd_packet_t *
l_uph_get_scratch_lin_16_khz_pcm_upd_packet(void);

/*****************************************************************************************/
/****    6 FUNCTIONS    ******************************************************************/

int evs_ingress_process(void *buff, int len, void *buf_out, int *len_out_8)
{
	struct context_t context;
	struct params_t *params = &(context.params);
	struct rtp_cmr_data_t *rtp_cmr_data = &(context.rtp_cmr_data);

	memset(&context, 0, sizeof(context));
	params->evs_mode = UPD_EVS_PRIMARY_MODE;
  params->evs_format = EVS_HDR_FMT_COMPACT;
  params->ptime = 20;
  params->mode_set = 0;
  params->br = UPD_MODE_EVS_PRIMARY_24400;

  upd_packet_t *upd_packet[M_MB_IPT_MAX_EVS_FRAMES_PER_PACKET] = { 0 };
  upd_packet_t *lin_pcm_upd_packet[M_MB_IPT_MAX_EVS_FRAMES_PER_PACKET] = { 0 };
  upd_packet_t *lin_16_khz_pcm_upd_packet[M_MB_IPT_MAX_EVS_FRAMES_PER_PACKET] = { 0 };
  uint16 *rtp_payload = buff;
  uint16 rtp_payload_size_8 = len;
	uint16 evs_mode = params->evs_mode; /* config with rtp.evs_mode_switch */
	uint16 evs_hdr_fmt;
	int i = 0;

  debugf("evs_ingress_process\n");
	debugf("EVS rtp_payload_size_8: %d\n", rtp_payload_size_8);


  /* step 1: get EVS head format with rtp payload size */
	evs_hdr_fmt = get_evs_hdr_format(SIZE_8_TO_1(rtp_payload_size_8), evs_mode);
  /* evs primary compact format has the same payload size as evs wbio head full SID */
	if (evs_hdr_fmt == EVS_HDR_FMT_PT_SIZE_56) {
    if (is_wbio_hf_sid(rtp_payload)) {
      evs_hdr_fmt = EVS_HDR_FMT_FULL;
    } else {
      evs_hdr_fmt = EVS_HDR_FMT_COMPACT;
    }
  }

  /* step 2: create upd packet */
  if ((evs_hdr_fmt == EVS_HDR_FMT_COMPACT)) {
    create_evs_upd_packet_compact(rtp_payload, SIZE_8_TO_1(rtp_payload_size_8), upd_packet);
    if (get_compact_evs_mode(SIZE_8_TO_1(rtp_payload_size_8)) == EVS_WBIO_MODE) {
      update_evs_cmr_data(rtp_cmr_data, params, rtp_payload, *upd_packet, EVS_WBIO_MODE, evs_hdr_fmt);
    }
  } else if ((evs_hdr_fmt == EVS_HDR_FMT_FULL)) {
		errorf("TODO ....\n");
	}

	debugf("EVS evs_mode: %s(%d)\n", str_evs_mode(evs_mode), evs_mode);
	debugf("EVS evs_hdr_fmt: %s(%d)\n", evs_hdr_fmt == EVS_HDR_FMT_COMPACT ? "compact" : "headful", evs_hdr_fmt);
	for (i = 0; i < ARRAY_SIZE(upd_packet); i++) {
		if (upd_packet[i]) {
			const upd_packet_t *p = upd_packet[i];
			debugf("upd_packet[%d]: codec:%s(%d), type:%s(%d), mode:%s(%d), form:%s(%d)\n",
				i, str_upd_codec(p->codec), p->codec,
				str_upd_type(p->type), p->type,
				str_evs_format(p->mode, p->form), p->mode,
				str_evs_mode(p->form), p->form);
		}
	}

  /* step 3: transcoding */
  static codec_dec_data_t *evs_dec = NULL;
  ecode_t ret = EA_NO_ERRORS;
  if (evs_dec == NULL) {
    init_intel_ipp(); // init
    evs_dec = (codec_dec_data_t *)malloc(sizeof(*evs_dec));
    memset(evs_dec, 0, sizeof(*evs_dec));
    ret = m_evs_dec_init(evs_dec);
    CHECK_ERROR("m_evs_dec_init", ret, evs_dec->extra_ecode);
  }
  for (i = 0; i < ARRAY_SIZE(upd_packet); i++) {
    if (upd_packet[i]) {
      lin_16_khz_pcm_upd_packet[i] = l_uph_get_scratch_lin_16_khz_pcm_upd_packet();
      evs_dec->ctrl_io.output_sample_rate_hz = L_UPD_WB_SAMPLE_RATE_HZ;
      evs_dec->data_io.upd_input_codec = upd_packet[i];
      evs_dec->data_io.upd_lin_pcm = lin_16_khz_pcm_upd_packet[i];
      ret = m_evs_dec(evs_dec);
      CHECK_ERROR("m_evs_dec", ret, evs_dec->extra_ecode);
    }
  }

  /* step 4: sample rate converting */
  static m_src_down_data *master = NULL;
  if (master == NULL) {
    master = (m_src_down_data *)malloc(sizeof(*master));
    memset(master, 0, sizeof(*master));
    ret = m_src_down_init(master);
    CHECK_ERROR("m_src_down_init", ret, master->extra_ecode);
  }
  for (i = 0; i < ARRAY_SIZE(upd_packet); i++) {
    if (upd_packet[i]) {
      lin_pcm_upd_packet[i] = l_uph_get_scratch_lin_08_khz_pcm_upd_packet();
      master->upd_in = lin_16_khz_pcm_upd_packet[i];
      master->upd_out = lin_pcm_upd_packet[i];
      ret = m_src_down(master);
      CHECK_ERROR("m_src_down", ret, master->extra_ecode);
    }
  }

  /* step 5: return PCM data */
  if (lin_pcm_upd_packet[0]) {
    void *rtp_paylod = l_upd_get_payload(lin_pcm_upd_packet[0]);
    uint16 output_data_buffer_size_8 =
      SIZE_1_TO_8(l_upd_get_payload_size_1(lin_pcm_upd_packet[0]));
    memcpy(buf_out, rtp_paylod, output_data_buffer_size_8);
    *len_out_8 = output_data_buffer_size_8;
  }

  return 0;
}

MGW_LOCAL uint16
get_evs_hdr_format(uint16 rtp_payload_size_1, uint16 evs_mode)
{
  uint16 evs_hdr_format = EVS_HDR_FMT_FULL;

  if (evs_mode == UPD_EVS_WBIO_MODE) {
    evs_hdr_format = get_evs_wbio_format(rtp_payload_size_1);
  } else {
    /* primary mode */
    evs_hdr_format = get_evs_primary_format(rtp_payload_size_1);
  }

  return evs_hdr_format;
}

MGW_LOCAL uint16
get_evs_wbio_format(uint16 rtp_payload_size_1)
{
  uint16 evs_wbio_format = EVS_HDR_FMT_FULL;

  switch (rtp_payload_size_1) {
  case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_136:
  case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_184:
  case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_256:
  case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_288:
  case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_320:
  case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_368:
  case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_400:
  case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_464:
  case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_480:
    evs_wbio_format = EVS_HDR_FMT_COMPACT;
    break;
  case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_56:
    evs_wbio_format = EVS_HDR_FMT_PT_SIZE_56;
    break;
  default:
    evs_wbio_format = EVS_HDR_FMT_FULL;
    break;
  }

  return evs_wbio_format;
}

MGW_LOCAL uint16
get_evs_primary_format(uint16 rtp_payload_size_1)
{
  uint16 evs_primary_format = EVS_HDR_FMT_FULL;

  switch (rtp_payload_size_1) {
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_48_SID:
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_144:
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_160:
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_192:
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_264:
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_328:
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_488:
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_640:
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_960:
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_1280:
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_1920:
  case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_2560:
    evs_primary_format = EVS_HDR_FMT_COMPACT;
    break;
  case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_56:
    evs_primary_format = EVS_HDR_FMT_PT_SIZE_56;
    break;
  default:
    evs_primary_format = EVS_HDR_FMT_FULL;
    break;
  }

  return evs_primary_format;
}

/*****************************************************************************************/
/**
@brief      identify whether it is evs wbio header-full payload or not
@param      rtp_payload RTP payload
*/
MGW_LOCAL bool16
is_wbio_hf_sid(uint16 *rtp_payload)
{
  bool16 wbio_hf_sid = FALSE;
  uint16 temp_data[2] = {0};

  l_mgw_or_bit_buf(rtp_payload, 0, temp_data, 15, 1);
  if (temp_data[0] == 1) {
    wbio_hf_sid = TRUE;
  }

  return wbio_hf_sid;
}

/*****************************************************************************************/
/**
@brief      Create EVS compact UPD packet(s)

Maps RTP payload to UPD packet(s).

@param      rtp_payload RTP payload
@param      header_size_1 Size of payload header in bits
@param      upd_packet Mapping target(s)
*/
MGW_LOCAL void
create_evs_upd_packet_compact(uint16 *rtp_payload,
                              uint16 rtp_payload_size, upd_packet_t **upd_packet)
{
  uint16 read_bits = 0;
  uint16 speech_data_size_1 = 0;
  uint16 evs_mode = 0;
  uint16 *upd_payload = NULL;
  uint16 *upd_buffer =
  l_scratch_get_buffer(L_SCRATCH_ID_MB_IPT_DATA_IO,
                       M_MB_IPT_MAX_SINGLE_EVS_DATA_OUTPUT_SIZE_16);
  (void) l_mgw_memset_16(upd_buffer, 0, M_MB_IPT_MAX_SINGLE_EVS_DATA_OUTPUT_SIZE_16);

  upd_packet[0] = (upd_packet_t *) upd_buffer;

  /* it should sames with the configured value */
  evs_mode = get_compact_evs_mode(rtp_payload_size);

  /* fill upd payload
   * while is evs amrwb io mode with compact format in trfo,
   * the last bit need to re-shuffling to the last
   * */
  upd_payload = (uint16 *) l_upd_get_payload(upd_packet[0]);
  if (evs_mode == EVS_WBIO_MODE) {
    upd_packet[0]->codec = UPD_CODEC_EVS;
    upd_packet[0]->mode = get_evs_compact_io_mode_from_payload_size(rtp_payload_size);
    upd_packet[0]->type = UPD_TYPE_SPEECH_GOOD;
    upd_packet[0]->form = evs_mode;
    upd_packet[0]->padding = 0;

    if ((upd_packet[0]->mode) <= UPD_MODE_EVS_WBIO_23850) {
      read_bits = EVS_COMPACT_CMR_ENTRY_SIZE;
      speech_data_size_1 = get_evs_amr_wb_io_payload_size_from_mode((uint16)(upd_packet[0]->mode));
      if (speech_data_size_1 > 0) {
        /* EVS AMRWB IO Compact speech sequence |1|2|3|4|.......|last|0| */
        l_mgw_or_bit_buf(rtp_payload, read_bits+(speech_data_size_1-1), upd_payload, 0, 1);
        l_mgw_or_bit_buf(rtp_payload, read_bits, upd_payload, 1, speech_data_size_1-1);
      }
    }
  } else {
    l_mgw_or_bit_buf(rtp_payload, 0, upd_payload, 0, rtp_payload_size);

    upd_packet[0]->codec = UPD_CODEC_EVS;
    upd_packet[0]->mode = get_evs_compact_primary_mode_from_payload_size(rtp_payload_size, upd_payload);
    upd_packet[0]->type = get_evs_compact_primary_upd_type(rtp_payload_size, upd_payload);
    upd_packet[0]->form = evs_mode;
    upd_packet[0]->padding = 0;
  }
}

MGW_LOCAL uint16
get_compact_evs_mode(uint16 rtp_payload_size)
{
  uint16 evs_mode = EVS_UNKNOW_MODE;

  switch (rtp_payload_size) {
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_48_SID:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_56:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_144:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_160:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_192:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_264:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_328:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_488:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_640:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_960:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_1280:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_1920:
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_2560:
      evs_mode = EVS_PRIMARY_MODE;
      break;
    case EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_136:
    case EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_184:
    case EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_256:
    case EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_288:
    case EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_320:
    case EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_368:
    case EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_400:
    case EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_464:
    case EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_480:
      evs_mode = EVS_WBIO_MODE;
      break;
    default :
      evs_mode = EVS_UNKNOW_MODE;
      break;
  }

  return evs_mode;
}
/*****************************************************************************************/
/*
get evs io upd mode for compact
*/
MGW_LOCAL uint16
get_evs_compact_io_mode_from_payload_size(uint16 rtp_payload_size)
{
  uint16 upd_mode = UPD_MODE_EVS_NOT_APPLICABLE;
  switch (rtp_payload_size)
  {
    case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_136:
        upd_mode = UPD_MODE_EVS_WBIO_6600;
        break;
    case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_184:
        upd_mode = UPD_MODE_EVS_WBIO_8850;
        break;
    case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_256:
        upd_mode = UPD_MODE_EVS_WBIO_12650;
        break;
    case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_288:
        upd_mode = UPD_MODE_EVS_WBIO_14250;
        break;
    case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_320:
        upd_mode = UPD_MODE_EVS_WBIO_15850;
        break;
    case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_368:
        upd_mode = UPD_MODE_EVS_WBIO_18250;
        break;
    case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_400:
        upd_mode = UPD_MODE_EVS_WBIO_19850;
        break;
    case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_464:
        upd_mode = UPD_MODE_EVS_WBIO_23050;
        break;
    case  EVS_RTP_COMPACT_WBIO_PAYLOAD_SIZE_480:
        upd_mode = UPD_MODE_EVS_WBIO_23850;
        break;
    default :
        upd_mode = UPD_MODE_EVS_NOT_APPLICABLE;
        break;
  }
  return upd_mode;
}


/*****************************************************************************************/
/*
get evs upd mode for compact
*/
MGW_LOCAL uint16
get_evs_compact_primary_mode_from_payload_size(uint16 rtp_payload_size, uint16 *upd_payload)
{
  uint16 upd_mode = UPD_MODE_EVS_NOT_APPLICABLE;
  switch (rtp_payload_size)
  {
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_48_SID:
        /*upd_mode = UPD_MODE_SPECIALY_EVS_PRIMARY_2400*/
        upd_mode = l_upd_get_evs_primary_sid_mode(upd_payload);
        break;
    case EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_56:
        upd_mode = UPD_MODE_EVS_PRIMARY_2800;
        break;
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_144:
        upd_mode = UPD_MODE_EVS_PRIMARY_7200; 
        break;
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_160:
        upd_mode = UPD_MODE_EVS_PRIMARY_8000;
        break;
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_192:
        upd_mode = UPD_MODE_EVS_PRIMARY_9600;
        break;
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_264:
        upd_mode = UPD_MODE_EVS_PRIMARY_13200;
        break;
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_328:
        upd_mode = UPD_MODE_EVS_PRIMARY_16400;
        break;
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_488:
        upd_mode = UPD_MODE_EVS_PRIMARY_24400;
        break;
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_640:
        upd_mode = UPD_MODE_EVS_PRIMARY_32000;
        break;
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_960:
        upd_mode = UPD_MODE_EVS_PRIMARY_48000;
        break;
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_1280:
        upd_mode = UPD_MODE_EVS_PRIMARY_64000;
        break;
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_1920:
        upd_mode = UPD_MODE_EVS_PRIMARY_96000;
        break;
    case  EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_2560:
        upd_mode = UPD_MODE_EVS_PRIMARY_128000;
        break;
    default :
        upd_mode = UPD_MODE_EVS_NOT_APPLICABLE;
        break;
  }
  return upd_mode;
}

MGW_LOCAL uint16
get_evs_amr_wb_io_payload_size_from_mode(uint16 mode)
{
  uint16 payload_size = 0;

  switch (mode)
  {
    case  UPD_MODE_EVS_WBIO_6600:
        payload_size = UPD_PAYLOAD_EVS_132;
        break;
    case  UPD_MODE_EVS_WBIO_8850:
        payload_size = UPD_PAYLOAD_EVS_177;
        break;
    case  UPD_MODE_EVS_WBIO_12650:
        payload_size = UPD_PAYLOAD_EVS_253;
        break;
    case  UPD_MODE_EVS_WBIO_14250:
        payload_size = UPD_PAYLOAD_EVS_285;
        break;
    case  UPD_MODE_EVS_WBIO_15850:
        payload_size = UPD_PAYLOAD_EVS_317;
        break;
    case  UPD_MODE_EVS_WBIO_18250:
        payload_size = UPD_PAYLOAD_EVS_365;
        break;
    case  UPD_MODE_EVS_WBIO_19850:
        payload_size = UPD_PAYLOAD_EVS_397;
        break;
    case  UPD_MODE_EVS_WBIO_23050:
        payload_size = UPD_PAYLOAD_EVS_461;
        break;
    case  UPD_MODE_EVS_WBIO_23850:
        payload_size = UPD_PAYLOAD_EVS_477;
        break;
    default :
        payload_size = 0;
        break;
  }

  return payload_size;
}

/*****************************************************************************************/
/*
get evs primary upd type for compact
*/
MGW_LOCAL upd_type_t
get_evs_compact_primary_upd_type(uint16 rtp_payload_size, uint16 *upd_payload)
{
  upd_type_t upd_type = UPD_TYPE_NOT_APPLICABLE;

  if (rtp_payload_size == EVS_RTP_COMPACT_PRIMARY_PAYLOAD_SIZE_48_SID) {
    /*upd_type = UPD_TYPE_SID_UPDATE;*/
    upd_type = l_upd_get_evs_primary_sid_type(upd_payload);
  } else {
    upd_type = UPD_TYPE_SPEECH_GOOD;
  }

  return upd_type;
}

/*****************************************************************************************/
/**
@brief      Updates evs CMR data

*/
MGW_LOCAL void
update_evs_cmr_data(struct rtp_cmr_data_t *rtp_cmr_data, struct params_t *params,
                    uint16 *rtp_payload, upd_packet_t *upd_packet,
                    uint16 evs_mode, uint16 evs_hdr_fmt)
{
	if (evs_hdr_fmt == EVS_HDR_FMT_COMPACT && evs_mode == UPD_EVS_WBIO_MODE) {
		//update_evs_cmr_data_in_wbio_compact(rtp_cmr_data, upd_packet, evs_mode, rtp_payload);
  	rtp_cmr_data->previous_ingress_mode = upd_packet->mode; /* ??? */
  	rtp_cmr_data->egress = get_egress_evs_cmr(upd_packet, rtp_cmr_data->egress, params, evs_mode);
  	rtp_cmr_data->ingress = map_upd_mode(((rtp_payload[0] >> 13) & 0x7), upd_packet->mode);
  	rtp_cmr_data->is_cmr_received = TRUE;
	} else if (evs_hdr_fmt == EVS_HDR_FMT_FULL) {
		if (evs_mode == UPD_EVS_WBIO_MODE || evs_mode == UPD_EVS_PRIMARY_MODE) {
			// update_evs_cmr_data_in_headfull(rtp_cmr_data, upd_packet, evs_mode, rtp_payload);
			if (check_if_cmr(rtp_payload)) { /*8 BIT CMR GET BIT4 TO BIT7*/
    		rtp_cmr_data->previous_ingress_mode = upd_packet->mode;
    		rtp_cmr_data->egress = get_egress_evs_cmr(upd_packet, rtp_cmr_data->egress, params, evs_mode);
    		rtp_cmr_data->ingress = (rtp_payload[0] >> 8) & 0x000F;
    		rtp_cmr_data->ingress_t = (rtp_payload[0] >> 11) &0x0007;
    		rtp_cmr_data->ingress_cam_offset_previous = rtp_cmr_data->ingress_cam_offset;
    		rtp_cmr_data->ingress_cam_offset = map_cam_offset(rtp_cmr_data->ingress_t,
                                                      rtp_cmr_data->ingress);
    		rtp_cmr_data->is_cmr_received = TRUE;
  		}
		}
	}
}

/*****************************************************************************************/
/**
@brief      Checks whether full header mode or compact mode for 56 bit situation
@note       First bit 1 have CMR
@param      rtp_payload RTP payload
*/
MGW_LOCAL bool16
check_if_cmr(uint16 *rtp_payload)
{
  uint16 temp_data[2] = {0};
  bool16 is_cmr = FALSE;

  l_mgw_or_bit_buf(rtp_payload, 0, temp_data, 15, 1);
  if(temp_data[0] == 1){
    is_cmr = TRUE;
  }

  return is_cmr;
}

MGW_LOCAL uint16
get_egress_evs_cmr(upd_packet_t *upd_packet, uint16 old_cmr, struct params_t *params, uint16 evs_mode)
{
  uint16 mode_set;
  if (evs_mode == UPD_EVS_WBIO_MODE){
    mode_set = params->mode_set;
  } else {
    mode_set = params->br;
  }

  if (old_cmr == CMR_FT_NO_DATA) { /* no mode request present */
    if (upd_packet->mode != UPD_MODE_EVS_NOT_APPLICABLE) {
      if (l_mgw_is_mode_in_mode_set(upd_packet->mode, mode_set) == FALSE) {
        return l_mgw_get_highest_evs_mode(evs_mode, mode_set);
      }
    }
  }

  return old_cmr;
}

/*****************************************************************************************/
/**
@brief      Gets the highest EVS mode among given mode set

Gets the highest EVS mode among given mode set.

@param      toc Used ft_mode
@param      mode_set Set of modes, LSB bit is the lowest mode etc.
@return     Highest mode among given mode set
@note       Added by Panu Ranta
*/
MGW_LOCAL uint16
l_mgw_get_highest_evs_mode(uint16 ft_mode, mode_set_t mode_set)
{
  uint16 i = 0;
  uint16 number_of_modes = 0;
  upd_mode_amr_t max_mode = l_upd_get_max_evs_mode(ft_mode);
  if (ft_mode == UPD_EVS_WBIO_MODE) {
    number_of_modes = UPD_NUMBER_OF_EVS_WBIO_MODES;
  } else if (ft_mode == UPD_EVS_PRIMARY_MODE) {
    number_of_modes = UPD_NUMBER_OF_EVS_PRIMARY_MODES;
  }
  for (i = 0; i < number_of_modes; i++) {
    if (((mode_set >> (max_mode - i)) & 0x0001) == 1) {
      return (max_mode - i);
    }
  }
  return UPD_MODE_EVS_NOT_APPLICABLE;
}

/*****************************************************************************************/
/**
@brief      map to upd mode

@param cmr_mode    mode of CMR header
@param upd_mode_in
@return upd mode
*/
MGW_LOCAL uint16
map_upd_mode(uint16 cmr_mode, uint16 upd_mode_in)
{
  uint16 upd_mode = UPD_MODE_EVS_PRIMARY_NOT_APPLICABLE;
  switch(cmr_mode){
    case CMR_COMPACT_660:
      upd_mode = UPD_MODE_EVS_WBIO_6600;
      break;
    case CMR_COMPACT_885:
      upd_mode = UPD_MODE_EVS_WBIO_8850;
      break;
    case CMR_COMPACT_1265:
      upd_mode = UPD_MODE_EVS_WBIO_12650;
      break;
    case CMR_COMPACT_1585:
      upd_mode = UPD_MODE_EVS_WBIO_15850;
      break;
    case CMR_COMPACT_1825:
      upd_mode = UPD_MODE_EVS_WBIO_18250;
      break;
    case CMR_COMPACT_2305:
      upd_mode = UPD_MODE_EVS_WBIO_23050;
      break;
    case CMR_COMPACT_2385:
      upd_mode = UPD_MODE_EVS_WBIO_23850;
      break;
    case CMR_COMPACT_NONE:
      upd_mode = UPD_MODE_EVS_PRIMARY_NOT_APPLICABLE;
      break;
    default:
      break;
  }
  if ((upd_mode_in == UPD_MODE_EVS_WBIO_14250) && (cmr_mode == CMR_COMPACT_1265)) {
    upd_mode = UPD_MODE_EVS_WBIO_14250;
  } else if ((upd_mode_in == UPD_MODE_EVS_WBIO_19850) && (cmr_mode == CMR_COMPACT_1825)) {
    upd_mode = UPD_MODE_EVS_WBIO_19850;
  } else {
    /*do nothing*/
  }
  return upd_mode;
}

MGW_LOCAL uint8
map_cam_offset(uint8 cmr_t, uint8 cmr_d)
{
  uint8 cam_offset = CHANNEL_AWARE_MODE_OFFSET_INVALID;

  if ((cmr_t == CMR_T_101) || (cmr_t == CMR_T_110)) {
    switch (cmr_d) {
    case CMR_CHANNEL_AWARE_MODE_0000:
    case CMR_CHANNEL_AWARE_MODE_0100:
      cam_offset = CHANNEL_AWARE_MODE_OFFSET2;
      break;
    case CMR_CHANNEL_AWARE_MODE_0001:
    case CMR_CHANNEL_AWARE_MODE_0101:
      cam_offset = CHANNEL_AWARE_MODE_OFFSET3;
      break;
    case CMR_CHANNEL_AWARE_MODE_0010:
    case CMR_CHANNEL_AWARE_MODE_0110:
      cam_offset = CHANNEL_AWARE_MODE_OFFSET5;
      break;
    case CMR_CHANNEL_AWARE_MODE_0011:
    case CMR_CHANNEL_AWARE_MODE_0111:
      cam_offset = CHANNEL_AWARE_MODE_OFFSET7;
      break;
    default:
      cam_offset = CHANNEL_AWARE_MODE_OFFSET_INVALID;
      break;
    }
  }
  return cam_offset;
}

MGW_LOCAL const char *
str_evs_mode(uint16 evs_mode)
{
	const str_table_t evs_mode_tab[] = {
		STR_TABLE_LINE(UPD_EVS_PRIMARY_MODE),
		STR_TABLE_LINE(UPD_EVS_WBIO_MODE),
		STR_TABLE_END
	};
	return value_to_str(evs_mode, evs_mode_tab);
}

MGW_LOCAL const char *
str_evs_format_primary(uint16 evs_format)
{
	const str_table_t evs_format_tab[] = {
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_2800),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_7200),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_8000),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_9600),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_13200),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_16400),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_24400),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_32000),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_48000),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_64000),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_96000),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_128000),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_SID),
		STR_TABLE_LINE(UPD_MODE_EVS_PRIMARY_NOT_APPLICABLE),
		STR_TABLE_END
	};
	return value_to_str(evs_format, evs_format_tab);
}

MGW_LOCAL const char *
str_evs_format_wbio(uint16 evs_format)
{
	const str_table_t evs_format_tab[] = {
		STR_TABLE_LINE(UPD_MODE_EVS_WBIO_6600),
		STR_TABLE_LINE(UPD_MODE_EVS_WBIO_8850),
		STR_TABLE_LINE(UPD_MODE_EVS_WBIO_12650),
		STR_TABLE_LINE(UPD_MODE_EVS_WBIO_14250),
		STR_TABLE_LINE(UPD_MODE_EVS_WBIO_15850),
		STR_TABLE_LINE(UPD_MODE_EVS_WBIO_18250),
		STR_TABLE_LINE(UPD_MODE_EVS_WBIO_19850),
		STR_TABLE_LINE(UPD_MODE_EVS_WBIO_23050),
		STR_TABLE_LINE(UPD_MODE_EVS_WBIO_23850),
		STR_TABLE_LINE(UPD_MODE_EVS_WBIO_SID),
		STR_TABLE_LINE(UPD_MODE_EVS_WBIO_NOT_APPLICABLE),
		STR_TABLE_END
	};
	return value_to_str(evs_format, evs_format_tab);
}

MGW_LOCAL const char *
str_evs_format(uint16 evs_format, uint16 evs_mode)
{
	if (UPD_EVS_PRIMARY_MODE == evs_mode) {
		return str_evs_format_primary(evs_format);
	} else if (UPD_EVS_WBIO_MODE == evs_mode) {
		return str_evs_format_wbio(evs_format);
	}
	return "null";
}

/*****************************************************************************************/
/**
@brief      Gets scratch memory linear 8 kHz PCM UPD packet

@return     Scratch memory linear 8 kHz PCM UPD packet
*/
MGW_LOCAL upd_packet_t *
l_uph_get_scratch_lin_08_khz_pcm_upd_packet(void)
{
  uint16 *scratch =
    l_scratch_get_buffer(L_SCRATCH_ID_UPH_LIN_08_KHZ_PCM,
                         SCRATCH_LIN_08_KHZ_PCM_UPD_PACKET_ALIGNMENT_SIZE_16 +
                         SCRATCH_LIN_08_KHZ_PCM_UPD_PACKET_SIZE_16);

  return (upd_packet_t *) (scratch + SCRATCH_LIN_08_KHZ_PCM_UPD_PACKET_ALIGNMENT_SIZE_16);
}

/*****************************************************************************************/
/**
@brief      Gets scratch memory linear 16 kHz PCM UPD packet

@return     Scratch memory linear 16 kHz PCM UPD packet
*/
MGW_LOCAL upd_packet_t *
l_uph_get_scratch_lin_16_khz_pcm_upd_packet(void)
{
  uint16 *scratch =
    l_scratch_get_buffer(L_SCRATCH_ID_UPH_LIN_16_KHZ_PCM,
                         SCRATCH_LIN_16_KHZ_PCM_UPD_PACKET_ALIGNMENT_SIZE_16 +
                         SCRATCH_LIN_16_KHZ_PCM_UPD_PACKET_SIZE_16);

  return (upd_packet_t *) (scratch + SCRATCH_LIN_16_KHZ_PCM_UPD_PACKET_ALIGNMENT_SIZE_16);
}

/*****************************************************************************************/


