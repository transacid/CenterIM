/*
*
* centericq icq protocol handling class
* $Id: icqhook.cc,v 1.4 2001/11/23 15:10:11 konst Exp $
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

#include "icqhook.h"
#include "icqface.h"
#include "icqhist.h"
#include "icqconf.h"
#include "icqoffline.h"
#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqmlist.h"
#include "centericq.h"
#include "accountmanager.h"

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TIMESTAMP maketm(hour-1, minute, day, month, year)

icqhook::icqhook() {
    time_t c = time(0);

    timer_keepalive = timer_tcp = timer_resolve = timer_offline = timer_ack = c;
    timer_reconnect = 0;

    newuin = 0;
    finddest = 0;

    seq_keepalive = n_keepalive = 0;
    flogged = connecting = factive = false;

    manualstatus = conf.getstatus(icq);
}

icqhook::~icqhook() {
    conf.savestatus(icq, manualstatus);
}

void icqhook::init(const icqconf::imaccount account) {
    string spuser, sppass;

    icq_Init(&icql, account.uin, account.password.c_str(), "");

    if(!conf.getsockshost().empty()) {
	conf.getsocksuser(spuser, sppass);
	icq_SetProxy(&icql, conf.getsockshost().c_str(), conf.getsocksport(),
	    spuser.empty() ? 0 : 1, spuser.c_str(), sppass.c_str());
    }

#ifdef DEBUG
    icq_LogLevel = ICQ_LOG_MESSAGE;
    icql.icq_Log = &log;
#else
    icq_LogLevel = 0;
    icql.icq_Log = 0;
#endif

    icql.icq_Logged = &loggedin;
    icql.icq_Disconnected = &ildisconnected;
    icql.icq_RecvMessage = &message;
    icql.icq_RecvURL = &url;
    icql.icq_RecvWebPager = &webpager;
    icql.icq_RecvMailExpress = &mailexpress;
    icql.icq_RecvChatReq = &chat;
    icql.icq_RecvFileReq = &file;
    icql.icq_RecvAdded = &added;
    icql.icq_RecvAuthReq = &auth;
    icql.icq_UserOnline = &useronline;
    icql.icq_UserOffline = &useroffline;
    icql.icq_UserStatusUpdate = &userstatus;
    icql.icq_MetaUserInfo = &metauserinfo;
    icql.icq_MetaUserAbout = &metauserabout;
    icql.icq_MetaUserMore = &metausermore;
    icql.icq_MetaUserWork = &metauserwork;
    icql.icq_MetaUserInterests = &metauserinterests;
    icql.icq_MetaUserAffiliations = &metauseraffiliations;
    icql.icq_WrongPassword = &wrongpass;
    icql.icq_InvalidUIN = &invaliduin;

    icql.icq_UserFound = &userfound;
    icql.icq_WhitePagesFound = &wpfound;
    icql.icq_SearchDone = icql.icq_WhitePagesDone = &searchdone;

    icql.icq_RequestNotify = &requestnotify;
    icql.icq_RecvContact = &contact;

    factive = true;
}

void icqhook::connect() {
    icqconf::imaccount acc = conf.getourid(icq);

    if(!enabled()) {
	face.log("+ initializing %s engine",
	    conf.getprotocolname(acc.pname).c_str());

	init(acc);
    }

    connecting = true;
    face.log(_("+ [icq] connecting to the server"));

    if(icq_Connect(&icql, acc.server.c_str(), acc.port) != -1) {
	sockfd = icq_GetSok(&icql);
	icq_Login(&icql, icql.icq_Status = STATUS_ONLINE);
    } else {
	face.log(_("+ [icq] unable to connect to the server"));
	icql.icq_Status = (unsigned int) STATUS_OFFLINE;
	connecting = false;
    }

    time(&timer_reconnect);
    icq_Russian = conf.getrussian() ? 1 : 0;
}

void icqhook::loggedin(struct icq_link *link) {
    ihook.flogged = true;
    ihook.connecting = false;
    ihook.timer_resolve = time(0)-PERIOD_RESOLVE+5;

    time(&ihook.logontime);
    time(&ihook.timer_keepalive);

    ihook.seq_keepalive = 0;
    ihook.n_keepalive = 0;

    offl.scan(0, ossendall);
    face.update();
    face.log(_("+ [icq] logged in"));

    string fname = conf.getdirname() + "/icq-infoset",
	rnick, remail, rfname, rlname;

    if(!access(fname.c_str(), R_OK)) {
	face.log(_("+ [icq] setting initial user info"));

	ifstream f(fname.c_str());

	if(f.is_open()) {
	    getstring(f, rnick);
	    getstring(f, remail);
	    getstring(f, rfname);
	    getstring(f, rlname);

	    icq_UpdateNewUserInfo(&ihook.icql, rnick.c_str(), rfname.c_str(),
		rlname.c_str(), remail.c_str());

	    f.close();
	    unlink(fname.c_str());
	}
    }

    ihook.sendcontactlist();
    ihook.files.clear();
}

void icqhook::ildisconnected(struct icq_link *link, int reason) {
    string msg;
    ihook.disconnected(link, reason);

    msg = _("+ [icq] disconnected");

    switch(reason) {
	case ICQ_DISCONNECT_FORCED:
	    msg += ", ";
	    msg += _("server forced");
	    break;
	case ICQ_DISCONNECT_BUSY:
	    msg += ", ";
	    msg += _("server is busy");
	    break;
    }

    face.log(msg);
}

void icqhook::disconnected(struct icq_link *link, int reason) {
    int i;
    icqcontact *c;

//    link->icq_UDPSok = -1;
    link->icq_Status = (long unsigned int) STATUS_OFFLINE;

    ihook.flogged = ihook.connecting = false;

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);
	if(c->getdesc().pname == icq) {
	    c->setstatus(offline);
	    c->setseq2(0);
	}
    }

    time(&ihook.timer_reconnect);
    face.update();
}

void icqhook::message(struct icq_link *link, unsigned long uin,
unsigned char hour, unsigned char minute, unsigned char day,
unsigned char month, unsigned short year, const char *msg) {
    imcontact cinfo(uin, icq);

    if(strlen(msg))
    if(!lst.inlist(cinfo, csignore)) {
	hist.putmessage(cinfo, msg, HIST_MSG_IN, TIMESTAMP);
	icqcontact *c = clist.get(cinfo);

	if(c) {
	    c->setmsgcount(c->getmsgcount()+1);
	    c->playsound(EVT_MSG);
	    face.update();
	}
    }
}

void icqhook::contact(struct icq_link *link, unsigned long uin,
unsigned char hour, unsigned char minute, unsigned char day,
unsigned char month, unsigned short year, icqcontactmsg *cont) {
    imcontact cinfo(uin, icq);

    if(!lst.inlist(cinfo, csignore)) {
	hist.putcontact(cinfo, cont, HIST_MSG_IN, TIMESTAMP);
	icqcontact *c = clist.get(cinfo);

	if(c) {
	    c->setmsgcount(c->getmsgcount()+1);
	    c->playsound(EVT_CONTACT);
	    face.update();
	}
    }
}

void icqhook::url(struct icq_link *link, unsigned long uin, unsigned char hour,
unsigned char minute, unsigned char day, unsigned char month,
unsigned short year, const char *url, const char *descr) {
    imcontact cinfo(uin, icq);

    if(!lst.inlist(cinfo, csignore)) {
	hist.puturl(cinfo, url, descr, HIST_MSG_IN, TIMESTAMP);
	icqcontact *c = clist.get(cinfo);

	if(c) {
	    c->setmsgcount(c->getmsgcount()+1);
	    c->playsound(EVT_URL);
	    face.update();
	}
    }
}

void icqhook::webpager(struct icq_link *link, unsigned char hour,
unsigned char minute, unsigned char day, unsigned char month,
unsigned short year, const char *nick, const char *email,
const char *msg) {
    hist.putmail(nick, email, msg, ICQMAIL_WEBPAGER, TIMESTAMP);
}

void icqhook::mailexpress(struct icq_link *link, unsigned char hour,
unsigned char minute, unsigned char day, unsigned char month,
unsigned short year, const char *nick, const char *email,
const char *msg) {
    hist.putmail(nick, email, msg, ICQMAIL_EXPRESS, TIMESTAMP);
}

void icqhook::chat(struct icq_link *link, unsigned long uin,
unsigned char hour, unsigned char minute, unsigned char day,
unsigned char month, unsigned short year, const char *descr,
unsigned long seq /*, const char *session, unsigned long port*/) {
    imcontact cinfo(uin, icq);

    if(!lst.inlist(cinfo, csignore)) {
	icqcontact *c = clist.get(cinfo);

	if(c) {
	    c->playsound(EVT_CHAT);
	}
    }
}

