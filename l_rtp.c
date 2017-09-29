/*****************************************************************************************/
/**
@brief RTP functions

Note: get and set functions use different pointer types (uint32/uint16) due to issues
      related to memory alignment. This also creates alignment issues if 8 bit bitfields
      and 32 bit variables are used within the header structures. Check \main\8 changes
      for details.
*/

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include "l_rtp.h"
#include <l_mgw.h>

/*****************************************************************************************/
/****    2 LOCAL CONSTANTS    ************************************************************/

/*****************************************************************************************/
/****    3 LOCAL MACROS    ***************************************************************/

/*****************************************************************************************/
/****    4 LOCAL DATA TYPES    ***********************************************************/

/**
@brief      RTP packet header (uncompressed)

As defined in IETF RFC 3550.
*/
struct rtp_header_uncompressed  {
  uint16 bitfield;
  /*
  Internal bitfield structure.

  BITFIELDS(
    bf16 version      : 2,
    bf16 padding      : 1,
    bf16 extension    : 1,
    bf16 csrc_count   : 4,
    bf16 marker       : 1,
    bf16 payload_type : 7
  )
  */
  uint16 seq_number;
  uint16 timestamp_hi;
  uint16 timestamp_lo;
  uint16 ssrc_hi;
  uint16 ssrc_lo;
};

/**
@brief      RTP packet header (compressed, Mb interface)

As defined in 3GPP TS 48.103.
*/
struct rtp_header_compressed_mb {
  uint8 seq_number;
  uint8 timestamp_msb;
  uint8 timestamp_lsb;
  uint8 bitfield;
  /*
  Internal bitfield structure.

  BITFIELDS(
    bf8 marker        : 1,
    bf8 payload_type  : 7
  )
  */
};

/**
@brief      RTP packet header (compressed, Nb interface)

As defined in 3GPP TS 29.414.
*/
struct rtp_header_compressed_nb {
  uint8 seq_number;
  uint8 timestamp_msb;
  uint8 timestamp_lsb;
  uint8 spare;
};

/*****************************************************************************************/
/****    5 LOCAL FUNCTION PROTOTYPES    **************************************************/

/*****************************************************************************************/
/****    6 FUNCTIONS    ******************************************************************/

/*****************************************************************************************/
/**
@brief      Gets size of RTP header

@param      type Type of RTP header
@return     Size of RTP header, 0 if invalid type
*/
MGW_GLOBAL uint16
l_rtp_get_header_size_8(l_rtp_type_t type)
{
  uint16 size_8 = 0;

  if (type == L_RTP_TYPE_RTP_UNCOMPRESSED) {
    size_8 = L_RTP_RTP_UNCOMPRESSED_HEADER_SIZE_8;
  } else if (type == L_RTP_TYPE_RTP_COMPRESSED_MB) {
    size_8 = L_RTP_RTP_COMPRESSED_MB_HEADER_SIZE_8;
  } else if (type == L_RTP_TYPE_RTP_COMPRESSED_NB) {
    size_8 = L_RTP_RTP_COMPRESSED_NB_HEADER_SIZE_8;
  }

  return size_8;
}

