#ifndef __IRCHOOK_H__
#define __IRCHOOK_H__

#include "abstracthook.h"

extern "C" {
#include "firetalk.h"
}

class irchook: public abstracthook {
    private:
	struct roomInfo {
	    string name;
	    vector<string> nicks;
	    bool fetched;

	    roomInfo(const string &aname): name(aname), fetched(false) {}

	    bool operator != (const string &aname) const;
	    bool operator == (const string &aname) const;

	    bool operator != (const bool &afetched) const;
	    bool operator == (const bool &afetched) const;
	};

    protected:
	bool fonline, flogged;
	firetalk_t handle;
	imstatus ourstatus;

	vector<char *> userlist;
	vector<roomInfo> rooms;

	void userstatus(const string &nickname, imstatus st);
	void processnicks();

	static void connected(void *conn, void *cli, ...);
	static void disconnected(void *conn, void *cli, ...);
	static void newnick(void *conn, void *cli, ...);
	static void gotinfo(void *conn, void *cli, ...);
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
};

extern irchook irhook;

#endif