void icqhook::file(struct icq_link *link, unsigned long uin,
unsigned char hour, unsigned char minute, unsigned char day,
unsigned char month, unsigned short year, const char *descr,
const char *filename, unsigned long filesize, unsigned long seq) {
    imcontact cinfo(uin, icq);

    if(!lst.inlist(cinfo, csignore)) {
	hist.putfile(cinfo, seq, filename, filesize, HIST_MSG_IN, TIMESTAMP);
	icqcontact *c = clist.get(cinfo);

	if(c) {
	    c->setmsgcount(c->getmsgcount()+1);
	    c->playsound(EVT_FILE);
	    face.update();
	}
    }
}

void icqhook::added(struct icq_link *link, unsigned long uin,
unsigned char hour, unsigned char minute, unsigned char day,
unsigned char month, unsigned short year, const char *nick,
const char *first, const char *last, const char *email) {
    imcontact cinfo(uin, icq);

    if(!lst.inlist(cinfo, csignore)) {
	string text = (string) "I" + i2str(uin) + " " + first + " " + last + " ";
	if(nick[0]) text += (string) "aka " + nick + " ";
	if(email[0]) text += (string) "<" + email + "> ";
	text += (string) _("has added you to his/her contact list");
	hist.putmessage(contactroot, text, HIST_MSG_IN, TIMESTAMP);
	clist.get(contactroot)->setmsgcount(clist.get(contactroot)->getmsgcount()+1);
	face.update();
    }
}

