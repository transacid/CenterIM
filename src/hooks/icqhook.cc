/*
*
* centericq icq protocol handling class
* $Id: icqhook.cc,v 1.159 2004/12/20 00:54:02 konst Exp $
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

#include "icqhook.h"
#include "icqconf.h"
#include "icqface.h"
#include "imlogger.h"
#include "icqgroups.h"

#include "accountmanager.h"
#include "eventmanager.h"

#include "src/Capabilities.h"

#define PERIOD_ICQPOLL  5
#define PERIOD_RESOLVE  10
#define DELAY_SENDNEW   5

#include <netinet/in.h>
#include <arpa/inet.h>

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

icqhook::icqhook(): abstracthook(icq) {
    fonline = false;
    blockmode = Normal;

    fcapabs.insert(hookcapab::urls);
    fcapabs.insert(hookcapab::setaway);
    fcapabs.insert(hookcapab::fetchaway);
    fcapabs.insert(hookcapab::changenick);
//    fcapabs.insert(hookcapab::changepassword);
    fcapabs.insert(hookcapab::changedetails);
    fcapabs.insert(hookcapab::authrequests);
    fcapabs.insert(hookcapab::authreqwithmessages);
    fcapabs.insert(hookcapab::contacts);
    fcapabs.insert(hookcapab::visibility);
    fcapabs.insert(hookcapab::cltemporary);
    fcapabs.insert(hookcapab::changeabout);

    cli.connected.connect(this, &icqhook::connected_cb);
    cli.disconnected.connect(this, &icqhook::disconnected_cb);
    cli.socket.connect(this, &icqhook::socket_cb);
    cli.messaged.connect(this, &icqhook::messaged_cb);
    cli.messageack.connect(this, &icqhook::messageack_cb);
    cli.contactlist.connect(this, &icqhook::contactlist_cb);
    cli.contact_userinfo_change_signal.connect(this, &icqhook::contact_userinfo_change_signal_cb);
    cli.contact_status_change_signal.connect(this, &icqhook::contact_status_change_signal_cb);
    cli.newuin.connect(this, &icqhook::newuin_cb);
    cli.rate.connect(this, &icqhook::rate_cb);
//    cli.password_changed.connect(this, &icqhook::password_changed_cb);
    cli.want_auto_resp.connect(this, &icqhook::want_auto_resp_cb);
    cli.search_result.connect(this, &icqhook::search_result_cb);
    cli.self_contact_userinfo_change_signal.connect(this, &icqhook::self_contact_userinfo_change_cb);
    cli.self_contact_status_change_signal.connect(this, &icqhook::self_contact_status_change_cb);
    cli.sbl_received.connect(this, &icqhook::sbl_received_cb);
}

icqhook::~icqhook() {
    cli.logger.disconnect(this);
    cli.connected.disconnect(this);
    cli.disconnected.disconnect(this);
    cli.socket.disconnect(this);
    cli.messaged.disconnect(this);
    cli.messageack.disconnect(this);
    cli.contactlist.disconnect(this);
    cli.contact_userinfo_change_signal.disconnect(this);
    cli.contact_status_change_signal.disconnect(this);
    cli.newuin.disconnect(this);
    cli.rate.disconnect(this);
    cli.search_result.disconnect(this);
    cli.want_auto_resp.disconnect(this);
    cli.self_contact_userinfo_change_signal.disconnect(this);
    cli.self_contact_status_change_signal.disconnect(this);
//    cli.password_changed.disconnect(this);
}

void icqhook::init() {
    manualstatus = conf.getstatus(proto);

    if(conf.getdebug())
	cli.logger.connect(this, &icqhook::logger_cb);
}

void icqhook::connect() {
    icqconf::imaccount acc = conf.getourid(proto);
    int i, ptpmin, ptpmax;
    icqcontact *c;

    if(acc.additional["webaware"].empty()) acc.additional["webaware"] = "1";
    conf.setourid(acc);

    if(!acc.server.empty()) cli.setLoginServerHost(acc.server);
    if(acc.port) cli.setLoginServerPort(acc.port);

    if(!conf.getsmtphost().empty()) cli.setSMTPServerHost(conf.getsmtphost());
    if(conf.getsmtpport()) cli.setSMTPServerPort(conf.getsmtpport());

    if(!conf.getbindhost().empty()) cli.setClientBindHost(conf.getbindhost());

    conf.getpeertopeer(ptpmin, ptpmax);
    if(ptpmax) {
	cli.setAcceptInDC(true);
	cli.setPortRangeLowerBound(ptpmin);
	cli.setPortRangeUpperBound(ptpmax);
	cli.setUsePortRange(true);
    } else {
	cli.setAcceptInDC(false);
    }

    log(logConnecting);

    cli.self_contact_userinfo_change_signal.disconnect(this);

    cli.setUIN(acc.uin);
    cli.setPassword(acc.password);
    cli.setWebAware(acc.additional["webaware"] == "1");

    cli.setStatus(stat2int[manualstatus], manualstatus == invisible);
    cli.self_contact_userinfo_change_signal.connect(this, &icqhook::self_contact_userinfo_change_cb);

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == proto && c->getdesc().uin) {
	    icqcontact::basicinfo bi = c->getbasicinfo();
	    bi.authawait = true;
	    c->setbasicinfo(bi);

	    addContact(c->getdesc().uin, groups.getname(c->getgroupid()));
	}
    }

    sendinvisible();

    fonline = true;
    flogged = false;
}

void icqhook::disconnect() {
    cli.setStatus(STATUS_OFFLINE);
    fonline = flogged = false;
}

void icqhook::resolve() {
    icqcontact *c;
    vector<icqcontact *> toresolve;

    for(int i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == proto)
	if(c->getdispnick() == i2str(c->getdesc().uin))
	    toresolve.push_back(c);
    }

    if(toresolve.size()) {
	c = toresolve[randlimit(0, toresolve.size()-1)];
	requestinfo(c);
    }
}

void icqhook::sendinvisible() {
    vector<visInfo> todo, nvislist;
    vector<visInfo>::iterator iv, it;
    icqlist::iterator i;

    for(i = lst.begin(); i != lst.end(); ++i) {
	if(i->getdesc().pname == proto) {
	    switch(i->getstatus()) {
		case csvisible:
		case csinvisible:
		    todo.push_back(pair<unsigned int, contactstatus>
			(i->getdesc().uin, i->getstatus()));
		    break;
	    }
	}
    }

    nvislist = todo;

    for(iv = vislist.begin(); iv != vislist.end(); ++iv) {
	it = find(todo.begin(), todo.end(), *iv);

	if(it != todo.end()) {
	    todo.erase(it);
	} else {
	    switch(iv->second) {
		case csvisible: cli.removeVisible(iv->first); break;
		case csinvisible: cli.removeInvisible(iv->first); break;
	    }
	}
    }

    for(it = todo.begin(); it != todo.end(); ++it) {
	switch(it->second) {
	    case csvisible: cli.addVisible(new Contact(it->first)); break;
	    case csinvisible: cli.addInvisible(new Contact(it->first)); break;
	}
    }

    vislist = nvislist;
}

void icqhook::exectimers() {
    if(logged()) {
	if(timer_current-timer_poll > PERIOD_ICQPOLL) {
	    cli.Poll();
	    sendinvisible();
	    timer_poll = timer_current;

	    /*
	    *
	    * Will to the mass-adding stuff here too..
	    *
	    */

	    vector<imcontact>::iterator ic = uinstosend.begin();

	    if(ic != uinstosend.end()) {
		imcontact imc = *ic;
		uinstosend.erase(ic);

		ContactRef icont = cli.getContact(imc.uin);
		if(!icont.get()) sendnewuser(imc);
	    }
	}

	if(timer_current-timer_resolve > PERIOD_RESOLVE) {
	    resolve();
	    timer_resolve = timer_current;
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

    for(i = rfds.begin(); i != rfds.end(); ++i) {
	FD_SET(*i, &rs);
	hsock = max(hsock, *i);
    }

    for(i = wfds.begin(); i != wfds.end(); ++i) {
	FD_SET(*i, &ws);
	hsock = max(hsock, *i);
    }

    for(i = efds.begin(); i != efds.end(); ++i) {
	FD_SET(*i, &es);
	hsock = max(hsock, *i);
    }

    if(select(hsock+1, &rs, &ws, &es, &tv) > 0) {
	for(i = rfds.begin(); i != rfds.end(); ++i) {
	    if(FD_ISSET(*i, &rs)) {
		cli.socket_cb(*i, SocketEvent::READ);
		break;
	    }
	}

	for(i = wfds.begin(); i != wfds.end(); ++i) {
	    if(FD_ISSET(*i, &ws)) {
		cli.socket_cb(*i, SocketEvent::WRITE);
		break;
	    }
	}

	for(i = efds.begin(); i != efds.end(); ++i) {
	    if(FD_ISSET(*i, &es)) {
		cli.socket_cb(*i, SocketEvent::EXCEPTION);
		break;
	    }
	}
    }
}

