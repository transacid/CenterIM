#include "connwrap.h"

#ifdef HAVE_OPENSSL

SSL_CTX *ctx = 0;

#endif


int cw_connect(int sockfd, const struct sockaddr *serv_addr, int addrlen, int ssl) {
#ifdef HAVE_OPENSSL
    int rc;

    if(!ctx) {
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ctx = SSL_CTX_new(SSLv2_client_method());
    }

    rc = connect(sockfd, serv_addr, addrlen);

    if(!rc) {
	ssl = SSL_new(ctx);

	if(SSL_connect(ssl) == FAIL)
	    return -1;

    } else {
	return rc;
    }

#else
    return connect(sockfd, serv_addr, addrlen);
#endif
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
