#ifndef __ICQCONF_H__
#define __ICQCONF_H__

#include "icqcommon.h"

#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>

#include "config.h"

#include "kkstrtext.h"
#include "kkfsys.h"
#include "conscommon.h"
#include "colorschemer.h"

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
    cp_clist_irc,
    cp_clist_jabber,
    cp_clist_rss
};

class icqconf {
    public:
	enum regsound { rscard, rsspeaker, rsdisable, rsdontchange };
	enum regcolor { rcdark, rcblue, rcdontchange };
	enum groupmode { group1, group2, nogroups };

	struct imserver {
	    string server;
	    int port, secureport;
	};

	struct imaccount {
	    imaccount();
	    imaccount(protocolname apname);

	    string nickname, server, password;
	    unsigned long uin, port;
	    protocolname pname;
	    map<string, string> additional;

	    void write(ofstream &f);
	    void read(const string &spec);

	    bool empty() const;

	    bool operator == (protocolname apname) const;
	    bool operator != (protocolname apname) const;
	};

	static imserver defservers[];

    protected:
	vector<icqgroup> groups;
	vector<imaccount> accounts;

	unsigned short socksport, smtpport, httpproxyport;

	int autoaway, autona, ptpmin, ptpmax;

	bool hideoffline, quote, savepwd, antispam, mailcheck,
	    makelog, fenoughdiskspace, askaway, bidi, logtimestamps,
	    logonline, logtyping, emacs;

	bool chatmode[protocolname_size], russian[protocolname_size];

	string sockshost, socksuser, sockspass, openurlcommand, basedir,
	    argv0, smtphost, bindhost, httpproxyhost;

	regsound rs;
	regcolor rc;
	groupmode fgroupmode;

	colorschemer<cicq_colorpairs> schemer;

	void loadmainconfig();

	void usage() const;

	void constructevent(const string &event, const string &proto,
	    const string &dest, const string &number) const;

	void externalstatuschange(char st, const string &proto) const;

	void initmultiproto(bool p[], string buf);

    public:
	icqconf();
	~icqconf();

	void checkdir();
	void load();

	int getcolor(cicq_colorpairs cp) const { return schemer[cp]; }

	int getprotcolor(protocolname pname) const;

	void save();

	void loadcolors();
	void loadsounds();
	void loadactions();

	regcolor getregcolor() const;
	void setregcolor(regcolor c);

	regsound getregsound() const;
	void setregsound(regsound s);

	bool getemacs() const { return emacs; }
	void setemacs(bool fem);

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

	bool getrussian(protocolname pname);
	void setrussian(protocolname pname, bool frussian);

	bool getbidi() const { return bidi; }
	void setbidi(bool fbidi);

	bool getaskaway() const { return askaway; }
	void setaskaway(bool faskaway);

	bool getchatmode(protocolname pname);
	void setchatmode(protocolname pname, bool fchatmode);

	groupmode getgroupmode() const { return fgroupmode; }
	void setgroupmode(groupmode amode);

	string getsockshost() const;
	unsigned int getsocksport() const;
	void setsockshost(const string &nsockshost);

	string getsmtphost() const;
	unsigned int getsmtpport() const;
	void setsmtphost(const string &asmtphost);

	string gethttpproxyhost() const;
	unsigned int gethttpproxyport() const;
	void sethttpproxyhost(const string &ahttpproxyhost);

	void getsocksuser(string &name, string &pass) const;
	void setsocksuser(const string &name, const string &pass);

	void getpeertopeer(int &min, int &max) const { min = ptpmin; max = ptpmax; }
	void setpeertopeer(int min, int max) { ptpmin = min; ptpmax = max; }

	void openurl(const string &url);

	imstatus getstatus(protocolname pname);
	void savestatus(protocolname pname, imstatus st);

	string getprotocolname(protocolname pname) const;
	protocolname getprotocolbyletter(char letter) const;
	string getprotocolprefix(protocolname pname) const;

	int getouridcount() const { return accounts.size(); }
	imaccount getourid(protocolname pname) const;
	void setourid(const imaccount &im);

	string getawaymsg(protocolname pname) const;
	void setawaymsg(protocolname pname, const string &amsg);

	string getdirname() const { return basedir; }
	string getconfigfname(const string &fname) const { return getdirname() + fname; }

	void commandline(int argc, char **argv);

	bool enoughdiskspace() const { return fenoughdiskspace; }
	void checkdiskspace();

	string getbindhost() const { return bindhost; }

	void getlogoptions(bool &flogtimestamps, bool &flogonline, bool &flogtyping)
	    { flogtimestamps = logtimestamps, flogonline = logonline, flogtyping = logtyping; }

	void setlogoptions(bool flogtimestamps, bool flogonline, bool flogtyping)
	    { logtimestamps = flogtimestamps, logonline = flogonline, logtyping = flogtyping; }
};

extern icqconf conf;

#endif