void icqhook::auth(struct icq_link *link, unsigned long uin,
unsigned char hour, unsigned char minute, unsigned char day,
unsigned char month, unsigned short year, const char *nick,
const char *first, const char *last, const char *email,
const char *reason) {
    imcontact cinfo(uin, icq);

    if(!lst.inlist(cinfo, csignore)) {
	string text = (string) "R" + i2str(uin) + " " + first + " " + last + " ";
	if(nick[0]) text += (string) "aka " + nick + " ";
	if(email[0]) text += (string) "<" + email + "> ";
	text += (string) _("has requested your authorization to add you to his/her contact list. The reason was: ") + reason;
	hist.putmessage(contactroot, text, HIST_MSG_IN, TIMESTAMP);
	clist.get(contactroot)->setmsgcount(clist.get(contactroot)->getmsgcount()+1);
	face.update();
    }
}

void icqhook::useronline(struct icq_link *link, unsigned long uin,
unsigned long status, unsigned long ip, unsigned short port,
unsigned long real_ip, unsigned char tcp_flag) {
    imcontact cinfo(uin, icq);

    icqcontact *c = clist.get(cinfo);
    time_t curtime = time(0);

    string lastip, sip, srip, sip_rev;
    unsigned char *bip = (unsigned char *) &ip;
    unsigned char *brip = (unsigned char *) &real_ip;
    string::reverse_iterator ch;

    sip = i2str(bip[3]) + "." + i2str(bip[2]) + "." + i2str(bip[1]) + "." + i2str(bip[0]);
    srip = i2str(brip[3]) + "." + i2str(brip[2]) + "." + i2str(brip[1]) + "." + i2str(brip[0]);

    for(ch = sip.rbegin(); ch != sip.rend(); ch++)
	sip_rev += *ch;

    if(sip != "0.0.0.0") lastip = sip;
    if(srip != "0.0.0.0" && srip != sip && srip != sip_rev) {
	if(!lastip.empty()) lastip += " ";
	lastip += srip;
    }

    if(c) {
	c->setstatus(ihook.icq2imstatus(status));
	c->setlastip(lastip);
	c->setdirect(tcp_flag == 0x04);
	c->setmsgdirect(tcp_flag == 0x04);
	c->setlastseen();

	if((curtime-ihook.logontime > 15)) {
	    c->playsound(EVT_ONLINE);
	}

	face.update();
    }
}

