#ifndef __JABBERHOOK_H__
#define __JABBERHOOK_H__

#include "abstracthook.h"

#include "jabber.h"

class jabberhook: public abstracthook {
    protected:
	imstatus ourstatus;
	jconn jc;
	int id;

	bool flogged, regmode, regdone;
	string regerr;

	vector<string> roster;
	map<string, string> awaymsgs;

	static void statehandler(jconn conn, int state);
	static void packethandler(jconn conn, jpacket packet);
	static void jlogger(jconn conn, int inout, const char *p);

	static string jidnormalize(const string &jid);
	static string jidtodisp(const string &jid);
	static void jidsplit(const string &jid, string &user, string &host, string &rest);

	void setjabberstatus(imstatus st, const string &msg);
	void checkinlist(imcontact ic);

	void sendnewuser(const imcontact &c, bool report);
	void removeuser(const imcontact &ic, bool report);

    public:
	jabberhook();
	~jabberhook();

	void init();

	void connect();
	void disconnect();
	void main();

	void getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const;
	bool isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const;

	bool online() const;
	bool logged() const;
	bool isconnecting() const;
	bool enabled() const;

	bool send(const imevent &ev);

	void sendnewuser(const imcontact &c);
	void removeuser(const imcontact &ic);

	void setautostatus(imstatus st);
	imstatus getstatus() const;

	void requestinfo(const imcontact &c);
	void requestawaymsg(const imcontact &c);

	bool regnick(const string &nick, const string &pass,
	    const string &serv, string &err);
};

extern jabberhook jhook;

#endif
