/*
firedns.c - firedns library
Copyright (C) 2002 Ian Gulliver

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include "firedns.h"

struct hostent {
	char    *h_name;
	char    **h_aliases;
	int     h_addrtype;
	int     h_length;
	char    **h_addr_list;
};
#define h_addr  h_addr_list[0]


static struct in_addr servers4[8];
static int i4;
#ifdef FIREDNS_USE_IPV6
static int i6;
static struct in6_addr servers6[8];
#endif

static int initdone = 0;
static int lastclose = -1;

struct s_connection {
	struct s_connection *next;
	unsigned char id[2];
	unsigned short class;
	unsigned short type;
	int fd;
#ifdef FIREDNS_USE_IPV6
	int v6;
#endif
};

struct s_rr_middle {
	unsigned short type;
	unsigned short class;
	unsigned long ttl;
	unsigned short rdlength;
};

#define FIREDNS_POINTER_VALUE 0xc000

static struct s_connection *connection_head = NULL;

struct s_header {
	unsigned char id[2];
	unsigned char flags1;
#define FLAGS1_MASK_QR 0x80
#define FLAGS1_MASK_OPCODE 0x78 /* bitshift right 3 */
#define FLAGS1_MASK_AA 0x04
#define FLAGS1_MASK_TC 0x02
#define FLAGS1_MASK_RD 0x01
	unsigned char flags2;
#define FLAGS2_MASK_RA 0x80
#define FLAGS2_MASK_Z  0x70
#define FLAGS2_MASK_RCODE 0x0f
	unsigned short qdcount;
	unsigned short ancount;
	unsigned short nscount;
	unsigned short arcount;
	unsigned char payload[512];
};

static void firedns_close(int fd) {
	if (lastclose != -1) {
		close(lastclose);
		lastclose = -1;
	}
	lastclose = fd;
}

static void init() {
	FILE *f;
	int i;
	struct in_addr *addr4;
	char buf[1024];
#ifdef FIREDNS_USE_IPV6
	struct in6_addr *addr6;
#endif
	if (initdone == 1)
		return;
#ifdef FIREDNS_USE_IPV6
	i6 = 0;
#endif
	i4 = 0;

	initdone = 1;
	srand((unsigned int) time(NULL));
	bzero(servers4,sizeof(struct in_addr) * 8);
#ifdef FIREDNS_USE_IPV6
	bzero(servers6,sizeof(struct in6_addr) * 8);
#endif
	/* read /etc/firedns.conf if we've got it, otherwise parse /etc/resolv.conf */
	f = fopen("/etc/firedns.conf","r");
	if (f == NULL) {
		f = fopen("/etc/resolv.conf","r");
		if (f == NULL)
			return;
		while (fgets(buf,1024,f) != NULL) {
			if (strncmp(buf,"nameserver",10) == 0) {
				i = 10;
				while (buf[i] == ' ' || buf[i] == '\t')
					i++;
#ifdef FIREDNS_USE_IPV6
				/* glibc /etc/resolv.conf seems to allow ipv6 server names */
				if (i6 < 8) {
					addr6 = firedns_aton6(&buf[i]);
					if (addr6 != NULL) {
						memcpy(&servers6[i6++],addr6,sizeof(struct in6_addr));
						continue;
					}
				}
#endif
				if (i4 < 8) {
					addr4 = firedns_aton4(&buf[i]);
					if (addr4 != NULL)
						memcpy(&servers4[i4++],addr4,sizeof(struct in_addr));
				}
			}
		}
	} else {
		while (fgets(buf,1024,f) != NULL) {
#ifdef FIREDNS_USE_IPV6
			if (i6 < 8) {
				addr6 = firedns_aton6(buf);
				if (addr6 != NULL) {
					memcpy(&servers6[i6++],addr6,sizeof(struct in6_addr));
					continue;
				}
			}
#endif
			if (i4 < 8) {
				addr4 = firedns_aton4(buf);
				if (addr4 != NULL)
					memcpy(&servers4[i4++],addr4,sizeof(struct in_addr));
			}
		}
	}
	fclose(f);

}

