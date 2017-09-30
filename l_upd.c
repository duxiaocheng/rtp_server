/*****************************************************************************************/
/**
@brief Function library for UPD

Remember to name the author of the function when adding one.

*/

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include "l_upd.h"
#include <l_mgw.h>
#include <l_string.h>

#include <string.h>

/*****************************************************************************************/
/****    2 LOCAL CONSTANTS    ************************************************************/

/* Codeword for GSM FR SID frames (defined in 3GPP 46.012). Bits are set to 0. */
/* Notice that this constant is not really needed but it is only shown for completeness.
MGW_LOCAL const uint16 gsm_fr_sid_codeword[SIZE_1_TO_16(UPD_PAYLOAD_GSM_FR_BITS)] = {
  0x0000, 0x0000, 0x0000, 0x036D, 0xB6DB, 0x6DB0, 0x0003, 0x6DB6,
  0xDB6D, 0xB000, 0x036D, 0xB6DB, 0x6DB0, 0x0003, 0x6D92, 0x4924, 0x9000
};*/

/** Codeword for GSM EFR SID frames (defined in 3GPP 46.062). Bits are set to 1. */
MGW_LOCAL const uint16 gsm_efr_sid_codeword[SIZE_1_TO_16(UPD_PAYLOAD_GSM_EFR_BITS)] = {
  0x0000, 0x0000, 0x0006, 0xFFFF, 0xF800, 0x0003, 0xBFFF, 0xFE00,
  0x0000, 0x0FFF, 0xFFF0, 0x0000, 0x0FFF, 0xCFFC, 0x0000, 0x0000
};

/** Codeword for GSM HR SID frames (defined in 3GPP 46.022). Bits are set to 1. */
MGW_LOCAL const uint16 gsm_hr_sid_codeword[SIZE_1_TO_16(UPD_PAYLOAD_GSM_HR_BITS)] = {
  0x0000, 0x0000, 0x7FFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
};

/** AMR-NB codec parameter bit order tables for mode 4.75 */
MGW_LOCAL const uint16
amrnb_frame_bit_order_0475[UPD_PAYLOAD_AMRNB_0475_BITS] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
  52, 36, 37, 53, 38, 54, 39, 16, 17, 18, 19, 20, 21, 44, 45, 91,
  67, 68, 83, 69, 70, 87, 50, 51, 49, 48, 47, 46, 31, 30, 29, 28,
  22, 23, 55, 56, 92, 71, 72, 84, 73, 74, 88, 57, 58, 24, 25, 59,
  60, 93, 75, 76, 85, 77, 78, 89, 61, 62, 43, 42, 41, 40, 35, 34,
  33, 32, 26, 27, 63, 64, 94, 79, 80, 86, 81, 82, 90, 65, 66 };

/** AMR-NB codec parameter bit order tables for mode 5.15 */
MGW_LOCAL const uint16
amrnb_frame_bit_order_0515[UPD_PAYLOAD_AMRNB_0515_BITS] = {
  7,   6,   5,   4,   3,   2,   1,   0,   15,  14,  13,  12,  11,  10,  9,   8,
  65,  52,  44,  64,  53,  59,  54,  16,  17,  18,  19,  20,  40,  49,  60,  91,
  83,  85,  99,  72,  79,  95,  68,  67,  55,  45,  36,  26,  25,  24,  21,  41,
  50,  61,  92,  84,  86,  100, 73,  80,  96,  70,  69,  56,  46,  37,  29,  28,
  27,  22,  42,  51,  62,  93,  87,  88,  101, 74,  81,  97,  76,  71,  57,  47,
  38,  32,  31,  30,  23,  43,  63,  66,  94,  89,  90,  102, 75,  82,  98,  78,
  77,  58,  48,  39,  35,  34,  33 };

/** AMR-NB codec parameter bit order tables for mode 5.9 */
MGW_LOCAL const uint16
amrnb_frame_bit_order_0590[UPD_PAYLOAD_AMRNB_0590_BITS] = {
  0,   1,   7,   4,   2,   3,   5,   6,   10,  11,  15,  12,  13,  8,   14,  9,
  16,  64,  63,  59,  65,  60,  62,  61,  66,  75,  23,  21,  17,  19,  25,  33,
  43,  45,  98,  94,  90,  114, 110, 86,  82,  106, 102, 68,  73,  76,  55,  51,
  47,  37,  29,  27,  35,  41,  80,  99,  95,  91,  115, 111, 87,  83,  107, 103,
  71,  72,  77,  56,  52,  48,  38,  30,  24,  22,  18,  20,  26,  34,  44,  46,
  100, 96,  92,  116, 112, 88,  84,  108, 104, 69,  74,  78,  57,  53,  49,  39,
  31,  28,  36,  42,  81,  101, 97,  93,  117, 113, 89,  85,  109, 105, 70,  67,
  79,  58,  54,  50,  40,  32 };

/** AMR-NB codec parameter bit order tables for mode 6.7 */
MGW_LOCAL const uint16
amrnb_frame_bit_order_0670[UPD_PAYLOAD_AMRNB_0670_BITS] = {
  0,   1,   8,   3,   2,   4,   5,   7,   9,   10,  15,  11,  13,  6,   14,  12,
  26,  62,  61,  53,  63,  59,  60,  54,  64,  65,  22,  20,  16,  18,  24,  31,
  39,  51,  106, 102, 130, 126, 98,  94,  122, 118, 90,  114, 110, 86,  78,  70,
  35,  85,  66,  41,  45,  55,  74,  27,  29,  33,  49,  107, 103, 131, 127, 99,
  95,  123, 119, 91,  115, 111, 87,  79,  71,  36,  84,  67,  42,  46,  56,  75,
  23,  21,  17,  19,  25,  32,  40,  52,  108, 104, 132, 128, 100, 96,  124, 120,
  92,  116, 112, 88,  80,  72,  37,  83,  68,  43,  47,  57,  76,  28,  30,  34,
  50,  109, 105, 133, 129, 101, 97,  125, 121, 93,  117, 113, 89,  81,  73,  38,
  82,  69,  44,  48,  58,  77 };

/** AMR-NB codec parameter bit order tables for mode 7.4 */
MGW_LOCAL const uint16
amrnb_frame_bit_order_0740[UPD_PAYLOAD_AMRNB_0740_BITS] = {
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,
  16,  50,  51,  52,  61,  47,  48,  49,  62,  63,  17,  19,  21,  23,  25,  53,
  88,  89,  120, 124, 128, 132, 136, 140, 144, 96,  97,  98,  99,  100, 101, 84,
  76,  72,  64,  27,  31,  78,  35,  39,  57,  68,  43,  45,  54,  90,  91,  121,
  125, 129, 133, 137, 141, 145, 102, 103, 104, 105, 106, 107, 85,  77,  73,  65,
  28,  32,  79,  36,  40,  58,  69,  18,  20,  22,  24,  26,  55,  92,  93,  122,
  126, 130, 134, 138, 142, 146, 108, 109, 110, 111, 112, 113, 86,  82,  74,  66,
  29,  33,  80,  37,  41,  59,  70,  44,  46,  56,  94,  95,  123, 127, 131, 135,
  139, 143, 147, 114, 115, 116, 117, 118, 119, 87,  83,  75,  67,  30,  34,  81,
  38,  42,  60,  71 };

/** AMR-NB codec parameter bit order tables for mode 7.95 */
MGW_LOCAL const uint16
amrnb_frame_bit_order_0795[UPD_PAYLOAD_AMRNB_0795_BITS] = {
  68,  67,  6,   5,   4,   3,   2,   1,   0,   9,   10,  14,  11,  12,  7,   13,
  8,   15,  21,  20,  16,  22,  17,  19,  18,  69,  70,  43,  45,  47,  49,  51,
  63,  71,  73,  128, 106, 151, 155, 127, 102, 147, 126, 98,  125, 124, 94,  123,
  122, 121, 120, 119, 35,  39,  79,  87,  23,  27,  31,  59,  83,  53,  55,  57,
  65,  75,  77,  134, 107, 152, 156, 133, 103, 148, 132, 99,  131, 130, 95,  129,
  118, 117, 116, 115, 36,  40,  80,  88,  24,  28,  32,  60,  84,  44,  46,  48,
  50,  52,  64,  72,  74,  140, 108, 153, 157, 139, 104, 149, 138, 100, 137, 136,
  96,  135, 110, 93,  92,  91,  37,  41,  81,  89,  25,  29,  33,  61,  85,  54,
  56,  58,  66,  76,  78,  146, 109, 154, 158, 145, 105, 150, 144, 101, 143, 142,
  97,  141, 114, 113, 112, 111, 38,  42,  82,  90,  26,  30,  34,  62,  86 };

/** AMR-NB codec parameter bit order tables for mode 10.2 */
MGW_LOCAL const uint16
amrnb_frame_bit_order_1020[UPD_PAYLOAD_AMRNB_1020_BITS] = {
  7,   6,   5,   4,   3,   2,   1,   0,   16,  15,  14,  13,  12,  11,  10,  9,
  8,   62,  61,  57,  63,  59,  60,  58,  64,  65,  17,  18,  19,  20,  21,  22,
  45,  46,  69,  68,  67,  66,  150, 151, 155, 158, 157, 164, 166, 199, 201, 196,
  153, 152, 154, 159, 156, 162, 167, 202, 203, 197, 160, 161, 165, 163, 195, 200,
  198, 33,  53,  83,  34,  35,  82,  84,  29,  30,  49,  50,  94,  73,  72,  71,
  70,  96,  97,  101, 104, 103, 110, 112, 190, 192, 187, 99,  98,  100, 105, 102,
  108, 113, 193, 194, 188, 106, 107, 111, 109, 186, 191, 189, 36,  54,  86,  37,
  38,  85,  87,  23,  24,  25,  26,  27,  28,  47,  48,  77,  76,  75,  74,  114,
  115, 119, 122, 121, 128, 130, 181, 183, 178, 117, 116, 118, 123, 120, 126, 131,
  184, 185, 179, 124, 125, 129, 127, 177, 182, 180, 39,  55,  89,  40,  41,  88,
  90,  31,  32,  51,  52,  95,  81,  80,  79,  78,  132, 133, 137, 140, 139, 146,
  148, 172, 174, 169, 135, 134, 136, 141, 138, 144, 149, 175, 176, 170, 142, 143,
  147, 145, 168, 173, 171, 42,  56,  92,  43,  44,  91,  93 };

