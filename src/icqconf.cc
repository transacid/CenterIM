/*
*
* centericq configuration handling routines
* $Id: icqconf.cc,v 1.108 2003/07/12 17:14:21 konst Exp $
*
* Copyright (C) 2001,2002 by Konstantin Klyagin <konst@konst.org.ua>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at
* your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
* USA
*
*/

#include <sys/types.h>
#include <dirent.h>
#include <fstream>

#ifdef __sun__
#include <sys/statvfs.h>
#endif

#include "icqconf.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "abstracthook.h"
#include "imexternal.h"
#include "eventmanager.h"
#include "imlogger.h"
#include "connwrap.h"

icqconf::icqconf() {
    rs = rscard;
    rc = rcdark;
    fgroupmode = nogroups;

    autoaway = autona = 0;

    hideoffline = antispam = makelog = askaway = logtimestamps =
	logonline = emacs = false;

    savepwd = mailcheck = fenoughdiskspace = logtyping = true;

    for(protocolname pname = icq; pname != protocolname_size; (int) pname += 1) {
	chatmode[pname] = true;
	russian[pname] = false;
    }

    basedir = (string) getenv("HOME") + "/.centericq/";
}

icqconf::~icqconf() {
}

icqconf::imaccount icqconf::getourid(protocolname pname) const {
    imaccount im;
    vector<imaccount>::const_iterator i;

    i = find(accounts.begin(), accounts.end(), pname);
    im = i == accounts.end() ? imaccount(pname) : *i;

    return im;
}

icqconf::imserver icqconf::defservers[protocolname_size] = {
    { "login.icq.com", 5190, 0 },
    { "scs.yahoo.com", 5050, 0 },
    { "messenger.hotmail.com", 1863, 0 },
    { "toc.oscar.aol.com", 9898, 0 },
    { "irc.oftc.net", 6667, 0 },
    { "jabber.com", 5222, 5223 },
    { "", 0, 0 }
};

void icqconf::setourid(const imaccount &im) {
    vector<imaccount>::iterator i;

    i = find(accounts.begin(), accounts.end(), im.pname);

    if(i != accounts.end()) {
	if(im.empty()) {
	    accounts.erase(i);
	    i = accounts.end();
	} else {
	    *i = im;
	}
    } else if(!im.empty()) {
	accounts.push_back(im);
	i = accounts.end()-1;
    }

    if(i != accounts.end()) {
	if(i->server.empty()) {
	    i->server = defservers[i->pname].server;
	    i->port = defservers[i->pname].port;
	}

	if(!i->port)
	    i->port = defservers[i->pname].port;

	switch(i->pname) {
	    case icq:
		if(i->password.size() > 8)
		    i->password = i->password.substr(0, 8);
		break;
	}

	gethook(i->pname).ouridchanged(*i);
    }
}

string icqconf::getawaymsg(protocolname pname) const {
    string r, buf, fname;
    ifstream f;

    if(pname == proto_all) {
	bool same = true;
	string allmsg;

	for(protocolname ipname = icq; ipname != protocolname_size && same; (int) ipname += 1) {
	    if(!getourid(ipname).empty()) {
		buf = getawaymsg(ipname);
		if(!buf.empty()) {
		    if(allmsg.empty()) allmsg = buf;
		    else same = same && buf == allmsg;
		}
	    }
	}

	return same ? allmsg : "";
    }

    if(!gethook(pname).getCapabs().count(hookcapab::setaway))
	return "";

    fname = conf.getconfigfname("awaymsg-" + getprotocolname(pname));
    f.open(fname.c_str());

    if(f.is_open()) {
	while(!f.eof()) {
	    getstring(f, buf);
	    if(!r.empty()) r += "\n";
	    r += buf;
	}

	f.close();
    }

    if(r.empty()) {
	char buf[512];

	sprintf(buf, _("I do really enjoy the default %s away message of %s %s."),
	    getprotocolname(pname).c_str(), PACKAGE, VERSION);

	return buf;
    }

    return r;
}

