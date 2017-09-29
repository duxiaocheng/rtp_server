/*****************************************************************************************/
/**
@brief Error definitions and declarations for MGW DSP SW
*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/

#ifndef ERROR_H
#define ERROR_H

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include <type_def.h>

/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

/** Mask for error code classification: internal */
#define ERROR_C_INTERNAL      0x0000
/** Mask for error code classification: configuration */
#define ERROR_C_CONFIGURATION 0x1000
/** Mask for error code classification: external interface */
#define ERROR_C_EXTERNAL_IF   0x2000
/** Mask for error code classification: internal interface */
#define ERROR_C_INTERNAL_IF   0x3000
/** Mask for error code classification: congestion */
#define ERROR_C_CONGESTION    0x5000


/** No errors (successful operation) (#ecode_t) */
#define EA_NO_ERRORS 0x00000000

#define DEVICE_ERROR_HI_BITS (((uint32)0xC) << 28)
#define FATAL_ERROR_HI_BITS  (((uint32)0x8) << 28)
#define SEVERE_ERROR_HI_BITS (((uint32)0x7) << 28)
#define MINOR_ERROR_HI_BITS  (((uint32)0x3) << 28)
#define INFO_ERROR_HI_BITS   (((uint32)0x2) << 28)

/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

/** Returns TRUE if severity of ecode (#ecode_t) is info, FALSE otherwise */
#define ERROR_IS_INFO(ecode)   (((ecode) & (((uint32) 0xF) << 28)) == INFO_ERROR_HI_BITS)
/** Returns TRUE if severity of ecode (#ecode_t) is minor, FALSE otherwise */
#define ERROR_IS_MINOR(ecode)  (((ecode) & (((uint32) 0xF) << 28)) == MINOR_ERROR_HI_BITS)
/** Returns TRUE if severity of ecode (#ecode_t) is severe, FALSE otherwise */
#define ERROR_IS_SEVERE(ecode) (((ecode) & (((uint32) 0xF) << 28)) == SEVERE_ERROR_HI_BITS)
/** Returns TRUE if severity of ecode (#ecode_t) is fatal, FALSE otherwise */
#define ERROR_IS_FATAL(ecode)  (((ecode) & (((uint32) 0xF) << 28)) == FATAL_ERROR_HI_BITS)

#define DEVICE_MASK(mask_nbr) GENERATE_ERROR_MASK(DEVICE_ERROR_HI_BITS, mask_nbr)
#define FATAL_MASK(mask_nbr)  GENERATE_ERROR_MASK(FATAL_ERROR_HI_BITS, mask_nbr)
#define SEVERE_MASK(mask_nbr) GENERATE_ERROR_MASK(SEVERE_ERROR_HI_BITS, mask_nbr)
#define MINOR_MASK(mask_nbr)  GENERATE_ERROR_MASK(MINOR_ERROR_HI_BITS, mask_nbr)
#define INFO_MASK(mask_nbr)   GENERATE_ERROR_MASK(INFO_ERROR_HI_BITS, mask_nbr)
#define GENERATE_ERROR_MASK(error_hi_bits, mask_nbr) \
  (((uint32)(error_hi_bits)) | \
  ((((uint32)(mask_nbr))&0x0FFF) << 16))

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

/**
@brief      Error code

Error code consists of 32 bits: 0xSMMMCEEE, where:

S = 4 bits: severity as defined in error_masks.h
M = 12 bits: module mask as defined in error_masks.h
C = 4 bits: classification mask as defined in this file
E = 12 bits: actual error code

*/
typedef uint32 ecode_t;

/**
@brief      Error code with extra information
*/
typedef struct {
  ecode_t ecode; /**< Error code */
  uint32 extra;  /**< Extra error information */
} ecode_and_extra_t;

/*****************************************************************************************/
/****    5 FUNCTION PROTOTYPES    ********************************************************/

MGW_GLOBAL void
l_sys_report_error(uint32 service_id, ecode_t ecode, uint32 extra);

MGW_GLOBAL uint32
l_sys_get_current_servid(void);

/*****************************************************************************************/
#endif /* ERROR_H */