/** AMR-NB codec parameter bit order tables for mode 12.2 */
MGW_LOCAL const uint16
amrnb_frame_bit_order_1220[UPD_PAYLOAD_AMRNB_1220_BITS] = {
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  16,
  17,  18,  19,  20,  21,  22,  23,  15,  24,  25,  26,  27,  28,  81,  82,  83,
  84,  85,  86,  87,  120, 121, 29,  31,  33,  35,  37,  39,  41,  43,  45,  47,
  51,  55,  88,  96,  124, 123, 122, 100, 127, 126, 125, 108, 130, 129, 128, 112,
  133, 132, 131, 116, 136, 135, 134, 184, 183, 182, 187, 186, 185, 190, 189, 188,
  193, 192, 191, 196, 195, 194, 59,  63,  67,  92,  104, 71,  73,  75,  77,  79,
  242, 48,  52,  56,  89,  97,  139, 138, 137, 101, 142, 141, 140, 109, 145, 144,
  143, 113, 148, 147, 146, 117, 151, 150, 149, 199, 198, 197, 202, 201, 200, 205,
  204, 203, 208, 207, 206, 211, 210, 209, 60,  64,  68,  93,  105, 30,  32,  34,
  36,  38,  40,  42,  44,  46,  49,  53,  57,  90,  98,  154, 153, 152, 102, 157,
  156, 155, 110, 160, 159, 158, 114, 163, 162, 161, 118, 166, 165, 164, 214, 213,
  212, 217, 216, 215, 220, 219, 218, 223, 222, 221, 226, 225, 224, 61,  65,  69,
  94,  106, 72,  74,  76,  78,  80,  243, 50,  54,  58,  91,  99,  169, 168, 167,
  103, 172, 171, 170, 111, 175, 174, 173, 115, 178, 177, 176, 119, 181, 180, 179,
  229, 228, 227, 232, 231, 230, 235, 234, 233, 238, 237, 236, 241, 240, 239, 62,
  66,  70,  95,  107 };

/** AMR-NB codec parameter bit order table address in easy to use array format */
MGW_LOCAL const uint16 * const
amrnb_frame_bit_orders[UPD_NUMBER_OF_AMRNB_MODES] = {
  amrnb_frame_bit_order_0475,
  amrnb_frame_bit_order_0515,
  amrnb_frame_bit_order_0590,
  amrnb_frame_bit_order_0670,
  amrnb_frame_bit_order_0740,
  amrnb_frame_bit_order_0795,
  amrnb_frame_bit_order_1020,
  amrnb_frame_bit_order_1220
};

/** AMR-NB codec parameter bit order table address in easy to use array format */
MGW_LOCAL const uint16
amrnb_payload_size_in_bits[UPD_NUMBER_OF_AMRNB_MODES] = {
  UPD_PAYLOAD_AMRNB_0475_BITS,
  UPD_PAYLOAD_AMRNB_0515_BITS,
  UPD_PAYLOAD_AMRNB_0590_BITS,
  UPD_PAYLOAD_AMRNB_0670_BITS,
  UPD_PAYLOAD_AMRNB_0740_BITS,
  UPD_PAYLOAD_AMRNB_0795_BITS,
  UPD_PAYLOAD_AMRNB_1020_BITS,
  UPD_PAYLOAD_AMRNB_1220_BITS
};
/** AMR-WB codec parameter bit order table address in easy to use array format */
MGW_LOCAL const uint16
amrwb_payload_size_in_bits[UPD_NUMBER_OF_AMRWB_MODES] = {
  UPD_PAYLOAD_AMRWB_0660_BITS,
  UPD_PAYLOAD_AMRWB_0885_BITS,
  UPD_PAYLOAD_AMRWB_1265_BITS,
  UPD_PAYLOAD_AMRWB_1425_BITS,
  UPD_PAYLOAD_AMRWB_1585_BITS,
  UPD_PAYLOAD_AMRWB_1825_BITS,
  UPD_PAYLOAD_AMRWB_1985_BITS,
  UPD_PAYLOAD_AMRWB_2305_BITS,
  UPD_PAYLOAD_AMRWB_2385_BITS
};

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

/*****************************************************************************************/
/****    3 LOCAL MACROS    ***************************************************************/

/*****************************************************************************************/
/****    4 LOCAL DATA TYPES    ***********************************************************/

/**
@brief      Position of a bit in a table of uint16 variables
@note       Added by Juha Sarmavuori
*/
struct bit_position {
  uint16 uint8_in_table; /**< index of the uint8 entry in the table */
  uint16 bit_in_uint8;   /**< index of a bit in the uint8 entry of the table */
};

/*****************************************************************************************/
/****    5 LOCAL FUNCTION PROTOTYPES    **************************************************/

MGW_LOCAL C_INLINE struct bit_position
get_bit_position_from_bit_number(uint16 bit_number);

MGW_LOCAL C_INLINE void
set_out_bit(uint8 *frame_out, uint16 bit_number, uint16 bit_on_left);

MGW_LOCAL C_INLINE uint8
get_bit_on_right(const uint8 *frame_in, uint16 bit_number);

MGW_LOCAL C_INLINE void
shuffle_frame(uint8 *frame_out, const uint8 *frame_in,
              const uint16 *bit_order, uint16 n_bits);

MGW_LOCAL C_INLINE void
de_shuffle_frame(uint8 *frame_out, const uint8 *frame_in,
                 const uint16 *bit_order, uint16 n_bits);

MGW_LOCAL C_INLINE uint16
get_opus_sample_rate_hz(uint16 *opus_packet);


/*****************************************************************************************/
/****    6 FUNCTIONS    ******************************************************************/

/*****************************************************************************************/
/**
@brief      Gets T.38 UDP payload size in octets

@param      upd_packet UPD packet whose size is queried
@return     T.38 UDP payload size in octets
*/
MGW_GLOBAL uint16
l_upd_get_t38_payload_size_8(const upd_packet_t *upd_packet)
{
  return ((upd_packet->type << 4) | upd_packet->mode);
}

/*****************************************************************************************/
/**
@brief      Sets T.38 UDP payload size in octets

@param      upd_packet UPD packet whose size is set
@param      size_8 Size in octets
*/
MGW_GLOBAL void
l_upd_set_t38_payload_size_8(upd_packet_t *upd_packet, uint16 size_8)
{
  upd_packet->type = (size_8 >> 4) & 0xF;
  upd_packet->mode = size_8 & 0xF;
}

/*****************************************************************************************/
/**
@brief      Gets Opus UDP payload size in octets

@param      upd_packet UPD packet whose size is queried
@return     Opus UDP payload size in octets
*/
MGW_GLOBAL uint16
l_upd_get_opus_payload_size_8(const upd_packet_t *upd_packet)
{
  return ((upd_packet->type << 8) | (upd_packet->mode << 4) | upd_packet->form);
}

/*****************************************************************************************/
/**
@brief      Sets Opus UDP payload size in octets

@param      upd_packet UPD packet whose size is set
@param      size_8 Size in octets
*/
MGW_GLOBAL void
l_upd_set_opus_payload_size_8(upd_packet_t *upd_packet, uint16 size_8)
{
  upd_packet->type = (size_8 >> 8) & 0xF;
  upd_packet->mode = (size_8 >> 4) & 0xF;
  upd_packet->form = size_8 & 0xF;
}

/*****************************************************************************************/
/**
@brief      Gets the size of the UPD packet

Gets the size of UPD packet (header and payload) in bits.

@param      upd_packet UPD packet whose size is queried
@return     The size of the UPD packet in bits
*/
MGW_GLOBAL uint16
l_upd_get_packet_size_1(upd_packet_t *upd_packet)
{
  uint16 header_size_1 = SIZE_16_TO_1(UPD_HEADER_SIZE_16);
  uint16 payload_size_1 = l_upd_get_payload_size_1(upd_packet);

  return (header_size_1 + payload_size_1);
}

