/*
*
* centericq icq protocol handling class
* $Id: icqhook.cc,v 1.37 2002/01/18 18:01:44 konst Exp $
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
#include "imlogger.h"

#include "libicq2000/userinfoconstants.h"

#define PERIOD_ICQPOLL  5

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
    cli.want_auto_resp.connect(slot(this, &icqhook::want_auto_resp_cb));
    cli.search_result.connect(slot(this, &icqhook::search_result_cb));

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

    if(!acc.server.empty()) cli.setLoginServerHost(acc.server);
    if(acc.port) cli.setLoginServerPort(acc.port);

    cli.setUIN(acc.uin);
    cli.setPassword(acc.password);

    face.log(_("+ [icq] connecting to the server"));

    sendinvisible();
    cli.setStatus(stat2int[manualstatus], manualstatus == invisible);
    logger.putourstatus(icq, offline, manualstatus);

    time(&timer_reconnect);
    fonline = true;
    flogged = false;
}

void icqhook::disconnect() {
    logger.putourstatus(icq, getstatus(), offline);
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

void icqhook::sendinvisible() {
    icqlist::iterator i;
    Contact *c;
    int k;
/*
    for(k = 0; k < clist.count; k++) {
	if(c = cli.getContact(((icqcontact *) clist.at(k))->getdesc().uin)) {
	    c->setInvisible(false);
	}
    }

    for(i = lst.begin(); i != lst.end(); i++) {
	switch(i->getstatus()) {
	    case csinvisible:
		if(c = cli.getContact(i->getdesc().uin)) {
		    c->setInvisible(true);
		}
		break;
	}
    }
*/
}

void icqhook::exectimers() {
    time_t tcurrent = time(0);

    if(logged()) {
	if(tcurrent-timer_poll > PERIOD_ICQPOLL) {
	    cli.Poll();
	    sendinvisible();
	    time(&timer_poll);
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
    vector<int>::iterator i;
    struct timeval tv;
    int hsock;
    fd_set rs, ws, es;

    FD_ZERO(&rs);
    FD_ZERO(&ws);
    FD_ZERO(&es);

    tv.tv_sec = tv.tv_usec = 0;
    hsock = 0;

    for(i = rfds.begin(); i != rfds.end(); i++) {
	FD_SET(*i, &rs);
	hsock = max(hsock, *i);
    }

    for(i = wfds.begin(); i != wfds.end(); i++) {
	FD_SET(*i, &ws);
	hsock = max(hsock, *i);
    }

    for(i = efds.begin(); i != efds.end(); i++) {
	FD_SET(*i, &es);
	hsock = max(hsock, *i);
    }

    if(select(hsock+1, &rs, &ws, &es, &tv) > 0) {
	for(i = rfds.begin(); i != rfds.end(); i++) {
	    if(FD_ISSET(*i, &rs)) {
		cli.socket_cb(*i, SocketEvent::READ);
		break;
	    }
	}

	for(i = wfds.begin(); i != wfds.end(); i++) {
	    if(FD_ISSET(*i, &ws)) {
		cli.socket_cb(*i, SocketEvent::WRITE);
		break;
	    }
	}

	for(i = efds.begin(); i != efds.end(); i++) {
	    if(FD_ISSET(*i, &es)) {
		cli.socket_cb(*i, SocketEvent::EXCEPTION);
		break;
	    }
	}
    }
}

void icqhook::getsockets(fd_set &rs, fd_set &ws, fd_set &es, int &hsocket) const {
    vector<int>::const_iterator i;

    for(i = rfds.begin(); i != rfds.end(); i++) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &rs);
    }

    for(i = wfds.begin(); i != wfds.end(); i++) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &ws);
    }

    for(i = efds.begin(); i != efds.end(); i++) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &es);
    }
}

