/*
irc.c - FireTalk IRC protocol definitions
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
#include <sys/socket.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

struct s_irc_whois {
	struct s_irc_whois *next;
	char *nickname;
	char *info;
	int flags;
	long idle;
};

struct s_irc_connection {
	char *nickname;
	char *password;
	char buffer[513];
	time_t lasttime;                                     /* last time we set idles */
	int isons;                                           /* number of ISON's we're waiting on replies to */
	struct s_irc_whois *whois_head;
	int passchange;                                      /* whether we are currently changing our pass */
	int usesilence;                                      /* are we on a network that understands SILENCE */
	int identified;                                 /* are we identified */
};

typedef struct s_irc_connection * client_t;
#define _HAVE_CLIENT_T

#include "firetalk-int.h"
#include "firetalk.h"
#include "irc.h"
#include "safestring.h"

#define IRC_SERVER "irc.openprojects.net"
#define IRC_PORT 6667
#define ROOMSTARTS "#+&"



static char *irc_html_to_irc(const char * const string);
static char *irc_irc_to_html(const char * const string);
static int irc_send_printf(client_t c, const char * const format, ...);
static int irc_internal_disconnect(client_t c, const int error);
static char **irc_recv_parse(client_t c, unsigned char *buffer, unsigned short *bufferpos);

/*

from buddy structure:

tempint1 = ison, default 0, used during ISON checks
tempint2 = away, default 0, used during WHOIS checks

*/

static char *irc_html_to_irc(const char * const string) {
	static char *output = NULL;
	int o = 0;
	size_t l,i=0;
	l = strlen(string);
	output = safe_realloc(output,l + 1);
	while (i < l) {
		switch(string[i]) {
			case '&':
				if (!strncasecmp(&string[i],"&amp;",5)) {
					output[o++] = '&';
					i += 5;
				} else if (!strncasecmp(&string[i],"&gt;",4)) {
					output[o++] = '>';
					i += 4;
				} else if (!strncasecmp(&string[i],"&lt;",4)) {
					output[o++] = '<';
					i += 4;
				} else
					output[o++] = string[i++];
				break;
			case '<':
				if (!strncasecmp(&string[i],"<b>",3)) {
					output[o++] = (char) 2;
					i += 3;
				} else if (!strncasecmp(&string[i],"</b>",4)) {
					output[o++] = (char) 2;
					i += 4;
				} else if (!strncasecmp(&string[i],"<i>",3)) {
					output[o++] = (char) 22;
					i += 3;
				} else if (!strncasecmp(&string[i],"</i>",4)) {
					output[o++] = (char) 22;
					i += 4;
				} else if (!strncasecmp(&string[i],"<u>",3)) {
					output[o++] = (char) 31;
					i += 3;
				} else if (!strncasecmp(&string[i],"</u>",4)) {
					output[o++] = (char) 31;
					i += 4;
				} else if (!strncasecmp(&string[i],"<br>",4)) {
					output[o++] = '\020';
					output[o++] = 'r';
					output[o++] = '\020';
					output[o++] = 'n';
					i += 4;
				} else
					output[o++] = string[i++];
				break;
			case '\r':
			case '\n':
				output[o++] = ' ';
				i++;
				break;
/*
			case '\r':
				output[o++] = '\020';
				output[o++] = 'r';
				i++;
				break;
			case '\n':
				output[o++] = '\020';
				output[o++] = 'n';
				i++;
				break;
			case '\020':
				output[o++] = '\020';
				output[o++] = '\020';
				i++;
				break;
*/
			default:
				output[o++] = string[i++];
				break;
		}
	}
	output[o] = '\0';
	return output;
}

static char *irc_irc_to_html(const char * const string) {
	static char *output = NULL;
	int o = 0;
	size_t l,i=0;
	int inbold = 0, initalics = 0, inunderline = 0;
	l = strlen(string);
	output = safe_realloc(output,(l * 5) + 1);
	while (i < l) {
		switch(string[i]) {
			case 2:
				if (inbold == 1) {
					memcpy(&output[o],"</b>",4);
					o += 4;
					inbold = 0;
				} else {
					memcpy(&output[o],"<b>",3);
					o += 3;
					inbold = 1;
				}
				break;
			case 22:
				if (initalics == 1) {
					memcpy(&output[o],"</i>",4);
					o += 4;
					initalics = 0;
				} else {
					memcpy(&output[o],"<i>",3);
					o += 3;
					initalics = 1;
				}
				break;
			case 31:
				if (inunderline == 1) {
					memcpy(&output[o],"</u>",4);
					o += 4;
					inunderline = 0;
				} else {
					memcpy(&output[o],"<u>",3);
					o += 3;
					inunderline = 1;
				}
				break;
			case '&':
				memcpy(&output[o],"&amp;",5);
				o += 5;
				break;
			case '<':
				memcpy(&output[o],"&lt;",4);
				o += 4;
				break;
			case '>':
				memcpy(&output[o],"&gt;",4);
				o += 4;
				break;
			case 16:
				switch(string[++i]) {
					case 16:
						output[o++] = '\020';
						break;
					case 'r':
						if (string[i+1] == '\020' && string[i+2] == 'n') {
							i += 2;
							memcpy(&output[o],"<br>",4);
							o += 4;
						} else
							output[o++] = '\r';
						break;
					case 'n':
						output[o++] = '\n';
						break;
					default:
						output[o++] = string[i];
						break;
				}
				break;
			default:
				output[o++] = string[i];
				break;
		}
		i++;
	}
	output[o] = '\0';
	return output;
}

