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
    cp_main_history_incoming,
    cp_main_history_outgoing,
    cp_clist_icq,
    cp_clist_yahoo,
    cp_clist_infocard,
    cp_clist_msn,
    cp_clist_aim,
    cp_clist_irc,
    cp_clist_jabber,
    cp_clist_rss,
    cp_clist_lj,
    cp_clist_gadu,
    cp_clist_online,
    cp_clist_offline,
    cp_clist_away,
    cp_clist_dnd,
    cp_clist_na,
    cp_clist_occupied,
    cp_clist_free_for_chat,
    cp_clist_invisible,
    cp_clist_not_in_list
};

enum cicq_keybindings {
    section_contact,
    section_history,
    section_editor,
    section_info,
    key_info,
    key_quit,
    key_change_status,
    key_history,
    key_fetch_away_message,
    key_user_menu,
    key_general_menu,
    key_hide_offline,
    key_contact_external_action,
    key_add,
    key_fullscreen,
    key_remove,
    key_send_message,
    key_compose,
    key_send_contact,
    key_send_url,
    key_rename,
    key_version,
    key_edit,
    key_ignore,
    key_quickfind,
    key_search,
    key_search_again,
    key_next_chat,
    key_prev_chat,
    key_show_urls,
    key_rss_check,
    key_multiple_recipients,
    key_user_external_action
};

class icqconf {
    public:
	enum regsound { rscard, rsspeaker, rsdisable, rsdontchange };
	enum regcolor { rcdark, rcblue, rcdontchange };
	enum groupmode { group1, group2, nogroups };
	enum colormode { cmproto, cmstatus };

	struct imserver {
	    string server;
	    int port, secureport;
	};

	struct keybinding {
	    int section, key, action;
	    bool alt, ctrl;
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

	static vector<keybinding> keys;

    protected:
	vector<icqgroup> groups;
	vector<imaccount> accounts;

	unsigned short socksport, smtpport, httpproxyport;

	int autoaway, autona, ptpmin, ptpmax;

	bool hideoffline, quote, savepwd, antispam, mailcheck,
	    makelog, fenoughdiskspace, askaway, bidi, logtimestamps,
	    logonline, emacs, proxyconnect, proxyssl, notitles, debug;

	bool startoffline;

	bool chatmode[protocolname_size];
	bool cpconvert[protocolname_size];
	bool entersends[protocolname_size];
	bool nonimonline[protocolname_size];

	string sockshost, socksuser, sockspass, basedir, argv0, smtphost,
	    bindhost, httpproxyhost, httpproxyuser, httpproxypasswd,
	    fromcharset, tocharset;

	map<string, string> actions;

	regsound rs;
	regcolor rc;
	groupmode fgroupmode;
	colormode cm;

	colorschemer<cicq_colorpairs> schemer;

	void loadmainconfig();
	void usage() const;

	void constructevent(const string &event, const string &proto,
	    const string &dest, const string &number) const;

	void externalstatuschange(char st, const string &proto) const;
	void initmultiproto(bool p[], string buf, bool excludenochat);

	void selfsignal(int signum) const;
	void setproxy();

    public:
	icqconf();
	~icqconf();

	void checkdir();
	void load();

	int getcolor(cicq_colorpairs cp) const { return schemer[cp]; }

	int getprotcolor(protocolname pname) const;

	int getstatuscolor(imstatus status) const;

	void save();

	void loadkeys();
	void loadcolors();
	void loadsounds();
	void loadactions();

	regcolor getregcolor() const;
	void setregcolor(regcolor c);

	regsound getregsound() const;
	void setregsound(regsound s);

	colormode getcolormode() const;
	void setcolormode(colormode c);

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

	bool getproxyconnect() const { return proxyconnect; }
	void setproxyconnect(bool proxyconnect);

	bool getproxyssl() const { return proxyssl; }
	void setproxyssl(bool sproxyssl);

	bool getcpconvert(protocolname pname) const;
	void setcpconvert(protocolname pname, bool fcpconvert);

	bool getbidi() const { return bidi; }
	void setbidi(bool fbidi);

	bool getaskaway() const { return askaway; }
	void setaskaway(bool faskaway);

	bool getchatmode(protocolname pname);
	void setchatmode(protocolname pname, bool fchatmode);

	bool getentersends(protocolname pname);
	void setentersends(protocolname pname, bool fentersends);

	bool getnonimonline(protocolname pname);
	void setnonimonline(protocolname pname, bool fnonimonline);

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
	string gethttpproxyuser() const;
	string gethttpproxypasswd() const;

	void getsocksuser(string &name, string &pass) const;
	void setsocksuser(const string &name, const string &pass);

	void getpeertopeer(int &min, int &max) const { min = ptpmin; max = ptpmax; }
	void setpeertopeer(int min, int max) { ptpmin = min; ptpmax = max; }

	string execaction(const string &action, const string &param = "");

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

	void getlogoptions(bool &flogtimestamps, bool &flogonline)
	    { flogtimestamps = logtimestamps, flogonline = logonline; }

	void setlogoptions(bool flogtimestamps, bool flogonline)
	    { logtimestamps = flogtimestamps, logonline = flogonline; }

	void setcharsets(const string &from, const string &to);

	const char *getconvertfrom(protocolname pname = protocolname_size) const;
	const char *getconvertto(protocolname pname = protocolname_size) const;

	bool getxtitles() const { return !notitles; }
	bool getdebug() const { return debug; }
};

extern icqconf conf;

#endif
