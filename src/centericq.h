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

	    reconnectInfo(): timer(0), period(30) {}
	};

	time_t timer_checkmail, timer_keypress, timer_update, timer_resend, timer_autosave;
	bool regmode;

	map<string, time_t> configstats;
	map<protocolname, reconnectInfo> reconnect;

	static void handlesignal(int signum);

	void checkparallel();
	void inithooks();
	bool checkpasswords();
	void rereadstatus();

	void checkmail();
	void checkconfigs();

	string quotemsg(const string &text);

	void setauto(imstatus astatus);
	void createconference(const imcontact &ic);

	void defaultcontacts(bool rus);
	void massmove();

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
	void joindialog();
	void linkfeed();

	icqcontact *addcontact(const imcontact &ic, bool reqauth = false);

	bool idle(int options = 0);
	void exectimers();

	icqface::eventviewresult readevent(const imevent &ev,
	    const vector<icqface::eventviewresult> &buttons = vector<icqface::eventviewresult>());

	icqface::eventviewresult readevent(const imevent &ev, bool &enough, bool &fin,
	    const vector<icqface::eventviewresult> &buttons = vector<icqface::eventviewresult>());

	void readevents(const imcontact cont);

	void history(const imcontact &cont);
	bool next_chat(bool next = false);
  
	bool sendevent(const imevent &ev, icqface::eventviewresult r);
};

extern centericq cicq;

#endif
