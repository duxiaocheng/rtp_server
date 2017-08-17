all : rtp_server

rtp_server: rtp_server.c
	gcc -g -o $@ $<

clean:
	rm -rf rtp_server.o

