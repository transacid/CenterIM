#include "connwrap.h"

#include <config.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#define PROXY_TIMEOUT   10
    // HTTP proxy timeout in seconds (for the CONNECT method)

#ifdef HAVE_OPENSSL

#define OPENSSL_NO_KRB5 1
#include <openssl/ssl.h>
#include <openssl/err.h>

#elif HAVE_GNUTLS

#include <gnutls/gnutls.h>

#elif HAVE_NSS_COMPAT

#include <nss_compat_ossl/nss_compat_ossl.h>

#endif

static int in_http_connect = 0;

#if defined(HAVE_OPENSSL) || defined(HAVE_NSS_COMPAT)

static SSL_CTX *ctx = NULL;
typedef struct { int fd; SSL *ssl; } sslsock;

#elif HAVE_GNUTLS

typedef struct { int fd; gnutls_session_t session; } sslsock;

#endif

#ifdef HAVE_SSL

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
    
#ifdef HAVE_GNUTLS
    gnutls_certificate_credentials_t xcred;
#endif

    if (socks)
    socks = (sslsock *) realloc(socks, sizeof(sslsock)*++sockcount);
    else
    socks = (sslsock *) malloc(sizeof(sslsock)*++sockcount);

    p = &socks[sockcount-1];

#if defined(HAVE_OPENSSL) || defined(HAVE_NSS_COMPAT)
    if(!ctx) {
	SSL_library_init();

    SSL_load_error_strings();

#ifdef HAVE_SSLEAY
    SSLeay_add_all_algorithms();
#elif HAVE_OPENSSL
    OpenSSL_add_all_algorithms();
#endif
    ctx = SSL_CTX_new(SSLv23_client_method());
    }
    p->ssl = SSL_new(ctx);
    SSL_set_fd(p->ssl, p->fd = fd);
#elif defined(HAVE_GNUTLS)
    gnutls_global_init ();
    gnutls_certificate_allocate_credentials (&xcred);
    gnutls_init (&(p->session), GNUTLS_CLIENT);
    gnutls_set_default_priority (p->session);
    gnutls_credentials_set (p->session, GNUTLS_CRD_CERTIFICATE, xcred);
    p->fd = fd;
    gnutls_transport_set_ptr(p->session,(gnutls_transport_ptr_t)fd);
#endif

    return p;
}

static void delsock(int fd) {
    int i, nsockcount;
    sslsock *nsocks;
    
    if (sockcount > 0)
    {
    nsockcount = 0;

    for(i = 0; i < sockcount; i++) {
	if(socks[i].fd != fd) {
	    socks[nsockcount++] = socks[i];
	} 
#if defined(HAVE_OPENSSL) || defined(HAVE_NSS_COMPAT)
	else {
	    SSL_free(socks[i].ssl);
	}
#elif HAVE_GNUTLS
	else {
            gnutls_bye( socks[i].session, GNUTLS_SHUT_WR);
            gnutls_deinit(socks[i].session);
	}
#endif
    }

    socks = realloc(socks, sizeof(sslsock)*(nsockcount));
    sockcount = nsockcount;
    }
}

#endif

static char *bindaddr = 0, *proxyhost = 0, *proxyuser = 0, *proxypass = 0;
static int proxyport = 3128;
static int proxy_ssl = 0;

#define SOCKOUT(s) write(sockfd, s, strlen(s))

