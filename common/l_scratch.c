/*****************************************************************************************/
/**
@brief Scratch memory
*/

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include "l_scratch.h"
#include <string.h>

/*****************************************************************************************/
/****    2 LOCAL CONSTANTS    ************************************************************/

/** Number of scratch buffers in scratch table. */
#define NUM_SCRATCH_BUFFS (sizeof(scratch) / sizeof(struct scratch_buffer_data))

/** Check pattern size */
#define CHECK_PATTERNS_SIZE_16 (sizeof_16(header) + sizeof_16(trailer))

/** Ater output scratch size in 16-bit words */
#define ATER_OUTPUT_SCRATCH_SIZE_16 82
/** Module internal scratch size in 16-bit words */
#define MOD_INTERNAL_SCRATCH_SIZE_16 600
/** UPH linear 8 kHz PCM scratch size in 16-bit words */
#define UPH_LIN_08_KHZ_PCM_SCRATCH_SIZE_16 244
/** UPH linear 16 kHz PCM scratch size in 16-bit words */
#define UPH_LIN_16_KHZ_PCM_SCRATCH_SIZE_16 484
/** UPH UPD scratch size in 16-bit words */
#define UPH_UPD_SCRATCH_SIZE_16 306
/** UPH AEC scratch size in 16-bit words */
#define UPH_AEC_SCRATCH_SIZE_16 1460
/** UPH EC reference buffer PCM scratch size in 16-bit words */
#define UPH_EC_REF_PCM_SCRATCH_SIZE_16 20
/** UPH EC reference buffer linear PCM scratch size in 16-bit words */
#define UPH_EC_REF_LPCM_SCRATCH_SIZE_16 40
/** GVAD DARAM work memory scratch size in 16-bit words */
#define GVAD_1_DA_SCRATCH_SIZE_16 226
/** GVAD SARAM work memory scratch size in 16-bit words */
#define GVAD_2_SA_SCRATCH_SIZE_16 80
/** GVAD SARAM work memory scratch size in 16-bit words. Used only with c64x. */
#define GVAD_3_SA_SCRATCH_SIZE_16 462
/** UPPH egress scratch size in 16-bit words */
#define UPPH_EGRESS_SCRATCH_SIZE_16 80
/** Internal Interface Handler tone UPD packet scratch size in 16-bit words */
#define INT_IFH_TONE_UPD_SCRATCH_SIZE_16 50
/** SRC c6x work memory scratch size in 16-bit words */
#define SRC_C6X_SCRATCH_SIZE_16 1032
/** IPE scratch size in 16-bit words */
#define IPE_SCRATCH_SIZE_16 352
/** AEC c6x scratch size in 16-bit words short echo path model buffer */
#define AEC_C6X_SHORT_SCRATCH_SIZE_16 1160
/** AEC c6x scratch size in 16-bit words long echo path model buffer */
#define AEC_C6X_LONG_SCRATCH_SIZE_16 4400
/** BFH PLC algorithm c64x scratch size in 16-bit words */
#define BFH_C6X_SCRATCH_SIZE_16 294
/** Mb IPT module data I/O scratch size in 16-bit words */
#define MB_IPT_DATA_IO_SCRATCH_SIZE_16 655
/** Conference scratch size in 16-bit words */
#define CONFERENCE_SCRATCH_SIZE_16 42

#ifdef BOARD_X86_VIRTUAL
  /** Event scratch size in 16-bit words */
  #define EVENT_SCRATCH_SIZE_16 552
#else
  /** Event scratch size in 16-bit words */
  #define EVENT_SCRATCH_SIZE_16 456
#endif

/** 64-bit header supervision pattern */
MGW_LOCAL const uint32 header[]  = { 0x12345678, 0x98765432 };
/** 64-bit trailer supervision pattern */
MGW_LOCAL const uint32 trailer[] = { 0x98765432, 0x12345678 };

/*****************************************************************************************/
/****    3 LOCAL MACROS    ***************************************************************/

