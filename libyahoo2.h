#ifndef LIBYAHOO2_H
#define LIBYAHOO2_H

enum yahoo_status {
	YAHOO_STATUS_AVAILABLE = 0,
	YAHOO_STATUS_BRB,
	YAHOO_STATUS_BUSY,
	YAHOO_STATUS_NOTATHOME,
	YAHOO_STATUS_NOTATDESK,
	YAHOO_STATUS_NOTINOFFICE,
	YAHOO_STATUS_ONPHONE,
	YAHOO_STATUS_ONVACATION,
	YAHOO_STATUS_OUTTOLUNCH,
	YAHOO_STATUS_STEPPEDOUT,
	YAHOO_STATUS_INVISIBLE = 12,
	YAHOO_STATUS_CUSTOM = 99,
	YAHOO_STATUS_IDLE = 999,
	YAHOO_STATUS_OFFLINE = 0x5a55aa56, /* don't ask */
	YAHOO_STATUS_TYPING = 0x16
};
#define YAHOO_STATUS_GAME	0x2 		/* Games don't fit into the regular status model */
#define YAHOO_STATUS_AWAY 	0x80000000	/* OR'ed with YAHOO_STATUS_CUSTOM if away */
#define YAHOO_STATUS_AVAILABLE	0x00000000	/* OR'ed with YAHOO_STATUS_CUSTOM if available */

enum yahoo_login_status {
	YAHOO_LOGIN_OK = 0,
	YAHOO_LOGIN_PASSWD = 13,
	YAHOO_LOGIN_LOCK = 14
};

/* Yahoo style/color directives */
#define YAHOO_COLOR_BLACK "\033[30m"
#define YAHOO_COLOR_BLUE "\033[31m"
#define YAHOO_COLOR_LIGHTBLUE "\033[32m"
#define YAHOO_COLOR_GRAY "\033[33m"
#define YAHOO_COLOR_GREEN "\033[34m"
#define YAHOO_COLOR_PINK "\033[35m"
#define YAHOO_COLOR_PURPLE "\033[36m"
#define YAHOO_COLOR_ORANGE "\033[37m"
#define YAHOO_COLOR_RED "\033[38m"
#define YAHOO_COLOR_OLIVE "\033[39m"
#define YAHOO_STYLE_ITALICON "\033[2m"
#define YAHOO_STYLE_ITALICOFF "\033[x2m"
#define YAHOO_STYLE_BOLDON "\033[1m"
#define YAHOO_STYLE_BOLDOFF "\033[x1m"
#define YAHOO_STYLE_UNDERLINEON "\033[4m"
#define YAHOO_STYLE_UNDERLINEOFF "\033[x4m"
#define YAHOO_STYLE_URLON "\033[lm"
#define YAHOO_STYLE_URLOFF "\033[xlm"

struct yahoo_data {
	char *user;
	char *password;

	char *cookie_y;
	char *cookie_t;
	char *login_cookie;

	struct yahoo_buddy **buddies;
	char **identities;
	char *login_id;

	int fd;
	unsigned char *rxqueue;
	int rxlen;
	unsigned char *rawbuddylist;

	GHashTable *hash;
	GHashTable *games;
	int current_status;
	int initial_status;
	gboolean logged_in;

	guint32 id;

	guint32 client_id;
};

struct yahoo_buddy {
	char *group;
	char *id;
	char *real_name;
};

int  yahoo_connect(char *host, int port);
int  yahoo_get_fd(guint32 id);

guint32 yahoo_login(char *username, char *password, int initial);
void yahoo_logoff(guint32 id);
void yahoo_refresh(guint32 id);
void yahoo_keepalive(guint32 id);

void yahoo_send_im(guint32 id, char *who, char *what, int len);
void yahoo_send_typing(guint32 id, char *who, int typ);

void yahoo_set_away(guint32 id, enum yahoo_status state, char *msg, int away);

void yahoo_add_buddy(guint32 id, char *who, char *group);
void yahoo_remove_buddy(guint32 id, char *who, char *group);

void yahoo_conference_invite(guint32 id, char **who, char *room, char *msg);
void yahoo_conference_addinvite(guint32 id, char *who, char *room, char *msg);
void yahoo_conference_decline(guint32 id, char **who, char *room, char *msg);
void yahoo_conference_message(guint32 id, char **who, char *room, char *msg);
void yahoo_conference_logon(guint32 id, char **who, char *room);
void yahoo_conference_logoff(guint32 id, char **who, char *room);

void yahoo_send_file(guint32 id, char *who, char *msg, char *name, long size, int datafd);

void yahoo_pending(guint32 id, int fd);

enum yahoo_status yahoo_current_status(guint32 id);
struct yahoo_buddy **get_buddylist(guint32 id);
char **get_identities(guint32 id);
char *get_cookie(guint32 id, char *which);

int yahoo_get_url_handle(guint32 id, char *url);

#include "yahoo_httplib.h"

#endif
