/*
*
* centericq yahoo! protocol handling class
* $Id: yahoohook.cc,v 1.40 2002/07/12 18:01:45 konst Exp $
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

#include "yahoohook.h"
#include "icqmlist.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "accountmanager.h"
#include "centericq.h"
#include "imlogger.h"

extern "C" {
#include "yahoo2_callbacks.h"
};

char pager_host[64], pager_port[64];

char *filetransfer_host = "filetransfer.msg.yahoo.com";
char *filetransfer_port = "80";

yahoohook yhook;

static int stat2int[imstatus_size] = {
    YAHOO_STATUS_OFFLINE,
    YAHOO_STATUS_AVAILABLE,
    YAHOO_STATUS_INVISIBLE,
    YAHOO_STATUS_CUSTOM,
    YAHOO_STATUS_BUSY,
    YAHOO_STATUS_NOTATDESK,
    YAHOO_STATUS_NOTATHOME,
    YAHOO_STATUS_BRB,
};

yahoohook::yahoohook() : fonline(false) {
    fcapabilities = hoptCanSetAwayMsg | hoptChangableServer;
    pager_host[0] = pager_port[0] = 0;
}

yahoohook::~yahoohook() {
}

void yahoohook::init() {
    manualstatus = conf.getstatus(yahoo);

    static struct yahoo_callbacks c;

    memset(&c, 0, sizeof(c));

    c.yahoo_login_done = &login_done;
    c.yahoo_got_buddies = &got_buddies;
    c.yahoo_status_changed = &status_changed;
    c.yahoo_got_im = &got_im;
    c.yahoo_got_conf_invite = &got_conf_invite;
    c.yahoo_conf_userdecline = &conf_userdecline;
    c.yahoo_conf_userjoin = &conf_userjoin;
    c.yahoo_conf_userleave = &conf_userleave;
    c.yahoo_conf_message = &conf_message;
    c.yahoo_got_file = &got_file;
    c.yahoo_contact_added = &contact_added;
    c.yahoo_typing_notify = &typing_notify;
    c.yahoo_game_notify = &game_notify;
    c.yahoo_mail_notify = &mail_notify;
    c.yahoo_system_message = &system_message;
    c.yahoo_error = &error;
    c.yahoo_add_input = &add_input;
    c.yahoo_remove_input = &remove_input;

    yahoo_register_callbacks(&c);
}

void yahoohook::connect() {
    icqconf::imaccount acc = conf.getourid(yahoo);
    int r;

    strcpy(pager_host, acc.server.c_str());
    strcpy(pager_port, i2str(acc.port).c_str());

    face.log(_("+ [yahoo] connecting to the server"));

    auto_ptr<char> nick(strdup(acc.nickname.c_str()));
    auto_ptr<char> pass(strdup(acc.password.c_str()));

    cid = yahoo_login(nick.get(), pass.get(), stat2int[manualstatus]);

    if(cid < 0) {
	string msg = _("+ [yahoo] cannot connect: ");

	switch(cid) {
	    case -1: msg += _("could not resolve hostname"); break;
	    case -2: msg += _("could not create socket"); break;
	    default: msg += _("verify the pager host and port entered"); break;
	}

	face.log(msg);
    } else {
	fonline = true;
	flogged = false;
    }
}

void yahoohook::main() {
    yahoo_pending(cid, yahoo_get_fd(cid));
}

void yahoohook::getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const {
    if(online()) {
	FD_SET(yahoo_get_fd(cid), &rfds);
	hsocket = max(yahoo_get_fd(cid), hsocket);
    }
}

bool yahoohook::isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const {
    return online() && FD_ISSET(yahoo_get_fd(cid), &rfds);
}

void yahoohook::disconnect() {
    if(online()) {
	yahoo_logoff(cid);
	logger.putourstatus(yahoo, getstatus(), offline);
	clist.setoffline(yahoo);
	fonline = false;
	face.log(_("+ [yahoo] disconnected"));
	face.update();
    }
}

void yahoohook::exectimers() {
}

struct tm *yahoohook::timestamp() {
    time_t t = time(0);
    return localtime(&t);
}

bool yahoohook::send(const imevent &ev) {
    icqcontact *c = clist.get(ev.getcontact());
    string text;
    string::iterator is;

    if(c) {
	if(ev.gettype() == imevent::message) {
	    const immessage *m = static_cast<const immessage *>(&ev);
	    if(m) text = rusconv("kw", m->gettext());
	} else if(ev.gettype() == imevent::url) {
	    const imurl *m = static_cast<const imurl *>(&ev);
	    if(m) text = rusconv("kw", m->geturl()) + "\n\n" + rusconv("kw", m->getdescription());
	}

	for(is = text.begin(); is != text.end(); is++)
	    if((unsigned) *is < 32) *is = ' ';

	auto_ptr<char> who(strdup(ev.getcontact().nickname.c_str()));
	auto_ptr<char> what(strdup(text.c_str()));

	yahoo_send_im(cid, who.get(), what.get(), strlen(what.get()));
	return true;
    }

    return false;
}

bool yahoohook::online() const {
    return fonline;
}

bool yahoohook::logged() const {
    return fonline && flogged;
}

bool yahoohook::isconnecting() const {
    return fonline && !flogged;
}

void yahoohook::sendnewuser(const imcontact &ic) {
    char *group;

    if(yahoo)
    if(online()) {
/*
	if(context->buddies && context->buddies[0]) {
	    group = context->buddies[0]->group;
	} else {
	    group = "group";
	}

	struct yahoo_buddy **lst = get_buddylist(cid);
*/
	if(1/*!yahoo_isbuddy(context, ic.nickname.c_str())*/) {
	    face.log(_("+ [yahoo] adding %s to the contacts"), ic.nickname.c_str());

	    auto_ptr<char> who(strdup(ic.nickname.c_str()));
	    auto_ptr<char> group(strdup("centericq"));

	    yahoo_add_buddy(cid, who.get(), group.get());
	}
    }

    requestinfo(ic);
}

