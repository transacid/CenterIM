/*
toc.c - FireTalk TOC protocol definitions
Copyright (C) 2000 Ian Gulliver

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
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#define __USE_XOPEN
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include "toc2_uuids.h"

/* Structures */

struct s_toc_room {
	struct s_toc_room *next;
	int exchange;
	char *name;
	long    id;
	unsigned char
		invited:1,
		joined:1;
};

#define TOC_HTML_MAXLEN 65536

struct s_toc_infoget {
	int sockfd;
	struct s_toc_infoget *next;
#define TOC_STATE_CONNECTING 0
#define TOC_STATE_TRANSFERRING 1
	int state;
	char buffer[TOC_HTML_MAXLEN];
	int buflen;
};

struct s_toc_connection {
	unsigned short local_sequence;                       /* our sequence number */
	unsigned short remote_sequence;                      /* toc's sequence number */
	char *nickname;                                      /* our nickname (correctly spaced) */
	struct s_toc_room *room_head;
	struct s_toc_infoget *infoget_head;
	time_t lasttalk;                                     /* last time we talked */
	long lastidle;                                  /* last idle that we told the server */
	double lastsend;
	int passchange;                                      /* whether we're changing our password right now */
	int online;
	int connectstate;
	int gotconfig;
	char    *profstr;
	size_t  proflen;
	int permit_mode;
	struct {
		char    *command,
			*ect;
	}       *profar;
	int     profc;
	struct {
		char    *dest,
			*ect;
	}       *ectqar;
	int     ectqc;
	char    buddybuf[1024];
	int     buddybuflen;
};

typedef struct s_toc_connection *client_t;
#define _HAVE_CLIENT_T

#include "firetalk-int.h"
#include "firetalk.h"
#include "toc.h"
#include "aim.h"
#include "safestring.h"

#define SFLAP_FRAME_SIGNON ((unsigned char)1)
#define SFLAP_FRAME_DATA ((unsigned char)2)
#define SFLAP_FRAME_ERROR ((unsigned char)3)
#define SFLAP_FRAME_SIGNOFF ((unsigned char)4)
#define SFLAP_FRAME_KEEPALIVE ((unsigned char)5)

#define SIGNON_STRING "FLAPON\r\n\r\n"

#define TOC_SERVER "toc.oscar.aol.com"
#define TOC_PORT 9898

#define TOC_HEADER_LENGTH 6
#define TOC_SIGNON_LENGTH 24
#define TOC_HOST_SIGNON_LENGTH 4
#define TOC_USERNAME_MAXLEN 16

#define TOC_HTML_MAXLEN 65536
#define TOC_CLIENTSEND_MAXLEN (2*1024)
#define TOC_SERVERSEND_MAXLEN (8*1024)
#define ECT_TOKEN       "<font ECT=\""
#define ECT_ENDING      "\"></font>"

static char lastinfo[TOC_USERNAME_MAXLEN + 1] = "";

/* Internal Function Declarations */

static unsigned short toc_fill_header(unsigned char *const header, unsigned char const frame_type, unsigned short const sequence, unsigned short const length);
static unsigned short toc_fill_signon(unsigned char *const signon, const char *const username);
static unsigned char toc_get_frame_type_from_header(const unsigned char *const header);
static unsigned short toc_get_sequence_from_header(const unsigned char *const header);
static unsigned short toc_get_length_from_header(const unsigned char *const header);
static char *toc_quote(const char *const string, const int outside_flag);
static char *toc_hash_password(const char *const password);
static int toc_internal_disconnect(client_t c, const int error);
static int toc_internal_add_room(client_t c, const char *const name, const int exchange);
static int toc_internal_find_exchange(client_t c, const char *const name);
static int toc_internal_set_joined(client_t c, const long id);
static int toc_internal_set_id(client_t c, const char *const name, const int exchange, const long id);
static int toc_internal_find_room_id(client_t c, const char *const name);
static char *toc_internal_find_room_name(client_t c, const long id);
static char **toc_parse_args(char *const instring, int maxargs);
static int toc_internal_split_exchange(const char *const string);
static char *toc_internal_split_name(const char *const string);
static char *toc_get_tlv_value(char ** args, const int startarg, const int type);
enum firetalk_error   toc_find_packet(client_t c, unsigned char *buffer, unsigned short *bufferpos, char *outbuffer, const int frametype, unsigned short *l);
static int toc_send_printf(client_t c, const char *const format, ...);

const char
	*firetalk_htmlentities(const char *str) {
	static char
		buf[1024];
	int     i, b = 0;

	for (i = 0; (str[i] != 0) && (b < sizeof(buf)-6-1); i++)
		switch (str[i]) {
		  case '<':
			buf[b++] = '&';
			buf[b++] = 'l';
			buf[b++] = 't';
			buf[b++] = ';';
			break;
		  case '>':
			buf[b++] = '&';
			buf[b++] = 'g';
			buf[b++] = 't';
			buf[b++] = ';';
			break;
		  case '&':
			buf[b++] = '&';
			buf[b++] = 'a';
			buf[b++] = 'm';
			buf[b++] = 'p';
			buf[b++] = ';';
			break;
		  case '"':
			buf[b++] = '&';
			buf[b++] = 'q';
			buf[b++] = 'u';
			buf[b++] = 'o';
			buf[b++] = 't';
			buf[b++] = ';';
			break;
		  case '\n':
			buf[b++] = '<';
			buf[b++] = 'b';
			buf[b++] = 'r';
			buf[b++] = '>';
			break;
		  default:
			buf[b++] = str[i];
			break;
		}
	buf[b] = 0;
	return(buf);
}

static void toc_uuid(const char *const uuid, int *A1, int *A2, int *B, int *C, int *D, int *E1, int *E2, int *E3) {
	if (strlen(uuid) <= 4) {
		*A1 = 0x0946;
		*A2 = strtol(uuid, NULL, 16);
		*B = 0x4C7F;
		*C = 0x11D1;
		*D = 0x8222;
		*E1 = 0x4445;
		*E2 = 0x5354;
		*E3 = 0x0000;
	} else {
		char    buf[5];

		buf[4] = 0;
		strncpy(buf, uuid+0, 4);
		*A1 = strtol(buf, NULL, 16);
		strncpy(buf, uuid+4, 4);
		*A2 = strtol(buf, NULL, 16);
		strncpy(buf, uuid+9, 4);
		*B = strtol(buf, NULL, 16);
		strncpy(buf, uuid+14, 4);
		*C = strtol(buf, NULL, 16);
		strncpy(buf, uuid+19, 4);
		*D = strtol(buf, NULL, 16);
		strncpy(buf, uuid+24, 4);
		*E1 = strtol(buf, NULL, 16);
		strncpy(buf, uuid+28, 4);
		*E2 = strtol(buf, NULL, 16);
		strncpy(buf, uuid+32, 4);
		*E3 = strtol(buf, NULL, 16);
	}
}

static int toc_cap(int A1, int A2, int B, int C, int D, int E1, int E2, int E3) {
	int     j;

	for (j = 0; j < sizeof(toc_uuids)/sizeof(*toc_uuids); j++)
		if (((toc_uuids[j].A1 == A1) || (toc_uuids[j].A1 == -1))
		 && ((toc_uuids[j].A2 == A2) || (toc_uuids[j].A2 == -1))
		 && ((toc_uuids[j].B  == B)  || (toc_uuids[j].B  == -1))
		 && ((toc_uuids[j].C  == C)  || (toc_uuids[j].C  == -1))
		 && ((toc_uuids[j].D  == D)  || (toc_uuids[j].D  == -1))
		 && ((toc_uuids[j].E1 == E1) || (toc_uuids[j].E1 == -1))
		 && ((toc_uuids[j].E2 == E2) || (toc_uuids[j].E2 == -1))
		 && ((toc_uuids[j].E3 == E3) || (toc_uuids[j].E3 == -1)))
		return(j);
	return(-1);
}

#ifdef DEBUG_ECHO
static void toc_echof(client_t c, const char *const where, const char *const format, ...) {
	va_list ap;
	char    buf[8*1024];
	void    statrefresh(void);

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	while (buf[strlen(buf)-1] == '\n')
		buf[strlen(buf)-1] = 0;
	if (*buf != 0)
		firetalk_callback_chat_getmessage(c, ":RAW", where, 0, firetalk_htmlentities(buf));

	statrefresh();
}

static void toc_echo_send(client_t c, const char *const where, const char *const data, size_t _length) {
	unsigned char   ft;
	unsigned short  sequence,
			length;

	length = toc_get_length_from_header(data);
	ft = toc_get_frame_type_from_header(data);
	sequence = toc_get_sequence_from_header(data);

	toc_echof(c, where, "frame=%X, sequence=out:%i, length=%i, value=[%.*s]\n",
		ft, sequence, length, length, data+TOC_HEADER_LENGTH);
}
#endif

/* Internal Function Definitions */

enum firetalk_error   toc_find_packet(client_t c, unsigned char *buffer, unsigned short *bufferpos, char *outbuffer, const int frametype, unsigned short *l) {
	unsigned char   ft;
	unsigned short  sequence,
			length;

	if (*bufferpos < TOC_HEADER_LENGTH) /* don't have the whole header yet */
		return FE_NOTFOUND;

	length = toc_get_length_from_header(buffer);
	if (length > (8192 - TOC_HEADER_LENGTH)) {
		toc_internal_disconnect(c,FE_PACKETSIZE);
		return FE_DISCONNECT;
	}

	if (*bufferpos < length + TOC_HEADER_LENGTH) /* don't have the whole packet yet */
		return FE_NOTFOUND;

	ft = toc_get_frame_type_from_header(buffer);
	sequence = toc_get_sequence_from_header(buffer);

	memcpy(outbuffer, &buffer[TOC_HEADER_LENGTH], length);
	*bufferpos -= length + TOC_HEADER_LENGTH;
	memmove(buffer, &buffer[TOC_HEADER_LENGTH + length], *bufferpos);
	outbuffer[length] = '\0';
	*l = length;

#ifdef DEBUG_ECHO
	/*if (ft == frametype)*/ {
		char    buf[1024*8];
		int     j;

		memmove(buf, outbuffer, length+1);
		for (j = 0; j < length; j++)
			if (buf[j] == 0)
				buf[j] = '0';
			else if (buf[j] == '\n')
				buf[j] = 'N';
			else if (buf[j] == '\r')
				buf[j] = 'R';

		toc_echof(c, "find_packet", "frame=%X, sequence=in:%i, length=%i, value=[%s]\n",
			ft, sequence, length, buf);
	}
#endif

	if (frametype == SFLAP_FRAME_SIGNON)
		c->remote_sequence = sequence;
	else {
		if (sequence != ++c->remote_sequence) {
			toc_internal_disconnect(c, FE_SEQUENCE);
			return(FE_DISCONNECT);
		}
	}

	if (ft == frametype)
		return(FE_SUCCESS);
	else
		return(FE_WIERDPACKET);
}

static unsigned short toc_fill_header(unsigned char *const header, unsigned char const frame_type, unsigned short const sequence, unsigned short const length) {
	header[0] = '*';            /* byte 0, length 1, magic 42 */
	header[1] = frame_type;     /* byte 1, length 1, frame type (defined above SFLAP_FRAME_* */
	header[2] = sequence/256;   /* byte 2, length 2, sequence number, network byte order */
	header[3] = (unsigned char) sequence%256;   
	header[4] = length/256;     /* byte 4, length 2, length, network byte order */
	header[5] = (unsigned char) length%256;
	return 6 + length;
}

static unsigned short toc_fill_signon(unsigned char *const signon, const char *const username) {
	size_t length;
	length = strlen(username);
	signon[0] = '\0';              /* byte 0, length 4, flap version (1) */
	signon[1] = '\0';
	signon[2] = '\0';
	signon[3] = '\001';
	signon[4] = '\0';              /* byte 4, length 2, tlv tag (1) */
	signon[5] = '\001';
	signon[6] = length/256;     /* byte 6, length 2, username length, network byte order */
	signon[7] = (unsigned char) length%256;
	memcpy(&signon[8],username,length);
	return(length + 8);
}

