#ifndef __MSNHOOK_H__
#define __MSNHOOK_H__

#include "abstracthook.h"
#include "libmsn.h"

class msnhook : public abstracthook {
    protected:
	time_t timer_reconnect;
	imstatus status;
	bool fonline;

	static void messaged(void *data);
	static void statuschanged(void *data);
	static void authrequested(void *data);
	static void disconnected(void *data);
	static void log(void *data);
	static void ring(void *data);

	imstatus msn2imstatus(int st) const;

    public:
	msnhook();
	~msnhook();

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
	void removeuser(const imcontact ic);

	void setautostatus(imstatus st);
	imstatus getstatus() const;
};

extern msnhook mhook;

#endif
