/*
*
* centericq icq protocol handling class
* $Id: icqhook.cc,v 1.25 2001/12/13 13:25:13 konst Exp $
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
#include "icqconf.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "accountmanager.h"
#include "centericq.h"

#include "userinfoconstants.h"

#define PERIOD_ICQPING  60

icqhook ihook;

static const Status stat2int[imstatus_size] = {
    STATUS_OFFLINE,
    STATUS_ONLINE,
    STATUS_ONLINE, /* invisible */
    STATUS_FREEFORCHAT,
    STATUS_DND,
    STATUS_OCCUPIED,
    STATUS_NA,
    STATUS_AWAY
};

icqhook::icqhook() {
    timer_reconnect = 0;
    fonline = false;

    fcapabilities = hoptCanSendURL | hoptCanSendSMS;

    cli.connected.connect(slot(this, &icqhook::connected_cb));
    cli.disconnected.connect(slot(this, &icqhook::disconnected_cb));
    cli.socket.connect(slot(this, &icqhook::socket_cb));
    cli.messaged.connect(slot(this, &icqhook::messaged_cb));
    cli.messageack.connect(slot(this, &icqhook::messageack_cb));
    cli.contactlist.connect(slot(this, &icqhook::contactlist_cb));
    cli.newuin.connect(slot(this, &icqhook::newuin_cb));
    cli.rate.connect(slot(this, &icqhook::rate_cb));
    cli.statuschanged.connect(slot(this, &icqhook::statuschanged_cb));

#ifdef DEBUG
    cli.logger.connect(slot(this, &icqhook::logger_cb));
#endif
}

icqhook::~icqhook() {
    cli.logger.clear();
    cli.connected.clear();
    cli.disconnected.clear();
    cli.socket.clear();
    cli.messaged.clear();
    cli.messageack.clear();
    cli.contactlist.clear();
    cli.newuin.clear();
    cli.rate.clear();
    cli.statuschanged.clear();
}

void icqhook::init() {
    manualstatus = conf.getstatus(icq);
}

void icqhook::connect() {
    icqconf::imaccount acc = conf.getourid(icq);
    int i;
    icqcontact *c;

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == icq) {
	    Contact ic(c->getdesc().uin);
	    cli.addContact(ic);
	}
    }

    cli.setUIN(acc.uin);
    cli.setPassword(acc.password);

    face.log(_("+ [icq] connecting to the server"));

    cli.setInvisible(manualstatus == invisible);
    cli.setStatus(stat2int[manualstatus]);

    time(&timer_reconnect);
    fonline = true;
    flogged = false;
}

void icqhook::disconnect() {
    cli.setStatus(STATUS_OFFLINE);
}

void icqhook::resolve() {
    int i;
    icqcontact *c;
    imcontact cont;

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);
	cont = c->getdesc();

	if(cont.pname == icq)
	if(c->getdispnick() == i2str(c->getdesc().uin)) {
	    requestinfo(cont);
	}
    }
}

void icqhook::exectimers() {
    time_t tcurrent = time(0);

    if(logged()) {
	if(tcurrent-timer_ping > PERIOD_ICQPING) {
	    cli.PingServer();
	    time(&timer_ping);
	} else if(tcurrent-timer_resolve > PERIOD_RESOLVE) {
	    resolve();
	    time(&timer_resolve);
	}
    } else {
	if(tcurrent-timer_reconnect > PERIOD_RECONNECT) {
	    if(online() && !logged()) {
		disconnect();
	    } else if(manualstatus != offline) {
		if(!manager.isopen()) {
		    connect();
		}
	    }
	}
    }
}

void icqhook::main() {
    struct timeval tv;
    fd_set s;
    vector<int>::iterator i;
    int hsock;

    FD_ZERO(&s);
    tv.tv_sec = tv.tv_usec = 0;
    hsock = 0;

    for(i = fds.begin(); i != fds.end(); i++) {
	FD_SET(*i, &s);
	hsock = max(hsock, *i);
    }

    if(select(hsock+1, &s, 0, 0, &tv) > 0) {
	for(i = fds.begin(); i != fds.end(); i++) {
	    if(FD_ISSET(*i, &s)) {
		cli.socket_cb(*i, SocketEvent::READ);
		break;
	    }
	}
    }
}