void icqconf::setawaymsg(protocolname pname, const string &amsg) {
    string fname;
    ofstream f;

    if(!gethook(pname).getCapabs().count(hookcapab::setaway))
	return;

    fname = conf.getconfigfname("awaymsg-" + getprotocolname(pname));
    f.open(fname.c_str());

    if(f.is_open()) {
	f << amsg;
	f.close();
    }
}

void icqconf::checkdir() {
    string dname = getdirname();
    dname.erase(dname.size()-1);

    if(access(dname.c_str(), F_OK))
	mkdir(dname.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
}

void icqconf::load() {
    loadmainconfig();
    loadcolors();
    loadactions();
    external.load();
}

void icqconf::loadmainconfig() {
    string fname = getconfigfname("config"), buf, param, rbuf;
    ifstream f(fname.c_str());
    imaccount im;
    protocolname pname;

    if(f.is_open()) {
	mailcheck = askaway = logtyping = false;
	savepwd = bidi = true;
	setsmtphost("");
	setpeertopeer(0, 65535);

	while(!f.eof()) {
	    getstring(f, buf);
	    rbuf = buf;
	    param = getword(buf);

	    if(param == "hideoffline") hideoffline = true; else
	    if(param == "emacs") emacs = true; else
	    if(param == "autoaway") autoaway = atol(buf.c_str()); else
	    if(param == "autona") autona = atol(buf.c_str()); else
	    if(param == "antispam") antispam = true; else
	    if(param == "mailcheck") mailcheck = true; else
	    if(param == "quotemsgs") quote = true; else
	    if(param == "sockshost") setsockshost(buf); else
	    if(param == "socksusername") socksuser = buf; else
	    if(param == "sockspass") sockspass = buf; else
	    if((param == "usegroups") || (param == "group1")) fgroupmode = group1; else
	    if(param == "group2") fgroupmode = group2; else
	    if(param == "smtp") setsmtphost(buf); else
	    if(param == "log") makelog = true; else
	    if(param == "chatmode") initmultiproto(chatmode, buf); else
	    if(param == "russian") initmultiproto(russian, buf); else
	    if(param == "nobidi") setbidi(false); else
	    if(param == "askaway") askaway = true; else
	    if(param == "logtyping") logtyping = true; else
	    if(param == "logtimestamps") logtimestamps = true; else
	    if(param == "logonline") logonline = true; else
	    if(param == "ptp") {
		ptpmin = atoi(getword(buf, "-").c_str());
		ptpmax = atoi(buf.c_str());
	    } else {
		for(pname = icq; pname != protocolname_size; (int) pname += 1) {
		    buf = getprotocolname(pname);
		    if(param.substr(0, buf.size()) == buf) {
			im = getourid(pname);
			im.read(rbuf);
			setourid(im);
		    }
		}
	    }
	}

	f.close();
    }
}

void icqconf::save() {
    string fname = getconfigfname("config"), param;
    int away, na;

    if(enoughdiskspace()) {
	ofstream f(fname.c_str());

	if(f.is_open()) {
	    if(!getsockshost().empty()) {
		string user, pass;
		getsocksuser(user, pass);
		f << "sockshost\t" << getsockshost() + ":" + i2str(getsocksport()) << endl;
		f << "socksusername\t" << user << endl;
		f << "sockspass\t" << pass << endl;
	    }

	    getauto(away, na);

	    if(away) f << "autoaway\t" << i2str(away) << endl;
	    if(na) f << "autona\t" << i2str(na) << endl;
	    if(hideoffline) f << "hideoffline" << endl;
	    if(emacs) f << "emacs" << endl;
	    if(getquote()) f << "quotemsgs" << endl;
	    if(getantispam()) f << "antispam" << endl;
	    if(getmailcheck()) f << "mailcheck" << endl;
	    if(getaskaway()) f << "askaway" << endl;

	    param = "";

	    for(protocolname pname = icq; pname != protocolname_size; (int) pname += 1)
		if(getchatmode(pname)) param += (string) " " + conf.getprotocolname(pname);

	    if(!param.empty())
		f << "chatmode" << param << endl;

	    param = "";

	    for(protocolname pname = icq; pname != protocolname_size; (int) pname += 1)
		if(getrussian(pname)) param += (string) " " + conf.getprotocolname(pname);

	    if(!param.empty())
		f << "russian" << param << endl;

	    if(!getbidi()) f << "nobidi" << endl;
	    if(logtyping) f << "logtyping" << endl;
	    if(logtimestamps) f << "logtimestamps" << endl;
	    if(logonline) f << "logonline" << endl;

	    f << "smtp\t" << getsmtphost() << ":" << dec << getsmtpport() << endl;
	    f << "ptp\t" << ptpmin << "-" << ptpmax << endl;

	    switch(getgroupmode()) {
		case group1: f << "group1" << endl; break;
		case group2: f << "group2" << endl; break;
	    }

	    if(getmakelog()) f << "log" << endl;

	    vector<imaccount>::iterator ia;
	    for(ia = accounts.begin(); ia != accounts.end(); ++ia)
		ia->write(f);

	    f.close();
	}
    }
}

void icqconf::loadcolors() {
    string tname = getconfigfname("colorscheme");

    switch(conf.getregcolor()) {
	case rcdark:
	    schemer.push(cp_status, "status black/white");
	    schemer.push(cp_dialog_text, "dialog_text   black/white");
	    schemer.push(cp_dialog_menu, "dialog_menu   black/white");
	    schemer.push(cp_dialog_selected, "dialog_selected   white/transparent   bold");
	    schemer.push(cp_dialog_highlight, "dialog_highlight red/white");
	    schemer.push(cp_dialog_frame, "dialog_frame blue/white");
	    schemer.push(cp_main_text, "main_text   cyan/transparent");
	    schemer.push(cp_main_menu, "main_menu   green/transparent");
	    schemer.push(cp_main_selected, "main_selected   black/white");
	    schemer.push(cp_main_highlight, "main_highlight yellow/transparent  bold");
	    schemer.push(cp_main_frame, "main_frame blue/transparent    bold");
	    schemer.push(cp_clist_icq, "clist_icq   green/transparent");
	    schemer.push(cp_clist_yahoo, "clist_yahoo   magenta/transparent");
	    schemer.push(cp_clist_infocard, "clist_infocard white/transparent");
	    schemer.push(cp_clist_msn, "clist_msn   cyan/transparent");
	    schemer.push(cp_clist_aim, "clist_aim   yellow/transparent");
	    schemer.push(cp_clist_irc, "clist_irc    blue/transparent");
	    schemer.push(cp_clist_jabber, "clist_jabber    red/transparent");
	    schemer.push(cp_clist_rss, "clist_rss    white/transparent   bold");
	    break;

	case rcblue:
	    schemer.push(cp_status, "status  black/white");
	    schemer.push(cp_dialog_text, "dialog_text black/cyan");
	    schemer.push(cp_dialog_menu, "dialog_menu black/cyan");
	    schemer.push(cp_dialog_selected, "dialog_selected white/black bold");
	    schemer.push(cp_dialog_highlight, "dialog_highlight    white/cyan  bold");
	    schemer.push(cp_dialog_frame, "dialog_frame    black/cyan");
	    schemer.push(cp_main_text, "main_text   white/blue");
	    schemer.push(cp_main_menu, "main_menu   white/blue");
	    schemer.push(cp_main_selected, "main_selected   black/cyan");
	    schemer.push(cp_main_highlight, "main_highlight  white/blue  bold");
	    schemer.push(cp_main_frame, "main_frame  blue/blue   bold");
	    schemer.push(cp_clist_icq, "clist_icq   green/blue");
	    schemer.push(cp_clist_yahoo, "clist_yahoo magenta/blue");
	    schemer.push(cp_clist_msn, "clist_msn   cyan/blue");
	    schemer.push(cp_clist_infocard, "clist_infocard  white/blue");
	    schemer.push(cp_clist_aim, "clist_aim   yellow/blue");
	    schemer.push(cp_clist_irc, "clist_irc   blue/blue   bold");
	    schemer.push(cp_clist_jabber, "clist_jabber    red/blue");
	    schemer.push(cp_clist_rss, "clist_rss    white/blue   bold");
	    break;
    }

    if(access(tname.c_str(), F_OK)) {
	schemer.save(tname);
    } else {
	schemer.load(tname);
    }
}

void icqconf::loadsounds() {
    string tname = getconfigfname("sounds"), buf, suin, skey;
    int n, ffuin, i;
    icqcontact *c;
    imevent::imeventtype it;

    typedef pair<imevent::imeventtype, string> eventsound;
    vector<eventsound> soundnames;
    vector<eventsound>::iterator isn;

    soundnames.push_back(eventsound(imevent::message, "msg"));
    soundnames.push_back(eventsound(imevent::sms, "sms"));
    soundnames.push_back(eventsound(imevent::url, "url"));
    soundnames.push_back(eventsound(imevent::online, "online"));
    soundnames.push_back(eventsound(imevent::email, "email"));

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	for(it = imevent::message; it != imevent::imeventtype_size; (int) it += 1) {
	    c->setsound(it, "");
	}
    }

    if(access(tname.c_str(), F_OK)) {
	ofstream fo(tname.c_str());

	if(fo.is_open()) {
	    fo << "# This file contains sound configuration for centericq" << endl;
	    fo << "# Every line should look like: <id> <event> <command>" << endl << "#" << endl;

	    fo << "# <id>\tid of a contact; can be one of the following" << endl;
	    fo << "# *\tmeans default sound for all the contacts" << endl;
	    fo << "# icq_<uin>\tfor icq contacts (e.g. icq_123)" << endl;
	    fo << "# yahoo_<nickname>\tfor yahoo (e.g. yahoo_thekonst)" << endl;
	    fo << "# .. etc. Similar for the other protocols" << endl << "#" << endl;

	    fo << "# <event>\tcan be: ";
	    for(isn = soundnames.begin(); isn != soundnames.end(); ++isn) {
		if(isn != soundnames.begin()) fo << ", ";
		fo << isn->second;
	    }

	    fo << endl << "# <command>\tcommand line to be executed to play a sound" << endl << endl;

	    switch(rs) {
		case rscard:
		    fo << "*\tmsg\tplay " << SHARE_DIR << "/msg.wav" << endl;
		    fo << "*\turl\tplay " << SHARE_DIR << "/url.wav" << endl;
		    fo << "*\temail\tplay " << SHARE_DIR << "/email.wav" << endl;
		    fo << "*\tonline\tplay " << SHARE_DIR << "/online.wav" << endl;
		    fo << "*\tsms\tplay " << SHARE_DIR << "/sms.wav" << endl;
		    break;

		case rsspeaker:
		    fo << "*\tmsg\t!spk1" << endl;
		    fo << "*\turl\t!spk2" << endl;
		    fo << "*\tsms\t!spk3" << endl;
		    fo << "*\temail\t!spk5" << endl;
		    break;
	    }

	    fo.close();
	}
    }

    ifstream fi(tname.c_str());
    if(fi.is_open()) {
	while(!fi.eof()) {
	    getstring(fi, buf);
	    if(buf.empty() || (buf.substr(0, 1) == "#")) continue;

	    suin = getword(buf);
	    skey = getword(buf);

	    it = imevent::imeventtype_size;

	    for(isn = soundnames.begin(); isn != soundnames.end(); ++isn) {
		if(skey == isn->second) {
		    it = isn->first;
		    break;
		}
	    }

	    if(it == imevent::imeventtype_size)
		continue;

	    c = 0;

	    if(suin != "*") {
		if((i = suin.find("_")) != -1) {
		    skey = suin.substr(0, i);
		    suin.erase(0, i+1);

		    imcontact ic;
		    protocolname pname;

		    for(pname = icq; pname != protocolname_size && skey != getprotocolname(pname); (int) pname += 1);

		    if(pname != protocolname_size) {
			if(pname == icq) ic = imcontact(strtoul(suin.c_str(), 0, 0), pname);
			else ic = imcontact(suin, pname);

			c = clist.get(ic);
		    }
		}
	    } else {
		c = clist.get(contactroot);
	    }

	    if(c) c->setsound(it, buf);
	}

	fi.close();
    }
}