static int firedns_send_requests(struct s_header *h, struct s_connection *s, int l) {
	int i;
	struct sockaddr_in addr4;
#ifdef FIREDNS_USE_IPV6
	struct sockaddr_in6 addr6;
	/* if we've got ipv6 support, an ip v6 socket, and ipv6 servers, send to them */
	if (i6 > 0 && s->v6 == 1) {
		for (i = 0; i < i6; i++) {
			memcpy(&addr6.sin6_addr,&servers6[i],sizeof(struct in6_addr));
			addr6.sin6_family = AF_INET6;
			addr6.sin6_port = htons(53);
			sendto(s->fd, h, l + 12, 0, (struct sockaddr *) &addr6, sizeof(addr6));
		}
	}
#endif

	for (i = 0; i < i4; i++) {
#ifdef FIREDNS_USE_IPV6
		/* send via ipv4-over-ipv6 if we've got an ipv6 socket */
		if (s->v6 == 1) {
			memcpy(addr6.sin6_addr.s6_addr,"\0\0\0\0\0\0\0\0\0\0\xff\xff",12);
			memcpy(&addr6.sin6_addr.s6_addr[12],&servers4[i].s_addr,4);
			addr6.sin6_family = AF_INET6;
			addr6.sin6_port = htons(53);
			sendto(s->fd, h, l + 12, 0, (struct sockaddr *) &addr6, sizeof(addr6));
			continue;
		}
#endif
		/* otherwise send via standard ipv4 boringness */
		memcpy(&addr4.sin_addr,&servers4[i],sizeof(struct in_addr));
		addr4.sin_family = AF_INET;
		addr4.sin_port = htons(53);
		sendto(s->fd, h, l + 12, 0, (struct sockaddr *) &addr4, sizeof(addr4));
	}

	return 0;
}

static struct s_connection *firedns_add_query(struct s_header *h) {
	struct s_connection *s;

	/* set header flags */
	h->id[0] = rand() % 255;
	h->id[1] = rand() % 255;
	h->flags1 = 0 | FLAGS1_MASK_RD;
	h->flags2 = 0;
	h->qdcount = htons(1);
	h->ancount = htons(0);
	h->nscount = htons(0);
	h->arcount = htons(0);

	/* create new connection object, add to linked list */
	s = malloc(sizeof(struct s_connection));
	if (s == NULL) {
		perror("malloc");
		abort();
		exit(1);
	}
	s->next = connection_head;
	connection_head = s;

	/* needed for security later */
	memcpy(s->id,h->id,2);

	/* try to create ipv6 or ipv4 socket */
#ifdef FIREDNS_USE_IPV6
	s->v6 = 0;
	s->fd = socket(PF_INET6, SOCK_DGRAM, 0);
	if (s->fd != -1) {
		if (fcntl(s->fd, F_SETFL, O_NONBLOCK) != 0) {
			firedns_close(s->fd);
			s->fd = -1;
		}
	}
	if (s->fd != -1) {
		struct sockaddr_in6 addr6;
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = 0;
		bzero(addr6.sin6_addr.s6_addr,16);
		if (bind(s->fd,(struct sockaddr *)&addr6,sizeof(struct sockaddr_in6)) == 0)
			s->v6 = 1;
		else
			firedns_close(s->fd);
	}
	if (s->v6 == 0) {
#endif
		s->fd = socket(PF_INET, SOCK_DGRAM, 0);
		if (s->fd != -1) {
			if (fcntl(s->fd, F_SETFL, O_NONBLOCK) != 0) {
				firedns_close(s->fd);
				s->fd = -1;
			}
		}
		if (s->fd != -1) {
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = 0;
			addr.sin_addr.s_addr = INADDR_ANY;
			if (bind(s->fd,(struct sockaddr *)&addr,sizeof(struct sockaddr_in)) != 0) {
				firedns_close(s->fd);
				s->fd = -1;
			}
		}
		if (s->fd == -1) {
			connection_head = s->next;
			free(s);
			return NULL;
		}
#ifdef FIREDNS_USE_IPV6
	}
#endif
	firedns_close(-1);
	return s;
}

static int firedns_build_query_payload(const char * const name, unsigned short rr, unsigned short class, unsigned char * payload) {
	short payloadpos;
	const char * tempchr, * tempchr2;
	unsigned short l;
	
	payloadpos = 0;
	tempchr2 = name;

	/* split name up into labels, create query */
	while ((tempchr = strchr(tempchr2,'.')) != NULL) {
		l = tempchr - tempchr2;
		if (payloadpos + l + 1 > 507)
			return -1;
		payload[payloadpos++] = l;
		memcpy(&payload[payloadpos],tempchr2,l);
		payloadpos += l;
		tempchr2 = &tempchr[1];
	}
	l = strlen(tempchr2);
	if (l) {
		if (payloadpos + l + 2 > 507)
			return -1;
		payload[payloadpos++] = l;
		memcpy(&payload[payloadpos],tempchr2,l);
		payloadpos += l;
		payload[payloadpos++] = '\0';
	}
	if (payloadpos > 508)
		return -1;
	l = htons(rr);
	memcpy(&payload[payloadpos],&l,2);
	l = htons(class);
	memcpy(&payload[payloadpos + 2],&l,2);
	return payloadpos + 4;
}