void icqhook::getsockets(fd_set &afds, int &hsocket) const {
    vector<int>::const_iterator i;

    for(i = fds.begin(); i != fds.end(); i++) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &afds);
    }
}

bool icqhook::isoursocket(fd_set &afds) const {
    vector<int>::const_iterator i;

    for(i = fds.begin(); i != fds.end(); i++)
	if(FD_ISSET(*i, &afds))
	    return true;

    return false;
}

bool icqhook::online() const {
    return fonline;
}

bool icqhook::logged() const {
    return fonline && flogged;
}

bool icqhook::isconnecting() const {
    return fonline && !flogged;
}

bool icqhook::enabled() const {
    return true;
}

bool icqhook::send(const imevent &ev) {
    unsigned int uin = ev.getcontact().uin;
    Contact ic(uin);

    if(ev.gettype() == imevent::message) {
	const immessage *m = static_cast<const immessage *> (&ev);
	cli.SendEvent(new NormalMessageEvent(&ic, rusconv("kw", m->gettext())));

    } else if(ev.gettype() == imevent::url) {
	const imurl *m = static_cast<const imurl *> (&ev);
	cli.SendEvent(new URLMessageEvent(&ic,
	    rusconv("kw", m->getdescription()),
	    rusconv("kw", m->geturl())));

    } else if(ev.gettype() == imevent::sms) {
	const imsms *m = static_cast<const imsms *> (&ev);
	ic.setMobileNo(clist.get(ev.getcontact())->getbasicinfo().cellular);
	cli.SendEvent(new SMSMessageEvent(&ic, rusconv("kw", m->gettext()), false));

    } else if(ev.gettype() == imevent::authorization) {
	const imauthorization *m = static_cast<const imauthorization *> (&ev);
	cli.SendEvent(new AuthAckEvent(&ic,
	    rusconv("kw", m->gettext()), m->getgranted()));

    } else if(ev.gettype() == imevent::sms) {
	const imsms *m = static_cast<const imsms *> (&ev);
	cli.SendEvent(new SMSMessageEvent(&ic, rusconv("kw", m->gettext()),
	    true));

    } else {
	return false;
    }

    return true;
}

void icqhook::sendnewuser(const imcontact c) {
    if(logged()) {
	Contact ic(c.uin);
	cli.addContact(ic);
	cli.fetchSimpleContactInfo(&ic);
    }
}

void icqhook::setautostatus(imstatus st) {
    if(st != offline) {
	if(getstatus() == offline) {
	    connect();
	} else {
	    cli.setInvisible(st == invisible);
	    cli.setStatus(stat2int[st]);
	}
    } else {
	if(getstatus() != offline) {
	    disconnect();
	}
    }
}

imstatus icqhook::getstatus() const {
    if(!fonline) {
	return offline;
    } else if(cli.getInvisible()) {
	return invisible;
    } else {
	return icq2imstatus(cli.getStatus());
    }
}

bool icqhook::regconnect(const string aserv) {
    return true;
}

bool icqhook::regattempt(unsigned int &auin, const string apassword) {
    fd_set rfds;
    struct timeval tv;
    int hsockfd;
    time_t regtimeout = time(0);

    reguin = 0;

    cli.setPassword(apassword);
    cli.RegisterUIN();

    while(!reguin && (time(0)-regtimeout < 60)) {
	hsockfd = 0;
	getsockets(rfds, hsockfd);

	tv.tv_sec = 30;
	tv.tv_usec = 0;

	if(isoursocket(rfds)) {
	    main();
	}
    }

    auin = reguin;
    return (bool) reguin;
}

imstatus icqhook::icq2imstatus(const Status st) const {
    switch(st) {
	case STATUS_AWAY:
	    return away;
	case STATUS_NA:
	    return notavail;
	case STATUS_OCCUPIED:
	    return occupied;
	case STATUS_DND:
	    return dontdisturb;
	case STATUS_FREEFORCHAT:
	    return freeforchat;
	case STATUS_OFFLINE:
	    return offline;
	case STATUS_ONLINE:
	default:
	    return available;
    }
}

void icqhook::requestinfo(const imcontact c) {
    if(logged()) {
	Contact ic(c.uin);
	cli.addContact(ic);
	cli.fetchSimpleContactInfo(&ic);
    }
}

