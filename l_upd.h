/*****************************************************************************************/
/**
@brief User Plane Data packet (UPD)

UPD packets are used to transfer User Plane Data inside the Termination Framework. UPD
packet consists of 32-bit header and variable length payload. The length of the payload is
defined by #l_upd_get_payload_size_1 function.

<pre>
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| codec | type  | mode  | form  |            padding            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            payload                            |
|                             .....                             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
</pre>

The contents of the first header field, codec, defines the meaning of the other header
fields and the payload.

  @section Codecs

  @subsection PCM

PCM payload duration is 5 ms, 10 ms, 20 ms or 30 ms. In speech frames form specifies the
encoding of the payload. In SID frames form specifies the size of the payload in octets.
SID payload is defined in IETF RFC 3389.

<pre>
  type, upd_type_t
  mode, upd_mode_pcm_t
  form, upd_form_pcm_t or the size of SID payload
</pre>

  @subsection gsm_fr GSM FR

GSM FR payload duration is 20 ms. Speech frame contains 260 bits as defined in 3GPP 46.010
(Table 1.1). SID frame contains 260 bits as defined in 3GPP 46.012 (Section 5.2).

<pre>
  type, upd_type_t
  mode, UPD_MODE_NOT_APPLICABLE
  form, UPD_FORM_NOT_APPLICABLE
</pre>

  @subsection gsm_efr GSM EFR

GSM EFR payload duration is 20 ms. Speech frame contains 244 bits as defined in 3GPP 46.060
(Table 6). SID frame contains 244 bits as defined in 3GPP 46.062 (Section 5.3).

<pre>
  type, upd_type_t
  mode, UPD_MODE_NOT_APPLICABLE
  form, UPD_FORM_NOT_APPLICABLE
</pre>

  @subsection gsm_hr GSM HR

GSM HR payload duration is 20 ms. Speech frame contains 112 bits as defined in 3GPP 46.020
(Annex B). SID frame contains 112 bits as defined in 3GPP 46.022 (Section 5.3).

<pre>
  type, upd_type_t
  mode, UPD_MODE_NOT_APPLICABLE
  form, UPD_FORM_NOT_APPLICABLE
</pre>

  @subsection amrnb AMR-NB

AMR-NB payload duration is 20 ms. Depending on the used mode speech frame contains 95-244
bits as defined in 3GPP 26.090 (Section 7). SID frame contains 39 bits as defined in 3GPP
26.101 (Section 4.2.3).

<pre>
  type, upd_type_t
  mode, upd_mode_amrnb_t
  form, UPD_FORM_NOT_APPLICABLE
</pre>

  @subsection amrwb AMR-WB

AMR-WB payload duration is 20 ms. Depending on the used mode speech frame contains 132-477
bits as defined in 3GPP 26.201 (Section 4.2.1). SID frame contains 40 bits as defined in
3GPP 26.201 (Section 4.2.3).

<pre>
  type, upd_type_t
  mode, upd_mode_amrwb_t
  form, UPD_FORM_NOT_APPLICABLE
</pre>

  @subsection bfp Block Floating Point (BFP)

BFP payload duration is 5 ms, 10 ms, 20 ms or 30 ms. Depending on the used mode speech
frame contains 640-3840 bits as defined in BFP module.

<pre>
  type, upd_type_t
  mode, upd_mode_bfp_t
  form, UPD_FORM_NOT_APPLICABLE
</pre>

  @subsection ilbc ILBC

ILBC payload duration is 20 ms or 30 ms. Depending on the used mode speech frame contains
304 or 400 bits as defined in IETF RFC 3952. In SID frames form specifies the size of the
payload in octets. SID payload is defined in IETF RFC 3389.

<pre>
  type, upd_type_t
  mode, upd_mode_ilbc_t
  form, UPD_FORM_NOT_APPLICABLE or the size of SID payload
</pre>

  @subsection g723 G.723

G.723 payload duration is 30 ms. Depending on the used mode speech frame contains 160 or
192 bits as defined in ITU-T G.723.1 (Table 5 and Table 6). SID frame contains 32 bits as
defined in ITU-T G.723.1 Annex A (Table A.1).

<pre>
  type, upd_type_t
  mode, upd_mode_g723_t
  form, UPD_FORM_NOT_APPLICABLE
</pre>

  @subsection g729 G.729

G.729 payload duration is 10 ms. Speech frame contains 80 bits as defined in ITU-T G.729
(Table 8). SID frame contains 15 bits as defined in ITU-T G.729 Annex B (Table B.2).

<pre>
  type, upd_type_t
  mode, UPD_MODE_NOT_APPLICABLE
  form, UPD_FORM_NOT_APPLICABLE
</pre>

  @subsection data Data

Represents all non-speech data encodings (except PCM UDI).

<pre>
  type, meaning is specified by form
  mode, meaning is specified by form
  form, upd_form_data_t
</pre>

  @subsection opus Opus

Opus payload duration can vary from 2.5 ms to 120 ms. Opus is a variable bitrate codec and
thus the size of each packet is informed within the UPD header, i.e. 12-bit size
information is delivered in the type, mode and form fields. The packet size can vary from 0
to 7662 octets. The maximum supported size is 960 octets which is the maximum UPD payload
size. No data frames are indicated with packet size of 0.

<pre>
  type, high 4 bits of packet size information
  mode, middle 4 bits of packet size information
  form, low 4 bits of packet size information
</pre>

*/

/*****************************************************************************************/
/****    INCLUDE GUARD    ****************************************************************/

#ifndef UPD_H
#define UPD_H

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/

#include <type_def.h>
#include <bitfields.h>

/*****************************************************************************************/
/****    2 CONSTANTS    ******************************************************************/

