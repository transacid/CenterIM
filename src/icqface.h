#ifndef __ICQFACE_H__
#define __ICQFACE_H__

#include <fstream>

#include "icq.h"
#include "icqhist.h"

#include "icqcommon.h"

#include "dialogbox.h"
#include "textinputline.h"
#include "texteditor.h"
#include "textbrowser.h"
#include "filemanager.h"

#include "icqconf.h"
#include "icqcontact.h"
#include "icqmlist.h"

#define DIALOG_WIDTH    60
#define DIALOG_HEIGHT   15

#define BIGDIALOG_WIDTH    70
#define BIGDIALOG_HEIGHT   18

#define WORKAREA_X1     25
#define WORKAREA_X2     (COLS-1)
#define WORKAREA_Y1     1
#define WORKAREA_Y2     (LINES-6)

#define ASK_YES         2
#define ASK_NO          4
#define ASK_CANCEL      8

#define ACT_QUIT        -10
#define ACT_MSG         -11
#define ACT_URL         -12
#define ACT_STATUS      -13
#define ACT_REMOVE      -14
#define ACT_FILE        -15
#define ACT_CHAT        -16
#define ACT_FIND        -17
#define ACT_INFO        -18
#define ACT_HISTORY     -19
#define ACT_IGNORE      -20
#define ACT_ADD         -21
#define ACT_MENU        -22
#define ACT_IGNORELIST  -23
#define ACT_DETAILS     -24
#define ACT_GMENU       -25
#define ACT_CONF        -26
#define ACT_RENAME      -27
#define ACT_NONICQ      -28
#define ACT_EDITUSER    -29
#define ACT_VISIBLELIST -30
#define ACT_INVISIBLELIST -31
#define ACT_QUICKFIND   -32

extern class centericq cicq;

class icqface {
    public:
	class icqprogress {
	    protected:
		textwindow *w;
		int curline;

	    public:
		icqprogress();
		~icqprogress();

		void log(const char *fmt, ...);
		void show(string title = "");
		void hide();
	} progress;

	struct searchparameters {
	    searchparameters() {
		onlineonly = false;
		uin = 0;
		minage = maxage = country = 0;
		gender = language = 0;
	    };

	    bool onlineonly;
	    unsigned int uin;
	    unsigned short minage, maxage, country;
	    unsigned char gender, language;
	    string firstname, lastname, nick, city, state;
	    string company, department, position, email;
	};

    protected:
	treeview *mcontacts;
	textinputline *il;
	textwindow mainw;
	filemanager *fm;
	ofstream flog;
	vector<string> extractedurls;

	linkedlist workareas;

	bool editdone, mainscreenblock, inited, onlinefolder, dotermresize;
	int extk, totalunread;
	unsigned int passuin;

	string rnick, rfname, rlname, remail;

	string textstatus(unsigned long st);

	static int editmsgkeys(texteditor *e, int k);
	static int editaboutkeys(texteditor *e, int k);
	static int contactskeys(verticalmenu *m, int k);
	static int multiplekeys(verticalmenu *m, int k);
	static int historykeys(dialogbox *m, int k);
	static int userinfokeys(dialogbox *db, int k);

	static void editidle(texteditor *e);
	static void textbrowseridle(textbrowser *b);
	static void textinputidle(textinputline *il);
	static void freeworkareabuf(void *p);
	static void detailsidle(dialogbox *db);

	static void termresize(void);

	void infoclear(dialogbox &db, icqcontact *c, unsigned int uin);
	void infogeneral(dialogbox &db, icqcontact *c);
	void infohome(dialogbox &db, icqcontact *c);
	void infowork(dialogbox &db, icqcontact *c);
	void infointerests(dialogbox &db, icqcontact *c);
	void infoabout(dialogbox &db, icqcontact *c);

	bool checkicqmessage(unsigned int uin, string text, bool &ret, int options);
	int showicq(unsigned int uin, string text, char imt, int options = 0);

	void gendetails(treeview *tree, icqcontact *c = 0);
	void selectgender(unsigned char &f);
	void selectcountry(unsigned short &f);
	void selectoccupation(unsigned short &f);
	void selectlanguage(unsigned char &f);
	void editabout(string &fabout);

	void saveworkarea();
	void restoreworkarea();
	void clearworkarea();
	void workarealine(int l, chtype c = HLINE);

	void showextractedurls();
	void extracturls(const string buf);

    public:
	list<unsigned int> muins;

	icqface();
	~icqface();

	void init();
	void done();

	static void menuidle(verticalmenu *caller);
	static void dialogidle(dialogbox *caller);

	void draw();
	void update();
	void showtopbar();
	icqcontact *mainloop(int &action);
	void fillcontactlist();
	void log(string text);
	void log(const char *fmt, ...);
	void status(string text);

	int changestatus(int old);
	int contextmenu(icqcontact *c);
	int generalmenu();

	bool editmsg(unsigned int uin, string &text);
	bool editurl(unsigned int uin, string &url, string &text);
	void read(unsigned int uin);
	void history(unsigned int uin);
	void modelist(contactstatus cs);

	bool showevent(unsigned int uin, int direction, time_t &lastread);
	int showmsg(unsigned int uin, string text, struct tm &recvtm, struct tm &senttm, int inout = HIST_MSG_IN, bool inhistory = false);
	int showurl(unsigned int uin, string url, string text, struct tm &recvtm, struct tm &senttm, int inout = HIST_MSG_IN, bool inhistory = false);
	int showfile(unsigned int uin, unsigned long seq, string fname, int fsize, struct tm &recvtm, struct tm &senttm, int inout = HIST_MSG_IN, bool inhistory = false);
	int showcontact(unsigned int uin, icqcontactmsg *cont, struct tm &recvtm, struct tm &senttm, int inout = HIST_MSG_IN, bool inhistory = false);
	void acceptfile(unsigned int uin, unsigned long seq, string fname);
	void refusefile(unsigned int uin, unsigned long seq);

	bool multicontacts(string head, list<unsigned int> &lst);
	void userinfo(unsigned int uin, bool nonicq, unsigned int realuin);

	bool regdialog(unsigned int &ruin, string &rpasswd);
	void getregdata(string &nick, string &fname, string &lname, string &email);
	bool updateconf(regsound &s, regcolor &c);

	bool finddialog(searchparameters &s);
	bool findresults();

	bool updatedetails(icqcontact *c = 0);
	bool sendfiles(unsigned int uin, string &msg, linkedlist &flist);

	void chat(unsigned int uin);

	void blockmainscreen();
	void unblockmainscreen();

	int ask(string q, int options, int deflt = -1);
	string inputstr(string q, string defl = "", char passwdchar = 0);
	string inputfile(string q, string defl = "");

	void quickfind(verticalmenu *multi = 0);
};

extern icqface face;

string getbdate(unsigned char fbday, unsigned char fbmonth, unsigned char fbyear);

const char *strgender(unsigned char fgender);
const char *stryesno(bool i);
const char *strregsound(regsound s);
const char *strregcolor(regcolor c);
const char *strint(unsigned int i);

#endif