/*****************************************************************************************/
/**
@brief      Gets the size of the UPD packet payload

Gets the size of UPD packet payload in bits.

@param      upd_packet UPD packet whose payload size is queried
@return     The size of the UPD packet payload in bits, 0 for invalid UPD packets
*/
MGW_GLOBAL uint16
l_upd_get_payload_size_1(const upd_packet_t *upd_packet)
{
  if (upd_packet->codec == UPD_CODEC_PCM) {
    if ((upd_packet->type == UPD_TYPE_SPEECH_GOOD) ||
        (upd_packet->type == UPD_TYPE_SPEECH_NO_DATA)) {
      if (upd_packet->form == UPD_FORM_PCM_LINEAR_08_KHZ) {
        if (upd_packet->mode == UPD_MODE_PCM_05_MS) {
          return UPD_PAYLOAD_PCM_05_MS_LINEAR_08_KHZ_BITS;
        } else if (upd_packet->mode == UPD_MODE_PCM_10_MS) {
          return UPD_PAYLOAD_PCM_10_MS_LINEAR_08_KHZ_BITS;
        } else if (upd_packet->mode == UPD_MODE_PCM_20_MS) {
          return UPD_PAYLOAD_PCM_20_MS_LINEAR_08_KHZ_BITS;
        } else if (upd_packet->mode == UPD_MODE_PCM_30_MS) {
          return UPD_PAYLOAD_PCM_30_MS_LINEAR_08_KHZ_BITS;
        }
      } else if (upd_packet->form == UPD_FORM_PCM_LINEAR_16_KHZ) {
        if (upd_packet->mode == UPD_MODE_PCM_05_MS) {
          return UPD_PAYLOAD_PCM_05_MS_LINEAR_16_KHZ_BITS;
        } else if (upd_packet->mode == UPD_MODE_PCM_10_MS) {
          return UPD_PAYLOAD_PCM_10_MS_LINEAR_16_KHZ_BITS;
        } else if (upd_packet->mode == UPD_MODE_PCM_20_MS) {
          return UPD_PAYLOAD_PCM_20_MS_LINEAR_16_KHZ_BITS;
        } else if (upd_packet->mode == UPD_MODE_PCM_30_MS) {
          return UPD_PAYLOAD_PCM_30_MS_LINEAR_16_KHZ_BITS;
        }
      } else {
        if (upd_packet->mode == UPD_MODE_PCM_05_MS) {
          return UPD_PAYLOAD_PCM_05_MS_8_BIT_BITS;
        } else if (upd_packet->mode == UPD_MODE_PCM_10_MS) {
          return UPD_PAYLOAD_PCM_10_MS_8_BIT_BITS;
        } else if (upd_packet->mode == UPD_MODE_PCM_20_MS) {
          return UPD_PAYLOAD_PCM_20_MS_8_BIT_BITS;
        } else if (upd_packet->mode == UPD_MODE_PCM_30_MS) {
          return UPD_PAYLOAD_PCM_30_MS_8_BIT_BITS;
        }
      }
    } else if ((upd_packet->type == UPD_TYPE_SID_UPDATE) ||
               (upd_packet->type == UPD_TYPE_SID_NO_DATA)) {
      return SIZE_8_TO_1(upd_packet->form);
    }
  } else if (upd_packet->codec == UPD_CODEC_GSM_FR) {
    if (l_upd_is_type_valid(UPD_CODEC_GSM_FR, upd_packet->type) == TRUE) {
      if (L_UPD_IS_TYPE_SPEECH(upd_packet->type) == TRUE) {
        return UPD_PAYLOAD_GSM_FR_BITS;
      } else if (L_UPD_IS_TYPE_SID(upd_packet->type) == TRUE) {
        return UPD_PAYLOAD_GSM_FR_SID_BITS;
      }
    }
  } else if (upd_packet->codec == UPD_CODEC_GSM_EFR) {
    if (l_upd_is_type_valid(UPD_CODEC_GSM_EFR, upd_packet->type) == TRUE) {
      if (L_UPD_IS_TYPE_SPEECH(upd_packet->type) == TRUE) {
        return UPD_PAYLOAD_GSM_EFR_BITS;
      } else if (L_UPD_IS_TYPE_SID(upd_packet->type) == TRUE) {
        return UPD_PAYLOAD_GSM_EFR_SID_BITS;
      }
    }
  } else if (upd_packet->codec == UPD_CODEC_GSM_HR) {
    if (l_upd_is_type_valid(UPD_CODEC_GSM_HR, upd_packet->type) == TRUE) {
      if (L_UPD_IS_TYPE_SPEECH(upd_packet->type) == TRUE) {
        return UPD_PAYLOAD_GSM_HR_BITS;
      } else if (L_UPD_IS_TYPE_SID(upd_packet->type) == TRUE) {
        return UPD_PAYLOAD_GSM_HR_SID_BITS;
      }
    }
  } else if (upd_packet->codec == UPD_CODEC_AMRNB) {
    if (l_upd_is_type_valid(UPD_CODEC_AMRNB, upd_packet->type) == TRUE) {
      if (L_UPD_IS_TYPE_SPEECH(upd_packet->type) == TRUE) {
        return l_upd_get_amr_payload_size_1(UPD_CODEC_AMRNB, upd_packet->mode);
      } else if (L_UPD_IS_TYPE_SID(upd_packet->type) == TRUE) {
        return UPD_PAYLOAD_AMRNB_SID_BITS;
      }
    }
  } else if (upd_packet->codec == UPD_CODEC_AMRWB) {
    if (upd_packet->type == UPD_TYPE_SPEECH_LOST) {
      return 0;
    } else if (l_upd_is_type_valid(UPD_CODEC_AMRWB, upd_packet->type) == TRUE) {
      if (L_UPD_IS_TYPE_SPEECH(upd_packet->type) == TRUE) {
        return l_upd_get_amr_payload_size_1(UPD_CODEC_AMRWB, upd_packet->mode);
      } else if (L_UPD_IS_TYPE_SID(upd_packet->type) == TRUE) {
        return UPD_PAYLOAD_AMRWB_SID_BITS;
      }
    }
  } else if (upd_packet->codec == UPD_CODEC_BFP) {
    if (l_upd_is_type_valid(UPD_CODEC_BFP, upd_packet->type) == TRUE) {
      if (L_UPD_IS_TYPE_SPEECH(upd_packet->type) == TRUE) {
        if (upd_packet->mode == UPD_MODE_BFP_05_MS) {
          return UPD_PAYLOAD_BFP_05_MS_BITS;
        } else if (upd_packet->mode == UPD_MODE_BFP_10_MS) {
          return UPD_PAYLOAD_BFP_10_MS_BITS;
        } else if (upd_packet->mode == UPD_MODE_BFP_20_MS) {
          return UPD_PAYLOAD_BFP_20_MS_BITS;
        } else if (upd_packet->mode == UPD_MODE_BFP_30_MS) {
          return UPD_PAYLOAD_BFP_30_MS_BITS;
        }
      }
    }
  } else if (upd_packet->codec == UPD_CODEC_ILBC) {
    if (upd_packet->type == UPD_TYPE_SPEECH_GOOD) {
      if (upd_packet->mode == UPD_MODE_ILBC_1520) {
        return UPD_PAYLOAD_ILBC_1520_BITS;
      } else if (upd_packet->mode == UPD_MODE_ILBC_1333) {
        return UPD_PAYLOAD_ILBC_1333_BITS;
      }
    } else if (upd_packet->type == UPD_TYPE_SID_UPDATE) {
      return SIZE_8_TO_1(upd_packet->form);
    }
  } else if (upd_packet->codec == UPD_CODEC_G723) {
    if ((upd_packet->type == UPD_TYPE_SPEECH_GOOD) ||
        (upd_packet->type == UPD_TYPE_SPEECH_BAD) ||
        (upd_packet->type == UPD_TYPE_SPEECH_NO_DATA)) {
      if (upd_packet->mode == UPD_MODE_G723_0630) {
        return UPD_PAYLOAD_G723_0630_BITS;
      } else if (upd_packet->mode == UPD_MODE_G723_0530) {
        return UPD_PAYLOAD_G723_0530_BITS;
      }
    } else if ((upd_packet->type == UPD_TYPE_SID_UPDATE) ||
               (upd_packet->type == UPD_TYPE_SID_NO_DATA)) {
      return UPD_PAYLOAD_G723_SID_BITS;
    }
  } else if (upd_packet->codec == UPD_CODEC_G729) {
    if (upd_packet->mode == UPD_MODE_NOT_APPLICABLE) {
      if ((upd_packet->type == UPD_TYPE_SPEECH_GOOD) ||
          (upd_packet->type == UPD_TYPE_SPEECH_BAD) ||
          (upd_packet->type == UPD_TYPE_SPEECH_NO_DATA)) {
        return UPD_PAYLOAD_G729_BITS;
      } else if ((upd_packet->type == UPD_TYPE_SID_UPDATE) ||
                 (upd_packet->type == UPD_TYPE_SID_NO_DATA)) {
        return UPD_PAYLOAD_G729_SID_BITS;
      }
    }
  } else if (upd_packet->codec == UPD_CODEC_DATA) {
    if (upd_packet->form == UPD_FORM_DATA_T_DATA) {
      if (upd_packet->type == UPD_TYPE_SPEECH_GOOD) {
        return UPD_PAYLOAD_DATA_T_BITS;
      }
    } else if (upd_packet->form == UPD_FORM_DATA_NT_DATA) {
      if (upd_packet->type == UPD_TYPE_SPEECH_GOOD) {
        return UPD_PAYLOAD_DATA_NT_BITS;
      }
    } else if (upd_packet->form == UPD_FORM_DATA_T38) {
      return SIZE_8_TO_1(l_upd_get_t38_payload_size_8(upd_packet));
    }
  } else if (upd_packet->codec == UPD_CODEC_OPUS) {
    return SIZE_8_TO_1(l_upd_get_opus_payload_size_8(upd_packet));
  } else if (upd_packet->codec == UPD_CODEC_EVRC) {
    if (L_UPD_IS_TYPE_SPEECH(upd_packet->type) == TRUE) {
      if (upd_packet->mode == UPD_MODE_EVRC_FULL_RATE) {
        return UPD_PAYLOAD_EVRC_FULL_RATE_BITS;
      } else if (upd_packet->mode == UPD_MODE_EVRC_HALF_RATE) {
        return UPD_PAYLOAD_EVRC_HALF_RATE_BITS;
      } else if (upd_packet->mode == UPD_MODE_EVRC_QUARTER_RATE) {
        return UPD_PAYLOAD_EVRC_QUARTER_RATE_BITS;
      } else if (upd_packet->mode == UPD_MODE_EVRC_EIGHTH_RATE) {
        return UPD_PAYLOAD_EVRC_EIGHTH_RATE_BITS;
      }
    }
  }/*target RTP header full only*/
  else if (upd_packet->codec == UPD_CODEC_EVS) {
    if (upd_packet->type == UPD_TYPE_SPEECH_LOST) {
      return 0;
    } else if (upd_packet->form == UPD_EVS_WBIO_MODE) {
      if (L_UPD_IS_TYPE_SID(upd_packet->type) == TRUE) {
        return UPD_PAYLOAD_EVS_35_SID;
      } else if (L_UPD_IS_TYPE_SPEECH(upd_packet->type) == TRUE) {
        return l_upd_get_evs_wbio_payload_size(upd_packet->mode);
      }

    } else if (upd_packet->form == UPD_EVS_PRIMARY_MODE) {
      if (L_UPD_IS_TYPE_SID(upd_packet->type) == TRUE) {
        return UPD_PAYLOAD_EVS_48_SID;
      } else if (L_UPD_IS_TYPE_SPEECH(upd_packet->type) == TRUE) {
        return l_upd_get_evs_primary_payload_size(upd_packet->mode);
      }
    }
  }

  return 0;
}


