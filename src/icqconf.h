#ifndef __ICQCONF_H__
#define __ICQCONF_H__

#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

#include "kkstrtext.h"
#include "kkfsys.h"
#include "conscommon.h"
#include "icq.h"

#include "icqcommon.h"
#include "icqgroup.h"

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

#define ICQ_SERVER      "icq.mirabilis.com:4000"

enum regsound {rscard, rsspeaker, rsdisable, rsdontchange};
enum regcolor {rcdark, rcblue, rcdontchange};

class icqconf {
    protected:
	vector<icqgroup> groups;

	unsigned int uin, port, socksport;

	int autoaway, autona;

	bool hideoffline, quote, savepwd, antispam, mailcheck,
	    serveronly, usegroups;

	string password, rnick, rfname, rlname, remail, server,
	    sockshost, socksuser, sockspass, openurlcommand;

	list<int> boldcolors;

	regsound rs;
	regcolor rc;

    public:
	icqconf();
	~icqconf();

	unsigned int getuin();
	const string &getpassword();

	void setpassword(const string npass);
	
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

	void registerinfo(unsigned int fuin, const string passwd,
	    const string nick, const string fname,
	    const string lname, const string email);
	
	regcolor getregcolor() const;
	void setregcolor(regcolor c);

	regsound getregsound() const;
	void setregsound(regsound s);

	bool gethideoffline() const;
	void sethideoffline(bool fho);

	bool getantispam() const;
	void setantispam(bool fas);

	bool getmailcheck() const;
	void setmailcheck(bool fmc);

	bool getserveronly() const;
	void setserveronly(bool fso);

	void setauto(int away, int na);
	void getauto(int &away, int &na) const;

	void setserver(const string nserv);
	const string getservername() const;
	unsigned int getserverport() const;

	bool getquote() const;
	void setquote(bool use);

	bool getsavepwd() const;
	void setsavepwd(bool ssave);

	bool getusegroups() const { return usegroups; }
	void setusegroups(bool ausegroups) { usegroups = ausegroups; }
 
	void setsockshost(const string nsockshost);
	const string getsockshost() const;
	unsigned int getsocksport() const;

	void getsocksuser(string &name, string &pass) const;
	void setsocksuser(const string name, const string pass);

	void openurl(const string url);
};

extern icqconf conf;
extern struct icq_link icql;

#endif
