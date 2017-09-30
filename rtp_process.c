/*****************************************************************************************/
/**
@brief RTP functions
*/

/*****************************************************************************************/
/****    1 INCLUDE FILES    **************************************************************/
#include <type_def.h>
#include <log_print.h>
#include "l_rtp.h"
#include "etcd-api/etcd-api.h"

#include <string.h>
#include <stdlib.h>

/*****************************************************************************************/
/****    2 LOCAL CONSTANTS    ************************************************************/

/*****************************************************************************************/
/****    3 LOCAL MACROS    ***************************************************************/

/*****************************************************************************************/
/****    4 LOCAL DATA TYPES    ***********************************************************/

/*****************************************************************************************/
/****    5 LOCAL FUNCTION PROTOTYPES    **************************************************/

/*****************************************************************************************/
/****    6 FUNCTIONS    ******************************************************************/

int rtp_process(void *buff, int len, void *buf_out, int *len_out_8)
{
#if 1
  int size8 = 0;
  unsigned int ssrc = 0;
	void *payload = NULL;
	int payload_len = 0;

  size8 = l_rtp_get_header_size_8(L_RTP_TYPE_RTP_UNCOMPRESSED);
  if (len < size8) {
    errorf("rtp_process len error:%d\n", len);
    return -1;
  }

  static uint32 s_sequence_number = 1;
  static uint32 s_timestamp = 10000;
  debugf("--- %03d ---------------------------\n", s_sequence_number);
  l_rtp_set_field(buff, L_RTP_TYPE_RTP_UNCOMPRESSED, L_RTP_FIELD_ID_SEQ_NUMBER, s_sequence_number++);
  l_rtp_set_field(buff, L_RTP_TYPE_RTP_UNCOMPRESSED, L_RTP_FIELD_ID_TIMESTAMP, s_timestamp);
  s_timestamp += 20;

  ssrc = l_rtp_get_field(buff, L_RTP_TYPE_RTP_UNCOMPRESSED, L_RTP_FIELD_ID_SSRC);
  debugf("ssrc: 0x%x (%u)\n", ssrc, ssrc);
  unsigned int seq_number = l_rtp_get_field(buff, L_RTP_TYPE_RTP_UNCOMPRESSED, L_RTP_FIELD_ID_SEQ_NUMBER);
  debugf("sequence number: %u\n", seq_number);
  unsigned int timestamp = l_rtp_get_field(buff, L_RTP_TYPE_RTP_UNCOMPRESSED, L_RTP_FIELD_ID_TIMESTAMP);
  debugf("timestamp: %u\n", timestamp);
  unsigned int payload_type = l_rtp_get_field(buff, L_RTP_TYPE_RTP_UNCOMPRESSED, L_RTP_FIELD_ID_PAYLOAD_TYPE);
  debugf("payload type: %u\n", payload_type);

  payload_len = len - size8;
  payload = (char *)buff + size8;

  void *payload_out = (char *)buf_out + size8;

  // do transcoding
  extern int evs_ingress_process(void *buff, int len, void *buff_out, int *len_out_8);
  evs_ingress_process(payload, payload_len, payload_out, len_out_8);

  // create PCM RTP packet
  memcpy(buf_out, buff, size8);
  len_out_8 += size8;
  int payload_type_pcm = 8;
  l_rtp_increment_field(buf_out, L_RTP_TYPE_RTP_UNCOMPRESSED, L_RTP_FIELD_ID_SSRC, 1);
  l_rtp_set_field(buf_out, L_RTP_TYPE_RTP_UNCOMPRESSED, L_RTP_FIELD_ID_PAYLOAD_TYPE, payload_type_pcm);

#else
  etcd_session sess;
  char *servers = "10.9.245.200:2379";
  char *key = "k8s/network/config";
  sess = etcd_open_str(servers);
  if (NULL == sess) {
    errorf("etcd_open_str(%s) failed!\n", servers);
    return -1;
  }
  infof("etcd_open_str(%s) success!\n", servers);
  char *value = etcd_get(sess, key);
  if (NULL == value) {
    errorf("etcd_get(%s) failed!\n", key);
  }
  debugf("%s: %s\n", key, value);
#endif
  return 0;
}

/*****************************************************************************************/