void toc_infoget_parse(client_t c, struct s_toc_infoget *i) {
	char    *tmp, *str = i->buffer,
		*name, *info;
	int     class = 0, warning;
	long    online, idle;

	i->buffer[i->buflen] = 0;
#ifdef DEBUG_ECHO
	toc_echof(c, "infoget_parse", "str=[%s]\n", str);
#endif

#define USER_STRING "Username : <B>"
	if ((tmp = strstr(str, USER_STRING)) == NULL) {
		firetalk_callback_error(c, FE_INVALIDFORMAT, (lastinfo[0] == '\0')?NULL:lastinfo, "Can't find username in info HTML");
		return;
	}
	tmp += sizeof(USER_STRING) - 1;
	if ((str = strchr(tmp, '<')) == NULL) {
		firetalk_callback_error(c, FE_INVALIDFORMAT, (lastinfo[0] == '\0')?NULL:lastinfo, "Can't find end of username in info HTML");
		return;
	}
	*str++ = 0;
	name = tmp;

#define WARNING_STRING "Warning Level : <B>"
	if ((tmp = strstr(str, WARNING_STRING)) == NULL) {
		firetalk_callback_error(c, FE_INVALIDFORMAT, name, "Can't find warning level in info HTML");
		return;
	}

	*tmp = 0;
	if ((strstr(str, "free_icon.") != NULL) || (strstr(str, "dt_icon.") != NULL))
		class |= FF_SUBSTANDARD;
	if (strstr(str, "aol_icon.") != NULL)
		class |= FF_NORMAL;
	if (strstr(str, "admin_icon.") != NULL)
		class |= FF_ADMIN;

	tmp += sizeof(WARNING_STRING) - 1;
	if ((str = strchr(tmp, '<')) == NULL) {
		firetalk_callback_error(c, FE_INVALIDFORMAT, (lastinfo[0] == '\0')?NULL:lastinfo, "Can't find end of warning level in info HTML");
		return;
	}
	*str++ = 0;
	warning = atoi(tmp);

#define ONLINE_STRING "Online Since : <B>"
	if ((tmp = strstr(str, ONLINE_STRING)) == NULL) {
		firetalk_callback_error(c, FE_INVALIDFORMAT, name, "Can't find online time in info HTML");
		return;
	}
	tmp += sizeof(ONLINE_STRING) - 1;
	if ((str = strchr(tmp, '<')) == NULL) {
		firetalk_callback_error(c, FE_INVALIDFORMAT, (lastinfo[0] == '\0')?NULL:lastinfo, "Can't find end of online time in info HTML");
		return;
	}
	*str++ = 0;
#ifdef HAVE_STRPTIME
	{
		struct tm tm;

		if (strptime(tmp, "%a %b %d %H:%M:%S %Y", &tm) == NULL)
			online = 0;
		else
			online = mktime(&tm);
# ifdef DEBUG_ECHO
		toc_echof(c, "infoget_parse", "tmp=[%s], tm={ tm_year=%i, tm_mon=%i, tm_mday=%i }, online=%lu\n", tmp, tm.tm_year, tm.tm_mon, tm.tm_mday, online);
# endif
	}
#else
	online = 0;
#endif

#define IDLE_STRING "Idle Minutes : <B>" 
	if ((tmp = strstr(str, IDLE_STRING)) == NULL) {
		firetalk_callback_error(c, FE_INVALIDFORMAT, name, "Can't find idle time in info HTML");
		return;
	}
	tmp += sizeof(IDLE_STRING) - 1;
	if ((str = strchr(tmp, '<')) == NULL) {
		firetalk_callback_error(c, FE_INVALIDFORMAT, (lastinfo[0] == '\0')?NULL:lastinfo, "Can't find end of idle time in info HTML");
		return;
	}
	*str++ = 0;
	idle = atoi(tmp);

#define INFO_STRING "<hr><br>\n"
#define INFO_END_STRING "<br><hr><I>Legend:"
	if ((tmp = strstr(str, INFO_STRING)) == NULL) {
		firetalk_callback_error(c, FE_INVALIDFORMAT, (lastinfo[0] == '\0')?NULL:lastinfo, "Can't find info string in info HTML");
		return;
	}
	tmp += sizeof(INFO_STRING) - 1;
	if ((str = strstr(tmp, INFO_END_STRING)) == NULL)
		info = NULL;
	else {
		*str = 0;
		info = aim_interpolate_variables(tmp, c->nickname);
		info = aim_handle_ect(c, name, info, 1);
	}

	firetalk_callback_gotinfo(c, name, info, warning, idle, class);
}

void toc_infoget_remove(client_t c, struct s_toc_infoget *i, char *error) {
	struct s_toc_infoget *m, *m2;
	if (error != NULL)
		firetalk_callback_error(c,FE_USERINFOUNAVAILABLE,NULL,error);
	m = c->infoget_head;
	m2 = NULL;
	while (m != NULL) {
		if (m == i) {
			if (m2 == NULL)
				c->infoget_head = m->next;
			else
				m2->next = m->next;
			close(m->sockfd);
			free(m);
			return;
		}
		m2 = m;
		m = m->next;
	}
}

enum firetalk_error   toc_preselect(client_t c, fd_set *read, fd_set *write, fd_set *except, int *n) {
	struct s_toc_infoget *i;
	i = c->infoget_head;
	while (i != NULL) {
		if (i->state == TOC_STATE_CONNECTING)
			FD_SET(i->sockfd,write);
		else if (i->state == TOC_STATE_TRANSFERRING)
			FD_SET(i->sockfd,read);
		FD_SET(i->sockfd,read);
		if (i->sockfd >= *n)
			*n = i->sockfd + 1;
		i = i->next;
	}

	if (c->ectqc > 0) {
		int     i;

		for (i = c->ectqc-1; i >= 0; i--)
			toc_im_send_message(c, c->ectqar[i].dest, "", 0);
	}
	if (c->buddybuflen > 0) {
		toc_send_printf(c, "toc_add_buddy%S", c->buddybuf);
		c->buddybuflen = 0;
	}
	return FE_SUCCESS;
}

enum firetalk_error   toc_postselect(client_t c, fd_set *read, fd_set *write, fd_set *except) {
	struct s_toc_infoget *i,*i2;
	i = c->infoget_head;
	while (i != NULL) {
		if (FD_ISSET(i->sockfd,except)) {
			toc_infoget_remove(c,i,strerror(errno));
			i = i->next;
			continue;
		}
		if (i->state == TOC_STATE_CONNECTING && FD_ISSET(i->sockfd,write)) {
			int r;
			unsigned int o = sizeof(int);
			if (getsockopt(i->sockfd,SOL_SOCKET,SO_ERROR,&r,&o)) {
				i2 = i->next;
				toc_infoget_remove(c,i,strerror(errno));
				i = i2;
				continue;
			}
			if (r != 0) {
				i2 = i->next;
				toc_infoget_remove(c,i,strerror(r));
				i = i2;
				continue;
			}
			if (send(i->sockfd,i->buffer,i->buflen,0) != i->buflen) {
				i2 = i->next;
				toc_infoget_remove(c,i,strerror(errno));
				i = i2;
				continue;
			}
			i->buflen = 0;
			i->state = TOC_STATE_TRANSFERRING;
		} else if (i->state == TOC_STATE_TRANSFERRING && FD_ISSET(i->sockfd,read)) {
			ssize_t s;
			while (1) {
				s = recv(i->sockfd,&i->buffer[i->buflen],TOC_HTML_MAXLEN - i->buflen - 1,MSG_DONTWAIT);
				if (s <= 0)
					break;
				i->buflen += s;
				if (i->buflen == TOC_HTML_MAXLEN - 1) {
					s = -2;
					break;
				}
			}
			if (s == -2) {
				i2 = i->next;
				toc_infoget_remove(c,i,"Too much data");
				i = i2;
				continue;
			}
			if (s == -1) {
				i2 = i->next;
				if (errno != EAGAIN)
					toc_infoget_remove(c,i,strerror(errno));
				i = i2;
				continue;
			}
			if (s == 0) {
				/* finished, parse results here */
				toc_infoget_parse(c,i);
				i2 = i->next;
				toc_infoget_remove(c,i,NULL);
				i = i2;
				continue;
			}

		}
		i = i->next;
	}

	return FE_SUCCESS;
}

static unsigned char toc_get_frame_type_from_header(const unsigned char *const header) {
	return header[1];
}

static unsigned short toc_get_sequence_from_header(const unsigned char *const header) {
	unsigned short sequence;
	sequence = ntohs(* ((unsigned short *)(&header[2])));
	return sequence;
}

static unsigned short toc_get_length_from_header(const unsigned char *const header) {
	unsigned short length;
	length = ntohs(* ((unsigned short *)(&header[4])));
	return length;
}

static char *toc_quote(const char *string, const int outside_flag) {
	static char output[2048];
	size_t length;
	size_t counter;
	int newcounter;

	while (*string == ' ')
		string++;

	length = strlen(string);
	if (outside_flag == 1) {
		newcounter = 1;
		output[0] = '"';
	} else
		newcounter = 0;

	while ((length > 0) && (string[length-1] == ' '))
		length--;

	for (counter = 0; counter < length; counter++) {
		if (string[counter] == '$' || string[counter] == '{' || string[counter] == '}' || string[counter] == '[' || string[counter] == ']' || string[counter] == '(' || string[counter] == ')' || string[counter] == '\'' || string[counter] == '`' || string[counter] == '"' || string[counter] == '\\') {
			if (newcounter > (sizeof(output)-4))
				return NULL;
			output[newcounter++] = '\\';
			output[newcounter++] = string[counter];
		} else {
			if (newcounter > (sizeof(output)-3))
				return NULL;
			output[newcounter++] = string[counter];
		}
	}

	if (outside_flag == 1)
		output[newcounter++] = '"';
	output[newcounter] = '\0';

	return output;
}

static char *toc_hash_password(const char *const password) {
#define HASH "Tic/Toc"
	const char hash[sizeof(HASH)] = HASH;
	static char output[2048];
	size_t counter;
	int newcounter;
	size_t length;

	length = strlen(password);

	output[0] = '0';
	output[1] = 'x';

	newcounter = 2;

	for (counter = 0; counter < length; counter++) {
		if (newcounter > 2044)
			return NULL;
		sprintf(&output[newcounter],"%02x",(unsigned int) password[counter] ^ hash[((counter) % (sizeof(HASH)-1))]);
		newcounter += 2;
	}

	output[newcounter] = '\0';

	return output;
}

static int toc_internal_disconnect(client_t c, const int error) {
	if (c->nickname) {
		free(c->nickname);
		c->nickname = NULL;
	}
	if (c->profc > 0) {
		int     i;

		for (i = 0; i < c->profc; i++) {
			free(c->profar[i].command);
			free(c->profar[i].ect);
		}
		free(c->profar);
		c->profar = NULL;
		c->profc = 0;
	}
	if (c->proflen > 0) {
		free(c->profstr);
		c->profstr = NULL;
		c->proflen = 0;
	}
	if (c->ectqc > 0) {
		int     i;

		for (i = 0; i < c->ectqc; i++) {
			free(c->ectqar[i].dest);
			free(c->ectqar[i].ect);
		}
		free(c->ectqar);
		c->ectqar = NULL;
		c->ectqc = 0;
	}

	firetalk_callback_disconnect(c,error);
	return FE_SUCCESS;
}

static int toc_internal_add_room(client_t c, const char *const name, const int exchange) {
	struct s_toc_room *iter;

	iter = c->room_head;
	c->room_head = calloc(1, sizeof(struct s_toc_room));
	if (c->room_head == NULL)
		abort();

	c->room_head->next = iter;
	c->room_head->name = strdup(name);
	if (c->room_head->name == NULL)
		abort();
	c->room_head->exchange = exchange;
	return(FE_SUCCESS);
}

static int toc_internal_set_joined(client_t c, const long id) {
	struct s_toc_room *iter;

	for (iter = c->room_head; iter != NULL; iter = iter->next)
		if (iter->joined == 0)
			if (iter->id == id) {
				iter->joined = 1;
				return(FE_SUCCESS);
			}
	return(FE_NOTFOUND);
}

static int toc_internal_set_id(client_t c, const char *const name, const int exchange, const long id) {
	struct s_toc_room *iter;

	for (iter = c->room_head; iter != NULL; iter = iter->next)
		if (iter->joined == 0)
			if ((iter->exchange == exchange) && (toc_compare_nicks(iter->name, name) == 0)) {
				iter->id = id;
				return(FE_SUCCESS);
			}
	return(FE_NOTFOUND);
}
	
static int toc_internal_find_exchange(client_t c, const char *const name) {
	struct s_toc_room *iter;

	iter = c->room_head;

	while (iter) {
		if (iter->joined == 0)
			if (toc_compare_nicks(iter->name,name) == 0)
				return iter->exchange;
		iter = iter->next;
	}

	firetalkerror = FE_NOTFOUND;
	return 0;
}

