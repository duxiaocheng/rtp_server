# RTP Server Dockerfile

# Pull base image
FROM centos:7

# Maintainer
MAINTAINER chason.du@nokia-sbell.com

# Copy file
ADD rtp_server /usr/bin/

# Expose container UDP port, and this is only a information.
# Use docker run -p or -P to make the ports of the container accessible to the host
EXPOSE 55555

# Entrypoint
CMD ["/usr/bin/rtp_server"]

