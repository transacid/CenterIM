#ifndef __ICQHOOK_H__
#define __ICQHOOK_H__

#include "abstracthook.h"

#include "libicq2000/Client.h"
#include "libicq2000/events.h"

using namespace ICQ2000;

class icqhook: public abstracthook, public SigC::Object {
    protected:
	Client cli;

	vector<int> rfds, wfds, efds;

	time_t timer_reconnect, timer_poll, timer_resolve;
	bool fonline, flogged;
	unsigned int reguin;
	SearchResultEvent *searchevent;

	void connected_cb(ConnectedEvent *ev);
	void disconnected_cb(DisconnectedEvent *ev);
	bool messaged_cb(MessageEvent *ev);
	void messageack_cb(MessageEvent *ev);
	void contactlist_cb(ContactListEvent *ev);
	void newuin_cb(NewUINEvent *ev);
	void rate_cb(RateInfoChangeEvent *ev);
	void logger_cb(LogEvent *ev);
	void socket_cb(SocketEvent *ev);
	void want_auto_resp_cb(AwayMessageEvent *ev);
	void search_result_cb(SearchResultEvent *ev);
	void self_event_cb(SelfEvent *ev);

	imstatus icq2imstatus(const Status st) const;

	void resolve();
	void sendinvisible();

    public:
	icqhook();
	~icqhook();

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
	void sendnewuser(const imcontact c);

	void setautostatus(imstatus st);
	imstatus getstatus() const;

	void requestinfo(const imcontact c);
	void requestawaymsg(const imcontact &c);

	bool regconnect(const string aserv);
	bool regattempt(unsigned int &auin, const string apassword);

	void lookup(const imsearchparams &params, verticalmenu &dest);
	void sendupdateuserinfo(const icqcontact &c);
};

extern icqhook ihook;

#endif