static int toc_internal_find_room_id(client_t c, const char *const name) {
	struct s_toc_room *iter;
	char *namepart;
	int exchange;

	namepart = toc_internal_split_name(name);
	exchange = toc_internal_split_exchange(name);

	for (iter = c->room_head; iter != NULL; iter = iter->next)
		if (iter->exchange == exchange)
			if (toc_compare_nicks(iter->name, namepart) == 0)
				return(iter->id);
	firetalkerror = FE_NOTFOUND;
	return(0);
}

static char *toc_internal_find_room_name(client_t c, const long id) {
	struct s_toc_room *iter;
	static char newname[TOC_CLIENTSEND_MAXLEN];

	for (iter = c->room_head; iter != NULL; iter = iter->next)
		if (iter->id == id) {
			snprintf(newname, sizeof(newname), "%d:%s", 
				iter->exchange, iter->name);
			return(newname);
		}
	firetalkerror = FE_NOTFOUND;
	return(NULL);
}

static int toc_internal_split_exchange(const char *const string) {
	return atoi(string);
}

static char *toc_internal_split_name(const char *const string) {
	return strchr(string,':') + 1;
}

static unsigned char toc_debase64(const char c) {
	if (c >= 'A' && c <= 'Z')
		return (unsigned char) (c - 'A');
	if (c >= 'a' && c <= 'z')
		return (unsigned char) ((char) 26 + (c - 'a'));
	if (c >= '0' && c <= '9')
		return (unsigned char) ((char) 52 + (c - '0'));
	if (c == '+')
		return (unsigned char) 62;
	if (c == '/')
		return (unsigned char) 63;
	return (unsigned char) 0;
}

static char *toc_get_tlv_value(char **args, const int startarg, const int type) {
	int i,o,s,l;
	static unsigned char out[256];
	i = startarg;
	while (args[i]) {
		if (atoi(args[i]) == type) {
			/* got it, now de-base 64 the next block */
			i++;
			o = -8;
			l = (int) strlen(args[i]);
			for (s = 0; s <= l - 3; s += 4) {
				if (o >= 0)
					out[o] = (toc_debase64(args[i][s]) << 2) | (toc_debase64(args[i][s+1]) >> 4);
				o++;
				if (o >= 0)
					out[o] = (toc_debase64(args[i][s+1]) << 4) | (toc_debase64(args[i][s+2]) >> 2);
				o++;
				if (o >= 0)
					out[o] = (toc_debase64(args[i][s+2]) << 6) | toc_debase64(args[i][s+3]);
				o++;
			}
			return (char *) out;
		}
		i += 2;
	}
	return NULL;
}

static int toc_internal_set_room_invited(client_t c, const char *const name, const int invited) {
	struct s_toc_room *iter;

	iter = c->room_head;

	while (iter) {
		if (toc_compare_nicks(iter->name,name) == 0) {
			iter->invited = invited;
			return FE_SUCCESS;
		}
		iter = iter->next;
	}

	return FE_NOTFOUND;
}

static int toc_internal_get_room_invited(client_t c, const char *const name) {
	struct s_toc_room *iter;

	iter = c->room_head;

	while (iter) {
		if (toc_compare_nicks(aim_normalize_room_name(iter->name),name) == 0)
			return iter->invited;
		iter = iter->next;
	}

	return -1;
}

static int toc_send_printf(client_t c, const char *const format, ...) {
	va_list ap;
	size_t  i,
		datai = TOC_HEADER_LENGTH;
	char    data[2048];

	va_start(ap, format);
	for (i = 0; format[i] != 0; i++) {
		if (format[i] == '%') {
			switch (format[++i]) {
				case 'd': {
						int     i = va_arg(ap, int);

						i = snprintf(data+datai, sizeof(data)-datai, "%i", i);
						if (i > 0)
							datai += i;
						break;
					}
				case 's': {
						const char
							*s = toc_quote(va_arg(ap, char *), 1);
						size_t  slen = strlen(s);

						if ((datai+slen) > (sizeof(data)-1))
							return(FE_PACKETSIZE);
						strcpy(data+datai, s);
						datai += slen;
						break;
					}
				case 'S': {
						const char
							*s = va_arg(ap, char *);
						size_t  slen = strlen(s);

						if ((datai+slen) > (sizeof(data)-1))
							return(FE_PACKETSIZE);
						strcpy(data+datai, s);
						datai += slen;
						break;
					}
				case '%':
					data[datai++] = '%';
					break;
			}
		} else {
			data[datai++] = format[i];
			if (datai > (sizeof(data)-1))
				return(FE_PACKETSIZE);
		}
	}
	va_end(ap);

#ifdef DEBUG_ECHO
	toc_echof(c, "send_printf", "frame=%X, sequence=out:%i, length=%i, value=[%.*s]\n",
		SFLAP_FRAME_DATA, c->local_sequence+1,
		datai-TOC_HEADER_LENGTH, datai-TOC_HEADER_LENGTH, data+TOC_HEADER_LENGTH);
#endif

	{
		struct s_firetalk_handle
			*fchandle;
		unsigned short
			length;

		fchandle = firetalk_find_handle(c);
		data[datai] = 0;
		length = toc_fill_header((unsigned char *)data, SFLAP_FRAME_DATA, ++c->local_sequence, datai-TOC_HEADER_LENGTH+1);
		firetalk_internal_send_data(fchandle, data, length, 0);
	}
	return(FE_SUCCESS);
}

static char *toc_get_arg0(char *const instring) {
	static char data[8192];
	char *tempchr;

	if (strlen(instring) > 8192) {
		firetalkerror = FE_PACKETSIZE;
		return NULL;
	}

	safe_strncpy(data,instring,8192);
	tempchr = strchr(data,':');
	if (tempchr)
		tempchr[0] = '\0';
	return data;
}

static char **toc_parse_args(char *const instring, const int maxargs) {
	static char *args[256];
	int curarg;
	char *tempchr;
	char *tempchr2;

	curarg = 0;
	tempchr = instring;

	while (curarg < (maxargs - 1) && curarg < 256 && ((tempchr2 = strchr(tempchr,':')) != NULL)) {
		args[curarg++] = tempchr;
		tempchr2[0] = '\0';
		tempchr = tempchr2 + 1;
	}
	args[curarg++] = tempchr;
	args[curarg] = NULL;
	return args;
}

/* External Function Definitions */

enum firetalk_error   toc_compare_nicks (const char *s1, const char *s2) {
	if ((s1 == NULL) || (s2 == NULL))
		return(FE_NOMATCH);

	while (*s1 == ' ')
		s1++;
	while (*s2 == ' ')
		s2++;
	while (*s1 != 0) {
		if (tolower((unsigned char)(*s1)) != tolower((unsigned char)(*s2)))
			return(FE_NOMATCH);
		s1++;
		s2++;
		while (*s1 == ' ')
			s1++;
		while (*s2 == ' ')
			s2++;
	}
	if (*s2 != 0)
		return(FE_NOMATCH);
	return(FE_SUCCESS);
}

enum firetalk_error   toc_isprint(const int c) {
	if ((c >= 0) && (c <= 255) && (isprint(c) || (c >= 160)))
		return(FE_SUCCESS);
	return(FE_INVALIDFORMAT);
}

client_t toc_create_handle() {
	client_t c;

	c = safe_malloc(sizeof(struct s_toc_connection));

	c->nickname = NULL;
	c->room_head = NULL;
	c->infoget_head = NULL;
	c->lasttalk = time(NULL);
	c->lastidle = 0;
	c->passchange = 0;
	c->local_sequence = 0;
	c->lastsend = 0;
	c->remote_sequence = 0;
	c->online = 0;
	c->profar = NULL;
	c->profc = 0;
	c->profstr = NULL;
	c->proflen = 0;
	c->ectqar = NULL;
	c->ectqc = 0;
	c->buddybuflen = 0;

	return c;
}

void toc_destroy_handle(client_t c) {
	(void) toc_internal_disconnect(c,FE_USERDISCONNECT);
	free(c);
}

enum firetalk_error   toc_disconnect(client_t c) {
	return toc_internal_disconnect(c,FE_USERDISCONNECT);
}

enum firetalk_error   toc_signon(client_t c, const char *const username) {
	struct s_firetalk_handle *conn;

	/* fill & send the flap signon packet */

	conn = firetalk_find_handle(c);

	c->lasttalk = time(NULL);
	c->connectstate = 0;
	c->gotconfig = 0;
	c->nickname = safe_strdup(username);

	/* send the signon string to indicate that we're speaking FLAP here */

#ifdef DEBUG_ECHO
	toc_echof(c, "signon", "frame=0, length=%i, value=[%s]\n", strlen(SIGNON_STRING), SIGNON_STRING);
#endif
	firetalk_internal_send_data(conn,SIGNON_STRING,strlen(SIGNON_STRING),0);

	return FE_SUCCESS;
}

enum firetalk_error   toc_im_add_buddy(client_t c, const char *const _nickname) {
	char    *name = toc_quote(_nickname, 1);
	int     slen = strlen(name);

	if ((c->buddybuflen+slen+1) >= sizeof(c->buddybuf)) {
		toc_send_printf(c, "toc_add_buddy%S", c->buddybuf);
		c->buddybuflen = 0;
	}
	if (c->online == 1) {
		strcpy(c->buddybuf+c->buddybuflen, " ");
		strcpy(c->buddybuf+c->buddybuflen+1, name);
		c->buddybuflen += slen+1;
	}
	return(FE_SUCCESS);
}

enum firetalk_error   toc_im_remove_buddy(client_t c, const char *const nickname) {
	return(toc_send_printf(c, "toc_remove_buddy %s", nickname));
}

enum firetalk_error   toc_im_add_deny(client_t c, const char *const nickname) {
	return(toc_send_printf(c, "toc_add_deny %s", nickname));
}

enum firetalk_error   toc_im_remove_deny(client_t c, const char *const nickname) {
	toc_send_printf(c, "toc_add_permit %s", nickname);
	return(toc_send_printf(c, "toc_add_deny"));
}

enum firetalk_error   toc_save_config(client_t c) {
	char data[2048];
	struct s_firetalk_handle *conn;
	struct s_firetalk_buddy *buddyiter;
	struct s_firetalk_deny *denyiter;

	conn = firetalk_find_handle(c);

	safe_strncpy(data,"m 4\ng Buddies\n",2048);
	buddyiter = conn->buddy_head;
	while (buddyiter) {
		if (strlen(data) > 2000)
			return FE_PACKETSIZE;
		safe_strncat(data,"b ",2048);
		safe_strncat(data,buddyiter->nickname,2048);
		safe_strncat(data,"\n",2048);
		buddyiter = buddyiter->next;
	}
	denyiter = conn->deny_head;
	while (denyiter) {
		if (strlen(data) > 2000)
			return FE_PACKETSIZE;
		safe_strncat(data,"d ",2048);
		safe_strncat(data,denyiter->nickname,2048);
		safe_strncat(data,"\n",2048);
		denyiter = denyiter->next;
	}
	return toc_send_printf(c,"toc_set_config %s",data);
}

enum firetalk_error   toc_im_upload_buddies(client_t c) {
	char data[2048];
	struct s_firetalk_buddy *buddyiter;
	struct s_firetalk_handle *fchandle;
	unsigned short length; 

	fchandle = firetalk_find_handle(c);


	safe_strncpy(&data[TOC_HEADER_LENGTH],"toc_add_buddy",(size_t) 2048 - TOC_HEADER_LENGTH);

	if (fchandle->buddy_head == NULL) {
		safe_strncat(&data[TOC_HEADER_LENGTH]," ",(size_t) 2048 - TOC_HEADER_LENGTH);
		safe_strncat(&data[TOC_HEADER_LENGTH],toc_quote("naimhelp",1),(size_t) 2048 - TOC_HEADER_LENGTH);
	}

