/*
*
* centericq yahoo! protocol handling class
* $Id: yahoohook.cc,v 1.24 2002/02/25 10:41:00 konst Exp $
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
#include "yahoolib.h"
#include "centericq.h"
#include "imlogger.h"

yahoohook yhook;

static int stat2int[imstatus_size] = {
    0,
    YAHOO_STATUS_AVAILABLE,
    YAHOO_STATUS_INVISIBLE,
    YAHOO_STATUS_CUSTOM,
    YAHOO_STATUS_BUSY,
    YAHOO_STATUS_NOTATDESK,
    YAHOO_STATUS_NOTATHOME,
    YAHOO_STATUS_BRB
};

yahoohook::yahoohook() {
    fonline = false;
    timer_reconnect = 0;
    context = 0;
}

yahoohook::~yahoohook() {
}

void yahoohook::init() {
    manualstatus = conf.getstatus(yahoo);
}

void yahoohook::initcontext(const icqconf::imaccount account) {
    yahoo_options options;

    if(enabled()) {
	yahoo_free_context(context);
	context = 0;
    }

    memset(&options, 0, sizeof(options));
    options.connect_mode = YAHOO_CONNECT_NORMAL;

    face.log("+ initializing %s engine",
	conf.getprotocolname(account.pname).c_str());

    alarm(5);

    if(context = yahoo_init(account.nickname.c_str(), account.password.c_str(),
    &options)) {
	context->yahoo_Disconnected = &disconnected;
	context->yahoo_UserLogon = &userlogon;
	context->yahoo_UserLogoff = &userlogoff;
	context->yahoo_UserStatus = &userstatus;
	context->yahoo_RecvBounced = &recvbounced;
	context->yahoo_RecvMessage = &recvmessage;

#ifdef DEBUG
	context->yahoo_Log = &log;
#else
	context->yahoo_Log = 0;
#endif
    } else {
	face.log("+ unable to init %s engine",
	    conf.getprotocolname(account.pname).c_str());
    }

    alarm(0);
}

void yahoohook::connect() {
    icqconf::imaccount acc = conf.getourid(yahoo);
    int r;

    initcontext(acc);

    if(context) {
	face.log(_("+ [yahoo] connecting to the server"));

	alarm(5);
	r = yahoo_connect(context);
	alarm(0);
	logger.putourstatus(yahoo, offline, manualstatus);

	if(!r) {
	    face.log(_("+ [yahoo] unable to connect to the server"));
	    disconnected(context);
	} else {
	    yahoo_get_config(context);

	    if(!yahoo_cmd_logon(context, stat2int[ourstatus = available])) {
		fonline = true;
		setstatus(manualstatus);
		face.log(_("+ [yahoo] logged in"));
	    }
	}
    }

    yahoo_Russian = 0;
    time(&timer_reconnect);
}

void yahoohook::main() {
    if(context) {
	yahoo_main(context);
    }
}

void yahoohook::getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const {
    if(online()) {
	FD_SET(context->sockfd, &rfds);
	hsocket = max(context->sockfd, hsocket);
    }
}

bool yahoohook::isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const {
    return online() && FD_ISSET(context->sockfd, &rfds);
}

void yahoohook::disconnect() {
    if(context) {
	yahoo_cmd_logoff(context);
	disconnected(context);
    }
}

void yahoohook::exectimers() {
    time_t tcurrent = time(0);

    if(logged()) {
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
    }

    if(!text.empty()) {
	for(is = text.begin(); is != text.end(); is++)
	    if((unsigned) *is < 32) *is = ' ';

	if((c->getstatus() != offline) && c->inlist()) {
	    yahoo_cmd_msg(context, conf.getourid(yahoo).nickname.c_str(),
		ev.getcontact().nickname.c_str(), text.c_str());
	} else {
	    yahoo_cmd_msg_offline(context, conf.getourid(yahoo).nickname.c_str(),
		ev.getcontact().nickname.c_str(), text.c_str());
	}

	return true;
    }

    return false;
}

bool yahoohook::online() const {
    return fonline;
}

bool yahoohook::logged() const {
    return fonline;
}

void yahoohook::sendnewuser(const imcontact ic) {
    char *group;

    if(yahoo)
    if(online()) {
	if(context->buddies && context->buddies[0]) {
	    group = context->buddies[0]->group;
	} else {
	    group = "group";
	}

	if(!yahoo_isbuddy(context, ic.nickname.c_str())) {
	    face.log(_("+ [yahoo] adding %s to the contacts"), ic.nickname.c_str());
	    yahoo_add_buddy(context, ic.nickname.c_str(),
		conf.getourid(yahoo).nickname.c_str(), group, "");
	}
    }
}

void yahoohook::removeuser(const imcontact ic) {
    if(context)
    if(online()) {
	face.log(_("+ [yahoo] removing %s from the contacts"), ic.nickname.c_str());
	yahoo_remove_buddy(context, ic.nickname.c_str(),
	    conf.getourid(yahoo).nickname.c_str(), "group", "");
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
    return (bool) context;
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
	    ourstatus = st;

	    if(st == available) {
		yahoo_cmd_set_back_mode(context, stat2int[st], "available");
	    } else {
		if(stat2int[st] == YAHOO_STATUS_IDLE) {
		    yahoo_cmd_idle(context);
		} else {
		    yahoo_cmd_set_away_mode(context, stat2int[st], "away");
		}
	    }
	}
    }
}

imstatus yahoohook::getstatus() const {
    return online() ? ourstatus : offline;
}

// ----------------------------------------------------------------------------

void yahoohook::disconnected(yahoo_context *y) {
    int i;
    icqcontact *c;

    if(yhook.fonline) {
	close(y->sockfd);
	face.log(_("+ [yahoo] disconnected from the network"));
	yhook.fonline = false;
    }

    clist.setoffline(yahoo);
    time(&yhook.timer_reconnect);
    face.update();
}

void yahoohook::userlogon(yahoo_context *y, const char *nick, int status) {
    yhook.userstatus(yhook.context, nick, status);
}

void yahoohook::userlogoff(yahoo_context *y, const char *nick) {
    yhook.userstatus(yhook.context, nick, -1);
}

void yahoohook::userstatus(yahoo_context *y, const char *nick, int status) {
    if(nick)
    if(strlen(nick)) {
	imcontact ic(nick, yahoo);
	icqcontact *c = clist.get(ic);

	if(!c) {
	    c = clist.addnew(ic, false);
	}

	logger.putonline(ic, c->getstatus(), yhook.yahoo2imstatus(status));
	c->setstatus(yhook.yahoo2imstatus(status));

	if(c->getstatus() != offline) {
	    c->setlastseen();
	}
    }
}

void yahoohook::recvbounced(yahoo_context *y, const char *nick) {
    face.log(_("+ [yahoo] bounced msg"));
}

void yahoohook::recvmessage(yahoo_context *y, const char *nick, const char *msg) {
    if(nick)
    if(strlen(nick)) {
	imcontact ic(nick, yahoo);

	if(strlen(msg)) {
	    em.store(immessage(ic, imevent::incoming, rusconv("wk", msg)));
	    icqcontact *c = clist.get(ic);

	    if(c)
	    if(c->getstatus() == offline) {
		c->setstatus(available);
	    }
	}
    }
}

void yahoohook::log(yahoo_context *y, const char *msg) {
    face.log(msg);
}
