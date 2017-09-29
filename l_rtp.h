/*****************************************************************************************/
/**
@brief RTP functions
*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/

#ifndef L_IRTP_H
#define L_IRTP_H

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/
#include <type_def.h>

/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

/** Size of the RTP uncompressed header in octets */
#define L_RTP_RTP_UNCOMPRESSED_HEADER_SIZE_8  12
/** Size of the RTP compressed Mb header in octets */
#define L_RTP_RTP_COMPRESSED_MB_HEADER_SIZE_8  4
/** Size of the RTP compressed Nb header in octets */
#define L_RTP_RTP_COMPRESSED_NB_HEADER_SIZE_8  3

/** RTP field: undefined (#l_rtp_field_id_t) */
#define L_RTP_FIELD_ID_UNDEFINED    0
/** RTP field: version (#l_rtp_field_id_t) */
#define L_RTP_FIELD_ID_VERSION      1
/** RTP field: padding (#l_rtp_field_id_t) */
#define L_RTP_FIELD_ID_PADDING      2
/** RTP field: extension (#l_rtp_field_id_t) */
#define L_RTP_FIELD_ID_EXTENSION    3
/** RTP field: CSRC count (#l_rtp_field_id_t) */
#define L_RTP_FIELD_ID_CSRC_COUNT   4
/** RTP field: marker (#l_rtp_field_id_t) */
#define L_RTP_FIELD_ID_MARKER       5
/** RTP field: payload type (#l_rtp_field_id_t) */
#define L_RTP_FIELD_ID_PAYLOAD_TYPE 6
/** RTP field: sequence number (#l_rtp_field_id_t) */
#define L_RTP_FIELD_ID_SEQ_NUMBER   7
/** RTP field: timestamp (#l_rtp_field_id_t) */
#define L_RTP_FIELD_ID_TIMESTAMP    8
/** RTP field: synchronization source (#l_rtp_field_id_t) */
#define L_RTP_FIELD_ID_SSRC         9

/** RTP header type: undefined (#l_rtp_type_t) */
#define L_RTP_TYPE_UNDEFINED         0
/** RTP header type: RTP uncompressed (#l_rtp_type_t) */
#define L_RTP_TYPE_RTP_UNCOMPRESSED  1
/** RTP header type: RTP compressed Mb (#l_rtp_type_t) */
#define L_RTP_TYPE_RTP_COMPRESSED_MB 2
/** RTP header type: RTP compressed Nb (#l_rtp_type_t) */
#define L_RTP_TYPE_RTP_COMPRESSED_NB 3

/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

/** RTP header field ID */
typedef uint16 l_rtp_field_id_t;

/** RTP header type */
typedef uint16 l_rtp_type_t;

/*****************************************************************************************/
/****    5 FUNCTION PROTOTYPES    ********************************************************/

MGW_GLOBAL uint16
l_rtp_get_header_size_8(l_rtp_type_t type);

MGW_GLOBAL uint32
l_rtp_get_field(void *rtp_header, l_rtp_type_t type, l_rtp_field_id_t field_id);

MGW_GLOBAL void
l_rtp_set_field(void *rtp_header, l_rtp_type_t type, l_rtp_field_id_t field_id,
                 uint32 value);

MGW_GLOBAL void
l_rtp_increment_field(void *rtp_header, l_rtp_type_t type, l_rtp_field_id_t field_id,
                       uint32 increment);

/*****************************************************************************************/
#endif /* L_IRTP_H */