void icqhook::getsockets(fd_set &rs, fd_set &ws, fd_set &es, int &hsocket) const {
    vector<int>::const_iterator i;

    for(i = rfds.begin(); i != rfds.end(); ++i) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &rs);
    }

    for(i = wfds.begin(); i != wfds.end(); ++i) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &ws);
    }

    for(i = efds.begin(); i != efds.end(); ++i) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &es);
    }
}

bool icqhook::isoursocket(fd_set &rs, fd_set &ws, fd_set &es) const {
    vector<int>::const_iterator i;

    for(i = rfds.begin(); i != rfds.end(); ++i)
	if(FD_ISSET(*i, &rs))
	    return true;

    for(i = wfds.begin(); i != wfds.end(); ++i)
	if(FD_ISSET(*i, &ws))
	    return true;

    for(i = efds.begin(); i != efds.end(); ++i)
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
    ContactRef ic = cli.getContact(uin);
    MessageEvent *sev = 0;
    ICQMessageEvent *iev;
    icqcontact *c;

    if(ev.getcontact().pname != proto)
	uin = 0;

    if(!uin)
	ic = cli.getSelfContact();

    if(!ic.get())
	return false;

    if(ev.gettype() == imevent::message) {
	const immessage *m = static_cast<const immessage *> (&ev);
	string text = m->gettext(), sub;

	if(ic->getStatus() == STATUS_OFFLINE)
	if(text.size() > 450) {
	    while(!text.empty()) {
		if(text.size() > 450) {
		    sub = text.substr(0, 450);
		    text.erase(0, 450);
		} else {
		    sub = text;
		    text = "";
		}

		cli.SendEvent(new NormalMessageEvent(ic, ruscrlfconv("kw", sub)));
	    }

	    return true;
	}

	sev = new NormalMessageEvent(ic, ruscrlfconv("kw", text));

    } else if(ev.gettype() == imevent::url) {
	const imurl *m = static_cast<const imurl *> (&ev);
	sev = new URLMessageEvent(ic,
	    ruscrlfconv("kw", m->getdescription()),
	    ruscrlfconv("kw", m->geturl()));

    } else if(ev.gettype() == imevent::sms) {
	const imsms *m = static_cast<const imsms *> (&ev);

	if(!uin) {
	    if(m->getphone().empty()) {
		if(ic->getMainHomeInfo().getMobileNo().empty()) {
		    cli.fetchSelfDetailContactInfo();
		    return false;
		}
	    } else {
		ic = new Contact(m->getphone());
		ic->setMobileNo(m->getphone());

		ContactTree &ct = cli.getContactTree();
		ContactTree::Group &g = ct.group_size() ? *ct.begin() : ct.add_group("New");
		g.add(ic);
	    }

	} else {
	    cli.contact_userinfo_change_signal.disconnect(this);
	    ic->setMobileNo(clist.get(ev.getcontact())->getbasicinfo().cellular);
	    cli.contact_userinfo_change_signal.connect(this, &icqhook::contact_userinfo_change_signal_cb);
	}

	sev = new SMSMessageEvent(ic, rusconv("ku", m->getmessage()), true);

    } else if(ev.gettype() == imevent::authorization) {
	const imauthorization *m = static_cast<const imauthorization *> (&ev);

	switch(m->getauthtype()) {
	    case imauthorization::Granted:
	    case imauthorization::Rejected:
		sev = new AuthAckEvent(ic, ruscrlfconv("kw", m->getmessage()),
		    m->getauthtype() == imauthorization::Granted);
		break;

	    case imauthorization::Request:
		sev = new AuthReqEvent(ic, ruscrlfconv("kw", m->getmessage()));
		if(c = clist.get(ev.getcontact())) {
		    icqcontact::basicinfo bi = c->getbasicinfo();
		    bi.authawait = true;
		    c->setbasicinfo(bi);
		}
		break;
	}

    } else if(ev.gettype() == imevent::contacts) {
	const imcontacts *m = static_cast<const imcontacts *> (&ev);
	vector< pair<unsigned int, string> >::const_iterator icc;
	list<ContactRef> slst;

	for(icc = m->getcontacts().begin(); icc != m->getcontacts().end(); ++icc) {
	    ContactRef ct(new Contact(icc->first));
	    ct->setAlias(ruscrlfconv("kw", icc->second));
	    slst.push_back(ct);
	}

	sev = new ContactMessageEvent(ic, slst);

    } else {
	return false;
    }

    if(sev) {
	if(ic->getStatus() == STATUS_DND || ic->getStatus() == STATUS_OCCUPIED)
	if(iev = dynamic_cast<ICQMessageEvent *>(sev))
	    iev->setUrgent(true);

	cli.SendEvent(sev);
    }

    return true;
}