/*****************************************************************************************/
/**
@brief      Gets the size of the speech AMR payload in bits

Gets the size of the speech AMR payload in bits. Speech AMR payload is defined in 3GPP
26.101 (Section 4.2.1) for AMR-NB and in 3GPP 26.201 (Section 4.2.1) for AMR-WB.

@param      codec Codec of the AMR payload (UPD_CODEC_AMRNB or UPD_CODEC_AMRWB)
@param      mode AMR mode of the payload
@return     The size of the AMR payload in bits, or 0 if codec or mode is invalid
*/
MGW_GLOBAL uint16
l_upd_get_amr_payload_size_1(upd_codec_t codec, upd_mode_amr_t mode)
{
  if (codec == UPD_CODEC_AMRNB) {
    if (mode <= UPD_MODE_AMRNB_1220) {
      return amrnb_payload_size_in_bits[mode];
    }
  } else if (codec == UPD_CODEC_AMRWB) {
    if (mode <= UPD_MODE_AMRWB_2385) {
      return amrwb_payload_size_in_bits[mode];
    }
  }

  return 0;
}

/*****************************************************************************************/
/**
@brief      Gets the duration of speech UPD packet (coded or PCM) in milliseconds

Notice that this function doesn't work for data codecs.

@param      codec UPD codec of speech UPD packet whose duration is queried
@param      mode UPD mode of speech UPD packet whose duration is queried
@return     Duration of the speech UPD packet in milliseconds
*/
MGW_GLOBAL uint16
l_upd_get_speech_duration_ms(upd_codec_t codec, upd_mode_t mode)
{
  if (codec == UPD_CODEC_PCM) {
    if (mode == UPD_MODE_PCM_05_MS) {
      return 5;
    } else if (mode == UPD_MODE_PCM_10_MS) {
      return 10;
    } else if (mode == UPD_MODE_PCM_30_MS) {
      return 30;
    }
  } else if (codec == UPD_CODEC_G723) {
    return 30;
  } else if (codec == UPD_CODEC_G729) {
    return 10;
  } else if (codec == UPD_CODEC_ILBC) {
    if (mode == UPD_MODE_ILBC_1333) {
      return 30;
    }
  } else if (codec == UPD_CODEC_BFP) {
    if (mode == UPD_MODE_BFP_05_MS) {
      return 5;
    } else if (mode == UPD_MODE_BFP_10_MS) {
      return 10;
    } else if (mode == UPD_MODE_BFP_30_MS) {
      return 30;
    }
  }

  return 20;
}

/*****************************************************************************************/
/**
@brief      Gets the number of encoded frames in one Opus packet according to RFC6716

@param      opus_packet Opus packet
@return     Number of frames in Opus packet
*/
MGW_GLOBAL uint16
l_upd_get_num_frames_in_opus_packet(upd_packet_t *upd_packet)
{
  uint8 *opus_packet_8 = l_upd_get_payload(upd_packet);
  uint8 packet_code = (uint8) (opus_packet_8[0] & 0x03);
  uint16 num_frames = 0;

  if (packet_code == 0) {
    num_frames = 1;
  } else if ((packet_code == 1) || (packet_code == 2)) {
    num_frames = 2;
  } else {
    num_frames = (uint16) (opus_packet_8[1] & 0x3F);
  }

  return num_frames;
}

/*****************************************************************************************/
/**
@brief      Gets the frame length of Opus codec

Fetches the frame length from the Opus packet's TOC byte according to RFC6716

@param      opus_packet Opus packet
@return     Frame length microseconds
*/
MGW_GLOBAL uint32
l_upd_get_opus_speech_duration_us(upd_packet_t *upd_packet)
{
  uint8 *opus_packet_8 = l_upd_get_payload(upd_packet);
  uint8 config = (uint8) ((opus_packet_8[0] >> 3) & 0x1F);
  uint16 frame_length_us = 0;
  uint16 num_frames_in_packet = 0;

  if (config < 12) {
    if ((config % 4) == 0) {
      frame_length_us = 10000;
    } else if ((config % 4) == 1) {
      frame_length_us = 20000;
    } else if ((config % 4) == 2) {
      frame_length_us = 40000;
    } else {
      frame_length_us = 60000;
    }
  } else if ((config >= 12) && (config < 16)) {
    if ((config % 2) == 0) {
      frame_length_us = 10000;
    } else {
      frame_length_us = 20000;
    }
  } else {
    if ((config % 4) == 0) {
      frame_length_us = 2500;
    } else if ((config % 4) == 1) {
      frame_length_us = 5000;
    } else if ((config % 4) == 2) {
      frame_length_us = 10000;
    } else {
      frame_length_us = 20000;
    }
  }

  num_frames_in_packet = l_upd_get_num_frames_in_opus_packet(upd_packet);

  return frame_length_us * num_frames_in_packet;
}

/*****************************************************************************************/
/**
@brief      Gets the speech duration in an Opus packet in milliseconds rounded to
            nearest integer

@param      upd_packet UPD packet containing Opus data
@return     TRUE if UPD packet duration is valid, otherwise FALSE
*/
MGW_GLOBAL uint16
l_upd_get_opus_speech_duration_ms(upd_packet_t *upd_packet)
{
  return (l_upd_get_opus_speech_duration_us(upd_packet) + 500) / 1000;
}

/*****************************************************************************************/
/**
@brief      Reads AMR-NB SID UPD mode from SID payload bits

AMR-NB SID payload is defined in 3GPP 26.101 Chapter 4.2.3. Mode is indicated in payload
bits 37,38,39 (37=LSB, 39=MSB).

@param      amrnb_payload Payload of AMR-NB UPD packet
@return     AMR-NB SID mode
*/
MGW_GLOBAL upd_mode_amrnb_t
l_upd_get_amrnb_sid_mode(uint16 *amrnb_payload)
{
  uint16 mode = 0;
  /* amrnb_payload[2] = payload bits 33-39 */
  mode  = ((amrnb_payload[2] & 0x0800) >> 11); /* set bit 37 */
  mode |= ((amrnb_payload[2] & 0x0400) >>  9); /* set bit 38 */
  mode |= ((amrnb_payload[2] & 0x0200) >>  7); /* set bit 39 */

  return mode;
}

/*****************************************************************************************/
/**
@brief      Reads AMR-NB SID UPD type from SID payload bits

AMR-NB SID payload is defined in 3GPP 26.101 Chapter 4.2.3. Frame type is indicated in
payload bit 36 (STI).

@param      amrnb_payload Payload of AMR-NB UPD packet
@return     AMR-NB SID UPD type
*/
MGW_GLOBAL upd_type_t
l_upd_get_amrnb_sid_type(uint16 *amrnb_payload)
{
  /* amrnb_payload[2] = payload bits 33-39 */
  if ((amrnb_payload[2] & 0x1000) == 0) { /* bit 36 (STI) */
    return UPD_TYPE_SID_FIRST;
  } else {
    return UPD_TYPE_SID_UPDATE;
  }
}

/*****************************************************************************************/
/**
@brief      Reads AMR-WB SID UPD mode from SID payload bits

AMR-WB SID payload is defined in 3GPP 26.201 Chapter 4.2.3. Mode is indicated in payload
bits 37,38,39,40 (37=MSB, 40=LSB).

@param      amrwb_payload Payload of AMR-WB UPD packet
@return     AMR-WB SID UPD mode
*/
MGW_GLOBAL upd_mode_amrwb_t
l_upd_get_amrwb_sid_mode(uint16 *amrwb_payload)
{
  /* amrwb_payload[2] = payload bits 33-40 */
  uint16 mode = ((amrwb_payload[2] & 0x0F00) >> 8); /* bits 37-40 */

  if (mode <= UPD_MODE_AMRWB_2385) {
    return mode;
  } else {
    return UPD_MODE_AMRWB_NOT_APPLICABLE;
  }
}

/*****************************************************************************************/
/**
@brief      Reads AMR-WB SID UPD type from SID payload bits

AMR-WB SID payload is defined in 3GPP 26.201 Chapter 4.2.3. Frame type is indicated in
payload bit 36 (STI).

@param      amrwb_payload Payload of AMR-WB UPD packet
@return     AMR-WB SID UPD type
*/
MGW_GLOBAL upd_type_t
l_upd_get_amrwb_sid_type(uint16 *amrwb_payload)
{
  /* amrwb_payload[2] = payload bits 33-40 */
  if ((amrwb_payload[2] & 0x1000) == 0) { /* bit 36 (STI) */
    return UPD_TYPE_SID_FIRST;
  } else {
    return UPD_TYPE_SID_UPDATE;
  }
}

