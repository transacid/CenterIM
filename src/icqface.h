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
    ACT_QUIT = -50, ACT_MSG, ACT_URL, ACT_STATUS, ACT_REMOVE, ACT_SMS,
    ACT_CHAT, ACT_FIND, ACT_INFO, ACT_HISTORY, ACT_IGNORE, ACT_ADD,
    ACT_MENU, ACT_IGNORELIST, ACT_DETAILS, ACT_GMENU, ACT_CONF, ACT_RENAME,
    ACT_PING, ACT_EDITUSER, ACT_VISIBLELIST, ACT_INVISLIST, ACT_QUICKFIND,
    ACT_FILE, ACT_GROUPMOVE, ACT_ORG_GROUPS, ACT_HIDEOFFLINE, ACT_FETCHAWAY,
    ACT_EMAIL, ACT_AUTH, ACT_CONTACT, ACT_VERSION, ACT_JOIN, ACT_LEAVE,
    ACT_CONFER, ACT_TRANSFERS, ACT_JOINDIALOG, ACT_EXTERN, ACT_RSS, ACT_LJ,
    ACT_MASS_MOVE, ACT_PGPKEY, ACT_PGPSWITCH, ACT_DUMMY
};

extern class centericq cicq;

class icqface {
    public:
	enum eventviewresult {
	    ok, next, cancel, forward, reply, open, accept, reject, info,
	    add, prev, eventviewresult_size
	};

	enum transferstatus {
	    tsInit,
	    tsStart,
	    tsProgress,
	    tsFinish,
	    tsError,
	    tsCancel
	};

	enum findsubject {
	    fsuser,
	    fschannel,
	    fsrss,
	    fs_size
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
	vector<string> extractedurls, lastlog, fstatus;

	linkedlist workareas;

	bool editdone, mainscreenblock, inited, onlinefolder,
	    dotermresize, fneedupdate, detailsfetched, inchat, doredraw;

	int chatlines;

	time_t chatlastread;

	imcontact passinfo;
	const imevent *passevent;
  
	icqcontact *last_selected;
	int find_next_action;

	struct filetransferitem {
	    string fname;
	    int btotal, bdone;
	    transferstatus status;
	    imfile fr;

	    filetransferitem(const imfile &afr)
	    : btotal(0), bdone(0), fr(afr) { }
	};

	vector<filetransferitem> transfers;

    protected:
	static int editmsgkeys(texteditor &e, int k);
	static int editaboutkeys(texteditor &e, int k);
	static int contactskeys(verticalmenu &m, int k);
	static int multiplekeys(verticalmenu &m, int k);
	static int historykeys(dialogbox &m, int k);
	static int userinfokeys(dialogbox &db, int k);
	static int findreskeys(dialogbox &db, int k);
	static int eventviewkeys(dialogbox &db, int k);
	static int findpgpkeys(dialogbox &db, int k);
	static int statuskeys(verticalmenu &m, int k);
	static int fullscreenkeys(textbrowser &m, int k);

	static void editidle(texteditor &e);
	static void textbrowseridle(textbrowser &b);
	static void textinputidle(textinputline &il);
	static void freeworkareabuf(void *p);
	static void detailsidle(dialogbox &db);
	static void transferidle(dialogbox &db);
	static void editchatidle(texteditor &e);

	static void termresize(void);

	void infoclear(dialogbox &db, icqcontact *c, const imcontact realdesc);
	void infogeneral(dialogbox &db, icqcontact *c);
	void infohome(dialogbox &db, icqcontact *c);
	void infowork(dialogbox &db, icqcontact *c);
	void infointerests(dialogbox &db, icqcontact *c);
	void infoabout(dialogbox &db, icqcontact *c);

	void inforss(dialogbox &db, icqcontact *c);
	void infolivejournal(dialogbox &db, icqcontact *c);
	void infofriends(dialogbox &db, icqcontact *c);
	void infoljrss(dialogbox &db, icqcontact *c);

	void gendetails(treeview *tree, icqcontact *c = 0);

	void selectgender(imgender &f);
	void selectcountry(unsigned short &f);
	void selectlanguage(unsigned short &f);
	void selectagerange(ICQ2000::AgeRange &r);
	void selectrandomgroup(unsigned short &f);

