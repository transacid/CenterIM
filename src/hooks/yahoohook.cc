/*
*
* centericq yahoo! protocol handling class
* $Id: yahoohook.cc,v 1.81 2003/09/26 07:13:24 konst Exp $
*
* Copyright (C) 2003 by Konstantin Klyagin <konst@konst.org.ua>
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

#ifdef BUILD_YAHOO

#include "yahoohook.h"
#include "icqmlist.h"
#include "icqface.h"
#include "imlogger.h"

#include "icqcontacts.h"
#include "icqgroups.h"

#include "accountmanager.h"
#include "eventmanager.h"

#include "libyahoo2.h"

#include <netdb.h>
#include <sys/socket.h>

#define PERIOD_REFRESH          60

char pager_host[255], pager_port[255], filetransfer_host[255],
    filetransfer_port[255], webcam_host[255], webcam_port[255],
    local_host[255], webcam_description[255];

int conn_type;

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

yahoohook::yahoohook() : abstracthook(yahoo), fonline(false), cid(0) {
    fcapabs.insert(hookcapab::setaway);
    fcapabs.insert(hookcapab::fetchaway);
    fcapabs.insert(hookcapab::synclist);
    fcapabs.insert(hookcapab::files);
    fcapabs.insert(hookcapab::conferencing);
    fcapabs.insert(hookcapab::directadd);
    fcapabs.insert(hookcapab::conferencesaretemporary);

    pager_host[0] = pager_port[0] = filetransfer_host[0] =
	filetransfer_port[0] = webcam_host[0] = webcam_port[0] =
	local_host[0] = 0;
}

yahoohook::~yahoohook() {
}

void yahoohook::init() {
    manualstatus = conf.getstatus(proto);

    static struct yahoo_callbacks c;

    memset(&c, 0, sizeof(c));

    c.ext_yahoo_login_response = &login_response;
    c.ext_yahoo_got_buddies = &got_buddies;
    c.ext_yahoo_status_changed = &status_changed;
    c.ext_yahoo_got_im = &got_im;
    c.ext_yahoo_got_conf_invite = &got_conf_invite;
    c.ext_yahoo_conf_userdecline = &conf_userdecline;
    c.ext_yahoo_conf_userjoin = &conf_userjoin;
    c.ext_yahoo_conf_userleave = &conf_userleave;
    c.ext_yahoo_conf_message = &conf_message;
    c.ext_yahoo_got_file = &got_file;
    c.ext_yahoo_contact_added = &contact_added;
    c.ext_yahoo_game_notify = &game_notify;
    c.ext_yahoo_mail_notify = &mail_notify;
    c.ext_yahoo_system_message = &system_message;
    c.ext_yahoo_error = &error;
    c.ext_yahoo_add_handler = &add_handler;
    c.ext_yahoo_remove_handler = &remove_handler;
    c.ext_yahoo_connect_async = &connect_async;

    bool lts, lo, lt;
    conf.getlogoptions(lts, lo, lt);
    if(lt)
	c.ext_yahoo_typing_notify = &typing_notify;

    yahoo_register_callbacks(&c);
}

void yahoohook::connect() {
    icqconf::imaccount acc = conf.getourid(proto);
    int r;

    strcpy(pager_host, acc.server.c_str());
    strcpy(pager_port, i2str(acc.port).c_str());

    strcpy(filetransfer_host, "filetransfer.msg.yahoo.com");
    strcpy(filetransfer_port, "80");

    strcpy(webcam_host, "webcam.yahoo.com");
    strcpy(webcam_port, "5100");
    strcpy(webcam_description, "Philips ToUcam Pro");
    strcpy(local_host, "");
    conn_type = 1;

    face.log(_("+ [yahoo] connecting to the server"));

    if(cid < 0) {
	yahoo_logoff(cid);
	yahoo_close(cid);
    }

    cid = yahoo_init(acc.nickname.c_str(), acc.password.c_str());
    yahoo_login(cid, stat2int[manualstatus]);

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
    vector<yfd>::iterator i;
    struct timeval tv;
    int hsock;
    fd_set rs, ws, es;

    if(cid < 0) {
	yahoo_input_condition cond = YAHOO_INPUT_READ;
	vector<yfd>::const_iterator ifd = rfds.begin();

	if(ifd == rfds.end()) {
	    ifd = wfds.begin();
	    cond = YAHOO_INPUT_WRITE;
	    if(ifd == wfds.end()) return;
	}

	connect_complete(ifd->data, ifd->fd, cond);
	return;
    }

    FD_ZERO(&rs);
    FD_ZERO(&ws);
    FD_ZERO(&es);

    tv.tv_sec = tv.tv_usec = 0;
    hsock = 0;

    for(i = rfds.begin(); i != rfds.end(); ++i) {
	FD_SET(i->fd, &rs);
	hsock = max(hsock, i->fd);
    }

    for(i = wfds.begin(); i != wfds.end(); ++i) {
	FD_SET(i->fd, &ws);
	hsock = max(hsock, i->fd);
    }

    if(select(hsock+1, &rs, &ws, 0, &tv) > 0) {
	for(i = rfds.begin(); i != rfds.end(); ++i) {
	    if(FD_ISSET(i->fd, &rs)) {
		yahoo_read_ready(cid, i->fd, i->data);
		break;
	    }
	}

	for(i = wfds.begin(); i != wfds.end(); ++i) {
	    if(FD_ISSET(i->fd, &ws)) {
		yahoo_write_ready(cid, i->fd, i->data);
		break;
	    }
	}
    }
}

void yahoohook::getsockets(fd_set &rs, fd_set &ws, fd_set &es, int &hsocket) const {
    if(online()) {
	vector<yfd>::const_iterator i;

	for(i = rfds.begin(); i != rfds.end(); ++i) {
	    hsocket = max(hsocket, i->fd);
	    FD_SET(i->fd, &rs);
	}

	for(i = wfds.begin(); i != wfds.end(); ++i) {
	    hsocket = max(hsocket, i->fd);
	    FD_SET(i->fd, &ws);
	}
    }
}

bool yahoohook::isoursocket(fd_set &rs, fd_set &ws, fd_set &es) const {
    vector<yfd>::const_iterator i;

    for(i = rfds.begin(); i != rfds.end(); ++i)
	if(FD_ISSET(i->fd, &rs))
	    return true;

    for(i = wfds.begin(); i != wfds.end(); ++i)
	if(FD_ISSET(i->fd, &ws))
	    return true;

    return false;
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
		yahoo_conference_logon(cid, 0, getmembers(it->second), room.get());
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
	if(ev.gettype() == imevent::message) {
	    const immessage *m = static_cast<const immessage *>(&ev);
	    if(m) text = rushtmlconv("kw", m->gettext());

	} else if(ev.gettype() == imevent::url) {
	    const imurl *m = static_cast<const imurl *>(&ev);
	    if(m) text = rushtmlconv("kw", m->geturl()) + "\n\n" + rusconv("kw", m->getdescription());

	} else if(ev.gettype() == imevent::file) {
/*
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

		if(fp = fopen(r.fname.c_str(), "r")) {
		    yahoo_send_file(cid, ev.getcontact().nickname.c_str(),
			m->getmessage().c_str(), justfname(r.fname).c_str(),
			r.size, fileno(fp));

		    fclose(fp);
		}
	    }
	    return true;
*/
	}

	if(!ischannel(c)) {
	    yahoo_send_im(cid, 0, ev.getcontact().nickname.c_str(), text.c_str(), 0);
	} else {
	    yahoo_conference_message(cid, 0, getmembers(ev.getcontact().nickname.substr(1)),
		ev.getcontact().nickname.c_str()+1, text.c_str(), 0);
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
    if(online() && !ischannel(ic)) {
	if(logged()) {
	    bool found = false;
	    const YList *buddies = yahoo_get_buddylist(cid);
	    const YList *bud = 0;

	    for(bud = buddies; bud && !found; bud = y_list_next(bud))
		found = ic.nickname == static_cast<yahoo_buddy *>(bud->data)->id;

	    if(!found) {
		if(report)
		    face.log(_("+ [yahoo] adding %s to the contacts"), ic.nickname.c_str());

		string groupname = "centericq";
		vector<icqgroup>::const_iterator ig = find(groups.begin(), groups.end(), clist.get(ic)->getgroupid());
		if(ig != groups.end()) groupname = ig->getname();

		yahoo_add_buddy(cid, ic.nickname.c_str(), groupname.c_str());
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
	if(!ischannel(ic)) {
	    if(report)
		face.log(_("+ [yahoo] removing %s from the contacts"),
		    ic.nickname.c_str());

	    const YList *buddies = yahoo_get_buddylist(cid);
	    const YList *bud = 0;

	    for(bud = buddies; bud; ) {
		if(ic.nickname == static_cast<yahoo_buddy *>(bud->data)->id) {
		    yahoo_remove_buddy(cid,
			static_cast<yahoo_buddy *>(bud->data)->id,
			static_cast<yahoo_buddy *>(bud->data)->group);

		    bud = buddies;
		} else {
		    bud = y_list_next(bud);
		}
	    }
	} else {
	    if(report)
		face.log(_("+ [yahoo] leaving the %s conference"),
		    ic.nickname.c_str());

	    yahoo_conference_logoff(cid, 0, getmembers(ic.nickname.substr(1)), ic.nickname.c_str()+1);
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

YList *yahoohook::getmembers(const string &room) {
    int i;
    static YList *smemb = 0;
    vector<string>::iterator ic;
    map<string, vector<string> >::iterator im;

    if(smemb) {
	for(YList *n = smemb; n; n = y_list_next(n))
	    free(n->data);

	y_list_free(smemb);
	smemb = 0;
    }

    if((im = confmembers.find(room)) != confmembers.end())
    for(ic = im->second.begin(); ic != im->second.end(); ++ic)
	smemb = y_list_append(smemb, strdup(ic->c_str()));

    return smemb;
}

vector<icqcontact *> yahoohook::getneedsync() {
    int i;
    icqcontact *c;
    vector<icqcontact *> r;
    bool found;

    const YList *buddies = yahoo_get_buddylist(cid);
    const YList *bud = 0;

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == proto) {
	    for(found = false, bud = buddies; bud && !found; bud = y_list_next(bud))
		found = c->getdesc().nickname == static_cast<yahoo_buddy *>(bud->data)->id;

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

    YList *who = 0;

    vector<imcontact>::const_iterator il = lst.begin();
    while(il != lst.end()) {
	who = y_list_append(who, strdup(il->nickname.c_str()));
	++il;
    }

    yahoo_conference_invite(cid, 0, who, room.c_str(), _("Please join my conference."));

    for(YList *w = who; w; w = y_list_next(w))
	free(w->data);

    y_list_free(who);
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

void yahoohook::login_response(int id, int succ, char *url) {
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

    if(succ != YAHOO_LOGIN_OK) {
	yhook.cid = -1;
    }

    face.update();
}

void yahoohook::got_buddies(int id, YList *buds) {
    vector<string> nicks;
    vector<string>::iterator in;

    icqconf::imaccount acc = conf.getourid(yahoo);
    icqcontact *c;
    const YList *buddy;

    int i;

    for(buddy = buds; buddy; buddy = y_list_next(buddy)) {
	string id = static_cast<yahoo_buddy*>(buddy->data)->id;
	if(find(nicks.begin(), nicks.end(), id) == nicks.end()) {
	    nicks.push_back(id);
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

void yahoohook::status_changed(int id, char *who, int stat, char *msg, int away) {
    yhook.userstatus(who, stat, msg ? msg : "", (bool) away);
}

void yahoohook::got_im(int id, char *who, char *msg, long tm, int stat, int utf8) {
    imcontact ic(who, yahoo);
    string text = cuthtml(msg, true);

    yhook.checkinlist(ic);
    text = yhook.encanalyze(who, text);

    if(!text.empty()) {
	em.store(immessage(ic, imevent::incoming, text));
    }
}

void yahoohook::got_conf_invite(int id, char *who, char *room, char *msg, YList *members) {
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

    for(YList *m = members; m; m = y_list_next(m)) {
	string id = static_cast<yahoo_buddy *>(m->data)->id;

	if(id != acc.nickname)
	if(find(yhook.confmembers[room].begin(), yhook.confmembers[room].end(), id) == yhook.confmembers[room].end()) {
	    yhook.confmembers[room].push_back(id);
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

void yahoohook::conf_userdecline(int id, char *who, char *room, char *msg) {
    icqcontact *c = clist.get(imcontact((string) "#" + room, yahoo));
    char buf[512];

    if(c) {
	sprintf(buf, _("The user %s has declined your invitation to join the conference"), who);
	em.store(imnotification(c, buf));
    }
}

void yahoohook::conf_userjoin(int id, char *who, char *room) {
    icqcontact *c = clist.get(imcontact((string) "#" + room, yahoo));
    char buf[512];

    if(c) {
	sprintf(buf, _("The user %s has joined the conference"), who);

	if(find(yhook.confmembers[room].begin(), yhook.confmembers[room].end(), who) == yhook.confmembers[room].end())
	    yhook.confmembers[room].push_back(who);

	em.store(imnotification(c, buf));
    }
}

void yahoohook::conf_userleave(int id, char *who, char *room) {
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

void yahoohook::conf_message(int id, char *who, char *room, char *msg, int utf8) {
    icqcontact *c = clist.get(imcontact((string) "#" + room, yahoo));

    string text = (string) who + ": " + cuthtml(msg, true);
    text = yhook.encanalyze(who, text);

    if(c) em.store(immessage(c, imevent::incoming, text));
}

void yahoohook::got_file(int id, char *who, char *url, long expires, char *msg, char *fname, unsigned long fesize) {
    imfile::record r;
    r.fname = fname;
    r.size = fesize;

    imfile fr(imcontact(who, yahoo), imevent::incoming, "",
	vector<imfile::record>(1, r));

    yhook.fvalid[fr] = url;
    em.store(fr);

    face.transferupdate(fname, fr, icqface::tsInit, fesize, 0);
}

void yahoohook::contact_added(int id, char *myid, char *who, char *msg) {
    string text = _("The user has added you to his/her contact list");

    if(msg)
    if(strlen(msg)) {
	text += (string) ", " + _("the message was: ") + msg;
    }

    em.store(imnotification(imcontact(who, yahoo), text));
}

void yahoohook::typing_notify(int id, char *who, int stat) {
    icqcontact *c = clist.get(imcontact(who, yahoo));
    static map<string, int> st;

    if(c) {
	if(st[who] != stat) {
	    face.log(stat ? _("+ [yahoo] %s: started typing") : _("+ [yahoo] %s: stopped typing"), who);
	    st[who] = stat;
	}
    }
}

void yahoohook::game_notify(int id, char *who, int stat) {
}

void yahoohook::mail_notify(int id, char *from, char *subj, int cnt) {
    char buf[1024];

    if(from && subj) {
	sprintf(buf, _("+ [yahoo] e-mail from %s, %s"), from, subj);
	face.log(buf);
	clist.get(contactroot)->playsound(imevent::email);
    }
}

void yahoohook::system_message(int id, char *msg) {
    face.log(_("+ [yahoo] system: %s"), msg);
}

void yahoohook::error(int id, char *err, int fatal) {
    if(fatal) {
	face.log(_("+ [yahoo] error: %s"), err);
	yhook.disconnected();
    }
}

void yahoohook::add_handler(int id, int fd, yahoo_input_condition cond, void *data) {
    switch(cond) {
	case YAHOO_INPUT_READ: yhook.rfds.push_back(yfd(fd, data)); break;
	case YAHOO_INPUT_WRITE: yhook.wfds.push_back(yfd(fd, data)); break;
    }
}

void yahoohook::remove_handler(int id, int fd) {
    vector<yfd>::iterator i;

    i = find(yhook.rfds.begin(), yhook.rfds.end(), fd);
    if(i != yhook.rfds.end())
	yhook.rfds.erase(i);

    i = find(yhook.wfds.begin(), yhook.wfds.end(), fd);
    if(i != yhook.wfds.end())
	yhook.wfds.erase(i);
}

int yahoohook::connect_async(int id, char *host, int port, yahoo_connect_callback callback, void *data) {
    struct sockaddr_in serv_addr;
    static struct hostent *server;
    int servfd;
    struct connect_callback_data * ccd;
    int error;

    if(!(server = gethostbyname(host))) {
	errno = h_errno;
	return -1;
    }

    if((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, *server->h_addr_list, server->h_length);
    serv_addr.sin_port = htons(port);

    error = ::connect(servfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if(!error) {
	callback(servfd, 0, data);
	return 0;
    } else if(error == -1 && errno == EINPROGRESS) {
	ccd = new connect_callback_data;
	ccd->callback = callback;
	ccd->callback_data = data;
	ccd->id = id;

	yhook.add_handler(-1, servfd, YAHOO_INPUT_WRITE, ccd);
	return 1;
    } else {
	close(servfd);
	return -1;
    }
}

void yahoohook::connect_complete(void *data, int source, yahoo_input_condition condition) {
    connect_callback_data *ccd = (connect_callback_data *) data;
    int error, err_size = sizeof(error);

    remove_handler(-1, source);
    getsockopt(source, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&err_size);

    if(error) {
	close(source);
	source = -1;
    }

    ccd->callback(source, error, ccd->callback_data);
    free(ccd);
}

#endif