void icqhook::useroffline(struct icq_link *link, unsigned long uin) {
    icqcontact *c;
    imcontact cinfo(uin, icq);

    if(c = clist.get(cinfo)) {
	c->setstatus(offline);
	face.update();
    }
}

void icqhook::userstatus(struct icq_link *link, unsigned long uin,
unsigned long status) {
    icqcontact *c;
    imcontact cinfo(uin, icq);

    if(c = clist.get(cinfo)) {
	c->setstatus(ihook.icq2imstatus(status));
	face.update();
    }
}

void icqhook::metauserinfo(struct icq_link *link, unsigned short seq2,
const char *nick, const char *first, const char *last, const char *pri_eml,
const char *sec_eml, const char *old_eml, const char *city, const char *state,
const char *phone, const char *fax, const char *street, const char *cellular,
unsigned long zip, unsigned short country, unsigned char timezone,
unsigned char auth) {
    icqcontact *c = clist.getseq2(seq2);

    if(c) {
	c->setnick(nick);

	if(c->getdispnick() == i2str(c->getdesc().uin)) {
	    c->setdispnick(nick);
	    face.update();
	}

	c->setinfo(first, last, pri_eml, sec_eml, old_eml, city, state, phone, fax, street, cellular, zip, country);
	c->setsecurity(auth != 0, false, false);
	c->settimezone(timezone);
    }
}

void icqhook::metauserabout(struct icq_link *link, unsigned short seq2, const char *about) {
    icqcontact *c = clist.getseq2(seq2);
    if(c) c->setabout(about); 
}

void icqhook::metauserwork(struct icq_link *link, unsigned short seq2,
const char *fwcity, const char *fwstate, const char *fwphone, const char *fwfax,
const char *fwaddress, unsigned long fwzip, unsigned short fwcountry,
const char *fcompany, const char *fdepartment, const char *fjob,
unsigned short foccupation, const char *fwhomepage) {
    icqcontact *c = clist.getseq2(seq2);
    if(c) {
	c->setworkinfo(fwcity, fwstate, fwphone, fwfax, fwaddress, fwzip, fwcountry, fcompany, fdepartment, fjob, foccupation, fwhomepage);
    }
}

void icqhook::metausermore(struct icq_link *link, unsigned short seq2,
unsigned short fage, unsigned char fgender, const char *fhomepage,
unsigned char byear, unsigned char bmonth, unsigned char bday,
unsigned char flang1, unsigned char flang2, unsigned char flang3) {
    icqcontact *c = clist.getseq2(seq2);

    if(c) {
	c->setmoreinfo(fage, fgender, fhomepage, flang1, flang2, flang3, bday, bmonth, byear);
    }
}

void icqhook::metauserinterests(struct icq_link *link, unsigned short seq2,
unsigned char num, unsigned short icat1, const char *int1,
unsigned short icat2, const char *int2, unsigned short icat3,
const char *int3, unsigned short icat4, const char *int4) {
    icqcontact *c = clist.getseq2(seq2);
    string intr[4];

    if(c) {
	intr[0] = int1 ? i2str(icat1) + " " + int1 : "";
	intr[1] = int2 ? i2str(icat2) + " " + int2 : "";
	intr[2] = int3 ? i2str(icat3) + " " + int3 : "";
	intr[3] = int4 ? i2str(icat4) + " " + int4 : "";
	c->setinterests(intr);
    }
}

