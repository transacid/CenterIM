/*
*
* centericq MSN protocol handling class
* $Id: msnhook.cc,v 1.44 2002/11/26 12:24:52 konst Exp $
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

#include "msnhook.h"
#include "icqconf.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "accountmanager.h"
#include "eventmanager.h"
#include "imlogger.h"

msnhook mhook;

#define TIMESTAMP maketm(d->hour-1, d->minute, d->day, d->month, d->year)

msnhook::msnhook() {
    status = offline;
    fonline = false;

    fcapabs.insert(hookcapab::synclist);
    fcapabs.insert(hookcapab::changedetails);
    fcapabs.insert(hookcapab::directadd);

    for(int i = MSN_RNG; i != MSN_NUM_EVENTS; i++) {
	msn_event[i] = 0;
    }

    MSN_RegisterCallback(MSN_MSG, &messaged);
    MSN_RegisterCallback(MSN_ILN, &statuschanged);
    MSN_RegisterCallback(MSN_NLN, &statuschanged);
    MSN_RegisterCallback(MSN_FLN, &statuschanged);
    MSN_RegisterCallback(MSN_AUTH, &authrequested);
    MSN_RegisterCallback(MSN_OUT, &disconnected);
    MSN_RegisterCallback(MSN_RNG, &ring);
    MSN_RegisterCallback(MSN_MAIL, &mailed);

#ifdef DEBUG
    MSN_RegisterCallback(MSN_LOG, &log);
#endif
}

msnhook::~msnhook() {
}

void msnhook::init() {
    manualstatus = conf.getstatus(msn);
}

void msnhook::connect() {
    icqconf::imaccount account = conf.getourid(msn);

    face.log(_("+ [msn] connecting to the server"));

    if(!MSN_Login(account.nickname.c_str(), account.password.c_str(),
    account.server.c_str(), account.port)) {
	face.log(_("+ [msn] logged in"));
	fonline = true;
	status = available;
	setautostatus(manualstatus);
    } else {
	face.log(_("+ [msn] unable to connect to the server"));
    }

    msn_Russian = conf.getrussian();
}

void msnhook::disconnect() {
    if(online()) {
	disconnected(0);
    }
}

void msnhook::exectimers() {
}

void msnhook::main() {
    if(online()) {
	MSN_Main();
    }
}

void msnhook::getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const {
    vector<int> sockets = MSN_GetSockets();
    vector<int>::iterator i;

    for(i = sockets.begin(); i != sockets.end(); ++i) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &rfds);
    }
}

bool msnhook::isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const {
    vector<int> sockets = MSN_GetSockets();
    vector<int>::iterator i;

    for(i = sockets.begin(); i != sockets.end(); ++i) {
	if(FD_ISSET(*i, &rfds))
	    return true;
    }

    return false;
}

bool msnhook::online() const {
    return fonline;
}

bool msnhook::logged() const {
    return fonline;
}

bool msnhook::isconnecting() const {
    return false;
}

bool msnhook::enabled() const {
    return true;
}

bool msnhook::send(const imevent &ev) {
    string text;

    if(ev.gettype() == imevent::message) {
	const immessage *m = static_cast<const immessage *>(&ev);
	if(m) text = m->gettext();
    } else if(ev.gettype() == imevent::url) {
	const imurl *m = static_cast<const imurl *>(&ev);
	if(m) text = m->geturl() + "\n\n" + m->getdescription();
    }

    icqcontact *c = clist.get(ev.getcontact());
    text = siconv(text, conf.getrussian() ? "koi8-u" : DEFAULT_CHARSET, "utf8");

    if(c)
    if(c->getstatus() != offline || !c->inlist()) {
	MSN_SendMessage(ev.getcontact().nickname.c_str(), text.c_str());
	return true;
    }

    return false;
}

void msnhook::sendnewuser(const imcontact &c) {
    if(logged()) {
	MSN_AddContact(c.nickname.c_str());
    }

    requestinfo(c);
}

void msnhook::setautostatus(imstatus st) {
    static int stat2int[imstatus_size] = {
	USER_FLN,
	USER_NLN,
	USER_HDN,
	USER_NLN,
	USER_BSY,
	USER_BSY,
	USER_AWY,
	USER_IDL
    };

    if(st != offline) {
	if(getstatus() == offline) {
	    connect();
	} else {
	    logger.putourstatus(msn, status, st);
	    MSN_ChangeState(stat2int[status = st]);
	}
    } else {
	if(getstatus() != offline) {
	    disconnect();
	}
    }
}

imstatus msnhook::getstatus() const {
    return status;
}

void msnhook::removeuser(const imcontact &ic) {
    removeuser(ic, true);
}

void msnhook::removeuser(const imcontact &ic, bool report) {
    if(online()) {
	if(report)
	    face.log(_("+ [msn] removing %s from the contacts"), ic.nickname.c_str());

	MSN_RemoveContact(ic.nickname.c_str());
    }
}

bool msnhook::isourid(const string &nick) {
    string s1, s2;
    icqconf::imaccount account = conf.getourid(msn);

    s1 = nick, s2 = account.nickname;

    if(s1.find("@") == -1) s1 += "@hotmail.com";
    if(s2.find("@") == -1) s2 += "@hotmail.com";

    return (s1 == s2);
}

imstatus msnhook::msn2imstatus(int st) {
    switch(st) {
	case USER_HDN:
	    return invisible;

	case USER_BSY:
	case USER_PHN:
	    return occupied;

	case USER_IDL:
	case USER_BRB:
	case USER_AWY:
	    return away;

	case USER_LUN:
	    return notavail;

	case USER_FLN:
	    return offline;

	case USER_NLN:
	default:
	    return available;
    }

    return offline;
}

void msnhook::requestinfo(const imcontact &ic) {
    icqcontact *c = clist.get(ic);

    if(c) {
	icqcontact::moreinfo m = c->getmoreinfo();
	icqcontact::basicinfo b = c->getbasicinfo();

	b.email = ic.nickname;
	if(b.email.find("@") == -1) b.email += "@hotmail.com";
	m.homepage = "http://members.msn.com/" + b.email;

	if(!friendlynicks[ic.nickname].empty()) {
	    c->setdispnick(friendlynicks[ic.nickname]);
	    b.fname = friendlynicks[ic.nickname];
	}

	c->setbasicinfo(b);
	c->setmoreinfo(m);

	face.relaxedupdate();
    }
}

void msnhook::lookup(const imsearchparams &params, verticalmenu &dest) {
    if(params.reverse) {
	vector< pair<string, string> > rl = MSN_GetLSTUsers("RL");
	vector< pair<string, string> >::const_iterator i = rl.begin();

	while(i != rl.end()) {
	    icqcontact *c = new icqcontact(imcontact(i->first, msn));
	    c->setdispnick(i->second);

	    dest.additem(conf.getcolor(cp_clist_msn), c, (string) " " + i->first);
	    ++i;
	}

	face.findready();

	face.log(_("+ [msn] reverse users listing finished, %d found"),
	    rl.size());

	dest.redraw();
    }
}

vector<icqcontact *> msnhook::getneedsync() {
    int i;
    vector<icqcontact *> r;
    bool found;
    vector< pair<string, string> > fl = MSN_GetLSTUsers("FL");

    for(i = 0; i < clist.count; i++) {
	icqcontact *c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == msn) {
	    vector< pair<string, string> >::const_iterator fi = fl.begin();

	    for(found = false; fi != fl.end() && !found; ++fi)
		found = c->getdesc().nickname == fi->first;

	    if(!found)
		r.push_back(c);
	}
    }

    return r;
}

void msnhook::sendupdateuserinfo(const icqcontact &c) {
    int pos;
    string nick = mime(c.getnick());

    while((pos = nick.find("+")) != -1)
	nick.replace(pos, 1, "%20");

    MSN_Set_Friendly_Handle(nick.c_str());
}

void msnhook::checkfriendly(icqcontact *c, const string friendlynick, bool forcefetch) {
    string oldnick = friendlynicks[c->getdesc().nickname];
    friendlynicks[c->getdesc().nickname] = unmime(friendlynick);

    if(forcefetch || (!oldnick.empty()
    && c->getdispnick() == oldnick
    && c->getbasicinfo().fname == oldnick))
	requestinfo(c);
}

// ----------------------------------------------------------------------------

void msnhook::messaged(void *data) {
    MSN_InstantMessage *d = (MSN_InstantMessage *) data;
    imcontact ic(d->sender, msn);
    icqcontact *c;
    string text;
    bool forcefetch;

    if(!isourid(d->sender))
    if(strlen(d->msg)) {
	c = clist.get(ic);

	if(forcefetch = !c)
	    c = clist.addnew(ic);

	if(c->inlist() && c->getstatus() == offline)
	    mhook.sendnewuser(ic);

	if(d->friendlyhandle)
	    mhook.checkfriendly(c, d->friendlyhandle, forcefetch);

	text = siconv(d->msg, "utf8", conf.getrussian() ? "koi8-u" : DEFAULT_CHARSET);
	em.store(immessage(ic, imevent::incoming, text));
    }
}

void msnhook::statuschanged(void *data) {
    MSN_StatusChange *d = (MSN_StatusChange *) data;
    imcontact ic(d->handle, msn);
    icqcontact *c = clist.get(ic);
    bool forcefetch;

    if(!isourid(d->handle)) {
	if(forcefetch = !c)
	    c = clist.addnew(ic, false);

	if(d->friendlyhandle)
	    mhook.checkfriendly(c, d->friendlyhandle, forcefetch);

	logger.putonline(ic, c->getstatus(), msn2imstatus(d->newStatus));
	c->setstatus(msn2imstatus(d->newStatus));
    }
}

void msnhook::authrequested(void *data) {
    MSN_AuthMessage *msg = (MSN_AuthMessage *) data;
    MSN_AuthorizeContact(msg->conn, msg->requestor);
}

void msnhook::disconnected(void *data) {
    int i;
    icqcontact *c;

    if(mhook.online()) {
	MSN_Logout();
	face.log(_("+ [msn] disconnected"));
	mhook.fonline = false;
    }

    clist.setoffline(msn);
    mhook.status = offline;
    face.update();
}

void msnhook::log(void *data) {
    face.log("%s", (const char *) data);
}

void msnhook::ring(void *data) {
    MSN_Ring *ring = (MSN_Ring *) data;

    if(ring->mode == outgoing) {
	face.log(_("+ [msn] connecting with %s"), ring->handle.c_str());
    } else {
	face.log(_("+ [msn] connection from %s"), ring->handle.c_str());
    }
}

#define cutdata(s) if(strlen(s) > 300) s[300] = 0;

void msnhook::mailed(void *data) {
    MSN_MailNotification *mail = (MSN_MailNotification *) data;

    if(mail->from) {
	char buf[1024];

	cutdata(mail->from);
	cutdata(mail->fromaddr);
	cutdata(mail->subject);

	sprintf(buf, _("+ [msn] e-mail from %s <%s>, %s"),
	    mail->from, mail->fromaddr, mail->subject);

	face.log(buf);
	clist.get(contactroot)->playsound(imevent::email);
    }
}