	buddyiter = fchandle->buddy_head;
	while (buddyiter) {
		safe_strncat(&data[TOC_HEADER_LENGTH]," ",(size_t) 2048 - TOC_HEADER_LENGTH);
		safe_strncat(&data[TOC_HEADER_LENGTH],toc_quote(buddyiter->nickname,1),(size_t) 2048 - TOC_HEADER_LENGTH);
		if (strlen(&data[TOC_HEADER_LENGTH]) > 2000) {
			length = toc_fill_header((unsigned char *)data,SFLAP_FRAME_DATA,++c->local_sequence,strlen(&data[TOC_HEADER_LENGTH])+1);

#ifdef DEBUG_ECHO
			toc_echo_send(c, "im_upload_buddies", data, length);
#endif
			firetalk_internal_send_data(fchandle,data,length,0);
			safe_strncpy(&data[TOC_HEADER_LENGTH],"toc_add_buddy",(size_t) 2048 - TOC_HEADER_LENGTH);
		}
		buddyiter = buddyiter->next;
	}
	length = toc_fill_header((unsigned char *)data,SFLAP_FRAME_DATA,++c->local_sequence,strlen(&data[TOC_HEADER_LENGTH])+1);

#ifdef DEBUG_ECHO
	toc_echo_send(c, "im_upload_buddies", data, length);
#endif
	firetalk_internal_send_data(fchandle,data,length,0);
	return FE_SUCCESS;
}

enum firetalk_error   toc_im_upload_denies(client_t c) {
	char data[2048];
	struct s_firetalk_deny *denyiter;
	unsigned short length; 
	struct s_firetalk_handle *fchandle;

	fchandle = firetalk_find_handle(c);

	if (fchandle->deny_head == NULL)
		return FE_SUCCESS;

	safe_strncpy(&data[TOC_HEADER_LENGTH],"toc_add_deny",(size_t) 2048 - TOC_HEADER_LENGTH);
	denyiter = fchandle->deny_head;
	while (denyiter) {
		safe_strncat(&data[TOC_HEADER_LENGTH]," ",(size_t) 2048 - TOC_HEADER_LENGTH);
		safe_strncat(&data[TOC_HEADER_LENGTH],toc_quote(denyiter->nickname,0),(size_t) 2048 - TOC_HEADER_LENGTH);
		if (strlen(&data[TOC_HEADER_LENGTH]) > 2000) {
			length = toc_fill_header((unsigned char *)data,SFLAP_FRAME_DATA,++c->local_sequence,strlen(&data[TOC_HEADER_LENGTH])+1);

#ifdef DEBUG_ECHO
			toc_echo_send(c, "im_upload_denies", data, length);
#endif
			firetalk_internal_send_data(fchandle,data,length,0);
			safe_strncpy(&data[TOC_HEADER_LENGTH],"toc_add_deny",(size_t) 2048 - TOC_HEADER_LENGTH);
		}
		denyiter = denyiter->next;
	}
	length = toc_fill_header((unsigned char *)data,SFLAP_FRAME_DATA,++c->local_sequence,strlen(&data[TOC_HEADER_LENGTH])+1);

#ifdef DEBUG_ECHO
	toc_echo_send(c, "im_upload_denies", data, length);
#endif
	firetalk_internal_send_data(fchandle,data,length,0);
	return FE_SUCCESS;
}

enum firetalk_error   toc_im_send_message(client_t c, const char *const dest, const char *const message, const int auto_flag) {
	if ((auto_flag == 0) && (toc_compare_nicks(dest,c->nickname) == FE_NOMATCH))
		c->lasttalk = time(NULL);

	if (strcasecmp(dest, ":RAW") == 0) {
		if (auto_flag != 1)
			return(toc_send_printf(c, "%S", message));
		else
			return(FE_SUCCESS);
	}

	if (auto_flag == 1)
		return toc_send_printf(c,"toc_send_im %s %s auto",dest,aim_interpolate_variables(message,dest));
	else {
		int     i;

		for (i = 0; i < c->ectqc; i++)
			if (toc_compare_nicks(dest, c->ectqar[i].dest) == 0) {
				char    *newstr = malloc(strlen(message) + strlen(c->ectqar[i].ect) + 1);

				if (newstr != NULL) {
					enum firetalk_error   fte;

					strcpy(newstr, message);
					strcat(newstr, c->ectqar[i].ect);
					fte = toc_send_printf(c, "toc_send_im %s %s",
						dest, newstr);
					free(newstr);

					free(c->ectqar[i].dest);
					free(c->ectqar[i].ect);
					memmove(c->ectqar+i, c->ectqar+i+1,
						c->ectqc-i-1);
					c->ectqc--;
					c->ectqar = realloc(c->ectqar, 
						(c->ectqc)*sizeof(*(c->ectqar)));

					return(fte);
				}
			}
		return(toc_send_printf(c, "toc_send_im %s %s", 
			dest, message));
	}
}

enum firetalk_error   toc_im_send_action(client_t c, const char *const dest, const char *const message, const int auto_flag) {
	char tempbuf[2048]; 

	if (strlen(message) > 2042)
		return FE_PACKETSIZE;

	safe_strncpy(tempbuf,"/me ",2048);
	safe_strncat(tempbuf,message,2048);

	if (auto_flag == 0)
		c->lasttalk = time(NULL);
	
	if (auto_flag == 1)
		return toc_send_printf(c,"toc_send_im %s %s auto",dest,tempbuf);
	else
		return toc_send_printf(c,"toc_send_im %s %s",dest,tempbuf);
}

enum firetalk_error   toc_get_info(client_t c, const char *const nickname) {
	safe_strncpy(lastinfo,nickname,(size_t) TOC_USERNAME_MAXLEN + 1);
	return toc_send_printf(c,"toc_get_info %s",nickname);
}

enum firetalk_error   toc_set_info(client_t c, const char *const info) {
	if (c->proflen > 0) {
		size_t  infolen = strlen(info);
		char    *newstr;

		if (((infolen = strlen(info)) + c->proflen) >= 1024) {
			firetalk_callback_error(c, FE_MESSAGETRUNCATED, NULL, "Profile too long");
			infolen = 1024 - c->proflen - 1;
		}

		newstr = malloc(infolen + c->proflen + 1);
		if (newstr != NULL) {
			enum firetalk_error   fte;

			strncpy(newstr, info, infolen);
			strcpy(newstr+infolen, c->profstr);
			fte = toc_send_printf(c, "toc_set_info %s", newstr);
			free(newstr);
			return(fte);
		}
	}
	return(toc_send_printf(c, "toc_set_info %s", info));
}

enum firetalk_error toc_set_away(client_t c, const char *const message) {
#ifdef DEBUG_ECHO
	toc_echof(c, "set_away", "message=%#p \"%s\", auto_flag=%i\n", message, message, auto_flag);
#endif
	if (message)
		return toc_send_printf(c,"toc_set_away %s",message);
	else
		return toc_send_printf(c,"toc_set_away");
}

enum firetalk_error   toc_set_nickname(client_t c, const char *const nickname) {
	return toc_send_printf(c,"toc_format_nickname %s",nickname);
}

enum firetalk_error   toc_set_password(client_t c, const char *const oldpass, const char *const newpass) {
	c->passchange++;
	return toc_send_printf(c,"toc_change_passwd %s %s",oldpass,newpass);
}

enum firetalk_error   toc_im_evil(client_t c, const char *const who) {
	return toc_send_printf(c,"toc_evil %s norm",who);
}

