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

int main(int argc, char **argv)
{
  const char *server_ip = "127.0.0.1";
  unsigned short port = 55555;
  struct sockaddr_in server;
  int sock;
  socklen_t addr_len = sizeof(struct sockaddr_in);

  if (argc > 1) {
    server_ip = argv[1];
  }
  printf("Connect to server at %s:%d\n", server_ip, port);

  if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
    printf("Server socket could not be created.\n");
    return 0;
  }

  // Initialize server address
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  inet_pton(AF_INET, server_ip, &(server.sin_addr.s_addr));

  while(1){
    printf(">> ");
    #define LINE_BUF_SIZE 512
    char cmd_line[LINE_BUF_SIZE] = {0};
    const char *buf = fgets(cmd_line, LINE_BUF_SIZE, stdin);
    if (buf == NULL || strncmp(buf, "quit", 4) == 0) {
      printf("\nBye.\n");
      break;
    }
    // only '\n'
    if (strlen(buf) < 2) {
      continue;
    }

    int n = sendto(sock, buf, strlen(buf), 0, (struct sockaddr*)&server, addr_len);
    if (n < 0) {
      perror("sendto");
    }
    char response[512] = {0};
    // in the next sendto, use the response server port
    n = recvfrom(sock, response, sizeof(response) - 1, 0, (struct sockaddr*)&server, &addr_len);
    if (n < 0) {
      perror("recvfrom");
    }
    printf("%s", response);
  }

  close(sock);

  return 0;
}