/*****************************************************************************************/
/**
@brief      Returns AMR-NB mode of given bits

Returns AMR-NB mode of given bits. If number of bits is not valid, returned value
is UPD_MODE_AMRNB_NOT_APPLICABLE.

@param      bits number of bits
@return     AMR-NB mode
*/
MGW_GLOBAL upd_mode_amrnb_t
l_upd_map_bits_to_amrnb_mode(uint16 bits)
{
  switch (bits) {
  case UPD_PAYLOAD_AMRNB_1220_BITS:
    return UPD_MODE_AMRNB_1220;
  case UPD_PAYLOAD_AMRNB_1020_BITS:
    return UPD_MODE_AMRNB_1020;
  case UPD_PAYLOAD_AMRNB_0795_BITS:
    return UPD_MODE_AMRNB_0795;
  case UPD_PAYLOAD_AMRNB_0740_BITS:
    return UPD_MODE_AMRNB_0740;
  case UPD_PAYLOAD_AMRNB_0670_BITS:
    return UPD_MODE_AMRNB_0670;
  case UPD_PAYLOAD_AMRNB_0590_BITS:
    return UPD_MODE_AMRNB_0590;
  case UPD_PAYLOAD_AMRNB_0515_BITS:
    return UPD_MODE_AMRNB_0515;
  case UPD_PAYLOAD_AMRNB_0475_BITS:
    return UPD_MODE_AMRNB_0475;
  default:
    return UPD_MODE_AMRNB_NOT_APPLICABLE;
  }
}

/*****************************************************************************************/
/**
@brief      Returns AMR-WB mode of given bits

Returns AMR-WB mode of given bits. If number of bits is not valid, returned value
is UPD_MODE_AMRWB_NOT_APPLICABLE.

@param      bits number of bits
@return     AMR-WB mode
*/
MGW_GLOBAL upd_mode_amrwb_t
l_upd_map_bits_to_amrwb_mode(uint16 bits)
{
  switch (bits) {
  case UPD_PAYLOAD_AMRWB_2385_BITS:
    return UPD_MODE_AMRWB_2385;
  case UPD_PAYLOAD_AMRWB_2305_BITS:
    return UPD_MODE_AMRWB_2305;
  case UPD_PAYLOAD_AMRWB_1985_BITS:
    return UPD_MODE_AMRWB_1985;
  case UPD_PAYLOAD_AMRWB_1825_BITS:
    return UPD_MODE_AMRWB_1825;
  case UPD_PAYLOAD_AMRWB_1585_BITS:
    return UPD_MODE_AMRWB_1585;
  case UPD_PAYLOAD_AMRWB_1425_BITS:
    return UPD_MODE_AMRWB_1425;
  case UPD_PAYLOAD_AMRWB_1265_BITS:
    return UPD_MODE_AMRWB_1265;
  case UPD_PAYLOAD_AMRWB_0885_BITS:
    return UPD_MODE_AMRWB_0885;
  case UPD_PAYLOAD_AMRWB_0660_BITS:
    return UPD_MODE_AMRWB_0660;
  default:
    return UPD_MODE_AMRWB_NOT_APPLICABLE;
  }
}

/*****************************************************************************************/
/**
@brief      Gets the maximum size of AMR payload

Gets the maximum size of AMR payload in bits.

@param      codec Codec whose payload size is queried
@return     Maximum size of AMR payload in bits
*/
MGW_GLOBAL uint16
l_upd_get_max_amr_size_bits(upd_codec_t codec)
{
  if (codec == UPD_CODEC_AMRNB) {
    return UPD_PAYLOAD_AMRNB_1220_BITS;
  } else if (codec == UPD_CODEC_AMRWB) {
    return UPD_PAYLOAD_AMRWB_2385_BITS;
  } else {
    return 0;
  }
}

/*****************************************************************************************/
/**
@brief      Gets the maximum AMR mode

Gets the maximum (highest bitrate) AMR mode.

@param      codec Codec whose maximum mode is queried
@return     Maximum AMR mode
*/
MGW_GLOBAL upd_mode_amr_t
l_upd_get_max_amr_mode(upd_codec_t codec)
{
  if (codec == UPD_CODEC_AMRNB) {
    return UPD_MODE_AMRNB_1220;
  } else if (codec == UPD_CODEC_AMRWB) {
    return UPD_MODE_AMRWB_2385;
  } else {
    return UPD_MODE_AMR_NOT_APPLICABLE;
  }
}

/*****************************************************************************************/
/**
@brief      Gets payload padding bitmask

Gets payload padding bitmask. Padding bitmask means the bitmask that can be used to set
payload padding bits to 0.

<pre>
Example usage:

payload[SIZE_1_TO_16(size_bits) - 1] &= l_upd_get_payload_padding_mask(size_bits);

bitmasks are:

size_in_bits % 16 ==  0, mask == 0xFFFF
size_in_bits % 16 ==  1, mask == 0x8000
size_in_bits % 16 ==  2, mask == 0xC000
...
size_in_bits % 16 == 14, mask == 0xFFFC
size_in_bits % 16 == 15, mask == 0xFFFE
</pre>

@param      size_in_bits Size of the payload in bits
@return     Padding bit mask
*/
MGW_GLOBAL uint16
l_upd_get_payload_padding_mask(uint16 size_in_bits)
{
  uint16 i = 0;
  uint16 mask = 0;

  if ((size_in_bits % 16) == 0) {
    mask = 0xFFFF;
  } else {
    for (i = 0; i < (size_in_bits % 16); i++) {
      mask |= (0x8000 >> i);
    }
  }

  return mask;
}

/*****************************************************************************************/
/**
@brief      Gets GSM FR/GSM EFR UPD type from control bits

Maps BFI, SID and TAF to GSM FR/GSM EFR UPD type.

@param      bfi Bad Frame Indicator
@param      sid Silence Indicator
@param      taf Time Alignment Flag
@return     UPD type corresponding the control bits, UPD_TYPE_NOT_APPLICABLE in error cases
*/
MGW_GLOBAL upd_type_t
l_upd_get_gsm_fr_efr_type(uint16 bfi, uint16 sid, uint16 taf)
{
  upd_type_t type = UPD_TYPE_NOT_APPLICABLE;

  if (sid == 0) { /* speech */
    if (bfi == 0) {
      type = UPD_TYPE_SPEECH_GOOD;
    } else {
      type = UPD_TYPE_SPEECH_BAD;
    }
  } else if (sid == 1) {
    if (bfi == 0) {
      type = UPD_TYPE_SID_UNRELIABLE;
    } else {
      type = UPD_TYPE_SID_BAD;
    }
  } else if (sid == 2) {
    if (bfi == 0) {
      if (taf == 0) {
        type = UPD_TYPE_SID_FIRST;
      } else {
        type = UPD_TYPE_SID_UPDATE;
      }
    } else {
      type = UPD_TYPE_SID_BAD;
    }
  }

  return type;
}

/*****************************************************************************************/
/**
@brief      Gets bitrate (bit/s) correspoding to UPD codec and mode

Maps UPD codec and mode to bitrate (bit/s). Mode relevant for AMR-WB, AMR-NB, iLBC and
G.723 codecs. Doesn't support UPD_CODEC_DATA.

@param      codec UPD codec
@param      mode UPD mode
@return     Bitrate (bit/s), 0 for error situations
*/
MGW_GLOBAL uint16
l_upd_get_bitrate(upd_codec_t codec, upd_mode_t mode)
{
  if (codec == UPD_CODEC_AMRNB) {
    if (mode == UPD_MODE_AMRNB_1220) {
      return 12200;
    } else if (mode == UPD_MODE_AMRNB_1020) {
      return 10200;
    } else if (mode == UPD_MODE_AMRNB_0795) {
      return 7950;
    } else if (mode == UPD_MODE_AMRNB_0740) {
      return 7400;
    } else if (mode == UPD_MODE_AMRNB_0670) {
      return 6700;
    } else if (mode == UPD_MODE_AMRNB_0590) {
      return 5900;
    } else if (mode == UPD_MODE_AMRNB_0515) {
      return 5150;
    } else if (mode == UPD_MODE_AMRNB_0475) {
      return 4750;
    } else {
      return 0;
    }
  } else if (codec == UPD_CODEC_AMRWB) {
    if (mode == UPD_MODE_AMRWB_2385) {
      return 23850;
    } else if (mode == UPD_MODE_AMRWB_2305) {
      return 23050;
    } else if (mode == UPD_MODE_AMRWB_1985) {
      return 19850;
    } else if (mode == UPD_MODE_AMRWB_1825) {
      return 18250;
    } else if (mode == UPD_MODE_AMRWB_1585) {
      return 15850;
    } else if (mode == UPD_MODE_AMRWB_1425) {
      return 14250;
    } else if (mode == UPD_MODE_AMRWB_1265) {
      return 12650;
    } else if (mode == UPD_MODE_AMRWB_0885) {
      return 8850;
    } else if (mode == UPD_MODE_AMRWB_0660) {
      return 6600;
    } else {
      return 0;
    }
  } else if (codec == UPD_CODEC_GSM_EFR) {
    return 12200;
  } else if (codec == UPD_CODEC_GSM_FR) {
    return 13000;
  } else if (codec == UPD_CODEC_GSM_HR) {
    return 5600;
  } else if (codec == UPD_CODEC_PCM) {
    return 64000;
  } else if (codec == UPD_CODEC_ILBC) {
    if (mode == UPD_MODE_ILBC_1520) {
      return 15200;
    } else if (mode == UPD_MODE_ILBC_1333) {
      return 13330;
    } else {
      return 0;
    }
  } else if (codec == UPD_CODEC_G723) {
    if (mode == UPD_MODE_G723_0630) {
      return 6300;
    } else if (mode == UPD_MODE_G723_0530) {
      return 5300;
    } else {
      return 0;
    }
  } else if (codec == UPD_CODEC_G729) {
    return 8000;
  } else {
    return 0;
  }
}

/*****************************************************************************************/
/**
@brief      Maps GSM FR SID payload format from long to short

Long format means the format used in UPD packet (260 bits).

Short format contains 42 bits: 36 bits of mean (LAR(i)) values followed by 6 bits of mean
(Xmax) value as defined in 3GPP 46.012.

@param      payload SID payload
*/
MGW_GLOBAL void
l_upd_map_gsm_fr_sid_long_to_short(uint16 *payload)
{
  /* Long:  Block amplitude (Xmax) (6 bits) in bits 48-53 */
  /* Short: Block amplitude (Xmax) (6 bits) in bits 37-42 */
  payload[2] = ((payload[2] & 0xF000) | ((payload[2] & 0x0001) << 11) |
               ((payload[3] & 0xF800) >> 5));
}

