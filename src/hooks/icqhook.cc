/*
*
* centericq icq protocol handling class
* $Id: icqhook.cc,v 1.11 2001/12/03 23:07:07 konst Exp $
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

#define PERIOD_ICQPING  60

icqhook ihook;

icqhook::icqhook() {
    manualstatus = conf.getstatus(icq);
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

unsigned long icqhook::sendmessage(const icqcontact *c, const string text) {
    NormalMessageEvent *sv;

    if(c->getstatus() != offline) {
	Contact ic(c->getdesc().uin);
	sv = new NormalMessageEvent(&ic, rusconv("kw", text));
	cli.SendEvent(sv);
	return 1;
    }

    return 0;

}

void icqhook::sendnewuser(const imcontact c) {
    Contact ic(c.uin);
    cli.addContact(ic);
}

void icqhook::setautostatus(imstatus st) {
}

imstatus icqhook::getstatus() const {
    return icq2imstatus(cli.getStatus());
}

bool icqhook::regconnect(const string aserv) {
    return true;
}

bool icqhook::regattempt(unsigned int &auin, const string apassword) {
    cli.setPassword(apassword);
    cli.RegisterUIN();
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

// ----------------------------------------------------------------------------

void icqhook::connected_cb(ConnectedEvent *ev) {
    face.log(_("+ [icq] logged in"));
    flogged = true;
    time(&timer_ping);
}

void icqhook::disconnected_cb(DisconnectedEvent *ev) {
    fds.clear();
    clist.setoffline(icq);
    fonline = false;
}

bool icqhook::messaged_cb(MessageEvent *ev) {
    icqcontact *c;
    NormalMessageEvent *nev;
    imcontact ic;
    time_t t;

    c = 0;
    ic = imcontact(ev->getContact()->getUIN(), icq);

    if(!lst.inlist(ic, csignore)) {
	switch(ev->getType()) {
	    case MessageEvent::Normal:
		if(nev = dynamic_cast<NormalMessageEvent*>(ev)) {
		    hist.putmessage(ic, rusconv("wk", nev->getMessage()),
			HIST_MSG_IN, localtime(&(t = nev->getTime())));

		    c = clist.get(ic);

		    if(c) {
			c->playsound(EVT_MSG);
		    }
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

	if(c) {
	    c->setmsgcount(c->getmsgcount()+1);
	    face.relaxedupdate();
	}
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

		MainHomeInfo &home = ic->getMainHomeInfo();
		HomepageInfo &hpage = ic->getHomepageInfo();
//                WorkInfo &work = ic->getWorkInfo();

		if(c->getnick() == c->getdispnick()) {
		    c->setdispnick(ic->getAlias());
		}

		c->setnick(ic->getAlias());

		c->setinfo(ic->getFirstName(), ic->getLastName(),
		    ic->getEmail(), "", "", home.city, home.state,
		    home.phone, home.fax, home.street, home.cellular,
		    atol(home.zip.c_str()), home.country);

		c->setmoreinfo(hpage.age, hpage.sex, hpage.homepage,
		    hpage.lang1, hpage.lang2, hpage.lang3, hpage.birth_day,
		    hpage.birth_month, hpage.birth_year);

/*
		c->setworkinfo(work.city, work.state, "",
		    "", work.street, atol(work.zip.c_str()),
		    work.country, work.company_name, work.company_dept,
		    work.company_position, 0, work.company_web);
*/
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
