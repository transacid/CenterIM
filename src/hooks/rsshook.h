#ifndef __RSSHOOK_H__
#define __RSSHOOK_H__

#include "abstracthook.h"

#ifdef BUILD_RSS

#include "HTTPClient.h"
#include <libicq2000/Xml.h>

class rsshook: public abstracthook, public sigslot::has_slots<> {
    protected:
	HTTPClient httpcli;
	vector<int> rfds, wfds, efds;
	time_t timer_check;

	void socket_cb(SocketEvent *ev);
	void messageack_cb(MessageEvent *ev);
	void logger_cb(LogEvent *ev);

	void fetchRSSParam(string &base, XmlBranch *i, const string &enc,
	    const string &name, const string &title,
	    const string &postfix = "\n");

    public:
	rsshook();
	~rsshook();

	void init();

	void exectimers();
	void main();

	void getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const;
	bool isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const;

	bool online() const;
	bool logged() const;
	bool isconnecting() const;
	bool enabled() const;

	void requestinfo(const imcontact &c);
	void requestversion(const imcontact &c);
	void ping(const imcontact &c);
};

extern rsshook rhook;

#endif

#endif