/*****************************************************************************************/
/**
@brief      Maps GSM EFR SID payload format from long to short

Long format means the format used in UPD packet (244 bits).

Short format contains 43 bits: 38 bits of the quantization indices derived from the
averaged LSF parameter vector followed by 5 bits of the quantization index derived from the
averaged fixed codebook gain value as defined in 3GPP 46.062 (Section 5.3).

@param      payload SID payload
*/
MGW_GLOBAL void
l_upd_map_gsm_efr_sid_long_to_short(uint16 *payload)
{
  /* Long:  Quantization index (5 bits) in bits 87-91 */
  /* Short: Quantization index (5 bits) in bits 39-43 */
  payload[2] = ((payload[2] & 0xFC00) | (payload[5] & 0x03E0));
}

/*****************************************************************************************/
/**
@brief      Maps GSM FR SID payload format from short to long

Long format means the format used in UPD packet (260 bits).

Short format contains 42 bits: 36 bits of mean (LAR(i)) values followed by 6 bits of mean
(Xmax) value as defined in 3GPP 46.012.

@param      payload SID payload
*/
MGW_GLOBAL void
l_upd_map_gsm_fr_sid_short_to_long(uint16 *payload)
{
  uint16 tmp = 0;

  /* Short: Xmax (mean) (6 bits) in bits 37-42 */
  /* Long : Xmax1 bits 48-53  Xmax2 bits 104-109 */
  /*        Xmax3 bits 160-165 Xmax4 bits 216-221 */
  tmp = ((payload[2] >> 6) & 0x003F); /* bits 37-42 */
  payload[2] = ((payload[2] & 0xF000) | (tmp >> 5)); /* bit 48 */
  payload[3] = (tmp << 11); /* bits 49-53 */
  payload[4] = 0;
  payload[5] = 0;
  payload[6] = (tmp << 3); /* bits 104-109 */
  payload[7] = 0;
  payload[8] = 0;
  payload[9] = (tmp >> 5); /* bit 160 */
  payload[10] = (tmp << 11); /* bits 161-165 */
  payload[11] = 0;
  payload[12] = 0;
  payload[13] = (tmp << 3); /* bits 216-221 */
  payload[14] = 0;
  payload[15] = 0;
  payload[16] = 0;

  /* no need to explicitly insert codeword as codeword bits are set to 0 */
}

/*****************************************************************************************/
/**
@brief      GSM EFR SID payload format from short to long

Long format means the format used in UPD packet (244 bits).

Short format contains 43 bits: 38 bits of the quantization indices derived from the
averaged LSF parameter vector followed by 5 bits of the quantization index derived from the
averaged fixed codebook gain value as defined in 3GPP 46.062 (Section 5.3).

@param      payload SID payload
*/
MGW_GLOBAL void
l_upd_map_gsm_efr_sid_short_to_long(uint16 *payload)
{
  uint16 i = 0;
  uint16 tmp = 0;

  /* Short: Quantization index (5 bits) in bits 39-43 */
  /* Long:  Quantization index (5 bits) in bits 87-91, 137-141, 190-194, 240-244 */
  tmp = ((payload[2] >> 5) & 0x001F); /* bits 39-43 */
  payload[2] = (payload[2] & 0xFC00);
  payload[3] = 0;
  payload[4] = 0;
  payload[5] = (tmp << 5); /* bits 87-91 */
  payload[6] = 0;
  payload[7] = 0;
  payload[8] = (tmp << 3); /* bits 137-141 */
  payload[9] = 0;
  payload[10] = 0;
  payload[11] = (tmp >>  2); /* bits 190-192 */
  payload[12] = (tmp << 14); /* bits 193-194 */
  payload[13] = 0;
  payload[14] = (tmp >>  4); /* bit 240 */
  payload[15] = (tmp << 12); /* bits 241-244 */

  /* insert codeword (set codeword bits to 1) */
  for (i = 0; i < SIZE_1_TO_16(UPD_PAYLOAD_GSM_EFR_BITS); i++) {
    payload[i] |= gsm_efr_sid_codeword[i];
  }
}

/*****************************************************************************************/
/**
@brief      Maps GSM HR SID payload format from short to long

Long format means the format used in UPD packet (112 bits).

Short format contains 33 bits: 5 bits of the quantization index derived from mean(R0[j])
followed by 27 bits of the VQ indices derived from mean(R[j](i)) as defined in 3GPP 46.022
(Section 5.3).

@param      payload SID payload
*/
MGW_GLOBAL void
l_upd_map_gsm_hr_sid_short_to_long(uint16 *payload)
{
  uint16 i = 0;

  /* insert codeword (set codeword bits to 1) */
  for (i = 0; i < SIZE_1_TO_16(UPD_PAYLOAD_GSM_HR_BITS); i++) {
    payload[i] |= gsm_hr_sid_codeword[i];
  }
}

/*****************************************************************************************/
/**
@brief      Converts bit number in array into index to array and bit number in a variable

@param      bit_number number of a bit in an array of uint16 variables
@return     Position of bit as an array index and bit number within the array entry
*/
MGW_LOCAL C_INLINE struct bit_position
get_bit_position_from_bit_number(uint16 bit_number)
{
  struct bit_position position = { 0 };

  position.uint8_in_table = bit_number / 8;
  position.bit_in_uint8 = bit_number % 8;

  return position;
}

/*****************************************************************************************/
/**
@brief      Sets a bit to 1 in a bit array, if input bit is 1

@param      frame_out Bit array, where the bit is to be set
@param      bit_number Number of the bit to be set
@param      bit_on_left The leftmost bit is the input bit. Other bits are 0.
@note       Added by Juha Sarmavuori
*/
MGW_LOCAL C_INLINE void
set_out_bit(uint8 *frame_out, uint16 bit_number, uint16 bit_on_left)
{
  struct bit_position position = get_bit_position_from_bit_number(bit_number);
  uint16 bit_shifted_to_right_position = bit_on_left >> position.bit_in_uint8;

  frame_out[position.uint8_in_table] |= bit_shifted_to_right_position;
}

/*****************************************************************************************/
/**
@brief      Gets a bit in a bit array

@param      frame_in Bit array
@param      bit_number Number of the bit to be got from the array
@return     The wanted bit in rightmost position
*/
MGW_LOCAL C_INLINE uint8
get_bit_on_right(const uint8 *frame_in, uint16 bit_number)
{
  struct bit_position position = get_bit_position_from_bit_number(bit_number);
  uint8 wanted_uint8 = frame_in[position.uint8_in_table];
  uint8 wanted_bit_shifted_to_left = wanted_uint8 << position.bit_in_uint8;
  uint8 wanted_bit_on_right = wanted_bit_shifted_to_left >> 7;

  return wanted_bit_on_right;
}

/*****************************************************************************************/
/**
@brief      Shuffle (permute) frame of bits

Bits in frame_in table are iterated in successive order. Each bit is mapped to new
position according to bit_order table, which is also iterated in succesive order.
Outer loop iterates throufh array elements. Inner loop iterates through bits of an uint8
element. Last loop iterates through the bits in the last incomplete uint8 entry.
Because frame_out table is not formed in successive order, it has to be zeroed first and
then ones can be set.

@param      frame_out Shuffled output frame
@param      frame_in  Unshuffled input frame
@param      bit_order Table holding the position of bits in shuffled order
@param      n_bits Number of bits to be shuffled
*/
MGW_LOCAL C_INLINE void
shuffle_frame(uint8 *frame_out, const uint8 *frame_in,
              const uint16 *bit_order, uint16 n_bits)
{
  struct bit_position n = get_bit_position_from_bit_number(n_bits);
  uint16 in_uint8_index = 0;
  uint16 frame_in_uint8 = 0;
  uint16 bit_index = 0;
  uint16 wanted_bit_on_left = 0;

  (void) memset(frame_out, 0, SIZE_1_TO_8(UPD_PAYLOAD_AMRNB_BITS));

  for (in_uint8_index = 0; in_uint8_index < n.uint8_in_table; in_uint8_index++) {
    frame_in_uint8 = *frame_in++;
    for (bit_index = 0; bit_index < 8; bit_index++) {
      wanted_bit_on_left = frame_in_uint8 & 0x80;
      set_out_bit(frame_out, *bit_order++, wanted_bit_on_left);
      frame_in_uint8 <<= 1;
    }
  }
  frame_in_uint8 = *frame_in;
  for (bit_index = 0; bit_index < n.bit_in_uint8; bit_index++) {
    wanted_bit_on_left = frame_in_uint8 & 0x80;
    set_out_bit(frame_out, *bit_order++, wanted_bit_on_left);
    frame_in_uint8 <<= 1;
  }
}

/*****************************************************************************************/
/**
@brief      De shuffle (permute) frame of bits

Bits to frame_out table are formed in succesive order. Each bit is mapped from a position
according to bit_order table, which is also iterated in succesive order.
Outer loop iterates through array elements. Inner loop forms bits to one uint8 element.
Last loop forms the bits into last incomplete uint8 entry. Because frame_out table is
formed in succesive order, single uint8 element can be zeroed at time and the ones can be
set for that uint8.

@param      frame_out De shuffled output frame
@param      frame_in Shuffled input frame
@param      bit_order Table holding the postition of bits in shuffled order
@param      n_bits Number of bits to be de shuffled
*/
MGW_LOCAL C_INLINE void
de_shuffle_frame(uint8 *frame_out, const uint8 *frame_in,
                 const uint16 *bit_order, uint16 n_bits)
{
  struct bit_position n = get_bit_position_from_bit_number(n_bits);
  uint16 out_uint8_index = 0;
  uint16 frame_out_uint8 = 0;
  uint16 bit_index = 0;

  for (out_uint8_index = 0; out_uint8_index < n.uint8_in_table; out_uint8_index++) {
    frame_out_uint8 = 0;
    for (bit_index = 0; bit_index < 8; bit_index++) {
      frame_out_uint8 <<= 1;
      frame_out_uint8 |= get_bit_on_right(frame_in, *bit_order++);
    }
    *frame_out++ = frame_out_uint8;
  }
  frame_out_uint8 = 0;
  for (bit_index = 0; bit_index < n.bit_in_uint8; bit_index++) {
    frame_out_uint8 <<= 1;
    frame_out_uint8 |= get_bit_on_right(frame_in, *bit_order++);
  }
  frame_out_uint8 <<= 8 - n.bit_in_uint8;
  *frame_out = frame_out_uint8;
}

