/*
*
* centericq gadu-gadu protocol handling class
* $Id: gaduhook.cc,v 1.5 2004/04/01 08:01:13 konst Exp $
*
* Copyright (C) 2004 by Konstantin Klyagin <konst@konst.org.ua>
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

#include "icqcommon.h"

#ifdef BUILD_GADU

#include "eventmanager.h"
#include "gaduhook.h"
#include "icqface.h"
#include "imlogger.h"

#include "libgadu-config.h"
#include "libgadu.h"

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PERIOD_PING  50

imstatus gg2imstatus(int st) {
    imstatus imst;

    switch(st) {
	case GG_STATUS_INVISIBLE: imst = invisible; break;
	case GG_STATUS_BUSY: imst = occupied; break;
	case GG_STATUS_NOT_AVAIL: imst = notavail; break;
	default: imst = available; break;
    }

    return imst;
}

int imstatus2gg(imstatus st) {
    int gst;

    switch(st) {
	case invisible:
	    gst = GG_STATUS_INVISIBLE;
	    break;
	case dontdisturb:
	case occupied:
	    gst = GG_STATUS_BUSY;
	    break;
	case notavail:
	case away:
	    gst = GG_STATUS_NOT_AVAIL;
	    break;
	case freeforchat:
	case available:
	default:
	    gst = GG_STATUS_AVAIL;
	    break;
    }

    return gst;
}

// ----------------------------------------------------------------------------

gaduhook ghook;

gg_pubdir50_t lreq = 0, ireq = 0;

gaduhook::gaduhook(): abstracthook(gadu), flogged(false), sess(0) {
    fcapabs.insert(hookcapab::changepassword);
    fcapabs.insert(hookcapab::changenick);
    fcapabs.insert(hookcapab::changedetails);
    fcapabs.insert(hookcapab::ssl);
}

gaduhook::~gaduhook() {
}

void gaduhook::init() {
    manualstatus = conf.getstatus(proto);
}

void gaduhook::connect() {
    icqconf::imaccount acc = conf.getourid(proto);
    static struct gg_login_params lp;
    static auto_ptr<char> pass(strdup(acc.password.c_str()));

    memset(&lp, 0, sizeof(lp));

    struct hostent *he = gethostbyname(acc.server.c_str());
    struct in_addr addr;

    if(he) {
	memcpy((char *) &addr, he->h_addr, sizeof(addr));
	lp.server_addr = addr.s_addr;
	lp.server_port = acc.port;

	lp.uin = acc.uin;
	lp.password = pass.get();
	lp.async = 1;
	lp.status = imstatus2gg(manualstatus);

	sess = gg_login(&lp);
	log(logConnecting);

    } else {
	face.log(_("+ [gg] cannot resolve %s"), acc.server.c_str());

    }
}

void gaduhook::cutoff() {
    if(sess) {
	gg_logoff(sess);
	gg_free_session(sess);
	sess = 0;
    }
}

void gaduhook::disconnect() {
    cutoff();
    log(logDisconnected);
    logger.putourstatus(proto, getstatus(), offline);
}

void gaduhook::exectimers() {
    if(logged()) {
	if(timer_current-timer_ping > PERIOD_PING) {
	    gg_ping(sess);
	    timer_ping = timer_current;
	}
    }
}

void gaduhook::main() {
    int i;
    fd_set rd, wd;
    struct timeval tv;

    struct gg_event *e;
    struct gg_notify_reply *nr;
    string text;

    e = gg_watch_fd(sess);

    if(e) {
	switch(e->type) {
	    case GG_EVENT_CONN_SUCCESS:
		flogged = true;
		time(&timer_ping);
		userlistsend();
		log(logLogged);
		face.update();
		break;

	    case GG_EVENT_CONN_FAILED:
		cutoff();
		face.log(_("+ [gg] connection to the server failed"));
		break;

	    case GG_EVENT_DISCONNECT:
		disconnect();
		break;

	    case GG_EVENT_MSG:
		if(e->event.msg.sender && e->event.msg.message) {
		    text = rusconv("wk", (const char *) e->event.msg.message);
		    em.store(immessage(imcontact(e->event.msg.sender, gadu),
			imevent::incoming, text, e->event.msg.time));
		}
		break;

	    case GG_EVENT_NOTIFY:
	    case GG_EVENT_NOTIFY_DESCR:
		nr = (e->type == GG_EVENT_NOTIFY) ? e->event.notify : e->event.notify_descr.notify;

		for(; nr->uin; nr++) {
		    char *desc = (e->type == GG_EVENT_NOTIFY_DESCR) ? e->event.notify_descr.descr : 0;

		    usernotify(nr->uin, nr->status, desc,
			nr->remote_ip, nr->remote_port,
			nr->version);
		}
		break;

	    case GG_EVENT_NOTIFY60:
		for(i = 0; e->event.notify60[i].uin; i++)
		    usernotify(e->event.notify60[i].uin,
			e->event.notify60[i].status,
			e->event.notify60[i].descr,
			e->event.notify60[i].remote_ip,
			e->event.notify60[i].remote_port,
			e->event.notify60[i].version);
		break;

	    case GG_EVENT_STATUS:
		userstatuschange(e->event.status.uin, e->event.status.status);
		break;

	    case GG_EVENT_STATUS60:
		userstatuschange(e->event.status60.uin, e->event.status60.status);
		break;

	    case GG_EVENT_ACK:
		break;

	    case GG_EVENT_PONG:
		break;

	    case GG_EVENT_PUBDIR50_SEARCH_REPLY:
		searchdone(e->event.pubdir50);
		break;

	    case GG_EVENT_PUBDIR50_READ:
		break;

	    case GG_EVENT_PUBDIR50_WRITE:
		break;

	    case GG_EVENT_USERLIST:
		if(e->event.userlist.type == GG_USERLIST_GET_REPLY) {
		    char *p = e->event.userlist.reply;
		}
		break;
	}

	gg_free_event(e);

    } else {
	cutoff();
	face.log(_("+ [gg] connection lost"));

    }
}

void gaduhook::getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const {
    if(sess && sess->fd != -1) {
	if((sess->check & GG_CHECK_READ))
	    FD_SET(sess->fd, &rfds);

	if((sess->check & GG_CHECK_WRITE))
	    FD_SET(sess->fd, &wfds);

	hsocket = max(sess->fd, hsocket);
    }
}

bool gaduhook::isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const {
    if(sess && sess->fd != -1) {
	return FD_ISSET(sess->fd, &rfds) || FD_ISSET(sess->fd, &wfds);
    }

    return false;
}

bool gaduhook::online() const {
    return sess;
}

bool gaduhook::logged() const {
    return sess && flogged;
}

bool gaduhook::isconnecting() const {
    return sess && !flogged;
}

bool gaduhook::enabled() const {
    return true;
}

bool gaduhook::send(const imevent &ev) {
    icqcontact *c = clist.get(ev.getcontact());
    string text;

    if(c) {
	if(ev.gettype() == imevent::message) {
	    const immessage *m = static_cast<const immessage *>(&ev);
	    if(m) text = rushtmlconv("kw", m->gettext());

	} else if(ev.gettype() == imevent::url) {
	    const imurl *m = static_cast<const imurl *>(&ev);
	    if(m) text = rushtmlconv("kw", m->geturl()) + "\n\n" + rusconv("kw", m->getdescription());

	}

	gg_send_message(sess, GG_CLASS_MSG, c->getdesc().uin, (const unsigned char *) text.c_str());
	return true;
    }

    return false;
}

void gaduhook::sendnewuser(const imcontact &c) {
    gg_add_notify(sess, c.uin);
    requestinfo(c);
}

void gaduhook::removeuser(const imcontact &c) {
    gg_remove_notify(sess, c.uin);
}

void gaduhook::setautostatus(imstatus st) {
    if(st == offline) {
	if(getstatus() != offline) disconnect();

    } else {
	if(getstatus() != offline) {
	    gg_change_status(sess, imstatus2gg(st));
	    logger.putourstatus(proto, getstatus(), st);
	} else {
	    connect();
	}

    }
}

imstatus gaduhook::getstatus() const {
    if(!sess) return offline;

    if(GG_S_A(sess->status)) return available; else
    if(GG_S_NA(sess->status)) return notavail; else
    if(GG_S_B(sess->status)) return occupied; else
    if(GG_S_I(sess->status)) return invisible;
}

void gaduhook::requestinfo(const imcontact &c) {
    if(ireq) gg_pubdir50_free(ireq);
    ireq = gg_pubdir50_new(GG_PUBDIR50_SEARCH_REQUEST);

    gg_pubdir50_add(ireq, GG_PUBDIR50_UIN, i2str(c.uin).c_str());
    gg_pubdir50(sess, ireq);
}

void gaduhook::requestawaymsg(const imcontact &c) {
}

bool gaduhook::regconnect(const string &aserv) {
    return true;
}

bool gaduhook::regattempt(unsigned int &auin, const string &apassword) {
    fd_set rd, wr, ex;
    struct gg_http *h;
    struct gg_pubdir *p;
    struct gg_token *t;
    bool r = false;
    int wret;
    string tokenid;

    enum {
	phToken, phRegister, phEnd
    } phase = phToken;

    h = gg_token(1);

    while(h && phase != phEnd) {
	FD_ZERO(&rd);
	FD_ZERO(&wr);
	FD_ZERO(&ex);

	if(h->check & GG_CHECK_READ) FD_SET(h->fd, &rd);
	if(h->check & GG_CHECK_WRITE) FD_SET(h->fd, &wr);
	FD_SET(h->fd, &ex);

	if(select(h->fd + 1, &rd, &wr, &ex, 0) == -1 || FD_ISSET(h->fd, &ex))
	    break;

	if(FD_ISSET(h->fd, &rd) || FD_ISSET(h->fd, &wr)) {
	    switch(phase) {
		case phToken: wret = gg_token_watch_fd(h); break;
		case phRegister: wret = gg_register_watch_fd(h); break;
	    }

	    if(wret == -1 || h->state == GG_STATE_ERROR)
		break;

	    if(h->state == GG_STATE_DONE)
	    switch(phase) {
		case phToken:
		    t = (struct gg_token *) h->data;
		    tokenid = t->tokenid;
//                    gg_register3("", password.c_str(), tokenid, const char *tokenval, int async);
		    break;
		case phRegister:
		    p = (struct gg_pubdir *) h->data;
		    r = !p->success;
		    auin = p->uin;
		    break;
	    }
	}
    }

    gg_free_register(h);
    return r;
}

void gaduhook::lookup(const imsearchparams &params, verticalmenu &dest) {
    searchdest = &dest;
    while(!foundguys.empty()) {
	delete foundguys.back();
	foundguys.pop_back();
    }

    if(lreq) gg_pubdir50_free(lreq);
    lreq = gg_pubdir50_new(GG_PUBDIR50_SEARCH_REQUEST);

    if(params.uin) gg_pubdir50_add(lreq, GG_PUBDIR50_UIN, i2str(params.uin).c_str());
    if(!params.nick.empty()) gg_pubdir50_add(lreq, GG_PUBDIR50_NICKNAME, params.nick.c_str());
    if(!params.firstname.empty()) gg_pubdir50_add(lreq, GG_PUBDIR50_FIRSTNAME, params.firstname.c_str());
    if(!params.lastname.empty()) gg_pubdir50_add(lreq, GG_PUBDIR50_LASTNAME, params.lastname.c_str());
    if(!params.city.empty()) gg_pubdir50_add(lreq, GG_PUBDIR50_CITY, params.city.c_str());

    if(params.onlineonly) gg_pubdir50_add(lreq, GG_PUBDIR50_ACTIVE, GG_PUBDIR50_ACTIVE_TRUE);
    if(params.gender != genderUnspec) gg_pubdir50_add(lreq, GG_PUBDIR50_GENDER, params.gender == genderMale ? GG_PUBDIR50_GENDER_MALE : GG_PUBDIR50_GENDER_FEMALE);

    gg_pubdir50(sess, lreq);
}

void gaduhook::sendupdateuserinfo(const icqcontact &c) {
    gg_pubdir50_t req;

    if(req = gg_pubdir50_new(GG_PUBDIR50_WRITE)) {
	icqcontact::basicinfo bi = c.getbasicinfo();
	icqcontact::moreinfo mi = c.getmoreinfo();

	gg_pubdir50_add(req, GG_PUBDIR50_NICKNAME, c.getnick().c_str());
	gg_pubdir50_add(req, GG_PUBDIR50_FIRSTNAME, bi.fname.c_str());
	gg_pubdir50_add(req, GG_PUBDIR50_LASTNAME, bi.lname.c_str());
	gg_pubdir50_add(req, GG_PUBDIR50_CITY, bi.city.c_str());

	switch(mi.gender) {
	    case genderMale:
		gg_pubdir50_add(req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_SET_MALE);
		break;
	    case genderFemale:
		gg_pubdir50_add(req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_SET_FEMALE);
		break;
	}

	gg_pubdir50(sess, req);
	gg_pubdir50_free(req);
    }
}

// ----------------------------------------------------------------------------

void gaduhook::searchdone(void *p) {
    gg_pubdir50_t sp = (gg_pubdir50_t) p;

    if(searchdest && lreq) {
	for(int i = 0; i < sp->count; i++) {
	    icqcontact *c = new icqcontact(imcontact(strtoul(gg_pubdir50_get(sp, i, GG_PUBDIR50_UIN), 0, 0), gadu));
	    icqcontact::basicinfo binfo = c->getbasicinfo();

	    const char *p = gg_pubdir50_get(sp, i, GG_PUBDIR50_NICKNAME);
	    if(p) {
		c->setnick(p);
		c->setdispnick(c->getnick());
	    }

	    p = gg_pubdir50_get(sp, i, GG_PUBDIR50_FIRSTNAME);
	    if(p) binfo.fname = p;

	    p = gg_pubdir50_get(sp, i, GG_PUBDIR50_LASTNAME);
	    if(p) binfo.lname = p;

	    string line;

	    p = gg_pubdir50_get(sp, i, GG_PUBDIR50_STATUS);
	    if(p && atoi(p) != GG_STATUS_NOT_AVAIL) line = "o ";
		else line = "  ";

	    line += c->getnick();
	    if(line.size() > 12) line.resize(12);
	    else line += string(12-line.size(), ' ');

	    line += " " + binfo.fname + " " + binfo.lname;

	    c->setbasicinfo(binfo);

	    foundguys.push_back(c);
	    searchdest->additem(conf.getcolor(cp_clist_gadu), c, line);
	}

	searchdest->redraw();
	face.findready();
	log(logSearchFinished, foundguys.size());
	searchdest = 0;

    } else if(ireq) {
	icqcontact *c = clist.get(imcontact(strtoul(gg_pubdir50_get(sp, 0, GG_PUBDIR50_UIN), 0, 0), gadu));
	string nick, gender;
	const char *p = 0;

	if(!c) c = clist.get(contactroot);

	icqcontact::basicinfo bi = c->getbasicinfo();
	icqcontact::moreinfo mi = c->getmoreinfo();

	p = gg_pubdir50_get(sp, 0, GG_PUBDIR50_FIRSTNAME);
	if(p) bi.fname = p;

	p = gg_pubdir50_get(sp, 0, GG_PUBDIR50_LASTNAME);
	if(p) bi.lname = p;

	p = gg_pubdir50_get(sp, 0, GG_PUBDIR50_NICKNAME);

	if(p) {
	    nick = p;

	    if((c->getnick() == c->getdispnick())
	    || (c->getdispnick() == i2str(c->getdesc().uin)))
		c->setdispnick(nick);

	    c->setnick(nick);

	} else {
	    c->setdispnick(bi.fname);

	}

	p = gg_pubdir50_get(sp, 0, GG_PUBDIR50_CITY);
	if(p) bi.city = p;

	p = gg_pubdir50_get(sp, 0, GG_PUBDIR50_GENDER);
	if(p) {
	    gender = p;

	    if(gender == GG_PUBDIR50_GENDER_MALE) mi.gender = genderMale; else
	    if(gender == GG_PUBDIR50_GENDER_FEMALE) mi.gender = genderFemale; else
		mi.gender = genderUnspec;
	}

	c->setbasicinfo(bi);
	c->setmoreinfo(mi);
    }
}

void gaduhook::userstatuschange(unsigned int uin, int status) {
    icqcontact *c = clist.get(imcontact(uin, gadu));
    if(c) {
	c->setstatus(gg2imstatus(status));
    }
}

void gaduhook::userlistsend() {
    int i;
    vector<uin_t> uins;
    icqcontact *c;

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == proto && c->getdesc().uin)
	    uins.push_back(c->getdesc().uin);
    }

    auto_ptr<uin_t> cuins(new uin_t[uins.size()]);
    auto_ptr<char> ctypes(new char[uins.size()]);

    for(vector<uin_t>::const_iterator iu = uins.begin(); iu != uins.end(); ++iu) {
	cuins.get()[iu-uins.begin()] = *iu;
	ctypes.get()[iu-uins.begin()] = GG_USER_NORMAL;
    }

    gg_notify_ex(sess, cuins.get(), ctypes.get(), uins.size());
}

void gaduhook::usernotify(unsigned int uin, int status, const char *desc,
unsigned int ip, int port, int version) {
    imcontact ic(uin, gadu);
    icqcontact *c = clist.get(ic);

    if(c) {
	struct in_addr addr;
	addr.s_addr = ntohl(ip);
	char *p = inet_ntoa(addr);
	if(p) c->setlastip(p);

	c->setstatus(gg2imstatus(status));
    }
}

#endif