int cw_http_connect(int sockfd, const struct sockaddr *serv_addr, int addrlen) {
    int err, pos, fl;
    struct hostent *server;
    struct sockaddr_in paddr;
    char buf[512];
    fd_set rfds;

    err = 0;
    in_http_connect = 1;

    if(!(server = gethostbyname(proxyhost))) {
	errno = h_errno;
	err = -1;
    }

    if(!err) {
	memset(&paddr, 0, sizeof(paddr));
	paddr.sin_family = AF_INET;
	memcpy(&paddr.sin_addr.s_addr, *server->h_addr_list, server->h_length);
	paddr.sin_port = htons(proxyport);

	fl = fcntl(sockfd, F_GETFL);
	fcntl(sockfd, F_SETFL, fl & ~O_NONBLOCK);

	buf[0] = 0;

	err = cw_connect(sockfd, (struct sockaddr *) &paddr, sizeof(paddr), proxy_ssl);
    }

    errno = ECONNREFUSED;

    if(!err) {
	struct sockaddr_in *sin = (struct sockaddr_in *) serv_addr;
	char *ip = inet_ntoa(sin->sin_addr), c;
	struct timeval tv;

	sprintf(buf, "%d", ntohs(sin->sin_port));
	SOCKOUT("CONNECT ");
	SOCKOUT(ip);
	SOCKOUT(":");
	SOCKOUT(buf);
	SOCKOUT(" HTTP/1.0\r\n");

	if(proxyuser) {
	    char *b;
	    SOCKOUT("Proxy-Authorization: Basic ");

	    sprintf(buf, "%s:%s", proxyuser, proxypass);
	    b = cw_base64_encode(buf);
	    SOCKOUT(b);
	    free(b);

	    SOCKOUT("\r\n");
	}

	SOCKOUT("\r\n");

	buf[0] = 0;

	while(err != -1) {
	    FD_ZERO(&rfds);
	    FD_SET(sockfd, &rfds);

	    tv.tv_sec = PROXY_TIMEOUT;
	    tv.tv_usec = 0;

	    err = select(sockfd+1, &rfds, 0, 0, &tv);

	    if(err < 1) err = -1;

	    if(err != -1 && FD_ISSET(sockfd, &rfds)) {
		err = read(sockfd, &c, 1);
		if(!err) err = -1;

		if(err != -1) {
		    pos = strlen(buf);
		    buf[pos] = c;
		    buf[pos+1] = 0;

		    if(strlen(buf) > 4)
		    if(!strcmp(buf+strlen(buf)-4, "\r\n\r\n"))
			break;
		}
	    }
	}
    }

    if(err != -1 && strlen(buf)) {
	char *p = strstr(buf, " ");

	err = -1;

	if(p)
	if(atoi(++p) == 200)
	    err = 0;

	fcntl(sockfd, F_SETFL, fl);
	if(fl & O_NONBLOCK) {
	    errno = EINPROGRESS;
	    err = -1;
	}
    }

    in_http_connect = 0;

    return err;
}

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
	    if ((rc==-1) && (errno == EINVAL))
		rc=0;
	} else {
	    rc = -1;
	}

	if(rc) return rc;
    }

    if(proxyhost && !in_http_connect) {
        rc = cw_http_connect(sockfd, serv_addr, addrlen);
     }
     else{
     	rc = connect(sockfd, serv_addr, addrlen);
     }

#ifdef HAVE_SSL
    if(ssl && !rc) {
	sslsock *p = addsock(sockfd);
#if defined(HAVE_OPENSSL) || defined(HAVE_NSS_COMPAT)
	if(SSL_connect(p->ssl) != 1){
	    //fprintf(stderr, "cw_connect(%d) - cannot connect to SSL\n", sockfd);
	    return -1;
	}
#elif defined(HAVE_GNUTLS)
	int ret;
	do {
	    ret = gnutls_handshake(p->session);
	} while ((ret == GNUTLS_E_AGAIN) || (ret == GNUTLS_E_INTERRUPTED));
	if (ret < 0) {
	      gnutls_perror (ret);
	      return -1;
        }
#endif
    }
#endif
    return rc;
}

int cw_nb_connect(int sockfd, const struct sockaddr *serv_addr, int addrlen, int ssl, int *state) {
    int rc = 0;
    int ret;
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
	    if ((rc==-1) && (errno == EINVAL))
		rc=0;
	} else {
	    rc = -1;
	}

	if(rc) return rc;
    }

#ifdef HAVE_SSL
    if(ssl) {
	if ( !(*state & CW_CONNECT_WANT_SOMETHING)){
	    rc = cw_connect(sockfd, serv_addr, addrlen, 0);
	}
	else{ /* check if the socket is connected correctly */
	    int optlen = sizeof(int), optval;
	    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) || optval){
	        /* Look for a better solution to print errors */
	        //fprintf(stderr,"cw_nb_connect(%d): getsockopt error!!\n", sockfd);
	        return -1;
	    }
	}

	if(!rc) {
	    sslsock *p;
	    if (*state & CW_CONNECT_SSL)
		p = getsock(sockfd);
	    else
		p = addsock(sockfd);
	    
#ifdef HAVE_GNUTLS
           ret = gnutls_handshake(p->session);
	    switch (ret) {
	    case GNUTLS_E_SUCCESS:
		*state = 0;
		return 0;
	    case GNUTLS_E_INTERRUPTED:
	    case GNUTLS_E_AGAIN:
		if (gnutls_record_get_direction(p->session)) {
		    *state = CW_CONNECT_SSL | CW_CONNECT_WANT_READ;
		} else {
		    *state = CW_CONNECT_SSL | CW_CONNECT_WANT_WRITE;
		}
		return 0;
	    default:
/*	      gnutls_deinit(p->session);
		will be dealt with in delsock()
*/
	      gnutls_perror (ret);
	      return -1;
	    }
#elif defined(HAVE_OPENSSL) || defined(HAVE_NSS_COMPAT)
	    rc = SSL_connect(p->ssl);
	    switch(rc){
	    case 1:
		*state = 0;
		return 0;
	    case 0:
		return -1;
	    default:
		switch (SSL_get_error(p->ssl, rc)){
		case SSL_ERROR_WANT_READ:
		    *state = CW_CONNECT_SSL | CW_CONNECT_WANT_READ;
		    return 0;
		case SSL_ERROR_WANT_WRITE:
		    *state = CW_CONNECT_SSL | CW_CONNECT_WANT_WRITE;
		    return 0;
		default:
		    return -1;
		}
	    }
#endif
	} else { // catch EINPROGRESS error from the connect call 
	    if (errno == EINPROGRESS){
		*state = CW_CONNECT_STARTED | CW_CONNECT_WANT_WRITE;
		return 0;
	    }
	}
	return rc;
    }
