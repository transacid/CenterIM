/*
*
* centericq yahoo! protocol handling class
* $Id: yahoohook.cc,v 1.78 2003/05/06 20:27:30 konst Exp $
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
#include "imlogger.h"

#include "accountmanager.h"
#include "eventmanager.h"

extern "C" {
#include "yahoo2_callbacks.h"
};

#define PERIOD_REFRESH          60

char pager_host[64], pager_port[64], filetransfer_host[64], filetransfer_port[64];

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

yahoohook::yahoohook() : abstracthook(yahoo), fonline(false) {
    fcapabs.insert(hookcapab::setaway);
    fcapabs.insert(hookcapab::fetchaway);
    fcapabs.insert(hookcapab::synclist);
    fcapabs.insert(hookcapab::files);
    fcapabs.insert(hookcapab::conferencing);
    fcapabs.insert(hookcapab::directadd);
    fcapabs.insert(hookcapab::conferencesaretemporary);

    pager_host[0] = pager_port[0] = filetransfer_host[0] = filetransfer_port[0] = 0;
}

yahoohook::~yahoohook() {
}

void yahoohook::init() {
    manualstatus = conf.getstatus(proto);

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
    c.yahoo_game_notify = &game_notify;
    c.yahoo_mail_notify = &mail_notify;
    c.yahoo_system_message = &system_message;
    c.yahoo_error = &error;
    c.yahoo_add_input = &add_input;
    c.yahoo_remove_input = &remove_input;

    bool lts, lo, lt;
    conf.getlogoptions(lts, lo, lt);
    if(lt)
	c.yahoo_typing_notify = &typing_notify;

    yahoo_register_callbacks(&c);
}

void yahoohook::connect() {
    icqconf::imaccount acc = conf.getourid(proto);
    int r;

    strcpy(pager_host, acc.server.c_str());
    strcpy(pager_port, i2str(acc.port).c_str());

    strcpy(filetransfer_host, "filetransfer.msg.yahoo.com");
    strcpy(filetransfer_port, "80");

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
    logger.putourstatus(proto, getstatus(), ourstatus = offline);
    clist.setoffline(proto);
    userenc.clear();
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

    if(logged()) {
	time_t tcurrent = time(0);

	if(tcurrent-timer_refresh > PERIOD_REFRESH) {
	    yahoo_refresh(cid);
	    timer_refresh = tcurrent;
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
	auto_ptr<char> who(strdup(ev.getcontact().nickname.c_str()));

	if(ev.gettype() == imevent::message) {
	    const immessage *m = static_cast<const immessage *>(&ev);
	    if(m) text = rushtmlconv("kw", m->gettext());

	} else if(ev.gettype() == imevent::url) {
	    const imurl *m = static_cast<const imurl *>(&ev);
	    if(m) text = rushtmlconv("kw", m->geturl()) + "\n\n" + rusconv("kw", m->getdescription());

	} else if(ev.gettype() == imevent::file) {
	    const imfile *m = static_cast<const imfile *>(&ev);
	    vector<imfile::record> files = m->getfiles();
	    vector<imfile::record>::const_iterator ir;

	    for(ir = files.begin(); ir != files.end(); ++ir) {
		FILE *fp;
		imfile::record r;

		r.fname = ir->fname;
		r.size = ir->size;

		imfile fr(c->getdesc(), imevent::outgoing, "",
		    vector<imfile::record>(1, r));

		auto_ptr<char> msg(strdup(m->getmessage().c_str()));
		auto_ptr<char> fname(strdup(justfname(r.fname).c_str()));

		if(fp = fopen(r.fname.c_str(), "r")) {
		    yahoo_send_file(cid, who.get(), msg.get(), fname.get(), r.size, fileno(fp));
		    fclose(fp);
		}
	    }

	    return true;
	}

	for(is = text.begin(); is != text.end(); ++is)
	    if((unsigned) *is < 32) *is = ' ';

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
    sendnewuser(ic, true);
}

void yahoohook::sendnewuser(const imcontact &ic, bool report) {
    char *group;

    if(online() && !ischannel(ic)) {
	if(logged()) {
	    bool found = false;
	    struct yahoo_buddy **buddies = get_buddylist(cid), **bud;

	    auto_ptr<char> who(strdup(ic.nickname.c_str()));
	    auto_ptr<char> group(strdup("centericq"));

	    for(bud = buddies; bud && *bud && !found; bud++)
		found = !strcmp((*bud)->id, who.get());

	    if(!found) {
		if(report)
		    face.log(_("+ [yahoo] adding %s to the contacts"), ic.nickname.c_str());

		yahoo_add_buddy(cid, who.get(), group.get());
	    }
	}
    }

    requestinfo(ic);
}

void yahoohook::removeuser(const imcontact &ic) {
    removeuser(ic, true);
}

void yahoohook::removeuser(const imcontact &ic, bool report) {
    if(logged()) {
	auto_ptr<char> who(strdup(ic.nickname.c_str()));

	if(!ischannel(ic)) {
	    if(report)
		face.log(_("+ [yahoo] removing %s from the contacts"),
		    ic.nickname.c_str());

	    struct yahoo_buddy **buddies = get_buddylist(cid), **bud;

	    for(bud = buddies; bud && *bud; bud++) {
		if(!strcmp((*bud)->id, who.get())) {
		    auto_ptr<char> grp(strdup((*bud)->group));
		    yahoo_remove_buddy(cid, who.get(), grp.get());
		    bud = buddies;
		}
	    }
	} else {
	    if(report)
		face.log(_("+ [yahoo] leaving the %s conference"),
		    ic.nickname.c_str());

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
	case YAHOO_STATUS_BUSY:
	    st = dontdisturb;
	    break;
	case YAHOO_STATUS_CUSTOM:
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
	    logger.putourstatus(proto, getstatus(), ourstatus = st);

	    if(st == freeforchat) {
		auto_ptr<char> msg(strdup("free for chat"));
		yahoo_set_away(cid, (yahoo_status) stat2int[st], msg.get(), 0);

	    } else if(st != away) {
		yahoo_set_away(cid, (yahoo_status) stat2int[st], 0, 0);

	    } else {
		auto_ptr<char> msg(strdup(rusconv("kw", conf.getawaymsg(proto)).c_str()));
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
    imcontact ic(nick, proto);
    icqcontact *c = clist.get(ic);

    awaymessages[nick] = rusconv("wk", message);

    if(!c) {
	c = clist.addnew(ic, false);
    }

    if(c) {
	logger.putonline(ic, c->getstatus(), yahoo2imstatus(st));

	c->setstatus(yahoo2imstatus(st));

	if(c->getstatus() == offline) {
	    map<string, Encoding>::iterator i = userenc.find(nick);
	    if(i != userenc.end()) userenc.erase(i);
	}
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

	if(c->getdesc().pname == proto) {
	    for(found = false, bud = buddies; bud && *bud && !found; bud++) {
		found = c->getdesc().nickname == (string) (*bud)->id;
	    }

	    if(!found)
		r.push_back(c);
	}
    }

    return r;
}

string yahoohook::encanalyze(const string &nick, const string &text) {
    map<string, Encoding>::const_iterator i = userenc.find(nick);

    if(i == userenc.end() || userenc[nick] == encUnknown)
	userenc[nick] = guessencoding(text);

    switch(yhook.userenc[nick]) {
	case encUTF: return siconv(text, "utf8", conf.getrussian(proto) ? "koi8-u" : DEFAULT_CHARSET);
	default: return rushtmlconv("wk", text);
    }

    return text;
}

bool yahoohook::knowntransfer(const imfile &fr) const {
    return fvalid.find(fr) != fvalid.end();
}

void yahoohook::replytransfer(const imfile &fr, bool accept, const string &localpath) {
    if(accept) {
	conf.openurl(fvalid[fr]);
	face.transferupdate(fr.getfiles().begin()->fname, fr, icqface::tsFinish, 0, 0);

    } else {
	fvalid.erase(fr);
    }
}

void yahoohook::aborttransfer(const imfile &fr) {
    face.transferupdate(fr.getfiles().begin()->fname, fr, icqface::tsCancel, 0, 0);
    fvalid.erase(fr);
}

void yahoohook::lookup(const imsearchparams &params, verticalmenu &dest) {
    string room;
    icqcontact *c;
    vector<string>::const_iterator i;

    searchdest = &dest;
    room = params.room.substr(1);

    if(!params.room.empty()) {
	i = confmembers[room].begin();

	while(i != confmembers[room].end()) {
	    if(c = clist.get(imcontact(*i, proto)))
		searchdest->additem(conf.getcolor(cp_clist_yahoo),
		    c, (string) " " + *i);

	    ++i;
	}
    }

    face.findready();

    face.log(_("+ [yahoo] members list fetching finished, %d found"),
	searchdest->getcount());

    searchdest->redraw();
    searchdest = 0;
}

void yahoohook::conferencecreate(const imcontact &confid, const vector<imcontact> &lst) {
    int i;
    string room = confid.nickname.substr(1);

    char **who = new char*[lst.size()+1];
    who[lst.size()] = 0;

    vector<imcontact>::const_iterator il = lst.begin();
    while(il != lst.end()) {
	who[il-lst.begin()] = strdup(il->nickname.c_str());
	++il;
    }

    auto_ptr<char> cname(strdup(room.c_str()));
    yahoo_conference_invite(cid, who, cname.get(), _("Please join my conference."));

    for(i = 0; who[i]; delete who[i++]);
    delete who;
}

void yahoohook::requestawaymsg(const imcontact &ic) {
    icqcontact *c = clist.get(ic);

    if(c) {
	if(awaymessages.find(ic.nickname) != awaymessages.end()) {
	    em.store(imnotification(ic, string() + _("Custom status message:") + "\n\n" +
		awaymessages[ic.nickname]));

	} else {
	    face.log(_("+ [yahoo] cannot fetch away msg from %s, %s"),
		c->getdispnick().c_str(), ic.totext().c_str());

	}
    }
}

void yahoohook::checkinlist(imcontact ic) {
    icqcontact *c = clist.get(ic);
    vector<icqcontact *> notremote = getneedsync();

    if(c)
    if(c->inlist())
    if(find(notremote.begin(), notremote.end(), c) != notremote.end())
	sendnewuser(ic, false);
}

// ----------------------------------------------------------------------------

void yahoohook::login_done(guint32 id, int succ, char *url) {
    vector<string>::iterator in;

    switch(succ) {
	case YAHOO_LOGIN_OK:
	    yhook.flogged = true;
	    logger.putourstatus(yahoo, offline, yhook.ourstatus = yhook.manualstatus);
	    face.log(_("+ [yahoo] logged in"));

	    for(int i = 0; i < clist.count; i++) {
		icqcontact *c = (icqcontact *) clist.at(i);
		imcontact ic = c->getdesc();

		if(ic.pname == yahoo)
		if(!c->inlist()) {
		    yhook.sendnewuser(ic);
		}
	    }

	    time(&yhook.timer_refresh);
	    yhook.setautostatus(yhook.manualstatus);
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
	    if(in != nicks.end()) nicks.erase(in);
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
    string text = cuthtml(msg, true);

    yhook.checkinlist(ic);
    text = yhook.encanalyze(who, text);

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

    inviter = confname.substr(1);
    if((i = inviter.rfind("-")) != -1) {
	inviter.erase(i);
    }

    sprintf(buf, _("The user %s has invited you to the %s conference, the topic there is: %s"),
	yhook.rusconv("wk", inviter).c_str(),
	yhook.rusconv("wk", room).c_str(),
	yhook.rusconv("wk", msg).c_str());

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

    c->setstatus(available);
    em.store(imnotification(cont, text));
    em.store(imnotification(cont, _("Auto-joined the conference")));

    yhook.tobedone.push_back(make_pair(tbdConfLogon, room));
}

void yahoohook::conf_userdecline(guint32 id, char *who, char *room, char *msg) {
    icqcontact *c = clist.get(imcontact((string) "#" + room, yahoo));
    char buf[512];

    if(c) {
	sprintf(buf, _("The user %s has declined your invitation to join the conference"), who);
	em.store(imnotification(c, buf));
    }
}

void yahoohook::conf_userjoin(guint32 id, char *who, char *room) {
    icqcontact *c = clist.get(imcontact((string) "#" + room, yahoo));
    char buf[512];

    if(c) {
	sprintf(buf, _("The user %s has joined the conference"), who);

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
	sprintf(buf, _("The user %s has left the conference"), who);
	em.store(imnotification(c, buf));

	im = find(yhook.confmembers[room].begin(), yhook.confmembers[room].end(), who);
	if(im != yhook.confmembers[room].end()) yhook.confmembers[room].erase(im);
    }
}

void yahoohook::conf_message(guint32 id, char *who, char *room, char *msg) {
    icqcontact *c = clist.get(imcontact((string) "#" + room, yahoo));

    string text = (string) who + ": " + cuthtml(msg, true);
    text = yhook.encanalyze(who, text);

    if(c) em.store(immessage(c, imevent::incoming, text));
}

void yahoohook::got_file(guint32 id, char *who, char *url, long expires, char *msg, char *fname, long fesize) {
    imfile::record r;
    r.fname = fname;
    r.size = fesize;

    imfile fr(imcontact(who, yahoo), imevent::incoming, "",
	vector<imfile::record>(1, r));

    yhook.fvalid[fr] = url;
    em.store(fr);

    face.transferupdate(fname, fr, icqface::tsInit, fesize, 0);
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
	    st[who] = stat;
	}
    }
}

void yahoohook::game_notify(guint32 id, char *who, int stat) {
}

void yahoohook::mail_notify(guint32 id, char *from, char *subj, int cnt) {
    char buf[1024];

    if(from && subj) {
	sprintf(buf, _("+ [yahoo] e-mail from %s, %s"), from, subj);
	face.log(buf);
	clist.get(contactroot)->playsound(imevent::email);
    }
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
}

void yahoohook::remove_input(guint32 id, int fd) {
}