/*****************************************************************************************/
/**
@brief      Gets value of RTP header field

@param      rtp_header RTP header
@param      type Type of RTP header
@param      field_id ID of requested field
@return     Value of the requested field, 0 if invalid type or field_id
*/
MGW_GLOBAL uint32
l_rtp_get_field(void *rtp_header, l_rtp_type_t type, l_rtp_field_id_t field_id)
{
  uint32 value = 0;

  struct rtp_header_uncompressed *rtp_uncompressed =
    (struct rtp_header_uncompressed *) rtp_header;
  struct rtp_header_compressed_mb *rtp_mb =
    (struct rtp_header_compressed_mb *) rtp_header;
  struct rtp_header_compressed_nb *rtp_nb =
    (struct rtp_header_compressed_nb *) rtp_header;

  if (type == L_RTP_TYPE_RTP_UNCOMPRESSED) {
    switch (field_id) {
    case L_RTP_FIELD_ID_VERSION:
      value = (L_MGW_BE_TO_NAT_16(rtp_uncompressed->bitfield) >> 14) & 0x0003;
      break;
    case L_RTP_FIELD_ID_PADDING:
      value = (L_MGW_BE_TO_NAT_16(rtp_uncompressed->bitfield) >> 13) & 0x0001;
      break;
    case L_RTP_FIELD_ID_EXTENSION:
      value = (L_MGW_BE_TO_NAT_16(rtp_uncompressed->bitfield) >> 12) & 0x0001;
      break;
    case L_RTP_FIELD_ID_CSRC_COUNT:
      value = (L_MGW_BE_TO_NAT_16(rtp_uncompressed->bitfield) >>  8) & 0x000F;
      break;
    case L_RTP_FIELD_ID_MARKER:
      value = (L_MGW_BE_TO_NAT_16(rtp_uncompressed->bitfield) >>  7) & 0x0001;
      break;
    case L_RTP_FIELD_ID_PAYLOAD_TYPE:
      value = (L_MGW_BE_TO_NAT_16(rtp_uncompressed->bitfield) >>  0) & 0x007F;
      break;
    case L_RTP_FIELD_ID_SEQ_NUMBER:
      value = L_MGW_BE_TO_NAT_16(rtp_uncompressed->seq_number);
      break;
    case L_RTP_FIELD_ID_TIMESTAMP:
      value = ((uint32) L_MGW_BE_TO_NAT_16(rtp_uncompressed->timestamp_hi) << 16) |
                        L_MGW_BE_TO_NAT_16(rtp_uncompressed->timestamp_lo);
      break;
    case L_RTP_FIELD_ID_SSRC:
      value = ((uint32) L_MGW_BE_TO_NAT_16(rtp_uncompressed->ssrc_hi) << 16) |
                        L_MGW_BE_TO_NAT_16(rtp_uncompressed->ssrc_lo);
      break;
    default:
      break;
    }
  } else if (type == L_RTP_TYPE_RTP_COMPRESSED_MB) {
    switch (field_id) {
    case L_RTP_FIELD_ID_SEQ_NUMBER:
      value = rtp_mb->seq_number;
      break;
    case L_RTP_FIELD_ID_TIMESTAMP:
      value = (rtp_mb->timestamp_msb << 8) | rtp_mb->timestamp_lsb;
      break;
    case L_RTP_FIELD_ID_MARKER:
      value = (rtp_mb->bitfield >> 7) & 0x01;
      break;
    case L_RTP_FIELD_ID_PAYLOAD_TYPE:
      value = (rtp_mb->bitfield >> 0) & 0x7F;
      break;
    default:
      break;
    }
  } else if (type == L_RTP_TYPE_RTP_COMPRESSED_NB) {
    switch (field_id) {
    case L_RTP_FIELD_ID_SEQ_NUMBER:
      value = rtp_nb->seq_number;
      break;
    case L_RTP_FIELD_ID_TIMESTAMP:
      value = (rtp_nb->timestamp_msb << 8) | rtp_nb->timestamp_lsb;
      break;
    default:
      break;
    }
  }

  return value;
}

