#ifndef __ICQFACE_H__
#define __ICQFACE_H__

#include <fstream>

#include "icqhist.h"

#include "icqcommon.h"

#include "dialogbox.h"
#include "textinputline.h"
#include "texteditor.h"
#include "textbrowser.h"
#include "fileselector.h"

#include "icqconf.h"
#include "icqcontact.h"
#include "icqmlist.h"
#include "icqhook.h"

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
#define ACT_INVISLIST   -31
#define ACT_QUICKFIND   -32
#define ACT_CONTACT     -33
#define ACT_GROUPMOVE   -34
#define ACT_ORG_GROUPS  -35
#define ACT_HIDEOFFLINE -36

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

    protected:
	treeview *mcontacts;
	textinputline input;
	textwindow mainw;
	fileselector selector;
	ofstream flog;
	vector<string> extractedurls;

	linkedlist workareas;

	bool editdone, mainscreenblock, inited, onlinefolder, dotermresize;
	int extk, totalunread;

	imcontact passinfo;

	string rnick, rfname, rlname, remail;

	static int editmsgkeys(texteditor &e, int k);
	static int editaboutkeys(texteditor &e, int k);
	static int contactskeys(verticalmenu &m, int k);
	static int multiplekeys(verticalmenu &m, int k);
	static int historykeys(dialogbox &m, int k);
	static int userinfokeys(dialogbox &db, int k);

	static void editidle(texteditor &e);
	static void textbrowseridle(textbrowser &b);
	static void textinputidle(textinputline &il);
	static void freeworkareabuf(void *p);
	static void detailsidle(dialogbox &db);

	static void termresize(void);

	void infoclear(dialogbox &db, icqcontact *c, const imcontact realdesc);
	void infogeneral(dialogbox &db, icqcontact *c);
	void infohome(dialogbox &db, icqcontact *c);
	void infowork(dialogbox &db, icqcontact *c);
	void infointerests(dialogbox &db, icqcontact *c);
	void infoabout(dialogbox &db, icqcontact *c);

	bool checkicqmessage(const imcontact ic, const string text, bool &ret, int options);
	int showicq(unsigned int uin, const string text, char imt, int options = 0);

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
	int groupmanager(const string text, bool sel);

    public:
	vector<imcontact> muins;

	icqface();
	~icqface();

	void init();
	void done();

	static void menuidle(verticalmenu &caller);
	static void dialogidle(dialogbox &caller);

	void draw();
	void update();
	void showtopbar();

	icqcontact *mainloop(int &action);
	void fillcontactlist();

	void log(const string text);
	void log(const char *fmt, ...);

	void status(const string text);

	bool changestatus(protocolname &pname, imstatus &st);
	int contextmenu(icqcontact *c);
	int generalmenu();

	bool editmsg(const imcontact cinfo, string &text);
	bool editurl(const imcontact cinfo, string &url, string &text);
	void read(const imcontact cinfo);
	void history(const imcontact cinfo);
	void modelist(contactstatus cs);

	bool showevent(const imcontact cinfo, int direction, time_t &lastread);
	int showmsg(const imcontact cinfo, const string text, struct tm &recvtm, struct tm &senttm, int inout = HIST_MSG_IN, bool inhistory = false);
	int showurl(const imcontact cinfo, const string url, const string text, struct tm &recvtm, struct tm &senttm, int inout = HIST_MSG_IN, bool inhistory = false);
	int showfile(const imcontact cinfo, unsigned long seq, const string fname, int fsize, struct tm &recvtm, struct tm &senttm, int inout = HIST_MSG_IN, bool inhistory = false);
	int showcontact(const imcontact cinfo, icqcontactmsg *cont, struct tm &recvtm, struct tm &senttm, int inout = HIST_MSG_IN, bool inhistory = false);
	void acceptfile(const imcontact cinfo, unsigned long seq, const string fname);
	void refusefile(const imcontact cinfo, unsigned long seq);

	bool multicontacts(const string head = "");
	void userinfo(const imcontact cinfo, const imcontact realinfo);

	bool updateconf(regsound &s, regcolor &c);

	bool finddialog(icqhook::searchparameters &s);
	bool findresults();

	bool updatedetails(icqcontact *c = 0);
	bool sendfiles(const imcontact cinfo, string &msg, linkedlist &flist);

	void blockmainscreen();
	void unblockmainscreen();

	int ask(string q, int options, int deflt = -1);
	const string inputstr(const string q, const string defl = "", char passwdchar = 0);
	const string inputfile(const string q, const string defl = "");

	void quickfind(verticalmenu *multi = 0);

	int selectgroup(const string text);
	void organizegroups();
	void makeprotocolmenu(verticalmenu &m);
};

extern icqface face;

string getbdate(unsigned char fbday, unsigned char fbmonth, unsigned char fbyear);

const char *strgender(unsigned char fgender);
const char *stryesno(bool i);
const char *strregsound(regsound s);
const char *strregcolor(regcolor c);
const char *strint(unsigned int i);

#endif
