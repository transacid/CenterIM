/*
*
* centericq configuration handling routines
* $Id: icqconf.cc,v 1.14 2001/11/11 14:30:13 konst Exp $
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

icqconf::icqconf(): icquin(0), rs(rscard), rc(rcdark), autoaway(0),
autona(0), hideoffline(false), savepwd(true), antispam(false),
mailcheck(true), usegroups(false) {
    setserver(ICQ_SERVER);
}

icqconf::~icqconf() {
}

void icqconf::geticqlogin(unsigned long &auin, string &apass) {
    auin = icquin;

    if(!conf.getsavepwd() && icqpass.empty()) {
	icqpass = face.inputstr(_("Password: "), "", '*');
	if(icqpass.empty()) {
	    exit(0);
	}
    }

    apass = icqpass;
}

void icqconf::getyahoologin(string &alogin, string &apass) {
    alogin = yahooid;
    apass = yahoopass;
}

void icqconf::checkdir() {
    string hostdir = (string) getenv("HOME") + "/.centericq";

    if(access(hostdir.c_str(), F_OK)) {
	mkdir(hostdir.c_str(), S_IREAD | S_IWRITE | S_IEXEC);
    }
}

void icqconf::load() {
    loadmainconfig();
    loadcolors();
    loadactions();
}

void icqconf::loadmainconfig() {
    string fname = (string) getenv("HOME") + "/.centericq/config", buf, param;
    ifstream f(fname.c_str());

    if(f.is_open()) {
	savepwd = mailcheck = serveronly = false;

	while(!f.eof()) {
	    getline(f, buf);
	    param = getword(buf);

	    if(param == "uin") {
		icquin = atol(buf.c_str());
	    } else if(param == "pass") {
		icqpass = buf;
		savepwd = !icqpass.empty();
	    } else if(param == "hideoffline") {
		hideoffline = true;
	    } else if(param == "russian") {
		icq_Russian = 1;
	    } else if(param == "autoaway") {
		autoaway = atol(buf.c_str());
	    } else if(param == "autona") {
		autona = atol(buf.c_str());
	    } else if(param == "antispam") {
		antispam = true;
	    } else if(param == "mailcheck") {
		mailcheck = true;
	    } else if(param == "serveronly") {
		serveronly = true;
	    } else if(param == "server") {
		setserver(buf);
	    } else if(param == "quotemsgs") {
		quote = true;
	    } else if(param == "sockshost") {
		setsockshost(buf);
	    } else if(param == "socksusername") {
		socksuser = buf;
	    } else if(param == "sockspass") {
		sockspass = buf;
	    } else if(param == "usegroups") {
		usegroups = true;
	    } else if(param == "yahoo_id") {
		yahooid = buf;
	    } else if(param == "yahoo_pass") {
		yahoopass = buf;
	    }
	}

	f.close();
    }
}

void icqconf::savemainconfig(unsigned int fuin = 0) {
    string fname = (string) getenv("HOME") + "/.centericq/config", param;
    ofstream f(fname.c_str());

    if(f.is_open()) {
	f << "server\t" << getservername() << ":" << i2str(getserverport()) << endl;
	f << "uin\t" << i2str(fuin ? (icquin = fuin) : icquin) << endl;

	if(getsavepwd()) f << "pass\t" << icqpass << endl;

	f << "yahoo_id\t" << yahooid << endl
	<< "yahoo_pass\t" << yahoopass << endl;

	if(!getsockshost().empty()) {
	    string user, pass;
	    getsocksuser(user, pass);
	    f << "sockshost\t" << getsockshost() + ":" + i2str(getsocksport()) << endl;
	    f << "socksusername\t" << user << endl;
	    f << "sockspass\t" << pass << endl;
	}

	int away, na;
	getauto(away, na);

	if(icq_Russian) f << "russian" << endl;
	if(away) f << "autoaway\t" << i2str(away) << endl;
	if(na) f << "autona\t" << i2str(na) << endl;
	if(hideoffline) f << "hideoffline" << endl;
	if(getquote()) f << "quotemsgs" << endl;
	if(getantispam()) f << "antispam" << endl;
	if(getmailcheck()) f << "mailcheck" << endl;
	if(getserveronly()) f << "serveronly" << endl;
	if(getusegroups()) f << "usegroups" << endl;

	f.close();
    }
}

int icqconf::getcolor(int npair) {
    return
	find(boldcolors.begin(), boldcolors.end(), npair) != boldcolors.end()
	? boldcolor(npair) : color(npair);
}

void icqconf::loadcolors() {
    FILE *f;
    char buf[512], sub[512], *p;
    string tname = (string) getenv("HOME") + "/.centericq/colorscheme";
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
    string tname = (string) getenv("HOME") + "/.centericq/sounds", buf, suin, skey;
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
    string fname = (string) getenv("HOME") + "/.centericq/actions", buf, name;

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

int icqconf::getstatus() {
    string fname = (string) getenv("HOME") + "/.centericq/laststatus";
    char buf[64] = "0";
    FILE *f;

    if(f = fopen(fname.c_str(), "r")) {
	freads(f, buf, 64);
	fclose(f);
	return atoi(buf);
    } else {
	return STATUS_ONLINE;
    }
}

void icqconf::savestatus(int st) {
    string fname = (string) getenv("HOME") + "/.centericq/laststatus";
    FILE *f;

    if(f = fopen(fname.c_str(), "w")) {
	fprintf(f, "%d\n", st);
	fclose(f);
    }
}

void icqconf::registerinfo(unsigned int fuin, const string passwd,
const string nick, const string fname, const string lname,
const string email) {
    fuin = icquin;
    icqpass = passwd;
    rnick = nick;
    rfname = fname;
    rlname = lname;
    remail = email;
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

void icqconf::setserver(const string nserv) {
    int pos;

    if(!nserv.empty()) {
	if((pos = nserv.find(":")) != -1) {
	    server = nserv.substr(0, pos);
	    port = atol(nserv.substr(pos+1).c_str());
	} else {
	    server = nserv;
	    port = 4000;
	}
    }
}

const string icqconf::getservername() const {
    return server;
}

unsigned int icqconf::getserverport() const {
    return port;
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
