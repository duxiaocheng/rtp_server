/*****************************************************************************************/
/**
@brief AMR wideband sample rate converter DAF-module header

This module performs the sample rate conversion of a linear PCM signal by the factor of
two. It is designed to work for sample rate conversions 8 kHz -> 16 kHz and
16 kHz -> 8 kHz, where its passband is 0 - 3400 Hz and stop band is 4000 Hz.

The module consists of two independent parts, upsampling module and downsampling module.
The functions for upsampling are denoted with affix _up_ and functions for downsampling
are denoted with affix _down_.

*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/

#ifndef M_SRC_H
#define M_SRC_H

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include <type_def.h>
#include <l_error.h>
#include "l_upd.h"

/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

/* SEVERE ERROR MASK */
#define E_M_SRC_SEVERE        SEVERE_MASK(0x044)
/* MINOR ERROR MASK */
#define E_M_SRC_MINOR         MINOR_MASK(0x044)

/** Null pointer. Extra: - */
#define E_M_SRC_NULL_POINTER                    (E_M_SRC_SEVERE | ERROR_C_INTERNAL | 0x002)
/** Overflow occurred during filtering. Extra: filter return parameter */
#define E_M_SRC_OVERFLOW                        (E_M_SRC_MINOR  | ERROR_C_INTERNAL | 0x004)
/** Input UPD packet for upsampling. Extra: UPD packet header */
#define E_M_SRC_UP_UPD_IN                       (E_M_SRC_SEVERE | ERROR_C_INTERNAL | 0x005)
/** Input UPD packet for downsampling. Extra: UPD packet header */
#define E_M_SRC_DOWN_UPD_IN                     (E_M_SRC_SEVERE | ERROR_C_INTERNAL | 0x006)

/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

/**
 @brief      Module master data structure

Module master structure for upsampling. Contains the state information.

*/
typedef struct {
  /** Input 8 kHz linear PCM UPD packet */
  upd_packet_t *upd_in;
  /** Output 16 kHz linear PCM UPD packet (in-place computaion is allowed) */
  upd_packet_t *upd_out;
  /** Extra error code */
  uint32 extra_ecode;
  /** Internal module state */
  void *internal;
} m_src_up_data;

/**
 @brief      Module master data structure

Module master structure for downsampling. Contains the state information.

*/
typedef struct {
  /** Input 16 kHz linear PCM UPD packet */
  upd_packet_t *upd_in;
  /** Output 8 kHz linear PCM UPD packet (in-place computaion is allowed) */
  upd_packet_t *upd_out;
  /** Extra error code */
  uint32 extra_ecode;
  /** Internal module state */
  void *internal;
} m_src_down_data;

/*****************************************************************************************/
/****    5 FUNCTION PROTOTYPES    ********************************************************/

MGW_GLOBAL ecode_t
m_src_up_init(m_src_up_data *master);

MGW_GLOBAL ecode_t
m_src_down_init(m_src_down_data *master);

MGW_GLOBAL ecode_t
m_src_up(m_src_up_data *master);

MGW_GLOBAL ecode_t
m_src_down(m_src_down_data *master);

MGW_GLOBAL ecode_t
m_src_up_release(m_src_up_data *master);

MGW_GLOBAL ecode_t
m_src_down_release(m_src_down_data *master);

/*****************************************************************************************/
#endif /* M_SRC_H */

