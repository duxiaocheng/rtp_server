all : rtp_server rtp_client

CFLAGS  = -Wall -g
LDFLAGS =	-lpthread

rtp_server: rtp_server.c
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $<

rtp_client: rtp_client.c
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -rf rtp_server.o rtp_client.o