#define UPD_CODEC_UNDEFINED 0 /**< Codec Undefined (#upd_codec_t) */
#define UPD_CODEC_PCM       1 /**< Codec PCM (#upd_codec_t) */
#define UPD_CODEC_GSM_FR    2 /**< Codec GSM FR (#upd_codec_t) */
#define UPD_CODEC_GSM_EFR   3 /**< Codec GSM EFR (#upd_codec_t) */
#define UPD_CODEC_GSM_HR    4 /**< Codec GSM HR (#upd_codec_t) */
#define UPD_CODEC_AMRNB     5 /**< Codec AMR-NB (#upd_codec_t) */
#define UPD_CODEC_AMRWB     6 /**< Codec AMR-WB (#upd_codec_t) */
#define UPD_CODEC_BFP       7 /**< Codec BFP (#upd_codec_t) */
#define UPD_CODEC_OPUS      8 /**< Codec OPUS (#upd_codec_t) */
#define UPD_CODEC_ILBC      9 /**< Codec ILBC (#upd_codec_t) */
#define UPD_CODEC_G723     10 /**< Codec G.723 (#upd_codec_t) */
#define UPD_CODEC_G729     11 /**< Codec G.729 (#upd_codec_t) */
#define UPD_CODEC_EVRC     12 /**< Codec EVRC (#upd_codec_t) */
#define UPD_CODEC_DATA     13 /**< Codec Data (#upd_codec_t) */
#define UPD_CODEC_EVS      14 /**< Codec EVS (#upd_codec_t)*/


#define UPD_TYPE_NO_DATA          0 /**< Type No Data (#upd_type_t) */
#define UPD_TYPE_SPEECH_GOOD      1 /**< Type Speech Good (#upd_type_t) */
#define UPD_TYPE_SPEECH_BAD       2 /**< Type Speech Bad (#upd_type_t) */
#define UPD_TYPE_SPEECH_DEGRADED  3 /**< Type Speech Degraded (#upd_type_t) */
#define UPD_TYPE_SPEECH_LOST      4 /**< Type Speech Lost (#upd_type_t) */
#define UPD_TYPE_SID_FIRST        5 /**< Type SID First (#upd_type_t) */
#define UPD_TYPE_SID_UPDATE       6 /**< Type SID Update (#upd_type_t) */
#define UPD_TYPE_SID_BAD          7 /**< Type SID Bad (#upd_type_t) */
#define UPD_TYPE_SID_UNRELIABLE   8 /**< Type SID Unreliable (#upd_type_t) */
#define UPD_TYPE_ONSET            9 /**< Type Onset (#upd_type_t) */
#define UPD_TYPE_SPEECH_NO_DATA  10 /**< Type Speech No Data (#upd_type_t) */
#define UPD_TYPE_SID_NO_DATA     11 /**< Type SID No Data (#upd_type_t) */
#define UPD_TYPE_SID_FOR_FEATURE 12 /**< Type SID No Data (#upd_type_t) */
#define UPD_TYPE_NOT_APPLICABLE  15 /**< Type Not Applicable (#upd_type_t) */


#define UPD_MODE_NOT_APPLICABLE 15 /**< UPD mode Not Applicable (#upd_mode_t) */


#define UPD_MODE_PCM_05_MS 0 /**< PCM UPD mode 5 ms (#upd_mode_pcm_t) */
#define UPD_MODE_PCM_20_MS 1 /**< PCM UPD mode 20 ms (#upd_mode_pcm_t) */
#define UPD_MODE_PCM_10_MS 2 /**< PCM UPD mode 10 ms (#upd_mode_pcm_t) */
#define UPD_MODE_PCM_30_MS 3 /**< PCM UPD mode 30 ms (#upd_mode_pcm_t) */
#define UPD_NUMBER_OF_PCM_MODES 4 /**< Number of PCM UPD modes */


#define UPD_MODE_AMRNB_0475 0 /**< AMR-NB UPD mode 4.75 kbit/s (#upd_mode_amrnb_t)*/
#define UPD_MODE_AMRNB_0515 1 /**< AMR-NB UPD mode 5.15 kbit/s (#upd_mode_amrnb_t)*/
#define UPD_MODE_AMRNB_0590 2 /**< AMR-NB UPD mode 5.90 kbit/s (#upd_mode_amrnb_t)*/
#define UPD_MODE_AMRNB_0670 3 /**< AMR-NB UPD mode 6.70 kbit/s (#upd_mode_amrnb_t)*/
#define UPD_MODE_AMRNB_0740 4 /**< AMR-NB UPD mode 7.40 kbit/s (#upd_mode_amrnb_t)*/
#define UPD_MODE_AMRNB_0795 5 /**< AMR-NB UPD mode 7.95 kbit/s (#upd_mode_amrnb_t)*/
#define UPD_MODE_AMRNB_1020 6 /**< AMR-NB UPD mode 10.20 kbit/s (#upd_mode_amrnb_t)*/
#define UPD_MODE_AMRNB_1220 7 /**< AMR-NB UPD mode 12.20 kbit/s (#upd_mode_amrnb_t)*/
#define UPD_MODE_AMRNB_NOT_APPLICABLE 15 /**< Mode Not Applicable (#upd_mode_amrnb_t) */
#define UPD_NUMBER_OF_AMRNB_MODES 8 /**< Number of AMR-NB UPD modes */


#define UPD_MODE_AMRWB_0660 0 /**< AMR-WB UPD mode 6.60 kbit/s (#upd_mode_amrwb_t)*/
#define UPD_MODE_AMRWB_0885 1 /**< AMR-WB UPD mode 8.85 kbit/s (#upd_mode_amrwb_t)*/
#define UPD_MODE_AMRWB_1265 2 /**< AMR-WB UPD mode 12.65 kbit/s (#upd_mode_amrwb_t)*/
#define UPD_MODE_AMRWB_1425 3 /**< AMR-WB UPD mode 14.25 kbit/s (#upd_mode_amrwb_t)*/
#define UPD_MODE_AMRWB_1585 4 /**< AMR-WB UPD mode 15.85 kbit/s (#upd_mode_amrwb_t)*/
#define UPD_MODE_AMRWB_1825 5 /**< AMR-WB UPD mode 18.25 kbit/s (#upd_mode_amrwb_t)*/
#define UPD_MODE_AMRWB_1985 6 /**< AMR-WB UPD mode 19.85 kbit/s (#upd_mode_amrwb_t)*/
#define UPD_MODE_AMRWB_2305 7 /**< AMR-WB UPD mode 23.05 kbit/s (#upd_mode_amrwb_t)*/
#define UPD_MODE_AMRWB_2385 8 /**< AMR-WB UPD mode 23.85 kbit/s (#upd_mode_amrwb_t)*/
#define UPD_MODE_AMRWB_NOT_APPLICABLE 15 /**< Mode Not Applicable (#upd_mode_amrwb_t)*/
#define UPD_NUMBER_OF_AMRWB_MODES 9 /**< Number of AMR-WB UPD modes */