struct in_addr *firedns_aton4(const char * const ipstring) {
	static unsigned char ip[4];
	int i,f;
	char instring[16];
	char *tempchr;

	/* fairly simple; find .'s, check that numbers between are valid */
	i = strlen(ipstring);
	if (i > 15)
		return NULL;
	memcpy(instring,ipstring,i+1);

	i = atoi(instring);
	if (i < 0 || i > 255)
		return NULL;
	ip[0] = i;
	tempchr = instring;
	for (f = 1; f < 4; f++) {
		tempchr = strchr(tempchr,'.');
		if (tempchr == NULL)
			return NULL;
		i = atoi(++tempchr);
		if (i < 0 || i > 255)
			return NULL;
		ip[f] = i;
	}
	return (struct in_addr *)ip;
}

static int hextoi(const char * const hexstring) {
	int out;
	/* return int value for 2 byte hex value (-1 if invalid) */
	switch (hexstring[0]) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			out = hexstring[0] - '0';
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			out = hexstring[0] - 'a' + 10;
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			out = hexstring[0] - 'A' + 10;
			break;
		default:
			return -1;
	}
	out <<= 4;
	switch (hexstring[1]) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			out += hexstring[1] - '0';
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			out += hexstring[1] - 'a' + 10;
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			out += hexstring[1] - 'A' + 10;
			break;
		default:
			return -1;
	}
	return out;
}

struct in6_addr *firedns_aton6(const char * const ipstring) {
	/* black magic */
	static struct in6_addr ip;
	char instring[40];
	char tempstr[5];
	int i,o;
	int direction = 1;
	char *tempchr,*tempchr2;;

	i = strlen(ipstring);
	if (i > 39)
		return NULL;
	memcpy(instring,ipstring,i+1);

	bzero(ip.s6_addr,16);

	tempchr2 = instring;
	i = 0;
	while (direction > 0) {
		if (direction == 1) {
			tempchr = strchr(tempchr2,':');
			if (tempchr == NULL && i != 14)
				return NULL;
			if (tempchr != NULL)
				tempchr[0] = '\0';
			o = strlen(tempchr2);
			if (o > 4)
				return NULL;
			strcpy(tempstr,"0000");
			strcpy(&tempstr[4 - o],tempchr2);
			o = hextoi(tempstr);
			if (o == -1)
				return NULL;
			ip.s6_addr[i++] = o;
			o = hextoi(&tempstr[2]);
			if (o == -1)
				return NULL;
			ip.s6_addr[i++] = o;
			if (i >= 15)
				break;
			tempchr2 = tempchr + 1;
			if (tempchr2[0] == ':') {
				tempchr2++;
				direction = 2;
				i = 15;
				continue;
			}
		}
		if (direction == 2) {
			tempchr = strrchr(tempchr2,':');
			if (tempchr == NULL)
				tempchr = tempchr2;
			else {
				tempchr[0] = '\0';
				tempchr++;
			}
			o = strlen(tempchr);
			if (o > 4)
				return NULL;
			strcpy(tempstr,"0000");
			strcpy(&tempstr[4 - o],tempchr);
			o = hextoi(&tempstr[2]);
			if (o == -1)
				return NULL;
			ip.s6_addr[i--] = o;
			o = hextoi(tempstr);
			if (o == -1)
				return NULL;
			ip.s6_addr[i--] = o;
			if (i <= 1)
				break;
			if (tempchr == tempchr2)
				break;
		}
	}
	return &ip;
}

int firedns_getip4(const char * const name) {
	struct s_header h;
	struct s_connection *s;
	int l;

	init();
	

	l = firedns_build_query_payload(name,1,1,(unsigned char *)&h.payload);
	if (l == -1)
		return -1;
	s = firedns_add_query(&h);
	if (s == NULL)
		return -1;
	s->class = 1;
	s->type = 1;
	if (firedns_send_requests(&h,s,l) == -1)
		return -1;

	return s->fd;
}