void icqhook::sendnewuser(const imcontact &ic) {
    static time_t lastadd = 0;
    icqcontact *cc = clist.get(ic);

    if(logged() && ic.uin && cc) {
	if(timer_current-lastadd > DELAY_SENDNEW) {
	    ContactRef cont = addContact(ic.uin, groups.getname(cc->getgroupid()));
	    cli.fetchSimpleContactInfo(cont);
	    cli.fetchDetailContactInfo(cont);

	    if(sblrecv && cc->inlist()) {
		ContactTree::Group &g = cli.getContactTree().lookup_group_containing_contact(cont);

		cont->setServerSideInfo(g.get_id(), 0);
		cont->setServerBased(true);
		cont->setAuthAwait(cc->getbasicinfo().authawait);

		cli.uploadServerBasedContact(cont);
	    }

	    lastadd = timer_current;

	} else {
	    uinstosend.push_back(ic);

	}
    }
}

void icqhook::removeuser(const imcontact &c) {
    ContactRef ic = cli.getContact(c.uin);

    if(ic.get()) {
	icqcontact *cc = clist.get(c);

	if(cc)
	if(cc->inlist()) {
	    if(ic->getServerBased()) {
		ic->setAlias(cc->getnick());
		ic->setAuthAwait(cc->getbasicinfo().authawait);
		cli.removeServerBasedContact(ic);
	    }
	}

	ContactTree& ct = cli.getContactTree();
	ContactTree::Group &gr = ct.lookup_group_containing_contact(ic);
	gr.remove(c.uin);
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

bool icqhook::regconnect(const string &aserv) {
    int pos;

    if((pos = aserv.find(":")) != -1) {
	cli.setLoginServerHost(aserv.substr(0, pos));
	cli.setLoginServerPort(strtoul(aserv.substr(pos+1).c_str(), 0, 0));
    }

    return true;
}

bool icqhook::regattempt(unsigned int &auin, const string &apassword, const string &email) {
    fd_set srfds, swfds, sefds;
    struct timeval tv;
    int hsockfd;
    time_t regtimeout = time(0);

    reguin = 0;

    cli.setPassword(apassword);
    cli.RegisterUIN();

    while(!reguin && (time(0)-regtimeout < 60)) {
	hsockfd = 0;

	FD_ZERO(&srfds);
	FD_ZERO(&swfds);
	FD_ZERO(&sefds);

	getsockets(srfds, swfds, sefds, hsockfd);

	tv.tv_sec = 30;
	tv.tv_usec = 0;

	select(hsockfd+1, &srfds, &swfds, &sefds, &tv);

	if(isoursocket(srfds, swfds, sefds)) {
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

void icqhook::requestinfo(const imcontact &c) {
    if(logged() && c.uin) {
	if(c == imcontact(conf.getourid(icq).uin, icq)) {
	    // Our info is requested
	    cli.fetchSelfDetailContactInfo();
	} else {
	    ContactRef icont = cli.getContact(c.uin);

	    if(!icont.get()) {
		addContact(c.uin, "");
	    } else {
		cli.fetchSimpleContactInfo(icont);
		cli.fetchDetailContactInfo(icont);
	    }
	}
    }
}

void icqhook::lookup(const imsearchparams &params, verticalmenu &dest) {
    ICQ2000::Sex sex;

    while(!foundguys.empty()) {
	delete foundguys.back();
	foundguys.pop_back();
    }

    searchdest = &dest;

    sex = params.gender == genderMale ? ICQ2000::SEX_MALE :
	params.gender == genderFemale ? ICQ2000::SEX_FEMALE :
	ICQ2000::SEX_UNKNOWN;

    if(params.uin) {
	searchevent = cli.searchForContacts(params.uin);

    } else if(!params.kwords.empty()) {
	searchevent = cli.searchForContacts(rusconv("kw", params.kwords));

    } else if(params.randomgroup) {
	searchevent = cli.searchForContacts((ICQ2000::RandomChatGroup) params.randomgroup);

    } else if(!params.email.empty() || params.minage || params.maxage ||
    !params.city.empty() || !params.state.empty() ||
    !params.company.empty() || !params.department.empty() ||
    !params.position.empty() || params.onlineonly ||
    params.country || params.language ||
    (sex != SEX_UNKNOWN) || (params.agerange != RANGE_NORANGE)) {
	searchevent = cli.searchForContacts(rusconv("kw", params.nick),
	    rusconv("kw", params.firstname), rusconv("kw", params.lastname),
	    rusconv("kw", params.email), params.agerange, sex,
	    params.language, rusconv("kw", params.city),
	    rusconv("kw", params.state), params.country,
	    rusconv("kw", params.company), rusconv("kw", params.department),
	    rusconv("kw", params.position), params.onlineonly);

    } else {
	searchevent = cli.searchForContacts(rusconv("kw", params.nick),
	    rusconv("kw", params.firstname), rusconv("kw", params.lastname));

    }
}

void icqhook::sendupdateuserinfo(const icqcontact &c) {
    ContactRef ic = cli.getSelfContact();

    cli.self_contact_userinfo_change_signal.disconnect(this);

    Contact::MainHomeInfo &home = ic->getMainHomeInfo();
    Contact::HomepageInfo &hpage = ic->getHomepageInfo();
    Contact::WorkInfo &work = ic->getWorkInfo();

    const icqcontact::basicinfo &cbinfo = c.getbasicinfo();
    const icqcontact::moreinfo &cminfo = c.getmoreinfo();
    const icqcontact::workinfo &cwinfo = c.getworkinfo();

    /* basic information */

    ic->setAlias(rusconv("kw", c.getnick()));
    ic->setFirstName(rusconv("kw", cbinfo.fname));
    ic->setLastName(rusconv("kw", cbinfo.lname));
    ic->setEmail(rusconv("kw", cbinfo.email));
    ic->setAuthReq(cbinfo.requiresauth);

    ic->setAboutInfo(rusconv("kw", c.getabout()));

    home.city = rusconv("kw", cbinfo.city);
    home.state = rusconv("kw", cbinfo.state);
    home.phone = rusconv("kw", cbinfo.phone);
    home.fax = rusconv("kw", cbinfo.fax);
    home.street = rusconv("kw", cbinfo.street);
    home.setMobileNo(rusconv("kw", cbinfo.cellular));

    home.zip = cbinfo.zip;
    home.country = (Country) cbinfo.country;
    home.timezone = (Timezone) getSystemTimezone();

    /* more information */

    hpage.age = cminfo.age;

    hpage.sex =
	cminfo.gender == genderFemale ? SEX_FEMALE :
	cminfo.gender == genderMale ? SEX_MALE :
	    SEX_UNKNOWN;

    hpage.homepage = rusconv("kw", cminfo.homepage);
    hpage.birth_day = cminfo.birth_day;
    hpage.birth_month = cminfo.birth_month;
    hpage.birth_year = cminfo.birth_year;

    hpage.lang1 = (Language) cminfo.lang1;
    hpage.lang2 = (Language) cminfo.lang2;
    hpage.lang3 = (Language) cminfo.lang3;

    /* work information */

    work.city = rusconv("kw", cwinfo.city);
    work.state = rusconv("kw", cwinfo.state);
    work.street = rusconv("kw", cwinfo.street);
    work.company_name = rusconv("kw", cwinfo.company);
    work.company_dept = rusconv("kw", cwinfo.dept);
    work.company_position = rusconv("kw", cwinfo.position);
    work.company_web = rusconv("kw", cwinfo.homepage);

    work.zip = cwinfo.zip;
    work.country = cwinfo.country;

    ic->setMainHomeInfo(home);
    ic->setHomepageInfo(hpage);
    ic->setWorkInfo(work);
    ic->setAuthReq(cbinfo.requiresauth);

    icqconf::imaccount acc = conf.getourid(icq);
    acc.additional["webaware"] = cbinfo.webaware ? "1" : "0";
    acc.additional["randomgroup"] = i2str(cbinfo.randomgroup);
    conf.setourid(acc);

    cli.setWebAware(cbinfo.webaware);
    cli.setRandomChatGroup(cbinfo.randomgroup);

    cli.uploadSelfDetails();
    cli.self_contact_userinfo_change_signal.connect(this, &icqhook::self_contact_userinfo_change_cb);
/*
    if(!c.getreginfo().password.empty())
	cli.changePassword(c.getreginfo().password);*/
}

void icqhook::requestawaymsg(const imcontact &c) {
    ContactRef ic = cli.getContact(c.uin);

    if(ic.get()) {
	cli.SendEvent(new AwayMessageEvent(ic));
    }
}

void icqhook::updateinforecord(ContactRef ic, icqcontact *c) {
    string sbuf;
    vector<string> pintinfo, backginfo;
    list<Contact::PersonalInterestInfo::Interest>::iterator ii;
    list<Contact::BackgroundInfo::School>::iterator isc;
    icqcontact::basicinfo cbinfo;
    icqcontact::moreinfo cminfo;
    icqcontact::workinfo cwinfo;

    if(ic.get() && c) {
	Contact::MainHomeInfo &home = ic->getMainHomeInfo();
	Contact::HomepageInfo &hpage = ic->getHomepageInfo();
	Contact::WorkInfo &work = ic->getWorkInfo();
	Contact::PersonalInterestInfo &pint = ic->getPersonalInterestInfo();
	Contact::BackgroundInfo &backg = ic->getBackgroundInfo();
	Contact::EmailInfo &email = ic->getEmailInfo();

	cbinfo = c->getbasicinfo();
	cminfo = c->getmoreinfo();
	cwinfo = c->getworkinfo();

	/* basic information */

	cbinfo.fname = rusconv("wk", ic->getFirstName());
	cbinfo.lname = rusconv("wk", ic->getLastName());
	if(!ic->getEmail().empty()) cbinfo.email = rusconv("wk", ic->getEmail());
	cbinfo.city = rusconv("wk", home.city);
	cbinfo.state = rusconv("wk", home.state);
	cbinfo.phone = rusconv("wk", home.phone);
	cbinfo.fax = rusconv("wk", home.fax);
	cbinfo.street = rusconv("wk", home.street);
	cbinfo.requiresauth = ic->getAuthReq();

	if(!home.getMobileNo().empty())
	    cbinfo.cellular = rusconv("wk", home.getMobileNo());

	cbinfo.zip = home.zip;
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

	cminfo.timezone = home.timezone;

	/* work information */

	cwinfo.city = rusconv("wk", work.city);
	cwinfo.state = rusconv("wk", work.state);
	cwinfo.street = rusconv("wk", work.street);
	cwinfo.company = rusconv("wk", work.company_name);
	cwinfo.dept = rusconv("wk", work.company_dept);
	cwinfo.position = rusconv("wk", work.company_position);
	cwinfo.homepage = rusconv("wk", work.company_web);

	cwinfo.zip = work.zip;
	cwinfo.country = work.country;

	/* personal interests */

	for(ii = pint.interests.begin(); ii != pint.interests.end(); ++ii) {
	    sbuf = getInterestsIDtoString(ii->first);

	    if(!ii->second.empty()) {
		if(!sbuf.empty()) sbuf += ": ";
		sbuf += rusconv("wk", ii->second);
	    }

	    if(!sbuf.empty()) pintinfo.push_back(sbuf);
	}

	/* education background */

	for(isc = backg.schools.begin(); isc != backg.schools.end(); ++isc) {
	    sbuf = getBackgroundIDtoString(isc->first);
	    if(!sbuf.empty()) sbuf += ": ";
	    sbuf += rusconv("wk", isc->second);
	    if(!sbuf.empty()) backginfo.push_back(sbuf);
	}

	/* nicknames stuff */

	string nick = rusconv("wk", ic->getAlias());

	if((c->getnick() == c->getdispnick())
	|| (c->getdispnick() == i2str(c->getdesc().uin))) {
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
}

void icqhook::processemailevent(const string &sender, const string &email, const string &message) {
    if(email.empty() && sender.empty() && message.empty())
	return;

    icqcontact *c = clist.getemail(email);

    if(!c)
    if(c = clist.addnew(imcontact(0, infocard), true)) {
	c->setdispnick(email);
	c->setnick(email);

	icqcontact::basicinfo b = c->getbasicinfo();

	if(sender != email) {
	    int pos = sender.find(" ");

	    if(pos != -1) {
		b.fname = sender.substr(0, pos);
		b.lname = sender.substr(pos+1);
	    } else {
		b.fname = sender;
	    }
	}

	b.email = email;
	c->setbasicinfo(b);
    }

    if(c) {
	em.store(imemail(c, imevent::incoming,
	    rusconv("wk", message)));
    }
}

void icqhook::updatecontact(icqcontact *c) {
    if(sblrecv) {
	ContactTree &tree = cli.getContactTree();
	ContactRef ic = tree[c->getdesc().uin];

	if(ic.get()) {
	    string gname = groups.getname(c->getgroupid());
	    ContactTree::Group &oldg = tree.lookup_group_containing_contact(ic);
	    ContactTree::Group *newg = 0;

	    if(oldg.get_label() != gname) {
		ContactTree::iterator curr = tree.begin();
		while(curr != tree.end()) {
		    if((*curr).get_label() == gname) {
			newg = &(*curr);
			break;
		    }
		    ++curr;
		}

		cli.removeServerBasedContact(ic);

		if(!newg) newg = &(tree.add_group(gname));
		tree.relocate_contact(ic, oldg, *newg);

		ic->setServerSideInfo(newg->get_id(), ic->getServerSideID());
		ic->setAuthAwait(c->getbasicinfo().authawait);

		cli.uploadServerBasedContact(ic);
	    }
	}
    }
}

void icqhook::renamegroup(const string &oldname, const string &newname) {
    if(logged()) {
/*
	ContactTree::Group *gp = 0;
	ContactTree &ct = cli.getContactTree();

	ContactTree::iterator curr = ct.begin();
	while(curr != ct.end()) {
	    if((*curr).get_label() == oldname) {
		gp = &(*curr);
		break;
	    }
	    ++curr;
	}

	if(gp) {
	    gp->set_label(newname);
	    cli.uploadServerBasedGroup(*gp);
	}
*/
    }
}

// ----------------------------------------------------------------------------

void icqhook::connected_cb(ConnectedEvent *ev) {
    flogged = true;

    timer_poll = timer_current;
    timer_resolve = timer_poll-PERIOD_RESOLVE+3;

    logger.putourstatus(icq, offline, manualstatus);

    cli.setRandomChatGroup(strtoul(conf.getourid(icq).additional["randomgroup"].c_str(), 0, 0));

    log(logLogged);
    face.update();

    string buf;
    ifstream f(conf.getconfigfname("icq-infoset").c_str());

    if(f.is_open()) {
	getstring(f, buf); cli.getSelfContact()->setAlias(buf);
	getstring(f, buf); cli.getSelfContact()->setEmail(buf);
	getstring(f, buf); cli.getSelfContact()->setFirstName(buf);
	getstring(f, buf); cli.getSelfContact()->setLastName(buf);
	f.close();

	cli.getSelfContact()->getMainHomeInfo().timezone = (Timezone) getSystemTimezone();
	cli.uploadSelfDetails();

	unlink(conf.getconfigfname("icq-infoset").c_str());
    }

    cli.fetchServerBasedContactList();
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
	_("dual login detected"),
	""
    };

    rfds.clear();
    wfds.clear();
    efds.clear();

    msg = isconnecting() ? _("+ [icq] cannot connect") : _("+ [icq] disconnected");

    logger.putourstatus(icq, getstatus(), offline);

    clist.setoffline(icq);
    fonline = false;

    if(!reasons[ev->getReason()].empty()) {
	msg += ": ";
	msg += reasons[ev->getReason()];

	logger.putmessage((string) _("icq disconnection reason") + ": " +
	    reasons[ev->getReason()]);

	if(ev->getReason() == DisconnectedEvent::FAILED_DUALLOGIN)
	    manualstatus = offline;
    }

    face.log(msg);
    face.update();
}

static string fixicqrtf(string msg) {
    int pos, bpos, n;
    string sub;

    static char *emoticons[] = {
	":-)", ":-O", ":-|", ":-\\", ":-(", ":-*", "8-/", ":~(",
	";-)", ">:-O", ":`(", ":-X", ":-P", "B-)", "O:-)", ":-D",
	"*ANNOYED*", "*DISGUSTED*", "*DROOLING*", "*GIGGLING*",
	"*JOKINGLY*", "*SHOCKED*", "*WHINNING*", "*SURPRISED*",
	"*SURPRISED*", "*IN LOVE*"
    };

    for(pos = 0; (pos = msg.find("<##icqimage", pos)) != -1; ) {
	if((bpos = msg.find(">", pos)) != -1) {
	    sub = msg.substr(pos, bpos-pos);

	    if(sub.size() > 2) sub.erase(0, sub.size()-2);
	    n = hex2int(sub);

	    if(n >= 0 && n <= 25) {
		msg.erase(pos, bpos-pos+1);
		msg.insert(pos, emoticons[n]);
	    } else {
		pos++;
	    }
	}
    }

    return msg;
}

void icqhook::messaged_cb(MessageEvent *ev) {
    imcontact ic;
    time_t t;
    string text;

    ic = imcontact(ev->getContact()->getUIN(), icq);

    if(ev->getType() == MessageEvent::Normal) {
	NormalMessageEvent *r = static_cast<NormalMessageEvent *>(ev);

	text = r->getMessage();
	if(text.substr(0, 6) == "{\\rtf1")
	    text = fixicqrtf(striprtf(text, conf.getconvertfrom(proto)));

	em.store(immessage(ic, imevent::incoming, rusconv("wk", text), r->getTime()));

    } else if(ev->getType() == MessageEvent::URL) {
	URLMessageEvent *r = static_cast<URLMessageEvent *>(ev);
	em.store(imurl(ic, imevent::incoming,
	    rusconv("wk", r->getURL()),
	    rusconv("wk", r->getMessage())));

    } else if(ev->getType() == MessageEvent::SMS) {
	SMSMessageEvent *r = static_cast<SMSMessageEvent *>(ev);
	icqcontact *c = clist.getmobile(r->getSender());

	if(!c)
	if(c = clist.addnew(imcontact(0, infocard), true)) {
	    icqcontact::basicinfo b = c->getbasicinfo();
	    b.cellular = r->getSender();
	    c->setbasicinfo(b);

	    c->setdispnick(b.cellular);
	    c->setnick(b.cellular);
	}

	if(c) {
	    em.store(imsms(c, imevent::incoming, rusconv("uk", r->getMessage())));
	}

    } else if(ev->getType() == MessageEvent::AuthReq) {
	AuthReqEvent *r = static_cast<AuthReqEvent *>(ev);
	em.store(imauthorization(ic, imevent::incoming, imauthorization::Request,
	    rusconv("wk", r->getMessage())));

    } else if(ev->getType() == MessageEvent::AuthAck) {
	AuthAckEvent *r = static_cast<AuthAckEvent *>(ev);
	icqcontact *c = clist.get(ic);

	if(c) {
	    icqcontact::basicinfo bi = c->getbasicinfo();
	    bi.authawait = false;
	    c->setbasicinfo(bi);

	    ContactRef cr = cli.getContactTree()[ic.uin];
	    if(cr.get()) {
		cr->setAuthAwait(false);
		cli.removeServerBasedContact(cr);
		cli.uploadServerBasedContact(cr);
	    }
	}

	if(r->isGranted()) {
	    em.store(imnotification(ic, _("The user has accepted your authorization request")));

	} else {
	    em.store(imnotification(ic, (string)
		_("The user has rejected your authorization request; the message was: ") + r->getMessage()));

	}

    } else if(ev->getType() == MessageEvent::EmailEx) {
	EmailExEvent *r = static_cast<EmailExEvent *>(ev);
	processemailevent(r->getSender(), r->getEmail(), r->getMessage());

    } else if(ev->getType() == MessageEvent::WebPager) {
	WebPagerEvent *r = static_cast<WebPagerEvent *>(ev);
	processemailevent(r->getSender(), r->getEmail(), r->getMessage());

    } else if(ev->getType() == MessageEvent::UserAdd) {
	UserAddEvent *r = static_cast<UserAddEvent *>(ev);
	em.store(imnotification(ic,
	    _("The user has added you to his/her contact list")));

    } else if(ev->getType() == MessageEvent::Contacts) {
	ContactMessageEvent *r = static_cast<ContactMessageEvent *>(ev);
	vector< pair<unsigned int, string> > lst;

	list<ContactRef> clst = r->getContacts();
	list<ContactRef>::iterator il;

	for(il = clst.begin(); il != clst.end(); ++il) {
	    lst.push_back(make_pair((*il)->getUIN(), (*il)->getAlias()));
	}

	em.store(imcontacts(ic, imevent::incoming, lst));

    }

    ev->setDelivered(true);
}

void icqhook::messageack_cb(MessageEvent *ev) {
    imcontact ic;
    icqcontact *c;
    ICQMessageEvent *me;

    ic = imcontact(ev->getContact()->getUIN(), icq);
    c = clist.get(ic);

    if(!c) {
	c = clist.get(contactroot);
	c->setdispnick(cli.getSelfContact()->getAlias());
    }

    if(ev->isFinished()) {
	// The FINAL decision.

	switch(ev->getType()) {
	    case MessageEvent::SMS:
		if(!ev->isDelivered()) {
		    if(ev->getContact()->getUIN()) {
			face.log(_("+ [icq] failed SMS to %s, %s"),
			    c->getdispnick().c_str(), ic.totext().c_str());
		    } else {
			face.log(_("+ [icq] failed SMS to %s"),
			    ev->getContact()->getAlias().c_str());
		    }
		}
		break;

	    case MessageEvent::AwayMessage:
		if(ev->isDelivered()) {
		    AwayMessageEvent *r;

		    if(r = dynamic_cast<AwayMessageEvent *>(ev)) {
			em.store(imnotification(ic, string() + _("Away message:") + "\n\n" +
			    rusconv("wk", r->getAwayMessage())));
		    }
		} else {
		    face.log(_("+ [icq] cannot fetch away msg from %s, %s"),
			c->getdispnick().c_str(), ic.totext().c_str());
		}
		break;
	}
    } else {
	/*
	*
	* Rejected, but there is still something to be done.
	*
	*/
    }
}

void icqhook::contactlist_cb(ContactListEvent *ev) {
}

void icqhook::contact_status_change_signal_cb(StatusChangeEvent *ev) {
    icqcontact *c = clist.get(imcontact(ev->getUIN(), icq));

    if(c) {
	ContactRef ic = ev->getContact();
	StatusChangeEvent *tev = dynamic_cast<StatusChangeEvent*>(ev);
	imstatus nst = icq2imstatus(tev->getStatus());

	if(ic->isInvisible()) nst = invisible;

	logger.putonline(c, c->getstatus(), nst);
	c->setstatus(nst);
    }
}

void icqhook::contact_userinfo_change_signal_cb(UserInfoChangeEvent *ev) {
    icqcontact *c = clist.get(imcontact(ev->getUIN(), icq));
    ContactRef ic = ev->getContact();

    if(!c) {
	c = clist.get(contactroot);
	/*
	*
	* In case we wanna look up info about a user which
	* is not on our contact list.
	*
	*/
    }

    if(!ev->isTransientDetail()) {
	updateinforecord(ic, c);

    } else {
	char *p;
	string lastip, sbuf;
	struct in_addr addr;

	addr.s_addr = ntohl(ic->getExtIP());
	if(p = inet_ntoa(addr)) lastip = p;

	if(lastip.find_first_not_of(".0") != -1) {
	    addr.s_addr = ntohl(ic->getLanIP());
	    if(p = inet_ntoa(addr)) {
		sbuf = p;

		if(sbuf.find_first_not_of(".0") != -1 && lastip != sbuf) {
		    if(!lastip.empty()) lastip += " ";
		    lastip += sbuf;
		}
	    }

	    c->setlastip(lastip);
	}
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
	    face.log(ev->getMessage());
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

void icqhook::want_auto_resp_cb(ICQMessageEvent *ev) {
    char buf[128];
    string ident;
    imcontact cont = imcontact(ev->getSenderUIN(), icq);
    icqcontact *c = clist.get(cont);

    ident = cont.totext();
    if(c) ident += " (" + c->getdispnick() + ")";

    logger.putmessage(buf);
    ev->setAwayMessage(rusconv("kw", conf.getawaymsg(icq)));
}

void icqhook::search_result_cb(SearchResultEvent *ev) {
    if(ev == searchevent) {
	ContactRef cont = ev->getLastContactAdded();

	if(searchdest && cont.get()) {
	    icqcontact *c = new icqcontact(imcontact(cont->getUIN(), icq));
	    icqcontact::basicinfo binfo = c->getbasicinfo();

	    if(ev->getSearchType() == SearchResultEvent::RandomChat) {
		binfo.fname = _("Random Chat User");

	    } else {
		c->setnick(rusconv("wk", cont->getAlias()));
		c->setdispnick(c->getnick());

		binfo.fname = rusconv("wk", cont->getFirstName());
		binfo.lname = rusconv("wk", cont->getLastName());
		binfo.email = rusconv("wk", cont->getEmail());

	    }

	    string line = (cont->getStatus() == STATUS_ONLINE ? "o " : "  ") + c->getnick();

	    if(line.size() > 12) line.resize(12);
	    else line += string(12-line.size(), ' ');

	    line += " " + binfo.fname + " " + binfo.lname;
	    if(!binfo.email.empty()) line += " <" + binfo.email + ">";

	    binfo.requiresauth = cont->getAuthReq();
	    binfo.authawait = cont->getAuthAwait();

	    c->setbasicinfo(binfo);

	    foundguys.push_back(c);
	    searchdest->additem(conf.getcolor(cp_clist_icq), c, line);
	    searchdest->redraw();
	}

	if(ev->isFinished() && searchdest) {
	    face.findready();
	    log(logSearchFinished, ev->getContactList().size());
	    searchdest = 0;
	}
    }
}

void icqhook::self_contact_userinfo_change_cb(UserInfoChangeEvent *ev) {
    if(!ev->isTransientDetail()) {
	icqcontact *c = clist.get(contactroot);

	updateinforecord(cli.getSelfContact(), c);
	updateinforecord(cli.getSelfContact(), clist.get(imcontact(cli.getUIN(), icq)));

	/*
	*
	* Fill in the locally stored parts now.
	*
	*/

	icqconf::imaccount im = conf.getourid(icq);
	icqcontact::basicinfo cbinfo = c->getbasicinfo();
	cbinfo.webaware = im.additional["webaware"] == "1";
	cbinfo.randomgroup = strtoul(im.additional["randomgroup"].c_str(), 0, 0);
	c->setbasicinfo(cbinfo);
    }
}

void icqhook::self_contact_status_change_cb(StatusChangeEvent *ev) {
    face.update();
}

/*

void icqhook::password_changed_cb(PasswordChangeEvent *ev) {
    if(ev->isSuccess()) {
	icqconf::imaccount acc = conf.getourid(icq);
	acc.password = ev->getPassword();
	conf.setourid(acc);
	log(logPasswordChanged);
    }
}

*/

void icqhook::sbl_received_cb(SBLReceivedEvent *ev) {
    icqcontact *cc;
    ContactTree &tree = cli.getContactTree();
    ContactTree::iterator curr = ev->tree.begin();

    sblrecv = false;

    while(curr != ev->tree.end()) {
	ContactTree::Group *lg = 0;

	ContactTree::iterator igl = tree.begin();
	while(igl != tree.end()) {
	    if((*igl).get_label() == (*curr).get_label()) {
		lg = &(*igl);
		break;
	    }
	    ++igl;
	}

	if(!lg) {
	    lg = &(tree.add_group((*curr).get_label(), (*curr).get_id()));
	} else {
	    lg->set_id((*curr).get_id());
	}

	ContactTree::Group::iterator ig = curr->begin();
	while(ig != curr->end()) {
	    imcontact ic((*ig)->getUIN(), proto);
	    clist.updateEntry(ic, (*curr).get_label());

	    ContactRef nc = tree[ic.uin];
	    if(nc.get()) {
		ContactTree::Group &cg = tree.lookup_group_containing_contact(nc);
		tree.relocate_contact(nc, cg, *lg);
	    } else {
		nc = lg->add(new Contact(ic.uin));
		nc->setAuthAwait((*ig)->getAuthAwait());
	    }

	    if(cc = clist.get(ic)) {
		icqcontact::basicinfo bi = cc->getbasicinfo();
		bi.authawait = nc->getAuthAwait();
		cc->setbasicinfo(bi);
	    }

	    nc->setServerBased(true);
	    nc->setServerSideInfo((*ig)->getServerSideGroupID(), (*ig)->getServerSideID());

	    ++ig;
	}
	++curr;
    }

    sblrecv = true;
}

ContactRef icqhook::addContact(unsigned int uin, const string &groupname) {
    ContactTree& ct = cli.getContactTree();
    ContactTree::Group *gp = 0;

    ContactTree::iterator curr = ct.begin();
    while(curr != ct.end()) {
	if((*curr).get_label() == groupname) {
	    gp = &(*curr);
	    break;
	}
	++curr;
    }

    if(!gp) gp = &(ct.add_group(groupname));

    ContactRef cont(new Contact(uin));
    gp->add(cont);

    return cont;
}
