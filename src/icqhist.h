#ifndef __ICQHIST_H__
#define __ICQHIST_H__

#include "icq.h"
#include "konst.string.h"
#include "konst.ui.menu.h"
#include "konst.fs.h"

#define HIST_MSG_OUT            2
#define HIST_MSG_IN             4
#define HIST_HISTORYMODE        8

#define ICQMAIL_WEBPAGER        0
#define ICQMAIL_EXPRESS         1

#define EVT_MSG                 0
#define EVT_URL                 1
#define EVT_FILE                2
#define EVT_CHAT                3
#define EVT_EMAIL               4
#define EVT_CONTACT             5
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

	FILE *open(unsigned int uin, const char *mode);
	linkedlist opens;

    public:
	icqhistory();
	~icqhistory();
	
	void putmessage(unsigned int uin, string text, int dir, struct tm *timestamp);
	void puturl(unsigned int uin, string url, string desc, int dir, struct tm *timestamp);
	void putfile(unsigned int uin, unsigned long seq, string fname, int fsize, int dir, struct tm *timestamp);
	void putmail(string nick, string email, string msg, int mailt, struct tm *timestamp);
	void putcontact(unsigned int uin, icqcontactmsg *cont, int dir, struct tm *timestamp);

	bool opencontact(unsigned int uin, time_t lastread = 0);
	int setpos(int n);
	int setposlastread(time_t lr);
	int find(const string sub, int pos = 0);
	int readevent(int &event, time_t &lastread, struct tm &sent, int &dir);
	void getmessage(string &text);
	void geturl(string &url, string &text);
	void getfile(unsigned long &seq, string &fname, int &fsize);
	void getcontact(icqcontactmsg **cont);
	void closecontact();

	int getpos();

	void fillmenu(unsigned int uin, verticalmenu *m);
};

extern icqhistory hist;

#endif