/*****************************************************************************************/
/**
@brief      Shuffles AMR-NB payload bits

Sorts AMR-NB payload bits in the order defined in 3GPP TS 26.101. Read bits one by one from
input table and put the bit in the place defined in the bit order table. Does nothing if
mode is invalid. See also #l_upd_de_shuffle_amrnb_payload.

@param      payload_in Input payload
@param      payload_out Output payload
@param      mode AMR-NB UPD mode
*/
MGW_GLOBAL void
l_upd_shuffle_amrnb_payload(const uint8 *payload_in, uint8 *payload_out,
                            upd_mode_amrnb_t mode)
{
  uint16 bits = 0;
  const uint16 *bit_order = NULL;

  if (mode <= UPD_MODE_AMRNB_1220) {
    bits = amrnb_payload_size_in_bits[mode];
    bit_order = amrnb_frame_bit_orders[mode];

    shuffle_frame(payload_out, payload_in, bit_order, bits);
  }
}

/*****************************************************************************************/
/**
@brief      De-shuffles AMR-NB payload bits

Reverses the #l_upd_shuffle_amrnb_payload operation. Read bits one by one from input table
and put the bit in the place defined in the bit order table. Does nothing if mode is
invalid.

@param      payload_in Input payload
@param      payload_out Output payload
@param      mode AMR-NB UPD mode
*/
MGW_GLOBAL void
l_upd_de_shuffle_amrnb_payload(const uint8 *payload_in, uint8 *payload_out,
                               upd_mode_amrnb_t mode)
{
  uint16 bits = 0;
  const uint16 *bit_order = NULL;

  if (mode <= UPD_MODE_AMRNB_1220) {
    bits = amrnb_payload_size_in_bits[mode];
    bit_order = amrnb_frame_bit_orders[mode];

    de_shuffle_frame(payload_out, payload_in, bit_order, bits);
  }
}

/*****************************************************************************************/
/**
@brief      Gets the size of PCM payload in samples

@param      mode PCM mode
@return     Size of PCM payload in samples
*/
MGW_GLOBAL uint16
l_upd_get_pcm_size_samples(upd_mode_pcm_t mode)
{
  if (mode == UPD_MODE_PCM_05_MS) {
    return PCM_05_MS_SIZE_SAMPLES;
  } else if (mode == UPD_MODE_PCM_10_MS) {
    return PCM_10_MS_SIZE_SAMPLES;
  } else if (mode == UPD_MODE_PCM_20_MS) {
    return PCM_20_MS_SIZE_SAMPLES;
  } else if (mode == UPD_MODE_PCM_30_MS) {
    return PCM_30_MS_SIZE_SAMPLES;
  } else {
    return 0;
  }
}

/*****************************************************************************************/
/**
@brief      Gets PCM mode

@param      pcm_size_samples Size of PCM payload in samples
@return     PCM mode
*/
MGW_GLOBAL upd_mode_pcm_t
l_upd_get_pcm_mode(uint16 pcm_size_samples)
{
  if (pcm_size_samples == PCM_30_MS_SIZE_SAMPLES) {
    return UPD_MODE_PCM_30_MS;
  } else if (pcm_size_samples == PCM_20_MS_SIZE_SAMPLES) {
    return UPD_MODE_PCM_20_MS;
  } else if (pcm_size_samples == PCM_10_MS_SIZE_SAMPLES) {
    return UPD_MODE_PCM_10_MS;
  } else if (pcm_size_samples == PCM_05_MS_SIZE_SAMPLES) {
    return UPD_MODE_PCM_05_MS;
  } else {
    return UPD_MODE_NOT_APPLICABLE;
  }
}

/*****************************************************************************************/
/**
@brief      Checks whether type is valid for given speech codec

@param      codec UPD speech codec
@param      type UPD type
@return     TRUE if type is valid for given speech codec, FALSE otherwise
*/
MGW_GLOBAL bool16
l_upd_is_type_valid(upd_codec_t codec, upd_type_t type)
{
  uint16 types = 0;

  switch (codec) {
  case UPD_CODEC_PCM:
    types = UPD_TYPES_PCM;
    break;
  case UPD_CODEC_GSM_FR:
    types = UPD_TYPES_GSM_FR;
    break;
  case UPD_CODEC_GSM_EFR:
    types = UPD_TYPES_GSM_EFR;
    break;
  case UPD_CODEC_GSM_HR:
    types = UPD_TYPES_GSM_HR;
    break;
  case UPD_CODEC_AMRNB:
    types = (UPD_TYPES_AMRNB_GSM | UPD_TYPES_AMRNB_UMTS);
    break;
  case UPD_CODEC_AMRWB:
    types = (UPD_TYPES_AMRWB_GSM | UPD_TYPES_AMRWB_UMTS);
    break;
  case UPD_CODEC_BFP:
    types = UPD_TYPES_BFP;
    break;
  case UPD_CODEC_ILBC:
    types = UPD_TYPES_ILBC;
    break;
  case UPD_CODEC_G723:
    types = UPD_TYPES_G723;
    break;
  case UPD_CODEC_G729:
    types = UPD_TYPES_G729;
    break;
  default:
    break;
  }

  return L_UPD_IS_TYPE_IN_TYPES(type, types);
}

/*****************************************************************************************/
/**
@brief      Gets the number of modes codec possesses

@param      codec UPD speech codec
@return     Number of modes
*/
MGW_GLOBAL uint16
l_upd_get_number_of_modes(upd_codec_t codec, uint16 mode)
{
  switch (codec) {
  case UPD_CODEC_PCM:
    return UPD_NUMBER_OF_PCM_MODES;
  case UPD_CODEC_AMRNB:
    return UPD_NUMBER_OF_AMRNB_MODES;
  case UPD_CODEC_AMRWB:
    return UPD_NUMBER_OF_AMRWB_MODES;
  case UPD_CODEC_BFP:
    return UPD_NUMBER_OF_BFP_MODES;
  case UPD_CODEC_ILBC:
    return UPD_NUMBER_OF_ILBC_MODES;
  case UPD_CODEC_G723:
    return UPD_NUMBER_OF_G723_MODES;
  case UPD_CODEC_EVS:
    if( mode == UPD_EVS_PRIMARY_MODE ){
      return UPD_NUMBER_OF_EVS_PRIMARY_MODES;
    }else if( mode == UPD_EVS_WBIO_MODE ){
      return UPD_NUMBER_OF_EVS_WBIO_MODES;
    } 
    return 0;
  default:
    break;
  }

  return 0;
}

/*****************************************************************************************/
/**
@brief      Swaps UPD packet endianess from native to big-endian

@param      upd_packet UPD packet
*/
MGW_GLOBAL void
l_upd_nat_to_be(upd_packet_t *upd_packet)
{
#ifdef BOARD_X86_VIRTUAL
  uint16 *upd_payload = NULL;
  uint16 upd_payload_size_16 = 0;
  uint16 i = 0;

  if (upd_packet != NULL) {
    if ((upd_packet->codec == UPD_CODEC_PCM) &&
        ((upd_packet->form == UPD_FORM_PCM_ANY_8_BIT) ||
         (upd_packet->form == UPD_FORM_PCM_G711_A_LAW) ||
         (upd_packet->form == UPD_FORM_PCM_G711_U_LAW) ||
         (upd_packet->form == UPD_FORM_PCM_UDI))) {
      /* Content is octets, no byte order conversions */
    } else {
      upd_payload_size_16 = SIZE_1_TO_16(l_upd_get_payload_size_1(upd_packet));
      upd_payload = (uint16 *) upd_packet->payload_8;
      for (i = 0; i < upd_payload_size_16; i++) {
        upd_payload[i] = L_MGW_NAT_TO_BE_16(upd_payload[i]);
      }
    }
    *((uint16 *) upd_packet) = L_MGW_NAT_TO_BE_16(*((uint16 *) upd_packet));
  }
#else
  (void) upd_packet;
#endif
}

/*****************************************************************************************/
/**
@brief      Swaps UPD packet endianess from big-endian to native

@param      upd_packet UPD packet
*/
MGW_GLOBAL void
l_upd_be_to_nat(upd_packet_t *upd_packet)
{
#ifdef BOARD_X86_VIRTUAL
  uint16 *upd_payload = NULL;
  uint16 upd_payload_size_16 = 0;
  uint16 i = 0;

  if (upd_packet != NULL) {
    *((uint16 *) upd_packet) = L_MGW_BE_TO_NAT_16(*((uint16 *) upd_packet));
    if ((upd_packet->codec == UPD_CODEC_PCM) &&
        ((upd_packet->form == UPD_FORM_PCM_ANY_8_BIT) ||
         (upd_packet->form == UPD_FORM_PCM_G711_A_LAW) ||
         (upd_packet->form == UPD_FORM_PCM_G711_U_LAW) ||
         (upd_packet->form == UPD_FORM_PCM_UDI))) {
      /* Content is octets, no byte order conversions */
    } else {
      upd_payload_size_16 = SIZE_1_TO_16(l_upd_get_payload_size_1(upd_packet));
      upd_payload = (uint16 *) upd_packet->payload_8;
      for (i = 0; i < upd_payload_size_16; i++) {
        upd_payload[i] = L_MGW_BE_TO_NAT_16(upd_payload[i]);
      }
    }
  }
#else
  (void) upd_packet;
#endif
}

