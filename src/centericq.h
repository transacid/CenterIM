#ifndef __CENTERICQ_H__
#define __CENTERICQ_H__

#include <signal.h>
#include <sys/wait.h>
#include <fstream>

#include "kkstrtext.h"
#include "icqconf.h"
#include "icqcontact.h"

#include "eventmanager.h"
#include "icqface.h"

#define HIDL_SOCKEXIT   2

#define DEFAULT_CHARSET "ISO-8859-1"

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

	void checkmail();
	void checkconfigs();

	string quotemsg(const string &text);

	void setauto(imstatus astatus);
	void joinleave(icqcontact *c);
	void createconference(const imcontact &ic);

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

	icqcontact *addcontact(const imcontact &ic);

	bool idle(int options = 0);
	void exectimers();

	icqface::eventviewresult readevent(const imevent &ev,
	    const vector<icqface::eventviewresult> &buttons = vector<icqface::eventviewresult>());

	icqface::eventviewresult readevent(const imevent &ev, bool &enough, bool &fin,
	    const vector<icqface::eventviewresult> &buttons = vector<icqface::eventviewresult>());

	void readevents(const imcontact cont);

	void history(const imcontact &cont);

	bool sendevent(const imevent &ev, icqface::eventviewresult r);
};

extern centericq cicq;

string rusconv(const string &tdir, const string &text);
string rushtmlconv(const string &tdir, const string &text);
string ruscrlfconv(const string &tdir, const string &text);

bool ischannel(const imcontact &cont);

enum Encoding {
    encUTF, encKOI, encUnknown
};

Encoding guessencoding(const string &text);

#endif