static int irc_internal_disconnect(client_t c, const int error) {
	struct s_irc_whois *whois_iter,*whois_iter2;
	
	if (c->nickname) {
		free(c->nickname);
		c->nickname = NULL;
	}
	if (c->password) {
		free(c->password);
		c->password = NULL;
	}
	whois_iter = c->whois_head;
	while (whois_iter) {
		whois_iter2 = whois_iter->next;
		if (whois_iter->nickname != NULL)
			free(whois_iter->nickname);
		if (whois_iter->info != NULL)
			free(whois_iter->info);
		free(whois_iter);
		whois_iter = whois_iter2;
	}
	c->whois_head = NULL;
	c->lasttime = 0;
	c->isons = 0;
	c->passchange = 0;
	c->usesilence = 1;
	c->identified = 0;
	firetalk_callback_disconnect(c,error);
	return FE_SUCCESS;
}

static int irc_send_printf(client_t c, const char * const format, ...) {
	va_list ap;
	size_t len,i,datai = 0;
	char data[513];
	char *tempchr;
	struct s_firetalk_handle *fchandle;

	va_start(ap,format);

	len = strlen(format);
	for (i = 0; i < len; i++) {
		if (format[i] == '%') {
			switch (format[++i]) {
				case 's':
					tempchr = irc_html_to_irc(va_arg(ap, char *));
					if (datai + strlen(tempchr) > 509)
						return FE_PACKETSIZE;
					safe_strncpy(&data[datai],tempchr,513-datai);
					datai += strlen(tempchr);
					break;
				case '%':
					data[datai++] = '%';
					break;
			}
		} else {
			data[datai++] = format[i];
			if (datai > 509)
				return FE_PACKETSIZE;
		}
	}
	data[datai] = '\0';

	safe_strncat(data,"\r\n",513);
	datai = strlen(data);

	fchandle = firetalk_find_handle(c);
	firetalk_internal_send_data(fchandle,data,datai,0);
	return FE_SUCCESS;
}

static char **irc_recv_parse(client_t c, unsigned char *buffer, unsigned short *bufferpos) {
	static char *args[256];
	static char data[513];
	size_t curarg;
	char *tempchr;
	char *tempchr2;

	args[0] = NULL;

	memcpy(data,buffer,*bufferpos);
	data[*bufferpos] = '\0';

	tempchr = strstr(data,"\r\n");
	if (tempchr == NULL)
		return NULL;
	tempchr[0] = '\0';
	*bufferpos -= (tempchr - data + 2);
	memmove(buffer,&buffer[tempchr - data + 2],*bufferpos);

	curarg = 0;
	tempchr = data;
	if (tempchr[0] == ':')
		tempchr++;

	while ((curarg < 256) && ((tempchr2 = strchr(tempchr,' ')) != NULL)) {
		args[curarg++] = tempchr;
		tempchr2[0] = '\0';
		tempchr = tempchr2 + 1;
		if (tempchr[0] == ':') {
			tempchr = irc_irc_to_html(&tempchr[1]);
			break;
		}
	}
	args[curarg++] = tempchr;
	args[curarg] = NULL;
	return args;
}

static char *irc_get_nickname(const char * const hostmask) {
	static char data[512];
	char *tempchr;
	safe_strncpy(data,hostmask,512);
	tempchr = strchr(data,'!');
	if (tempchr)
		tempchr[0] = '\0';
	return data;
}

static char *irc_get_email(const char * const hostmask) {
	static char data[512];
	char *tempchr;
	data[0] = 0;
	tempchr = strchr(hostmask,'!');

	if(tempchr) {
	    int nc;
	    nc = tempchr-hostmask;
	    strcpy(data, hostmask+nc+1);
	}

	return data;
}

enum firetalk_error irc_set_nickname(client_t c, const char * const nickname) {
	return irc_send_printf(c,"NICK %s",nickname);
}

enum firetalk_error irc_set_password(client_t c, const char * const oldpass, const char * const newpass) {
	c->passchange++;
	return irc_send_printf(c,"PRIVMSG NickServ :SET PASSWORD %s",newpass);
}

void irc_destroy_handle(client_t c) {
	irc_send_printf(c,"QUIT :Handle destroyed");
	irc_internal_disconnect(c,FE_USERDISCONNECT);
	free(c);
}

enum firetalk_error irc_disconnect(client_t c) {
	irc_send_printf(c,"QUIT :User disconnected");
	return irc_internal_disconnect(c,FE_USERDISCONNECT);
}

client_t irc_create_handle() {
	client_t c;
	c = safe_malloc(sizeof(struct s_irc_connection));

	c->lasttime = 0;
	c->isons = 0;
	c->whois_head = NULL;
	c->nickname = NULL;
	c->password = NULL;
	c->passchange = 0;
	c->usesilence = 1;
	c->identified = 0;

	return c;
}

