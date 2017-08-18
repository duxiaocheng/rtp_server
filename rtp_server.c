// This is RTP server
// Test method: nc --udp 10.9.245.198 55555
// Author: DU Chason, Aug 18, 2017

#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

static char* str_toupper(char *str)
{
  char *orign = str;
  for (; *str != '\0'; str++) {
    *str = toupper(*str);
  }
  return orign;
}

static void rtp_server()
{
  printf("Welcome! This is a RTPserver.\n");
  printf("Now, I can only receive message from client and reply with CAPITAL same message.\n");

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(55555);
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
  char buff[512];
  struct sockaddr_in clientAddr;
  int n;
  int len = sizeof(clientAddr);
  while (1)
  {
    n = recvfrom(sock, buff, sizeof(buff) - 1, 0, (struct sockaddr *)&clientAddr,(socklen_t *) &len);
    if (n > 0)
    {
      buff[n] = 0;
      printf("[%s:%u] %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buff);
      n = sendto(sock, str_toupper(buff), n, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
      if (n < 0)
      {
        perror("sendto");
        break;
      }
    }
    else
    {
      perror("recv");
      break;
    }
  }
}

int main()
{
  printf("Hello, here is rtp_server!\n");
  printf("I am printf log to STDOUT.\n");
  syslog(LOG_INFO, "I am syslog.\n");

  rtp_server();

  return 0;
}


