#ifndef __YAHOOHOOK_H__
#define __YAHOOHOOK_H__

#include "icqcommon.h"

extern "C" {
#include "yahoolib.h"
}

class yahoohook {
    protected:
	struct yahoo_context *yahoo;
	string username;

	static void disconnected(yahoo_context *y);
	static void userlogon(yahoo_context *y, const char *nick);
	static void userlogoff(yahoo_context *y, const char *nick);
	static void userstatus(yahoo_context *y, const char *nick);
	static void recvbounced(yahoo_context *y, const char *nick);
	static void recvmessage(yahoo_context *y, const char *nick, const char *msg);

	static struct tm *timestamp();

    public:
	yahoohook();
	~yahoohook();

	void init(const string id, const string pass);
	void connect();
	void main();
	void exectimers();
	void disconnect();

	int getsockfd() const;

	void sendmessage(const string nick, const string msg);
};

extern yahoohook yhook;

#endif
