#include <sys/types.h>
#include <dirent.h>
#include <fstream.h>

#include "icqconf.h"
#include "icqhist.h"
#include "icqface.h"
#include "icqcontact.h"
#include "icqcontacts.h"

icqconf::icqconf(): uin(0), rs(rscard), rc(rcdark), autoaway(0),
autona(0), hideoffline(false), savepwd(true), antispam(false) {
    setserver(ICQ_SERVER);
}

icqconf::~icqconf() {
}

unsigned int icqconf::getuin() {
    return uin;
}

const string &icqconf::getpassword() {
    return password;
}

void icqconf::setpassword(string npass) {
    password = npass;
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
    loadsounds();
}

void icqconf::loadmainconfig() {
    string fname = (string) getenv("HOME") + "/.centericq/config", buf, param;
    ifstream f(fname.c_str());

    if(f.is_open()) {
	savepwd = false;

	while(!f.eof()) {
	    getline(f, buf);
	    param = getword(buf);

	    if(param == "uin") {
		uin = atol(buf.c_str());
	    } else if(param == "pass") {
		password = buf;
		savepwd = !password.empty();
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
	f << "uin\t" << i2str(fuin ? (uin = fuin) : getuin()) << endl;

	if(getsavepwd()) f << "pass\t" << getpassword() << endl;

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

	f.close();
    }
}

int icqconf::getcolor(int npair) {
    return
	find(boldcolors.begin(), boldcolors.end(), npair) != boldcolors.end()
	? boldcolor(npair) : color(npair);
}

int icqconf::findcolor(string s) {
    int i;
    string colors[] = {"BLACK", "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "CYAN", "WHITE", ""};
    
    for(i = 0; i < s.size(); i++) s[i] = toupper(s[i]);
    for(i = 0; !colors[i].empty() && (s != colors[i]); i++);
    
    return i;
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
		    fo << "*\tmsg\tplay " << SHARE_DIR << "/centericq/msg.wav" << endl;
		    fo << "*\turl\tplay " << SHARE_DIR << "/centericq/url.wav" << endl;
		    fo << "*\tfile\tplay " << SHARE_DIR << "/centericq/file.wav" << endl;
		    fo << "*\tchat\tplay " << SHARE_DIR << "/centericq/chat.wav" << endl;
		    fo << "*\temail\tplay " << SHARE_DIR << "/centericq/email.wav" << endl;
		    fo << "*\tonline\tplay " << SHARE_DIR << "/centericq/online.wav" << endl;
		    fo << "*\tcont\tplay " << SHARE_DIR << "/centericq/cont.wav" << endl;
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

	    if(suin != "*") c = (ffuin = atol(suin.c_str())) ? c = clist.get(ffuin) : 0;
	    else c = clist.get(0);

	    if(c) c->setsound(event, buf);
	}

	fi.close();
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

void icqconf::registerinfo(unsigned int fuin, string passwd, string nick,
string fname, string lname, string email) {
    fuin = uin;
    password = passwd;
    rnick = nick;
    rfname = fname;
    rlname = lname;
    remail = email;
}

regcolor icqconf::getregcolor() {
    return rc;
}

void icqconf::setregcolor(regcolor c) {
    rc = c;
}

regsound icqconf::getregsound() {
    return rs;
}

void icqconf::setregsound(regsound s) {
    rs = s;
}

bool icqconf::gethideoffline() {
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
}

void icqconf::getauto(int &away, int &na) {
    away = autoaway;
    na = autona;
}

bool icqconf::getquote() {
    return quote;
}

void icqconf::setquote(bool use) {
    quote = use;
}

void icqconf::setserver(string nserv) {
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

string icqconf::getservername() {
    return server;
}

unsigned int icqconf::getserverport() {
    return port;
}

void icqconf::setsockshost(string nsockshost) {
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

string icqconf::getsockshost() {
    return sockshost;
}

unsigned int icqconf::getsocksport() {
    return socksport;
}

void icqconf::getsocksuser(string &name, string &pass) {
    name = socksuser;
    pass = sockspass;
}

void icqconf::setsocksuser(string name, string pass) {
    socksuser = name;
    sockspass = pass;
}

bool icqconf::getsavepwd() {
    return savepwd;
}

void icqconf::setsavepwd(bool ssave) {
    savepwd = ssave;
}

void icqconf::sethideoffline(bool fho) {
    hideoffline = fho;
}

bool icqconf::getantispam() {
    return antispam;
}

void icqconf::setantispam(bool fas) {
    antispam = fas;
}