#define UPD_MODE_BFP_05_MS 0 /**< BFP UPD mode 5 ms (#upd_mode_bfp_t) */
#define UPD_MODE_BFP_10_MS 1 /**< BFP UPD mode 10 ms (#upd_mode_bfp_t) */
#define UPD_MODE_BFP_20_MS 2 /**< BFP UPD mode 20 ms (#upd_mode_bfp_t) */
#define UPD_MODE_BFP_30_MS 3 /**< BFP UPD mode 30 ms (#upd_mode_bfp_t) */
#define UPD_NUMBER_OF_BFP_MODES 4 /**< Number of BFP UPD modes */


/** AMR UPD mode (NB or WB) Not Applicable (#upd_mode_amr_t) */
#define UPD_MODE_AMR_NOT_APPLICABLE 15


#define UPD_MODE_DATA_NT_144 0 /**< NT Data UPD mode 14.4 kbit/s (#upd_mode_data_t) */
#define UPD_MODE_DATA_NT_288 1 /**< NT Data UPD mode 28.8 kbit/s (#upd_mode_data_t) */
#define UPD_MODE_DATA_NT_576 2 /**< NT Data UPD mode 57.6 kbit/s (#upd_mode_data_t) */
#define UPD_NUMBER_OF_DATA_NT_MODES 3 /**< Number of NT Data UPD modes */


#define UPD_MODE_ILBC_1333 0 /**< ILBC UPD mode 13.33 kbit/s (30 ms) (#upd_mode_ilbc_t) */
#define UPD_MODE_ILBC_1520 1 /**< ILBC UPD mode 15.20 kbit/s (20 ms) (#upd_mode_ilbc_t) */
#define UPD_NUMBER_OF_ILBC_MODES 2 /**< Number of ILBC UPD modes */


#define UPD_MODE_G723_0530 0 /**< G.723 UPD mode 5.3 kbit/s (#upd_mode_g723_t)*/
#define UPD_MODE_G723_0630 1 /**< G.723 UPD mode 6.3 kbit/s (#upd_mode_g723_t)*/
#define UPD_NUMBER_OF_G723_MODES 2 /**< Number of G.723 UPD modes */


#define UPD_MODE_EVRC_BLANK_RATE   0 /**< EVRC UPD mode Blanked frame (#upd_mode_evrc_t)*/
#define UPD_MODE_EVRC_EIGHTH_RATE  1 /**< EVRC UPD mode 1/8 rate frame (#upd_mode_evrc_t)*/
#define UPD_MODE_EVRC_QUARTER_RATE 2 /**< EVRC UPD mode 1/4 rate frame (#upd_mode_evrc_t)*/
#define UPD_MODE_EVRC_HALF_RATE    3 /**< EVRC UPD mode 1/2 rate frame (#upd_mode_evrc_t)*/
#define UPD_MODE_EVRC_FULL_RATE    4 /**< EVRC UPD mode 1 rate frame (#upd_mode_evrc_t)*/
#define UPD_NUMBER_OF_EVRC_MODES 5 /**< Number of EVRC UPD modes */


#define UPD_FORM_NOT_APPLICABLE 15 /**< UPD form Not Applicable (#upd_form_t) */


/** PCM UPD form any of the encodings with 8-bit samples except G.722 (#upd_form_pcm_t) */
#define UPD_FORM_PCM_ANY_8_BIT       0
/** PCM UPD form G.711 A-law as defined in ITU-T G.711 Section 3 (#upd_form_pcm_t) */
#define UPD_FORM_PCM_G711_A_LAW      1
/** PCM UPD form G.711 u-law as defined in ITU-T G.711 Section 3 (#upd_form_pcm_t) */
#define UPD_FORM_PCM_G711_U_LAW      2
/** PCM UPD form Unrestricted Digital Information: any other encoding with 8-bit samples
than A-law, u-law and G.722 (#upd_form_pcm_t) */
#define UPD_FORM_PCM_UDI             3
/** PCM UPD form 8 kHz linear (also known as uniform), big endian (#upd_form_pcm_t) */
#define UPD_FORM_PCM_LINEAR_08_KHZ   4
/** PCM UPD form 16 kHz linear (also known as uniform), big endian (#upd_form_pcm_t) */
#define UPD_FORM_PCM_LINEAR_16_KHZ   5
/** PCM UPD form G.722 as defined in ITU-T G.722 (#upd_form_pcm_t) */
#define UPD_FORM_PCM_G722            6
/** PCM UPD form Not Applicable (#upd_form_pcm_t) */
#define UPD_FORM_PCM_NOT_APPLICABLE 15


/** Data UPD form Transparent data (#upd_form_data_t) */
#define UPD_FORM_DATA_T_DATA          0
/** Data UPD form Non-transparent data (#upd_form_data_t) */
#define UPD_FORM_DATA_NT_DATA         1
/** Data UPD form T.38 data (#upd_form_data_t) */
#define UPD_FORM_DATA_T38             2
/** Data UPD form Not Applicable (#upd_form_data_t) */
#define UPD_FORM_DATA_NOT_APPLICABLE 15


/** UPD packet header size */
#define UPD_HEADER_SIZE_16        2
/** Maximum length of UPD payload (buffer), greater than or equal to the maximum payload
length of any codec */
#define UPD_MAX_PAYLOAD_SIZE_8  960
#define UPD_MAX_PAYLOAD_SIZE_16 480

