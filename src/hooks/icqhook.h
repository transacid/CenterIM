#ifndef __ICQHOOK_H__
#define __ICQHOOK_H__

#include "abstracthook.h"
#include "icqmlist.h"
#include "icqcontacts.h"

#include "libicq2000/Client.h"
#include "libicq2000/events.h"

using namespace ICQ2000;

class icqhook: public abstracthook, public SigC::Object {
    protected:
	Client cli;

	vector<int> rfds, wfds, efds;

	enum {
	    reqUpload,
	    ackUpload,
	    reqFetch,
	    ackFetch
	} syncstatus;

	enum {
	    Normal,
	    SyncList
	} blockmode;

	time_t timer_poll, timer_resolve;
	bool fonline, flogged;
	unsigned int reguin;
	SearchResultEvent *searchevent;

	typedef pair<unsigned int, contactstatus> visInfo;
	vector<visInfo> vislist;
	vector<icqcontact *> foundguys;

	void connected_cb(ConnectedEvent *ev);
	void disconnected_cb(DisconnectedEvent *ev);
	void messaged_cb(MessageEvent *ev);
	void messageack_cb(MessageEvent *ev);
	void contactlist_cb(ContactListEvent *ev);
	void contact_userinfo_change_signal_cb(UserInfoChangeEvent *ev);
	void contact_status_change_signal_cb(StatusChangeEvent *ev);
	void newuin_cb(NewUINEvent *ev);
	void rate_cb(RateInfoChangeEvent *ev);
	void logger_cb(LogEvent *ev);
	void socket_cb(SocketEvent *ev);
	void want_auto_resp_cb(ICQMessageEvent *ev);
	void search_result_cb(SearchResultEvent *ev);
	void server_based_contact_list_cb(ServerBasedContactEvent *ev);
	void self_contact_userinfo_change_cb(UserInfoChangeEvent *ev);
	void self_contact_status_change_cb(StatusChangeEvent *ev);

	imstatus icq2imstatus(const Status st) const;

	void resolve();
	void sendinvisible();
	void updateinforecord(ContactRef ic, icqcontact *c);
	void processemailevent(const string &sender, const string &email, const string &message);

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

	void sendnewuser(const imcontact &c);
	void removeuser(const imcontact &c);

	void setautostatus(imstatus st);
	imstatus getstatus() const;

	void requestinfo(const imcontact &c);
	void requestawaymsg(const imcontact &c);

	bool regconnect(const string &aserv);
	bool regattempt(unsigned int &auin, const string &apassword);

	void lookup(const imsearchparams &params, verticalmenu &dest);
	void sendupdateuserinfo(const icqcontact &c);

	void synclist();
	void getsyncstatus(int &synchronized, vector<icqcontact *> &tobestored, vector<icqcontact *> &needauth);
	void sendaddauth();
};

extern icqhook ihook;

#endif