enum firetalk_error irc_signon(client_t c, const char * const nickname) {
	const char *username, *hostname, *servername, *realname;
	struct s_firetalk_handle *conn;
	char pass[128];

	conn = firetalk_find_handle(c);

	firetalk_callback_needpass(c, pass, 128);

	if(strlen(pass))
	if(irc_send_printf(c, "PASS %s", pass) != FE_SUCCESS)
	    return FE_PACKET;

	username = getenv("USER");
	hostname = getenv("HOSTNAME");

	if(!hostname) hostname = nickname;
	if(!username) username = nickname;

	if(irc_send_printf(c, "USER %s %s %s :%s", username, hostname, conn->server, nickname) != FE_SUCCESS)
		return FE_PACKET;

	if(irc_send_printf(c, "NICK %s", nickname) != FE_SUCCESS)
		return FE_PACKET;

	c->nickname = safe_strdup(nickname);

	return FE_SUCCESS;
}

enum firetalk_error irc_save_config(client_t c) {
	return FE_SUCCESS;
}

enum firetalk_error irc_preselect(client_t c, fd_set *read, fd_set *write, fd_set *except, int *n) {
	return FE_SUCCESS;
}

enum firetalk_error irc_postselect(client_t c, fd_set *read, fd_set *write, fd_set *except) {
	return FE_SUCCESS;
}

enum firetalk_error irc_got_data(client_t c, unsigned char * buffer, unsigned short * bufferpos) {
	char **args;
	char *tempchr, *tempchr2;
	struct s_firetalk_buddy *buddyiter;
	struct s_irc_whois *whoisiter, *whoisiter2;
	struct s_firetalk_handle *fchandle;
	int tempint,tempint2,tempint3;
	char tempbuf[512];

	fchandle = firetalk_find_handle(c);

