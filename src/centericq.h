#ifndef __CENTERICQ_H__
#define __CENTERICQ_H__

#include <signal.h>
#include <sys/wait.h>
#include <fstream>

#include "kkstrtext.h"
#include "kkfsys.h"
#include "icqcontact.h"

#include "eventmanager.h"
#include "icqface.h"

#define HIDL_SOCKEXIT   2

class centericq {
    public:
	enum msgmode {
	    reply,
	    forward,
	    scratch
	};

    protected:
	time_t timer_checkmail, timer_keypress, timer_update, timer_resend;
	bool regmode;

	static void handlesignal(int signum);

	void checkparallel();
	void inithooks();
	bool checkpasswords();

	const string quotemsg(const string text);

	void setauto(imstatus astatus);

    public:
	centericq();
	~centericq();

	void commandline(int argc, char **argv);
	void exec();
	void reg();
	void mainloop();
	void userinfo(const imcontact cinfo);
	void changestatus();
	bool updateconf();

	void find();
	void checkmail();

	icqcontact *addcontact(const imcontact ic);

	bool idle(int options = 0);
	void exectimers();

	void readevents(const imcontact &cont);
	void history(const imcontact &cont);

	void sendevent(const imevent &ev, icqface::eventviewresult r);
};

extern centericq cicq;

const string rusconv(const string tdir, const string text);

#endif