#endif

    if ( !(*state & CW_CONNECT_WANT_SOMETHING)){
	rc = connect(sockfd, serv_addr, addrlen);
    }
    else{ /* check if the socket is connected correctly */
	int optlen = sizeof(int), optval;
	if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) || optval)	{
            //fprintf(stderr,"getsockopt error!!");
	    return -1;
        }
	*state = 0;
	return 0;
    }
    if (rc)
	if (errno == EINPROGRESS){
	    *state = CW_CONNECT_STARTED | CW_CONNECT_WANT_WRITE;
	    return 0;
	}
    return rc;
}

int cw_accept(int s, struct sockaddr *addr, int *addrlen, int ssl) {
#if defined(HAVE_OPENSSL) || defined(HAVE_NSS_COMPAT)
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
#ifdef HAVE_SSL
    sslsock *p;
    int ret;

    if(ssl)
#endif
#ifdef HAVE_GNUTLS
     if(p = getsock(fd)){
           ret = gnutls_record_send( p->session, buf, count);
           return ret;
     }
#elif defined(HAVE_OPENSSL) || defined(HAVE_NSS_COMPAT)
     if(p = getsock(fd))
	return SSL_write(p->ssl, buf, count);
#endif
    return write(fd, buf, count);
}

int cw_read(int fd, void *buf, int count, int ssl) {
#ifdef HAVE_SSL
    sslsock *p;
    int ret;

    if(ssl)
#endif
#ifdef HAVE_GNUTLS
     if(p = getsock(fd)){
       do {
	           ret = gnutls_record_recv(p->session, buf, count);
       } while ( ret < 0 && (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN) );
       return ret;
       }
#elif defined(HAVE_OPENSSL) || defined(HAVE_NSS_COMPAT)
     if(p = getsock(fd))
	return SSL_read(p->ssl, buf, count);
#endif
    return read(fd, buf, count);
}

int cw_close(int fd) {
#ifdef HAVE_SSL
    delsock(fd);
#endif
    return close(fd);
}

#define FREEVAR(v) if(v) free(v), v = 0;

void cw_setbind(const char *abindaddr) {
    FREEVAR(bindaddr);
    bindaddr = strdup(abindaddr);
}

void cw_setproxy(const char *aproxyhost, int aproxyport, const char *aproxyuser, const char *aproxypass) {
    FREEVAR(proxyhost);
    FREEVAR(proxyuser);
    FREEVAR(proxypass);

    if(aproxyhost && strlen(aproxyhost)) proxyhost = strdup(aproxyhost);
    if(aproxyuser && strlen(aproxyuser)) proxyuser = strdup(aproxyuser);
    if(aproxypass && strlen(aproxypass)) proxypass = strdup(aproxypass);
    proxyport = aproxyport;
}

char *cw_base64_encode(const char *in) {
    static char base64digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._";

    int j = 0;
    int inlen = strlen(in);
    char *out = (char *) malloc(inlen*4+1), c;

    for(out[0] = 0; inlen >= 3; inlen -= 3) {
	strncat(out, &base64digits[ in[j] >> 2 ], 1);
	strncat(out, &base64digits[ ((in[j] << 4) & 0x30) | (in[j+1] >> 4) ], 1);
	strncat(out, &base64digits[ ((in[j+1] << 2) & 0x3c) | (in[j+2] >> 6) ], 1);
	strncat(out, &base64digits[ in[j+2] & 0x3f ], 1);
	j += 3;
    }

    if(inlen > 0) {
	unsigned char fragment;

	strncat(out, &base64digits[in[j] >> 2], 1);
	fragment = (in[j] << 4) & 0x30;

	if(inlen > 1)
	    fragment |= in[j+1] >> 4;

	strncat(out, &base64digits[fragment], 1);

	c = (inlen < 2) ? '-' : base64digits[ (in[j+1] << 2) & 0x3c ];
	strncat(out, &c, 1);
	c = '-';
	strncat(out, &c, 1);
    }
    
    return out;
}