static struct {
	enum firetalk_error fte;
	unsigned char   hastarget:1;
	const char      *str;
} toc2_errors[] = {
  /* General Errors */
	/* 900 */ {     FE_UNKNOWN,             0, NULL },
	/* 901 */ {     FE_USERUNAVAILABLE,     1, NULL },
			/* TOC1 $1 not currently available */
			/* TOC2 User Not Logged In
				You can only send Instant Messanges to, or Chat with, buddies who are currently signed on. You need to wait and try again later when your buddy is signed on.
			*/
	/* 902 */ {     FE_USERUNAVAILABLE,     1, "You have either tried to warn someone you haven't sent a message to yet, or you have already warned them too many times today." },
			/* TOC1 Warning of $1 not currently available */
			/* TOC2 Warning Not Allowed
				You have either tried to warn someone you haven't sent an Instant Message to yet; or, you have already warned them too many times today.
			*/
	/* 903 */ {     FE_TOOFAST,             0, "You are sending commands too fast." },
			/* TOC1 A message has been dropped, you are exceeding the server speed limit */
			/* TOC2 Server Limit Exceeded
				If you send messages too fast, you will exceed the server's capacity to handle them. Please try to send your message again.
			*/
	/* 904 */ {     FE_UNKNOWN,             0, NULL },
	/* 905 */ {     FE_UNKNOWN,             0, NULL },
	/* 906 */ {     FE_UNKNOWN,             0, NULL },
	/* 907 */ {     FE_UNKNOWN,             0, NULL },
	/* 908 */ {     FE_UNKNOWN,             0, NULL },
	/* 909 */ {     FE_UNKNOWN,             0, NULL },


  /* Admin Errors */
	/* 910 */ {     FE_UNKNOWN,             0, NULL },
	/* 911 */ {     FE_BADUSER,             0, NULL },
	/* 912 */ {     FE_UNKNOWN,             0, NULL },
	/* 913 */ {     FE_UNKNOWN,             0, NULL },
	/* 914 */ {     FE_UNKNOWN,             0, NULL },
	/* 915 */ {     FE_UNKNOWN,             0, NULL },
	/* 916 */ {     FE_UNKNOWN,             0, NULL },
	/* 917 */ {     FE_UNKNOWN,             0, NULL },
	/* 918 */ {     FE_UNKNOWN,             0, NULL },
	/* 919 */ {     FE_UNKNOWN,             0, NULL },


  /* Unknown */
	/* 920 */ {     FE_UNKNOWN,             0, NULL },
	/* 921 */ {     FE_UNKNOWN,             0, NULL },
	/* 922 */ {     FE_UNKNOWN,             0, NULL },
	/* 923 */ {     FE_UNKNOWN,             0, NULL },
	/* 924 */ {     FE_UNKNOWN,             0, NULL },
	/* 925 */ {     FE_UNKNOWN,             0, NULL },
	/* 926 */ {     FE_UNKNOWN,             0, NULL },
	/* 927 */ {     FE_UNKNOWN,             0, NULL },
	/* 928 */ {     FE_UNKNOWN,             0, NULL },
	/* 929 */ {     FE_UNKNOWN,             0, NULL },


  /* Buddy List Errors */
	/* 930 */ {     FE_UNKNOWN,             0, "The Buddy List server is unavailable at this time. Wait a few minutes, and try to sign on again." },
			/* TOC1 UNDOCUMENTED */
			/* TOC2 Buddy List Server is Unavailable
				The Buddy List server is unavailable at this time, and your AIM Express (TM) connection will be closed.
				Wait a few minutes, and try to sign on again.
			*/
	/* 931 */ {     FE_UNKNOWN,             0, NULL },
	/* 932 */ {     FE_UNKNOWN,             0, NULL },
	/* 933 */ {     FE_UNKNOWN,             0, NULL },
	/* 934 */ {     FE_UNKNOWN,             0, NULL },
	/* 935 */ {     FE_UNKNOWN,             0, NULL },
	/* 936 */ {     FE_UNKNOWN,             0, NULL },
	/* 937 */ {     FE_UNKNOWN,             0, NULL },
	/* 938 */ {     FE_UNKNOWN,             0, NULL },
	/* 939 */ {     FE_UNKNOWN,             0, NULL },


  /* Unknown */
	/* 940 */ {     FE_UNKNOWN,             0, NULL },
	/* 941 */ {     FE_UNKNOWN,             0, NULL },
	/* 942 */ {     FE_UNKNOWN,             0, NULL },
	/* 943 */ {     FE_UNKNOWN,             0, NULL },
	/* 944 */ {     FE_UNKNOWN,             0, NULL },
	/* 945 */ {     FE_UNKNOWN,             0, NULL },
	/* 946 */ {     FE_UNKNOWN,             0, NULL },
	/* 947 */ {     FE_UNKNOWN,             0, NULL },
	/* 948 */ {     FE_UNKNOWN,             0, NULL },
	/* 949 */ {     FE_UNKNOWN,             0, NULL },


  /* Chat Errors */
	/* 950 */ {     FE_ROOMUNAVAILABLE,     1, NULL },
			/* TOC1 Chat in $1 is unavailable. */
			/* TOC2 User Unavailable
				You can only Chat with buddies who are currently signed on and available. AOL Instant Messenger users have the option to put up an "Unavailable" notice, indicating that they are away from their computer or simply too busy to answer messages.
				You need to wait and try again later when your buddy is available.
			*/
	/* 951 */ {     FE_UNKNOWN,             0, NULL },
	/* 952 */ {     FE_UNKNOWN,             0, NULL },
	/* 953 */ {     FE_UNKNOWN,             0, NULL },
	/* 954 */ {     FE_UNKNOWN,             0, NULL },
	/* 955 */ {     FE_UNKNOWN,             0, NULL },
	/* 956 */ {     FE_UNKNOWN,             0, NULL },
	/* 957 */ {     FE_UNKNOWN,             0, NULL },
	/* 958 */ {     FE_UNKNOWN,             0, NULL },
	/* 959 */ {     FE_UNKNOWN,             0, NULL },


  /* IM & Info Errors */
	/* 960 */ {     FE_TOOFAST,             1, "You are sending messages too fast." },
			/* TOC1 You are sending message too fast to $1 */
			/* TOC2 Server Limit Exceeded
				Sending messages too fast will exceed the server's capacity to handle them.
			*/
	/* 961 */ {     FE_INCOMINGERROR,       1, "You missed a message because it was too big." },
			/* TOC1 You missed an im from $1 because it was too big. */
			/* TOC2 Server Limit Exceeded
				The sender sent their messages too fast, exceeding the server's capacity to handle them.
			*/
	/* 962 */ {     FE_INCOMINGERROR,       1, "You missed a message because it was sent too fast." },
			/* TOC1 You missed an im from $1 because it was sent too fast. */
			/* TOC2 Server Limit Exceeded
				The Instant Messenging system is designed to accommodate normal conversions, consisting of a few lines of text at a time. Larger messages, like a magazine article or a full-length letter, might get dropped. Please ask the sender to send their message through e-mail instead.
			*/
	/* 963 */ {     FE_UNKNOWN,             0, NULL },
	/* 964 */ {     FE_UNKNOWN,             0, NULL },
	/* 965 */ {     FE_UNKNOWN,             0, NULL },
	/* 966 */ {     FE_UNKNOWN,             0, NULL },
	/* 967 */ {     FE_UNKNOWN,             0, NULL },
	/* 968 */ {     FE_UNKNOWN,             0, NULL },
	/* 969 */ {     FE_UNKNOWN,             0, NULL },


  /* Dir Errors */
	/* 970 */ {     FE_SUCCESS,             0, NULL },
			/* TOC1 Failure */
			/* TOC2 UNDOCUMENTED */
	/* 971 */ {     FE_USERINFOUNAVAILABLE, 0, "Too many matches." },
			/* TOC1 Too many matches */
			/* TOC2 UNDOCUMENTED */
	/* 972 */ {     FE_USERINFOUNAVAILABLE, 0, "Need more qualifiers." },
			/* TOC1 Need more qualifiers */
			/* TOC2 UNDOCUMENTED */
	/* 973 */ {     FE_USERINFOUNAVAILABLE, 0, "Directory service unavailable." },
			/* TOC1 Dir service temporarily unavailable */
			/* TOC2 UNDOCUMENTED */
	/* 974 */ {     FE_USERINFOUNAVAILABLE, 0, "Email lookup restricted." },
			/* TOC1 Email lookup restricted */
			/* TOC2 UNDOCMENTED */
	/* 975 */ {     FE_USERINFOUNAVAILABLE, 0, "Keyword ignored." },
			/* TOC1 Keyword Ignored */
			/* TOC2 UNDOCUMENTED */
	/* 976 */ {     FE_USERINFOUNAVAILABLE, 0, "No keywords." },
			/* TOC1 No Keywords */
			/* TOC2 UNDOCUMENTED */
	/* 977 */ {     FE_SUCCESS,             0, NULL },
			/* TOC1 Language not supported */
			/* TOC2 UNDOCUMENTED */
	/* 978 */ {     FE_USERINFOUNAVAILABLE, 0, "Country not supported." },
			/* TOC1 Country not supported */
			/* TOC2 UNDOCUMENTED */
	/* 979 */ {     FE_USERINFOUNAVAILABLE, 0, "Failure unknown." },
			/* TOC1 Failure unknown $1 */
			/* TOC2 UNDOCUMENTED */


  /* Auth errors */
	/* 980 */ {     FE_BADUSERPASS,         0, "The Screen Name or Password was incorrect. Passwords are case sensitive, so make sure your caps lock key isn't on." },
			/* TOC1 Incorrect nickname or password. */
			/* TOC2 Incorrect Screen Name or Password
				The Screen Name or Password was incorrect. Passwords are case sensitive, so make sure your caps lock key isn't on.
				AIM Express (TM) uses your AIM Screen Name and Password, not your AOL Password. If you need an AIM password please visit AOL Instant Messenger.
				Forgot your password? Click here.
			*/
	/* 981 */ {     FE_SERVER,              0, "The service is unavailable currently. Please try again in a few minutes." },
			/* TOC1 The service is temporarily unavailable. */
			/* TOC2 Service Offline
				The service is unavailable currently. Please try again in a few minutes.
			*/
	/* 982 */ {     FE_BLOCKED,             0, "Your warning level is too high to sign on." },
			/* TOC1 Your warning level is currently too high to sign on. */
			/* TOC2 Warning Level too High
				Your warning level is current too high to sign on. You will need to wait between a few minutes and several hours before you can log back in.
			*/
	/* 983 */ {     FE_BLOCKED,             0, "You have been connected and disconnecting too frequently. Wait 10 minutes and try again. If you continue to try, you will need to wait even longer." },
			/* TOC1 You have been connecting and disconnecting too frequently.  Wait 10 minutes and try again. If you continue to try, you will need to wait even longer. */
			/* TOC2 Connecting too Frequently
				You have been connecting and disconnecting too frequently. You will need to wait around 10 minutes before you will be allowed to sign on again.
			*/
	/* 984 */ {     FE_UNKNOWN,             0, NULL },
	/* 985 */ {     FE_UNKNOWN,             0, NULL },
	/* 986 */ {     FE_UNKNOWN,             0, NULL },
	/* 987 */ {     FE_UNKNOWN,             0, NULL },
	/* 988 */ {     FE_UNKNOWN,             0, NULL },
	/* 989 */ {     FE_UNKNOWN,             0, "An unknown signon error has occured. Please wait 10 minutes and try again." },
			/* TOC1 An unknown signon error has occurred $1 */
			/* TOC2 Unknown Error
				An unknown signon error has occured. Please wait 10 minutes and try again.
			*/


  /* Client Errors */
	/* 990 */ {     FE_UNKNOWN,             0, NULL },
	/* 991 */ {     FE_UNKNOWN,             0, NULL },
	/* 992 */ {     FE_UNKNOWN,             0, NULL },
	/* 993 */ {     FE_UNKNOWN,             0, NULL },
	/* 994 */ {     FE_UNKNOWN,             0, NULL },
	/* 995 */ {     FE_UNKNOWN,             0, NULL },
	/* 996 */ {     FE_UNKNOWN,             0, NULL },
	/* 997 */ {     FE_UNKNOWN,             0, NULL },
	/* 998 */ {     FE_UNKNOWN,             0, "The service believes you are using an outdated client; this is possibly an attempt to block third-party programs such as the one you are using. Check your software's web site for an updated version." },
			/* TOC1 UNDOCUMENTED */
			/* TOC2 AIM Express (TM) Update
				Your browser is using a cached version of Quick Buddy that is now outdated. To remedy this, simply quit or close your browser and re-launch it. The next time you load Quick Buddy, it will be automatically updated.
			*/
	/* 999 */ {     FE_UNKNOWN,             0, NULL },
};

static struct {
	const char *name;
	const int val;
} toc_firstcaps[] = {
	{ "WinAIM_COMPAT",      0x0000 },
	{ "ICQ2Go_COMPAT_AIM",  0x0002 },
	{ "ICQ2Go_COMPAT_ICQ",  0x0210 },
};

static struct {
	const char *name;
	const int val;
} toc_barts[] = {
	{ "BUDDY_ICON_SMALL",   0 },
	{ "BUDDY_ICON",         1 },
	{ "STATUS_TEXT",        2 },
	{ "ARRIVE_SOUND",       3 },
	{ "DEPART_SOUND",       96 },
	{ "IM_BACKGROUND",      128 },
	{ "IM_CHROME",          129 },
	{ "IM_SKIN",            130 },
	{ "IM_SOUND",           131 },
	{ "BL_BACKGROUND",      256 },
	{ "BL_IMAGE",           257 },
	{ "BL_SKIN",            258 },
	{ "SMILEY_SET",         1024 },
	{ "MAIL_STATIONERY",    1025 },
};

static char **toc_parse_args2(char *str, const int maxargs, const char sep) {
	static char *args[256];
	int     curarg = 0;
	char    *colon;

	while ((curarg < (maxargs-1)) && (curarg < 256)
		&& ((colon = strchr(str, sep)) != NULL)) {
		args[curarg++] = str;
		*colon = 0;
		str = colon+1;
	}
	args[curarg++] = str;
	args[curarg] = NULL;
	return(args);
}

static const char *toc_make_fake_cap(const unsigned char *const str, const int len) {
	static char buf[sizeof("FFFFFFFF-cccc-dddd-eeee-ffffgggghhhh")];
	const int ar[] = { 9, 11, 14, 16, 19, 21, 24, 26, 28, 30, 32, 34 };
	const int maxlen = 12;
	int     i;

	strcpy(buf, "FFFFFFFF-0000-0000-0000-000000000000");
	for (i = 0; (i < len) && (i < maxlen); i++) {
		char    b[3];

		sprintf(b, "%02X", str[i]);
		memcpy(buf+ar[i], b, 2);
	}
	return(buf);
}

enum firetalk_error toc_got_data_connecting(client_t c, unsigned char *buffer, unsigned short *bufferpos) {
	char data[TOC_SERVERSEND_MAXLEN - TOC_HEADER_LENGTH + 1];
	char password[128];
	enum firetalk_error r;
	unsigned short length;
	char *arg0;
	char **args;
	char *tempchr1;
	firetalk_t fchandle;

got_data_connecting_start:
	
	r = toc_find_packet(c, buffer, bufferpos, data, (c->connectstate==0)?SFLAP_FRAME_SIGNON:SFLAP_FRAME_DATA, &length);
	if (r == FE_NOTFOUND)
		return(FE_SUCCESS);
	else if (r != FE_SUCCESS)
		return(r);

