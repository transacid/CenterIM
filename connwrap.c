#include "connwrap.h"

int cw_connect(int sockfd, const struct sockaddr *serv_addr, int addrlen, int ssl) {
    return connect(sockfd, serv_addr, addrlen);
}

int cw_accept(int s, struct sockaddr *addr, int *addrlen, int ssl) {
    return accept(s, addr, addrlen);
}

int cw_write(int fd, const void *buf, int count, int ssl) {
    return write(fd, buf, count);
}

int cw_read(int fd, void *buf, int count, int ssl) {
    return read(fd, buf, count);
}