void icqconf::loadactions() {
    string fname = getconfigfname("actions"), buf, name, browser;
    ifstream f;
    bool cont;

    if(access(fname.c_str(), F_OK)) {
	buf = getenv("PATH") ? getenv("PATH") : "";

	if(pathfind(browser = "mozilla", buf, X_OK).empty()) {
	    browser = "netscape";
	}

	ofstream of(fname.c_str());

	if(of.is_open()) {
	    of << "# This file contains external actions configuration for centericq" << endl;
	    of << "# Every line should look like: <action> <command>" << endl;
	    of << "# Possible actions are: openurl" << endl << endl;

	    of << "openurl \\" << endl;
	    of << "    (if test ! -z \"`ps x | grep /" << browser << " | grep -v grep`\"; \\" << endl;
	    of << "\tthen DISPLAY=0:0 " << browser << " -remote 'openURL($url$, new-window)'; \\" << endl;
	    of << "\telse DISPLAY=0:0 " << browser << " \"$url$\"; \\" << endl;
	    of << "    fi) >/dev/null 2>&1 &" << endl;

	    of.close();
	}
    }

    f.open(fname.c_str());

    if(f.is_open()) {
	openurlcommand = "";
	cont = false;

	while(!f.eof()) {
	    getstring(f, buf);

	    if(!cont) {
		name = getword(buf);
	    }

	    if(name == "openurl") {
		if(!buf.empty()) {
		    if(cont = buf.substr(buf.size()-1, 1) == "\\")
			buf.erase(buf.size()-1, 1);
		}

		openurlcommand += buf;
	    }
	}

	f.close();
    }
}