void icqhook::metauseraffiliations(struct icq_link *link, unsigned short seq2,
unsigned char anum, unsigned short acat1, const char *aff1,
unsigned short acat2, const char *aff2,
unsigned short acat3, const char *aff3,
unsigned short acat4, const char *aff4,
unsigned char bnum, unsigned short bcat1, const char *back1,
unsigned short bcat2, const char *back2,
unsigned short bcat3, const char *back3,
unsigned short bcat4, const char *back4) {
    int i;
    string af[4], bg[4];
    icqcontact *c = clist.getseq2(seq2);

    if(c) {
	if(aff1) af[0] = i2str(acat1) + " " + aff1;
	if(aff2) af[1] = i2str(acat2) + " " + aff2;
	if(aff3) af[2] = i2str(acat3) + " " + aff3;
	if(aff4) af[3] = i2str(acat4) + " " + aff4;

	if(back1) bg[0] = i2str(bcat1) + " " + back1;
	if(back2) bg[1] = i2str(bcat2) + " " + back2;
	if(back3) bg[2] = i2str(bcat3) + " " + back3;
	if(back4) bg[3] = i2str(bcat4) + " " + back4;

	c->setaffiliations(af);
	c->setbackground(bg);
    }
}

void icqhook::wrongpass(struct icq_link *link) {
    ihook.disconnected(link, 0);
    face.log(_("+ [icq] wrong password"));
}

void icqhook::invaliduin(struct icq_link *link) {
    ihook.disconnected(link, 0);
    face.log(_("+ [icq] invalid uin"));
}

bool icqhook::logged() const {
    return flogged;
}

void icqhook::userfound(struct icq_link *link, unsigned long uin,
const char *nick, const char *first, const char *last,
const char *email, char auth) {
    if(ihook.finddest) {
	string rname = (string) first + " " + last, remail, rnick = nick;
	if(email[0]) remail = (string) "<" + email + ">";
	rnick.resize(10);

	ihook.finddest->additemf(" %-*s %s %s", 10,
	rnick.c_str(), rname.c_str(), remail.c_str());

	ihook.finddest->redraw();
	ihook.founduins.push_back(uin);
    }
}

void icqhook::searchdone(struct icq_link *link) {
    face.log(_("+ [icq] search done"));
}

void icqhook::wpfound(struct icq_link *link, unsigned short seq2,
unsigned long uin, const char *nick, const char *first,
const char *last, const char *email, char auth, char status) {
    ihook.userfound(link, uin, nick, first, last, email, auth);
}

void icqhook::requestnotify(struct icq_link *link, unsigned long id,
int result, unsigned int length, void *data) {
    vector<icqfileassociation>::iterator i;

    for(i = ihook.files.begin(); i != ihook.files.end(); i++) {
	if(i->seq == id) {
	    icqcontact *c = (icqcontact *) clist.get(imcontact(i->uin, icq));

	    if(!c) return;

	    if((result == length) && !result) {
		face.log(_("+ [icq] file %s refused by %s, %lu"),
		    justfname(i->fname).c_str(),
		    c->getdispnick().c_str(),
		    c->getdesc().uin);
		return;
	    } else {
		if(result == ICQ_NOTIFY_FILE)
		if(length == FILE_STATUS_NEXT_FILE) {
		    if(i->seq == id) {
			if(i->dir == HIST_MSG_IN) {
			    face.log(_("+ [icq] file %s from %s, %lu received"),
				justfname(i->fname).c_str(),
				c->getdispnick().c_str(),
				i->uin);
			} else {
			    face.log(_("+ [icq] file %s to %s, %lu sent"),
				justfname(i->fname).c_str(),
				c->getdispnick().c_str(),
				i->uin);
			}

			ihook.files.erase(i);
			return;
		    }
		}
	    }
	}
    }

    switch(result) {
	case ICQ_NOTIFY_SUCCESS:
	    if(ihook.logged()) {
		if(ihook.seq_keepalive == id) {
		    ihook.seq_keepalive = 0;
		    ihook.n_keepalive = 0;
		} else {
		    offl.scan(id, osremove);
		}
	    }
	    break;
	case ICQ_NOTIFY_FAILED:
	    if(ihook.logged()) offl.scan(id, osresend);
	    break;
    }

    time(&ihook.timer_ack);
}

