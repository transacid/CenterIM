#ifndef __CONNWRAP_H__
#define __CONNWRAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/socket.h>

int cw_connect(int sockfd, const struct sockaddr *serv_addr,
    int addrlen, int ssl);

int cw_accept(int s, struct sockaddr *addr, int *addrlen, int ssl);

int cw_write(int fd, const void *buf, int count, int ssl);
int cw_read(int fd, void *buf, int count, int ssl);

#ifdef __cplusplus
}
#endif

#endif