/*****************************************************************************************/
/**
@brief      Sets value of RTP header field

Sets value of RTP header field. Does nothing if invalid type or field_id.

@param      rtp_header RTP header
@param      type Type of RTP header
@param      field_id ID of field to be set
@param      value Value to be set
*/
MGW_GLOBAL void
l_rtp_set_field(void *rtp_header, l_rtp_type_t type, l_rtp_field_id_t field_id,
                uint32 value)
{
  struct rtp_header_uncompressed *rtp_uncompressed =
    (struct rtp_header_uncompressed *) rtp_header;
  struct rtp_header_compressed_mb *rtp_mb =
    (struct rtp_header_compressed_mb *) rtp_header;
  struct rtp_header_compressed_nb *rtp_nb =
    (struct rtp_header_compressed_nb *) rtp_header;

  if (type == L_RTP_TYPE_RTP_UNCOMPRESSED) {
    switch (field_id) {
    case L_RTP_FIELD_ID_VERSION:
      rtp_uncompressed->bitfield &= ~L_MGW_NAT_TO_BE_16(0xC000);
      rtp_uncompressed->bitfield |=  L_MGW_NAT_TO_BE_16((value & 0x0003) << 14);
      break;
    case L_RTP_FIELD_ID_MARKER:
      rtp_uncompressed->bitfield &= ~L_MGW_NAT_TO_BE_16(0x0080);
      rtp_uncompressed->bitfield |=  L_MGW_NAT_TO_BE_16((value & 0x0001) <<  7);
      break;
    case L_RTP_FIELD_ID_PAYLOAD_TYPE:
      rtp_uncompressed->bitfield &= ~L_MGW_NAT_TO_BE_16(0x007F);
      rtp_uncompressed->bitfield |=  L_MGW_NAT_TO_BE_16((value & 0x007F) <<  0);
      break;
    case L_RTP_FIELD_ID_SEQ_NUMBER:
      rtp_uncompressed->seq_number = L_MGW_NAT_TO_BE_16(value);
      break;
    case L_RTP_FIELD_ID_TIMESTAMP:
      rtp_uncompressed->timestamp_hi =
        L_MGW_NAT_TO_BE_16((uint16) ((value & 0xFFFF0000) >> 16));
      rtp_uncompressed->timestamp_lo =
        L_MGW_NAT_TO_BE_16((uint16) ((value & 0x0000FFFF) >>  0));
      break;
    case L_RTP_FIELD_ID_SSRC:
      rtp_uncompressed->ssrc_hi =
        L_MGW_NAT_TO_BE_16((uint16) ((value & 0xFFFF0000) >> 16));
      rtp_uncompressed->ssrc_lo =
        L_MGW_NAT_TO_BE_16((uint16) ((value & 0x0000FFFF) >>  0));
      break;
    default:
      break;
    }
  } else if (type == L_RTP_TYPE_RTP_COMPRESSED_MB) {
    switch (field_id) {
    case L_RTP_FIELD_ID_MARKER:
      rtp_mb->bitfield &= ~0x80;
      rtp_mb->bitfield |=  (value & 0x01) << 7;
      break;
    case L_RTP_FIELD_ID_PAYLOAD_TYPE:
      rtp_mb->bitfield &= ~0x7F;
      rtp_mb->bitfield |=  (value & 0x7F) << 0;
      break;
    case L_RTP_FIELD_ID_SEQ_NUMBER:
      rtp_mb->seq_number = value;
      break;
    case L_RTP_FIELD_ID_TIMESTAMP:
      rtp_mb->timestamp_msb = (value >> 8) & 0x000000FF;
      rtp_mb->timestamp_lsb = (value >> 0) & 0x000000FF;
      break;
    default:
      break;
    }
  } else if (type == L_RTP_TYPE_RTP_COMPRESSED_NB) {
    switch (field_id) {
    case L_RTP_FIELD_ID_SEQ_NUMBER:
      rtp_nb->seq_number = value;
      break;
    case L_RTP_FIELD_ID_TIMESTAMP:
      rtp_nb->timestamp_msb = (value >> 8) & 0x000000FF;
      rtp_nb->timestamp_lsb = (value >> 0) & 0x000000FF;
      break;
    default:
      break;
    }
  }
}

/*****************************************************************************************/
/**
@brief      Increments value of RTP header field

Increments value of RTP header field. Does nothing if invalid field_id.

@param      rtp_header RTP header
@param      type Type of RTP header
@param      field_id ID of field to be incremented
@param      increment Value to be added to the field
*/
MGW_GLOBAL void
l_rtp_increment_field(void *rtp_header, l_rtp_type_t type, l_rtp_field_id_t field_id,
                      uint32 increment)
{
  uint32 old_value = l_rtp_get_field((uint16 *) rtp_header, type, field_id);

  switch (field_id) {
  case L_RTP_FIELD_ID_SEQ_NUMBER:
    l_rtp_set_field(rtp_header, type, field_id, old_value + increment);
    break;
  case L_RTP_FIELD_ID_TIMESTAMP:
    l_rtp_set_field(rtp_header, type, field_id, old_value + increment);
    break;
  default:
    break;
  }
}


