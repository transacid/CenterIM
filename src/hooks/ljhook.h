#ifndef __LJHOOK_H__
#define __LJHOOK_H__

#include "abstracthook.h"

#ifdef BUILD_LJ

#include "HTTPClient.h"

class ljhook: public abstracthook, public sigslot::has_slots<> {
    protected:
	HTTPClient httpcli;

	string baseurl, md5pass, username;
	vector<int> rfds, wfds, efds;
	bool fonline, flogged;
	imcontact self;
	time_t timer_getfriends;

	verticalmenu *sdest;
	string lookfor;

	enum RequestType {
	    reqLogin,
	    reqPost,
	    reqPostComment,
	    reqGetFriends,
	    reqDelFriend,
	    reqAddFriend,
	    reqLookup,
	    reqDone
	};

	map<HTTPRequestEvent *, RequestType> sent;
	vector<string> friendof, journals, moods, pictures;

	void socket_cb(SocketEvent *ev);
	void messageack_cb(MessageEvent *ev);
	void logger_cb(LogEvent *ev);

	void requestfriends();

	string getfeedurl(const string &nick) const;
	string getoldfeedurl(const string &nick) const;

    public:
	ljhook();
	~ljhook();

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

	bool send(const imevent &sev);

	void sendnewuser(const imcontact &ic);
	void removeuser(const imcontact &ic);

	void setautostatus(imstatus st);
	imstatus getstatus() const;

	void requestinfo(const imcontact &c);

	void lookup(const imsearchparams &params, verticalmenu &dest);
	void stoplookup();

	void requestawaymsg(const imcontact &c);
	void requestversion(const imcontact &c);
	void ping(const imcontact &c);

	void ouridchanged(const icqconf::imaccount &ia);

	void updatecontact(icqcontact *c);

	vector<string> getjournals() const { return journals; }
	vector<string> getmoods() const { return moods; }
	vector<string> getpictures() const { return pictures; }
};

extern ljhook lhook;

#endif

#endif