	args = irc_recv_parse(c,buffer,bufferpos);
	while (args != NULL && args[0] != NULL) {
		/* zero argument items */
		if (strcmp(args[0],"PING") == 0) {
			if (args[1]) {
				if (irc_send_printf(c,"PONG %s",args[1]) != 0) {
					irc_internal_disconnect(c,FE_PACKET);
					return FE_PACKET;
				}
			} else {
				if (irc_send_printf(c,"PONG") != 0) {
					irc_internal_disconnect(c,FE_PACKET);
					return FE_PACKET;
				}
			}
		}
		if (args[1] != NULL) {
			tempint = atoi(args[1]);
			if (strcmp(args[1],"QUIT") == 0) {
				firetalk_callback_im_buddyonline(c,irc_get_nickname(args[0]),0);
				if (irc_compare_nicks(c->nickname,irc_get_nickname(args[0])) == 0)
					irc_internal_disconnect(c,FE_DISCONNECT);
				else
					firetalk_callback_chat_user_quit(c,irc_get_nickname(args[0]),args[2]);
			}
		}
		if ((args[1] != NULL) && (args[2] != NULL)) {
			/* two-arg commands */
			if (strcmp(args[1],"JOIN") == 0) {
				firetalk_callback_im_buddyonline(c,irc_get_nickname(args[0]),1);
				if (irc_compare_nicks(c->nickname,irc_get_nickname(args[0])) == 0) {
					firetalk_callback_chat_joined(c,args[2]);
					if (c->identified == 1) {
						if (irc_send_printf(c,"PRIVMSG ChanServ :OP %s %s",args[2],c->nickname) != FE_SUCCESS) {
							irc_internal_disconnect(c,FE_PACKET);
							return FE_PACKET;
						}
					}
				} else
					firetalk_callback_chat_user_joined(c,args[2],irc_get_nickname(args[0]),irc_get_email(args[0]));
			} else if (strcmp(args[1],"PART") == 0) {
				if (irc_compare_nicks(c->nickname,irc_get_nickname(args[0])) == 0)
					firetalk_callback_chat_left(c,args[2]);
				else
					firetalk_callback_chat_user_left(c,args[2],irc_get_nickname(args[0]),args[3]);
			} else if (strcmp(args[1],"NICK") == 0) {
				if (irc_compare_nicks(c->nickname,irc_get_nickname(args[0])) == 0) {
					free(c->nickname);
					c->nickname = safe_strdup(args[2]);
					firetalk_callback_newnick(c,c->nickname);
				}
				firetalk_callback_user_nickchanged(c,irc_get_nickname(args[0]),args[2]);
			}
		}
		if ((args[1] != NULL) && (args[2] != NULL) && (args[3] != NULL)) {
			if (strcmp(args[1],"PRIVMSG") == 0) {
				/* scan for CTCP's */
				while ((tempchr = strchr(args[3],1))) {
					if ((tempchr2 = strchr(&tempchr[1],1)))
						tempchr2[0] = '\0';
					/* we have a ctcp */
					if (strchr(ROOMSTARTS,args[2][0])) {
						/* chat room subcode */
						if (strncasecmp(&tempchr[1],"ACTION ",7) == 0)
							firetalk_callback_chat_getaction(c,args[2],irc_get_nickname(args[0]),0,&tempchr[8]);
					} else {
						char *endcommand;
						endcommand = strchr(&tempchr[1],' ');
						if (endcommand) {
							*endcommand = '\0';
							endcommand++;
							firetalk_callback_subcode_request(c,irc_get_nickname(args[0]),&tempchr[1],endcommand);
						} else
							firetalk_callback_subcode_request(c,irc_get_nickname(args[0]),&tempchr[1],NULL);
					}
					if (tempchr2)
						memmove(tempchr,&tempchr2[1],strlen(&tempchr2[1]) + 1); else break;
				}
				if (args[3][0] != '\0') {
					if (strchr(ROOMSTARTS,args[2][0]))
						firetalk_callback_chat_getmessage(c,args[2],irc_get_nickname(args[0]),0,args[3]);
					else
						firetalk_callback_im_getmessage(c,irc_get_nickname(args[0]),0,args[3]);
				}
			} else if (strcmp(args[1],"NOTICE") == 0) {
				/* scan for CTCP's */
				while ((tempchr = strchr(args[3],1))) {
					if ((tempchr2 = strchr(&tempchr[1],1)))
						tempchr2[0] = '\0';
					/* we have a ctcp */
					if (strchr(ROOMSTARTS,args[2][0]) == NULL) {
						char *endcommand;
						endcommand = strchr(&tempchr[1],' ');
						if (endcommand) {
							*endcommand = '\0';
							endcommand++;
							firetalk_callback_subcode_reply(c,irc_get_nickname(args[0]),&tempchr[1],endcommand);
						} else
							firetalk_callback_subcode_reply(c,irc_get_nickname(args[0]),&tempchr[1],NULL);
					}
					if (tempchr2)
						memcpy(tempchr,&tempchr2[1],strlen(&tempchr2[1]) + 1); else break;
				}
				if (!strcasecmp(irc_get_nickname(args[0]),"NickServ")) {
					if ((strstr(args[3],"IDENTIFY") != NULL) && (strstr(args[3],"/msg") != NULL) && (strstr(args[3],"HELP") == NULL)) {
						/*
						c->identified = 0;
						// nickserv seems to be asking us to identify ourselves, and we have a password
						if (!c->password) {
							c->password = safe_malloc(128);
							firetalk_callback_needpass(c,c->password,128);
						}
						if ((c->password != NULL) && irc_send_printf(c,"PRIVMSG NickServ :IDENTIFY %s",c->password) != 0) {
							irc_internal_disconnect(c,FE_PACKET);
							return FE_PACKET;
						}
						*/
					}
					if ((strstr(args[3],"Password changed") != NULL) && (c->passchange != 0)) {
						/* successful change */
						c->passchange--;
						firetalk_callback_passchanged(c);
					}
					if ((strstr(args[3],"authentication required") != NULL) && (c->passchange != 0)) {
						/* didn't log in with the right password initially, not happening */
						c->identified = 0;
						c->passchange--;
						firetalk_callback_error(c,FE_NOCHANGEPASS,NULL,args[3]);
					}
					if ((strstr(args[3],"isn't registered") != NULL) && (c->passchange != 0)) {
						/* nick not registered, fail */
						c->passchange--;
						firetalk_callback_error(c,FE_NOCHANGEPASS,NULL,args[3]);
					}
					if (strstr(args[3],"Password accepted") != NULL) {
						/* we're recognized */
						c->identified = 1;
						if (irc_send_printf(c,"PRIVMSG ChanServ :OP ALL") != FE_SUCCESS) {
							irc_internal_disconnect(c,FE_PACKET);
							return FE_PACKET;
						}
					}
				}
				if (args[3][0] != '\0') {
					if (strchr(ROOMSTARTS,args[2][0]))
						firetalk_callback_chat_getmessage(c,args[2],irc_get_nickname(args[0]),1,args[3]);
					else
						firetalk_callback_im_getmessage(c,irc_get_nickname(args[0]),1,args[3]);
				}
			} else if (strcmp(args[1],"TOPIC") == 0) {
				firetalk_callback_chat_gottopic(c,args[2],args[3],irc_get_nickname(args[0]));
			} else if (strcmp(args[1],"KICK") == 0) {
				if (irc_compare_nicks(c->nickname,irc_get_nickname(args[3])) == 0)
					firetalk_callback_chat_kicked(c,args[2],irc_get_nickname(args[0]),args[4]);
				else {
					tempchr = safe_strdup(irc_get_nickname(args[0]));
					firetalk_callback_chat_user_kicked(c,args[2],irc_get_nickname(args[3]),tempchr,args[4]);
					free(tempchr);
				}
			} else {
				switch (tempint) {
					case 301: /* RPL_AWAY */
						buddyiter = fchandle->buddy_head;
						while (buddyiter) {
							if (irc_compare_nicks(args[3],buddyiter->nickname) == 0) {
								buddyiter->tempint2 = 1;
								firetalk_callback_im_buddyaway(c,args[3],args[4],1);
							}
							buddyiter = buddyiter->next;
						}
						break;
					case 303: /* RPL_ISON */
						tempchr = args[3];
						while ((tempchr != NULL) && (tempchr[0] != '\0')) {
							tempchr2 = strchr(tempchr,' ');
							if (tempchr2)
								tempchr2[0] = '\0';
							buddyiter = fchandle->buddy_head;
							while (buddyiter) {
								if (irc_compare_nicks(tempchr,buddyiter->nickname) == 0)
									buddyiter->tempint = 1;
								buddyiter = buddyiter->next;
							}
							if (tempchr2)
								tempchr = tempchr2 + 1;
							else
								tempchr = NULL;
						}
						if (--c->isons <= 0) {
							c->isons = 0;
							/* done, send the appropriate data */
							buddyiter = fchandle->buddy_head;
							while (buddyiter) {
								firetalk_callback_im_buddyonline(c,buddyiter->nickname,buddyiter->tempint);
								if (buddyiter->tempint != 0) {
									buddyiter->tempint2 = 0; /* away */
									if (irc_send_printf(c,"WHOIS %s",buddyiter->nickname) != 0) {
										irc_internal_disconnect(c,FE_PACKET);
										return FE_PACKET;
									}
								}
								buddyiter = buddyiter->next;
							}
						}
						break;
					case 313: /* RPL_WHOISOPER */
						whoisiter = c->whois_head;
						while (whoisiter) {
							if (irc_compare_nicks(args[3],whoisiter->nickname) == 0)
								whoisiter->flags |= FF_ADMIN;
							whoisiter = whoisiter->next;
						}
						break;
					case 315:
						firetalk_callback_chat_end_extended(c);
						break;
					case 318: /* RPL_ENDOFWHOIS */
						whoisiter = c->whois_head;
						whoisiter2 = NULL;
						while (whoisiter) {
							if (irc_compare_nicks(args[3],whoisiter->nickname) == 0) {
								/* manual whois */
								firetalk_callback_gotinfo(c,whoisiter->nickname,whoisiter->info,0,whoisiter->idle,whoisiter->flags);
								free(whoisiter->nickname);
								if (whoisiter->info)
									free(whoisiter->info);
								if (whoisiter2)
									whoisiter2->next = whoisiter->next;
								else
									c->whois_head = whoisiter->next;
								free(whoisiter);
								break;
							}
							whoisiter2 = whoisiter;
							whoisiter = whoisiter->next;
						}
						buddyiter = fchandle->buddy_head;
						while (buddyiter) {
							if (irc_compare_nicks(args[3],buddyiter->nickname) == 0) {
								if (buddyiter->tempint2 == 0)
									firetalk_callback_im_buddyaway(c,buddyiter->nickname,"",0);
							}
							buddyiter = buddyiter->next;
						}
						break;
					case 319:
						firetalk_callback_gotchannels(c, args[3], args[4]);
						break;
					case 352:
						firetalk_callback_chat_list_extended(c, args[7], args[3], args[4], args[5], args[6], args[9]);
						break;
					case 366:
						firetalk_callback_chat_names(c, args[3]);
						break;
					case 401: /* ERR_NOSUCHNICK */
					case 441: /* ERR_USERNOTINCHANNEL */
					case 443: /* ERR_USERONCHANNEL */
						firetalk_callback_im_buddyonline(c,args[3],0);
						firetalk_callback_error(c,FE_BADUSER,args[3],args[4]);
						if (!strcasecmp(args[3],"NickServ") && c->passchange)
							c->passchange--;
						whoisiter = c->whois_head;
						whoisiter2 = NULL;
						while (whoisiter) {
							if (irc_compare_nicks(args[3],whoisiter->nickname) == 0) {
								free(whoisiter->nickname);
								if (whoisiter->info)
									free(whoisiter->info);
								if (whoisiter2)
									whoisiter2->next = whoisiter->next;
								else
									c->whois_head = whoisiter->next;
								free(whoisiter);
								break;
							}
							whoisiter2 = whoisiter;
							whoisiter = whoisiter->next;
						}
						break;
					case 403: /* ERR_NOSUCHCHANNEL */
					case 442: /* ERR_NOTONCHANNEL */
						firetalk_callback_error(c,FE_BADROOM,&args[3][1],args[4]);
						break;
					case 404: /* ERR_CANNOTSENDTOCHAN */
					case 405: /* ERR_TOOMANYCHANNELS */
					case 471: /* ERR_CHANNELISFULL */
					case 473: /* ERR_INVITEONLYCHAN */
					case 474: /* ERR_BANNEDFROMCHAN */
					case 475: /* ERR_BADCHANNELKEY */
					case 477:
						firetalk_callback_error(c,FE_ROOMUNAVAILABLE,&args[3][1],args[4]);
						break;
					case 412: /* ERR_NOTEXTTOSEND */
						firetalk_callback_error(c,FE_BADMESSAGE,NULL,args[4]);
						break;
					case 421: /* ERR_UNKNOWNCOMMAND */
						if (strcmp(args[3],"SILENCE") == 0)
							c->usesilence = 0;
						break;
					case 433: /* ERR_NICKNAMEINUSE */
						firetalk_callback_error(c,FE_BADUSER,NULL,"Nickname in use.");
						break;
					case 482: /* ERR_CHANOPRIVSNEEDED */
						firetalk_callback_error(c,FE_NOPERMS,&args[3][1],"You need to be a channel operator.");
						break;
				}
			}
		}
		if ((args[1] != NULL) && (args[2] != NULL) && (args[3] != NULL) && (args[4] != NULL)) {
			if (strcmp(args[1],"MODE") == 0) {
				tempint = 0;
				tempint2 = 4;
				tempint3 = 0;
				while ((args[tempint2] != NULL) && (args[3][tempint] != '\0')) {
					switch (args[3][tempint++]) {
						case '+':
							tempint3 = 1;
							break;
						case '-':
							tempint3 = -1;
							break;
						case 'o':
							if (tempint3 == 1) {
								firetalk_callback_chat_user_opped(c,args[2],args[tempint2++],irc_get_nickname(args[0]));
								if (irc_compare_nicks(args[tempint2-1],c->nickname) == FE_SUCCESS)
									firetalk_callback_chat_opped(c,args[2],irc_get_nickname(args[0]));
							} else if (tempint3 == -1) {
								firetalk_callback_chat_user_deopped(c,args[2],args[tempint2++],irc_get_nickname(args[0]));
								if (irc_compare_nicks(args[tempint2-1],c->nickname) == FE_SUCCESS) {
									firetalk_callback_chat_deopped(c,args[2],irc_get_nickname(args[0]));
									if (c->identified == 1) {
										/* this is us, and we're identified, so we can request a reop */
										if (irc_send_printf(c,"PRIVMSG ChanServ :OP %s %s",args[2],c->nickname) != FE_SUCCESS) {
											irc_internal_disconnect(c,FE_PACKET);
											return FE_PACKET;
										}
									}
								}
							}
							break;
						default:
							tempint2++;
							break;
					}
				}
			} else {
				switch (tempint) {
					case 317: /* RPL_WHOISIDLE */
						whoisiter = c->whois_head;
						while (whoisiter) {
							if (irc_compare_nicks(args[3],whoisiter->nickname) == 0)
								whoisiter->idle = atol(args[4])/60;
							whoisiter = whoisiter->next;
						}
						buddyiter = fchandle->buddy_head;
						while (buddyiter) {
							if (irc_compare_nicks(args[3],buddyiter->nickname) == 0)
								firetalk_callback_idleinfo(c,args[3],atol(args[4]) / 60);
							buddyiter = buddyiter->next;
						}
						break;
					case 332: /* RPL_TOPIC */
						firetalk_callback_chat_gottopic(c,args[3],args[4],NULL);
						break;
				}
			}
		}
		if ((args[1] != NULL) && (args[2] != NULL) && (args[3] != NULL) && (args[4] != NULL) && (args[5] != NULL)) {
			tempint = atoi(args[1]);
			if (tempint == 353) {
				tempchr2 = args[5];
				while (1) {
					if ((tempchr = strchr(tempchr2,' ')) != NULL)
						tempchr[0] = '\0';
					if (tempchr2[0] == '\0')
						break;
					if (tempchr2[0] == '@' || tempchr2[0] == '+') {
//                                                firetalk_callback_chat_user_joined(c,args[4],&tempchr2[1],0);
						firetalk_chat_internal_add_member(firetalk_find_handle(c),args[4],&tempchr2[1]);
						firetalk_callback_im_buddyonline(c,&tempchr2[1],1);
						if (tempchr2[0] == '@') {
							firetalk_callback_chat_user_opped(c,args[4],&tempchr2[1],NULL);
							if (irc_compare_nicks(&tempchr2[1],c->nickname) == FE_SUCCESS)
								firetalk_callback_chat_opped(c,args[4],NULL);
						}
					} else {
//                                                firetalk_callback_chat_user_joined(c,args[4],tempchr2,0);
						firetalk_chat_internal_add_member(firetalk_find_handle(c),args[4],tempchr2);
						firetalk_callback_im_buddyonline(c,tempchr2,1);
					}
					if (tempchr == NULL)
						break;
					tempchr2 = tempchr + 1;
				}
			}
		}
		if ((args[1] != NULL) && (args[2] != NULL) && (args[3] != NULL) && (args[4] != NULL) && (args[5] != NULL) && (args[6] != NULL) && (args[7] != NULL)) {
			switch (tempint) {
				case 311: /* RPL_WHOISUSER */
					whoisiter = c->whois_head;
					while (whoisiter) {
						if (irc_compare_nicks(args[3],whoisiter->nickname) == 0) {
							if (whoisiter->info)
								free(whoisiter->info);
							safe_snprintf(tempbuf,512,"%s@%s: %s",args[4],args[5],args[7]);
							whoisiter->info = safe_strdup(tempbuf);
						}
						whoisiter = whoisiter->next;
					}
				break;
			}
		}
		
		args = irc_recv_parse(c,buffer,bufferpos);
	}
	return FE_SUCCESS;
}

