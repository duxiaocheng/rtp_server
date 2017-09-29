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
	int frame_number_max = 1;
	int frame_number = 0;
  struct sockaddr_in server;
  int sock = -1;
	unsigned int i = 0;
  socklen_t addr_len = sizeof(struct sockaddr_in);

  if (argc > 1) {
    server_ip = argv[1];
  }
	if (argc > 2) {
		frame_number_max = atoi(argv[2]);
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

  FILE *fp = fopen("evs.pcap.rtp", "r");

  while (1) {
    char data_str[1500] = { 0 };
    char data_buf[1500] = { 0 };
		unsigned int data_len = 0;
    if (NULL == fgets(data_str, sizeof(data_str), fp)) {
      // seek to begin, and then read continue
      fseek(fp, 0, SEEK_SET);
      fgets(data_str, sizeof(data_str), fp);
    }
		if (data_str[strlen(data_str)-1] == '\n') {
			data_str[strlen(data_str)-1] = '\0';
		}
    data_len = strlen(data_str) >> 1;
    printf("%s:%lu (%u bytes)\n", data_str, strlen(data_str), data_len);
    for (i = 0; i < data_len; ++i) {
        int t = 0;
        sscanf(data_str + (i * 2), "%02x", &t);
        data_buf[i] = (char)t;
    }

    int n = sendto(sock, data_buf, data_len, 0, (struct sockaddr*)&server, addr_len);
    if (n < 0) {
      perror("sendto");
    }
    
    char response[1500] = {0};
    // in the next sendto, use the response server port
    n = recvfrom(sock, response, sizeof(response), 0, (struct sockaddr*)&server, &addr_len);
    if (n < 0) {
      perror("recvfrom");
    }
    usleep(20 * 1000);
    frame_number ++;
		if (frame_number >= frame_number_max) {
			break;
		}
  }

  close(sock);
  fclose(fp);

  return 0;
}