	switch (c->connectstate) {
	  case 0: /* we're waiting for the flap version number */
		if (length != TOC_HOST_SIGNON_LENGTH) {
			firetalk_callback_connectfailed(c, FE_PACKETSIZE, "Host signon length incorrect");
			return(FE_PACKETSIZE);
		}
		if ((data[0] != 0) || (data[1] != 0) || (data[2] != 0) || (data[3] != 1)) {
			firetalk_callback_connectfailed(c, FE_VERSION, NULL);
			return(FE_VERSION);
		}
#if 0
		srand((unsigned int) time(NULL));
		c->local_sequence = (unsigned short)1+(unsigned short)(65536.0*rand()/(RAND_MAX+1.0));
#else
		c->local_sequence = 31337;
#endif

		length = toc_fill_header((unsigned char *)data, SFLAP_FRAME_SIGNON, ++c->local_sequence, toc_fill_signon((unsigned char *)&data[TOC_HEADER_LENGTH], c->nickname));

		fchandle = firetalk_find_handle(c);
#ifdef DEBUG_ECHO
		toc_echo_send(c, "got_data_connecting", data, length);
#endif
		firetalk_internal_send_data(fchandle, data, length, 0);

		firetalk_callback_needpass(c, password, sizeof(password));

		c->connectstate = 1;
		{
			int     sn = tolower(c->nickname[0]) - 'a' + 1,
				pw = tolower(password[0]) - 'a' + 1,
				A = sn*7696 + 738816,
				B = sn*746512,
				C = pw*A,
				magic = C - A + B + 71665152;

			if (!isdigit(c->nickname[0]))                   /* AIM Express */
			  r = toc_send_printf(c, "toc2_login "
				 "login.oscar.aol.com"  // authorizer host      [login.oscar.aol.com]
				" 29999"                // authorizer port      [29999]
				" %s"                   // username
				" %S"                   // roasted, armored password
				" English"              // language             [English]
				" %s"                   // client version
				" 160"                  // unknown number       [160]
				" %S"                   // country code         [US]
				" %s"                   // unknown string       [""]
				" %s"                   // unknown string       [""]
				" 3"                    // unknown number       [3]
				" 0"                    // unknown number       [0]
				" 30303"                // unknown number       [30303]
				" -kentucky"            // unknown flag         [-kentucky]
				" -utf8"                // unknown flag         [-utf8]
				" -preakness"           // this enables us to talk to ICQ users
				" %i",                  // magic number based on username and password
					c->nickname,
					toc_hash_password(password),
					PACKAGE_NAME ":" PACKAGE_VERSION ":contact " PACKAGE_BUGREPORT,
					"US",
					"",
					"",
					magic);
			else                                            /* ICQ2Go */
			  r = toc_send_printf(c, "toc2_login "
				 "login.icq.com"        // authorizer host      [login.icq.com]
				" 5190"                 // authorizer port      [5190]
				" %S"                   // username
				" %S"                   // roasted, armored password
				" en"                   // language             [en]
				" %s"                   // client version
				" 135"                  // unknown number       [135]
				" %s"                   // country code         ["US"]
				" %s"                   // unknown string       [""]
				" %s"                   // unknown string       [""]
				" 30"                   // unknown number       [30]
				" 2"                    // unknown number       [2]
				" 321"                  // unknown number       [321]
				" -utf8"                // unknown flag         [-utf8]
				" -preakness"           // this enables us to talk to ICQ users
				" %i",                  // magic number based on username and password
					c->nickname,
					toc_hash_password(password),
					PACKAGE_NAME ":" PACKAGE_VERSION ":contact " PACKAGE_BUGREPORT,
					"US",
					"",
					"",
					magic);
		}
		if (r != FE_SUCCESS) {
			firetalk_callback_connectfailed(c,r,NULL);
			return(r);
		}
		break;
	  case 1:
		arg0 = toc_get_arg0(data);
		if (strcmp(arg0, "SIGN_ON") != 0) {
			if (strcmp(arg0, "ERROR") == 0) {
				args = toc_parse_args2(data, 3, ':');
				if (args[1] != NULL) {
					int     err = atoi(args[1]);

					if ((err >= 900) && (err <= 999)) {
						err -= 900;
						firetalk_callback_connectfailed(c, toc2_errors[err].fte, toc2_errors[err].str);
						return(toc2_errors[err].fte);
					}
				}
			}
			firetalk_callback_connectfailed(c, FE_UNKNOWN, NULL);
			return(FE_UNKNOWN);
		}
		c->connectstate = 2;
		break;
	  case 2:
	  case 3:
		arg0 = toc_get_arg0(data);
		if (arg0 == NULL)
			return(FE_SUCCESS);
		if (strcmp(arg0, "NICK") == 0) {
			/* NICK:<Nickname> */

			args = toc_parse_args2(data, 2, ':');
			if (args[1]) {
				free(c->nickname);
				c->nickname = strdup(args[1]);
				if (c->nickname == NULL)
					abort();
			}
			c->connectstate = 3;
		} else if (strcmp(arg0, "CONFIG2") == 0) {
			/* CONFIG2:<config> */
			char    *nl, *curgroup = strdup("Saved buddy");

			fchandle = firetalk_find_handle(c);
			args = toc_parse_args2(data, 2, ':');
			if (!args[1]) {
				firetalk_callback_connectfailed(c, FE_INVALIDFORMAT, "CONFIG2");
				return(FE_INVALIDFORMAT);
			}
			tempchr1 = args[1];
			c->permit_mode = 0;
			while ((nl = strchr(tempchr1, '\n'))) {
				*nl = 0;

				if (tempchr1[1] == ':') {
					/* b:ACCOUNT:REAL NAME:?:CELL PHONE SLOT NUMBER:w:NOTES */

					args = toc_parse_args2(tempchr1, 4, ':');

					switch (args[0][0]) {
					  case 'g':     /* Buddy Group (All Buddies until the next g or the end of config are in this group.) */
						free(curgroup);
						curgroup = strdup(args[1]);
						break;
					  case 'b':     /* A Buddy */
					  case 'a':     /* another kind of buddy */
						if (strcmp(curgroup, "Mobile Device") != 0)
							firetalk_im_add_buddy(fchandle, args[1]/*, curgroup*/);
						break;
					  case 'p':     /* Person on permit list */
						toc_send_printf(c, "toc_add_permit %s", args[1]);
						break;
					  case 'd':     /* Person on deny list */
						firetalk_im_internal_add_deny(fchandle, args[1]);
						break;
					  case 'm':     /* Permit/Deny Mode.  Possible values are 1 - Permit All 2 - Deny All 3 - Permit Some 4 - Deny Some */
						c->permit_mode = atoi(args[1]);
						break;
					}
				}
				tempchr1 = nl+1;
			}
			free(curgroup);

			if ((c->permit_mode < 1) || (c->permit_mode > 5))
				c->permit_mode = 4;

			toc_send_printf(c, "toc2_set_pdmode %i", c->permit_mode);
			c->gotconfig = 1;
		} else {
			firetalk_callback_connectfailed(c, FE_WEIRDPACKET, data);
			return(FE_WEIRDPACKET);
		}

		if ((c->gotconfig == 1) && (c->connectstate == 3)) {
#ifdef ENABLE_GETREALNAME
			char    realname[128];
#endif

			/* ask the client to handle its init */
			firetalk_callback_doinit(c, c->nickname);
			if (toc_im_upload_buddies(c) != FE_SUCCESS) {
				firetalk_callback_connectfailed(c, FE_PACKET, "Error uploading buddies");
				return(FE_PACKET);
			}
			if (toc_im_upload_denies(c) != FE_SUCCESS) {
				firetalk_callback_connectfailed(c, FE_PACKET, "Error uploading denies");
				return(FE_PACKET);
			}
			r = toc_send_printf(c, "toc_init_done");
			if (r != FE_SUCCESS) {
				firetalk_callback_connectfailed(c, r, "Finalizing initialization");
				return(r);
			}

			r = toc_send_printf(c, "toc_set_caps %S %S",
				"094613494C7F11D18222444553540000",
				toc_make_fake_cap(PACKAGE_STRING, strlen(PACKAGE_STRING)));
			if (r != FE_SUCCESS) {
				firetalk_callback_connectfailed(c, r, "Setting capabilities");
				return(r);
			}

#ifdef ENABLE_GETREALNAME
			firetalk_getrealname(c, realname, sizeof(realname));
			if (*realname != 0) {
				char    *first, *mid, *last;

				first = strtok(realname, " ");
				mid = strtok(NULL, " ");
				last = strtok(NULL, " ");

				if (mid == NULL)
					mid = "";
				if (last == NULL) {
					last = mid;
					mid = "";
				}

				/* first name:middle name:last name:maiden name:city:state:country:email:allow web searches */
				r = toc_send_printf(c, "toc_set_dir \"%S:%S:%S\"", first, mid, last);
				if (r != FE_SUCCESS) {
					firetalk_callback_connectfailed(c, r, "Setting directory information");
					return(r);
				}
			}
#endif

			firetalk_callback_connected(c);
			return(FE_SUCCESS);
		}
		break;
	}
	goto got_data_connecting_start;
}

enum firetalk_error toc_got_data(client_t c, unsigned char *buffer, unsigned short *bufferpos) {
	char *tempchr1;
	char data[TOC_SERVERSEND_MAXLEN - TOC_HEADER_LENGTH + 1];
	char *arg0;
	char **args;
	enum firetalk_error r;
	unsigned short l;

  got_data_start:
	r = toc_find_packet(c, buffer, bufferpos, data, SFLAP_FRAME_DATA, &l);
	if (r == FE_NOTFOUND)
		return(FE_SUCCESS);
	else if (r != FE_SUCCESS)
		return(r);