enum firetalk_error irc_got_data_connecting(client_t c, unsigned char * buffer, unsigned short * bufferpos) {
	char **args;

	args = irc_recv_parse(c,buffer,bufferpos);
	while (args) {
		if (args[0] == NULL) {
			firetalk_callback_connectfailed(c,FE_PACKET,args[0]);
			return FE_PACKET;
		}
		/* zero argument items */
		if (strcmp(args[0],"ERROR") == 0) {
			irc_send_printf(c,"QUIT :error");
			firetalk_callback_connectfailed(c,FE_PACKET,"Server returned ERROR");
			return FE_PACKET;
		}

		if (!args[1])
			continue;
		/* one argument items */
		if (strcmp(args[1],"ERROR") == 0) {
			irc_send_printf(c,"QUIT :error");
			firetalk_callback_connectfailed(c,FE_PACKET,"Server returned ERROR");
			return FE_PACKET;
		}
		if (strcmp(args[0],"PING") == 0) {
			if (irc_send_printf(c,"PONG %s",args[1]) != 0) {
				irc_send_printf(c,"QUIT :error");
				firetalk_callback_connectfailed(c,FE_PACKET,"Packet transfer error");
				return FE_PACKET;
			}
		} else {
			switch (atoi(args[1])) {
				case 376:
				case 422:
					firetalk_callback_doinit(c,c->nickname);
					firetalk_callback_connected(c);
					break;
				case 431:
				case 432:
				case 436:
				case 461:
					irc_send_printf(c,"QUIT :Invalid nickname");
					firetalk_callback_connectfailed(c,FE_BADUSER,"Invalid nickname");
					return FE_BADUSER;
				case 433:
					irc_send_printf(c,"QUIT :Invalid nickname");
					firetalk_callback_connectfailed(c,FE_BADUSER,"Nickname in use");
					return FE_BADUSER;
				case 465:
					irc_send_printf(c,"QUIT :banned");
					firetalk_callback_connectfailed(c,FE_BLOCKED,"You are banned");
					return FE_BLOCKED;
			}
		}
		args = irc_recv_parse(c,buffer,bufferpos);
	}

