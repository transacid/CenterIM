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

#include "icqgroup.h"
#include "icqcontact.h"

#define cp_status               1
#define cp_dialog_text          2
#define cp_dialog_menu          3
#define cp_dialog_selected      4
#define cp_dialog_highlight     5
#define cp_dialog_frame         6
#define cp_main_text            7
#define cp_main_menu            8
#define cp_main_selected        9
#define cp_main_highlight       10
#define cp_main_frame           11
#define cp_clist_icq            12
#define cp_clist_yahoo          13
#define cp_clist_infocard       14
#define cp_clist_root           15
#define cp_clist_msn            16

enum regsound { rscard, rsspeaker, rsdisable, rsdontchange };
enum regcolor { rcdark, rcblue, rcdontchange };

class icqconf {
    public:
	struct imaccount {
	    imaccount();
	    imaccount(protocolname apname);

	    string nickname, server, password;
	    unsigned long uin, port;
	    protocolname pname;

	    void write(ofstream &f);
	    void read(const string spec);

	    bool empty() const;

	    bool operator == (protocolname apname) const;
	    bool operator != (protocolname apname) const;
	};

    protected:
	vector<icqgroup> groups;
	vector<imaccount> accounts;
	list<int> boldcolors;

	unsigned int socksport;

	int autoaway, autona;

	bool hideoffline, quote, savepwd, antispam, mailcheck,
	    usegroups, russian, makelog;

	string sockshost, socksuser, sockspass,
	    openurlcommand, basedir;

	regsound rs;
	regcolor rc;

	void loadmainconfig();

    public:
	icqconf();
	~icqconf();

	void checkdir();
	void load();

	int getcolor(int npair) const;

	void save();

	void loadcolors();
	void loadsounds();
	void loadactions();

	void initpairs();

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

	void setauto(int away, int na);
	void getauto(int &away, int &na) const;

	bool getquote() const;
	void setquote(bool use);

	bool getsavepwd() const;
	void setsavepwd(bool ssave);

	bool getmakelog() const;
	void setmakelog(bool slog);

	bool getrussian() const;
	void setrussian(bool frussian);

	bool getusegroups() const { return usegroups; }
	void setusegroups(bool ausegroups) { usegroups = ausegroups; }
 
	void setsockshost(const string nsockshost);
	const string getsockshost() const;
	unsigned int getsocksport() const;

	void getsocksuser(string &name, string &pass) const;
	void setsocksuser(const string name, const string pass);

	void openurl(const string url);

	imstatus getstatus(protocolname pname);
	void savestatus(protocolname pname, imstatus st);

	int getprotcolor(protocolname pname) const;
	const string getprotocolname(protocolname pname) const;

	imaccount getourid(protocolname pname);
	void setourid(const imaccount im);
	int getouridcount() const;

	const string getdirname() const;
	const string getconfigfname(const string fname) const;

	void commandline(int argc, char **argv);
};

extern icqconf conf;

#endif
