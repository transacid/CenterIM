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

enum cicq_colorpairs {
    cp_status = 1,
    cp_dialog_text,
    cp_dialog_menu,
    cp_dialog_selected,
    cp_dialog_highlight,
    cp_dialog_frame,
    cp_main_text,
    cp_main_menu,
    cp_main_selected,
    cp_main_highlight,
    cp_main_frame,
    cp_clist_icq,
    cp_clist_yahoo,
    cp_clist_infocard,
    cp_clist_msn,
    cp_clist_aim,
    cp_clist_irc
};

class icqconf {
    public:
	enum regsound { rscard, rsspeaker, rsdisable, rsdontchange };
	enum regcolor { rcdark, rcblue, rcdontchange };
	enum groupmode { group1, group2, nogroups };

	struct imaccount {
	    imaccount();
	    imaccount(protocolname apname);

	    string nickname, server, password;
	    unsigned long uin, port;
	    protocolname pname;

	    void write(ofstream &f);
	    void read(const string &spec);

	    bool empty() const;

	    bool operator == (protocolname apname) const;
	    bool operator != (protocolname apname) const;
	};

    protected:
	vector<icqgroup> groups;
	vector<imaccount> accounts;

	list<int> boldcolors;

	unsigned short socksport, smtpport;

	int autoaway, autona;

	bool hideoffline, quote, savepwd, antispam, mailcheck, russian,
	    makelog, fenoughdiskspace;

	string sockshost, socksuser, sockspass, openurlcommand, basedir,
	    argv0, smtphost;

	regsound rs;
	regcolor rc;
	groupmode fgroupmode;

	void loadmainconfig();

	void usage() const;

	void constructevent(const string &event, const string &proto,
	    const string &dest) const;

	void externalstatuschange(char st, const string &proto) const;

    public:
	icqconf();
	~icqconf();

	void checkdir();
	void load();

	int getcolor(int npair) const {
	    return find(boldcolors.begin(), boldcolors.end(), npair) != boldcolors.end()
		? boldcolor(npair) : normalcolor(npair);
	}

	int getprotcolor(protocolname pname) const;

	void save();

	void loadcolors();
	void loadsounds();
	void loadactions();

	void initpairs();

	regcolor getregcolor() const;
	void setregcolor(regcolor c);

	regsound getregsound() const;
	void setregsound(regsound s);

	bool gethideoffline() const { return hideoffline; }
	void sethideoffline(bool fho);

	bool getantispam() const { return antispam; }
	void setantispam(bool fas);

	bool getmailcheck() const { return mailcheck; }
	void setmailcheck(bool fmc);

	void getauto(int &away, int &na) const;
	void setauto(int away, int na);

	bool getquote() const { return quote; }
	void setquote(bool use);

	bool getsavepwd() const { return savepwd; }
	void setsavepwd(bool ssave);

	bool getmakelog() const { return makelog; }
	void setmakelog(bool slog);

	bool getrussian() const { return russian; }
	void setrussian(bool frussian);

	groupmode getgroupmode() const { return fgroupmode; }
	void setgroupmode(groupmode amode);
 
	string getsockshost() const;
	unsigned int getsocksport() const;
	void setsockshost(const string &nsockshost);

	string getsmtphost() const;
	unsigned int getsmtpport() const;
	void setsmtphost(const string &asmtphost);

	void getsocksuser(string &name, string &pass) const;
	void setsocksuser(const string &name, const string &pass);

	void openurl(const string &url);

	imstatus getstatus(protocolname pname);
	void savestatus(protocolname pname, imstatus st);

	string getprotocolname(protocolname pname) const;
	protocolname getprotocolbyletter(char letter) const;

	int getouridcount() const { return accounts.size(); }
	imaccount getourid(protocolname pname);
	void setourid(const imaccount &im);

	string getawaymsg(protocolname pname) const;
	void setawaymsg(protocolname pname, const string &amsg);

	string getdirname() const { return basedir; }
	string getconfigfname(const string &fname) const { return getdirname() + fname; }

	void commandline(int argc, char **argv);

	bool enoughdiskspace() const { return fenoughdiskspace; }
	void checkdiskspace();
};

extern icqconf conf;

#endif