void yahoohook::removeuser(const imcontact &ic) {
    if(online()) {
	face.log(_("+ [yahoo] removing %s from the contacts"), ic.nickname.c_str());

	auto_ptr<char> who(strdup(ic.nickname.c_str()));
	auto_ptr<char> group(strdup("centericq"));

	yahoo_remove_buddy(cid, who.get(), group.get());
    }
}

imstatus yahoohook::yahoo2imstatus(int status) const {
    imstatus st = offline;

    switch(status) {
	case YAHOO_STATUS_AVAILABLE:
	    st = available;
	    break;
	case YAHOO_STATUS_CUSTOM:
	    st = freeforchat;
	    break;
	case YAHOO_STATUS_BUSY:
	    st = dontdisturb;
	    break;
	case YAHOO_STATUS_BRB:
	case YAHOO_STATUS_IDLE:
	case YAHOO_STATUS_ONPHONE:
	    st = away;
	    break;
	case YAHOO_STATUS_NOTATDESK:
	    st = occupied;
	    break;
	case YAHOO_STATUS_NOTATHOME:
	case YAHOO_STATUS_NOTINOFFICE:
	case YAHOO_STATUS_ONVACATION:
	case YAHOO_STATUS_OUTTOLUNCH:
	case YAHOO_STATUS_STEPPEDOUT:
	    st = notavail;
	    break;
	case YAHOO_STATUS_INVISIBLE:
	    st = invisible;
	    break;
	case -1:
	    st = offline;
	    break;
    }

    return st;
}

bool yahoohook::enabled() const {
    return true;
}

void yahoohook::setautostatus(imstatus st) {
    if(st == offline) {
	if(getstatus() != offline) {
	    disconnect();
	}
    } else {
	if(getstatus() == offline) {
	    connect();
	} else {
	    logger.putourstatus(yahoo, getstatus(), st);

	    if(st == available) {
		auto_ptr<char> msg(strdup("available"));
		yahoo_set_away(cid, (yahoo_status) stat2int[st], msg.get(), 0);
	    } else {
		/*if(stat2int[st] == YAHOO_STATUS_IDLE) {
		    yahoo_cmd_idle(context);
		} else*/ {
		    auto_ptr<char> msg(strdup(rusconv("kw", conf.getawaymsg(yahoo)).c_str()));
		    yahoo_set_away(cid, (yahoo_status) stat2int[st], msg.get(), 1);
		}
	    }
	}
    }
}