int firedns_getip6(const char * const name) {
	struct s_header h;
	struct s_connection *s;
	int l;

	init();

	l = firedns_build_query_payload(name,28,1,(unsigned char *)&h.payload);
	if (l == -1)
		return -1;
	s = firedns_add_query(&h);
	if (s == NULL)
		return -1;
	s->class = 1;
	s->type = 28;
	if (firedns_send_requests(&h,s,l) == -1)
		return -1;

	return s->fd;
}

int firedns_gettxt(const char * const name) {
	struct s_header h;
	struct s_connection *s;
	int l;

	init();

	l = firedns_build_query_payload(name,16,1,(unsigned char *)&h.payload);
	if (l == -1)
		return -1;
	s = firedns_add_query(&h);
	if (s == NULL)
		return -1;
	s->class = 1;
	s->type = 16;
	if (firedns_send_requests(&h,s,l) == -1)
		return -1;

	return s->fd;
}

int firedns_getname4(const struct in_addr *ip) {
	char query[512];
	struct s_header h;
	struct s_connection *s;
	unsigned char *c;
	int l;

	init();

	c = (unsigned char *)&ip->s_addr;

	sprintf(query,"%d.%d.%d.%d.in-addr.arpa",c[3],c[2],c[1],c[0]);

	l = firedns_build_query_payload(query,12,1,(unsigned char *)&h.payload);
	if (l == -1)
		return -1;
	s = firedns_add_query(&h);
	if (s == NULL)
		return -1;
	s->class = 1;
	s->type = 12;
	if (firedns_send_requests(&h,s,l) == -1)
		return -1;

	return s->fd;
}

int firedns_getname6(const struct in6_addr *ip) {
	char query[512];
	struct s_header h;
	struct s_connection *s;
	int l;

	init();

	sprintf(query,"%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.%0x.ip6.int",
			ip->s6_addr[15] & 0x0f,
			(ip->s6_addr[15] & 0xf0) >> 4,
			ip->s6_addr[14] & 0x0f,
			(ip->s6_addr[14] & 0xf0) >> 4,
			ip->s6_addr[13] & 0x0f,
			(ip->s6_addr[13] & 0xf0) >> 4,
			ip->s6_addr[12] & 0x0f,
			(ip->s6_addr[12] & 0xf0) >> 4,
			ip->s6_addr[11] & 0x0f,
			(ip->s6_addr[11] & 0xf0) >> 4,
			ip->s6_addr[10] & 0x0f,
			(ip->s6_addr[10] & 0xf0) >> 4,
			ip->s6_addr[9] & 0x0f,
			(ip->s6_addr[9] & 0xf0) >> 4,
			ip->s6_addr[8] & 0x0f,
			(ip->s6_addr[8] & 0xf0) >> 4,
			ip->s6_addr[7] & 0x0f,
			(ip->s6_addr[7] & 0xf0) >> 4,
			ip->s6_addr[6] & 0x0f,
			(ip->s6_addr[6] & 0xf0) >> 4,
			ip->s6_addr[5] & 0x0f,
			(ip->s6_addr[5] & 0xf0) >> 4,
			ip->s6_addr[4] & 0x0f,
			(ip->s6_addr[4] & 0xf0) >> 4,
			ip->s6_addr[3] & 0x0f,
			(ip->s6_addr[3] & 0xf0) >> 4,
			ip->s6_addr[2] & 0x0f,
			(ip->s6_addr[2] & 0xf0) >> 4,
			ip->s6_addr[1] & 0x0f,
			(ip->s6_addr[1] & 0xf0) >> 4,
			ip->s6_addr[0] & 0x0f,
			(ip->s6_addr[0] & 0xf0) >> 4
			);

	l = firedns_build_query_payload(query,12,1,(unsigned char *)&h.payload);
	if (l == -1)
		return -1;
	s = firedns_add_query(&h);
	if (s == NULL)
		return -1;
	s->class = 1;
	s->type = 12;
	if (firedns_send_requests(&h,s,l) == -1)
		return -1;

	return s->fd;
}

char *firedns_ntoa4(const struct in_addr *ip) {
	static char result[128];
	unsigned char *m;
	m = (unsigned char *)&ip->s_addr;
	sprintf(result,"%d.%d.%d.%d",m[0],m[1],m[2],m[3]);
	return result;
}

