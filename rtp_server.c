// This is RTP server
// Author: DU Chason, Aug 18, 2017

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include <log_print.h>
#include "rtp_process.h"


#define RTP_SERVICE_VERSION "1.0"

struct rtp_request{
  struct sockaddr_in client;
  char buff[1500];
  int size;
  char buf_out[1500];
  int size_out_8;
};

static char* str_toupper(char *str)
{
  char *orign = str;
  for (; *str != '\0'; str++) {
    *str = toupper(*str);
  }
  return orign;
}

void *work_thread(void *arg) {
  int sock;
  struct rtp_request *request = (struct rtp_request *)arg;
  struct sockaddr_in server;
  socklen_t addr_len = sizeof(struct sockaddr_in);
  struct sockaddr_in *clientAddr = NULL;

  infof("work_thread started.\n");
  if (NULL == request) {
    errorf("work_thread param error!\n");
    return NULL;
  }
  clientAddr = &(request->client);

  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    errorf("work_thread socket could not be created.\n");
    goto QUIT;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = 0;

  if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0){
    errorf("work_thread bind failed.\n");
    goto QUIT;
  }

  // point out the destination address: ip and port, for send/recv
  if (connect(sock, (struct sockaddr*)&(request->client), addr_len) < 0){
    errorf("Can't connect to client.\n");
    goto QUIT;
  }
  while (request->size > 0) {
    // process request
    infof("[%s:%u] %s\n", inet_ntoa(clientAddr->sin_addr), ntohs(clientAddr->sin_port), request->buff);
    rtp_process(request->buff, request->size, request->buf_out, &(request->size_out_8));

    // send response
    int n = send(sock, str_toupper(request->buff), request->size, 0);
    if (n != request->size)
    {
      perror("sendto");
      break;
    }
    // receive new request
    memset(request->buff, 0, sizeof(request->buff));
    request->size = recv(sock, request->buff, sizeof(request->buff) - 1, 0);
  }

QUIT:
  if (request) free(request);
  if (sock >= 0) close(sock);
  infof("exit work_thread %lu\n", pthread_self());
  return NULL;
}

static void rtp_server()
{
  infof("Welcome! This is a RTPserver %s\n", RTP_SERVICE_VERSION);

  struct sockaddr_in addr;
  int port = 55555;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int sock;
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    errorf("create socker failed: %s\n", strerror(errno));
    exit(1);
  }
  if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
  {
    errorf("bind socker failed: %s\n", strerror(errno));
    exit(1);
  }
  infof("Server started at 0.0.0.0:%d.\n", port);

  socklen_t addr_len = sizeof(struct sockaddr_in);
  struct rtp_request *request = NULL;
  while (1)
  {
    request = (struct rtp_request *)malloc(sizeof(struct rtp_request));
    memset(request, 0, sizeof(struct rtp_request));
    request->size = recvfrom(sock, request->buff, sizeof(request->buff), 0,
        (struct sockaddr *)&(request->client), &addr_len);
#if 0
    pthread_t t_id;
    if (pthread_create(&t_id, NULL, work_thread, request) != 0) {
      errorf("create thread failed:%s\n", strerror(errno));
    }
    infof("create thread %lu success\n", t_id);
    // TODO: when and how to exit work thread.
#else
    rtp_process(request->buff, request->size, request->buf_out, &(request->size_out_8));
    // send response
    int n = sendto(sock, request->buf_out, request->size_out_8, 0, (struct sockaddr*)&(request->client), addr_len);
    if (n != request->size_out_8) {
      errorf("send response error: %s\n", strerror(errno));
      break;
    }
#endif
  }

}

int main()
{
  rtp_server();
  return 0;
}

// Refer to:
// UDP multi threads model: https://github.com/ideawu/tftpx

