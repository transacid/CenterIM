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
	struct reconnectInfo {
	    time_t timer;
	    int period;

	    reconnectInfo(): timer(0), period(40) {}
	};

	time_t timer_checkmail, timer_keypress, timer_update, timer_resend;
	bool regmode;

	static void handlesignal(int signum);

	void checkparallel();
	void inithooks();
	bool checkpasswords();
	void rereadstatus();

	string quotemsg(const string &text);

	void setauto(imstatus astatus);

    public:
	centericq();
	~centericq();

	void commandline(int argc, char **argv);
	void exec();
	void reg();
	void mainloop();
	void userinfo(const imcontact &cinfo);
	void changestatus();
	bool updateconf();

	void find();
	void checkmail();

	icqcontact *addcontact(const imcontact &ic);

	bool idle(int options = 0);
	void exectimers();

	void readevent(const imevent &ev, bool &enough, bool &fin);
	void readevents(const imcontact cont);

	void history(const imcontact &cont);

	bool sendevent(const imevent &ev, icqface::eventviewresult r);
};

extern centericq cicq;

string rusconv(const string &tdir, const string &text);
string rushtmlconv(const string &tdir, const string &text);
string ruscrlfconv(const string &tdir, const string &text);

bool ischannel(const imcontact &cont);

#endif
