#include <stdio.h>
#include <syslog.h>

int main()
{
  printf("Hello, here is rtp_server!\n");
  printf("I am printf log to STDOUT.\n");

  syslog(LOG_INFO, "I am syslog.\n");

  return 0;
}