/** UPD packet payload, PCM maximum frame length in bits (30 ms, 480 16-bit samples) */
#define UPD_PAYLOAD_PCM_BITS        7680
/** UPD packet payload, GSM FR speech frame length in bits */
#define UPD_PAYLOAD_GSM_FR_BITS      260
/** UPD packet payload, GSM FR SID frame length in bits */
#define UPD_PAYLOAD_GSM_FR_SID_BITS  260
/** UPD packet payload, GSM EFR speech frame length in bits */
#define UPD_PAYLOAD_GSM_EFR_BITS     244
/** UPD packet payload, GSM EFR SID frame length in bits */
#define UPD_PAYLOAD_GSM_EFR_SID_BITS 244
/** UPD packet payload, GSM HR speech frame length in bits */
#define UPD_PAYLOAD_GSM_HR_BITS      112
/** UPD packet payload, GSM HR SID frame length in bits */
#define UPD_PAYLOAD_GSM_HR_SID_BITS  112
/** UPD packet payload, AMR-NB speech maximum frame length in bits (highest mode) */
#define UPD_PAYLOAD_AMRNB_BITS       244
/** UPD packet payload, AMR-NB SID frame length in bits */
#define UPD_PAYLOAD_AMRNB_SID_BITS    39
/** UPD packet payload, AMR-WB speech maximum frame length in bits (highest mode) */
#define UPD_PAYLOAD_AMRWB_BITS       477
/** UPD packet payload, AMR-WB SID frame length in bits */
#define UPD_PAYLOAD_AMRWB_SID_BITS    40
/** UPD packet payload, BFP speech maximum frame length in bits (highest mode) */
#define UPD_PAYLOAD_BFP_BITS        3840
/** UPD packet payload, ILBC speech maximum frame length in bits (not the highest mode) */
#define UPD_PAYLOAD_ILBC_BITS        400
/** UPD packet payload, G.723 speech maximum frame length in bits (highest mode) */
#define UPD_PAYLOAD_G723_BITS        192
/** UPD packet payload, G.723 SID frame length in bits */
#define UPD_PAYLOAD_G723_SID_BITS     32
/** UPD packet payload, G.729 speech frame length in bits */
#define UPD_PAYLOAD_G729_BITS         80
/** UPD packet payload, G.729 SID frame length in bits */
#define UPD_PAYLOAD_G729_SID_BITS     15
/** UPD packet payload, Opus maximum supported packet length in bits */
#define UPD_PAYLOAD_MAX_OPUS_BITS   7680
/** UPD packet payload, EVRC speech maximum frame length in bits */
#define UPD_PAYLOAD_EVRC_BITS        171
/** UPD packet payload, transparent data frame length in bits */
#define UPD_PAYLOAD_DATA_T_BITS      640
/** UPD packet payload, non-transparent data frame length in bits */
#define UPD_PAYLOAD_DATA_NT_BITS     576
/** UPD packet payload, T.38 maximum frame length in bits (184 octets) */
#define UPD_PAYLOAD_DATA_T38_BITS   1472

/** UPD packet payload, 8-bit PCM frame length in bits (240 8-bit samples) */
#define UPD_PAYLOAD_PCM_30_MS_8_BIT_BITS  1920
/** UPD packet payload, 8-bit PCM frame length in bits (160 8-bit samples) */
#define UPD_PAYLOAD_PCM_20_MS_8_BIT_BITS  1280
/** UPD packet payload, 8-bit PCM frame length in bits (80 8-bit samples) */
#define UPD_PAYLOAD_PCM_10_MS_8_BIT_BITS   640
/** UPD packet payload, 8-bit PCM frame length in bits (40 8-bit samples) */
#define UPD_PAYLOAD_PCM_05_MS_8_BIT_BITS   320

/** UPD packet payload, linear 8 kHz PCM frame length in bits (240 16-bit samples) */
#define UPD_PAYLOAD_PCM_30_MS_LINEAR_08_KHZ_BITS 3840
/** UPD packet payload, linear 8 kHz PCM frame length in bits (160 16-bit samples) */
#define UPD_PAYLOAD_PCM_20_MS_LINEAR_08_KHZ_BITS 2560
/** UPD packet payload, linear 8 kHz PCM frame length in bits (80 16-bit samples) */
#define UPD_PAYLOAD_PCM_10_MS_LINEAR_08_KHZ_BITS 1280
/** UPD packet payload, linear 8 kHz PCM frame length in bits (40 16-bit samples) */
#define UPD_PAYLOAD_PCM_05_MS_LINEAR_08_KHZ_BITS  640

/** UPD packet payload, linear 16 kHz PCM frame length in bits (480 16-bit samples) */
#define UPD_PAYLOAD_PCM_30_MS_LINEAR_16_KHZ_BITS 7680
/** UPD packet payload, linear 16 kHz PCM frame length in bits (320 16-bit samples) */
#define UPD_PAYLOAD_PCM_20_MS_LINEAR_16_KHZ_BITS 5120
/** UPD packet payload, linear 16 kHz PCM frame length in bits (160 16-bit samples) */
#define UPD_PAYLOAD_PCM_10_MS_LINEAR_16_KHZ_BITS 2560
/** UPD packet payload, linear 16 kHz PCM frame length in bits (80 16-bit samples) */
#define UPD_PAYLOAD_PCM_05_MS_LINEAR_16_KHZ_BITS 1280

/** UPD packet payload, AMR-NB speech frame mode 12.2 length in bits */
#define UPD_PAYLOAD_AMRNB_1220_BITS  244
/** UPD packet payload, AMR-NB speech frame mode 10.2 length in bits */
#define UPD_PAYLOAD_AMRNB_1020_BITS  204
/** UPD packet payload, AMR-NB speech frame mode 7.95 length in bits */
#define UPD_PAYLOAD_AMRNB_0795_BITS  159
/** UPD packet payload, AMR-NB speech frame mode 7.4 length in bits */
#define UPD_PAYLOAD_AMRNB_0740_BITS  148
/** UPD packet payload, AMR-NB speech frame mode 6.7 length in bits */
#define UPD_PAYLOAD_AMRNB_0670_BITS  134
/** UPD packet payload, AMR-NB speech frame mode 5.9 length in bits */
#define UPD_PAYLOAD_AMRNB_0590_BITS  118
/** UPD packet payload, AMR-NB speech frame mode 5.15 length in bits */
#define UPD_PAYLOAD_AMRNB_0515_BITS  103
/** UPD packet payload, AMR-NB speech frame mode 4.75 length in bits */
#define UPD_PAYLOAD_AMRNB_0475_BITS   95

