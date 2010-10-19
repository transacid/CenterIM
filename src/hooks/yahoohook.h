#ifndef __YAHOOHOOK_H__
#define __YAHOOHOOK_H__

#include "abstracthook.h"

#ifdef BUILD_YAHOO

#include "icqconf.h"

#define USE_STRUCT_CALLBACKS
#include "yahoo2_callbacks.h"

class yahoohook: public abstracthook {
    protected:
	enum Action {
	    tbdConfLogon
	};

	struct connect_callback_data {
	    yahoo_connect_callback callback;
	    void *callback_data;
	    int id, tag;
	};
	
	struct y_c {
	    int fd;
	    bool ssl;
	    int state;
	    y_c(int afd, bool assl, int astate) : fd(afd), ssl(assl), state(astate) { }
	};

	struct yfd {
	    struct y_c con;
	    int tag;
	    void *data;
	    bool isconnect;
	    yahoo_input_condition cond;

	    static int connection_tags;

	    yfd(int afd, void *adata, bool assl, yahoo_input_condition acond, bool aisconnect = false)
		: con(afd, assl, 0), tag(connection_tags++), data(adata), isconnect(aisconnect), cond(acond) {}

	    bool operator == (int atag) const { return tag == atag; }
	    bool operator != (int atag) const { return tag != atag; }
	};

	vector<yfd> fds;
	vector<pair<Action, string> > tobedone;

	vector<char *> sfiles;
	map<const char *, imfile> srfiles;

	map<imfile, string> fvalid;
	map<string, string> awaymessages;

	bool fonline, flogged, searchonlineonly;
	map<string, vector<string> > confmembers;
	imstatus ourstatus;
	int cid;

	time_t timer_refresh, timer_close;

	static void login_response(int id, int succ, const char *url);
	static void got_buddies(int id, YList *buds);
	static void got_identities(int id, YList *ids);
	static void status_changed(int id, const char *who, int stat, const char *msg, int away, int idle, int mobile);
	static void got_im(int id, const char *me, const char *who, const char *msg, long tm, int stat, int utf8);
	static void got_conf_invite(int id, const char *me, const char *who, const char *room, const char *msg, YList *members);
	static void conf_userdecline(int id, const char *me, const char *who, const char *room, const char *msg);
	static void conf_userjoin(int id, const char *me, const char *who, const char *room);
	static void conf_userleave(int id, const char *me, const char *who, const char *room);
	static void conf_message(int id, const char *me, const char *who, const char *room, const char *msg, int utf8);
	static void got_file(int id, const char *me, const char *who, const char *msg, const char *fname, unsigned long fesize, char *trid);
	static void contact_added(int id, const char *myid, const char *who, const char *msg);
	static void typing_notify(int id, const char *me, const char *who, int stat);
	static void game_notify(int id, const char *me, const char *who, int stat, const char *msg);
	static void mail_notify(int id, const char *from, const char *subj, int cnt);
	static void system_message(int id, const char *me, const char *who, const char *msg);
	static void got_ping(int id, const char *msg);
	static void error(int id, const char *err, int fatal, int num);
	static void got_ignore(int id, YList * igns);
	static void got_cookies(int id);
	static void got_search_result(int id, int found, int start, int total, YList *contacts);
	static void chat_cat_xml(int id, const char *xml);
	static void chat_join(int id, const char *me, const char *room, const char *topic, YList *members, void *fd);
	static void chat_userjoin(int id, const char *me, const char *room, struct yahoo_chat_member *who);
	static void chat_userleave(int id, const char *me, const char *room, const char *who);
	static void chat_message(int id, const char *me, const char *who, const char *room, const char *msg, int msgtype, int utf8);
	static void rejected(int id, const char *who, const char *msg);
	static void got_webcam_image(int id, const char * who, const unsigned char *image, unsigned int image_size, unsigned int real_size, unsigned int timestamp);
	static void webcam_invite(int id, const char *me, const char *from);
	static void webcam_invite_reply(int id, const char *me, const char *from, int accept);
	static void webcam_closed(int id, const char *who, int reason);
	static void webcam_viewer(int id, const char *who, int connect);
	static void webcam_data_request(int id, int send);
	static void got_buddyicon_request(int id, const char *me, const char *who);
	static void got_buddyicon(int id, const char *me, const char *who, const char *url, int checksum);
	static void auth_request(int id, const char *who, const char *msg);
	static void auth_response(int id, const char *who, char granted, const char *msg);
	static void buddyicon_uploaded(int id, const char *url);
	static void chat_yahooerror(int id, const char *me);
	static void chat_yahoologout(int id, const char *me);
	static int yahoo_connect(const char *host, int port);
	static void file_transfer_done(int id, int result, void *data);
	static char *get_ip_addr(const char *domain);
	static void got_buddy_change_group(int id, const char *me, const char *who, const char *old_group, const char *new_group);
	static void got_buddyicon_checksum(int id, char const *me, const char *who, int checksum);
	static void got_buzz(int id, const char *me, const char *who, long tm);
	static void got_ft_data(int id, const unsigned char *in, int len, void *data);
	
	static int ylog(const char *fmt, ...);

	static int add_handler(int id, void *fd, yahoo_input_condition cond, void *data);
	static void remove_handler(int id, int tag);
	static int connect_async(int id, const char *host, int port, yahoo_connect_callback callback, void *data, int use_ssl);
	static int y_write(void *fd, char *buf, int len);
	static int y_read(void *fd, char *buf, int len);
	static void y_close(void *fd);

	static void get_fd(int id, void *fd, int error, void *data);
	static void get_url(int id, int fd, int error, const char *filename,
	    unsigned long size, void *data);

	static struct tm *timestamp();
	static imstatus yahoo2imstatus(int status);

	string decode(string text, bool utf);

	YList *getmembers(const string &room);
	void userstatus(const string &nick, int st, const string &message, bool away);
	void disconnected();

	void checkinlist(imcontact ic);
	void sendnewuser(const imcontact &ic, bool report);
	void removeuser(const imcontact &ic, bool report);

	void connect_complete(void *data, struct y_c *con);

    public:
	yahoohook();
	~yahoohook();

	void init();

	void connect();
	void main();
	void exectimers();
	void disconnect();

	void getsockets(fd_set &rs, fd_set &ws, fd_set &es, int &hsocket) const;
	bool isoursocket(fd_set &rs, fd_set &ws, fd_set &es) const;

	bool online() const;
	bool logged() const;
	bool isconnecting() const;
	bool enabled() const;

	void sendnewuser(const imcontact &ic);
	void removeuser(const imcontact &ic);
	void requestinfo(const imcontact &ic);

	bool send(const imevent &ev);

	void setautostatus(imstatus st);
	imstatus getstatus() const;

	void lookup(const imsearchparams &params, verticalmenu &dest);

	bool knowntransfer(const imfile &fr) const;
	void replytransfer(const imfile &fr, bool accept, const string &localpath = string());
	void aborttransfer(const imfile &fr);

	void requestawaymsg(const imcontact &ic);
	void conferencecreate(const imcontact &confid, const vector<imcontact> &lst);

	void updatecontact(icqcontact *c);
	void renamegroup(const string &oldname, const string &newname);
};

extern yahoohook yhook;

#endif

#endif
