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

#define ASK_YES         2
#define ASK_NO          4
#define ASK_CANCEL      8

enum interfaceAction {
    ACT_QUIT = -10,
    ACT_MSG = -11,
    ACT_URL = -12,
    ACT_STATUS = -13,
    ACT_REMOVE = -14,
    ACT_SMS = -15,
    ACT_CHAT = -16,
    ACT_FIND = -17,
    ACT_INFO = -18,
    ACT_HISTORY = -19,
    ACT_IGNORE = -20,
    ACT_ADD = -21,
    ACT_MENU = -22,
    ACT_IGNORELIST = -23,
    ACT_DETAILS = -24,
    ACT_GMENU = -25,
    ACT_CONF = -26,
    ACT_RENAME = -27,
    ACT_PING = -28,
    ACT_EDITUSER = -29,
    ACT_VISIBLELIST = -30,
    ACT_INVISLIST = -31,
    ACT_QUICKFIND = -32,
    ACT_FILE = -33,
    ACT_GROUPMOVE = -34,
    ACT_ORG_GROUPS = -35,
    ACT_HIDEOFFLINE = -36,
    ACT_FETCHAWAY = -37,
    ACT_EMAIL = -38,
    ACT_AUTH = -39,
    ACT_CONTACT = -40,
    ACT_VERSION = -41
};

extern class centericq cicq;

class icqface {
    public:
	enum eventviewresult {
	    ok, next, cancel, forward, reply, open, accept, reject, info,
	    add, prev, eventviewresult_size
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

	int chatlines;

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

	void workarealine(int l, chtype c = HLINE);

	void showextractedurls();
	void extracturls(const string &buf);

	int groupmanager(const string &text, bool sel);

	void renderchathistory();
	bool sprofmanager(string &name, string &act);

    public:
	int extk;
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

	void clearworkarea();
	void saveworkarea();
	void restoreworkarea();

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

	bool multicontacts(const string &head = "", const set<protocolname> &protos = set<protocolname>());
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
const char *stryesno(bool b);
const char *strgender(imgender g);
const char *seteventviewresult(icqface::eventviewresult r);

#endif
