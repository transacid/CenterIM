#ifndef __CENTERICQ_H__
#define __CENTERICQ_H__

#include "icq.h"
#include <signal.h>
#include <sys/wait.h>
#include <fstream>

#include "kkstrtext.h"
#include "kkfsys.h"
#include "icqcontact.h"

#include "icqcommon.h"

class centericq {
    public:
	enum msgmode {
	    reply,
	    forward,
	    scratch
	};

    protected:
	static void handlesignal(int signum);

	void checkparallel();
	const string quotemsg(const string text);

    public:
	centericq();
	~centericq();

	bool message(unsigned int uin, const string text, msgmode mode);

	void commandline(int argc, char **argv);
	void exec();
	void reg();
	void mainloop();
	void userinfo(unsigned int uin, bool nonicq = false);
	void changestatus();
	void updatedetails();
	void updateconf();
	void sendfiles(unsigned int uin);
	void sendcontacts(unsigned int uin);
	void find();
	void nonicq(int id);
	void checkmail();

	icqcontact *adduin(unsigned int uin);
};

extern centericq cicq;

#endif
