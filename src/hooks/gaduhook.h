#ifndef __GADUHOOK_H__
#define __GADUHOOK_H__

#include "abstracthook.h"

#ifdef BUILD_GADU

struct gg_session;

class gaduhook: public abstracthook {
    protected:
	time_t timer_ping;
	struct gg_session *sess;
	bool flogged;
	map<long unsigned int, string> awaymsgs;

	void searchdone(void *p);
	void userstatuschange(unsigned int uin, int status, const char *desc);
	void userlistsend();
	void usernotify(unsigned int uin, int status, const char *desc,
	    unsigned int ip, int port, int version);

	string handletoken(struct gg_http *h);

	void cutoff();

    public:
	gaduhook();
	~gaduhook();

	void init();

	void connect();
	void disconnect();
	void exectimers();
	void main();

	void getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const;
	bool isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const;

	bool online() const;
	bool logged() const;
	bool isconnecting() const;
	bool enabled() const;

	bool send(const imevent &ev);

	void sendnewuser(const imcontact &c);
	void removeuser(const imcontact &c);

	void setautostatus(imstatus st);
	imstatus getstatus() const;

	void requestinfo(const imcontact &c);
	void requestawaymsg(const imcontact &ic);

	bool regconnect(const string &aserv);
	bool regattempt(unsigned int &auin, const string &apassword, const string &email);

	void lookup(const imsearchparams &params, verticalmenu &dest);
	void sendupdateuserinfo(const icqcontact &c);
};

extern gaduhook ghook;

#endif

#endif