	arg0 = toc_get_arg0(data);
	if (arg0 == NULL)
		return(FE_SUCCESS);
	if (strcmp(arg0, "ERROR") == 0) {
		/* ERROR:<Error Code>:Var args */
		int     err;

		args = toc_parse_args2(data, 3, ':');
		if (args[1] == NULL) {
			toc_internal_disconnect(c, FE_INVALIDFORMAT);
			return(FE_INVALIDFORMAT);
		}
		err = atoi(args[1]);
		if ((err < 900) || (err > 999)) {
			toc_internal_disconnect(c, FE_INVALIDFORMAT);
			return(FE_INVALIDFORMAT);
		}
		if (err == 911) {
			/* TOC1 Error validating input */
			/* TOC2 UNDOCUMENTED */
			if (c->passchange != 0) {
				c->passchange--;
				firetalk_callback_error(c, FE_NOCHANGEPASS, NULL, NULL);
				goto got_data_start;
			}
		}
		err -= 900;
		if (toc2_errors[err].fte != FE_SUCCESS)
			firetalk_callback_error(c, toc2_errors[err].fte,
				toc2_errors[err].hastarget?args[2]:NULL,
				toc2_errors[err].str?toc2_errors[err].str:(toc2_errors[err].fte == FE_UNKNOWN)?args[1]:NULL);
	} else if (strcmp(arg0, "IM_IN_ENC2") == 0) {
		/* IM_IN:<Source User>:<Auto Response T/F?>:<Message> */
		/* 1 source
		** 2 'T'=auto, 'F'=normal
		** 3 UNKNOWN TOGGLE
		** 4 UNKNORN TOGGLE
		** 5 user class
		** 6 UNKNOWN TOGGLE from toc2_send_im_enc arg 2
		** 7 encoding [toc2_send_im_enc arg 3] "A"=ASCII "L"=Latin1 "U"=UTF8
		** 8 language, for example "en"
		** 9 message
		** 
		** Cell phone   IM_IN_ENC2:+number:F:F:F: C,:F:L:en:message
		** Spam bot     IM_IN_ENC2:buddysn:F:F:F: U,:F:A:en:message
		** naim 0.11.6  IM_IN_ENC2:buddysn:F:F:F: O,:F:A:en:message
		** naim 0.12.0  IM_IN_ENC2:buddysn:F:F:T: O,:F:A:en:message
		** WinAIM       IM_IN_ENC2:buddysn:F:F:T: O,:F:A:en:<HTML><BODY BGCOLOR="#ffffff"><FONT LANG="0">message</FONT></BODY></HTML>
		** ICQ user     IM_IN_ENC2:useruin:F:F:T: I,:F:A:en:message
		**                                 | | |  |  | |  |
		**                                 | | |  |  | |  language, limited to real languages (either two-letter code or "x-bad"), set by sender in toc2_send_im_enc
		**                                 | | |  |  | character encoding, can be A L or U, set by sender in toc2_send_im_enc
		**                                 | | |  |  unknown meaning, can be T or F, set by sender in toc2_send_im_enc
		**                                 | | |  class, see UPDATE_BUDDY2 below
		**                                 | | seems to be T for TOC2/Oscar and F for cell phones and TOC1
		**                                 | unknown, always seems to be F
		**                                 away status, T when away and F normally
		*/
		char    *name, *message;
		int     isauto;

		args = toc_parse_args2(data, 10, ':');
		if ((args[1] == NULL) || (args[2] == NULL) || (args[9] == NULL)) {
			toc_internal_disconnect(c, FE_INVALIDFORMAT);
			return(FE_INVALIDFORMAT);
		}

		name = args[1];
		isauto = (args[2][0]=='T')?1:0;
		message = args[9];

		aim_handle_ect(c, name, message, isauto);
		if (*message != 0) {
			char    *mestart;

			if (strncasecmp(message, "/me ",4) == 0)
				firetalk_callback_im_getaction(c, name, isauto,
					message+4);
			else if ((mestart = strstr(message, ">/me ")) != NULL)
				firetalk_callback_im_getaction(c, name, isauto,
					mestart+5);
			else {
#if defined(DEBUG_ECHO) && defined(HAVE_ASPRINTF)
				char    *A, *B, *C, *encoding, *newmessage = NULL;

				if (args[5][0] == ' ')
					A = "";
				else if (args[5][0] == 'A')
					A = "AOL ";
				else
					A = "unknown0 ";

				if (args[5][1] == ' ')
					B = "";
				else if (args[5][1] == 'A')
					B = "ADMINISTRATOR";
				else if (args[5][1] == 'C')
					B = "WIRELESS";
				else if (args[5][1] == 'I')
					B = "ICQ";
				else if (args[5][1] == 'O')
					B = "OSCAR_FREE";
				else if (args[5][1] == 'U')
					B = "DAMNED_TRANSIENT";
				else
					B = "unknown1";

				if ((args[5][2] == ' ') || (args[5][2] == 0) || (args[5][2] == ','))
					C = "";
				else if (args[5][1] == 'U')
					C = " UNAVAILABLE";
				else
					C = " unknown2";

				if (args[7][0] == 'A')
					encoding = "ASCII";
				else if (args[7][0] == 'L')
					encoding = "Latin1";
				else if (args[7][0] == 'U')
					encoding = "UTF8";
				else
					encoding = "unknown";

				asprintf(&newmessage, "[%s%s%s, %s, %s %s %s %s] %s",
					A, B, C,
					encoding,
					args[2], args[3], args[4], args[6],
					message);
				message = newmessage;
#endif
				if (isauto) /* interpolate only auto-messages */
					firetalk_callback_im_getmessage(c,
						name, 1, aim_interpolate_variables(message, c->nickname));
				else
					firetalk_callback_im_getmessage(c,
						name, 0, message);
#if defined(DEBUG_ECHO) && defined(HAVE_ASPRINTF)
				free(message);
#endif
			}
		}
		firetalk_callback_im_buddyonline(c, name, 1);
#ifdef ENABLE_TYPING
		firetalk_callback_typing(c, name, 0);
#endif
	} else if (strcmp(arg0, "CLIENT_EVENT2") == 0) {
		/* 1 source
		** 2 status
		*/
		char    *name;
		int     typinginfo;

		args = toc_parse_args2(data, 3, ':');
		if (!args[1] || !args[2]) {
			toc_internal_disconnect(c, FE_INVALIDFORMAT);
			return(FE_INVALIDFORMAT);
		}

		name = args[1];
		typinginfo = atol(args[2]);

#ifdef ENABLE_TYPING
		firetalk_callback_typing(c, name, typinginfo);
#endif
	} else if (strcmp(arg0, "UPDATE_BUDDY2") == 0) {
		/* UPDATE_BUDDY:<Buddy User>:<Online? T/F>:<Evil Amount>:<Signon Time>:<IdleTime>:<UC>:<status code> */
		/* 1 source
		** 2 'T'=online, 'F'=offline
		** 3 warning level out of 100
		** 4 signon time in seconds since epoch
		** 5 idle time in minutes
		** 6 flags: ABC; A in 'A'=AOL user;
		**      B in 'A'=admin, 'C'=cell phone, 'I'=ICQ, 'O'=normal, 'U'=unconfirmed;
		**      C in 'U'=away
		** 7 status code, used in ICQ
		*/
		char    *name;
		int     isaway;
		long    online, warn, idle;

		args = toc_parse_args2(data, 8, ':');
		if (!args[1] || !args[2] || !args[3] || !args[4] || !args[5]
			|| !args[6] || !args[7]) {
			toc_internal_disconnect(c, FE_INVALIDFORMAT);
			return(FE_INVALIDFORMAT);
		}

		name = args[1];
		online = (args[2][0]=='T')?1:0;
		isaway = (args[6][2]=='U')?1:0;
		warn = atol(args[3]);
		idle = atol(args[5]);
		
		firetalk_callback_im_buddyonline(c, name, online);
		if (online != 0) {
			firetalk_callback_im_buddyaway(c, name, "", isaway);
			firetalk_callback_idleinfo(c, name, idle);
#ifdef ENABLE_WARNINFO
			firetalk_callback_warninfo(c, name, warn);
#endif
		} else {
		}
	} else if (strcmp(arg0, "BUDDY_CAPS2") == 0) {
		/* 1 source
		** 2 capabilities separated by commas
		*/
		char    capstring[1024],
			*name, **caps;
		int     i, firstcap;

		args = toc_parse_args2(data, 3, ':');
		if (!args[1] || !args[2]) {
			toc_internal_disconnect(c, FE_INVALIDFORMAT);
			return(FE_INVALIDFORMAT);
		}

		name = args[1];
		caps = toc_parse_args2(args[2], 255, ',');
		firstcap = strtol(caps[0], NULL, 16);
		for (i = 0; i < sizeof(toc_firstcaps)/sizeof(*toc_firstcaps); i++)
			if (toc_firstcaps[i].val == firstcap) {
				snprintf(capstring, sizeof(capstring), "%s", toc_firstcaps[i].name);
				break;
			}
		if (i == sizeof(toc_firstcaps)/sizeof(*toc_firstcaps))
			snprintf(capstring, sizeof(capstring), "UNKNOWN_TYPE_%X", firstcap);

		for (i = 1; (caps[i] != NULL) && (*caps[i] != 0); i++) {
			int     j, A1, A2, B, C, D, E1, E2, E3;

			toc_uuid(caps[i], &A1, &A2, &B, &C, &D, &E1, &E2, &E3);
			j = toc_cap(A1, A2, B, C, D, E1, E2, E3);
			if (j != -1) {
				if (strcmp(toc_uuids[j].name, "THIRD_PARTY_RANGE") != 0)
					snprintf(capstring+strlen(capstring), sizeof(capstring)-strlen(capstring),
						" %s", toc_uuids[j].name);
				else {
					snprintf(capstring+strlen(capstring), sizeof(capstring)-strlen(capstring),
						" +[%c%c%c%c%c%c%c%c%c%c%c%c", B>>8, B&0xFF, C>>8, C&0xFF, D>>8, D&0xFF, E1>>8, E1&0xFF, E2>>8, E2&0xFF, E3>>8, E3&0xFF);
					snprintf(capstring+strlen(capstring), sizeof(capstring)-strlen(capstring), "]");
				}
			} else {
				if (strlen(caps[i]) > 4)
					snprintf(capstring+strlen(capstring), sizeof(capstring)-strlen(capstring),
						" ?[%04X%04X-%04X-%04X-%04X-%04X%04X%04X]", A1, A2, B, C, D, E1, E2, E3);
				else
					snprintf(capstring+strlen(capstring), sizeof(capstring)-strlen(capstring),
						" UNKNOWN_CAP_%04X", A2);
			}
		}
#ifdef ENABLE_CAPABILITIES
		firetalk_callback_capabilities(c, name, capstring);
#endif
	} else if (strcmp(arg0, "BART2") == 0) {
		/* 1 source
		** 2 base64-encoded strings, unidentified
		*/
		char    **barts;
		int     i;

		args = toc_parse_args2(data, 3, ':');
		barts = toc_parse_args2(args[2], 255, ' ');
		for (i = 0; (barts[i] != NULL) && (barts[i+1] != NULL) && (barts[i+2] != NULL); i += 3) {
			int     j, flag = atoi(barts[i]), type = atoi(barts[i+1]);

			for (j = 0; j < sizeof(toc_barts)/sizeof(*toc_barts); j++)
				if (toc_barts[j].val == type) {
#ifdef DEBUG_ECHO
					toc_echof(c, "got_data", "BART %i %i: %s: %s [%s]\n", flag, type, toc_barts[j].name, barts[i+2], firetalk_debase64(args[i+2]));
#endif
					break;
				}
		}
	} else if (strcmp(arg0, "NICK") == 0) {
		/* NICK:<Nickname>
		**    Tells you your correct nickname (ie how it should be capitalized and
		**    spacing)
		*/
		args = toc_parse_args2(data, 2, ':');
		if (!args[1]) {
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "NICK");
			return(FE_SUCCESS);
		}
		firetalk_callback_user_nickchanged(c, c->nickname, args[1]);
		free(c->nickname);
		c->nickname = strdup(args[1]);
		if (c->nickname == NULL)
			abort();
		firetalk_callback_newnick(c, args[1]);
	} else if (strcmp(arg0, "EVILED") == 0) {
		/* EVILED:<new evil>:<name of eviler, blank if anonymous>
		**    The user was just eviled.
		*/
		args = toc_parse_args2(data, 3, ':');
		if (!args[1]) {
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "EVILED");
			return(FE_SUCCESS);
		}

		firetalk_callback_eviled(c, atoi(args[1]), args[2]);
	} else if (strcmp(arg0, "CHAT_JOIN") == 0) {
		/* CHAT_JOIN:<Chat Room Id>:<Chat Room Name>
		**    We were able to join this chat room.  The Chat Room Id is
		**    internal to TOC.
		*/
		long    id;
		char    *name;
		int     exchange, ret;

		args = toc_parse_args2(data, 3, ':');
		if (!args[1] || !args[2]) {
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "CHAT_JOIN");
			return(FE_SUCCESS);
		}
		id = atol(args[1]);
		name = args[2];
		exchange = toc_internal_find_exchange(c, name);

		ret = toc_internal_set_id(c, name, exchange, id);
		ret = toc_internal_set_joined(c, id);
		firetalk_callback_chat_joined(c, toc_internal_find_room_name(c, id));
	} else if (strcmp(arg0, "CHAT_IN_ENC") == 0) {
		/* CHAT_IN:<Chat Room Id>:<Source User>:<Whisper? T/F>:<Message>
		**    A chat message was sent in a chat room.
		*/
		/* 1 room ID
		** 2 source
		** 3 'T'=private, 'F'=public
		** 4 unknown
		** 5 language
		** 6 message
		*/
		long    id;
		char    *source, *message, *mestart;

		args = toc_parse_args2(data, 7, ':');
		if (!args[1] || !args[2] || !args[3] || !args[4] || !args[5] || !args[6]) {
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "CHAT_IN_ENC");
			return(FE_SUCCESS);
		}
		id = atol(args[1]);
		source = args[2];
		message = args[6];

		if (strncasecmp(message, "<HTML><PRE>", 11) == 0) {
			message += 11;
			if ((tempchr1 = strchr(message, '<')))
				*tempchr1 = 0;
		}
		if (strncasecmp(message, "/me ", 4) == 0)
			firetalk_callback_chat_getaction(c,
				toc_internal_find_room_name(c, id),
				source, 0, message+4);
		else if ((mestart = strstr(message, ">/me ")) != NULL)
			firetalk_callback_chat_getaction(c,
				toc_internal_find_room_name(c, id),
				source, 0, mestart+5);
		else
			firetalk_callback_chat_getmessage(c,
				toc_internal_find_room_name(c, id),
				source, 0, message);
	} else if (strcmp(arg0, "CHAT_UPDATE_BUDDY") == 0) {
		/* CHAT_UPDATE_BUDDY:<Chat Room Id>:<Inside? T/F>:<User 1>:<User 2>...
		**    This one command handles arrival/departs from a chat room.  The
		**    very first message of this type for each chat room contains the
		**    users already in the room.
		*/
		int     joined;
		long    id;
		char    *recip,
			*source,
			*colon;

		args = toc_parse_args2(data, 4, ':');
		if (!args[1] || !args[2] || !args[3]) {
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "CHAT_UPDATE_BUDDY");
			return(FE_SUCCESS);
		}
		joined = (args[2][0]=='T')?1:0;
		id = atol(args[1]);
		recip = toc_internal_find_room_name(c, id);
		source = args[3];
