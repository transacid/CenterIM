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
	time_t timer_checkmail, timer_keypress;

	static void handlesignal(int signum);

	void checkparallel();

	const string quotemsg(const string text);

    public:
	centericq();
	~centericq();

	bool message(const imcontact cinfo, const string text, msgmode mode);

	void commandline(int argc, char **argv);
	void exec();
	void reg();
	void mainloop();
	void userinfo(const imcontact cinfo);
	void changestatus();
	void updatedetails();
	void updateconf();
	void sendfiles(const imcontact cinfo);
	void sendcontacts(const imcontact cinfo);
	void find();
	void nonicq(int id);
	void checkmail();

	icqcontact *addcontact(const imcontact ic);

	bool idle(int options = 0);
	void exectimers();
	time_t getkeypress() const;
};

extern centericq cicq;

#endif
