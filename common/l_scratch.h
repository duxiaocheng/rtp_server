/*****************************************************************************************/
/**
@brief Scratch memory
*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/

#ifndef L_SCRATCH_H
#define L_SCRATCH_H

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include <type_def.h>
#include <l_mgw.h>
#include <l_error.h>

/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

/* FATAL ERROR MASK */
#define E_L_SCRATCH_FATAL     FATAL_MASK(0x052)
/* SEVERE ERROR MASK */
#define E_L_SCRATCH_SEVERE    SEVERE_MASK(0x052)
/* MINOR ERROR MASK */
#define E_L_SCRATCH_MINOR     MINOR_MASK(0x052)

/** Fatal error code for invalid scratch memory id.
    Extra: requested size in 16-bit words << 16 | scratch id */
#define E_L_SCRATCH_INVALID_ID               (E_L_SCRATCH_FATAL | ERROR_C_INTERNAL | 0x001)
/** Fatal error code for invalid scratch memory size.
    Extra: requested size in 16-bit words << 16 | scratch id */
#define E_L_SCRATCH_INVALID_SIZE             (E_L_SCRATCH_FATAL | ERROR_C_INTERNAL | 0x002)
/** Fatal error code for unexpected NULL pointer (scratch linked to NULL i.e. request not
    desired). Extra: requested size in 16-bit words << 16 | scratch id */
#define E_L_SCRATCH_UNEXPECTED_NULL_POINTER  (E_L_SCRATCH_FATAL | ERROR_C_INTERNAL | 0x003)
/** Fatal error code for scratch memory header corrupted in free.
    Extra: requested size in 16-bit words << 16 | scratch id */
#define E_L_SCRATCH_HEADER_CORRUPTED         (E_L_SCRATCH_FATAL | ERROR_C_INTERNAL | 0x004)
/** Fatal error code for scratch memory trailer corrupted in free.
    Extra: requested size in 16-bit words << 16 | scratch id */
#define E_L_SCRATCH_TRAILER_CORRUPTED        (E_L_SCRATCH_FATAL | ERROR_C_INTERNAL | 0x005)

/** Scratch identifier for Ater output scratch */
#define L_SCRATCH_ID_ATER_OUTPUT           0
/** Scratch identifier for module internal scratch */
#define L_SCRATCH_ID_MOD_INTERNAL          1
/** Scratch identifier for framework event scratch */
#define L_SCRATCH_ID_EVENT                 2
/** Scratch identifier for UPH linear 8 kHz PCM scratch */
#define L_SCRATCH_ID_UPH_LIN_08_KHZ_PCM    3
/** Scratch identifier for UPH linear 16 kHz PCM scratch */
#define L_SCRATCH_ID_UPH_LIN_16_KHZ_PCM    4
/** Scratch identifier for UPD scratch */
#define L_SCRATCH_ID_UPH_UPD               5
/** Scratch identifier for AEC scratch */
#define L_SCRATCH_ID_UPH_AEC               6
/** Scratch identifier for EC reference PCM scratch */
#define L_SCRATCH_ID_UPH_EC_REF_PCM        7
/** Scratch identifier for EC reference linear PCM scratch */
#define L_SCRATCH_ID_UPH_EC_REF_LPCM       8
/** Scratch identifier for GVAD 1 DARAM scratch */
#define L_SCRATCH_ID_GVAD_1_DA             9
/** Scratch identifier for GVAD 2 SARAM/DARAM scratch */
#define L_SCRATCH_ID_GVAD_2_SA             10
/** Scratch identifier for GVAD 3 SARAM/DARAM scratch */
#define L_SCRATCH_ID_GVAD_3_SA             11
/** Scratch identifier for UPPH egress data scratch */
#define L_SCRATCH_ID_UPPH_EGRESS           12
/** Scratch identifier for Internal Interface Handler tone UPD packet */
#define L_SCRATCH_ID_INT_IFH_TONE_UPD      13
/** Scratch identifier for SRC c6x scratch */
#define L_SCRATCH_ID_SRC_C6X               14
/** Scratch identifier for IPE scratch */
#define L_SCRATCH_ID_IPE                   15
/** Scratch identifier for AEC c6x short scratch */
#define L_SCRATCH_ID_AEC_C6X_SHORT         16
/** Scratch identifier for AEC c6x long scratch */
#define L_SCRATCH_ID_AEC_C6X_LONG          17
/** Scratch identifier for BFH c6x scratch */
#define L_SCRATCH_ID_BFH_C6X               18
/** Scratch identifier for Mb IPT module data I/O scratch */
#define L_SCRATCH_ID_MB_IPT_DATA_IO        19
/** Scratch identifier for conference scratch */
#define L_SCRATCH_ID_CONFERENCE            20
/** Scratch identifier for dtmf det of uph speech */
#define L_SCRATCH_ID_UPH_SPEECH_DTMF_DET   21
/** Scratch identifier for 16khz LIN codec data buffer uph speech */
#define L_SCRATCH_ID_UPH_SPEECH_LIN_16_KHZ 22

/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

/*****************************************************************************************/
/****    5 FUNCTION PROTOTYPES    ********************************************************/

MGW_GLOBAL void *
l_scratch_get_buffer(uint16 scratch_id, uint16 size_16);

MGW_GLOBAL void
l_scratch_init_supervision(void);

MGW_GLOBAL void
l_scratch_supervision(void);

/*****************************************************************************************/
#endif /* L_SCRATCH_H */


