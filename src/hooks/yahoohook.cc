/*
*
* centericq yahoo! protocol handling class
* $Id: yahoohook.cc,v 1.55 2002/09/13 13:15:56 konst Exp $
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
    YAHOO_STATUS_IDLE,
};

yahoohook::yahoohook() : fonline(false) {
    fcapabs.insert(hookcapab::setaway);
    fcapabs.insert(hookcapab::synclist);

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
	disconnected();
    }
}

void yahoohook::disconnected() {
    logger.putourstatus(yahoo, getstatus(), ourstatus = offline);
    clist.setoffline(yahoo);
    fonline = false;
    face.log(_("+ [yahoo] disconnected"));
    face.update();
}

void yahoohook::exectimers() {
    vector<pair<Action, string> >::iterator it;

    for(it = tobedone.begin(); it != tobedone.end(); ++it) {
	switch(it->first) {
	    case tbdConfLogon:
		auto_ptr<char> room(strdup(it->second.c_str()));
		yahoo_conference_logon(cid, getmembers(it->second), room.get());
		break;
	}
    }

    tobedone.clear();
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
	    if(m) text = rushtmlconv("kw", m->gettext());
	} else if(ev.gettype() == imevent::url) {
	    const imurl *m = static_cast<const imurl *>(&ev);
	    if(m) text = rushtmlconv("kw", m->geturl()) + "\n\n" + rusconv("kw", m->getdescription());
	}

	for(is = text.begin(); is != text.end(); ++is)
	    if((unsigned) *is < 32) *is = ' ';

	auto_ptr<char> who(strdup(ev.getcontact().nickname.c_str()));
	auto_ptr<char> what(strdup(text.c_str()));

	if(!ischannel(c)) {
	    yahoo_send_im(cid, who.get(), what.get(), strlen(what.get()));
	} else {
	    yahoo_conference_message(cid, getmembers(who.get()+1), who.get()+1, what.get());
	}

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

    if(online() && !ischannel(ic)) {
	if(logged()) {
	    bool found = false;
	    struct yahoo_buddy **buddies = get_buddylist(cid), **bud;

	    auto_ptr<char> who(strdup(ic.nickname.c_str()));
	    auto_ptr<char> group(strdup("centericq"));

	    for(bud = buddies; *bud && !found; bud++)
		found = !strcmp((*bud)->id, who.get());

	    if(!found) {
		face.log(_("+ [yahoo] adding %s to the contacts"), ic.nickname.c_str());
		yahoo_add_buddy(cid, who.get(), group.get());
		yahoo_refresh(cid);
	    }
	}
    }

    requestinfo(ic);
}

void yahoohook::removeuser(const imcontact &ic) {
    if(logged()) {
	auto_ptr<char> who(strdup(ic.nickname.c_str()));

	if(!ischannel(ic)) {
	    face.log(_("+ [yahoo] removing %s from the contacts"), ic.nickname.c_str());

	    struct yahoo_buddy **buddies = get_buddylist(cid), **bud;

	    for(bud = buddies; *bud; bud++) {
		if(!strcmp((*bud)->id, who.get())) {
		    auto_ptr<char> grp(strdup((*bud)->group));
		    yahoo_remove_buddy(cid, who.get(), grp.get());
		    bud = buddies;
		}
	    }
	} else {
	    face.log(_("+ [yahoo] leaving the %s conference"), ic.nickname.c_str());
	    yahoo_conference_logoff(cid, getmembers(who.get()+1), who.get()+1);
	}
    }
}

imstatus yahoohook::yahoo2imstatus(int status) {
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
	    logger.putourstatus(yahoo, getstatus(), ourstatus = st);

	    if(st == freeforchat) {
		auto_ptr<char> msg(strdup("free for chat"));
		yahoo_set_away(cid, (yahoo_status) stat2int[st], msg.get(), 0);

	    } else if(st != away) {
		yahoo_set_away(cid, (yahoo_status) stat2int[st], 0, 0);

	    } else {
		auto_ptr<char> msg(strdup(rusconv("kw", conf.getawaymsg(yahoo)).c_str()));
		yahoo_set_away(cid, (yahoo_status) stat2int[st], msg.get(), 1);

	    }
	}
    }
}

imstatus yahoohook::getstatus() const {
    return online() ? ourstatus : offline;
}

void yahoohook::requestinfo(const imcontact &ic) {
    icqcontact *c = clist.get(ic);

    if(c) {
	icqcontact::moreinfo m = c->getmoreinfo();
	icqcontact::basicinfo b = c->getbasicinfo();

	m.homepage = "http://profiles.yahoo.com/" + ic.nickname;
	b.email = ic.nickname + "@yahoo.com";

	c->setmoreinfo(m);
	c->setbasicinfo(b);
    }
}

void yahoohook::userstatus(const string &nick, int st, const string &message, bool away) {
    imcontact ic(nick, yahoo);
    icqcontact *c = clist.get(ic);

    if(!c) {
	c = clist.addnew(ic, false);
    }

    if(c) {
	logger.putonline(ic, c->getstatus(), yahoo2imstatus(st));

	c->setstatus(yahoo2imstatus(st));
	if(c->getstatus() != offline) c->setlastseen();

	c->setabout(rusconv("wk", message));
    }
}

char **yahoohook::getmembers(const string &room) {
    int i;
    static char **smemb = 0;
    vector<string>::iterator ic;
    map<string, vector<string> >::iterator im;

    if(smemb) {
	for(i = 0; smemb[i]; delete smemb[i++]);
	delete smemb;
	smemb = 0;
    }

    if((im = confmembers.find(room)) != confmembers.end()) {
	smemb = new char*[im->second.size()+1];
	smemb[im->second.size()] = 0;
	for(ic = im->second.begin(); ic != im->second.end(); ++ic) {
	    smemb[ic-im->second.begin()] = strdup(ic->c_str());
	}
    } else {
	smemb = new char*[1];
	smemb[0] = 0;
    }

    return smemb;
}

vector<icqcontact *> yahoohook::getneedsync() {
    int i;
    icqcontact *c;
    yahoo_buddy **buddies = get_buddylist(cid), **bud;
    vector<icqcontact *> r;
    bool found;

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == yahoo) {
	    for(found = false, bud = buddies; *bud && !found; bud++) {
		found = c->getdesc().nickname == (string) (*bud)->id;
	    }

	    if(!found)
		r.push_back(c);
	}
    }

    return r;
}

// ----------------------------------------------------------------------------

void yahoohook::login_done(guint32 id, int succ, char *url) {
    vector<string>::iterator in;

    switch(succ) {
	case YAHOO_LOGIN_OK:
	    yhook.flogged = true;
	    logger.putourstatus(yahoo, offline, yhook.ourstatus = yhook.manualstatus);
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
    vector<string> nicks;
    vector<string>::iterator in;
    icqconf::imaccount acc = conf.getourid(yahoo);
    icqcontact *c;
    int i;

    for(buddy = buds; buddy && *buddy; buddy++) {
	if(find(nicks.begin(), nicks.end(), (*buddy)->id) == nicks.end()) {
	    nicks.push_back((*buddy)->id);
	}
    }

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == yahoo && !ischannel(c)) {
	    in = find(nicks.begin(), nicks.end(), c->getdesc().nickname);

	    if(in == nicks.end()) {
//                yhook.sendnewuser(c);
	    } else {
		nicks.erase(in);
	    }
	}
    }

    for(in = nicks.begin(); in != nicks.end(); ++in)
	clist.addnew(imcontact(*in, yahoo), false);
}

void yahoohook::status_changed(guint32 id, char *who, int stat, char *msg, int away) {
    yhook.userstatus(who, stat, msg ? msg : "", (bool) away);
}

void yahoohook::got_im(guint32 id, char *who, char *msg, long tm, int stat) {
    imcontact ic(who, yahoo);
    string text = rushtmlconv("wk", cuthtml(msg, true));

    if(!text.empty()) {
	em.store(immessage(ic, imevent::incoming, text));
    }
}

void yahoohook::got_conf_invite(guint32 id, char *who, char *room, char *msg, char **members) {
    icqconf::imaccount acc = conf.getourid(yahoo);
    string confname = (string) "#" + room, inviter, text;
    vector<string>::iterator ic;
    char buf[1024];
    int i;

    imcontact cont(confname, yahoo);
    icqcontact *c = clist.get(cont);
    if(!c) c = clist.addnew(cont);

    c->setstatus(available);

    inviter = confname.substr(1);
    if((i = inviter.rfind("-")) != -1) {
	inviter.erase(i);
    }

    sprintf(buf, _("The user %s has invited you to the %s conference, the topic there is: %s"),
	rusconv("wk", inviter).c_str(), rusconv("wk", room).c_str(), rusconv("wk", msg).c_str());

    text = (string) buf + "\n\n" + _("Current conference members are: ");
    yhook.confmembers[room].push_back(inviter);

    for(i = 0; members[i]; i++) {
	if(members[i] != acc.nickname)
	if(find(yhook.confmembers[room].begin(), yhook.confmembers[room].end(), members[i]) == yhook.confmembers[room].end()) {
	    yhook.confmembers[room].push_back(members[i]);
	}
    }

    for(ic = yhook.confmembers[room].begin(); ic != yhook.confmembers[room].end(); ) {
	text += *ic;
	if(++ic != yhook.confmembers[room].end())
	    text += ", ";
    }

    em.store(imnotification(cont, text));
    em.store(imnotification(cont, _("Auto-joined the conference")));

    yhook.tobedone.push_back(make_pair(tbdConfLogon, room));
}

void yahoohook::conf_userdecline(guint32 id, char *who, char *room, char *msg) {
    face.log("conf_userdecline");
}

void yahoohook::conf_userjoin(guint32 id, char *who, char *room) {
    icqcontact *c = clist.get(imcontact((string) "#" + room, yahoo));
    char buf[512];

    if(c) {
	sprintf(buf, _("The user %s has joined the conference"), rushtmlconv("wk", who).c_str());

	if(find(yhook.confmembers[room].begin(), yhook.confmembers[room].end(), who) == yhook.confmembers[room].end())
	    yhook.confmembers[room].push_back(who);

	em.store(imnotification(c, buf));
    }
}

void yahoohook::conf_userleave(guint32 id, char *who, char *room) {
    icqcontact *c = clist.get(imcontact((string) "#" + room, yahoo));
    char buf[512];
    vector<string>::iterator im;

    if(c) {
	sprintf(buf, _("The user %s has left the conference"), rushtmlconv("wk", who).c_str());
	em.store(imnotification(c, buf));

	im = find(yhook.confmembers[room].begin(), yhook.confmembers[room].end(), who);
	if(im != yhook.confmembers[room].end()) yhook.confmembers[room].erase(im);
    }
}

void yahoohook::conf_message(guint32 id, char *who, char *room, char *msg) {
    icqcontact *c = clist.get(imcontact((string) "#" + room, yahoo));

    string text = rushtmlconv("wk", who) + ": " + rushtmlconv("wk", cuthtml(msg, true));
    if(c) em.store(immessage(c, imevent::incoming, text));
}

void yahoohook::got_file(guint32 id, char *who, char *url, long expires, char *msg, char *fname, long fesize) {
#ifdef DEBUG
    face.log("got_file");
#endif
}

void yahoohook::contact_added(guint32 id, char *myid, char *who, char *msg) {
    string text = _("The user has added you to his/her contact list");

    if(msg)
    if(strlen(msg)) {
	text += (string) ", " + _("the message was: ") + msg;
    }

    em.store(imnotification(imcontact(who, yahoo), text));
}

void yahoohook::typing_notify(guint32 id, char *who, int stat) {
    icqcontact *c = clist.get(imcontact(who, yahoo));
    static map<string, int> st;

    if(c) {
	if(st[who] != stat) {
	    face.log(stat ? _("+ [yahoo] %s: started typing") : _("+ [yahoo] %s: stopped typing"), who);
	    if(c->getstatus() == offline) {
		c->setstatus(available);
		face.relaxedupdate();
	    }

	    st[who] = stat;
	}
    }
}

void yahoohook::game_notify(guint32 id, char *who, int stat) {
#ifdef DEBUG
    face.log("game_notify");
#endif
}

void yahoohook::mail_notify(guint32 id, char *from, char *subj, int cnt) {
    char buf[1024];

    sprintf(buf, _("+ [yahoo] e-mail from %s, %s"), from, subj);
    face.log(buf);

    clist.get(contactroot)->playsound(imevent::email);
}

void yahoohook::system_message(guint32 id, char *msg) {
    face.log(_("+ [yahoo] system: %s"), msg);
}

void yahoohook::error(guint32 id, char *err, int fatal) {
    if(fatal) {
	face.log(_("+ [yahoo] error: %s"), err);
	yhook.disconnected();
    }
}

void yahoohook::add_input(guint32 id, int fd) {
#ifdef DEBUG
    face.log("add_input");
#endif
}

void yahoohook::remove_input(guint32 id, int fd) {
#ifdef DEBUG
    face.log("remove_input");
#endif
}