char *firedns_ntoa6(const struct in6_addr *ip) {
	static char result[128];
	sprintf(result,"%x:%x:%x:%x:%x:%x:%x:%x",
			ntohs(*((unsigned short *)&ip->s6_addr[0])),
			ntohs(*((unsigned short *)&ip->s6_addr[2])),
			ntohs(*((unsigned short *)&ip->s6_addr[4])),
			ntohs(*((unsigned short *)&ip->s6_addr[6])),
			ntohs(*((unsigned short *)&ip->s6_addr[8])),
			ntohs(*((unsigned short *)&ip->s6_addr[10])),
			ntohs(*((unsigned short *)&ip->s6_addr[12])),
			ntohs(*((unsigned short *)&ip->s6_addr[14])));
	return result;
}

char *firedns_getresult(const int fd) {
	static char result[1024];
	struct s_header h;
	struct s_connection *c, *prev;
	int l,i,q,curanswer,o;
	struct s_rr_middle *rr;
	unsigned short p;

	prev = NULL;
	c = connection_head;
	while (c != NULL) {
		if (c->fd == fd)
			break;
		prev = c;
		c = c->next;
	}
	if (c == NULL)
		return NULL;

	if (prev != NULL)
		prev->next = c->next;
	else
		connection_head = c->next;

	/* TODO: recv packet and decode */
	l = recv(c->fd,&h,sizeof(struct s_header),0);
	firedns_close(c->fd);
	if (l < 12) {
		free(c);
		return NULL;
	}
	if (c->id[0] != h.id[0] || c->id[1] != h.id[1]) {
		free(c);
		return NULL;
	}
	if ((h.flags1 & FLAGS1_MASK_QR) == 0) {
		free(c);
		return NULL;
	}
	if ((h.flags1 & FLAGS1_MASK_OPCODE) != 0) {
		free(c);
		return NULL;
	}
	if ((h.flags2 & FLAGS2_MASK_RCODE) != 0) {
		free(c);
		return NULL;
	}
	h.ancount = ntohs(h.ancount);
	if (h.ancount < 1)  { /* no sense going on if we don't have any answers */
		free(c);
		return NULL;
	}
	/* skip queries */
	i = 0;
	q = 0;
	l -= 12;
	h.qdcount = ntohs(h.qdcount);
	while (q < h.qdcount && i < l) {
		if (h.payload[i] > 63) { /* pointer */
			i += 6; /* skip pointer, class and type */
			q++;
		} else { /* label */
			if (h.payload[i] == 0) {
				q++;
				i += 5; /* skip nil, class and type */
			} else
				i += h.payload[i] + 1; /* skip length and label */
		}
	}
	/* &h.payload[i] should now be the start of the first response */
	curanswer = 0;
	while (curanswer < h.ancount) {
		q = 0;
		while (q == 0 && i < l) {
			if (h.payload[i] > 63) { /* pointer */
				i += 2; /* skip pointer */
				q = 1;
			} else { /* label */
				if (h.payload[i] == 0) {
					q = 1;
				} else
					i += h.payload[i] + 1; /* skip length and label */
			}
		}
		if (l - i < 10) {
			free(c);
			return NULL;
		}
		rr = (struct s_rr_middle *)&h.payload[i];
		i += 10;
		rr->rdlength = ntohs(rr->rdlength);
		if (ntohs(rr->type) != c->type) {
			curanswer++;
			i += rr->rdlength;
			continue;
		}
		if (ntohs(rr->class) != c->class) {
			curanswer++;
			i += rr->rdlength;
			continue;
		}
		break;
	}
	free(c);
	if (curanswer == h.ancount)
		return NULL;
	if (i + rr->rdlength > l)
		return NULL;
	if (rr->rdlength > 1023)
		return NULL;

	switch (ntohs(rr->type)) {
		case 12:
			o = 0;
			q = 0;
			while (q == 0 && i < l && o + 256 < 1023) {
				if (h.payload[i] > 63) { /* pointer */
					memcpy(&p,&h.payload[i],2);
					i = ntohs(p) - FIREDNS_POINTER_VALUE - 12;
				} else { /* label */
					if (h.payload[i] == 0)
						q = 1;
					else {
						result[o] = '\0';
						if (o != 0)
							result[o++] = '.';
						memcpy(&result[o],&h.payload[i + 1],h.payload[i]);
						o += h.payload[i];
						i += h.payload[i] + 1;
					}
				}
			}
			result[o] = '\0';
			break;
		case 16:
			memcpy(result,&h.payload[i + 1],h.payload[i]);
			result[h.payload[i]] = '\0';
			break;
		default:
			memcpy(result,&h.payload[i],rr->rdlength);
			result[rr->rdlength] = '\0';
			break;
	}
	return result;
}

