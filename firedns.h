/*
firedns.h - firedns library declarations
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

#ifndef _FIREDNS_H
#define _FIREDNS_H

#include <sys/socket.h>
#include <netinet/in.h>

#ifdef AF_INET6
#ifndef FIREDNS_NO_IPV6
#define FIREDNS_USE_IPV6
#endif
#endif

#define FIREDNS_VERSION "0.1.4"

#ifndef AF_INET6
struct in6_addr {
	unsigned char   s6_addr[16];
};
#endif

struct in_addr *firedns_aton4(const char * const ipstring);
struct in6_addr *firedns_aton6(const char * const ipstring);
char *firedns_ntoa4(const struct in_addr *ip);
char *firedns_ntoa6(const struct in6_addr *ip);
int firedns_getip4(const char * const name);
int firedns_getip6(const char * const name);
int firedns_gettxt(const char * const name);
int firedns_getname4(const struct in_addr *ip);
int firedns_getname6(const struct in6_addr *ip);
char *firedns_getresult(const int fd);

#endif