/** UPD packet payload, AMR-WB speech frame mode 23.85 length in bits */
#define UPD_PAYLOAD_AMRWB_2385_BITS  477
/** UPD packet payload, AMR-WB speech frame mode 23.05 length in bits */
#define UPD_PAYLOAD_AMRWB_2305_BITS  461
/** UPD packet payload, AMR-WB speech frame mode 19.85 length in bits */
#define UPD_PAYLOAD_AMRWB_1985_BITS  397
/** UPD packet payload, AMR-WB speech frame mode 18.25 length in bits */
#define UPD_PAYLOAD_AMRWB_1825_BITS  365
/** UPD packet payload, AMR-WB speech frame mode 15.85 length in bits */
#define UPD_PAYLOAD_AMRWB_1585_BITS  317
/** UPD packet payload, AMR-WB speech frame mode 14.25 length in bits */
#define UPD_PAYLOAD_AMRWB_1425_BITS  285
/** UPD packet payload, AMR-WB speech frame mode 12.65 length in bits */
#define UPD_PAYLOAD_AMRWB_1265_BITS  253
/** UPD packet payload, AMR-WB speech frame mode 8.85 length in bits */
#define UPD_PAYLOAD_AMRWB_0885_BITS  177
/** UPD packet payload, AMR-WB speech frame mode 6.60 length in bits */
#define UPD_PAYLOAD_AMRWB_0660_BITS  132

/** UPD packet payload, BFP speech frame mode 30 ms length in bits */
#define UPD_PAYLOAD_BFP_30_MS_BITS  3840
/** UPD packet payload, BFP speech frame mode 20 ms length in bits */
#define UPD_PAYLOAD_BFP_20_MS_BITS  2560
/** UPD packet payload, BFP speech frame mode 10 ms length in bits */
#define UPD_PAYLOAD_BFP_10_MS_BITS  1280
/** UPD packet payload, BFP speech frame mode 5 ms length in bits */
#define UPD_PAYLOAD_BFP_05_MS_BITS   640

/** UPD packet payload, ILBC speech frame mode 13.33 length in bits */
#define UPD_PAYLOAD_ILBC_1333_BITS  400
/** UPD packet payload, ILBC speech frame mode 15.20 length in bits */
#define UPD_PAYLOAD_ILBC_1520_BITS  304

/** UPD packet payload, G.723 speech frame mode 6.30 length in bits */
#define UPD_PAYLOAD_G723_0630_BITS  192
/** UPD packet payload, G.723 speech frame mode 5.30 length in bits */
#define UPD_PAYLOAD_G723_0530_BITS  160

/** UPD packet payload, EVRC full rate frame length in bits */
#define UPD_PAYLOAD_EVRC_FULL_RATE_BITS    171
/** UPD packet payload, EVRC half rate frame length in bits */
#define UPD_PAYLOAD_EVRC_HALF_RATE_BITS     80
/** UPD packet payload, EVRC quarter rate frame length in bits */
#define UPD_PAYLOAD_EVRC_QUARTER_RATE_BITS  40
/** UPD packet payload, EVRC eighth rate frame length in bits */
#define UPD_PAYLOAD_EVRC_EIGHTH_RATE_BITS   16

/** Allowed UPD Types for PCM */
#define UPD_TYPES_PCM         0x0443 /* 0000 0100 0100 0011 */
/** Allowed UPD Types for GSM FR */
#define UPD_TYPES_GSM_FR      0x0DE7 /* 0000 1101 1110 0111 */
/** Allowed UPD Types for GSM EFR */
#define UPD_TYPES_GSM_EFR     0x0DE7 /* 0000 1101 1110 0111 */
/** Allowed UPD Types for GSM HR */
#define UPD_TYPES_GSM_HR      0x0DEF /* 0000 1101 1110 1111 */
/** Allowed UPD Types for AMR-NB GSM */
#define UPD_TYPES_AMRNB_GSM   0x02EF /* 0000 0010 1110 1111 */
/** Allowed UPD Types for AMR-NB UMTS */
#define UPD_TYPES_AMRNB_UMTS  0x0CE7 /* 0000 1100 1110 0111 */
/** Allowed UPD Types for AMR-WB GSM */
#define UPD_TYPES_AMRWB_GSM   0x02EF /* 0000 0010 1110 1111 */
/** Allowed UPD Types for AMR-WB UMTS */
#define UPD_TYPES_AMRWB_UMTS  0x0CF7 /* 0000 1100 1111 0111 */
/** Allowed UPD Types for BFP */
#define UPD_TYPES_BFP         0x0003 /* 0000 0000 0000 0011 */
/** Allowed UPD Types for ILBC */
#define UPD_TYPES_ILBC        0x0043 /* 0000 0000 0100 0011 */
/** Allowed UPD Types for G.723 */
#define UPD_TYPES_G723        0x0C47 /* 0000 1100 0100 0111 */
/** Allowed UPD Types for G.729 */
#define UPD_TYPES_G729        0x0C47 /* 0000 1100 0100 0111 */

/**evs UPD size*/
#define UPD_PAYLOAD_EVS_56        56         /**Primary 2.8 kbps*/
#define UPD_PAYLOAD_EVS_144       144        /**Primary 7.2 kbps*/
#define UPD_PAYLOAD_EVS_160       160        /**Primary 8.0 kbps*/
#define UPD_PAYLOAD_EVS_192       192        /**Primary 9.6 kbps*/
#define UPD_PAYLOAD_EVS_264       264        /**Primary 13.2 kbps*/
#define UPD_PAYLOAD_EVS_328       328        /**Primary 16.4 kbps*/
#define UPD_PAYLOAD_EVS_488       488        /**Primary 24.4 kbps*/
#define UPD_PAYLOAD_EVS_640       640        /**Primary 32.0 kbps*/
#define UPD_PAYLOAD_EVS_960       960        /**Primary 48.0 kbps*/
#define UPD_PAYLOAD_EVS_1280      1280       /**Primary 64.0 kbps*/
#define UPD_PAYLOAD_EVS_1920      1920       /**Primary 96.0 kbps*/
#define UPD_PAYLOAD_EVS_2560      2560       /**Primary 128.0 kbps*/
#define UPD_PAYLOAD_EVS_48_SID    48         /**Primary 2.4kbps SID*/

