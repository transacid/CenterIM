/*
*
* centericq single IM contact class
* $Id: icqcontact.cc,v 1.105 2005/02/13 12:10:55 iulica Exp $
*
* Copyright (C) 2001-2004 by Konstantin Klyagin <konst@konst.org.ua>
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
#include "icqface.h"
#include "abstracthook.h"
#include "imexternal.h"
#include "eventmanager.h"

#include <time.h>
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif

icqcontact::icqcontact(const imcontact adesc) {
    string fname, tname;
    imevent::imeventtype ie;
    int i;

    clear();
    lastread = fhistoffset = 0;
    status = offline;
    finlist = true;
    congratulated = false;
    openedforchat = false;

    for(ie = imevent::message; ie != imevent::imeventtype_size; ie++)
	sound[ie] = "";

    cdesc = adesc;

    switch(cdesc.pname) {
	case infocard:
	case rss:
	    if(!cdesc.uin) {
		fname = conf.getdirname() + conf.getprotocolprefix(cdesc.pname);

		for(i = 1; ; i++) {
		    tname = fname + i2str(i);
		    if(access(tname.c_str(), F_OK)) break;
		}

		cdesc.uin = i;
	    }

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
	case icq:
	case infocard:
	case rss:
	case gadu:
	    ret += conf.getprotocolprefix(cdesc.pname) + i2str(cdesc.uin);
	    break;
	default:
	    ret += conf.getprotocolprefix(cdesc.pname) + cdesc.nickname;
	    break;
    }

    ret += "/";
    return ret;
}

void icqcontact::clear() {
    fupdated = groupid = fhistoffset = lasttyping = 0;
    finlist = true;
    modified = usepgpkey = false;
    cdesc = contactroot;

    binfo = basicinfo();
    minfo = moreinfo();
    winfo = workinfo();
    rinfo = reginfo();

    interests.clear();
    background.clear();

    nick = about = dispnick = postponed = lastip = "";
    lastseen = 0;
}

void icqcontact::save() {
    ofstream f;

    if(cdesc == contactroot)
	return;

    string lrname = getdirname() + "lastread";
    string infoname = getdirname() + "info";
    string aboutname = getdirname() + "about";
    string postponedname = getdirname() + "postponed";

    string dname = getdirname();
    dname.erase(dname.size()-1);

    modified = modified
	|| access(lrname.c_str(), F_OK)
	|| access(infoname.c_str(), F_OK)
	|| access(aboutname.c_str(), F_OK);

    if(modified && conf.enoughdiskspace()) {
	mkdir(dname.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);

	if(!access(getdirname().c_str(), W_OK)) {
	    f.open(lrname.c_str());
	    if(f.is_open()) {
		f << lastread << endl;
		f.close();
		f.clear();
	    }

	    f.open(infoname.c_str());
	    if(f.is_open()) {
		string options;
		if(binfo.requiresauth) options += "a";
		if(binfo.authawait) options += "w";
		if(usepgpkey) options += "p";

		f << nick << endl <<
		    tosane(binfo.fname) << endl <<
		    tosane(binfo.lname) << endl <<
		    tosane(binfo.email) << endl <<
		    options << endl <<
		    pgpkey << endl <<
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
		    minfo.checkfreq << endl <<
		    minfo.checklast << endl <<
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

	    f.open(aboutname.c_str());
	    if(f.is_open()) {
		f << about;
		f.close();
		f.clear();
	    }

	    if(!postponed.empty()) {
		f.open(postponedname.c_str());
		if(f.is_open()) {
		    f << postponed;
		    f.close();
		    f.clear();
		}
	    } else {
		unlink(postponedname.c_str());
	    }

	    if(!finlist) {
		f.open((getdirname() + "excluded").c_str());
		if(f.is_open()) f.close();
	    }
	}

	modified = false;
    }
}

void icqcontact::load() {
    int i;
    ifstream f;
    struct stat st;
    string tname = getdirname(), fn, buf;

    imcontact savedesc = cdesc;
    clear();
    cdesc = savedesc;

    f.clear();
    f.open((fn = tname + "/info").c_str());

    if(f.is_open()) {
	for(i = 0; getstring(f, buf); i++) {
	    switch(i) {
		case  0: nick = buf; break;
		case  1: binfo.fname = buf; break;
		case  2: binfo.lname = buf; break;
		case  3: binfo.email = buf; break;
		case  4:
		    binfo.requiresauth = (buf.find('a') != -1);
		    binfo.authawait = (buf.find('w') != -1);
		    usepgpkey = (buf.find('p') != -1);
		    break;
		case  5: pgpkey = buf; break;
		case  6: binfo.city = buf; break;
		case  7: binfo.state = buf; break;
		case  8: binfo.phone = buf; break;
		case  9: binfo.fax = buf; break;
		case 10: binfo.street = buf; break;
		case 11: binfo.cellular = buf; break;
		case 12: binfo.zip = buf; break;
		case 13: binfo.country = strtoul(buf.c_str(), 0, 0); break;
		case 14: winfo.city = buf; break;
		case 15: winfo.state = buf; break;
		case 16: winfo.phone = buf; break;
		case 17: winfo.fax = buf; break;
		case 18: winfo.street = buf; break;
		case 19: winfo.zip = buf; break;
		case 20: winfo.country = strtoul(buf.c_str(), 0, 0); break;
		case 21: winfo.company = buf; break;
		case 22: winfo.dept = buf; break;
		case 23: winfo.position = buf; break;
		case 24: minfo.timezone = atoi(buf.c_str()); break;
		case 25: winfo.homepage = buf; break;
		case 26: minfo.age = atoi(buf.c_str()); break;
		case 27: minfo.gender = (imgender) atoi(buf.c_str()); break;
		case 28: minfo.homepage = buf; break;
		case 29: minfo.lang1 = strtoul(buf.c_str(), 0, 0); break;
		case 30: minfo.lang2 = strtoul(buf.c_str(), 0, 0); break;
		case 31: minfo.lang3 = strtoul(buf.c_str(), 0, 0); break;
		case 32: minfo.birth_day = atoi(buf.c_str()); break;
		case 33: minfo.birth_month = atoi(buf.c_str()); break;
		case 34: minfo.birth_year = atoi(buf.c_str()); break;
		case 35: if(!buf.empty()) interests.push_back(buf); break;
		case 36: if(!buf.empty()) interests.push_back(buf); break;
		case 37: if(!buf.empty()) interests.push_back(buf); break;
		case 38: if(!buf.empty()) interests.push_back(buf); break;
		case 39: if(!buf.empty()) background.push_back(buf); break;
		case 40: if(!buf.empty()) background.push_back(buf); break;
		case 41: if(!buf.empty()) background.push_back(buf); break;
		case 42: minfo.checkfreq = atoi(buf.c_str()); break;
		case 43: minfo.checklast = atoi(buf.c_str()); break;
		case 44: lastip = buf; break;
		case 45: dispnick = buf; break;
		case 46: lastseen = strtoul(buf.c_str(), 0, 0); break;
		case 47: break;
		case 48: break;
		case 49: break;
		case 50: break;
		case 51: groupid = atoi(buf.c_str()); break;
	    }
	}
	f.close();

    } else {
	if(cdesc.uin)
	    nick = i2str(cdesc.uin);

    }

    f.clear();
    f.open((fn = tname + "/about").c_str());

    if(f.is_open()) {
	while(getstring(f, buf)) {
	    if(about.size()) about += '\n';
	    about += buf;
	}
	f.close();
    }

    f.clear();
    f.open((fn = tname + "/postponed").c_str());

    if(f.is_open()) {
	while(getstring(f, buf)) {
	    if(postponed.size()) postponed += '\n';
	    postponed += buf;
	}
	f.close();
    }

    f.clear();
    f.open((fn = tname + "/lastread").c_str());

    if(f.is_open()) {
	getstring(f, buf);
	lastread = strtoul(buf.c_str(), 0, 0);
	f.close();
    }

    finlist = stat((fn = tname + "/excluded").c_str(), &st);

    if(!isbirthday())
	unlink((tname + "/congratulated").c_str());

    if(nick.empty())
	nick = cdesc.nickname;

    if(dispnick.empty())
	dispnick = nick;

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

void icqcontact::includeintolist(int agroupid, bool reqauth) {
    binfo.requiresauth = binfo.authawait = reqauth;
    if(groupid) groupid = agroupid;

    includeintolist();
}

void icqcontact::includeintolist() {
    if(cdesc.pname != icq)
	status = offline;

    if(!ischannel(cdesc))
	gethook(cdesc.pname).sendnewuser(cdesc);

    unlink((getdirname() + "excluded").c_str());
    finlist = true;
}

bool icqcontact::inlist() const {
    return finlist;
}

void icqcontact::scanhistory() {
    string fn = getdirname() + "history", block;
    char buf[65];
    int pos, backstep, r;
    FILE *f = fopen(fn.c_str(), "r");
    struct stat st;

    sethasevents(false);
    stat(fn.c_str(), &st);

    if(f) {
	fseek(f, 0, SEEK_END);
	sethistoffset(0);
	pos = 0;

	while(ftell(f)) {
	    backstep = 64;
	    if(ftell(f) < backstep) backstep = ftell(f);

	    if(fseek(f, -backstep, SEEK_CUR)) break;
	    if((r = fread(buf, 1, backstep, f)) <= 0) break;
	    if(fseek(f, -backstep, SEEK_CUR)) break;

	    buf[r] = 0;
	    block.insert(0, buf);

	    if((r = block.find("\f\nIN\n")) != -1) {
		pos = st.st_size-block.size()+r;

		block.erase(0, r+2);

	    #ifdef HAVE_SSTREAM
		ostringstream oevdata;
		oevdata << block;
		istringstream evdata(oevdata.str());
	    #else
		strstream evdata;
		evdata << block;
	    #endif

		for(r = 0; (r < 3) && getline(evdata, block); r++);
		if(r == 3) sethasevents(strtoul(block.c_str(), 0, 0) > lastread);
		break;
	    }
	}

	fclose(f);
	if(!gethistoffset()) sethistoffset(pos);
    }
}

void icqcontact::setstatus(imstatus fstatus, bool reflect) {
    if(status != fstatus) {
	if(!ischannel(cdesc)) {
	    if(fstatus != offline && status == offline
	    || fstatus == offline && status != offline) {
		imevent::imeventtype et =
		    status == offline ? imevent::online : imevent::offline;

		external.exec(imrawevent(et, cdesc, imevent::incoming));
		playsound(et);
	    }

	} else if(reflect) {
	    abstracthook &h = gethook(cdesc.pname);
	    if(fstatus == offline) h.removeuser(cdesc);
	    else h.sendnewuser(cdesc);

	}

	setlastseen();
	status = fstatus;
	face.relaxedupdate();
    }
}

void icqcontact::setdesc(const imcontact &ic) {
    string dir = getdirname();
    cdesc = ic;
    rename(dir.c_str(), getdirname().c_str());
}

void icqcontact::setnick(const string &fnick) {
    nick = fnick;
    modified = true;
    fupdated++;
}

void icqcontact::setdispnick(const string &fnick) {
    dispnick = fnick;
    modified = true;
}

void icqcontact::setlastread(time_t flastread) {
    lastread = flastread;
    scanhistory();
    modified = true;
}

void icqcontact::unsetupdated() {
    fupdated = 0;
}

void icqcontact::setbasicinfo(const basicinfo &ainfo) {
    binfo = ainfo;
    fupdated++;
    modified = true;
}

void icqcontact::setmoreinfo(const moreinfo &ainfo) {
    minfo = ainfo;
    fupdated++;
    modified = true;
}

void icqcontact::setworkinfo(const workinfo &ainfo) {
    winfo = ainfo;
    fupdated++;
    modified = true;
}

void icqcontact::setreginfo(const reginfo &arinfo) {
    rinfo = arinfo;
}

void icqcontact::setinterests(const vector<string> &ainterests) {
    interests = ainterests;
    fupdated++;
    modified = true;
}

void icqcontact::setbackground(const vector<string> &abackground) {
    background = abackground;
    fupdated++;
    modified = true;
}

void icqcontact::setabout(const string &data) {
    about = data;
    fupdated++;
    modified = true;
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
		    if(i) usleep(90000);
		    setbeep((i+1)*100, 60);
		    printf("\a");
		    fflush(stdout);
		}
	    } else if(sf.substr(1) == "spk2") {
		for(i = 0; i < 2; i++) {
		    if(i) usleep(90000);
		    setbeep((i+1)*300, 60);
		    printf("\a");
		    fflush(stdout);
		}
	    } else if(sf.substr(1) == "spk3") {
		for(i = 3; i > 0; i--) {
		    setbeep((i+1)*200, 60-i*10);
		    printf("\a");
		    fflush(stdout);
		    usleep(90000-i*10000);
		}
	    } else if(sf.substr(1) == "spk4") {
		for(i = 0; i < 4; i++) {
		    setbeep((i+1)*400, 60);
		    printf("\a");
		    fflush(stdout);
		    usleep(90000);
		}
	    } else if(sf.substr(1) == "spk5") {
		for(i = 0; i < 4; i++) {
		    setbeep((i+1)*250, 60+i);
		    printf("\a");
		    fflush(stdout);
		    usleep(90000-i*5000);
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
    modified = true;
}

string icqcontact::getpgpkey() const {
    return pgpkey;
}

void icqcontact::setpgpkey(const string &key) {
    pgpkey = key;
    usepgpkey = modified = true;
    fupdated++;
}

bool icqcontact::getusepgpkey() const {
    return usepgpkey;
}

void icqcontact::setusepgpkey(bool usekey) {
    usepgpkey = usekey;
    modified = true;
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
    if(conf.getnonimonline(cdesc.pname)) return available;
    return status;
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
    modified = true;
}

time_t icqcontact::getlastseen() const {
    return lastseen;
}

char icqcontact::getshortstatus() const {
    if(status >= offline && status < imstatus_size) {
	return imstatus2char[getstatus()];
    } else {
	return imstatus2char[offline];
    }
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
    if(apostponed.find_first_not_of(" \r\n") != -1) postponed = apostponed;
	else postponed = "";

    modified = true;
}

string icqcontact::getpostponed() const {
    return postponed;
}

void icqcontact::setgroupid(int agroupid, bool reflect) {
    groupid = agroupid;
    modified = true;

    if(reflect)
	gethook(cdesc.pname).updatecontact(this);
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

    if(!congratulated && r) {
	congratulated = !access(tname.c_str(), F_OK);

	if(!congratulated) {
	    ofstream f(tname.c_str());
	    if(f.is_open()) f.close(), f.clear();

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
	r = abstracthook::getTimezoneIDtoString(timezone) + ", " +
	    abstracthook::getTimezonetoLocaltime(timezone);
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