#ifdef FIREDNS_EMULATE_BIND
struct hostent *gethostbyname(const char *name);
struct hostent *gethostbyname2(const char *name, int af);
int inet_pton(int af, const char *src, void *dst);
char *inet_ntop(int af, const void *src, char *dst, size_t cnt);
struct hostent *getipnodebyname(const char *name, int af, int flags, int *error_num);
struct hostent *getipnodebyaddr(const void *addr, size_t len, int af, int *error_num);
void freehostent(struct hostent *ip);
struct hostent *gethostbyaddr(const char *addr, int len, int type);

struct hostent *gethostbyname(const char *name) {
	return getipnodebyname(name,AF_INET,0,NULL);
}

struct hostent *gethostbyname2(const char *name, int af) {
	return getipnodebyname(name,af,0,NULL);
}

int inet_pton(int af, const char *src, void *dst) {
	void *m;
	switch (af) {
		case AF_INET:
			m = firedns_aton4(src);
			if (m == NULL)
				return 0;
			memcpy(dst,m,sizeof(struct in_addr));
			return 1;
			break;
#ifdef AF_INET6
		case AF_INET6:
			m = firedns_aton6(src);
			if (m == NULL)
				return 0;
			memcpy(dst,m,sizeof(struct in6_addr));
			break;
#endif
	}
	errno = EAFNOSUPPORT;
	return -1;
}

char *inet_ntop(int af, const void *src, char *dst, size_t cnt) {
	char *m;
	switch (af) {
		case AF_INET:
			m = firedns_ntoa4(src);
			break;
#ifdef AF_INET6
		case AF_INET6:
			m = firedns_ntoa6(src);
			break;
		default:
			errno = EAFNOSUPPORT;
			return NULL;
#endif
	}
	if (m == NULL)
		return NULL;
	if (strlen(m) >= cnt) {
		errno = ENOSPC;
		return NULL;
	}
	strcpy(dst,m);
	return dst;
}

struct hostent *getipnodebyname(const char *name, int af, int flags, int *error_num) {
	static struct hostent h;
	int fd;
	fd_set s;
	int n;
	struct timeval tv;
	static char *addr_list[2];

	switch (af) {
		case AF_INET:
			h.h_length = 4;
			h.h_addrtype = AF_INET;
			fd = firedns_getip4(name);
			break;
#ifdef AF_INET6
		case AF_INET6:
			h.h_length = 16;
			h.h_addrtype = AF_INET6;
			fd = firedns_getip6(name);
			break;
#endif
		default:
			return NULL;
	}
	if (fd == -1)
		return NULL;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	FD_ZERO(&s);
	FD_SET(fd,&s);
	n = select(fd + 1,&s,NULL,NULL,&tv);
	addr_list[0] = firedns_getresult(fd);
	if (addr_list[0] == NULL)
		return NULL;
	h.h_name = NULL;
	h.h_aliases = NULL;
	h.h_addr_list = addr_list;
	addr_list[1] = NULL;
	return &h;
}

struct hostent *getipnodebyaddr(const void *addr, size_t len, int af, int *error_num) {
	static struct hostent h;
	int fd;
	fd_set s;
	int n;
	struct timeval tv;

	switch (af) {
		case AF_INET:
			h.h_length = 4;
			h.h_addrtype = AF_INET;
			fd = firedns_getname4(addr);
			break;
#ifdef AF_INET6
		case AF_INET6:
			h.h_length = 16;
			h.h_addrtype = AF_INET6;
			fd = firedns_getname6(addr);
			break;
#endif
		default:
			return NULL;
	}
	if (fd == -1)
		return NULL;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	FD_ZERO(&s);
	FD_SET(fd,&s);
	n = select(fd + 1,&s,NULL,NULL,&tv);
	h.h_name = firedns_getresult(fd);
	if (h.h_name == NULL)
		return NULL;
	h.h_aliases = NULL;
	h.h_addr_list = NULL;
	return &h;
}

void freehostent(struct hostent *ip) {
	return;
}

struct hostent *gethostbyaddr(const char *addr, int len, int type) {
	return getipnodebyaddr(addr,len,type,NULL);
}

#endif
