/*****************************************************************************************/
/**
@brief EVS codec module header file.

*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/

#ifndef M_EVS_H
#define M_EVS_H

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include "codec.h"
#include <type_def.h>
#include <l_error.h>


/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

/* SERVER ERROR MASK */
#define E_M_EVS_SEVERE SEVERE_MASK(0x0D4)
#define E_M_EVS_MINOR  MINOR_MASK(0x0D4)

#define E_M_EVS_UNEXPECTED_NULL_POINTER       (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x001)
#define E_M_EVS_ENC_INIT_FAILED               (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x002)
#define E_M_EVS_ENC_CLOSE_FAILED              (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x003)
#define E_M_EVS_DEC_INIT_FAILED               (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x004)
#define E_M_EVS_DEC_CLOSE_FAILED              (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x005)

#define E_M_EVS_ENC_CONTROL_FAILED            (E_M_EVS_MINOR | ERROR_C_INTERNAL | 0x001)
#define E_M_EVS_ENC_PROCESS_FAILED            (E_M_EVS_MINOR | ERROR_C_INTERNAL | 0x002)
#define E_M_EVS_DEC_PROCESS_FAILED            (E_M_EVS_MINOR | ERROR_C_INTERNAL | 0x003)
#define E_M_EVS_DEC_BAD_FRAME                 (E_M_EVS_MINOR | ERROR_C_INTERNAL | 0x004)
#define E_M_EVS_DEC_NO_DATA_FRAME             (E_M_EVS_MINOR | ERROR_C_INTERNAL | 0x005)
#define E_M_EVS_DEC_TOO_BIG_FRAME             (E_M_EVS_MINOR | ERROR_C_INTERNAL | 0x006)

#define E_M_EVS_ENC_ALG_ALLOC_FAIL            (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x006)
#define E_M_EVS_ENC_CREATE_FAIL               (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x007)
#define E_M_EVS_DEC_ALG_ALLOC_FAIL            (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x008)
#define E_M_EVS_DEC_CREATE_FAIL               (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x009)
#define E_M_EVS_ENC_UNKNOWN_CODEC_MODE        (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x100)
#define E_M_EVS_INCORRECT_UPD_CODEC_TYPE      (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x101)
#define E_M_EVS_MEM_NULL_POINTER              (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x102)
#define E_M_EVS_DEC_UNKNOWN_CODEC_MODE        (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x200)
#define E_M_EVS_DEC_CONTROL_PROCESS_FAILED    (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x201)
#define E_M_EVS_DEC_DECODE_PROCESS_FAILED     (E_M_EVS_SEVERE | ERROR_C_INTERNAL | 0x202)

/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

#define ENC_SHARED_SCRATCH_SIZE_16 200 /**< Required encoder scratch buffer size*/
#define DEC_SHARED_SCRATCH_SIZE_16 200
#define SID_PAYLOAD_EVS_48         48
#define CMR_BIT_3                  3
#define ZERO_PADDING_1             1
#define ZERO_PADDING_4             4
#define JBM_BLOCKS_NUM             2

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

typedef enum  {
   RX_SPEECH_GOOD=0,
   RX_SPEECH_PROBABLY_DEGRADED,
   RX_SPEECH_LOST,
   RX_SPEECH_BAD,
   RX_SID_FIRST,
   RX_SID_UPDATE,
   RX_SID_BAD,
   RX_NO_DATA
} RXFrameType;

typedef enum  {
   TX_SPEECH=0,
   TX_SID_FIRST,
   TX_SID_UPDATE,
   TX_NO_DATA,
   TX_SPEECH_PROBABLY_DEGRADED,
   TX_SPEECH_LOST,
   TX_SPEECH_BAD,
   TX_SID_BAD
} TXFrameType;


/*****************************************************************************************/
/****    5 FUNCTION PROTOTYPES    ********************************************************/

MGW_GLOBAL void
init_intel_ipp(void);

MGW_GLOBAL ecode_t
m_evs_enc_init(codec_enc_data_t *evs_enc);

MGW_GLOBAL ecode_t
m_evs_dec_init(codec_dec_data_t *evs_dec);

MGW_GLOBAL ecode_t
m_evs_enc(codec_enc_data_t *evs_enc);

MGW_GLOBAL ecode_t
m_evs_dec(codec_dec_data_t *evs_dec);

MGW_GLOBAL ecode_t
m_evs_enc_release(codec_enc_data_t *evs_enc);

MGW_GLOBAL ecode_t
m_evs_dec_release(codec_dec_data_t *evs_dec);

/*****************************************************************************************/
#endif /* M_EVS_H */

