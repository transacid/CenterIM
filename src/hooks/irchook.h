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
	    bool joined, fetched;
	    vector<string> nicks;

	    channelInfo(const string &aname):
		name(aname), joined(false), fetched(false) {}

	    bool operator != (const string &aname) const;
	    bool operator == (const string &aname) const;
	};

    protected:
	bool fonline, flogged;
	firetalk_t handle;
	imstatus ourstatus;

	vector<char *> userlist;
	vector<channelInfo> channels;
	vector<string> searchchannels;

	void userstatus(const string &nickname, imstatus st);
	void processnicks();

	static void connected(void *conn, void *cli, ...);
	static void disconnected(void *conn, void *cli, ...);
	static void newnick(void *conn, void *cli, ...);
	static void gotinfo(void *conn, void *cli, ...);
	static void gotchannels(void *conn, void *cli, ...);
	static void getmessage(void *conn, void *cli, ...);
	static void buddyonline(void *conn, void *cli, ...);
	static void buddyoffline(void *conn, void *cli, ...);
	static void buddyaway(void *conn, void *cli, ...);
	static void buddynickchanged(void *conn, void *cli, ...);
	static void connectfailed(void *connection, void *cli, ...);
	static void listmember(void *connection, void *cli, ...);
	static void log(void *connection, void *cli, ...);
	static void chatnames(void *connection, void *cli, ...);

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
	void sendupdateuserinfo(icqcontact &c, const string &newpass);

	void lookup(const imsearchparams &params, verticalmenu &dest);

	vector<channelInfo> getautochannels() const;
	void setautochannels(vector<channelInfo> &achannels);
};

extern irchook irhook;

#endif