#define UPD_PAYLOAD_EVS_132           132       /**AMR-WB IO 6.6 kbps*/
#define UPD_PAYLOAD_EVS_177           177       /**AMR-WB IO 8.85 kbps*/
#define UPD_PAYLOAD_EVS_253           253       /**AMR-WB IO 12.65 kbps*/
#define UPD_PAYLOAD_EVS_285           285       /**AMR-WB IO 14.25 kbps*/
#define UPD_PAYLOAD_EVS_317           317       /**AMR-WB IO 15.85 kbps*/
#define UPD_PAYLOAD_EVS_365           365       /**AMR-WB IO 18.25 kbps*/
#define UPD_PAYLOAD_EVS_397           397       /**AMR-WB IO 19.85 kbps*/
#define UPD_PAYLOAD_EVS_461           461       /**AMR-WB IO 23.05 kbps*/
#define UPD_PAYLOAD_EVS_477           477       /**AMR-WB IO 23.85 kbps*/
#define UPD_PAYLOAD_EVS_35_SID        35        /**AMR-WB IO 2.0 kbps SID*/


/*EVS UPD mode*/
#define UPD_MODE_EVS_PRIMARY_2800                0       /**EVS frame type  Primary 2.8 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_7200                1       /**EVS frame type  Primary 7.2 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_8000                2       /**EVS frame type  Primary 8.0 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_9600                3       /**EVS frame type  Primary 9.6 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_13200               4       /**EVS frame type  Primary 13.2 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_16400               5       /**EVS frame type  Primary 16.4 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_24400               6       /**EVS frame type  Primary 24.4 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_32000               7       /**EVS frame type  Primary 32.0 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_48000               8       /**EVS frame type  Primary 48.0 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_64000               9       /**EVS frame type  Primary 64.0 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_96000               10      /**EVS frame type  Primary 96.0 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_128000              11      /**EVS frame type  Primary 128.0 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_SID                 12      /**EVS frame type  Primary 2.4 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_PRIMARY_NOT_APPLICABLE      15      /**< Mode Not Applicable (#UPD_MODE_EVS_ft_t)*/
#define UPD_NUMBER_OF_EVS_PRIMARY_MODES          12      /**< Number of EVS_PRIMARY UPD modes */


#define UPD_MODE_EVS_WBIO_6600               0       /**EVS frame type  AMR-WB IO 6.6 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_WBIO_8850               1       /**EVS frame type  AMR-WB IO 8.85 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_WBIO_12650              2       /**EVS frame type  AMR-WB IO 12.65 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_WBIO_14250              3       /**EVS frame type  AMR-WB IO 14.25 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_WBIO_15850              4       /**EVS frame type  AMR-WB IO 15.85 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_WBIO_18250              5       /**EVS frame type  AMR-WB IO 18.25 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_WBIO_19850              6       /**EVS frame type  AMR-WB IO 19.85 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_WBIO_23050              7       /**EVS frame type  AMR-WB IO 23.05 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_WBIO_23850              8       /**EVS frame type  AMR-WB IO 23.85 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_WBIO_SID                9       /**EVS frame type  AMR-WB IO 2.0 kbps (#UPD_MODE_EVS_ft_t) */
#define UPD_MODE_EVS_WBIO_NOT_APPLICABLE     15      /**< Mode Not Applicable (#UPD_MODE_EVS_ft_t)*/
#define UPD_NUMBER_OF_EVS_WBIO_MODES         9       /**< Number of EVS_WBIOUPD modes */

#define UPD_MODE_EVS_NOT_APPLICABLE          15      /**< Mode Not Applicable (#UPD_MODE_EVS_ft_t)*/


#define UPD_EVS_PRIMARY_MODE        0
#define UPD_EVS_WBIO_MODE           1
#define UPD_EVS_UNKNOW_MODE         3

#define EVS_PRIMARY_MODE        0
#define EVS_WBIO_MODE           1
#define EVS_UNKNOW_MODE         3


/*evs header format*/
#define EVS_HDR_FMT_COMPACT   0
#define EVS_HDR_FMT_FULL      1
/* evs primary compact format has the same payload size as evs wbio head full SID */
#define EVS_HDR_FMT_PT_SIZE_56  2


/** UPD packet payload, EVS speech maximum frame length in bits (highest mode) */
#define UPD_PAYLOAD_EVS_MAX_BITS       2560

/*****************************************************************************************/
/****    3 MACROS    *********************************************************************/

/*****************************************************************************************/
/****    4 DATA TYPES    *****************************************************************/

/**
@brief      UPD codec

UPD codec defines the contents of the payload and the other header fields of UPD packet.
Possible values are 0-15 (length is 4 bits).
*/
typedef uint16 upd_codec_t;