	return FE_SUCCESS;
}

static char irc_tolower(const char c) {
	if ((c >= 'A') && (c <= 'Z'))
		return (c - 'A') + 'a';
	if (c == '[')
		return '{';
	if (c == ']')
		return '{';
	if (c == '\\')
		return '|';
	return c;
}

enum firetalk_error irc_compare_nicks(const char * const nick1, const char * const nick2) {
	int i = 0;

	while (nick1[i] != '\0') {
		if (irc_tolower(nick1[i]) != irc_tolower(nick2[i]))
			return FE_NOMATCH;
		i++;
	}
	if (nick2[i] != '\0')
		return FE_NOMATCH;

	return FE_SUCCESS;
}

enum firetalk_error irc_chat_join(client_t c, const char * const room, const char * const passwd) {
	return irc_send_printf(c,"JOIN %s %s",room, passwd);
}

enum firetalk_error irc_chat_part(client_t c, const char * const room) {
	return irc_send_printf(c,"PART %s",room);
}

enum firetalk_error irc_chat_send_message(client_t c, const char * const room, const char * const message, const int auto_flag) {
	if (auto_flag == 1)
		return irc_send_printf(c,"NOTICE %s :%s",room,message);
	else
		return irc_send_printf(c,"PRIVMSG %s :%s",room,message);
}

