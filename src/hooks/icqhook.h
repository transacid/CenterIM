#ifndef __ICQHOOK_H__
#define __ICQHOOK_H__

#include "abstracthook.h"
#include "Client.h"

using namespace ICQ2000;

class icqhook: public abstracthook, public SigC::Object {
    protected:
	Client cli;
	set<int> fds;
	time_t timer_reconnect, timer_ping, timer_resolve;
	bool fonline, flogged;
	unsigned int reguin;

	void connected_cb(ConnectedEvent *ev);
	void disconnected_cb(DisconnectedEvent *ev);
	bool messaged_cb(MessageEvent *ev);
	void messageack_cb(MessageEvent *ev);
	void contactlist_cb(ContactListEvent *ev);
	void newuin_cb(NewUINEvent *ev);
	void rate_cb(RateInfoChangeEvent *ev);
	void logger_cb(LogEvent *ev);
	void socket_cb(SocketEvent *ev);
	void statuschanged_cb(MyStatusChangeEvent *ev);

	imstatus icq2imstatus(const Status st) const;
	const string getcountryname(int code) const;

	void resolve();

    public:
	icqhook();
	~icqhook();

	void init();

	void connect();
	void disconnect();
	void exectimers();
	void main();

	void getsockets(fd_set &fds, int &hsocket) const;
	bool isoursocket(fd_set &fds) const;

	bool online() const;
	bool logged() const;
	bool isconnecting() const;
	bool enabled() const;

	bool send(const imevent &ev);
	void sendnewuser(const imcontact c);

	void setautostatus(imstatus st);
	imstatus getstatus() const;

	void requestinfo(const imcontact c);

	bool regconnect(const string aserv);
	bool regattempt(unsigned int &auin, const string apassword);
};

extern icqhook ihook;

#endif