bool icqhook::isoursocket(fd_set &rs, fd_set &ws, fd_set &es) const {
    vector<int>::const_iterator i;

    for(i = rfds.begin(); i != rfds.end(); i++)
	if(FD_ISSET(*i, &rs))
	    return true;

    for(i = wfds.begin(); i != wfds.end(); i++)
	if(FD_ISSET(*i, &ws))
	    return true;

    for(i = efds.begin(); i != efds.end(); i++)
	if(FD_ISSET(*i, &es))
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
    Contact *ic = cli.getContact(uin);

    if(!ic) {
	return false;
    }

    if(ev.gettype() == imevent::message) {
	const immessage *m = static_cast<const immessage *> (&ev);
	cli.SendEvent(new NormalMessageEvent(ic, rusconv("kw", m->gettext())));

    } else if(ev.gettype() == imevent::url) {
	const imurl *m = static_cast<const imurl *> (&ev);
	cli.SendEvent(new URLMessageEvent(ic,
	    rusconv("kw", m->getdescription()),
	    rusconv("kw", m->geturl())));

    } else if(ev.gettype() == imevent::sms) {
	const imsms *m = static_cast<const imsms *> (&ev);
	ic->setMobileNo(clist.get(ev.getcontact())->getbasicinfo().cellular);
	cli.SendEvent(new SMSMessageEvent(ic, rusconv("kw", m->gettext()), false));

    } else if(ev.gettype() == imevent::authorization) {
	const imauthorization *m = static_cast<const imauthorization *> (&ev);
	cli.SendEvent(new AuthAckEvent(ic,
	    rusconv("kw", m->gettext()), m->getgranted()));

    } else if(ev.gettype() == imevent::sms) {
	const imsms *m = static_cast<const imsms *> (&ev);
	cli.SendEvent(new SMSMessageEvent(ic, rusconv("kw", m->gettext()),
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
	cli.fetchDetailContactInfo(cli.getContact(c.uin));
    }
}

void icqhook::setautostatus(imstatus st) {
    if(st != offline) {
	if(getstatus() == offline) {
	    connect();
	} else {
	    sendinvisible();
	    logger.putourstatus(icq, getstatus(), st);
	    cli.setStatus(stat2int[st], st == invisible);
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
    int pos;

    if((pos = aserv.find(":")) != -1) {
	cli.setLoginServerHost(aserv.substr(0, pos));
	cli.setLoginServerPort(strtoul(aserv.substr(pos+1).c_str(), 0, 0));
    }

    return true;
}

bool icqhook::regattempt(unsigned int &auin, const string apassword) {
    fd_set rfds, wfds, efds;
    struct timeval tv;
    int hsockfd;
    time_t regtimeout = time(0);

    reguin = 0;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    cli.setPassword(apassword);
    cli.RegisterUIN();

    while(!reguin && (time(0)-regtimeout < 60)) {
	hsockfd = 0;
	getsockets(rfds, wfds, efds, hsockfd);

	tv.tv_sec = 30;
	tv.tv_usec = 0;

	if(isoursocket(rfds, wfds, efds)) {
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
	cli.fetchDetailContactInfo(cli.getContact(c.uin));
    }
}

void icqhook::lookup(const imsearchparams &params, verticalmenu &dest) {
    ICQ2000::Sex sex;

    searchdest = &dest;

    sex = params.gender == genderMale ? ICQ2000::SEX_MALE :
	params.gender == genderFemale ? ICQ2000::SEX_FEMALE :
	ICQ2000::SEX_UNSPECIFIED;

    if(!params.email.empty() || params.minage || params.maxage ||
    !params.city.empty() || !params.state.empty() ||
    !params.company.empty() || !params.department.empty() ||
    !params.position.empty() || params.onlineonly ||
    params.country || params.language || (sex != ICQ2000::SEX_UNSPECIFIED)) {
	searchevent = cli.searchForContacts(params.nick, params.firstname,
	    params.lastname, params.email, params.minage, params.maxage,
	    sex, params.language, params.city, params.state, params.country,
	    params.company, params.department, params.position,
	    params.onlineonly);
    } else {
	searchevent = cli.searchForContacts(params.nick, params.firstname,
	    params.lastname);
    }
}

void icqhook::sendupdateuserinfo(const icqcontact &c) {
}

// ----------------------------------------------------------------------------

void icqhook::connected_cb(ConnectedEvent *ev) {
    flogged = true;

    time(&timer_poll);
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

    rfds.clear();
    wfds.clear();
    efds.clear();

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

    } else if(ev->getType() == MessageEvent::AwayMessage) {
	AwayMessageEvent *r;
	if(r = dynamic_cast<AwayMessageEvent *>(ev)) {
	    em.store(immessage(ic, imevent::incoming,
		string() + _("* Away message:") + "\n\n" +
		rusconv("wk", r->getMessage())));
	}

    }

    return true;
}

void icqhook::messageack_cb(MessageEvent *ev) {
    imcontact ic;
    icqcontact *c;

    ic = imcontact(ev->getContact()->getUIN(), icq);
    c = clist.get(ic);

    if(ev->isFinished()) {
	switch(ev->getType()) {
	    case MessageEvent::SMS:
		if(!ev->isDelivered()) {
		    face.log(_("+ [icq] failed SMS to %s, %s"),
			c->getdispnick().c_str(), ic.totext().c_str());
		}
		break;
	}
    }
}

void icqhook::contactlist_cb(ContactListEvent *ev) {
    icqcontact *c = clist.get(imcontact(ev->getUIN(), icq));
    char buf[64];
    string lastip, sbuf;
    int ip;

    switch(ev->getType()) {
	case ContactListEvent::StatusChange:
	    if(dynamic_cast<StatusChangeEvent*>(ev))
	    if(c) {
		Contact *ic = cli.getContact(ev->getUIN());
		StatusChangeEvent *tev = dynamic_cast<StatusChangeEvent*>(ev);

		logger.putonline(c->getdesc(),
		    c->getstatus(),
		    icq2imstatus(tev->getStatus()));

		c->setstatus(icq2imstatus(tev->getStatus()));

		if(c->getstatus() != offline) {
		    c->setlastseen();

		    if(inet_ntop(AF_INET, &(ip = ntohl(ic->getExtIP())), buf, 64)) {
			lastip = buf;
		    }

		    if(lastip.find_first_not_of(".0") != -1) {
			if(inet_ntop(AF_INET, &(ip = ntohl(ic->getLanIP())), buf, 64)) {
			    sbuf = buf;

			    if(sbuf.find_first_not_of(".0") != -1) {
				if(lastip != sbuf) {
				    if(!lastip.empty()) lastip += " ";
				    lastip += sbuf;
				}
			    }
			}

			c->setlastip(lastip);
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

		MainHomeInfo &home = ic->getMainHomeInfo();
		HomepageInfo &hpage = ic->getHomepageInfo();
		WorkInfo &work = ic->getWorkInfo();
		PersonalInterestInfo &pint = ic->getPersonalInterestInfo();
		BackgroundInfo &backg = ic->getBackgroundInfo();
		EmailInfo &email = ic->getEmailInfo();

		icqcontact::basicinfo cbinfo = c->getbasicinfo();
		icqcontact::moreinfo cminfo = c->getmoreinfo();
		icqcontact::workinfo cwinfo = c->getworkinfo();

		/* basic information */

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
		cbinfo.country = home.country;

		/* more information */

		cminfo.age = hpage.age;

		cminfo.gender =
		    hpage.sex == 1 ? genderFemale :
		    hpage.sex == 2 ? genderMale :
			genderUnspec;

		cminfo.homepage = rusconv("wk", hpage.homepage);
		cminfo.birth_day = hpage.birth_day;
		cminfo.birth_month = hpage.birth_month;
		cminfo.birth_year = hpage.birth_year;

		cminfo.lang1 = hpage.lang1;
		cminfo.lang2 = hpage.lang2;
		cminfo.lang3 = hpage.lang3;

		/* work information */

		cwinfo.city = rusconv("wk", work.city);
		cwinfo.state = rusconv("wk", work.state);
		cwinfo.street = rusconv("wk", work.street);
		cwinfo.company = rusconv("wk", work.company_name);
		cwinfo.dept = rusconv("wk", work.company_dept);
		cwinfo.position = rusconv("wk", work.company_position);
		cwinfo.homepage = rusconv("wk", work.company_web);

		cwinfo.zip = strtoul(work.zip.c_str(), 0, 0);
		cwinfo.country = work.country;

		/* personal interests */

		vector<string> pintinfo;
		list<PersonalInterestInfo::Interest>::iterator ii;

		for(ii = pint.interests.begin(); ii != pint.interests.end(); ii++)
		    pintinfo.push_back(rusconv("wk", ii->second));

		/* education background */

		vector<string> backginfo;
		list<BackgroundInfo::School>::iterator isc;

		for(isc = backg.schools.begin(); isc != backg.schools.end(); isc++)
		    backginfo.push_back(rusconv("wk", isc->second));

		/* nicknames stuff */

		string nick = rusconv("wk", ic->getAlias());

		if((c->getnick() == c->getdispnick())
		|| (c->getdispnick() == i2str(ev->getUIN()))) {
		    c->setdispnick(nick);
		}

		c->setnick(nick);
		c->setbasicinfo(cbinfo);
		c->setmoreinfo(cminfo);
		c->setworkinfo(cwinfo);
		c->setinterests(pintinfo);
		c->setbackground(backginfo);
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

	case ContactListEvent::ServerBasedContact:
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
#if PACKETDEBUG
	    face.log(ev->getMessage());
#endif
	    break;

	default:
	    face.log(ev->getMessage());
	    break;
    }
}

void icqhook::socket_cb(SocketEvent *ev) {
    vector<int>::iterator i;

    if(dynamic_cast<AddSocketHandleEvent*>(ev)) {
	AddSocketHandleEvent *cev = dynamic_cast<AddSocketHandleEvent*>(ev);

	if(cev->isRead()) rfds.push_back(cev->getSocketHandle());
	if(cev->isWrite()) wfds.push_back(cev->getSocketHandle());
	if(cev->isException()) efds.push_back(cev->getSocketHandle());

    } else if(dynamic_cast<RemoveSocketHandleEvent*>(ev)) {
	RemoveSocketHandleEvent *cev = dynamic_cast<RemoveSocketHandleEvent*>(ev);

	i = find(rfds.begin(), rfds.end(), cev->getSocketHandle());
	if(i != rfds.end())
	    rfds.erase(i);

	i = find(wfds.begin(), wfds.end(), cev->getSocketHandle());
	if(i != wfds.end())
	    wfds.erase(i);

	i = find(efds.begin(), efds.end(), cev->getSocketHandle());
	if(i != efds.end())
	    efds.erase(i);
    }
}

void icqhook::statuschanged_cb(MyStatusChangeEvent *ev) {
    face.update();
}

void icqhook::want_auto_resp_cb(AwayMessageEvent *ev) {
    ev->setMessage("");
}

void icqhook::search_result_cb(SearchResultEvent *ev) {
    if(ev == searchevent) {
	string line;
	Contact *c = ev->getLastContactAdded();

	if(searchdest && c) {
	    line = (c->getStatus() == STATUS_ONLINE ? "o " : "  ") +
		c->getAlias();

	    if(line.size() > 12) line.resize(12);
	    else line += string(12-line.size(), ' ');

	    line += " " + c->getFirstName() + " " + c->getLastName();
	    if(!c->getEmail().empty()) line += " <" + c->getEmail() + ">";

	    searchdest->additem(0, c->getUIN(), line);

	    searchdest->redraw();
	}

	if(ev->isFinished()) {
	    face.log(_("+ [icq] whitepages search finished, %d found"),
		ev->getContactList().size());
	}
    }
}