imstatus yahoohook::getstatus() const {
    return online() ? yahoo2imstatus(yahoo_current_status(cid)) : offline;
}

void yahoohook::requestinfo(const imcontact &ic) {
    icqcontact *c = clist.get(ic);

    if(c) {
	icqcontact::moreinfo m = c->getmoreinfo();
	icqcontact::basicinfo b = c->getbasicinfo();

	m.homepage = "http://profiles.yahoo.com/" + ic.nickname;
	b.email = ic.nickname + "@yahoo.com";

	c->setbasicinfo(b);
	c->setmoreinfo(m);
    }
}

void yahoohook::userstatus(const string &nick, int st, const string &message, bool away) {
    imcontact ic(nick, yahoo);
    icqcontact *c = clist.get(ic);

    if(!c) {
	c = clist.addnew(ic, false);
    }

    if(c) {
	logger.putonline(ic, c->getstatus(), yhook.yahoo2imstatus(st));

	c->setstatus(yhook.yahoo2imstatus(st));
	if(c->getstatus() != offline) c->setlastseen();

	c->setabout(rusconv("wk", message));
    }
}

// ----------------------------------------------------------------------------

void yahoohook::login_done(guint32 id, int succ, char *url) {
    switch(succ) {
	case YAHOO_LOGIN_OK:
	    yhook.flogged = true;
	    logger.putourstatus(yahoo, offline,
		yhook.yahoo2imstatus(yahoo_current_status(yhook.cid)));
	    face.log(_("+ [yahoo] logged in"));
	    break;

	case YAHOO_LOGIN_PASSWD:
	    yhook.fonline = false;
	    face.log(_("+ [yahoo] cannot login: username and password mismatch"));
	    break;

	case YAHOO_LOGIN_LOCK:
	    yhook.fonline = false;
	    face.log(_("+ [yahoo] cannot login: the account has been blocked"));
	    face.log(_("+ to reactivate visit %s"), url);
	    break;
    }

    face.update();
}

void yahoohook::got_buddies(guint32 id, struct yahoo_buddy **buds) {
    struct yahoo_buddy **buddy;
    icqcontact *c;

    for(buddy = buds; buddy && *buddy; buddy++) {
	struct yahoo_buddy *bud = *buddy;
	imcontact ic(bud->id, yahoo);

	c = clist.get(ic);
	if(!c) {
	    clist.addnew(ic, false);
	}
    }
}

void yahoohook::status_changed(guint32 id, char *who, int stat, char *msg, int away) {
    yhook.userstatus(who, stat, msg ? msg : "", (bool) away);
}

void yahoohook::got_im(guint32 id, char *who, char *msg, long tm, int stat) {
    imcontact ic(who, yahoo);
    em.store(immessage(ic, imevent::incoming, rusconv("wk", msg)));
}

void yahoohook::got_conf_invite(guint32 id, char *who, char *room, char *msg, char **members) {
}

void yahoohook::conf_userdecline(guint32 id, char *who, char *room, char *msg) {
}

void yahoohook::conf_userjoin(guint32 id, char *who, char *room) {
}

void yahoohook::conf_userleave(guint32 id, char *who, char *room) {
}

void yahoohook::conf_message(guint32 id, char *who, char *room, char *msg) {
}

void yahoohook::got_file(guint32 id, char *who, char *url, long expires, char *msg, char *fname, long fesize) {
}

void yahoohook::contact_added(guint32 id, char *myid, char *who, char *msg) {
}

void yahoohook::typing_notify(guint32 id, char *who, int stat) {
}

void yahoohook::game_notify(guint32 id, char *who, int stat) {
}

void yahoohook::mail_notify(guint32 id, char *from, char *subj, int cnt) {
}

void yahoohook::system_message(guint32 id, char *msg) {
}

void yahoohook::error(guint32 id, char *err, int fatal) {
    if(fatal) {
	face.log(_("[yahoo] error: %s"), err);
	yhook.disconnect();
    }
}

void yahoohook::add_input(guint32 id, int fd) {
}

void yahoohook::remove_input(guint32 id, int fd) {
}
