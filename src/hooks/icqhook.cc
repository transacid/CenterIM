/*
*
* centericq icq protocol handling class
* $Id: icqhook.cc,v 1.13 2001/12/05 17:13:49 konst Exp $
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

icqhook::icqhook() {
//    manualstatus = conf.getstatus(icq);
    fcapabilities = hoptCanNotify;
    timer_reconnect = 0;
    fonline = false;

    cli.connected.connect(slot(this, &icqhook::connected_cb));
    cli.disconnected.connect(slot(this, &icqhook::disconnected_cb));
    cli.socket.connect(slot(this, &icqhook::socket_cb));
    cli.messaged.connect(slot(this, &icqhook::messaged_cb));
    cli.messageack.connect(slot(this, &icqhook::messageack_cb));
    cli.away_message.connect(slot(this, &icqhook::away_message_cb));
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
    cli.away_message.clear();
    cli.contactlist.clear();
    cli.newuin.clear();
    cli.rate.clear();
    cli.statuschanged.clear();
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
    cli.Connect();

    time(&timer_reconnect);
    fonline = true;
    flogged = false;
}

void icqhook::disconnect() {
    cli.Disconnect();
}

void icqhook::exectimers() {
    time_t tcurrent = time(0);

    if(logged()) {
	if(tcurrent-timer_ping > PERIOD_ICQPING) {
	    cli.PingServer();
	    time(&timer_ping);
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
    set<int>::iterator i;
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
	    }
	}
    }
}

void icqhook::getsockets(fd_set &afds, int &hsocket) const {
    set<int>::iterator i;

    for(i = fds.begin(); i != fds.end(); i++) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &afds);
    }
}

bool icqhook::isoursocket(fd_set &afds) const {
    set<int>::iterator i;

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

bool icqhook::send(const imcontact &cont, const imevent &ev) {
    Contact ic(cont.uin);

    if(ev.gettype() == imevent::message) {
	const immessage *m = static_cast<const immessage *> (&ev);
	cli.SendEvent(new NormalMessageEvent(&ic, m->gettext()));
    } else if(ev.gettype() == imevent::url) {
	const imurl *m = static_cast<const imurl *> (&ev);
	cli.SendEvent(new URLMessageEvent(&ic, m->getdescription(), m->geturl()));
    } else if(ev.gettype() == imevent::sms) {
	const imsms *m = static_cast<const imsms *> (&ev);
	cli.SendEvent(new SMSMessageEvent(&ic, m->gettext(), false));
    } else if(ev.gettype() == imevent::authorization) {
	const imauthorization *m = static_cast<const imauthorization *> (&ev);
	cli.SendEvent(new AuthReqEvent(&ic, m->gettext()));
    } else {
	return false;
    }

    return true;
}

void icqhook::sendnewuser(const imcontact c) {
    Contact ic(c.uin);
    cli.addContact(ic);
}

void icqhook::setautostatus(imstatus st) {
    static Status stat2int[imstatus_size] = {
	STATUS_OFFLINE,
	STATUS_ONLINE,
	 STATUS_OFFLINE,
	STATUS_FREEFORCHAT,
	STATUS_DND,
	STATUS_OCCUPIED,
	STATUS_NA,
	STATUS_AWAY
    };

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
    fd_set fds;
    struct timeval tv;
    int hsockfd;
    time_t regtimeout = time(0);

    reguin = 0;
    cli.setPassword(apassword);
    cli.RegisterUIN();

    while(!reguin && (time(0)-regtimeout < 60)) {
	hsockfd = 0;
	FD_ZERO(&fds);
	getsockets(fds, hsockfd);

	tv.tv_sec = 30;
	tv.tv_usec = 0;

	select(hsockfd+1, &fds, 0, 0, &tv);

	if(isoursocket(fds)) {
	    main();
	}
    }

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
    face.log(_("+ [icq] logged in"));
    flogged = true;
    time(&timer_ping);
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
}

bool icqhook::messaged_cb(MessageEvent *ev) {
    NormalMessageEvent *nev;
    imcontact ic;
    time_t t;

    ic = imcontact(ev->getContact()->getUIN(), icq);

    switch(ev->getType()) {
	case MessageEvent::Normal:
	    if(nev = dynamic_cast<NormalMessageEvent*>(ev)) {
		em.store(immessage(ic, imevent::incoming, nev->getMessage()));
	    }
	    break;

	case MessageEvent::URL:
	case MessageEvent::SMS:
	case MessageEvent::SMS_Response:
	case MessageEvent::SMS_Receipt:
	case MessageEvent::AuthReq:
	case MessageEvent::AuthAck:
	    face.log("! received something we don't support yet");
	    break;
    }

    return true;
}

void icqhook::messageack_cb(MessageEvent *ev) {
}

void icqhook::away_message_cb(AwayMsgEvent *ev) {
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

		icqcontact::basicinfo cbinfo;
		MainHomeInfo &home = ic->getMainHomeInfo();

		cbinfo.fname = ic->getFirstName();
		cbinfo.lname = ic->getLastName();
		cbinfo.email = ic->getEmail();
		cbinfo.city = home.city;
		cbinfo.state = home.state;
		cbinfo.phone = home.phone;
		cbinfo.fax = home.fax;
		cbinfo.street = home.street;
		cbinfo.cellular = home.cellular;
		cbinfo.zip = strtoul(home.zip.c_str(), 0, 0);
		cbinfo.country = getcountryname(home.country);

		icqcontact::moreinfo cminfo;
		HomepageInfo &hpage = ic->getHomepageInfo();

		cminfo.age = hpage.age;
		cminfo.gender = genderUnspec;
		cminfo.homepage = hpage.homepage;
		cminfo.birth_day = hpage.birth_day;
		cminfo.birth_month = hpage.birth_month;
		cminfo.birth_year = hpage.birth_year;

		if((c->getnick() == c->getdispnick())
		|| (c->getdispnick() == i2str(ev->getUIN()))) {
		    c->setdispnick(ic->getAlias());
		}

		c->setnick(ic->getAlias());
		c->setbasicinfo(cbinfo);
		c->setmoreinfo(cminfo);
		c->setabout(ic->getAboutInfo());

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
    if(ev->getType() != LogEvent::PACKET) {
	face.log(ev->getMessage());
    }
}

void icqhook::socket_cb(SocketEvent *ev) {
    if(dynamic_cast<AddSocketHandleEvent*>(ev) != NULL) {
	AddSocketHandleEvent *cev = dynamic_cast<AddSocketHandleEvent*>(ev);
	fds.insert(cev->getSocketHandle());
    } else if(dynamic_cast<RemoveSocketHandleEvent*>(ev) != NULL) {
	RemoveSocketHandleEvent *cev = dynamic_cast<RemoveSocketHandleEvent*>(ev);
	fds.erase(cev->getSocketHandle());
    }
}

void icqhook::statuschanged_cb(MyStatusChangeEvent *ev) {
}