void icqhook::log(struct icq_link *link, time_t time,
unsigned char level, const char *str) {
    string text = str;
    text.resize(text.size()-1);
    face.log(text);
}

unsigned int icqhook::getfinduin(int pos) {
    int r;
    if(pos > 0 && pos <= founduins.size()) r = founduins[pos-1]; else r = 0;
    return r;
}

void icqhook::exectimers() {
    time_t timer_current = time(0);
    int period;

    if(logged()) {
	period = timer_current-timer_keepalive;

	if((!seq_keepalive &&
	(period > (conf.getsockshost().empty() ? PERIOD_KEEPALIVE : PERIOD_SOCKSALIVE))) ||
	(seq_keepalive && (period > PERIOD_WAIT_KEEPALIVE))) {
	    seq_keepalive = icq_KeepAlive(&icql);
	    n_keepalive++;
	    time(&timer_keepalive);
	}
	else
	if(timer_current-timer_tcp > PERIOD_TCP) {
	    icq_TCPMain(&icql);
	    time(&timer_tcp);
	}
	else
	if(timer_current-timer_resolve > PERIOD_RESOLVE) {
	    for(int i = 0; i < clist.count; i++) {
		icqcontact *c = (icqcontact *) clist.at(i);

		if(!c->getdesc().empty())
		if(c->getnick() == i2str(c->getdesc().uin))
		if(c->getinfotryn() < 5) {
		    c->setseq2(icq_SendMetaInfoReq(&icql, c->getdesc().uin));
		    c->incinfotryn();
		}
	    }

	    offl.scan(0, osexpired);
	    time(&timer_resolve);
	}

	if((timer_current-timer_ack > PERIOD_DISCONNECT) || (n_keepalive > 3)) {
	    icq_Logout(&icql);
	    icq_Disconnect(&icql);
	    ildisconnected(&icql, ICQ_DISCONNECT_KEEPALIVE);
	}
    } else {
	if(manualstatus != offline)
	if(!connecting && (timer_current-timer_reconnect > PERIOD_RECONNECT))
	if(!manager.isopen()) {
	    connect();
	}

	if(connecting && (timer_current-timer_reconnect > PERIOD_RECONNECT)) {
	    icq_Disconnect(&icql);
	    disconnected(&icql, 0);
	}
    }
}

void icqhook::setfinddest(verticalmenu *m) {
    finddest = m;
}

void icqhook::clearfindresults() {
    founduins.clear();
}

void icqhook::addfile(unsigned int uin, unsigned long seq, string fname, int dir) {
    files.push_back(icqfileassociation(uin, seq, fname, dir));
}

bool icqhook::isconnecting() const {
    return connecting;
}

void icqhook::getsockets(fd_set &fds, int &hsocket) const {
    if(online()) {
	FD_SET(sockfd, &fds);
	hsocket = max(sockfd, hsocket);
    }
}

bool icqhook::isoursocket(fd_set &fds) const {
    return online() && FD_ISSET(sockfd, &fds);
}

bool icqhook::online() const {
    return connecting || logged();
}

unsigned long icqhook::sendmessage(const icqcontact *c, const string text) {
    return online() ?
	icq_SendMessage(&icql, c->getdesc().uin, text.c_str(),
	    c->getmsgdirect() ? ICQ_SEND_BESTWAY : ICQ_SEND_THRUSERVER)
	: 0;
}

imstatus icqhook::icq2imstatus(int status) const {
    imstatus st = available;

    if((unsigned long) STATUS_OFFLINE == status) st = offline; else
    if((status & STATUS_INVISIBLE) == STATUS_INVISIBLE) st = invisible; else
    if((status & STATUS_FREE_CHAT) == STATUS_FREE_CHAT) st = freeforchat; else
    if((status & STATUS_DND) == STATUS_DND) st = dontdisturb; else
    if((status & STATUS_OCCUPIED) == STATUS_OCCUPIED) st = occupied; else
    if((status & STATUS_NA) == STATUS_NA) st = notavail; else
    if((status & STATUS_AWAY) == STATUS_AWAY) st = away;

    return st;
}