const string icqhook::getcountryname(int code) const {
    int i;

    for(i = 0; i < Country_table_size; i++) {
	if(Country_table[i].code == code) {
	    return Country_table[i].name;
	}
    }

    return "";
}

// ----------------------------------------------------------------------------

void icqhook::connected_cb(ConnectedEvent *ev) {
    flogged = true;

    time(&timer_ping);
    timer_resolve = time(0)-PERIOD_RESOLVE+2;

    face.log(_("+ [icq] logged in"));
    face.update();
}

void icqhook::disconnected_cb(DisconnectedEvent *ev) {
    string msg;

    static const string reasons[DisconnectedEvent::FAILED_UNKNOWN+1] = {
	_("as requested"),
	_("socket problems"),
	_("bad username"),
	_("turboing"),
	_("bad password"),
	_("username and password mismatch"),
	_("already logged"),
	""
    };

    fds.clear();

    clist.setoffline(icq);
    fonline = false;

    msg = _("+ [icq] disconnected");
    if(!reasons[ev->getReason()].empty()) {
	msg += ", ";
	msg += reasons[ev->getReason()];
    }

    face.log(msg);
    face.update();
}

bool icqhook::messaged_cb(MessageEvent *ev) {
    imcontact ic;
    time_t t;

    ic = imcontact(ev->getContact()->getUIN(), icq);

    if(ev->getType() == MessageEvent::Normal) {
	NormalMessageEvent *r;
	if(r = dynamic_cast<NormalMessageEvent *>(ev)) {
	    em.store(immessage(ic, imevent::incoming,
		rusconv("wk", r->getMessage())));
	}

    } else if(ev->getType() == MessageEvent::URL) {
	URLMessageEvent *r;
	if(r = dynamic_cast<URLMessageEvent *>(ev)) {
	    em.store(imurl(ic, imevent::incoming,
		rusconv("wk", r->getURL()),
		rusconv("wk", r->getMessage())));
	}

    } else if(ev->getType() == MessageEvent::SMS) {
	SMSMessageEvent *r;

	if(r = dynamic_cast<SMSMessageEvent *>(ev)) {
	    icqcontact *c = clist.getmobile(r->getSender());

	    if(!c)
	    if(c = clist.addnew(imcontact(0, infocard), true)) {
		c->setdispnick("mobile");
		icqcontact::basicinfo b = c->getbasicinfo();
		b.cellular = r->getSender();
		c->setbasicinfo(b);
		c->setnick(b.cellular);
	    }

	    if(c) {
		em.store(imsms(c->getdesc(), imevent::incoming,
		    rusconv("wk", r->getMessage())));
	    }
	}
    } else if(ev->getType() == MessageEvent::SMS_Response) {
	SMSResponseEvent *r;
	if(r = dynamic_cast<SMSResponseEvent *>(ev)) {
	    face.log("sms response: %s", r->deliverable() ? "deliverable" : "undeliverable");
	    face.log(r->getSource());
	    face.log(r->getNetwork());
	    face.log(r->getErrorParam());
	}

    } else if(ev->getType() == MessageEvent::SMS_Receipt) {
	SMSReceiptEvent *r;
	if(r = dynamic_cast<SMSReceiptEvent *>(ev)) {
	    face.log("sms receipt %s", r->delivered() ? "delivered" : "failed");
	    face.log(r->getMessage());
	    face.log(r->getMessageId());
	    face.log(r->getDestination());
	    face.log(r->getSubmissionTime());
	    face.log(r->getDeliveryTime());
	}

    } else if(ev->getType() == MessageEvent::AuthReq) {
	AuthReqEvent *r;
	if(r = dynamic_cast<AuthReqEvent *>(ev)) {
	    em.store(imauthorization(ic, imevent::incoming, false,
		rusconv("wk", r->getMessage())));
	}

    } else if(ev->getType() == MessageEvent::AuthAck) {
    }

    return true;
}

void icqhook::messageack_cb(MessageEvent *ev) {
    Contact *ic;

    if(ev->isFinished() && ev->isDelivered()) {
	if(ev->getType() != MessageEvent::AwayMessage) {
	    if(ic = ev->getContact()) {
//                busy.erase(ic->getUIN());
	    }
	}
    }
}