icqconf::regcolor icqconf::getregcolor() const {
    return rc;
}

void icqconf::setregcolor(icqconf::regcolor c) {
    rc = c;
}

icqconf::regsound icqconf::getregsound() const {
    return rs;
}

void icqconf::setregsound(icqconf::regsound s) {
    rs = s;
}

void icqconf::setauto(int away, int na) {
    autoaway = away;
    autona = na;

    if(away == na)
	autoaway = 0;
}

void icqconf::getauto(int &away, int &na) const {
    away = autoaway;
    na = autona;

    if(away == na)
	away = 0;
}

void icqconf::setquote(bool use) {
    quote = use;
}

void icqconf::setsockshost(const string &nsockshost) {
    int pos;

    if(!nsockshost.empty()) {
	if((pos = nsockshost.find(":")) != -1) {
	    sockshost = nsockshost.substr(0, pos);
	    socksport = atol(nsockshost.substr(pos+1).c_str());
	} else {
	    sockshost = nsockshost;
	    socksport = 1080;
	}
    } else {
	sockshost = "";
    }
}

string icqconf::getsockshost() const {
    return sockshost;
}

unsigned int icqconf::getsocksport() const {
    return socksport;
}

void icqconf::getsocksuser(string &name, string &pass) const {
    name = socksuser;
    pass = sockspass;
}

