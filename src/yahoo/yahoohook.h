#ifndef __YAHOOHOOK_H__
#define __YAHOOHOOK_H__

#include "icqcommon.h"
#include "icqcontact.h"

extern "C" {
#include "yahoolib.h"
}

class yahoohook {
    protected:
	struct yahoo_context *yahoo;
	string username;
	bool fonline;
	int ourstatus;
	imstatus manualstatus;

	time_t timer_reconnect;

	static void disconnected(yahoo_context *y);
	static void userlogon(yahoo_context *y, const char *nick);
	static void userlogoff(yahoo_context *y, const char *nick);
	static void userstatus(yahoo_context *y, const char *nick, int status);
	static void recvbounced(yahoo_context *y, const char *nick);
	static void recvmessage(yahoo_context *y, const char *nick, const char *msg);

	static struct tm *timestamp();

	imstatus yahoo2imstatus(int status) const;

    public:
	yahoohook();
	~yahoohook();

	void init(const string id, const string pass);
	void connect();
	void main();
	void exectimers();
	void disconnect();

	int getsockfd() const;
	bool online() const;
	bool logged() const;
	bool enabled() const;

	void sendnewuser(const imcontact ic);
	void removeuser(const imcontact ic);

	unsigned long sendmessage(const icqcontact *c, const string text);
	void setstatus(imstatus st);
	imstatus getstatus() const;
};

extern yahoohook yhook;

#endif