enum firetalk_error irc_chat_send_action(client_t c, const char * const room, const char * const message, const int auto_flag) {
	if (auto_flag == 1)
		return irc_send_printf(c,"NOTICE %s :\001ACTION %s\001",room,message);
	else
		return irc_send_printf(c,"PRIVMSG %s :\001ACTION %s\001",room,message);
}

enum firetalk_error irc_chat_invite(client_t c, const char * const room, const char * const who, const char * const message) {
	return irc_send_printf(c,"INVITE %s %s",who,room);
}

enum firetalk_error irc_im_send_message(client_t c, const char * const dest, const char * const message, const int auto_flag) {
	if (auto_flag == 1)
		return irc_send_printf(c,"NOTICE %s :%s",dest,message);
	else
		return irc_send_printf(c,"PRIVMSG %s :%s",dest,message);
}

enum firetalk_error irc_im_send_action(client_t c, const char * const dest, const char * const message, const int auto_flag) {
	if (auto_flag == 1)
		return irc_send_printf(c,"NOTICE %s :\001ACTION %s\001",dest,message);
	else
		return irc_send_printf(c,"PRIVMSG %s :\001ACTION %s\001",dest,message);
}

enum firetalk_error irc_chat_set_topic(client_t c, const char * const room, const char * const topic) {
	return irc_send_printf(c,"TOPIC %s :%s",room,topic);
}

enum firetalk_error irc_chat_op(client_t c, const char * const room, const char * const who) {
	return irc_send_printf(c,"MODE %s +o %s",room,who);
}

enum firetalk_error irc_chat_deop(client_t c, const char * const room, const char * const who) {
	return irc_send_printf(c,"MODE %s -o %s",room,who);
}

enum firetalk_error irc_chat_kick(client_t c, const char * const room, const char * const who, const char * const reason) {
	if (reason)
		return irc_send_printf(c,"KICK %s %s :%s",room,who,reason);
	else
		return irc_send_printf(c,"KICK %s %s",room,who);
}

