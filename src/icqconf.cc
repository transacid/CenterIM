/*
*
* centericq configuration handling routines
* $Id: icqconf.cc,v 1.22 2001/11/26 13:02:51 konst Exp $
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
#include "icqhist.h"
#include "icqface.h"
#include "icqcontact.h"
#include "icqcontacts.h"

icqconf::icqconf() {
    rs = rscard;
    rc = rcdark;
    autoaway = autona = 0;

    hideoffline = antispam = usegroups = russian = false;
    savepwd = mailcheck = true;
}

icqconf::~icqconf() {
}

icqconf::imaccount icqconf::getourid(protocolname pname) {
    vector<imaccount>::iterator i;

    if((i = find(accounts.begin(), accounts.end(), pname)) != accounts.end()) {
	if(!conf.getsavepwd() && i->password.empty()) {
	    i->password = face.inputstr(_("Password: "), "", '*');
	    if(i->password.empty()) i = accounts.end();
	}
    }

    return i == accounts.end() ? imaccount(pname) : *i;
}

int icqconf::getouridcount() const {
    return accounts.size();
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
}

void icqconf::loadmainconfig() {
    string fname = getconfigfname("config"), buf, param, rbuf;
    ifstream f(fname.c_str());
    imaccount im;
    protocolname pname;

    if(f.is_open()) {
	mailcheck = serveronly = false;
	savepwd = true;

	while(!f.eof()) {
	    getline(f, buf);
	    rbuf = buf;
	    param = getword(buf);

	    if(param == "hideoffline") hideoffline = true; else
	    if(param == "russian") russian = true; else
	    if(param == "autoaway") autoaway = atol(buf.c_str()); else
	    if(param == "autona") autona = atol(buf.c_str()); else
	    if(param == "antispam") antispam = true; else
	    if(param == "mailcheck") mailcheck = true; else
	    if(param == "serveronly") serveronly = true; else
	    if(param == "quotemsgs") quote = true; else
	    if(param == "sockshost") setsockshost(buf); else
	    if(param == "socksusername") socksuser = buf; else
	    if(param == "sockspass") sockspass = buf; else
	    if(param == "usegroups") usegroups = true; else {
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

void icqconf::setourid(const imaccount im) {
    vector<imaccount>::iterator i;

    i = find(accounts.begin(), accounts.end(), im.pname);

    if(i != accounts.end()) {
	if(im.empty()) {
	    accounts.erase(i);
	} else {
	    *i = im;
	}
    } else {
	if(!im.empty()) {
	    accounts.push_back(im);
	    i = accounts.end()-1;

	    switch(i->pname) {
		case icq:
		    if(i->server.empty()) {
			i->server = "icq.mirabilis.com";
			i->port = 4000;
		    }
		    break;
	    }
	}
    }
}

void icqconf::save() {
    string fname = getconfigfname("config"), param;
    ofstream f(fname.c_str());
    int away, na;
    vector<imaccount>::iterator ia;

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
	if(getserveronly()) f << "serveronly" << endl;
	if(getusegroups()) f << "usegroups" << endl;

	for(ia = accounts.begin(); ia != accounts.end(); ia++) {
	    ia->write(f);
	}

	f.close();
    }
}

int icqconf::getcolor(int npair) const {
    return
	find(boldcolors.begin(), boldcolors.end(), npair) != boldcolors.end()
	? boldcolor(npair) : color(npair);
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
		fprintf(f, "dialog_selected\twhite/black\tbold\n");
		fprintf(f, "dialog_highlight\tred/white\n");
		fprintf(f, "dialog_frame\tblue/white\n");
		fprintf(f, "main_text\tcyan/black\n");
		fprintf(f, "main_menu\tgreen/black\n");
		fprintf(f, "main_selected\tblack/white\n");
		fprintf(f, "main_highlight\tyellow/black\tbold\n");
		fprintf(f, "main_frame\tblue/black\tbold\n");
		fprintf(f, "clist_icq\tgreen/black\n");
		fprintf(f, "clist_msn\tcyan/black\n");
		fprintf(f, "clist_yahoo\tmagenta/black\n");
		fprintf(f, "clist_infocard\twhite/black\n");
		fprintf(f, "clist_root\tred/black\tbold\n");

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
		fprintf(f, "clist_root\tred/blue\tbold\n");
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
	    if(tname == "clist_root") npair = cp_clist_root; else
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
    int event, n, ffuin;
    icqcontact *c;
    int i, k;

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);
	for(k = 0; k < SOUND_COUNT; k++) {
	    c->setsound(k, "");
	}
    }

    if(access(tname.c_str(), F_OK)) {
	ofstream fo(tname.c_str());

	if(fo.is_open()) {
	    fo << "# This file contains sound configuration for centericq" << endl;
	    fo << "# Every line should look like: <uin> <event> <command>" << endl << endl;

	    fo << "# <uin>      uin of specific user or *" << endl;
	    fo << "# <event>    can be either msg, url, file, chat, cont, online or email" << endl;
	    fo << "# <command>  command line to be executed to play a sound" << endl << endl;

	    switch(rs) {
		case rscard:
		    fo << "*\tmsg\tplay " << SHARE_DIR << "/msg.wav" << endl;
		    fo << "*\turl\tplay " << SHARE_DIR << "/url.wav" << endl;
		    fo << "*\tfile\tplay " << SHARE_DIR << "/file.wav" << endl;
		    fo << "*\tchat\tplay " << SHARE_DIR << "/chat.wav" << endl;
		    fo << "*\temail\tplay " << SHARE_DIR << "/email.wav" << endl;
		    fo << "*\tonline\tplay " << SHARE_DIR << "/online.wav" << endl;
		    fo << "*\tcont\tplay " << SHARE_DIR << "/cont.wav" << endl;
		    break;
		case rsspeaker:
		    fo << "*\tmsg\t!spk1" << endl;
		    fo << "*\turl\t!spk2" << endl;
		    fo << "*\tfile\t!spk3" << endl;
		    fo << "*\tchat\t!spk4" << endl;
		    fo << "*\temail\t!spk5" << endl;
		    fo << "*\tonline\t!spk6" << endl;
		    fo << "*\tcont\t!spk7" << endl;
		    break;
	    }

	    fo.close();
	}
    }

    ifstream fi(tname.c_str());
    if(fi.is_open()) {
	while(!fi.eof()) {
	    getline(fi, buf);
	    if(buf.empty() || (buf.substr(0, 1) == "#")) continue;

	    suin = getword(buf);
	    skey = getword(buf);

	    if(skey == "msg") event = EVT_MSG; else
	    if(skey == "url") event = EVT_URL; else
	    if(skey == "file") event = EVT_FILE; else
	    if(skey == "email") event = EVT_EMAIL; else
	    if(skey == "online") event = EVT_ONLINE; else
	    if(skey == "chat") event = EVT_CHAT; else continue;

	    if(suin != "*") {
		c = (ffuin = atol(suin.c_str())) ?
		    clist.get(imcontact(ffuin, icq)) : 0;
	    } else {
		c = clist.get(contactroot);
	    }

	    if(c) c->setsound(event, buf);
	}

	fi.close();
    }
}

void icqconf::loadactions() {
    string fname = getconfigfname("actions"), buf, name;

    if(access(fname.c_str(), F_OK)) {
	ofstream of(fname.c_str());
	if(of.is_open()) {
	    of << "# This file contains external actions configuration for centericq" << endl;
	    of << "# Every line should look like: <action> <command>" << endl;
	    of << "# Possible actions are: openurl" << endl << endl;

	    of << "openurl\t";
	    of << "(if test ! -z \"`ps x | grep /netscape | grep -v grep`\"; ";
	    of << "then DISPLAY=0:0 netscape -remote 'openURL($url$, new-window)'; else ";
	    of << "DISPLAY=0:0 netscape \"$url$\"; fi) >/dev/null 2>&1 &" << endl;

	    of.close();
	}
    }

    ifstream f(fname.c_str());
    if(f.is_open()) {
	while(!f.eof()) {
	    getline(f, buf);
	    name = getword(buf);

	    if(name == "openurl") {
		openurlcommand = buf;
	    }
	}

	f.close();
    }
}

regcolor icqconf::getregcolor() const {
    return rc;
}

void icqconf::setregcolor(regcolor c) {
    rc = c;
}

regsound icqconf::getregsound() const {
    return rs;
}

void icqconf::setregsound(regsound s) {
    rs = s;
}

bool icqconf::gethideoffline() const {
    return hideoffline;
}

void icqconf::initpairs() {
    boldcolors.clear();

    boldcolors.push_back(cp_dialog_selected);
    boldcolors.push_back(cp_main_highlight);
    boldcolors.push_back(cp_main_frame);
    boldcolors.push_back(cp_clist_root);

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
    init_pair(cp_clist_root, COLOR_RED, COLOR_BLACK);
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

bool icqconf::getquote() const {
    return quote;
}

void icqconf::setquote(bool use) {
    quote = use;
}

void icqconf::setsockshost(const string nsockshost) {
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

const string icqconf::getsockshost() const {
    return sockshost;
}

unsigned int icqconf::getsocksport() const {
    return socksport;
}

void icqconf::getsocksuser(string &name, string &pass) const {
    name = socksuser;
    pass = sockspass;
}

void icqconf::setsocksuser(const string name, const string pass) {
    socksuser = name;
    sockspass = pass;
}

bool icqconf::getsavepwd() const {
    return savepwd;
}

void icqconf::setsavepwd(bool ssave) {
    savepwd = ssave;
}

void icqconf::sethideoffline(bool fho) {
    hideoffline = fho;
}

bool icqconf::getantispam() const {
    return antispam;
}

void icqconf::setantispam(bool fas) {
    antispam = fas;
}

bool icqconf::getmailcheck() const {
    return mailcheck;
}

void icqconf::setmailcheck(bool fmc) {
    mailcheck = fmc;
}

void icqconf::openurl(const string url) {
    int npos;
    string torun = openurlcommand;

    while((npos = torun.find("$url$")) != -1)
	torun.replace(npos, 5, url);

    system(torun.c_str());
}

bool icqconf::getserveronly() const {
    return serveronly;
}

void icqconf::setserveronly(bool fso) {
    serveronly = fso;
}

const string icqconf::getprotocolname(protocolname pname) const {
    static const string ptextnames[protocolname_size] = {
	"icq", "yahoo", "msn", "infocard"
    };

    return ptextnames[pname];
}

imstatus icqconf::getstatus(protocolname pname) {
    string fname, buf;
    ifstream f;
    imstatus st = available;

    fname = getconfigfname("status-" + getprotocolname(pname));
    f.open(fname.c_str());

    if(f.is_open()) {
	getline(f, buf);
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
	case icq:
	    return getcolor(cp_clist_icq);
	case yahoo:
	    return getcolor(cp_clist_yahoo);
	case infocard:
	    return getcolor(cp_clist_infocard);
	case msn:
	    return getcolor(cp_clist_msn);
	default:
	    return getcolor(cp_main_text);
    }
}

const string icqconf::getdirname() const {
    return (string) getenv("HOME") + "/.centericq/";
}

const string icqconf::getconfigfname(const string fname) const {
    return getdirname() + fname;
}

bool icqconf::getrussian() const {
    return russian;
}

void icqconf::setrussian(bool frussian) {
    russian = frussian;
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

void icqconf::imaccount::read(const string spec) {
    int pos = spec.find("_");
    string pname, buf;

    if(pos != -1) {
	buf = spec;
	pname = getword(buf.erase(0, pos+1));

	if(pname == "nick") nickname = buf; else
	if(pname == "uin") uin = strtoul(buf.c_str(), 0, 0); else
	if(pname == "pass") password = buf; else
	if(pname == "server") {
	    if((pos = buf.find(":")) != -1) {
		server = buf.substr(0, pos);
		port = strtoul(buf.substr(pos+1).c_str(), 0, 0);
	    } else {
		server = buf;
	    }
	}
    }
}

bool icqconf::imaccount::operator == (protocolname apname) const {
    return pname == apname;
}

bool icqconf::imaccount::operator != (protocolname apname) const {
    return pname != apname;
}