/*
		while ((colon = strchr(source, ':'))) {
			*colon = 0;
			if (joined)
				firetalk_callback_chat_user_joined(c, recip, source);
			else
				firetalk_callback_chat_user_left(c, recip, source, NULL);
			source = colon+1;
		}
		if (joined) {
			firetalk_callback_chat_user_joined(c, recip, source);
			firetalk_callback_chat_user_joined(c, recip, NULL);
		} else
			firetalk_callback_chat_user_left(c, recip, source, NULL);
*/
	} else if (strcmp(arg0, "CHAT_INVITE") == 0) {
		/* CHAT_INVITE:<Chat Room Name>:<Chat Room Id>:<Invite Sender>:<Message>
		**    We are being invited to a chat room.
		*/
		args = toc_parse_args2(data, 5, ':');
		if (!args[1] || !args[2] || !args[3] || !args[4]) {
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "CHAT_INVITE");
			return(FE_SUCCESS);
		}
		if (toc_internal_add_room(c, args[1], 4) == FE_SUCCESS)
			if (toc_internal_set_room_invited(c, args[1], 1) == FE_SUCCESS)
				if (toc_internal_set_id(c, args[1], 4, atol(args[2])) == FE_SUCCESS)
					firetalk_callback_chat_invited(c, args[1], args[3], args[4]);
	} else if (strcmp(arg0, "CHAT_LEFT") == 0) {
		/* CHAT_LEFT:<Chat Room Id>
		**    Tells tic connection to chat room has been dropped
		*/
		args = toc_parse_args2(data, 2, ':');
		if (!args[1]) {
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "CHAT_LEFT");
			return(FE_SUCCESS);
		}
		firetalk_callback_chat_left(c, toc_internal_find_room_name(c, atol(args[1])));
	} else if (strcmp(arg0, "GOTO_URL") == 0) {
		/* GOTO_URL:<Window Name>:<Url>
		**    Goto a URL.  Window Name is the suggested internal name of the window
		**    to use.  (Java supports this.)
		*/
		struct s_toc_infoget *i;

		/* create a new infoget object and set it to connecting state */
		args = toc_parse_args2(data, 3, ':');
		if (!args[1] || !args[2]) {
			toc_internal_disconnect(c, FE_INVALIDFORMAT);
			return(FE_INVALIDFORMAT);
		}

		i = c->infoget_head;
		c->infoget_head = calloc(1, sizeof(struct s_toc_infoget));
		if (c->infoget_head == NULL)
			abort();
		snprintf(c->infoget_head->buffer, TOC_HTML_MAXLEN, "GET /%s HTTP/1.0\r\n\r\n", args[2]);
		c->infoget_head->buflen = strlen(c->infoget_head->buffer);
		c->infoget_head->next = i;
		c->infoget_head->state = TOC_STATE_CONNECTING;
		c->infoget_head->sockfd = firetalk_internal_connect(firetalk_internal_remotehost4(c)
#ifdef _FC_USE_IPV6
				, firetalk_internal_remotehost6(c)
#endif
				);
		if (c->infoget_head->sockfd == -1) {
			firetalk_callback_error(c, FE_CONNECT, (lastinfo[0]=='\0')?NULL:lastinfo, "Failed to connect to info server");
			free(c->infoget_head);
			c->infoget_head = i;
			return(FE_SUCCESS);
		}
	} else if (strcmp(arg0, "NEW_BUDDY_REPLY2") == 0) {
		/* NEW_BUDDY_REPLY2:19033926:added */
	} else if (strcmp(arg0, "UPDATED2") == 0) {
		/* UPDATED2:a:19033926:Buddy */
	} else if (strcmp(arg0, "DIR_STATUS") == 0) {
		/* DIR_STATUS:<Return Code>:<Optional args>
		**    <Return Code> is always 0 for success status.
		*/
		args = toc_parse_args2(data, 2, ':');
		if (args[1] == NULL) {
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "DIR_STATUS with no status code");
			return(FE_SUCCESS);
		}
		switch (atoi(args[1])) {
		  case 0:
		  case 1:
			break;
		  case 2:
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "DIR_STATUS failed, invalid format");
			break;
		  default:
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "DIR_STATUS with unknown code");
			break;
		}
	} else if (strcmp(arg0, "ADMIN_NICK_STATUS") == 0) {
		/* ADMIN_NICK_STATUS:<Return Code>:<Optional args>
		**    <Return Code> is always 0 for success status.
		*/
	} else if (strcmp(arg0, "ADMIN_PASSWD_STATUS") == 0) {
		/* ADMIN_PASSWD_STATUS:<Return Code>:<Optional args>
		**    <Return Code> is always 0 for success status.
		*/
		c->passchange--;
		args = toc_parse_args2(data, 3, ':');
		if (!args[1]) {
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "ADMIN_PASSWD_STATUS");
			return(FE_SUCCESS);
		}
		if (atoi(args[1]) != 0)
			firetalk_callback_error(c, FE_NOCHANGEPASS, NULL, NULL);
		else
			firetalk_callback_passchanged(c);
	} else if (strcmp(arg0, "PAUSE") == 0) {
		/* PAUSE
		**    Tells TIC to pause so we can do migration
		*/
		c->connectstate = 1;
		firetalk_internal_set_connectstate(c, FCS_WAITING_SIGNON);
	} else if (strcmp(arg0, "RVOUS_PROPOSE") == 0) {
		/* RVOUS_PROPOSE:<user>:<uuid>:<cookie>:<seq>:<rip>:<pip>:<vip>:<port>
		**               [:tlv tag1:tlv value1[:tlv tag2:tlv value2[:...]]]
		**    Another user has proposed that we rendezvous with them to
		**    perform the service specified by <uuid>.  They want us
		**    to connect to them, we have their rendezvous ip, their
		**    proposer_ip, and their verified_ip. The tlv values are
		**    base64 encoded.
		*/
		int     j, A1, A2, B, C, D, E1, E2, E3;

		args = toc_parse_args2(data, 255, ':');
		if (!args[1] || !args[2] || !args[3] || !args[4] || !args[5] || !args[6] || !args[7] || !args[8]) {
			firetalk_callback_error(c, FE_INVALIDFORMAT, NULL, "RVOUS_PROPOSE");
			return(FE_SUCCESS);
		}
#ifdef DEBUG_ECHO
		{
			int     i;

			toc_echof(c, "got_data", "RVOUS_PROPOSE\n1user=[%s]\n2uuid=%s\n3cookie=[%s]\n4seq=%s\n5rendezvous_ip=%s\n6proposer_ip=%s\n7verified_ip=%s\n8port=%s\n",
				args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			for (i = 9; args[i] && args[i+1]; i += 2)
				toc_echof(c, "got_data", "RVOUS_PROPOSE\n%itype=%s\n%ivalue=%s [%s]\n", i, args[i], i+1, args[i+1], firetalk_debase64(args[i+1]));
		}
#endif
		toc_uuid(args[2], &A1, &A2, &B, &C, &D, &E1, &E2, &E3);
		j = toc_cap(A1, A2, B, C, D, E1, E2, E3);
		if (j == -1)
			firetalk_callback_error(c, FE_WEIRDPACKET, NULL, "Unknown rendezvous UUID");
		else {
			if (strcmp(toc_uuids[j].name, "FILE_TRANSFER") == 0) {
				const char *message, *file;

				if ((message = toc_get_tlv_value(args, 9, 12)) != NULL)
					firetalk_callback_im_getmessage(c, args[1], 0, message);

				if ((file = toc_get_tlv_value(args, 9, 10001)) != NULL) {
					unsigned long   size = ntohl(*((uint32_t *)(file+4)));

					firetalk_callback_file_offer(c,
						/* from */      args[1],                /* user */
						/* filename */  file+8,                 /* filename */
						/* size */      size,
						/* ipstring */  args[7],                /* verified_ip */
						/* ip6string */ NULL,
						/* port */      (uint16_t)atoi(args[8]),/* port */
						/* type */      FF_TYPE_RAW);
				}
			} else
				firetalk_callback_error(c, FE_WEIRDPACKET, NULL, "Unhandled rendezvous UUID, do we have a capability set we do not actually support?");
		}
	} else
		firetalk_callback_error(c, FE_WEIRDPACKET, NULL, data);

	goto got_data_start;
}

enum firetalk_error   toc_periodic(struct s_firetalk_handle *const c) {
	struct s_toc_connection *conn;
	long idle;
	char data[32];

	conn = c->handle;

	if (firetalk_internal_get_connectstate(conn) != FCS_ACTIVE)
		return FE_NOTCONNECTED;

	idle = (long)(time(NULL) - conn->lasttalk);
	firetalk_callback_setidle(conn, &idle);

	if (idle < 600)
		idle = 0;

	if (idle/60 == conn->lastidle/60)
		return FE_IDLEFAST;

	if ((conn->lastidle/60 == 0) && (idle/60 == 0))
		return(FE_IDLEFAST);
	if ((conn->lastidle/60 != 0) && (idle/60 != 0))
		return(FE_IDLEFAST);

	conn->lastidle = idle;
	sprintf(data, "%ld", idle);
	return(toc_send_printf(conn, "toc_set_idle %s", data));
}

enum firetalk_error toc_chat_join(client_t c, const char *const room, const char * const password) {
	int i;
	i = toc_internal_get_room_invited(c,room);
	if (i == 1) {
		(void) toc_internal_set_room_invited(c,room,0);
		return toc_send_printf(c,"toc_chat_accept %s",toc_internal_find_room_id(c,room));
	} else {
		int m;
		char *s;
		(void) toc_internal_add_room(c,s = toc_internal_split_name(room),m = toc_internal_split_exchange(room));
		return toc_send_printf(c,"toc_chat_join %d %s",m,s);
	}
}

enum firetalk_error   toc_chat_set_topic(client_t c, const char *const room, const char *const topic) {
	return FE_SUCCESS;
}

enum firetalk_error   toc_chat_op(client_t c, const char *const room, const char *const who) {
	return FE_SUCCESS;
}

enum firetalk_error   toc_chat_deop(client_t c, const char *const room, const char *const who) {
	return FE_SUCCESS;
}

enum firetalk_error   toc_chat_kick(client_t c, const char *const room, const char *const who, const char *const reason) {
	return FE_SUCCESS;
}

enum firetalk_error   toc_chat_send_message(client_t c, const char *const room, const char *const message, const int auto_flag) {
	if (strlen(message) > 232)
		return(FE_PACKETSIZE);

	if (strcasecmp(room, ":RAW") == 0)
		return(toc_send_printf(c, "%S", message));
	else {
		int     id = toc_internal_find_room_id(c,room);

		if (id == 0)
			return(FE_ROOMUNAVAILABLE);
		return(toc_send_printf(c, "toc_chat_send %i %s", id, message));
	}
}

enum firetalk_error   toc_chat_send_action(client_t c, const char *const room, const char *const message, const int auto_flag) {
	char tempbuf[2048];

	if (strlen(message) > 232-4)
		return FE_PACKETSIZE;

	safe_strncpy(tempbuf,"/me ",2048);
	safe_strncat(tempbuf,message,2048);
	return toc_send_printf(c,"toc_chat_send %s %s",toc_internal_find_room_id(c,room),tempbuf);
}

enum firetalk_error   toc_chat_invite(client_t c, const char *const room, const char *const who, const char *const message) {
	int     id = toc_internal_find_room_id(c,room);

	if (id != 0)
		return(toc_send_printf(c, "toc_chat_invite %i %s %s", id, message, who));
	return(FE_NOTFOUND);
}

void    ect_prof(client_t c, const char *const command,
		const char *const ect) {
	int     i;
	size_t  len = 0;

	for (i = 0; i < c->profc; i++)
		if (strcmp(c->profar[i].command, command) == 0)
			break;
	if (i == c->profc) {
		c->profc++;
		c->profar = realloc(c->profar, (c->profc)*sizeof(*(c->profar)));
		c->profar[i].command = strdup(command);
		c->profar[i].ect = strdup(ect);
	} else {
		free(c->profar[i].ect);
		c->profar[i].ect = strdup(ect);
	}

	for (i = 0; i < c->profc; i++)
		len += strlen(c->profar[i].ect);
	free(c->profstr);
	c->profstr = malloc(len+1);
	*(c->profstr) = 0;
	for (i = 0; i < c->profc; i++)
		strcat(c->profstr, c->profar[i].ect);
	c->proflen = len;
}

void    ect_queue(client_t c, const char *const to,
		const char *const ect) {
	int     i;

	for (i = 0; i < c->ectqc; i++)
		if (toc_compare_nicks(to, c->ectqar[i].dest) == 0) {
			char    *newstr = malloc(strlen(c->ectqar[i].ect)
					+ strlen(ect) + 1);

			if (newstr != NULL) {
				strcpy(newstr, c->ectqar[i].ect);
				strcat(newstr, ect);
				free(c->ectqar[i].ect);
				c->ectqar[i].ect = newstr;
				return;
			}
		}

	c->ectqc++;
	c->ectqar = realloc(c->ectqar, (c->ectqc)*sizeof(*(c->ectqar)));
	c->ectqar[i].dest = strdup(to);
	c->ectqar[i].ect = strdup(ect);
}

char    *subcode_ect(char *buf, size_t buflen, const char *const command,
		const char *const args) {
	if (command == NULL)
		return(NULL);

	if (args == NULL)
		safe_snprintf(buf, buflen, "<font ECT=\"%s\"></font>",
			command);
	else
		safe_snprintf(buf, buflen, "<font ECT=\"%s %s\"></font>",
			command, firetalk_htmlentities(args));
	return(buf);
}

enum firetalk_error   toc_subcode_send_request(client_t c, const char *const to, const char *const command, const char *const args) {
	char    buf[2048],
		*ect;

	if (isdigit(c->nickname[0]))
		return(FE_SUCCESS);
	if ((ect = subcode_ect(buf, sizeof(buf), command, args)) == NULL)
		return(FE_SUCCESS);

	ect_queue(c, to, ect);
	return(FE_SUCCESS);
}

enum firetalk_error   toc_subcode_send_reply(client_t c, const char *const to, const char *const command, const char *const args) {
	char    buf[2048],
		*ect;

	if ((ect = subcode_ect(buf, sizeof(buf), command, args)) == NULL)
		return(FE_SUCCESS);

	if (to != NULL)
		return(toc_im_send_message(c, to, ect, 1));
	else {
		if (args != NULL)
			ect_prof(c, command, ect);
		else
			ect_prof(c, command, "");

		return(FE_SUCCESS);
	}
}