/** Get header address */
#define GET_HEADER(buffer) (buffer)
/** Gets scratch address */
#define GET_SUPERVISED_SCRATCH(buffer) ((void *) ((uint16 *) (buffer) + sizeof_16(header)))
/** Gets trailer address */
#define GET_TRAILER(buffer, allocated_size_16) \
  ((void *) ((uint16 *) (buffer) + ((allocated_size_16) - sizeof_16(trailer))))
/** Get scratch buffer size (incl. check pattern) in uint32[] */
#define GET_BUF_SIZE_32(buffer_size_16) \
  (SIZE_16_TO_32(buffer_size_16))
/** Get scratch buffer size (incl. check pattern) in uint32[] */
#define GET_SUPERVISED_BUF_SIZE_32(buffer_size_16) \
  (SIZE_16_TO_32((buffer_size_16) + CHECK_PATTERNS_SIZE_16))

/*****************************************************************************************/
/****    4 LOCAL DATA TYPES    ***********************************************************/

/**
@brief      Scratch buffer data
*/
struct scratch_buffer_data {
  uint32 *buffer;            /**< Scratch buffer */
  uint16 size_16;            /**< Size of the scratch buffer in 16-bit words */
  bool16 supervision_in_use; /**< True if buffer is supervised */
};

/*****************************************************************************************/
/****    Z LOCAL VARIABLES    ************************************************************/

/** Scratch buffer for Ater output */
MGW_LOCAL uint32 ater_output_scratch[GET_BUF_SIZE_32(ATER_OUTPUT_SCRATCH_SIZE_16)];
/** Scratch buffer for module internal */
MGW_LOCAL uint32 mod_internal_scratch[GET_SUPERVISED_BUF_SIZE_32(
                                      MOD_INTERNAL_SCRATCH_SIZE_16)];
/** Scratch buffer termination framework events */
MGW_LOCAL uint32 event_scratch[GET_SUPERVISED_BUF_SIZE_32(EVENT_SCRATCH_SIZE_16)];
/** Scratch buffer for UPH linear 8 kHz PCM */
MGW_LOCAL uint32 uph_lin_08_khz_pcm_scratch[GET_SUPERVISED_BUF_SIZE_32(
                                            UPH_LIN_08_KHZ_PCM_SCRATCH_SIZE_16)];
/** Scratch buffer for UPH linear 16 kHz PCM */
MGW_LOCAL uint32 uph_lin_16_khz_pcm_scratch[GET_SUPERVISED_BUF_SIZE_32(
                                            UPH_LIN_16_KHZ_PCM_SCRATCH_SIZE_16)];
/** Scratch buffer for UPH UPD */
MGW_LOCAL uint32 uph_upd_scratch[GET_SUPERVISED_BUF_SIZE_32(UPH_UPD_SCRATCH_SIZE_16)];
/** Scratch buffer for AEC */
MGW_LOCAL uint32 uph_aec_scratch[GET_BUF_SIZE_32(UPH_AEC_SCRATCH_SIZE_16)];
/** Scratch buffer for UPH EC reference buffer PCM */
MGW_LOCAL uint32 uph_ec_ref_pcm_scratch[GET_BUF_SIZE_32(UPH_EC_REF_PCM_SCRATCH_SIZE_16)];
/** Scratch buffer for UPH EC reference buffer linear PCM */
MGW_LOCAL uint32 uph_ec_ref_lpcm_scratch[GET_BUF_SIZE_32(UPH_EC_REF_LPCM_SCRATCH_SIZE_16)];
/** Scratch buffer for GVAD 1 DARAM work buffer */
MGW_LOCAL uint32 gvad_1_da_scratch[GET_BUF_SIZE_32(GVAD_1_DA_SCRATCH_SIZE_16)];
/** Scratch buffer for GVAD 2 SARAM/DARAM work buffer */
MGW_LOCAL uint32 gvad_2_sa_scratch[GET_BUF_SIZE_32(GVAD_2_SA_SCRATCH_SIZE_16)];
/** Scratch buffer for GVAD 3 SARAM/DARAM work buffer */
MGW_LOCAL uint32 gvad_3_sa_scratch[GET_BUF_SIZE_32(GVAD_3_SA_SCRATCH_SIZE_16)];
/** Scratch buffer for UPPH IuUP egress data */
MGW_LOCAL uint32
upph_egress_scratch[GET_SUPERVISED_BUF_SIZE_32(UPPH_EGRESS_SCRATCH_SIZE_16)];
/** Scratch buffer for Internal Interface Handler tone UPD packet */
MGW_LOCAL uint32 int_ifh_tone_upd_scratch[GET_BUF_SIZE_32(
                                          INT_IFH_TONE_UPD_SCRATCH_SIZE_16)];