	enum spmode { spIMonly, spIMplusRSS, spnonIM };
	void selectproto(bool prots[], spmode mode = spIMonly);

	void workarealine(int l, chtype c = HLINE);

	void showextractedurls();
	void extracturls(const string &buf);

	int groupmanager(const string &text, bool sel);

	void renderchathistory();
	bool sprofmanager(string &name, string &act);
	void showeventbottom(const imcontact &ic);

	void invokelist(string &s, vector<string> &v, const string &def, textwindow *w);
	bool setljparams(imxmlevent *ev);
	void multichange(bool conv[], bool newstate);

	string extracttime(const imevent &ev);
	void peerinfo(int line, const imcontact &ic);
	void findpgpkey(dialogbox &db, const vector<string> &items);

    public:
	int extk;
	vector<imcontact> muins;

	icqface();
	~icqface();

	void init();
	void done();

	static void menuidle(verticalmenu &caller);
	static void dialogidle(dialogbox &caller);

	string getmenuitem(string mtext, int width, int key, int section);
	string getstatkey(int key, int section) const;

	void redraw();

	void draw();
	void update();
	void showtopbar();

	void clearworkarea();
	void saveworkarea();
	void restoreworkarea();

	void relaxedupdate();
	bool updaterequested();

	int key2action(int k, int s);
	string action2key(int a, int s, int n = 1) const;
  
	icqcontact *find_next_chat();
	bool next_chat(bool next = true);
  
	icqcontact *mainloop(int &action);
	void fillcontactlist();

	void log(const string &text);
	void log(const char *fmt, ...);

	void status(const string &text);
	void status(const char *fmt, ...);

	void xtermtitle(const string &text = "");
	void xtermtitle(const char *fmt, ...);
	void xtermtitlereset();

	bool changestatus(vector<protocolname> &pnames, imstatus &st);
	int contextmenu(icqcontact *c);
	int generalmenu();

	void modelist(contactstatus cs);

	bool multicontacts(const string &head = "",
	    const set<protocolname> &protos = set<protocolname>(),
	    contactstatus cs = csnone);

	void userinfo(const imcontact &cinfo, const imcontact &realinfo);

	bool updateconf(icqconf::regsound &s, icqconf::regcolor &c);

	bool finddialog(imsearchparams &s, findsubject subj);
	bool findresults(const imsearchparams &sp, bool auto = false);
	void findready();

	bool conferencedialog(protocolname &pname, string &name);

	bool updatedetails(icqcontact *c = 0, protocolname upname = icq);
	bool sendfiles(const imcontact &cinfo, string &msg, linkedlist &flist);

	void blockmainscreen();
	void unblockmainscreen();

	int ask(string q, int options, int deflt = -1);

	string inputstr(const string &q, const string &defl = "", char passwdchar = 0);
	string inputfile(const string &q, const string &defl = "");
	string inputdir(const string &q, const string &defl = "");

	int getlastinputkey() const;

	void quickfind(verticalmenu *multi = 0);

	int selectgroup(const string &text);

	void makeprotocolmenu(verticalmenu &m);

	void organizegroups();

	void histmake(const vector<imevent *> &hist);
	bool histexec(imevent *&im);

	bool eventedit(imevent &ev);

	eventviewresult eventview(const imevent *ev,
	    vector<eventviewresult> abuttons = vector<eventviewresult>(),
	    bool nobuttons = false);

	void fullscreenize(const imevent *ev);

	bool edit(string &txt, const string &header);
	void chat(const imcontact &ic);

	void transferupdate(const string &fname, const imfile &fr,
	    transferstatus st, int btotal, int bdone);

	void transfermonitor();
	void userinfoexternal(const imcontact &ic);

	bool selectpgpkey(string &keyid, bool secretonly = false);
};

extern icqface face;

string getbdate(unsigned char fbday, unsigned char fbmonth, unsigned char fbyear);

const char *strregsound(icqconf::regsound s);
const char *strregcolor(icqconf::regcolor c);
const char *strcolormode(icqconf::colormode cm);
const char *strint(unsigned int i);
const char *strgroupmode(icqconf::groupmode gmode);
const char *stryesno(bool b);
const char *strgender(imgender g);
const char *seteventviewresult(icqface::eventviewresult r);

#endif