void icqconf::setsocksuser(const string &name, const string &pass) {
    socksuser = name;
    sockspass = pass;
}

void icqconf::setsavepwd(bool ssave) {
    savepwd = ssave;
}

void icqconf::sethideoffline(bool fho) {
    hideoffline = fho;
}

void icqconf::setemacs(bool fem) {
    emacs = fem;
}

void icqconf::setantispam(bool fas) {
    antispam = fas;
}

void icqconf::setmailcheck(bool fmc) {
    mailcheck = fmc;
}

void icqconf::setaskaway(bool faskaway) {
    askaway = faskaway;
}

bool icqconf::getchatmode(protocolname pname) {
    switch(pname) {
	case infocard:
	case rss:
	    return false;
    }

    return chatmode[pname];
}

void icqconf::setchatmode(protocolname pname, bool fchatmode) {
    chatmode[pname] = fchatmode;
}

bool icqconf::getrussian(protocolname pname) {
    return russian[pname];
}

void icqconf::setrussian(protocolname pname, bool frussian) {
    russian[pname] = frussian;
}

void icqconf::openurl(const string &url) {
    int npos;
    string torun = openurlcommand;

    while((npos = torun.find("$url$")) != -1)
	torun.replace(npos, 5, url);

    system(torun.c_str());

    face.log(_("+ launched the openurl command"));
}

string icqconf::getprotocolname(protocolname pname) const {
    static const string ptextnames[protocolname_size] = {
	"icq", "yahoo", "msn", "aim", "irc", "jabber", "rss", "infocard"
    };

    return ptextnames[pname];
}