/*****************************************************************************************/
/**
@brief      Generate 32-bit extra error code from UPD-header in native endianess.

@param      UPD packet
@return     Extra error code
*/
MGW_GLOBAL uint32
l_upd_generate_extra_ecode_32(upd_packet_t *upd_packet)
{
  uint32 extra_ecode = 0;

  if (upd_packet != NULL) {
    *((uint16 *) upd_packet) = L_MGW_NAT_TO_BE_16(*((uint16 *) upd_packet));
    extra_ecode = L_MGW_NAT_TO_BE_32(*((uint32 *) upd_packet));
    *((uint16 *) upd_packet) = L_MGW_BE_TO_NAT_16(*((uint16 *) upd_packet));
  }

  return extra_ecode;
}

/*****************************************************************************************/
/**
@brief      Convert the UPD-header (16-bit) from big endian to native endianess

@param      upd_packet UPD packet
*/
MGW_GLOBAL void
l_upd_header_be_to_nat(upd_packet_t *upd_packet)
{
  if (upd_packet != NULL) {
    *((uint16 *) upd_packet) = L_MGW_BE_TO_NAT_16(*((uint16 *) upd_packet));
  }
}

/*****************************************************************************************/
/**
@brief      Convert the UPD-header (16-bit) from native endianess
            to big endian

@param      upd_packet UPD packet
*/
MGW_GLOBAL void
l_upd_header_nat_to_be(upd_packet_t *upd_packet)
{
  if (upd_packet != NULL) {
    *((uint16 *) upd_packet) = L_MGW_NAT_TO_BE_16(*((uint16 *) upd_packet));
  }
}

/*****************************************************************************************/
/**
@brief      Returns pointer to upd packet payload

@param      upd_packet UPD packet
@return     Pointer to upd packet payload
*/
MGW_GLOBAL void *
l_upd_get_payload(const upd_packet_t *upd_packet)
{
  return (void *) upd_packet->payload_8;
}

/*****************************************************************************************/
/**
@brief      Returns the type of the upd packet

@param      upd_packet UPD packet
@return     UPD packet's type
*/
MGW_GLOBAL upd_type_t
l_upd_get_type(const upd_packet_t *upd_packet)
{
  if (upd_packet->codec == UPD_CODEC_OPUS) {
    if (l_upd_get_opus_payload_size_8(upd_packet) == 0) {
      return UPD_TYPE_NO_DATA;
    } else {
      return UPD_TYPE_SPEECH_GOOD;
    }
  }

  return upd_packet->type;
}

/*****************************************************************************************/
/**
@brief      Gets the sample rate of a PCM stored in a UPD packet

@param      upd_packet UPD packet
@return     Sample rate in Hz
*/
MGW_GLOBAL uint16
l_upd_get_pcm_sample_rate_hz(const upd_packet_t *upd_packet)
{
  if ((upd_packet->form == UPD_FORM_PCM_LINEAR_16_KHZ) ||
      (upd_packet->form == UPD_FORM_PCM_G722)) {
    return L_UPD_WB_SAMPLE_RATE_HZ;
  } else {
    return L_UPD_NB_SAMPLE_RATE_HZ;
  }
}

/*****************************************************************************************/
/**
@brief      Gets form for the linear pcm UPD packet, depending on sample rate

@param      sample_rate_hz Sample rate in Hz
@return     Form of the UPD packet
*/
MGW_GLOBAL upd_form_t
l_upd_get_lin_pcm_form(uint16 sample_rate_hz)
{
  upd_form_t form = UPD_FORM_PCM_NOT_APPLICABLE;

  if (sample_rate_hz == L_UPD_NB_SAMPLE_RATE_HZ) {
    form = UPD_FORM_PCM_LINEAR_08_KHZ;
  } else {
    form = UPD_FORM_PCM_LINEAR_16_KHZ;
  }

  return form;
}

/*****************************************************************************************/
/**
@brief      Gets the target average bitrate for encoder

For variable bitrate codecs the target average bitrate is calculated according to call
control parameters. For constant bitrate codecs the target average bitrate equals its
mode.

@param      codec Codec
@param      mode Mode request for the encoder
@param      opus_max_average_bitrate_request Maximum average bitrate request for Opus codec
@param      sample_rate_hz Sample rate of the linear PCM to be encoded
@return     Target average bitrate for the encoder
*/
MGW_GLOBAL uint16
l_upd_get_target_average_bitrate_for_enc(upd_codec_t codec, upd_mode_t mode,
                                         uint32 opus_max_average_bitrate_request,
                                         uint16 sample_rate_hz)
{
  uint16 target_average_bitrate = l_upd_get_bitrate(codec, mode);

  if (codec == UPD_CODEC_OPUS) {
    target_average_bitrate = opus_max_average_bitrate_request;

    if ((sample_rate_hz == L_UPD_NB_SAMPLE_RATE_HZ) &&
        ((opus_max_average_bitrate_request >= L_UPD_OPUS_NB_DEFAULT_BITRATE) ||
         (opus_max_average_bitrate_request < L_UPD_OPUS_MINIMUM_BITRATE))) {
      target_average_bitrate = L_UPD_OPUS_NB_DEFAULT_BITRATE;
    }

    if ((sample_rate_hz == L_UPD_WB_SAMPLE_RATE_HZ) &&
        ((opus_max_average_bitrate_request >= L_UPD_OPUS_WB_DEFAULT_BITRATE) ||
         (opus_max_average_bitrate_request < L_UPD_OPUS_MINIMUM_BITRATE))) {
      target_average_bitrate = L_UPD_OPUS_WB_DEFAULT_BITRATE;
    }
  }

  return target_average_bitrate;
}

/*****************************************************************************************/
/**
@brief      Gets the sample rate of the Opus encoded data

Sample rate is read from the packet's payload as defined in RFC 6716.

@param      opus_packet Opus packet
@return     Sample rate in Hz
*/
MGW_LOCAL C_INLINE uint16
get_opus_sample_rate_hz(uint16 *opus_packet)
{
  uint8 opus_config = (uint8) (opus_packet[0] >> 11);
  uint16 sample_rate_hz = 0;

  if ((opus_config % 16) <= 3) {
    sample_rate_hz = L_UPD_NB_SAMPLE_RATE_HZ;
  } else if ((opus_config >= 4) && (opus_config <= 7)) {
    sample_rate_hz = L_UPD_MB_SAMPLE_RATE_HZ;
  } else if (((opus_config >= 8) && (opus_config <= 11)) ||
             ((opus_config >= 20) && (opus_config <= 23))) {
    sample_rate_hz = L_UPD_WB_SAMPLE_RATE_HZ;
  } else if (((opus_config >= 12) && (opus_config <= 13)) ||
             ((opus_config >= 24) && (opus_config <= 27))) {
    sample_rate_hz = L_UPD_SWB_SAMPLE_RATE_HZ;
  } else {
    sample_rate_hz = L_UPD_FB_SAMPLE_RATE_HZ;
  }

  return sample_rate_hz;
}

/*****************************************************************************************/
/**
@brief      Gets the sample rate from evs bandwidth

@param      evs bandwidth
@return     Sample rate in Hz
*/
MGW_GLOBAL uint16
get_evs_sample_rate_hz(const uint16 ingress_bandwidth)
{
  uint16 sample_rate_hz = 0;

  switch (ingress_bandwidth) {
  case L_UPD_EVS_CIT_BW_NARROWBAND:
    sample_rate_hz = L_UPD_NB_SAMPLE_RATE_HZ;
    break;
  case L_UPD_EVS_CIT_BW_WIDEBAND:
    sample_rate_hz = L_UPD_WB_SAMPLE_RATE_HZ;
    break;
  case L_UPD_EVS_CIT_BW_SUPERWIDEBAND:
    sample_rate_hz = L_UPD_SWB_SAMPLE_RATE_HZ;
    break;
  case L_UPD_EVS_CIT_BW_FULLBAND:
    sample_rate_hz = L_UPD_FB_SAMPLE_RATE_HZ;
    break;
  default:
    sample_rate_hz = L_UPD_WB_SAMPLE_RATE_HZ;
    break;
  }

  return sample_rate_hz;
}


/*****************************************************************************************/
/**
@brief      Gets the sample rate of the UPD packet

@param      upd_packet UPD packet
@return     Sample rate in Hz
*/
MGW_GLOBAL uint16
l_upd_get_sample_rate_hz(const upd_packet_t *upd_packet)
{
  uint16 sample_rate_hz = 0;

  switch (upd_packet->codec) {
  case UPD_CODEC_PCM:
    sample_rate_hz = l_upd_get_pcm_sample_rate_hz(upd_packet);
    break;
  case UPD_CODEC_GSM_FR:
  case UPD_CODEC_GSM_EFR:
  case UPD_CODEC_GSM_HR:
  case UPD_CODEC_AMRNB:
  case UPD_CODEC_ILBC:
  case UPD_CODEC_G723:
  case UPD_CODEC_G729:
    sample_rate_hz = L_UPD_NB_SAMPLE_RATE_HZ;
    break;
  case UPD_CODEC_AMRWB:
  case UPD_CODEC_BFP:
  case UPD_CODEC_EVS:
    sample_rate_hz = L_UPD_WB_SAMPLE_RATE_HZ;
    break;
  case UPD_CODEC_OPUS:
    sample_rate_hz = get_opus_sample_rate_hz(l_upd_get_payload(upd_packet));
    break;
  default:
    sample_rate_hz = L_UPD_NB_SAMPLE_RATE_HZ;
    break;
  }

  return sample_rate_hz;
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

/*****************************************************************************************/

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