/** Scratch buffer for SRC c6x module */
MGW_LOCAL uint32 src_c6x_scratch[GET_BUF_SIZE_32(SRC_C6X_SCRATCH_SIZE_16)];
/** Scratch buffer for IPE module */
MGW_LOCAL uint32 ipe_scratch[GET_SUPERVISED_BUF_SIZE_32(IPE_SCRATCH_SIZE_16)];
/** Scratch buffer for AEC module on c6x short echo path model buffer */
MGW_LOCAL uint32 aec_c6x_short_scratch[GET_BUF_SIZE_32(AEC_C6X_SHORT_SCRATCH_SIZE_16)];
/** Scratch buffer for AEC module on c6x long echo path model buffer */
MGW_LOCAL uint32 aec_c6x_long_scratch[GET_BUF_SIZE_32(AEC_C6X_LONG_SCRATCH_SIZE_16)];
/** Scratch buffer for BFH c6x module */
MGW_LOCAL uint32 bfh_c6x_scratch[GET_BUF_SIZE_32(BFH_C6X_SCRATCH_SIZE_16)];
/** Scratch buffer for Mb IPT module data I/O */
MGW_LOCAL uint32 mb_ipt_data_io_scratch[GET_BUF_SIZE_32(MB_IPT_DATA_IO_SCRATCH_SIZE_16)];
/** Scratch buffer for conference */
MGW_LOCAL uint32 conference_scratch[GET_BUF_SIZE_32(CONFERENCE_SCRATCH_SIZE_16)];

/** Scratch buffer data table */
MGW_LOCAL const struct scratch_buffer_data scratch[] =
{
  { ater_output_scratch,        sizeof_16(ater_output_scratch),       FALSE },
  { mod_internal_scratch,       sizeof_16(mod_internal_scratch),      TRUE },
  { event_scratch,              sizeof_16(event_scratch),             TRUE },
  { uph_lin_08_khz_pcm_scratch, sizeof_16(uph_lin_08_khz_pcm_scratch), TRUE },
  { uph_lin_16_khz_pcm_scratch, sizeof_16(uph_lin_16_khz_pcm_scratch), TRUE },
  { uph_upd_scratch,            sizeof_16(uph_upd_scratch),           TRUE },
  { uph_aec_scratch,            sizeof_16(uph_aec_scratch),           FALSE },
  { uph_ec_ref_pcm_scratch,     sizeof_16(uph_ec_ref_pcm_scratch),    FALSE },
  { uph_ec_ref_lpcm_scratch,    sizeof_16(uph_ec_ref_lpcm_scratch),   FALSE },
  { gvad_1_da_scratch,          sizeof_16(gvad_1_da_scratch),         FALSE },
  { gvad_2_sa_scratch,          sizeof_16(gvad_2_sa_scratch),         FALSE },
  { gvad_3_sa_scratch,          sizeof_16(gvad_3_sa_scratch),         FALSE },
  { upph_egress_scratch,        sizeof_16(upph_egress_scratch),       TRUE },
  { int_ifh_tone_upd_scratch,   sizeof_16(int_ifh_tone_upd_scratch),  FALSE },
  { src_c6x_scratch,            sizeof_16(src_c6x_scratch),           FALSE },
  { ipe_scratch,                sizeof_16(ipe_scratch),               FALSE },
  { aec_c6x_short_scratch,      sizeof_16(aec_c6x_short_scratch),     FALSE },
  { aec_c6x_long_scratch,       sizeof_16(aec_c6x_long_scratch),      FALSE },
  { bfh_c6x_scratch,            sizeof_16(bfh_c6x_scratch),           FALSE },
  { mb_ipt_data_io_scratch,     sizeof_16(mb_ipt_data_io_scratch),    FALSE },
  { conference_scratch,         sizeof_16(conference_scratch),        FALSE }
};