protocolname icqconf::getprotocolbyletter(char letter) const {
    switch(letter) {
	case 'y': return yahoo;
	case 'a': return aim;
	case 'i': return irc;
	case 'm': return msn;
	case 'n': return infocard;
	case 'j': return jabber;
	case 'r': return rss;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': return icq;
    }

    return protocolname_size;
}

string icqconf::getprotocolprefix(protocolname pname) const {
    static const string pprefixes[protocolname_size] = {
	"", "y", "m", "a", "i", "j", "r", "n"
    };

    return pprefixes[pname];
}

imstatus icqconf::getstatus(protocolname pname) {
    imstatus st = available;
    map<string, string>::iterator ia;
    imaccount a = getourid(pname);

    if((ia = a.additional.find("status")) != a.additional.end()) {
	if(!ia->second.empty()) {
	    for(st = offline; st != imstatus_size && imstatus2char[st] != ia->second[0]; (int) st += 1);
	    if(st == imstatus_size) st = available;
	}
    }

    return st;
}

void icqconf::savestatus(protocolname pname, imstatus st) {
    imaccount im = getourid(pname);
    im.additional["status"] = string(1, imstatus2char[st]);
    setourid(im);
}

int icqconf::getprotcolor(protocolname pname) const {
    switch(pname) {
	case      icq : return getcolor(cp_clist_icq);
	case    yahoo : return getcolor(cp_clist_yahoo);
	case infocard : return getcolor(cp_clist_infocard);
	case      msn : return getcolor(cp_clist_msn);
	case      aim : return getcolor(cp_clist_aim);
	case      irc : return getcolor(cp_clist_irc);
	case   jabber : return getcolor(cp_clist_jabber);
	case      rss : return getcolor(cp_clist_rss);
	default       : return getcolor(cp_main_text);
    }
}

void icqconf::setbidi(bool fbidi) {
    bidi = fbidi;
#ifdef USE_FRIBIDI
    use_fribidi = bidi;
#endif
}

void icqconf::commandline(int argc, char **argv) {
    int i;
    string event, proto, dest, number;
    char st = 0;

    argv0 = argv[0];

    for(i = 1; i < argc; i++) {
	string args = argv[i];

	if((args == "-a") || (args == "--ascii")) {
	    kintf_graph = 0;

	} else if((args == "-b") || (args == "--basedir")) {
	    if(argv[++i]) {
		basedir = argv[i];
		if(basedir.substr(basedir.size()-1) != "/") basedir += "/";
	    }

	} else if((args == "-B") || (args == "--bind")) {
	    if(argv[++i]) {
		bindhost = argv[i];
		cw_setbind(bindhost.c_str());
	    }

	} else if((args == "-P") || (args == "--http-proxy")) {
	    if(argv[++i]) {
		proxyhost = argv[i];
//                cw_setproxy(proxyhost.c_str());
	    }

	} else if((args == "-s") || (args == "--send")) {
	    if(argv[++i]) event = argv[i];

	} else if((args == "-p") || (args == "--proto")) {
	    if(argv[++i]) proto = argv[i];

	} else if((args == "-t") || (args == "--to")) {
	    if(argv[++i]) dest = argv[i];

	} else if((args == "-n") || (args == "--number")) {
	    if(argv[++i]) number = argv[i];

	} else if((args == "-S") || (args == "--status")) {
	    if(argv[++i])
	    if(strlen(argv[i]))
		st = argv[i][0];

	} else if((args == "-v") || (args == "--version")) {
	    cout << PACKAGE << " " << VERSION << endl
		<< "Written by Konstantin Klyagin." << endl << endl
		<< "This is free software; see the source for copying conditions.  There is NO" << endl
		<< "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << endl;

	    exit(0);

	} else {
	    usage();
	    exit(0);
	}
    }

    if(!number.empty()) {
	if(event.empty()) event = "sms";
	if(proto.empty()) proto = "icq";
	dest = "0";
    }

    constructevent(event, proto, dest, number);
    externalstatuschange(st, proto);
}