/**
@brief      UPD type

Type describes the contents of speech frames (i.e. not UPD_CODEC_DATA). Following table
list types that are used with each supported codec.

Note that there may be external interface specific exceptions to this table, for further
details refer to /dsp/app/mgw/doc/frame_types/frames.vsd.

<pre>
upd_type_t               | PCM |GSM FR/|GSM HR|AMR-NB/|AMR-NB|AMR-WB| BFP |G.723/|ILBC |
                         |     |GSM EFR|      |AMR-WB | UMTS | UMTS |     |G.729 |     |
                         |     |       |      | GSM   |      |      |     |      |     |
----------------------------------------------------------------------------------------
UPD_TYPE_NO_DATA         | Yes |  Yes  |  Yes |  Yes  |  Yes |  Yes | Yes | Yes  | Yes |
UPD_TYPE_SPEECH_GOOD     | Yes |  Yes  |  Yes |  Yes  |  Yes |  Yes | Yes | Yes  | Yes |
UPD_TYPE_SPEECH_BAD      | No  |  Yes  |  Yes |  Yes  |  Yes |  Yes | No  | Yes  | No  |
UPD_TYPE_SPEECH_DEGRADED | No  |  No   |  Yes |  Yes  |  No  |  No  | No  | No   | No  |
UPD_TYPE_SPEECH_LOST     | No  |  No   |  No  |  No   |  No  |  Yes | No  | No   | No  |
UPD_TYPE_SID_FIRST       | No  |  Yes  |  Yes |  Yes  |  Yes |  Yes | No  | No   | No  |
UPD_TYPE_SID_UPDATE      | Yes |  Yes  |  Yes |  Yes  |  Yes |  Yes | No  | Yes  | Yes |
UPD_TYPE_SID_BAD         | No  |  Yes  |  Yes |  Yes  |  Yes |  Yes | No  | No   | No  |
UPD_TYPE_SID_UNRELIABLE  | No  |  Yes  |  Yes |  No   |  No  |  No  | No  | No   | No  |
UPD_TYPE_ONSET           | No  |  No   |  No  |  Yes  |  No  |  No  | No  | No   | No  |
UPD_TYPE_SPEECH_NO_DATA  | Yes |  Yes  |  Yes |  No   |  Yes |  Yes | No  | Yes  | No  |
UPD_TYPE_SID_NO_DATA     | No  |  Yes  |  Yes |  No   |  Yes |  Yes | No  | Yes  | No  |
UPD_TYPE_NOT_APPLICABLE  | No  |  No   |  No  |  No   |  No  |  No  | No  | No   | No  |
----------------------------------------------------------------------------------------
</pre>

Types of AMR-NB GSM are defined in 3GPP 26.093 Table 3 and Table 4
Types of AMR-NB UMTS are defined in 3GPP 26.093 Table 1 and Table 2.
Types of AMR-WB GSM are defined in 3GPP 26.193 Table 3 and Table 4.
Types of AMR-WB UMTS are defined in 3GPP 26.193 Table 1 and Table 2.

Types of GSM FR and GSM EFR are defined in terms of BFI, SID and TAF. See 3GPP 46.031 for
GSM FR and 3GPP 46.081 for GSM EFR. Following table shows BFI, SID and TAF can be mapped to
UPD type and vice versa. The values in parenthesis bear no meaning.

<pre>
upd_type_t               | BFI | SID | TAF |
--------------------------------------------
UPD_TYPE_SPEECH_GOOD     |  0  |  0  | (x) |
UPD_TYPE_SPEECH_BAD      |  1  |  0  | (x) |
UPD_TYPE_SID_FIRST       |  0  |  2  |  0  |
UPD_TYPE_SID_UPDATE      |  0  |  2  |  1  |
UPD_TYPE_SID_BAD         |  1  | 1,2 | (x) |
UPD_TYPE_SID_UNRELIABLE  |  0  |  1  | (x) |
UPD_TYPE_NO_DATA         |  1  |  0  |  0  |
--------------------------------------------

BFI = Bad Frame Indicator
SID = Silence Descriptor
TAF = Time Alignment Flag

Type of GSM HR is defined in terms of UFI in addition to BFI, SID and TAF. See 3GPP 46.041.
Mapping between the control bits and type is shown in the following table. The values in
parenthesis bear no meaning.

upd_type_t               | BFI | SID | TAF | UFI |
--------------------------------------------------
UPD_TYPE_SPEECH_GOOD     |  0  |  0  | (x) |  0  |
UPD_TYPE_SPEECH_BAD      |  1  |  0  | (x) | (x) |
UPD_TYPE_SPEECH_DEGRADED |  0  |  0  | (x) |  1  |
UPD_TYPE_SID_FIRST       |  0  |  2  |  0  |  0  |
UPD_TYPE_SID_UPDATE      |  0  |  2  |  1  |  0  |
UPD_TYPE_SID_BAD         |  1  |  2  | (x) | (x) |
UPD_TYPE_SID_UNRELIABLE  |  0  |  2  | (x) |  1  |
UPD_TYPE_NO_DATA         |  1  |  0  |  0  | (x) |
--------------------------------------------------

UFI = Unreliable Frame Indicator
</pre>

Possible values are 0-15 (length is 4 bits).
*/
typedef uint16 upd_type_t;

/**
@brief      UPD mode

UPD mode (#upd_mode_pcm_t, #upd_mode_amr_t, #upd_mode_ilbc_t, #upd_mode_g723_t,
#upd_mode_bfp_t or #upd_mode_data_t). Value UPD_MODE_NOT_APPLICABLE may be used with any
codec. Possible values are 0-15 (length is 4 bits).
*/
typedef uint16 upd_mode_t;

/**
@brief      PCM UPD mode

Mode defines PCM packet duration (5 ms, 10 ms, 20 ms or 30 ms).
*/
typedef uint16 upd_mode_pcm_t;

/**
@brief      AMR UPD mode

AMR-NB (#upd_mode_amrnb_t) or AMR-WB (#upd_mode_amrwb_t) mode.

Possible values depend on type as shown in the following table.

<pre>
upd_type_t               | UPD_MODE_AMR_NOT_APPLICABLE | other |
----------------------------------------------------------------
UPD_TYPE_NO_DATA         |             Yes             |  Yes  |
UPD_TYPE_SPEECH_GOOD     |             No              |  Yes  |
UPD_TYPE_SPEECH_BAD      |             No              |  Yes  |
UPD_TYPE_SPEECH_DEGRADED |             No              |  Yes  |
UPD_TYPE_SPEECH_LOST     |             Yes             |  No   |
UPD_TYPE_SID_FIRST       |             No              |  Yes  |
UPD_TYPE_SID_UPDATE      |             No              |  Yes  |
UPD_TYPE_SID_BAD         |             No              |  Yes  |
UPD_TYPE_SID_UNRELIABLE  |              -              |   -   |
UPD_TYPE_ONSET           |             No              |  Yes  |
UPD_TYPE_SPEECH_NO_DATA  |             No              |  Yes  |
UPD_TYPE_SID_NO_DATA     |             Yes             |  No   |
UPD_TYPE_NOT_APPLICABLE  |              -              |   -   |
----------------------------------------------------------------
</pre>

*/
typedef uint16 upd_mode_amr_t;

