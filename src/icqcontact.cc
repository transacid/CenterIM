/*
*
* centericq single icq contact class
* $Id: icqcontact.cc,v 1.60 2002/08/16 16:48:26 konst Exp $
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

#include "icqcontacts.h"
#include "icqgroups.h"
#include "icqconf.h"
#include "centericq.h"
#include "icqface.h"
#include "abstracthook.h"
#include "imexternal.h"

#include <time.h>
#include <libicq2000/userinfohelpers.h>

icqcontact::icqcontact(const imcontact adesc) {
    string fname, tname;
    imevent::imeventtype ie;
    int i;

    clear();
    nmsgs = lastread = fhistoffset = 0;
    status = offline;
    finlist = true;
    congratulated = false;

    for(ie = imevent::message; ie != imevent::imeventtype_size; (int) ie += 1)
	sound[ie] = "";

    cdesc = adesc;

    switch(cdesc.pname) {
	case infocard:
	    if(!cdesc.uin) {
		fname = conf.getdirname() + "n";

		for(i = 1; ; i++) {
		    tname = fname + i2str(i);
		    if(access(tname.c_str(), F_OK)) break;
		}

		cdesc.uin = i;
	    }
	    load();
	    break;

	default:
	    load();
	    scanhistory();
	    break;
    }
}

icqcontact::~icqcontact() {
}

string icqcontact::tosane(const string &p) const {
    string buf;
    string::iterator i;

    for(buf = p, i = buf.begin(); i != buf.end(); ++i) {
	if(strchr("\n\r", *i)) *i = ' ';
    }

    return buf;
}

string icqcontact::getdirname() const {
    string ret;

    ret = conf.getdirname();

    switch(cdesc.pname) {
	case infocard:
	    ret += "n" + i2str(cdesc.uin);
	    break;
	case icq:
	    ret += i2str(cdesc.uin);
	    break;
	case yahoo:
	    ret += "y" + cdesc.nickname;
	    break;
	case msn:
	    ret += "m" + cdesc.nickname;
	    break;
	case aim:
	    ret += "a" + cdesc.nickname;
	    break;
	case irc:
	    ret += "i" + cdesc.nickname;
	    break;
    }

    ret += "/";
    return ret;
}

void icqcontact::clear() {
    nmsgs = fupdated = groupid = fhistoffset = 0;
    finlist = true;
    cdesc = contactroot;

    binfo = basicinfo();
    minfo = moreinfo();
    winfo = workinfo();

    interests.clear();
    background.clear();

    nick = dispnick = about = "";
    lastseen = 0;
}

void icqcontact::save() {
    string tname;
    ofstream f;

    if(conf.enoughdiskspace()) {
	mkdir(getdirname().c_str(), S_IREAD | S_IWRITE | S_IEXEC);

	if(!access(getdirname().c_str(), W_OK)) {
	    tname = getdirname() + "lastread";
	    f.open(tname.c_str());
	    if(f.is_open()) {
		f << lastread << endl;
		f.close();
		f.clear();
	    }

	    tname = getdirname() + "info";
	    f.open(tname.c_str());
	    if(f.is_open()) {
		string options;
		if(binfo.requiresauth) options += "a";
		if(binfo.authawait) options += "w";

		f << nick << endl <<
		    tosane(binfo.fname) << endl <<
		    tosane(binfo.lname) << endl <<
		    tosane(binfo.email) << endl <<
		    options << endl <<
		    endl <<
		    tosane(binfo.city) << endl <<
		    tosane(binfo.state) << endl <<
		    tosane(binfo.phone) << endl <<
		    tosane(binfo.fax) << endl <<
		    tosane(binfo.street) << endl <<
		    tosane(binfo.cellular) << endl <<
		    tosane(binfo.zip) << endl <<
		    binfo.country << endl <<
		    tosane(winfo.city) << endl <<
		    tosane(winfo.state) << endl <<
		    tosane(winfo.phone) << endl <<
		    tosane(winfo.fax) << endl <<
		    tosane(winfo.street) << endl <<
		    tosane(winfo.zip) << endl <<
		    winfo.country << endl <<
		    tosane(winfo.company) << endl <<
		    tosane(winfo.dept) << endl <<
		    tosane(winfo.position) << endl <<
		    (int) minfo.timezone << endl <<
		    tosane(winfo.homepage) << endl <<
		    (int) minfo.age << endl <<
		    (int) minfo.gender << endl <<
		    tosane(minfo.homepage) << endl <<
		    minfo.lang1 << endl <<
		    minfo.lang2 << endl <<
		    minfo.lang3 << endl <<
		    minfo.birth_day << endl <<
		    minfo.birth_month << endl <<
		    minfo.birth_year << endl <<
		    (interests.size() > 0 ? *(interests.begin()+0) : "") << endl <<
		    (interests.size() > 1 ? *(interests.begin()+1) : "") << endl <<
		    (interests.size() > 2 ? *(interests.begin()+2) : "") << endl <<
		    (interests.size() > 3 ? *(interests.begin()+3) : "") << endl <<
		    (background.size() > 0 ? *(background.begin()+0) : "") << endl <<
		    (background.size() > 1 ? *(background.begin()+1) : "") << endl <<
		    (background.size() > 2 ? *(background.begin()+2) : "") << endl <<
		    endl <<
		    endl <<
		    lastip << endl <<
		    tosane(dispnick) << endl <<
		    lastseen << endl <<
		    endl <<
		    endl <<
		    endl <<
		    endl <<
		    groupid << endl;

		f.close();
		f.clear();
	    }

	    tname = getdirname() + "about";
	    f.open(tname.c_str());
	    if(f.is_open()) {
		f << about;
		f.close();
		f.clear();
	    }

	    if(!finlist) {
		tname = getdirname() + "excluded";
		f.open(tname.c_str());
		if(f.is_open()) f.close();
	    }
	}
    }
}

void icqcontact::load() {
    int i;
    FILE *f;
    char buf[512];
    struct stat st;
    string tname = getdirname(), fn;

    imcontact savedesc = cdesc;
    clear();
    cdesc = savedesc;

    if(f = fopen((fn = tname + "/info").c_str(), "r")) {
	for(i = 0; !feof(f); i++) {
	    freads(f, buf, 512);
	    switch(i) {
		case  0: nick = buf; break;
		case  1: binfo.fname = buf; break;
		case  2: binfo.lname = buf; break;
		case  3: binfo.email = buf; break;
		case  4:
		    binfo.requiresauth = (strstr(buf, "a") != 0);
		    binfo.authawait = (strstr(buf, "w") != 0);
		    break;
		case  5: break;
		case  6: binfo.city = buf; break;
		case  7: binfo.state = buf; break;
		case  8: binfo.phone = buf; break;
		case  9: binfo.fax = buf; break;
		case 10: binfo.street = buf; break;
		case 11: binfo.cellular = buf; break;
		case 12: binfo.zip = buf; break;
		case 13: binfo.country = strtoul(buf, 0, 0); break;
		case 14: winfo.city = buf; break;
		case 15: winfo.state = buf; break;
		case 16: winfo.phone = buf; break;
		case 17: winfo.fax = buf; break;
		case 18: winfo.street = buf; break;
		case 19: winfo.zip = buf; break;
		case 20: winfo.country = strtoul(buf, 0, 0); break;
		case 21: winfo.company = buf; break;
		case 22: winfo.dept = buf; break;
		case 23: winfo.position = buf; break;
		case 24: minfo.timezone = atoi(buf); break;
		case 25: winfo.homepage = buf; break;
		case 26: minfo.age = atoi(buf); break;
		case 27: minfo.gender = (imgender) atoi(buf); break;
		case 28: minfo.homepage = buf; break;
		case 29: minfo.lang1 = strtoul(buf, 0, 0); break;
		case 30: minfo.lang2 = strtoul(buf, 0, 0); break;
		case 31: minfo.lang3 = strtoul(buf, 0, 0); break;
		case 32: minfo.birth_day = atoi(buf); break;
		case 33: minfo.birth_month = atoi(buf); break;
		case 34: minfo.birth_year = atoi(buf); break;
		case 35: if(strlen(buf)) interests.push_back(buf); break;
		case 36: if(strlen(buf)) interests.push_back(buf); break;
		case 37: if(strlen(buf)) interests.push_back(buf); break;
		case 38: if(strlen(buf)) interests.push_back(buf); break;
		case 39: if(strlen(buf)) background.push_back(buf); break;
		case 40: if(strlen(buf)) background.push_back(buf); break;
		case 41: if(strlen(buf)) background.push_back(buf); break;
		case 42: break;
		case 43: break;
		case 44: lastip = buf; break;
		case 45: dispnick = buf; break;
		case 46: lastseen = strtoul(buf, 0, 0); break;
		case 47: break;
		case 48: break;
		case 49: break;
		case 50: break;
		case 51: groupid = atoi(buf); break;
	    }
	}
	fclose(f);
    }

    if(f = fopen((fn = tname + "/about").c_str(), "r")) {
	while(!feof(f)) {
	    freads(f, buf, 512);
	    if(about.size()) about += '\n';
	    about += buf;
	}
	fclose(f);
    }

    if(f = fopen((fn = tname + "/lastread").c_str(), "r")) {
	freads(f, buf, 512);
	sscanf(buf, "%lu", &lastread);
	fclose(f);
    }

    finlist = stat((fn = tname + "/excluded").c_str(), &st);

    if(!isbirthday())
	unlink((tname + "/congratulated").c_str());

    if(nick.empty()) {
	if(cdesc.uin) nick = i2str(cdesc.uin);
	else nick = cdesc.nickname;
    }

    if(dispnick.empty()) dispnick = nick;

    if(conf.getgroupmode() != icqconf::nogroups)
    if(find(groups.begin(), groups.end(), groupid) == groups.end()) {
	groupid = 1;
    }
}

bool icqcontact::isbirthday() const {
    bool ret = false;
    time_t curtime = time(0);
    struct tm tbd, *tcur = localtime(&curtime);

    memset(&tbd, 0, sizeof(tbd));

    tbd.tm_year = tcur->tm_year;
    tbd.tm_mday = minfo.birth_day;
    tbd.tm_mon = minfo.birth_month-1;

    if(tbd.tm_mday == tcur->tm_mday)
    if(tbd.tm_mon == tcur->tm_mon) {
	ret = true;
    }

    return ret;
}

void icqcontact::remove() {
    string dname = getdirname(), fname;
    struct dirent *e;
    struct stat st;
    DIR *d;

    gethook(cdesc.pname).removeuser(cdesc);

    if(d = opendir(dname.c_str())) {
	while(e = readdir(d)) {
	    fname = dname + "/" + e->d_name;
	    if(!stat(fname.c_str(), &st) && !S_ISDIR(st.st_mode))
		unlink(fname.c_str());
	}
	closedir(d);
	rmdir(dname.c_str());
    }
}

void icqcontact::excludefromlist() {
    FILE *f;
    string fname = getdirname() + "excluded";
    if(f = fopen(fname.c_str(), "w")) fclose(f);
    finlist = false;
}

void icqcontact::includeintolist() {
    gethook(cdesc.pname).sendnewuser(cdesc);
    unlink((getdirname() + "excluded").c_str());
    finlist = true;
}

bool icqcontact::inlist() const {
    return finlist;
}

void icqcontact::scanhistory() {
    string fn = getdirname() + "history";
    char buf[512];
    int line, pos;
    FILE *f = fopen(fn.c_str(), "r");
    bool docount;

    setmsgcount(0);

    if(f) {
//      fseek(f, gethistoffset(), SEEK_SET);
	sethistoffset(0);
	pos = 0;

	while(!feof(f)) {
	    freads(f, buf, 512);

	    if((string) buf == "\f") {
		line = 0;
		pos = ftell(f);
	    } else {
		switch(line) {
		    case 1:
			docount = ((string) buf == "IN");
			break;
		    case 4:
			if(docount && (atol(buf) > lastread)) {
			    nmsgs++;
			    if(!gethistoffset()) sethistoffset(pos);
			}
			break;
		}
	    }

	    line++;
	}

	fclose(f);
	if(!gethistoffset()) sethistoffset(pos);
    }
}

void icqcontact::setstatus(imstatus fstatus) {
    if(status != fstatus) {
	if(status == offline) {
	    external.exec(imrawevent(imevent::online, cdesc, imevent::incoming));
	    playsound(imevent::online);
	}

	status = fstatus;
	face.relaxedupdate();
    }
}

void icqcontact::setnick(const string &fnick) {
    string dir;

    nick = fnick;

    if(!cdesc.nickname.empty()) {
	dir = getdirname();
	cdesc.nickname = fnick;
	rename(dir.c_str(), getdirname().c_str());
    }
}

void icqcontact::setdispnick(const string &fnick) {
    dispnick = fnick;
}

void icqcontact::setlastread(time_t flastread) {
    lastread = flastread;
    scanhistory();
}

void icqcontact::unsetupdated() {
    fupdated = 0;
}

void icqcontact::setmsgcount(int n) {
    nmsgs = n;
    face.relaxedupdate();
}

void icqcontact::setbasicinfo(const basicinfo &ainfo) {
    binfo = ainfo;
    fupdated++;
}

void icqcontact::setmoreinfo(const moreinfo &ainfo) {
    minfo = ainfo;
    fupdated++;
}

void icqcontact::setworkinfo(const workinfo &ainfo) {
    winfo = ainfo;
    fupdated++;
}

void icqcontact::setinterests(const vector<string> &ainterests) {
    interests = ainterests;
    fupdated++;
}

void icqcontact::setbackground(const vector<string> &abackground) {
    background = abackground;
    fupdated++;
}

void icqcontact::setabout(const string &data) {
    about = data;
    fupdated++;
}

const icqcontact::basicinfo &icqcontact::getbasicinfo() const {
    return binfo;
}

const icqcontact::moreinfo &icqcontact::getmoreinfo() const {
    return minfo;
}

const icqcontact::workinfo &icqcontact::getworkinfo() const {
    return winfo;
}

const vector<string> &icqcontact::getinterests() const {
    return interests;
}

const vector<string> &icqcontact::getbackground() const {
    return background;
}

void icqcontact::setsound(imevent::imeventtype event, const string &sf) {
    sound[event] = sf;
}

void icqcontact::playsound(imevent::imeventtype event) const {
    string sf = sound[event], cline;
    int i;

    if(sf.size()) {
	if(sf[0] == '!') {
	    static time_t lastmelody = 0;

	    if(time(0)-lastmelody < 5) return;
	    time(&lastmelody);

	    if(sf.substr(1) == "spk1") {
		for(i = 0; i < 3; i++) {
		    if(i) delay(90);
		    setbeep((i+1)*100, 60);
		    printf("\a");
		    fflush(stdout);
		}
	    } else if(sf.substr(1) == "spk2") {
		for(i = 0; i < 2; i++) {
		    if(i) delay(90);
		    setbeep((i+1)*300, 60);
		    printf("\a");
		    fflush(stdout);
		}
	    } else if(sf.substr(1) == "spk3") {
		for(i = 3; i > 0; i--) {
		    setbeep((i+1)*200, 60-i*10);
		    printf("\a");
		    fflush(stdout);
		    delay(90-i*10);
		}
	    } else if(sf.substr(1) == "spk4") {
		for(i = 0; i < 4; i++) {
		    setbeep((i+1)*400, 60);
		    printf("\a");
		    fflush(stdout);
		    delay(90);
		}
	    } else if(sf.substr(1) == "spk5") {
		for(i = 0; i < 4; i++) {
		    setbeep((i+1)*250, 60+i);
		    printf("\a");
		    fflush(stdout);
		    delay(90-i*5);
		}
	    }
	} else {
	    static int pid = 0;

	    if(pid) kill(pid, SIGKILL);
	    pid = fork();
	    if(!pid) {
		string cline = sf + " >/dev/null 2>&1";
		execlp("/bin/sh", "/bin/sh", "-c", cline.c_str(), 0);
		exit(0);
	    }
	}
    } else if(cdesc != contactroot) {
	icqcontact *c = clist.get(contactroot);
	c->playsound(event);
    }
}

void icqcontact::setlastip(const string &flastip) {
    lastip = flastip;
    fupdated++;
}

string icqcontact::getabout() const {
    return about;
}

string icqcontact::getlastip() const {
    return lastip;
}

time_t icqcontact::getlastread() const {
    return lastread;
}

imstatus icqcontact::getstatus() const {
    return status;
}

int icqcontact::getmsgcount() const {
    return nmsgs;
}

string icqcontact::getnick() const {
    return nick;
}

string icqcontact::getdispnick() const {
    return dispnick;
}

int icqcontact::updated() const {
    return fupdated;
}

void icqcontact::setlastseen() {
    time(&lastseen);
    fupdated++;
}

time_t icqcontact::getlastseen() const {
    return lastseen;
}

char icqcontact::getshortstatus() const {
    return imstatus2char[status];
}

bool icqcontact::operator > (const icqcontact &acontact) const {
    if(acontact.lastread != lastread) {
	return acontact.lastread > lastread;
    } else if(acontact.cdesc.uin != cdesc.uin) {
	return acontact.cdesc.uin > cdesc.uin;
    } else {
	return acontact.cdesc.nickname.compare(cdesc.nickname);
    }
}

void icqcontact::setpostponed(const string &apostponed) {
    postponed = apostponed;
}

string icqcontact::getpostponed() const {
    return postponed;
}

void icqcontact::setgroupid(int agroupid) {
    groupid = agroupid;
    save();
}

int icqcontact::getgroupid() const {
    return groupid;
}

const imcontact icqcontact::getdesc() const {
    return cdesc;
}

int icqcontact::gethistoffset() const {
    return fhistoffset;
}

void icqcontact::sethistoffset(int aoffset) {
    fhistoffset = aoffset;
}

void icqcontact::remindbirthday(bool r) {
    string tname = getdirname() + "congratulated";
    ofstream f;

    if(!congratulated && r) {
	congratulated = !access(tname.c_str(), F_OK);

	if(!congratulated) {
	    f.open(tname.c_str());
	    if(f.is_open()) f.close();

	    em.store(imnotification(getdesc(), _("The user has a birthday today")));
	    congratulated = true;
	}

    } else if(congratulated && !r) {
	congratulated = false;
	unlink(tname.c_str());

    }
}

// ----------------------------------------------------------------------------

string icqcontact::moreinfo::strtimezone() const {
    string r;

    if(timezone <= 24 && timezone >= -24) {
	r = ICQ2000::UserInfoHelpers::getTimezoneIDtoString(timezone) + ", " +
	    ICQ2000::UserInfoHelpers::getTimezonetoLocaltime(timezone);
    }

    return r;
}

string icqcontact::moreinfo::strbirthdate() const {
    string r;

    static const string smonths[12] = {
	_("Jan"), _("Feb"), _("Mar"), _("Apr"),
	_("May"), _("Jun"), _("Jul"), _("Aug"),
	_("Sep"), _("Oct"), _("Nov"), _("Dec")
    };

    if((birth_day > 0) && (birth_day <= 31))
    if((birth_month > 0) && (birth_month <= 12)) {
	r = i2str(birth_day) + "-" + smonths[birth_month-1] + "-" + i2str(birth_year);
    }

    return r;
}