void icqhook::contactlist_cb(ContactListEvent *ev) {
    icqcontact *c = clist.get(imcontact(ev->getUIN(), icq));
    char buf[64];
    int ip;
    string lastip;

    switch(ev->getType()) {
	case ContactListEvent::StatusChange:
	    if(dynamic_cast<StatusChangeEvent*>(ev))
	    if(c) {
		Contact *ic = cli.getContact(ev->getUIN());
		StatusChangeEvent *tev = dynamic_cast<StatusChangeEvent*>(ev);
		c->setstatus(icq2imstatus(tev->getStatus()));

		if(c->getstatus() != offline) {
		    if(inet_ntop(AF_INET, &(ip = ic->getExtIP()), buf, 64))
			lastip = buf;

		    if(inet_ntop(AF_INET, &(ip = ic->getLanIP()), buf, 64)) {
			if(!lastip.empty()) lastip += " ";
			lastip += buf;
		    }
		}
	    }
	    break;

	case ContactListEvent::UserInfoChange:
	    if(dynamic_cast<UserInfoChangeEvent*>(ev)) {
		Contact *ic = cli.getContact(ev->getUIN());

		if(!c) {
		    c = clist.get(contactroot);
		}

		icqcontact::basicinfo cbinfo = c->getbasicinfo();
		MainHomeInfo &home = ic->getMainHomeInfo();

		cbinfo.fname = rusconv("wk", ic->getFirstName());
		cbinfo.lname = rusconv("wk", ic->getLastName());
		cbinfo.email = rusconv("wk", ic->getEmail());
		cbinfo.city = rusconv("wk", home.city);
		cbinfo.state = rusconv("wk", home.state);
		cbinfo.phone = rusconv("wk", home.phone);
		cbinfo.fax = rusconv("wk", home.fax);
		cbinfo.street = rusconv("wk", home.street);

		if(!home.cellular.empty())
		    cbinfo.cellular = rusconv("wk", home.cellular);

		cbinfo.zip = strtoul(home.zip.c_str(), 0, 0);
		cbinfo.country = getcountryname(home.country);

		icqcontact::moreinfo cminfo = c->getmoreinfo();
		HomepageInfo &hpage = ic->getHomepageInfo();

		cminfo.age = hpage.age;
		cminfo.gender = genderUnspec;
		cminfo.homepage = rusconv("wk", hpage.homepage);
		cminfo.birth_day = hpage.birth_day;
		cminfo.birth_month = hpage.birth_month;
		cminfo.birth_year = hpage.birth_year;

		string nick = rusconv("wk", ic->getAlias());

		if((c->getnick() == c->getdispnick())
		|| (c->getdispnick() == i2str(ev->getUIN()))) {
		    c->setdispnick(nick);
		}

		c->setnick(nick);
		c->setbasicinfo(cbinfo);
		c->setmoreinfo(cminfo);
		c->setabout(rusconv("wk", ic->getAboutInfo()));

		face.relaxedupdate();
	    }
	    break;

	case ContactListEvent::UserAdded:
	    break;

	case ContactListEvent::UserRemoved:
	    break;

	case ContactListEvent::MessageQueueChanged:
	    break;
    }
}

void icqhook::newuin_cb(NewUINEvent *ev) {
    reguin = ev->getUIN();
}

void icqhook::rate_cb(RateInfoChangeEvent *ev) {
}

void icqhook::logger_cb(LogEvent *ev) {
    switch(ev->getType()) {
	case LogEvent::PACKET:
	case LogEvent::DIRECTPACKET:
	    break;

	default:
	    face.log(ev->getMessage());
	    break;
    }
}

void icqhook::socket_cb(SocketEvent *ev) {
    if(dynamic_cast<AddSocketHandleEvent*>(ev)) {
	AddSocketHandleEvent *cev = dynamic_cast<AddSocketHandleEvent*>(ev);
	fds.push_back(cev->getSocketHandle());
    } else if(dynamic_cast<RemoveSocketHandleEvent*>(ev)) {
	RemoveSocketHandleEvent *cev = dynamic_cast<RemoveSocketHandleEvent*>(ev);

	vector<int>::iterator i = find(fds.begin(), fds.end(), cev->getSocketHandle());
	if(i != fds.end()) {
	    fds.erase(i);
	}
    }
}

void icqhook::statuschanged_cb(MyStatusChangeEvent *ev) {
    face.update();
}