/**
@brief      AMR-NB UPD mode

AMR modes as defined in 3GPP 26.101 (Table 1a).
*/
typedef uint16 upd_mode_amrnb_t;

/**
@brief      AMR-WB UPD mode

AMR-WB modes as defined in 3GPP 26.201 (Table 1a).
*/
typedef uint16 upd_mode_amrwb_t;

/**
@brief      EVS UPD mode

AMR-WB modes as defined in 3GPP 26.201 (Table 1a).
*/
typedef uint16 upd_mode_evs_t;

/**
@brief      BFP UPD mode

Mode defines BFP packet duration (5 ms, 10 ms, 20 ms or 30 ms).
*/
typedef uint16 upd_mode_bfp_t;

/**
@brief      Data UPD mode

Mode defines non-transparent data bitrate.
*/
typedef uint16 upd_mode_data_t;

/**
@brief      ILBC UPD mode

Mode defines ILBC packet duration (20 ms or 30 ms).
*/
typedef uint16 upd_mode_ilbc_t;

/**
@brief      G.723 UPD mode

Mode defines G.723 packet size (160 bits or 192 bits).
*/
typedef uint16 upd_mode_g723_t;

/**
@brief      EVRC UPD mode

Mode defines EVRC packet size (16 bits to 171 bits).
*/
typedef uint16 upd_mode_evrc_t;

/**
@brief      UPD form

UPD form (#upd_form_pcm_t or #upd_form_data_t). Value UPD_FORM_NOT_APPLICABLE may be used
with any codec. Possible values are 0-15 (length is 4 bits).
*/
typedef uint16 upd_form_t;

/**
@brief      PCM UPD form

Form defines PCM packet payload encoding of speech frames. Note that G.722 uses sample
rate of 16 kHz with the audio signal but the sample rate of the encoded samples in UPD
payload is 8 kHz.

<pre>
upd_form_pcm_t              | sample | sample | number of samples | payload size octets|
                            |rate kHz|size bit|  5   10   20   30 |   5   10   20   30 |
----------------------------------------------------------------------------------------
UPD_FORM_PCM_ANY_8_BIT      |    8   |    8   | 40,  80, 160, 240 |  40,  80, 160, 240 |
UPD_FORM_PCM_G711_A_LAW     |    8   |    8   | 40,  80, 160, 240 |  40,  80, 160, 240 |
UPD_FORM_PCM_G711_U_LAW     |    8   |    8   | 40,  80, 160, 240 |  40,  80, 160, 240 |
UPD_FORM_PCM_UDI            |    8   |    8   | 40,  80, 160, 240 |  40,  80, 160, 240 |
UPD_FORM_PCM_LINEAR_08_KHZ  |    8   |   16   | 40,  80, 160, 240 |  80, 160, 320, 480 |
UPD_FORM_PCM_LINEAR_16_KHZ  |   16   |   16   | 80, 160, 320, 480 | 160, 320, 640, 960 |
UPD_FORM_PCM_G722           |    8   |    8   | 40,  80, 160, 240 |  40,  80, 160, 240 |
UPD_FORM_PCM_NOT_APPLICABLE |    -   |    -   |  -,   -,   -,   - |   -,   -,   -,   - |
----------------------------------------------------------------------------------------
</pre>

Not all types are used with all forms.

<pre>
upd_form_pcm_t              | NO_DATA | SPEECH_GOOD | SID_UPDATE | SPEECH_NO_DATA |
-----------------------------------------------------------------------------------
UPD_FORM_PCM_ANY_8_BIT      |   Yes   |     Yes     |    Yes     |      Yes       |
UPD_FORM_PCM_G711_A_LAW     |   Yes   |     Yes     |    Yes     |      Yes       |
UPD_FORM_PCM_G711_U_LAW     |   Yes   |     Yes     |    Yes     |      Yes       |
UPD_FORM_PCM_UDI            |   Yes   |     Yes     |    No      |      No        |
UPD_FORM_PCM_LINEAR_08_KHZ  |   No    |     Yes     |    No      |      No        |
UPD_FORM_PCM_LINEAR_16_KHZ  |   No    |     Yes     |    No      |      No        |
UPD_FORM_PCM_G722           |   Yes   |     Yes     |    No      |      Yes       |
UPD_FORM_PCM_NOT_APPLICABLE |   -     |      -      |     -      |       -        |
-----------------------------------------------------------------------------------
</pre>
*/
typedef uint16 upd_form_pcm_t;

/**
@brief      Data UPD form

Form defines the encoding of the payload and the meaning of type and mode.

<pre>
upd_form_data_t             |                type                 |         mode          |
-------------------------------------------------------------------------------------------
UPD_FORM_DATA_T_DATA        |UPD_TYPE_SPEECH_GOOD/UPD_TYPE_NO_DATA|UPD_MODE_NOT_APPLICABLE|
UPD_FORM_DATA_NT_DATA       |UPD_TYPE_SPEECH_GOOD/UPD_TYPE_NO_DATA|    upd_mode_data_t    |
UPD_FORM_DATA_T38           |  payload length in octets (#l_upd_get_t38_payload_size_8)   |
UPD_FORM_DATA_NOT_APPLICABLE|                 -                   |          -            |
-------------------------------------------------------------------------------------------
</pre>
*/
typedef uint16 upd_form_data_t;

/**
@brief      User Plane Data packet (UPD)

User data is transferred inside the Termination Framework in User Plane Data (UPD) packets.
The header is always 32 bits long but the payload is of variable length.
*/
typedef struct {
  BITFIELDS(
    bf16 codec : 4, /**< Codec (#upd_codec_t) */
    bf16 type  : 4, /**< Type (#upd_type_t) */
    bf16 mode  : 4, /**< Mode (#upd_mode_t) */
    bf16 form  : 4  /**< Form (#upd_form_t) */
  )
  uint16 padding; /**< 32-bit alignment padding (always 0) */
  uint8 payload_8[UPD_MAX_PAYLOAD_SIZE_8]; /**< UPD payload buffer */
} upd_packet_t;

/*****************************************************************************************/
/****    5 FUNCTION PROTOTYPES    ********************************************************/

MGW_GLOBAL void *
l_upd_get_payload(const upd_packet_t *upd_packet);

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

