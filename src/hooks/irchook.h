#ifndef __IRCHOOK_H__
#define __IRCHOOK_H__

#include "abstracthook.h"

extern "C" {
#include "firetalk.h"
}

class irchook: public abstracthook {
    public:
	struct channelInfo {
	    string name;
	    bool joined, fetched, contactlist;

	    vector<string> nicks;

	    channelInfo(const string &aname):
		name(aname), joined(false), fetched(false) {}

	    bool operator != (const string &aname) const;
	    bool operator == (const string &aname) const;
	};

    protected:
	enum searchMode {
	    Channel,
	    Email
	};

	searchMode smode;

	bool fonline, flogged;
	firetalk_t handle;
	imstatus ourstatus;
	string ircname, emailsub, namesub;

	vector<channelInfo> channels;
	vector<string> searchchannels, extlisted;
	map<string, string> awaymessages;
	vector<icqcontact *> foundguys;

	void userstatus(const string &nickname, imstatus st);
	void processnicks();

	void saveconfig() const;
	void loadconfig();

	static void connected(void *conn, void *cli, ...);
	static void disconnected(void *conn, void *cli, ...);
	static void newnick(void *conn, void *cli, ...);
	static void gotinfo(void *conn, void *cli, ...);
	static void gotchannels(void *conn, void *cli, ...);
	static void getmessage(void *conn, void *cli, ...);
	static void buddyonline(void *conn, void *cli, ...);
	static void buddyoffline(void *conn, void *cli, ...);
	static void buddyaway(void *conn, void *cli, ...);
	static void connectfailed(void *connection, void *cli, ...);
	static void listmember(void *connection, void *cli, ...);
	static void log(void *connection, void *cli, ...);
	static void chatnames(void *connection, void *cli, ...);
	static void listextended(void *connection, void *cli, ...);
	static void endextended(void *connection, void *cli, ...);
	static void chatmessage(void *connection, void *cli, ...);
	static void chatjoined(void *connection, void *cli, ...);
	static void chatleft(void *connection, void *cli, ...);
	static void chatkicked(void *connection, void *cli, ...);
	static void errorhandler(void *connection, void *cli, ...);
	static void nickchanged(void *connection, void *cli, ...);

	void rawcommand(const string &cmd);
	void channelfatal(string room, const char *fmt, ...);

    public:
	irchook();
	~irchook();

	void init();

	void connect();
	void disconnect();
	void exectimers();
	void main();

	void getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const;
	bool isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const;

	bool online() const;
	bool logged() const;
	bool enabled() const;
	bool isconnecting() const;

	bool send(const imevent &ev);

	void sendnewuser(const imcontact &c);
	void removeuser(const imcontact &ic);

	void setautostatus(imstatus st);

	imstatus getstatus() const;

	void requestinfo(const imcontact &c);
	void requestawaymsg(const imcontact &c);
	void sendupdateuserinfo(icqcontact &c, const string &newpass);

	void lookup(const imsearchparams &params, verticalmenu &dest);

	vector<channelInfo> getautochannels() const;
	void setautochannels(vector<channelInfo> &achannels);
};

extern irchook irhook;

#endif
