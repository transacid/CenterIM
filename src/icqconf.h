#ifndef __ICQCONF_H__
#define __ICQCONF_H__

#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

#include <string>
#include <list>
#include <algorithm>

#include "kkstrtext.h"
#include "kkfsys.h"
#include "conscommon.h"
#include "icq.h"

#include "icqcommon.h"

#define cp_status           1

#define cp_dialog_text      2
#define cp_dialog_menu      3
#define cp_dialog_selected  4
#define cp_dialog_highlight 5
#define cp_dialog_frame     6

#define cp_main_text        7
#define cp_main_menu        8
#define cp_main_selected    9
#define cp_main_highlight   10
#define cp_main_frame       11

#ifndef VERSION
#define VERSION "devel"
#endif

/*
#ifndef SHARE_DIR
#define SHARE_DIR "/usr/local/share"
#endif

#ifndef LOCALE_DIR
#define LOCALE_DIR "/usr/local/share/locale"
#endif
*/

#ifdef ENABLE_NLS

#include <libintl.h>
#define _(s)    gettext(s)

#else

#define _(s)    (s)

#endif

#define ICQ_SERVER      "icq.mirabilis.com:4000"

enum regsound {rscard, rsspeaker, rsdisable, rsdontchange};
enum regcolor {rcdark, rcblue, rcdontchange};

class icqconf {
    protected:
	unsigned int uin, port, socksport;

	int autoaway, autona;
	bool hideoffline, quote, savepwd, antispam, mailcheck, serveronly;

	string password, rnick, rfname, rlname, remail, server;
	string sockshost, socksuser, sockspass, openurlcommand;

	list<int> boldcolors;

	regsound rs;
	regcolor rc;

	int findcolor(string s);

    public:
	icqconf();
	~icqconf();

	unsigned int getuin();
	const string &getpassword();

	void setpassword(string npass);
	
	void checkdir();
	void load();

	int getcolor(int npair);

	int getstatus();
	void savestatus(int st);
	
	void loadmainconfig();
	void savemainconfig(unsigned int fuin = 0);

	void loadcolors();
	void loadsounds();
	void loadactions();
	void initpairs();

	void registerinfo(unsigned int fuin, string passwd, string nick,
	string fname, string lname, string email);
	
	regcolor getregcolor();
	void setregcolor(regcolor c);

	regsound getregsound();
	void setregsound(regsound s);

	bool gethideoffline();
	void sethideoffline(bool fho);

	bool getantispam();
	void setantispam(bool fas);

	bool getmailcheck();
	void setmailcheck(bool fmc);

	bool getserveronly();
	void setserveronly(bool fso);

	void setauto(int away, int na);
	void getauto(int &away, int &na);

	void setserver(string nserv);
	string getservername();
	unsigned int getserverport();

	bool getquote();
	void setquote(bool use);

	bool getsavepwd();
	void setsavepwd(bool ssave);
 
	void setsockshost(string nsockshost);
	string getsockshost();
	unsigned int getsocksport();

	void getsocksuser(string &name, string &pass);
	void setsocksuser(string name, string pass);

	void openurl(const string url);
};

extern icqconf conf;
extern struct icq_link icql;

#endif
