#ifndef __MSNHOOK_H__
#define __MSNHOOK_H__

#include "abstracthook.h"
#include "libmsn.h"

class msnhook : public abstracthook {
    protected:
	imstatus status;
	bool fonline;

	static void messaged(void *data);
	static void statuschanged(void *data);
	static void authrequested(void *data);
	static void disconnected(void *data);
	static void log(void *data);
	static void ring(void *data);
	static void mailed(void *data);

	static imstatus msn2imstatus(int st);
	static bool isourid(const string &nick);

    public:
	msnhook();
	~msnhook();

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
	void removeuser(const imcontact &ic);

	void setautostatus(imstatus st);
	imstatus getstatus() const;
};

extern msnhook mhook;

#endif