/*****************************************************************************************/
/****    5 LOCAL FUNCTION PROTOTYPES    **************************************************/

/*****************************************************************************************/
/****    6 FUNCTIONS    ******************************************************************/

/*****************************************************************************************/
/**
@brief      Gets scratch buffer based on the scratch identifier.

@param      scratch_id Identifier of the requested buffer
@param      size_16 Size of the requested buffer in 16-bit words
@return     Scratch buffer (aligned to uint32)
*/
MGW_GLOBAL void *
l_scratch_get_buffer(uint16 scratch_id, uint16 size_16)
{
  ecode_t ecode = EA_NO_ERRORS;
  uint32 *buffer = NULL;
  uint16 buffer_size = 0;

  if (scratch_id < NUM_SCRATCH_BUFFS) {
    buffer = scratch[scratch_id].buffer;
    if (buffer != NULL) {
      buffer_size = scratch[scratch_id].size_16;
      if (scratch[scratch_id].supervision_in_use == TRUE) {
        buffer_size -= CHECK_PATTERNS_SIZE_16;
        buffer = GET_SUPERVISED_SCRATCH(buffer);
      }
      if (size_16 > buffer_size) {
        ecode = E_L_SCRATCH_INVALID_SIZE;
      }
    } else {
      ecode = E_L_SCRATCH_UNEXPECTED_NULL_POINTER;
    }
  } else {
    ecode = E_L_SCRATCH_INVALID_ID;
  }

  if (ecode != EA_NO_ERRORS) {
    l_sys_report_error(l_sys_get_current_servid(), ecode,
                       ((uint32) size_16 << 16) | scratch_id);
  }

  return (void *) buffer;
}

/*****************************************************************************************/
/**
@brief      Inits scatch supervision

Writes start and endmarks to selected scratch buffers

*/
MGW_GLOBAL void
l_scratch_init_supervision(void)
{
  uint16 i = 0;
  uint16 size_16 = 0;
  uint32 *buffer = NULL;
  for (i = 0; i < NUM_SCRATCH_BUFFS; i++) {
    if ((scratch[i].supervision_in_use == TRUE) && (scratch[i].buffer != NULL)) {
      buffer = scratch[i].buffer;
      size_16 = scratch[i].size_16;
      (void) memcpy(GET_HEADER(buffer), header, sizeof(header));
      (void) memcpy(GET_TRAILER(buffer, size_16), trailer, sizeof(trailer));
    }
  }
}

/*****************************************************************************************/
/**
@brief      Scatch supervision

Checks start and endmarks and takes appropriate action!

@return     Fatal error on corruption
*/
MGW_GLOBAL void
l_scratch_supervision(void)
{
  uint16 i = 0;
  uint16 size_16 = 0;
  uint32 *buffer = NULL;

  for (i = 0; i < NUM_SCRATCH_BUFFS; i++) {
    if ((scratch[i].supervision_in_use == TRUE) && (scratch[i].buffer != NULL)) {
      buffer = scratch[i].buffer;
      size_16 = scratch[i].size_16;
      if (memcmp(GET_HEADER(buffer), header, sizeof(header)) != 0) {
        l_sys_report_error(l_sys_get_current_servid(), E_L_SCRATCH_HEADER_CORRUPTED,
                           ((uint32) size_16 << 16) | i);
      } else if (memcmp(GET_TRAILER(buffer, size_16), trailer, sizeof(trailer)) != 0) {
        l_sys_report_error(l_sys_get_current_servid(), E_L_SCRATCH_TRAILER_CORRUPTED,
                           ((uint32) size_16 << 16) | i);
      }
    }
  }
}