void icqconf::constructevent(const string &event, const string &proto,
const string &dest, const string &number) const {

    imevent *ev = 0;
    imcontact cdest;
    string text, buf;
    int pos;

    if(event.empty() && dest.empty()) return; else
    if(event.empty() || proto.empty() || dest.empty()) {
	cout << _("event sending error: not enough parameters") << endl;
	exit(1);
    }

    while(getline(cin, buf)) {
	if(!text.empty()) text += "\n";
	text += buf;
    }

    if(proto == "icq") {
	if(dest.find_first_not_of("0123456789") != -1) {
	    cout << _("event sending error: only UINs are allowed with icq protocol") << endl;
	    exit(1);
	}
	cdest = imcontact(strtoul(dest.c_str(), 0, 0), icq);
    } else {
	protocolname pname;

	for(pname = icq; pname != protocolname_size; (int) pname += 1) {
	    if(getprotocolname(pname) == proto) {
		cdest = imcontact(dest, pname);
		break;
	    }
	}

	if(pname == protocolname_size) {
	    cout << _("event sending error: unknown IM type") << endl;
	    exit(1);
	}
    }

    if(event == "msg") {
	ev = new immessage(cdest, imevent::outgoing, text);
    } else if(event == "url") {
	string url;

	if((pos = text.find("\n")) != -1) {
	    url = text.substr(0, pos);
	    text.erase(0, pos+1);
	} else {
	    url = text;
	    text = "";
	}

	ev = new imurl(cdest, imevent::outgoing, url, text);
    } else if(event == "sms") {
	ev = new imsms(cdest, imevent::outgoing, text, number);
    } else {
	cout << _("event sending error: unknown event type") << endl;
	exit(1);
    }

    if(ev) {
	icqcontact *c = new icqcontact(cdest);

	if(!access(c->getdirname().c_str(), X_OK)) {
	    clist.add(c);
	    em.store(*ev);

	    char buf[512];
	    sprintf(buf, _("%s to %s has been put to the queue"),
		streventname(ev->gettype()),
		number.empty() ? c->getdesc().totext().c_str() : number.c_str());

	    cout << buf << endl;
	} else {
	    cout << _("event sending error: the destination contact doesn't exist on your list") << endl;
	    exit(1);
	}

	delete c;
	delete ev;

	exit(0);
    }
}

void icqconf::externalstatuschange(char st, const string &proto) const {
    imstatus imst;
    protocolname pname;
    int rpid;

    if(st) {
	for(pname = icq; pname != protocolname_size; (int) pname += 1)
	    if(getprotocolname(pname) == proto)
		break;

	for(imst = offline; imst != imstatus_size; (int) imst += 1)
	    if(imstatus2char[imst] == st)
		break;

	try {
	    if(pname != protocolname_size) {
		if(imst != imstatus_size) {
		    ifstream f(conf.getconfigfname("pid").c_str());
		    if(f.is_open()) {
			f >> rpid;
			f.close();

			if(rpid > 0) {
			    string sfname = conf.getconfigfname((string) "status-" + proto);
			    ofstream sf(sfname.c_str());
			    if(sf.is_open()) sf << imstatus2char[imst], sf.close();

			    kill(rpid, SIGUSR1);
			}
		    }
		} else {
		    throw _("unknown status character was given");
		}
	    } else {
		throw _("unknown IM type");
	    }
	} catch(char *p) {
	    cout << _("status change error: ") << p << endl;
	    exit(1);
	}

	exit(0);
    }
}

void icqconf::usage() const {
    cout << _("Usage: ") << argv0 << " [OPTION].." << endl;

    cout << endl << _("General options:") << endl;
    cout << _("  --ascii, -a              use ASCII characters for windows and UI controls") << endl;
    cout << _("  --basedir, -b <path>     set a custom base directory") << endl;
    cout << _("  --bind, -B <host/ip>     bind a custom local IP") << endl;
    cout << _("  --help                   display this stuff") << endl;
    cout << _("  --version, -v            show the program version info") << endl;

    cout << endl << _("Events sending options:") << endl;
    cout << _("  -s, --send <event type>  event type; can be msg, sms or url") << endl;
    cout << _("  -S, --status <status>    change the current IM status") << endl;
    cout << _("  -p, --proto <protocol>   protocol type; can be icq, yahoo, msn, aim, irc or jabber") << endl;
    cout << _("  -t, --to <destination>   destination UIN or nick (depends on protocol)") << endl;
    cout << _("  -n, --number <phone#>    mobile number to send an event to (sms only)") << endl;

    cout << endl << _("Report bugs to <centericq-bugs@konst.org.ua>.") << endl;
}

