#ifndef __YAHOOHOOK_H__
#define __YAHOOHOOK_H__

#include "abstracthook.h"

#ifdef BUILD_YAHOO

#include "icqconf.h"
#include "yahoo2_callbacks.h"

class yahoohook: public abstracthook {
    protected:
	enum Action {
	    tbdConfLogon
	};

	struct connect_callback_data {
	    yahoo_connect_callback callback;
	    void *callback_data;
	    int id;
	};

	struct yfd {
	    int fd;
	    void *data;
	    yfd(int afd, void *adata): fd(afd), data(adata) {}

	    bool operator == (int afd) const { return fd == afd; }
	    bool operator != (int afd) const { return fd != afd; }
	};

	vector<yfd> rfds, wfds;
	vector<pair<Action, string> > tobedone;

	map<string, Encoding> userenc;
	map<imfile, string> fvalid;
	map<string, string> awaymessages;

	bool fonline, flogged;
	map<string, vector<string> > confmembers;
	imstatus ourstatus;
	int cid;

	time_t timer_refresh;

	static void login_response(int id, int succ, char *url);
	static void got_buddies(int id, YList *buds);
	static void status_changed(int id, char *who, int stat, char *msg, int away);
	static void got_im(int id, char *who, char *msg, long tm, int stat, int utf8);
	static void got_conf_invite(int id, char *who, char *room, char *msg, YList *members);
	static void conf_userdecline(int id, char *who, char *room, char *msg);
	static void conf_userjoin(int id, char *who, char *room);
	static void conf_userleave(int id, char *who, char *room);
	static void conf_message(int id, char *who, char *room, char *msg, int utf8);
	static void got_file(int id, char *who, char *url, long expires, char *msg, char *fname, unsigned long fesize);
	static void contact_added(int id, char *myid, char *who, char *msg);
	static void typing_notify(int id, char *who, int stat);
	static void game_notify(int id, char *who, int stat);
	static void mail_notify(int id, char *from, char *subj, int cnt);
	static void system_message(int id, char *msg);
	static void error(int id, char *err, int fatal);
	static void add_handler(int id, int fd, yahoo_input_condition cond, void *data);
	static void remove_handler(int id, int fd);
	static int connect_async(int id, char *host, int port, yahoo_connect_callback callback, void *data);

	static struct tm *timestamp();
	static imstatus yahoo2imstatus(int status);

	string encanalyze(const string &nick, const string &text);

	YList *getmembers(const string &room);
	void userstatus(const string &nick, int st, const string &message, bool away);
	void disconnected();

	void checkinlist(imcontact ic);
	void sendnewuser(const imcontact &ic, bool report);
	void removeuser(const imcontact &ic, bool report);

	void connect_complete(void *data, int source, yahoo_input_condition condition);

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

	vector<icqcontact *> getneedsync();

	bool knowntransfer(const imfile &fr) const;
	void replytransfer(const imfile &fr, bool accept, const string &localpath = string());
	void aborttransfer(const imfile &fr);

	void requestawaymsg(const imcontact &ic);
	void conferencecreate(const imcontact &confid, const vector<imcontact> &lst);
};

extern yahoohook yhook;

#endif

#endif
