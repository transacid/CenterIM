#ifndef __JABBERHOOK_H__
#define __JABBERHOOK_H__

#include "abstracthook.h"

#include "jabber.h"

class jabberhook: public abstracthook {
    protected:
	imstatus status;
	jconn jc;

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

	void requestinfo(const imcontact &c);
};

extern jabberhook jhook;

#endif