void icqconf::setmakelog(bool slog) {
    makelog = slog;
}

void icqconf::setgroupmode(icqconf::groupmode amode) {
    fgroupmode = amode;
}

void icqconf::initmultiproto(bool p[], string buf) {
    string w;
    protocolname pname;

    for(pname = icq; pname != protocolname_size; (int) pname += 1)
	p[pname] = buf.empty();

    while(!(w = getword(buf)).empty()) {
	for(pname = icq; pname != protocolname_size; (int) pname += 1) {
	    if(getprotocolname(pname) == w) {
		p[pname] = true;
		break;
	    }
	}
    }
}

void icqconf::setsmtphost(const string &asmtphost) {
    int pos;

    smtphost = "";
    smtpport = 0;

    if(!asmtphost.empty()) {
	if((pos = asmtphost.find(":")) != -1) {
	    smtphost = asmtphost.substr(0, pos);
	    smtpport = atol(asmtphost.substr(pos+1).c_str());
	} else {
	    smtphost = asmtphost;
	    smtpport = 25;
	}
    }

    if(smtphost.empty() || !smtpport) {
	smtphost = getsmtphost();
	smtpport = getsmtpport();
    }
}

string icqconf::getsmtphost() const {
    return smtphost.empty() ? "localhost" : smtphost;
}

unsigned int icqconf::getsmtpport() const {
    return smtpport ? smtpport : 25;
}

void icqconf::checkdiskspace() {
    fenoughdiskspace = true;

#ifndef __sun__
    struct statfs st;
    if(!statfs(conf.getdirname().c_str(), &st)) {
#else
    struct statvfs st;
    if(!statvfs(conf.getdirname().c_str(), &st)) {
#endif
	fenoughdiskspace = ((double) st.f_bavail) * st.f_bsize >= 10240;
    }
}

// ----------------------------------------------------------------------------

icqconf::imaccount::imaccount() {
    uin = port = 0;
    pname = infocard;
}

icqconf::imaccount::imaccount(protocolname apname) {
    uin = port = 0;
    pname = apname;
}

bool icqconf::imaccount::empty() const {
    return !uin && nickname.empty();
}

void icqconf::imaccount::write(ofstream &f) {
    string prefix = conf.getprotocolname(pname) + "_";
    map<string, string>::iterator ia;

    f << endl;
    if(!nickname.empty()) f << prefix << "nick\t" << nickname << endl;
    if(uin) f << prefix << "uin\t" << uin << endl;
    if(conf.getsavepwd()) f << prefix << "pass\t" << password << endl;
    if(!server.empty()) {
	f << prefix << "server\t" << server;
	if(port) f << ":" << port;
	f << endl;
    }

    for(ia = additional.begin(); ia != additional.end(); ++ia)
	if(!ia->first.empty() && !ia->second.empty())
	    f << prefix << ia->first << "\t" << ia->second << endl;
}

void icqconf::imaccount::read(const string &spec) {
    int pos = spec.find("_");
    string spname, buf;

    if(pos != -1) {
	buf = spec;
	spname = getword(buf.erase(0, pos+1));

	if(spname == "nick") nickname = buf; else
	if(spname == "uin") uin = strtoul(buf.c_str(), 0, 0); else
	if(spname == "pass") password = buf; else
	if(spname == "server") {
	    if((pos = buf.find(":")) != -1) {
		server = buf.substr(0, pos);
		port = strtoul(buf.substr(pos+1).c_str(), 0, 0);
	    } else {
		server = buf;
	    }

	    if(pname == icq)
	    if(server == "icq.mirabilis.com")
		server = "";
	} else {
	    additional[spname] = buf;
	}
    }
}

bool icqconf::imaccount::operator == (protocolname apname) const {
    return pname == apname;
}

bool icqconf::imaccount::operator != (protocolname apname) const {
    return pname != apname;
}
