#ifndef __ICQHIST_H__
#define __ICQHIST_H__

#include "kkstrtext.h"
#include "cmenus.h"

#include "icqcommon.h"
#include "imcontact.h"

#define HIST_MSG_OUT            2
#define HIST_MSG_IN             4
#define HIST_HISTORYMODE        8

#define EVT_MSG                 0
#define EVT_URL                 1
#define EVT_CHAT                3
#define EVT_EMAIL               4
#define EVT_ONLINE              6

#define ICQM_REQUEST    'R'
#define ICQM_ADDED      'I'

class icqhistory {
    protected:
	class histentry { public:
	    string text;
	    int lastread, color;
	};      // for fillmenu

	class storedopen {
	    public:
		storedopen(FILE *nf) { f = nf; rn = 0; }
		FILE *f;
		int rn;
		vector<string> lastevent;
	};

	FILE *open(const imcontact cinfo, const char *mode);
	vector<storedopen> opens;

    public:
	icqhistory();
	~icqhistory();
	
	void putmessage(const imcontact cinfo, const string text, int dir, struct tm *timestamp);
	void puturl(const imcontact cinfo, const string url, const string desc, int dir, struct tm *timestamp);
	void putfile(const imcontact cinfo, unsigned long seq, const string fname, int fsize, int dir, struct tm *timestamp);
	void putmail(const string nick, const string email, const string msg, int mailt, struct tm *timestamp);

	bool opencontact(const imcontact cinfo, time_t lastread = 0);
	int setpos(int n);
	int setposlastread(time_t lr);
	int find(const string sub, int pos = 0);
	int readevent(int &event, time_t &lastread, struct tm &sent, int &dir);
	void getmessage(string &text);
	void geturl(string &url, string &text);
	void getfile(unsigned long &seq, string &fname, int &fsize);
	void closecontact();

	int getpos();

	void fillmenu(const imcontact cinfo, verticalmenu *m);
};

extern icqhistory hist;

#endif