bool icqhook::enabled() const {
    return factive;
}

void icqhook::setautostatus(imstatus st) {
    static int stat2int[imstatus_size] = {
	STATUS_OFFLINE,
	STATUS_ONLINE,
	STATUS_INVISIBLE,
	STATUS_FREE_CHAT,
	STATUS_DND,
	STATUS_OCCUPIED,
	STATUS_NA,
	STATUS_AWAY
    };

    if(st != offline) {
	if(getstatus() == offline) {
	    connect();
	} else {
	    icq_ChangeStatus(&icql, stat2int[st]);
	}
    } else {
	if(getstatus() != offline) {
	    disconnect();
	}
    }
}

imstatus icqhook::getstatus() const {
    return online() ? icq2imstatus(icql.icq_Status) : offline;
}

void icqhook::disconnect() {
    icq_Logout(&icql);
    icq_Disconnect(&icql);
    ildisconnected(&icql, ICQ_DISCONNECT_SOCKET);
}

void icqhook::regnewuin(struct icq_link *link, unsigned long uin) {
    ihook.reguin = uin;
}

void icqhook::main() {
    if(enabled()) {
	icq_HandleServerResponse(&icql);
    }
}

bool icqhook::regconnect(const string aserver) {
    icqconf::imaccount account(icq);
    string server;
    int pos, port;

    if((pos = aserver.find(":")) != -1) {
	server = aserver.substr(0, pos);
	port = strtoul(aserver.substr(pos+1).c_str(), 0, 0);
    } else {
	server = aserver;
	port = 4000;
	if(server.empty()) server = "icq.mirabilis.com";
    }

    if(!factive) {
	init(account);
    }

    icql.icq_NewUIN = &regnewuin;
    return icq_Connect(&icql, server.c_str(), port) != -1;
}

bool icqhook::regattempt(unsigned long &uin, const string password) {
    struct timeval tv;
    time_t spent;
    int sockfd;
    fd_set fds;

    reguin = 0;
    icq_RegNewUser(&icql, password.c_str());

    for(spent = time(0); !reguin && time(0)-spent < PERIOD_WAITNEWUIN; ) {
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET((sockfd = icq_GetSok(&icql)), &fds);
	select(sockfd+1, &fds, 0, 0, &tv);
	
	if(FD_ISSET(sockfd, &fds)) {
	    icq_HandleServerResponse(&icql);
	}
    }

    if(reguin) {
	icq_Disconnect(&icql);
	icq_Done(&icql);
	factive = false;

	uin = reguin;
    }

    return (bool) reguin;
}

void icqhook::sendnewuser(const imcontact ic) {
    if(enabled()) {
	icq_SendNewUser(&icql, ic.uin);
    }
}

void icqhook::sendcontactlist() {
    int i;
    icqcontact *c;

    if(logged()) {
	icq_ContactClear(&icql);

	for(i = 0; i < clist.count; i++) {
	    c = (icqcontact *) clist.at(i);

	    if(c->getdesc() != contactroot)
	    if(c->getdesc().pname == icq) {
		icq_ContactAdd(&icql, c->getdesc().uin);
		icq_ContactSetVis(&icql, c->getdesc().uin,
		    lst.inlist(c->getdesc(), csvisible) ? ICQ_CONT_VISIBLE :
		    lst.inlist(c->getdesc(), csinvisible) ? ICQ_CONT_INVISIBLE :
		    ICQ_CONT_NORMAL);
	    }
	}

	icq_SendContactList(&icql);
	icq_SendVisibleList(&icql);
	icq_SendInvisibleList(&icql);
    }
}

bool icqhook::isdirectopen(const imcontact c) {
    return (bool) icq_TCPLinkOpen(&icql, c.uin);
}

