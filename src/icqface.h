#ifndef __ICQFACE_H__
#define __ICQFACE_H__

#include "icqcommon.h"

#include "dialogbox.h"
#include "textinputline.h"
#include "texteditor.h"
#include "textbrowser.h"
#include "fileselector.h"

#include "icqconf.h"
#include "icqcontact.h"
#include "icqmlist.h"
#include "imcontroller.h"

#include <libicq2000/userinfohelpers.h>
#include <libicq2000/constants.h>

#define ASK_YES         2
#define ASK_NO          4
#define ASK_CANCEL      8

#define ACT_QUIT        -10
#define ACT_MSG         -11
#define ACT_URL         -12
#define ACT_STATUS      -13
#define ACT_REMOVE      -14
#define ACT_SMS         -15
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
#define ACT_EDITUSER    -29
#define ACT_VISIBLELIST -30
#define ACT_INVISLIST   -31
#define ACT_QUICKFIND   -32
#define ACT_FILE        -33
#define ACT_GROUPMOVE   -34
#define ACT_ORG_GROUPS  -35
#define ACT_HIDEOFFLINE -36
#define ACT_FETCHAWAY   -37
#define ACT_EMAIL       -38

extern class centericq cicq;

class icqface {
    public:
	enum eventviewresult {
	    ok, next, cancel, forward, reply, open,
	    accept, reject, info, eventviewresult_size
	};

	class icqprogress {
	    protected:
		textwindow *w;
		int curline;

	    public:
		icqprogress();
		~icqprogress();

		void log(const char *fmt, ...);
		void show(const string &title = "");
		void hide();
	} progress;

	struct { int x1, y1, x2, y2; } sizeWArea;
	struct { int width, height; } sizeDlg, sizeBigDlg;

    protected:
	treeview *mcontacts;
	textinputline input;
	textwindow mainw;
	fileselector selector;
	verticalmenu mhist;

	ofstream flog;
	vector<string> extractedurls, lastlog;
	string fstatus;

	linkedlist workareas;

	bool editdone, mainscreenblock, inited, onlinefolder,
	    dotermresize, fneedupdate, detailsfetched, inchat;

	int extk, chatlines;

	time_t chatlastread;

	imcontact passinfo;

    protected:
	static int editmsgkeys(texteditor &e, int k);
	static int editaboutkeys(texteditor &e, int k);
	static int contactskeys(verticalmenu &m, int k);
	static int multiplekeys(verticalmenu &m, int k);
	static int historykeys(dialogbox &m, int k);
	static int userinfokeys(dialogbox &db, int k);
	static int findreskeys(dialogbox &db, int k);

	static void editidle(texteditor &e);
	static void textbrowseridle(textbrowser &b);
	static void textinputidle(textinputline &il);
	static void freeworkareabuf(void *p);
	static void detailsidle(dialogbox &db);
	static void editchatidle(texteditor &e);

	static void termresize(void);

	void infoclear(dialogbox &db, icqcontact *c, const imcontact realdesc);
	void infogeneral(dialogbox &db, icqcontact *c);
	void infohome(dialogbox &db, icqcontact *c);
	void infowork(dialogbox &db, icqcontact *c);
	void infointerests(dialogbox &db, icqcontact *c);
	void infoabout(dialogbox &db, icqcontact *c);

	void gendetails(treeview *tree, icqcontact *c = 0);

	void selectgender(imgender &f);
	void selectcountry(unsigned short &f);
	void selectlanguage(unsigned short &f);
	void selectagerange(ICQ2000::AgeRange &r);
	void selectrandomgroup(unsigned short &f);

	void saveworkarea();
	void restoreworkarea();
	void clearworkarea();
	void workarealine(int l, chtype c = HLINE);

	void showextractedurls();
	void extracturls(const string &buf);

	int groupmanager(const string &text, bool sel);

	void renderchathistory();
	bool sprofmanager(string &name, string &act);

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

	void relaxedupdate();
	bool updaterequested();

	icqcontact *mainloop(int &action);
	void fillcontactlist();

	void log(const string &text);
	void log(const char *fmt, ...);

	void status(const string &text);

	bool changestatus(protocolname &pname, imstatus &st);
	int contextmenu(icqcontact *c);
	int generalmenu();

	void modelist(contactstatus cs);

	bool multicontacts(const string &head = "");
	void userinfo(const imcontact &cinfo, const imcontact &realinfo);

	bool updateconf(icqconf::regsound &s, icqconf::regcolor &c);

	bool finddialog(imsearchparams &s);
	bool findresults(const imsearchparams &sp);
	void findready();

	bool updatedetails(icqcontact *c = 0, protocolname upname = icq);
	bool sendfiles(const imcontact &cinfo, string &msg, linkedlist &flist);

	void blockmainscreen();
	void unblockmainscreen();

	int ask(string q, int options, int deflt = -1);

	string inputstr(const string &q, const string &defl = "", char passwdchar = 0);
	string inputfile(const string &q, const string &defl = "");
	int getlastinputkey() const;

	void quickfind(verticalmenu *multi = 0);

	int selectgroup(const string &text);
	void organizegroups();
	void makeprotocolmenu(verticalmenu &m);

	void histmake(const vector<imevent *> &hist);
	bool histexec(imevent *&im);

	bool eventedit(imevent &ev);

	eventviewresult eventview(const imevent *ev,
	    vector<eventviewresult> abuttons = vector<eventviewresult>());

	bool edit(string &txt, const string &header);
	void chat(const imcontact &ic);
};

extern icqface face;

string getbdate(unsigned char fbday, unsigned char fbmonth, unsigned char fbyear);

const char *strregsound(icqconf::regsound s);
const char *strregcolor(icqconf::regcolor c);
const char *strint(unsigned int i);
const char *strgroupmode(icqconf::groupmode gmode);

static const char *stryesno[true+1] = {
    _("no"), _("yes")
};

static const char *strgender[imgender_size] = {
    _("Not specified"),
    _("Male"),
    _("Female")
};

static const char *stragerange[ICQ2000::range_60_above+1] = {
  "",
  "18-22",
  "23-29",
  "30-39",
  "40-49",
  "50-59",
  _("60-above")
};

static const char *strrandomgroup[ICQ2000::group_SeekingMen+2] = {
    "",
    _("General Chat"),
    _("Romance"),
    _("Games"),
    _("Students"),
    _("20 Something"),
    _("30 Something"),
    _("40 Something"),
    _("50 Plus"),
    _("Seeking Women"),
    _("Seeking Men")
};

static const char *eventviewresultnames[icqface::eventviewresult_size] = {
    _("Ok"), _("Next"), "",
    _("Fwd"), _("Reply"),
    _("Open"), _("Accept"),
    _("Reject"), _("User info")
};

#endif
