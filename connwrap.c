#include "connwrap.h"

#include <netdb.h>
#include <string.h>
#include <netinet/in.h>

#ifdef HAVE_OPENSSL

#include <openssl/ssl.h>
#include <openssl/err.h>

#elif HAVE_GNUTLS

#include <gnutls/openssl.h>
#define HAVE_OPENSSL

#endif

#ifdef HAVE_OPENSSL

static SSL_CTX *ctx = 0;

typedef struct { int fd; SSL *ssl; } sslsock;

static sslsock *socks = 0;
static int sockcount = 0;

static sslsock *getsock(int fd) {
    int i;

    for(i = 0; i < sockcount; i++)
	if(socks[i].fd == fd)
	    return &socks[i];

    return 0;
}

static sslsock *addsock(int fd) {
    sslsock *p;
    socks = (sslsock *) realloc(socks, sizeof(sslsock)*++sockcount);

    p = &socks[sockcount-1];

    if(!ctx) {
	SSL_library_init();
	SSL_load_error_strings();

#ifdef HAVE_SSLEAY
	SSLeay_add_all_algorithms();
#else
	OpenSSL_add_all_algorithms();
#endif

	ctx = SSL_CTX_new(SSLv23_client_method());
    }

    p->ssl = SSL_new(ctx);
    SSL_set_fd(p->ssl, p->fd = fd);

    return p;
}

static void delsock(int fd) {
    int i, nsockcount;
    sslsock *nsocks;

    nsockcount = 0;
    nsocks = (sslsock *) malloc(sizeof(sslsock)*(sockcount-1));

    for(i = 0; i < sockcount; i++) {
	if(socks[i].fd != fd) {
	    nsocks[nsockcount++] = socks[i];
	} else {
	    SSL_free(socks[i].ssl);
	}
    }

    free(socks);

    socks = nsocks;
    sockcount = nsockcount;
}

#endif

static char *bindaddr = 0;

int cw_connect(int sockfd, const struct sockaddr *serv_addr, int addrlen, int ssl) {
    int rc;
    struct sockaddr_in ba;

    if(bindaddr)
    if(strlen(bindaddr)) {
#ifdef HAVE_INET_ATON
	struct in_addr addr;
	rc = inet_aton(bindaddr, &addr);
	ba.sin_addr.s_addr = addr.s_addr;
#else
	rc = inet_pton(AF_INET, bindaddr, &ba);
#endif

	if(rc) {
	    ba.sin_port = 0;
	    rc = bind(sockfd, (struct sockaddr *) &ba, sizeof(ba));
	} else {
	    rc = -1;
	}

	if(rc) return rc;
    }

#ifdef HAVE_OPENSSL
    if(ssl) {
	rc = connect(sockfd, serv_addr, addrlen);

	if(!rc) {
	    sslsock *p = addsock(sockfd);
	    if(SSL_connect(p->ssl) != 1)
		return -1;
	}

	return rc;
    }
#endif
    return connect(sockfd, serv_addr, addrlen);
}

int cw_accept(int s, struct sockaddr *addr, int *addrlen, int ssl) {
#ifdef HAVE_OPENSSL
    int rc;

    if(ssl) {
	rc = accept(s, addr, addrlen);

	if(!rc) {
	    sslsock *p = addsock(s);
	    if(SSL_accept(p->ssl) != 1)
		return -1;

	}

	return rc;
    }
#endif
    return accept(s, addr, addrlen);
}

int cw_write(int fd, const void *buf, int count, int ssl) {
#ifdef HAVE_OPENSSL
    sslsock *p;

    if(ssl)
    if(p = getsock(fd))
	return SSL_write(p->ssl, buf, count);
#endif
    return write(fd, buf, count);
}

int cw_read(int fd, void *buf, int count, int ssl) {
#ifdef HAVE_OPENSSL
    sslsock *p;

    if(ssl)
    if(p = getsock(fd))
	return SSL_read(p->ssl, buf, count);
#endif
    return read(fd, buf, count);
}

int cw_close(int fd) {
#ifdef HAVE_OPENSSL
    delsock(fd);
#endif
    close(fd);
}

void cw_setbind(const char *abindaddr) {
    if(bindaddr) free(bindaddr);
    bindaddr = strdup(abindaddr);
}