void icqhook::sendinforeq(icqcontact *c, unsigned long uin) {
    c->setseq2(icq_SendMetaInfoReq(&icql, c->getdesc().uin));
}

void icqhook::sendauth(unsigned long uin) {
    icq_SendAuthMsg(&icql, uin);
}

bool icqhook::acceptfile(unsigned long uin, unsigned long seq) {
    return (bool) icq_AcceptFileRequest(&icql, uin, seq);
}

void icqhook::refusefile(unsigned long uin, unsigned long seq) {
    icq_RefuseFileRequest(&icql, uin, seq, "refused");
}

unsigned long icqhook::sendurl(const imcontact c, const string url,
const string text, bool direct) {
    return icq_SendURL(&icql, c.uin, url.c_str(), text.c_str(),
	direct ? ICQ_SEND_BESTWAY : ICQ_SEND_THRUSERVER);
}

void icqhook::sendupdateuserinfo(icqcontact *c) {
    string fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate;
    string fphone, ffax, fstreet, fcellular, fhomepage, fwcity, fwstate;
    string fwphone, fwfax, fwaddress, fcompany, fdepartment, fjob;
    string fwhomepage, icat1, int1, icat2, int2, icat3, int3, icat4;
    string int4, fabout;

    unsigned char flang1, flang2, flang3, fbyear, fbmonth, fbday, fage, fgender;
    unsigned long fzip, fwzip;
    unsigned short fcountry, fwcountry, foccupation;

    c->getinfo(fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate, fphone, ffax, fstreet, fcellular, fzip, fcountry);

    c->getmoreinfo(fage, fgender, fhomepage, flang1, flang2, flang3, fbday, fbmonth, fbyear);

    c->getworkinfo(fwcity, fwstate, fwphone, fwfax, fwaddress, fwzip,
	fwcountry, fcompany, fdepartment, fjob, foccupation, fwhomepage);

    fabout = c->getabout();

    icq_UpdateUserInfo(&icql, c->getnick().c_str(), fname.c_str(),
	lname.c_str(), fprimemail.c_str());

    icq_UpdateMetaInfoHomepage(&icql, fage, fhomepage.c_str(),
	fbyear, fbmonth, fbday, flang1, flang2, flang3, fgender);

    icq_UpdateMetaInfoWork(&icql, fwcity.c_str(), fwstate.c_str(),
	fwphone.c_str(), fwfax.c_str(), fwaddress.c_str(),
	fcompany.c_str(), fdepartment.c_str(), fjob.c_str(),
	fwhomepage.c_str(), foccupation, fwcountry, fwzip);

    icq_UpdateMetaInfoSet(&icql, c->getnick().c_str(), fname.c_str(),
	lname.c_str(), fprimemail.c_str(), fsecemail.c_str(),
	foldemail.c_str(), fcity.c_str(), fstate.c_str(), fphone.c_str(),
	ffax.c_str(), fstreet.c_str(), fcellular.c_str(), fzip, fcountry,
	0);

    icq_UpdateMetaInfoAbout(&icql, fabout.c_str());
}

void icqhook::sendsearchreq(const searchparameters s) {
    if(s.uin) {
	icq_SendSearchUINReq(&icql, s.uin);
    } else
    if(s.minage || s.maxage || s.country || s.language
    || !s.city.empty() || !s.state.empty() || !s.company.empty()
    || !s.department.empty() || !s.position.empty() || s.onlineonly) {
	icq_SendWhitePagesSearchReq(&icql, s.firstname.c_str(),
	    s.lastname.c_str(), s.nick.c_str(), s.email.c_str(),
	    s.minage, s.maxage, s.gender, s.language, s.city.c_str(),
	    s.state.c_str(), s.country, s.company.c_str(),
	    s.department.c_str(), s.position.c_str(),
	    s.onlineonly ? 1 : 0);

    } else {
	icq_SendSearchReq(&icql, s.email.c_str(),
	    s.nick.c_str(), s.firstname.c_str(),
	    s.lastname.c_str());
    }
}

void icqhook::sendalert(const imcontact im) {
    icq_AlertAddUser(&icql, im.uin);
}
