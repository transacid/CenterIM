/*
*
* centericq single icq contact class
* $Id: icqcontact.cc,v 1.20 2001/11/21 14:35:55 konst Exp $
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

#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqgroups.h"
#include "icqconf.h"
#include "centericq.h"
#include "icqhook.h"
#include "yahoohook.h"

icqcontact::icqcontact(const imcontact adesc) {
    string fname, tname;
    int i;

    clear();
    for(i = 0; i < SOUND_COUNT; i++) sound[i] = "";
    seq2 = nmsgs = lastread = 0;
    finlist = true;

    cdesc = adesc;

    switch(cdesc.pname) {
	case infocard:
	    if(!cdesc.uin) {
		fname = (string) getenv("HOME") + "/.centericq/n";

		for(i = 1; ; i++) {
		    tname = fname + i2str(i);
		    if(access(tname.c_str(), F_OK)) break;
		}

		cdesc.uin = i;
	    }
	    load();
	    break;

	case icq:
	case yahoo:
	    load();
	    scanhistory();
	    break;
    }
}

icqcontact::~icqcontact() {
}

const string icqcontact::tosane(const string p) const {
    string buf;
    string::iterator i;

    for(buf = p, i = buf.begin(); i != buf.end(); i++) {
	if(strchr("\n\r", *i)) *i = ' ';
    }

    return buf;
}

const string icqcontact::getdirname() const {
    string ret;

    ret = (string) getenv("HOME") + "/.centericq/";

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
    }

    return ret;
}

void icqcontact::clear() {
    int i;

    seq2 = nmsgs = fupdated = infotryn = groupid = 0;
    finlist = true;
    status = offline;
    direct = false;
    cdesc = contactroot;

    nick = dispnick = firstname = lastname = primemail = secemail = oldemail =
    city = state = phone = fax = street = cellular = wcity = wstate = wphone =
    wfax = waddress = company = department = job = whomepage = homepage =
    about = "";

    for(i = 0; i < 4; i++) fint[i] = faf[i] = fbg[i] = "";
    
    lang1 = lang2 = lang3 = bday = bmonth = byear = age = gender = 0;
    zip = wzip = occupation = 0;
    country = wcountry = 0;
    lastseen = 0;
    utimezone = 0;
}

void icqcontact::save() {
    FILE *f;
    string fn = getdirname(), tname;
    mkdir(fn.c_str(), S_IREAD | S_IWRITE | S_IEXEC);
    char buf[1024];

    getcwd(buf, 1024);

    if(!chdir(fn.c_str())) {
	if(f = fopen((tname = fn + "/lastread").c_str(), "w")) {
	    fprintf(f, "%lu\n", lastread);
	    fclose(f);
	}

	if(f = fopen((tname = fn + "/info").c_str(), "w")) {
	    fprintf(f, "%s\n", nick.c_str());
	    fprintf(f, "%s\n", firstname.c_str());
	    fprintf(f, "%s\n", lastname.c_str());
	    fprintf(f, "%s\n", primemail.c_str());
	    fprintf(f, "%s\n", secemail.c_str());
	    fprintf(f, "%s\n", oldemail.c_str());
	    fprintf(f, "%s\n", city.c_str());
	    fprintf(f, "%s\n", state.c_str());
	    fprintf(f, "%s\n", phone.c_str());
	    fprintf(f, "%s\n", fax.c_str());
	    fprintf(f, "%s\n", street.c_str());
	    fprintf(f, "%s\n", cellular.c_str());
	    fprintf(f, "%lu\n", zip);
	    fprintf(f, "%lu\n", country);
	    fprintf(f, "%s\n", wcity.c_str());
	    fprintf(f, "%s\n", wstate.c_str());
	    fprintf(f, "%s\n", wphone.c_str());
	    fprintf(f, "%s\n", wfax.c_str());
	    fprintf(f, "%s\n", waddress.c_str());
	    fprintf(f, "%lu\n", wzip);
	    fprintf(f, "%lu\n", wcountry);
	    fprintf(f, "%s\n", company.c_str());
	    fprintf(f, "%s\n", department.c_str());
	    fprintf(f, "%s\n", job.c_str());
	    fprintf(f, "%lu\n", occupation);
	    fprintf(f, "%s\n", whomepage.c_str());
	    fprintf(f, "%d\n", age);
	    fprintf(f, "%d\n", gender);
	    fprintf(f, "%s\n", homepage.c_str());
	    fprintf(f, "%d\n", lang1);
	    fprintf(f, "%d\n", lang2);
	    fprintf(f, "%d\n", lang3);
	    fprintf(f, "%d\n", bday);
	    fprintf(f, "%d\n", bmonth);
	    fprintf(f, "%d\n", byear);
	    fprintf(f, "%s\n", fint[0].c_str());
	    fprintf(f, "%s\n", fint[1].c_str());
	    fprintf(f, "%s\n", fint[2].c_str());
	    fprintf(f, "%s\n", fint[3].c_str());
	    fprintf(f, "%s\n", faf[0].c_str());
	    fprintf(f, "%s\n", faf[1].c_str());
	    fprintf(f, "%s\n", faf[2].c_str());
	    fprintf(f, "%s\n", faf[3].c_str());
	    fprintf(f, "%c%c%c\n", reqauth ? '1' : '0', webaware ? '1' : '0', pubip ? '1' : '0');
	    fprintf(f, "%s\n", lastip.c_str());
	    fprintf(f, "%s\n", dispnick.c_str());
	    fprintf(f, "%lu\n", lastseen);
	    fprintf(f, "%s\n", fbg[0].c_str());
	    fprintf(f, "%s\n", fbg[1].c_str());
	    fprintf(f, "%s\n", fbg[2].c_str());
	    fprintf(f, "%s\n", fbg[3].c_str());
	    fprintf(f, "%d\n", groupid);
	    fprintf(f, "%d\n", utimezone);
	    fclose(f);
	}

	if(f = fopen((tname = fn + "/about").c_str(), "w")) {
	    fprintf(f, "%s", about.c_str());
	    fclose(f);
	}

	if(!finlist)
	if(f = fopen((tname = fn + "/excluded").c_str(), "w")) {
	    fclose(f);
	}
    }

    chdir(buf);
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
		case  1: firstname = buf; break;
		case  2: lastname = buf; break;
		case  3: primemail = buf; break;
		case  4: secemail = buf; break;
		case  5: oldemail = buf; break;
		case  6: city = buf; break;
		case  7: state = buf; break;
		case  8: phone = buf; break;
		case  9: fax = buf; break;
		case 10: street = buf; break;
		case 11: cellular = buf; break;
		case 12: zip = strtoul(buf, 0, 0); break;
		case 13: country = strtoul(buf, 0, 0); break;
		case 14: wcity = buf; break;
		case 15: wstate = buf; break;
		case 16: wphone = buf; break;
		case 17: wfax = buf; break;
		case 18: waddress = buf; break;
		case 19: wzip = strtoul(buf, 0, 0); break;
		case 20: wcountry = strtoul(buf, 0, 0); break;
		case 21: company = buf; break;
		case 22: department = buf; break;
		case 23: job = buf; break;
		case 24: occupation = strtoul(buf, 0, 0); break;
		case 25: whomepage = buf; break;
		case 26: age = atoi(buf); break;
		case 27: gender = atoi(buf); break;
		case 28: homepage = buf; break;
		case 29: lang1 = atoi(buf); break;
		case 30: lang2 = atoi(buf); break;
		case 31: lang3 = atoi(buf); break;
		case 32: bday = atoi(buf); break;
		case 33: bmonth = atoi(buf); break;
		case 34: byear = atoi(buf); break;
		case 35: fint[0] = buf; break;
		case 36: fint[1] = buf; break;
		case 37: fint[2] = buf; break;
		case 38: fint[3] = buf; break;
		case 39: faf[0] = buf; break;
		case 40: faf[1] = buf; break;
		case 41: faf[2] = buf; break;
		case 42: faf[3] = buf; break;
		case 43:
		    reqauth = buf[0] == '1';
		    webaware = buf[1] == '1';
		    pubip = buf[2] == '1';
		    break;
		case 44: lastip = buf; break;
		case 45: dispnick = buf; break;
		case 46: lastseen = strtoul(buf, 0, 0); break;
		case 47: fbg[0] = buf; break;
		case 48: fbg[1] = buf; break;
		case 49: fbg[2] = buf; break;
		case 50: fbg[3] = buf; break;
		case 51: groupid = atoi(buf); break;
		case 52: utimezone = atoi(buf); break;
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

    if(nick.empty()) nick = i2str(cdesc.uin);
    if(dispnick.empty()) dispnick = nick;

    if(conf.getusegroups())
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
    tbd.tm_mday = bday;
    tbd.tm_mon = bmonth-1;

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

    switch(cdesc.pname) {
	case yahoo:
	    yhook.removeuser(cdesc);
	    break;
    }

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
    string fname = getdirname() + "/excluded";

    if(cdesc.pname != infocard) {
	if(f = fopen(fname.c_str(), "w")) fclose(f);
	finlist = false;
    }
}

void icqcontact::includeintolist() {
    string fname = getdirname() + "/excluded";
    unlink(fname.c_str());
    finlist = true;

    switch(cdesc.pname) {
	case yahoo:
	    yhook.sendnewuser(cdesc);
	    break;
	case icq:
	    icq_SendNewUser(&icql, cdesc.uin);
	    break;
    }
}

bool icqcontact::inlist() const {
    return finlist;
}

void icqcontact::scanhistory() {
    string fn = getdirname() + "/history";
    char buf[512];
    int line;
    FILE *f = fopen(fn.c_str(), "r");
    bool docount;

    nmsgs = 0;
    if(f) {
	while(!feof(f)) {
	    freads(f, buf, 512);

	    if((string) buf == "\f") line = 0; else
	    switch(line) {
		case 1: docount = ((string) buf == "IN"); break;
		case 4: if(docount && (atol(buf) > lastread)) nmsgs++; break;
	    }

	    line++;
	}
	fclose(f);
    }
}

void icqcontact::setstatus(imstatus fstatus) {
    status = fstatus;
}

void icqcontact::setnick(const string fnick) {
    nick = fnick;
}

void icqcontact::setdispnick(const string fnick) {
    dispnick = fnick;
}

void icqcontact::setlastread(time_t flastread) {
    lastread = flastread;
    scanhistory();
}

void icqcontact::setseq2(unsigned short fseq2) {
    seq2 = fseq2;
    unsetupdated();
}

void icqcontact::unsetupdated() {
    fupdated = 0;
}

void icqcontact::setmsgcount(int n) {
    nmsgs = n;
}

void icqcontact::getinfo(string &fname, string &lname, string &fprimemail,
string &fsecemail, string &foldemail, string &fcity,
string &fstate, string &fphone, string &ffax, string &fstreet,
string &fcellular, unsigned long &fzip, unsigned short &fcountry) const {
    fname = firstname;
    lname = lastname;
    fprimemail = primemail;
    fsecemail = secemail;
    foldemail = oldemail;
    fcity = city;
    fstate = state;
    fphone = phone;
    ffax = fax;
    fstreet = street;
    fcellular = cellular;
    fzip = zip;
    fcountry = country;
}

void icqcontact::setinfo(const string fname, const string lname,
const string fprimemail, const string fsecemail, const string foldemail,
const string fcity, const string fstate, const string fphone,
const string ffax, const string fstreet, const string fcellular,
unsigned long fzip, unsigned short fcountry) {
    firstname = tosane(fname);
    lastname = tosane(lname);
    primemail = tosane(fprimemail);
    secemail = tosane(fsecemail);
    oldemail = tosane(foldemail);
    city = tosane(fcity);
    state = tosane(fstate);
    phone = tosane(fphone);
    fax = tosane(ffax);
    street = tosane(fstreet);
    cellular = tosane(fcellular);
    zip = fzip;
    country = fcountry;
    fupdated++;
}

void icqcontact::getmoreinfo(unsigned char &fage, unsigned char &fgender,
string &fhomepage, unsigned char &flang1, unsigned char &flang2,
unsigned char &flang3, unsigned char &fbday, unsigned char &fbmonth,
unsigned char &fbyear) const {
    fage = age;
    fgender = gender;
    fhomepage = homepage;
    flang1 = lang1;
    flang2 = lang2;
    flang3 = lang3;
    fbday = bday;
    fbmonth = bmonth;
    fbyear = byear;
}

void icqcontact::setmoreinfo(unsigned char fage, unsigned char fgender,
const string fhomepage, unsigned char flang1, unsigned char flang2,
unsigned char flang3, unsigned char fbday, unsigned char fbmonth,
unsigned char fbyear) {
    age = fage;
    gender = fgender;
    homepage = tosane(fhomepage);
    lang1 = flang1;
    lang2 = flang2;
    lang3 = flang3;
    bday = fbday;
    bmonth = fbmonth;
    byear = fbyear;
    fupdated++;
}

void icqcontact::getworkinfo(string &fwcity, string &fwstate, string &fwphone,
string &fwfax, string &fwaddress, unsigned long &fwzip,
unsigned short &fwcountry, string &fcompany, string &fdepartment,
string &fjob, unsigned short &foccupation, string &fwhomepage) const {
    fwcity = wcity;
    fwstate = wstate;
    fwphone = wphone;
    fwfax = wfax;
    fwaddress = waddress;
    fwzip = wzip;
    fwcountry = wcountry;
    fcompany = company;
    fdepartment = department;
    fjob = job;
    foccupation = occupation;
    fwhomepage = whomepage;
}

void icqcontact::setworkinfo(string fwcity, const string fwstate,
const string fwphone, const string fwfax, const string fwaddress,
unsigned long fwzip, unsigned short fwcountry, const string fcompany,
const string fdepartment, const string fjob, unsigned short foccupation,
const string fwhomepage) {
    wcity = tosane(fwcity);
    wstate = tosane(fwstate);
    wphone = tosane(fwphone);
    wfax = tosane(fwfax);
    waddress = tosane(fwaddress);
    wzip = fwzip;
    wcountry = fwcountry;
    company = tosane(fcompany);
    department = tosane(fdepartment);
    job = tosane(fjob);
    occupation = foccupation;
    whomepage = tosane(fwhomepage);
    fupdated++;
}

void icqcontact::getinterests(string &int1, string &int2, string &int3, string &int4) const {
    int1 = fint[0];
    int2 = fint[1];
    int3 = fint[2];
    int4 = fint[3];
}

void icqcontact::setabout(const string data) {
    about = data;
    fupdated++;
}

void icqcontact::setsound(int event, const string sf) {
    if(event <= SOUND_COUNT) {
	sound[event] = sf;
    }
}

void icqcontact::playsound(int event) const {
    string sf = sound[event], cline;
    int i;

    if(event <= SOUND_COUNT) {
	if(sf.size()) {
	    if(sf[0] == '!') {
		time_t lastmelody = 0;

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
}

void icqcontact::setsecurity(bool freqauth, bool fwebaware, bool fpubip) {
    reqauth = freqauth;
    webaware = fwebaware;
    pubip = fpubip;
}

void icqcontact::getsecurity(bool &freqauth, bool &fwebaware, bool &fpubip) const {
    freqauth = reqauth;
    fwebaware = webaware;
    fpubip = pubip;
}

void icqcontact::setlastip(const string flastip) {
    lastip = flastip;
}

const string icqcontact::getabout() const {
    return about;
}

const string icqcontact::getlastip() const {
    return lastip;
}

time_t icqcontact::getlastread() const {
    return lastread;
}

imstatus icqcontact::getstatus() const {
    return status;
}

unsigned short icqcontact::getseq2() const {
    return seq2;
}

int icqcontact::getmsgcount() const {
    return nmsgs;
}

const string icqcontact::getnick() const {
    return nick;
}

const string icqcontact::getdispnick() const {
    return dispnick;
}

int icqcontact::updated() const {
    return fupdated;
}

void icqcontact::setdirect(bool flag) {
    direct = flag;
}

bool icqcontact::getdirect() const {
    return direct && (status != STATUS_OFFLINE);
}

void icqcontact::setlastseen() {
    time(&lastseen);
}

time_t icqcontact::getlastseen() const {
    return lastseen;
}

void icqcontact::setinterests(const string nint[]) {
    for(int i = 0; i < 4; i++) fint[i] = tosane(nint[i]);
    fupdated++;
}

void icqcontact::setaffiliations(const string naf[]) {
    for(int i = 0; i < 4; i++) faf[i] = tosane(naf[i]);
}

void icqcontact::setbackground(const string nbg[]) {
    for(int i = 0; i < 4; i++) fbg[i] = tosane(nbg[i]);
}

void icqcontact::getaffiliations(string &naf1, string &naf2, string &naf3,
string &naf4) const {
    naf1 = faf[0];
    naf2 = faf[1];
    naf3 = faf[2];
    naf4 = faf[3];
}

void icqcontact::getbackground(string &nbg1, string &nbg2, string &nbg3,
string &nbg4) const {
    nbg1 = fbg[0];
    nbg2 = fbg[1];
    nbg3 = fbg[2];
    nbg4 = fbg[3];
}

char icqcontact::getshortstatus() const {
    return status2char[status];
}

int icqcontact::getinfotryn() const {
    return infotryn;
}

void icqcontact::incinfotryn() {
    infotryn++;
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

void icqcontact::setpostponed(const string apostponed) {
    postponed = apostponed;
}

const string icqcontact::getpostponed() const {
    return postponed;
}

void icqcontact::setmsgdirect(bool flag) {
    msgdirect = flag;
}

bool icqcontact::getmsgdirect() const {
    bool specific = false;

    switch(cdesc.pname) {
	case icq:
	    specific = icq_TCPLinkOpen(&icql, cdesc.uin);
	    break;
	case yahoo:
	    return true;
    }

    return
	msgdirect && (!conf.getserveronly() || specific || islocal());
}

void icqcontact::setgroupid(int agroupid) {
    groupid = agroupid;
}

int icqcontact::getgroupid() const {
    return groupid;
}

const string icqcontact::gettimezone() const {
    string r;
    time_t t = time(0), rt;

    r = (utimezone > 0 ? "-" : "+");
    r += i2str(abs(utimezone/2)) + ":" + (utimezone%2 ? "30" : "00");

    rt = t+icq_GetSystemTimeZone()*1800;
    rt -= utimezone*1800;

    r += (string) ", " + ctime(&rt);

    return r;
}

bool icqcontact::islocal() const {
    vector<string> addrs;
    vector<string>::iterator i;
    string lc, a;

    /*
    *
    * I know, it's stupid. Just a fast solution.
    *
    */

    addrs.push_back("127.");
    addrs.push_back("192.");
    addrs.push_back("172.");
    addrs.push_back("154.");
    addrs.push_back("10.");

    lc = lastip;

    while(!lc.empty()) {
	a = getword(lc);
	for(i = addrs.begin(); i != addrs.end(); i++) {
	    if(a.substr(0, i->size()) == *i) return true;
	}
    }

    return false;
}

const imcontact icqcontact::getdesc() const {
    return cdesc;
}