enum firetalk_error irc_chat_requestextended(client_t c, const char * const room) {
	return irc_send_printf(c,"WHO %s", room);
}

enum firetalk_error irc_im_add_buddy(client_t c, const char * const nickname) {
	c->isons++;
	return irc_send_printf(c,"ISON %s",nickname);
}

enum firetalk_error irc_im_remove_buddy(client_t c, const char * const nickname) {
	return FE_SUCCESS;
}

enum firetalk_error irc_im_add_deny(client_t c, const char * const nickname) {
	if (c->usesilence == 1)
		return irc_send_printf(c,"SILENCE +%s!*@*",nickname);
	else
		return FE_SUCCESS;
}

enum firetalk_error irc_im_remove_deny(client_t c, const char * const nickname) {
	if (c->usesilence == 1)
		return irc_send_printf(c,"SILENCE -%s!*@*",nickname);
	else
		return FE_SUCCESS;
}

enum firetalk_error irc_im_upload_buddies(client_t c) {
	return FE_SUCCESS;
}

enum firetalk_error irc_im_upload_denies(client_t c) {
	return FE_SUCCESS;
}

enum firetalk_error irc_im_evil(client_t c, const char * const who) {
	return FE_SUCCESS;
}

enum firetalk_error irc_im_searchemail(client_t c, const char * const email) {
	return irc_send_printf(c,"WHO %s",email);
}

enum firetalk_error irc_get_info(client_t c, const char * const nickname) {
	struct s_irc_whois *whoistemp;

	whoistemp = c->whois_head;
	c->whois_head = safe_malloc(sizeof(struct s_irc_whois));
	c->whois_head->nickname = safe_strdup(nickname);
	c->whois_head->flags = FF_NORMAL;
	c->whois_head->idle = 0;
	c->whois_head->info = NULL;
	c->whois_head->next = whoistemp;
	return irc_send_printf(c,"WHOIS %s",nickname);
}

enum firetalk_error irc_set_info(client_t c, const char * const info) {
	return FE_SUCCESS;
}

enum firetalk_error irc_set_away(client_t c, const char * const message) {
	if (message)
		return irc_send_printf(c,"AWAY :%s",message);
	else
		return irc_send_printf(c,"AWAY");
}

enum firetalk_error irc_periodic(struct s_firetalk_handle * const c) {
	client_t conn;
	struct s_firetalk_buddy *buddyiter;
	char obuf[1024];
	size_t i;

	obuf[0] = '\0';

	conn = c->handle;

	if (conn->lasttime > (time(NULL) - 20))
		return FE_IDLEFAST;

	if (conn->isons > 0)
		return FE_IDLEFAST;

	buddyiter = c->buddy_head;
	while (buddyiter) {
		if (strlen(obuf) + strlen(buddyiter->nickname) > 502) {
			i = strlen(obuf);
			if (i > 0)
				obuf[i-1] = '\0';
			if (irc_send_printf(conn,"ISON %s",obuf) != 0) {
				irc_internal_disconnect(conn,FE_PACKET);
				return FE_PACKET;
			}
			conn->isons++;
			obuf[0] = '\0';
		}
		safe_strncat(obuf,buddyiter->nickname,1024);
		safe_strncat(obuf," ",1024);
		buddyiter->tempint = 0;
		buddyiter = buddyiter->next;
	}

	i = strlen(obuf);
	if (i > 0) {
		obuf[i-1] = '\0';
		if (irc_send_printf(conn,"ISON %s",obuf) != 0) {
			irc_internal_disconnect(conn,FE_PACKET);
			return FE_PACKET;
		}
		conn->isons++;
	}

	time(&conn->lasttime);
	return FE_SUCCESS;
}

enum firetalk_error irc_subcode_send_request(client_t c, const char * const to, const char * const command, const char * const args) {
	if (args == NULL) {
		if (irc_send_printf(c,"PRIVMSG %s :\001%s\001",to,command) != 0) {
			irc_internal_disconnect(c,FE_PACKET);
			return FE_PACKET;
		}
	} else {
		if (irc_send_printf(c,"PRIVMSG %s :\001%s %s\001",to,command,args) != 0) {
			irc_internal_disconnect(c,FE_PACKET);
			return FE_PACKET;
		}
	}
	return FE_SUCCESS;
}

enum firetalk_error irc_subcode_send_reply(client_t c, const char * const to, const char * const command, const char * const args) {
	if (args == NULL) {
		if (irc_send_printf(c,"NOTICE %s :\001%s\001",to,command) != 0) {
			irc_internal_disconnect(c,FE_PACKET);
			return FE_PACKET;
		}
	} else {
		if (irc_send_printf(c,"NOTICE %s :\001%s %s\001",to,command,args) != 0) {
			irc_internal_disconnect(c,FE_PACKET);
			return FE_PACKET;
		}
	}
	return FE_SUCCESS;
}

const char * const irc_normalize_room_name(const char * const name) {
	static char newname[2048];
	if (strchr(ROOMSTARTS,name[0]))
		return name;
	safe_strncpy(newname,"#",2048);
	safe_strncat(newname,name,2048);
	return newname;
}
