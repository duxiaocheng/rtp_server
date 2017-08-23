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

struct rtp_request{
  int size;
  struct sockaddr_in client;
  char buff[512];
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

  printf("work_thread started.\n");
  if (NULL == request) {
    printf("work_thread param error!\n");
    return NULL;
  }
  clientAddr = &(request->client);

  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    printf("work_thread socket could not be created.\n");
    goto QUIT;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = 0;

  if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0){
    printf("work_thread bind failed.\n");
    goto QUIT;
  }

  // point out the destination address: ip and port, for send/recv
  if (connect(sock, (struct sockaddr*)&(request->client), addr_len) < 0){
    printf("Can't connect to client.\n");
    goto QUIT;
  }
  while (request->size > 0) {
    printf("[%s:%u] %s\n", inet_ntoa(clientAddr->sin_addr), ntohs(clientAddr->sin_port), request->buff);
    int n = send(sock, str_toupper(request->buff), request->size, 0);
    if (n != request->size)
    {
      perror("sendto");
      break;
    }
    memset(request->buff, 0, sizeof(request->buff));
    request->size = recv(sock, request->buff, sizeof(request->buff) - 1, 0);
  }

QUIT:
  if (request) free(request);
  if (sock >= 0) close(sock);
  printf("exit work_thread %lu\n", pthread_self());
  return NULL;
}

static void rtp_server()
{
  printf("Welcome! This is a RTPserver.\n");
  printf("Now, I can only receive message from client and reply with CAPITAL same message.\n");

  struct sockaddr_in addr;
  int port = 55555;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int sock;
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket");
    exit(1);
  }
  if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    exit(1);
  }
  printf("Server started at 0.0.0.0:%d.\n", port);

  socklen_t addr_len = sizeof(struct sockaddr_in);
  struct rtp_request *request = NULL;
  while (1)
  {
    request = (struct rtp_request *)malloc(sizeof(struct rtp_request));
    memset(request, 0, sizeof(struct rtp_request));
    request->size = recvfrom(sock, request->buff, sizeof(request->buff) - 1, 0,
        (struct sockaddr *)&(request->client), &addr_len);
    pthread_t t_id;
    if (pthread_create(&t_id, NULL, work_thread, request) != 0) {
      printf("create thread failed:%s\n", strerror(errno));
    }
    printf("create thread %lu success\n", t_id);
    // TODO: when and how to exit work thread.
  }

}

int main()
{
  rtp_server();

  return 0;
}


