/*
*
* centericq configuration handling routines
* $Id: icqconf.cc,v 1.65 2002/04/11 17:14:35 konst Exp $
*
* Copyright (C) 2001 by Konstantin Klyagin <konst@konst.org.ua>
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

#include "icqconf.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "abstracthook.h"
#include "imexternal.h"
#include "eventmanager.h"
#include "imlogger.h"

icqconf::icqconf() {
    rs = rscard;
    rc = rcdark;
    fgroupmode = nogroups;

    autoaway = autona = 0;

    hideoffline = antispam = russian = makelog = askaway = false;
    savepwd = mailcheck = fenoughdiskspace = true;

    basedir = (string) getenv("HOME") + "/.centericq/";
}

icqconf::~icqconf() {
}

icqconf::imaccount icqconf::getourid(protocolname pname) {
    imaccount im;
    vector<imaccount>::iterator i;

    i = find(accounts.begin(), accounts.end(), pname);
    im = i == accounts.end() ? imaccount(pname) : *i;

    return im;
}

void icqconf::setourid(const imaccount &im) {
    vector<imaccount>::iterator i;

    static struct {
	string server;
	int port;
    } defservers[protocolname_size] = {
	{ "login.icq.com", 5190 },
	{ "", 0 },
	{ "messenger.hotmail.com", 1863 },
	{ "toc.oscar.aol.com", 9898 },
	{ "irc.openprojects.net", 6667 }
    };

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

	if(!i->port) i->port = defservers[i->pname].port;
    }
}

string icqconf::getawaymsg(protocolname pname) const {
    string r, buf, fname;
    ifstream f;

    if(!(gethook(pname).getcapabilities() & hoptCanSetAwayMsg))
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

    if(!(gethook(pname).getcapabilities() & hoptCanSetAwayMsg))
	return;

    fname = conf.getconfigfname("awaymsg-" + getprotocolname(pname));
    f.open(fname.c_str());

    if(f.is_open()) {
	f << amsg;
	f.close();
    }
}

void icqconf::checkdir() {
    if(access(getdirname().c_str(), F_OK)) {
	mkdir(getdirname().c_str(), S_IREAD | S_IWRITE | S_IEXEC);
    }
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
	mailcheck = askaway = false;
	savepwd = true;
	setsmtphost("");

	while(!f.eof()) {
	    getstring(f, buf);
	    rbuf = buf;
	    param = getword(buf);

	    if(param == "hideoffline") hideoffline = true; else
	    if(param == "russian") russian = true; else
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
	    if(param == "askaway") askaway = true; else {
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

	    if(russian) f << "russian" << endl;
	    if(away) f << "autoaway\t" << i2str(away) << endl;
	    if(na) f << "autona\t" << i2str(na) << endl;
	    if(hideoffline) f << "hideoffline" << endl;
	    if(getquote()) f << "quotemsgs" << endl;
	    if(getantispam()) f << "antispam" << endl;
	    if(getmailcheck()) f << "mailcheck" << endl;
	    if(getaskaway()) f << "askaway" << endl;

	    f << "smtp\t" << getsmtphost() << ":" << dec << getsmtpport() << endl;

	    switch(getgroupmode()) {
		case group1: f << "group1" << endl; break;
		case group2: f << "group2" << endl; break;
	    }

	    if(getmakelog()) f << "log" << endl;

	    vector<imaccount>::iterator ia;
	    for(ia = accounts.begin(); ia != accounts.end(); ia++)
		ia->write(f);

	    f.close();
	}
    }
}

void icqconf::loadcolors() {
    FILE *f;
    char buf[512], sub[512], *p;
    string tname = getconfigfname("colorscheme");
    int nfg, nbg, npair;

    if(access(tname.c_str(), F_OK))
    if(f = fopen(tname.c_str(), "w")) {
	switch(conf.getregcolor()) {
	    case rcdark:
		fprintf(f, "status\tblack/white\n");
		fprintf(f, "dialog_text\tblack/white\n");
		fprintf(f, "dialog_menu\tblack/white\n");
		fprintf(f, "dialog_selected\twhite/transparent\tbold\n");
		fprintf(f, "dialog_highlight\tred/white\n");
		fprintf(f, "dialog_frame\tblue/white\n");
		fprintf(f, "main_text\tcyan/transparent\n");
		fprintf(f, "main_menu\tgreen/transparent\n");
		fprintf(f, "main_selected\tblack/white\n");
		fprintf(f, "main_highlight\tyellow/transparent\tbold\n");
		fprintf(f, "main_frame\tblue/transparent\tbold\n");
		fprintf(f, "clist_icq\tgreen/transparent\n");
		fprintf(f, "clist_msn\tcyan/transparent\n");
		fprintf(f, "clist_yahoo\tmagenta/transparent\n");
		fprintf(f, "clist_infocard\twhite/transparent\n");
		fprintf(f, "clist_aim\tyellow/transparent\n");
		fprintf(f, "clist_irc\tblue/transparent\n");
		break;

	    case rcblue:
		fprintf(f, "status\tblack/white\n");
		fprintf(f, "dialog_text\tblack/cyan\n");
		fprintf(f, "dialog_menu\tblack/cyan\n");
		fprintf(f, "dialog_selected\twhite/black\tbold\n");
		fprintf(f, "dialog_highlight\twhite/cyan\tbold\n");
		fprintf(f, "dialog_frame\tblack/cyan\n");
		fprintf(f, "main_text\twhite/blue\n");
		fprintf(f, "main_menu\twhite/blue\n");
		fprintf(f, "main_selected\tblack/cyan\n");
		fprintf(f, "main_highlight\twhite/blue\tbold\n");
		fprintf(f, "main_frame\tblue/blue\tbold\n");
		fprintf(f, "clist_icq\tgreen/blue\n");
		fprintf(f, "clist_yahoo\tmagenta/blue\n");
		fprintf(f, "clist_msn\tcyan/blue\n");
		fprintf(f, "clist_infocard\twhite/blue\n");
		fprintf(f, "clist_aim\tyellow/blue\n");
		fprintf(f, "clist_irc\tblue/blue\tbold\n");
		break;
	}
	fclose(f);
    }

    if(f = fopen(tname.c_str(), "r")) {
	boldcolors.clear();

	while(!feof(f)) {
	    freads(f, buf, 512);
	    sscanf(buf, "%s", sub);
	    tname = sub;

	    sscanf(buf+tname.size(), "%s", sub);
	    if(p = strchr(sub, '/')) {
		*p = 0;
		nfg = findcolor(sub);
		nbg = findcolor(p+1);
	    }

	    if(tname == "status") npair = cp_status; else
	    if(tname == "dialog_text") npair = cp_dialog_text; else
	    if(tname == "dialog_menu") npair = cp_dialog_menu; else
	    if(tname == "dialog_selected") npair = cp_dialog_selected; else
	    if(tname == "dialog_highlight") npair = cp_dialog_highlight; else
	    if(tname == "dialog_frame") npair = cp_dialog_frame; else
	    if(tname == "main_text") npair = cp_main_text; else
	    if(tname == "main_menu") npair = cp_main_menu; else
	    if(tname == "main_selected") npair = cp_main_selected; else
	    if(tname == "main_highlight") npair = cp_main_highlight; else
	    if(tname == "main_frame") npair = cp_main_frame; else
	    if(tname == "clist_icq") npair = cp_clist_icq; else
	    if(tname == "clist_yahoo") npair = cp_clist_yahoo; else
	    if(tname == "clist_msn") npair = cp_clist_msn; else
	    if(tname == "clist_infocard") npair = cp_clist_infocard; else
	    if(tname == "clist_aim") npair = cp_clist_aim; else
	    if(tname == "clist_irc") npair = cp_clist_irc; else
	    continue;

	    init_pair(npair, nfg, nbg);
	    if(!strcmp(buf+strlen(buf)-4, "bold")) {
		boldcolors.push_back(npair);
	    }
	}
	fclose(f);
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
	    fo << "# icq_<uin>\tfor icq contacts" << endl;
	    fo << "# yahoo_<nickname>\tfor yahoo" << endl;
	    fo << "# msn_<nickname>\tmsn contacts" << endl << "#" << endl;

	    fo << "# <event>\tcan be: ";
	    for(isn = soundnames.begin(); isn != soundnames.end(); isn++) {
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

	    for(isn = soundnames.begin(); isn != soundnames.end(); isn++) {
		if(skey == isn->second) {
		    it = isn->first;
		    break;
		}
	    }

	    if(it == imevent::imeventtype_size)
		continue;

	    if(suin != "*") {
		if((i = suin.find("_")) != -1) {
		    skey = suin.substr(0, i);
		    suin.erase(0, i+1);

		    if(skey == "icq") c = clist.get(imcontact(strtoul(suin.c_str(), 0, 0), icq)); else
		    if(skey == "yahoo") c = clist.get(imcontact(suin, yahoo)); else
		    if(skey == "msn") c = clist.get(imcontact(suin, msn));
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

void icqconf::initpairs() {
    boldcolors.clear();

    boldcolors.push_back(cp_dialog_selected);
    boldcolors.push_back(cp_main_highlight);
    boldcolors.push_back(cp_main_frame);

    init_pair(cp_status, COLOR_BLACK, COLOR_WHITE);

    init_pair(cp_dialog_text, COLOR_BLACK, COLOR_WHITE);
    init_pair(cp_dialog_menu, COLOR_BLACK, COLOR_WHITE);
    init_pair(cp_dialog_selected, COLOR_WHITE, COLOR_BLACK);
    init_pair(cp_dialog_highlight, COLOR_RED, COLOR_WHITE);
    init_pair(cp_dialog_frame, COLOR_BLUE, COLOR_WHITE);

    init_pair(cp_main_text, COLOR_CYAN, COLOR_BLACK);
    init_pair(cp_main_menu, COLOR_GREEN, COLOR_BLACK);
    init_pair(cp_main_selected, COLOR_BLACK, COLOR_WHITE);
    init_pair(cp_main_highlight, COLOR_YELLOW, COLOR_BLACK);
    init_pair(cp_main_frame, COLOR_BLUE, COLOR_BLACK);

    init_pair(cp_clist_icq, COLOR_GREEN, COLOR_BLACK);
    init_pair(cp_clist_msn, COLOR_CYAN, COLOR_BLACK);
    init_pair(cp_clist_yahoo, COLOR_YELLOW, COLOR_BLACK);
    init_pair(cp_clist_infocard, COLOR_WHITE, COLOR_BLACK);
    init_pair(cp_clist_aim, COLOR_YELLOW, COLOR_BLACK);
    init_pair(cp_clist_irc, COLOR_BLUE, COLOR_BLACK);
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

void icqconf::setantispam(bool fas) {
    antispam = fas;
}

void icqconf::setmailcheck(bool fmc) {
    mailcheck = fmc;
}

void icqconf::setaskaway(bool faskaway) {
    askaway = faskaway;
}

void icqconf::openurl(const string &url) {
    int npos;
    string torun = openurlcommand;

    while((npos = torun.find("$url$")) != -1)
	torun.replace(npos, 5, url);

    system(torun.c_str());
}

string icqconf::getprotocolname(protocolname pname) const {
    static const string ptextnames[protocolname_size] = {
	"icq", "yahoo", "msn", "aim", "irc", "infocard"
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
	"", "y", "m", "a", "i", "n"
    };

    return pprefixes[pname];
}

imstatus icqconf::getstatus(protocolname pname) {
    string fname, buf;
    ifstream f;
    imstatus st = available;

    fname = getconfigfname("status-" + getprotocolname(pname));
    f.open(fname.c_str());

    if(f.is_open()) {
	getstring(f, buf);
	st = (imstatus) ((int) atol(buf.c_str()));
	f.close();
    }

    return st;
}

void icqconf::savestatus(protocolname pname, imstatus st) {
    string fname;
    ofstream f;

    fname = getconfigfname("status-" + getprotocolname(pname));
    f.open(fname.c_str());

    if(f.is_open()) {
	f << (int) st << endl;
	f.close();
    }
}

int icqconf::getprotcolor(protocolname pname) const {
    switch(pname) {
	case      icq : return getcolor(cp_clist_icq);
	case    yahoo : return getcolor(cp_clist_yahoo);
	case infocard : return getcolor(cp_clist_infocard);
	case      msn : return getcolor(cp_clist_msn);
	case      aim : return getcolor(cp_clist_aim);
	case      irc : return getcolor(cp_clist_irc);
	default       : return getcolor(cp_main_text);
    }
}

void icqconf::setrussian(bool frussian) {
    russian = frussian;
}

void icqconf::commandline(int argc, char **argv) {
    int i;
    string event, proto, dest;
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

	} else if((args == "-s") || (args == "--send")) {
	    if(argv[++i]) event = argv[i];

	} else if((args == "-p") || (args == "--proto")) {
	    if(argv[++i]) proto = argv[i];

	} else if((args == "-t") || (args == "--to")) {
	    if(argv[++i]) dest = argv[i];

	} else if((args == "-S") || (args == "--status")) {
	    if(argv[++i])
	    if(strlen(argv[i]))
		st = argv[i][0];

	} else {
	    usage();
	    exit(0);
	}
    }

    constructevent(event, proto, dest);
    externalstatuschange(st, proto);
}

void icqconf::constructevent(const string &event, const string &proto, const string &dest) const {
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
	ev = new imsms(cdest, imevent::outgoing, text);
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
		eventnames[ev->gettype()], c->getdesc().totext().c_str());

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

	for(imst = available; imst != imstatus_size; (int) imst += 1)
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
			    conf.savestatus(pname, imst);
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
    cout << "Usage: " << argv0 << " [OPTION].." << endl;

    cout << endl << "General options:" << endl;
    cout << "  --ascii, -a              use characters for windows and UI controls" << endl;
    cout << "  --help                   display this stuff" << endl;

    cout << endl << "Events sending options:" << endl;
    cout << "  -s, --send <event type>  event type; can be msg, sms or url" << endl;
    cout << "  -S, --status <status>    change the current IM status" << endl;
    cout << "  -p, --proto <protocol>   protocol type; can be icq, yahoo or msn" << endl;
    cout << "  -t, --to <destination>   destination UIN or nick (depends on protocol)" << endl;

    cout << endl << "Report bugs to <centericq-bugs@konst.org.ua>." << endl;
}

void icqconf::setmakelog(bool slog) {
    makelog = slog;
}

void icqconf::setgroupmode(icqconf::groupmode amode) {
    fgroupmode = amode;
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
    struct statfs st;

    fenoughdiskspace = true;

    if(!statfs(conf.getdirname().c_str(), &st)) {
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

    f << endl;
    if(!nickname.empty()) f << prefix << "nick\t" << nickname << endl;
    if(uin) f << prefix << "uin\t" << uin << endl;
    if(conf.getsavepwd()) f << prefix << "pass\t" << password << endl;
    if(!server.empty()) {
	f << prefix << "server\t" << server;
	if(port) f << ":" << port;
	f << endl;
    }
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
	}
    }
}

bool icqconf::imaccount::operator == (protocolname apname) const {
    return pname == apname;
}

bool icqconf::imaccount::operator != (protocolname apname) const {
    return pname != apname;
}
