/*
*
* centericq yahoo! protocol handling class
* $Id: yahoohook.cc,v 1.112 2004/12/20 00:54:02 konst Exp $
*
* Copyright (C) 2003-2004 by Konstantin Klyagin <konst@konst.org.ua>
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

#include "yahoo2.h"
#include "connwrap.h"

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PERIOD_REFRESH          60
#define PERIOD_CLOSE            6

int yahoohook::yfd::connection_tags = 0;

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
    fcapabs.insert(hookcapab::directadd);
    fcapabs.insert(hookcapab::files);
    fcapabs.insert(hookcapab::conferencing);
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
    c.ext_yahoo_got_identities = &got_identities;
    c.ext_yahoo_got_ignore = &got_ignore;
    c.ext_yahoo_got_cookies = &got_cookies;
    c.ext_yahoo_chat_cat_xml = &chat_cat_xml;
    c.ext_yahoo_chat_join = &chat_join;
    c.ext_yahoo_chat_userjoin = &chat_userjoin;
    c.ext_yahoo_chat_userleave = &chat_userleave;
    c.ext_yahoo_chat_message = &chat_message;
    c.ext_yahoo_rejected = &rejected;
    c.ext_yahoo_typing_notify = &typing_notify;
    c.ext_yahoo_got_webcam_image = &got_webcam_image;
    c.ext_yahoo_webcam_invite = &webcam_invite;
    c.ext_yahoo_webcam_invite_reply = &webcam_invite_reply;
    c.ext_yahoo_webcam_closed = &webcam_closed;
    c.ext_yahoo_webcam_viewer = &webcam_viewer;
    c.ext_yahoo_webcam_data_request = &webcam_data_request;
    c.ext_yahoo_got_search_result = &got_search_result;
    c.ext_yahoo_log = &ylog;

    yahoo_register_callbacks(&c);
}

void yahoohook::connect() {
    icqconf::imaccount acc = conf.getourid(proto);
    int r;

    log(logConnecting);

    if(cid < 0) {
	yahoo_logoff(cid);
	yahoo_close(cid);
    }

    cid = yahoo_init_with_attributes(acc.nickname.c_str(), acc.password.c_str(),
	"pager_host", acc.server.c_str(),
	"pager_port", acc.port, 0);

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
		if(i->isconnect) {
		    connect_complete(i->data, i->fd);
		} else {
		    yahoo_write_ready(cid, i->fd, i->data);
		}
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
	time(&timer_close);
    }
}

void yahoohook::disconnected() {
    if(logged()) {
	rfds.clear();
	wfds.clear();

	logger.putourstatus(proto, getstatus(), ourstatus = offline);
	clist.setoffline(proto);
	fonline = false;
	log(logDisconnected);
	face.update();
    }
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
	if(timer_current-timer_refresh > PERIOD_REFRESH) {
	    yahoo_refresh(cid);
	    timer_refresh = timer_current;

	} else if(timer_close && timer_current-timer_close > PERIOD_CLOSE) {
	    yahoo_close(cid);
	    disconnected();

	}
    }
}

struct tm *yahoohook::timestamp() {
    return localtime(&timer_current);
}

bool yahoohook::send(const imevent &ev) {
    icqcontact *c = clist.get(ev.getcontact());
    string text;

    if(c) {
	if(ev.gettype() == imevent::message) {
	    const immessage *m = static_cast<const immessage *>(&ev);
	    if(m) text = rushtmlconv("ku", m->gettext());

	} else if(ev.gettype() == imevent::url) {
	    const imurl *m = static_cast<const imurl *>(&ev);
	    if(m) text = rushtmlconv("ku", m->geturl()) + "\n\n" + rusconv("kw", m->getdescription());

	} else if(ev.gettype() == imevent::file) {
	    const imfile *m = static_cast<const imfile *>(&ev);
	    vector<imfile::record> files = m->getfiles();
	    vector<imfile::record>::const_iterator ir;

	    for(ir = files.begin(); ir != files.end(); ++ir) {
		sfiles.push_back(strdup(ir->fname.c_str()));
		srfiles[sfiles.back()] = *m;

		yahoo_send_file(cid, ev.getcontact().nickname.c_str(),
		    m->getmessage().c_str(), justfname(ir->fname).c_str(),
		    ir->size, &get_fd, sfiles.back());
	    }

	    return true;
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
		if(report) log(logContactAdd, ic.nickname.c_str());
		vector<icqgroup>::const_iterator ig = find(groups.begin(), groups.end(), clist.get(ic)->getgroupid());
		if(ig != groups.end()) {
		    yahoo_add_buddy(cid, ic.nickname.c_str(), ig->getname().c_str(), "");
		}
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
	    if(report) log(logContactRemove, ic.nickname.c_str());

	    const YList *buddies = yahoo_get_buddylist(cid);
	    const YList *bud = 0;

	    for(bud = buddies; bud; bud = y_list_next(bud)) {
		if(ic.nickname == ((yahoo_buddy *) bud->data)->id)
		    yahoo_remove_buddy(cid,
			((yahoo_buddy *) bud->data)->id,
			((yahoo_buddy *) bud->data)->group);
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
		auto_ptr<char> msg(strdup(rusconv("ku", conf.getawaymsg(proto)).c_str()));
		yahoo_set_away(cid, (yahoo_status) stat2int[st], msg.get(), 1);

	    }
	}
    }
}

imstatus yahoohook::getstatus() const {
    return online() ? ourstatus : offline;
}

void yahoohook::requestinfo(const imcontact &ic) {
    requestfromfound(ic);

    icqcontact *c = clist.get(ic);
    if(!c) c = clist.get(contactroot);

    icqcontact::moreinfo m = c->getmoreinfo();
    icqcontact::basicinfo b = c->getbasicinfo();

    m.homepage = "http://profiles.yahoo.com/" + ic.nickname;
    b.email = ic.nickname + "@yahoo.com";

    c->setmoreinfo(m);
    c->setbasicinfo(b);
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

string yahoohook::decode(string text, bool utf) {
    int npos, mpos;

    text = rushtmlconv(utf ? "uk" : "wk", text);

    while((npos = text.find("\e[")) != -1) {
	if((mpos = text.substr(npos).find("m")) == -1)
	    mpos = text.size()-npos-1;

	text.erase(npos, mpos+1);
    }

    return text;
}

bool yahoohook::knowntransfer(const imfile &fr) const {
    return fvalid.find(fr) != fvalid.end();
}

void yahoohook::replytransfer(const imfile &fr, bool accept, const string &localpath) {
    if(accept) {
	string localname = localpath + "/";
	localname += fr.getfiles()[0].fname;
	sfiles.push_back(strdup(localname.c_str()));
	srfiles[sfiles.back()] = fr;
	yahoo_get_url_handle(cid, fvalid[fr].c_str(), &get_url, sfiles.back());

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

    while(!foundguys.empty()) {
	delete foundguys.back();
	foundguys.pop_back();
    }

    yahoo_search_gender gender = YAHOO_GENDER_NONE;

    switch(params.gender) {
	case genderMale: gender = YAHOO_GENDER_MALE; break;
	case genderFemale: gender = YAHOO_GENDER_FEMALE; break;
    }

    searchonlineonly = params.onlineonly;
    searchdest = &dest;

    if(!params.kwords.empty()) {
	yahoo_search(cid, YAHOO_SEARCH_KEYWORD, decode(params.kwords, false).c_str(),
		gender, YAHOO_AGERANGE_NONE, params.photo ? 1 : 0, 1);

    } else if(!params.firstname.empty()) {
	yahoo_search(cid, YAHOO_SEARCH_NAME, decode(params.firstname, false).c_str(),
		gender, YAHOO_AGERANGE_NONE, params.photo ? 1 : 0, 1);

    } else if(!params.room.empty()) {
	room = params.room.substr(1);
	i = confmembers[room].begin();

	while(i != confmembers[room].end()) {
	    if(c = clist.get(imcontact(*i, proto)))
		searchdest->additem(conf.getcolor(cp_clist_yahoo),
		    c, (string) " " + *i);

	    ++i;
	}

	face.findready();
	log(logConfMembers, searchdest->getcount());

	searchdest->redraw();
	searchdest = 0;

    }
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
    bool found = false;
    icqcontact *c = clist.get(ic);

    if(c)
    if(c->inlist()) {
	const YList *buddies = yahoo_get_buddylist(cid);

	for(const YList *bud = buddies; bud && !found; ) {
	    yahoo_buddy *yb = (yahoo_buddy *) bud->data;
	    found = (c->getdesc().nickname == yb->id);
	    bud = y_list_next(bud);
	}

	if(!found) sendnewuser(ic, false);
    }
}

void yahoohook::updatecontact(icqcontact *c) {
    if(logged() && conf.getgroupmode() != icqconf::nogroups) {
	bool found = false;
	const YList *buddies = yahoo_get_buddylist(cid);
	string newgroupname = groups.getname(c->getgroupid());

	for(const YList *bud = buddies; bud; ) {
	    yahoo_buddy *yb = (yahoo_buddy *) bud->data;

	    if(c->getdesc().nickname == yb->id) {
		if(!found) {
		    if(newgroupname != yb->group)
			yahoo_change_buddy_group(cid, yb->id, yb->group, newgroupname.c_str());

		    found = true;
		} else {
		    yahoo_remove_buddy(cid, yb->id, yb->group);
		}
	    }

	    bud = y_list_next(bud);
	}
    }
}

void yahoohook::renamegroup(const string &oldname, const string &newname) {
    if(logged()) {
	string tempname = oldname + "___" + i2str(getpid()) + "_temp";
	yahoo_group_rename(cid, oldname.c_str(), tempname.c_str());
	yahoo_group_rename(cid, tempname.c_str(), newname.c_str());
    }
}

// ----------------------------------------------------------------------------

void yahoohook::login_response(int id, int succ, char *url) {
    vector<string>::iterator in;

    switch(succ) {
	case YAHOO_LOGIN_OK:
	    yhook.flogged = true;
	    logger.putourstatus(yahoo, offline, yhook.ourstatus = yhook.manualstatus);
	    yhook.log(logLogged);
	    time(&yhook.timer_refresh);
	    yhook.setautostatus(yhook.manualstatus);
	    yhook.timer_close = 0;
	    break;

	case YAHOO_LOGIN_PASSWD:
	    yhook.fonline = yhook.fonline = false;
	    yahoo_close(yhook.cid);
	    face.log(_("+ [yahoo] cannot login: username and password mismatch"));
	    break;

	case YAHOO_LOGIN_LOCK:
	    yhook.fonline = yhook.fonline = false;
	    yahoo_close(yhook.cid);
	    face.log(_("+ [yahoo] cannot login: the account has been blocked"));
	    face.log(_("+ to reactivate visit %s"), url);
	    break;

	case YAHOO_LOGIN_DUPL:
	    face.log(_("+ [yahoo] another logon detected"));
	    yahoo_logoff(yhook.cid);
	    yhook.manualstatus = offline;
	    break;

	case YAHOO_LOGIN_SOCK:
	    face.log(_("+ [yahoo] server closed socket"));
	    yhook.disconnected();
	    break;
    }

    face.update();
}

void yahoohook::got_buddies(int id, YList *buds) {
    const YList *buddy;

    for(buddy = buds; buddy; buddy = y_list_next(buddy)) {
	yahoo_buddy *bud = static_cast<yahoo_buddy *>(buddy->data);
	clist.updateEntry(imcontact(bud->id, yahoo), bud->group ? bud->group : "");
    }
}

void yahoohook::got_identities(int id, YList *buds) {
}

void yahoohook::status_changed(int id, char *who, int stat, char *msg, int away) {
    yhook.userstatus(who, stat, msg ? msg : "", (bool) away);
}

void yahoohook::got_im(int id, char *me, char *who, char *msg, long tm, int stat, int utf8) {
    imcontact ic(who, yahoo);
    string text = cuthtml(msg, chCutBR | chLeaveLinks);

    yhook.checkinlist(ic);
    text = yhook.decode(text, utf8);

    if(!text.empty()) {
	em.store(immessage(ic, imevent::incoming, text, tm));
    }
}

void yahoohook::got_search_result(int id, int found, int start, int total, YList *contacts) {
    yahoo_found_contact *fc;
    icqcontact *c;
    YList *ir;
    string sg, line;

    if(!yhook.searchdest)
	return;

    for(ir = contacts; ir; ir = ir->next) {
	fc = (yahoo_found_contact *) ir->data;

	if(yhook.searchonlineonly && !fc->online)
	    continue;

	c = new icqcontact(imcontact(fc->id, yahoo));

	icqcontact::basicinfo binfo = c->getbasicinfo();
	icqcontact::moreinfo minfo = c->getmoreinfo();

	if(!fc->id || !fc->gender)
	    continue;

	c->setnick(fc->id);
	c->setdispnick(c->getnick());

	sg = up(fc->gender);
	if(sg == "MALE") minfo.gender = genderMale;
	else if(sg == "FEMALE") minfo.gender = genderFemale;

	minfo.age = fc->age;
	if(fc->location) binfo.street = fc->location;

	line = (fc->online ? "o " : "  ") + c->getnick();

	if(fc->age || fc->location || strlen(fc->gender) >= 4) {
	    line += " <";
	    if(fc->age) line += i2str(fc->age);

	    if(strlen(fc->gender) >= 4) {
		if(fc->age) line += ", ";
		line += fc->gender;
	    }

	    if(fc->location && strlen(fc->location)) {
		if(fc->age || strlen(fc->gender) >= 4) line += ", ";
		line += fc->location;
	    }

	    line += ">";
	}

	c->setbasicinfo(binfo);
	c->setmoreinfo(minfo);

	yhook.foundguys.push_back(c);
	yhook.searchdest->additem(conf.getcolor(cp_clist_yahoo), c, line);
    }

    yhook.searchdest->redraw();

    if(start + found >= total) {
	face.findready();
	yhook.log(logSearchFinished, yhook.foundguys.size());
	yhook.searchdest = 0;
    } else {
	yahoo_search_again(yhook.cid, -1);
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
	string id = (char *) m->data;

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

    string text = (string) who + ": " + cuthtml(msg, chCutBR | chLeaveLinks);
    text = yhook.decode(text, utf8);

    if(c) em.store(immessage(c, imevent::incoming, text));
}

void yahoohook::got_file(int id, char *me, char *who, char *url, long expires, char *msg, char *fname, unsigned long fesize) {
    if(!who || !url)
	return;

    if(!fname) {
	fname = strrchr(url, '/');
	if(fname) fname++;
	    else return;
    }

    int pos;
    imfile::record r;
    r.fname = fname;
    r.size = fesize;
    if(!fesize) r.size = -1;

    if((pos = r.fname.find('?')) != -1)
	r.fname.erase(pos);

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

void yahoohook::typing_notify(int id, char *me, char *who, int stat) {
    icqcontact *c = clist.get(imcontact(who, yahoo));
    if(c) c->setlasttyping(stat ? timer_current : 0);
}

void yahoohook::game_notify(int id, char *me, char *who, int stat) {
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

void yahoohook::error(int id, char *err, int fatal, int num) {
    if(fatal) {
	face.log(_("+ [yahoo] error: %s"), err);
	yhook.disconnected();
    }
}

int yahoohook::add_handler(int id, int fd, yahoo_input_condition cond, void *data) {
    int tag = -1;

    switch(cond) {
	case YAHOO_INPUT_READ:
	    yhook.rfds.push_back(yfd(fd, data));
	    tag = yhook.rfds.back().tag;
	    break;
	case YAHOO_INPUT_WRITE:
	    yhook.wfds.push_back(yfd(fd, data));
	    tag = yhook.wfds.back().tag;
	    break;
    }

    return tag;
}

void yahoohook::remove_handler(int id, int tag) {
    vector<yfd>::iterator i;

    i = find(yhook.rfds.begin(), yhook.rfds.end(), tag);
    if(i != yhook.rfds.end())
	yhook.rfds.erase(i);

    i = find(yhook.wfds.begin(), yhook.wfds.end(), tag);
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

    int f = fcntl(servfd, F_GETFL);
    fcntl(servfd, F_SETFL, f | O_NONBLOCK);

    error = cw_connect(servfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr), 0);

    if(!error) {
	callback(servfd, 0, data);
	return 0;
    } else if(error == -1 && errno == EINPROGRESS) {
	ccd = new connect_callback_data;
	ccd->callback = callback;
	ccd->callback_data = data;
	ccd->id = id;

	yhook.wfds.push_back(yfd(servfd, ccd, true));
	ccd->tag = yhook.wfds.back().tag;
	return 1;
    } else {
	close(servfd);
	return -1;
    }
}

void yahoohook::connect_complete(void *data, int source) {
    connect_callback_data *ccd = (connect_callback_data *) data;
    int so_error;
    socklen_t err_size = sizeof(so_error);

    remove_handler(0, ccd->tag);

    if(getsockopt(source, SOL_SOCKET, SO_ERROR, (char *) &so_error, &err_size) == -1 || so_error != 0) {
	if(yhook.logged())
	    face.log(_("+ [yahoo] direct connection failed"));

	close(source);
	source = -1;
    } else {
	ccd->callback(source, so_error, ccd->callback_data);
    }

    free(ccd);
}

void yahoohook::got_ignore(int id, YList * igns) {
}

void yahoohook::got_cookies(int id) {
}

void yahoohook::chat_cat_xml(int id, char *xml) {
}

void yahoohook::chat_join(int id, char *room, char *topic, YList *members, int fd) {
}

void yahoohook::chat_userjoin(int id, char *room, struct yahoo_chat_member *who) {
}

void yahoohook::chat_userleave(int id, char *room, char *who) {
}

void yahoohook::chat_message(int id, char *who, char *room, char *msg, int msgtype, int utf8) {
}

void yahoohook::rejected(int id, char *who, char *msg) {
}

void yahoohook::got_webcam_image(int id, const char * who, const unsigned char *image, unsigned int image_size, unsigned int real_size, unsigned int timestamp) {
}

void yahoohook::webcam_invite(int id, char *me, char *from) {
}

void yahoohook::webcam_invite_reply(int id, char *me, char *from, int accept) {
}

void yahoohook::webcam_closed(int id, char *who, int reason) {
}

void yahoohook::webcam_viewer(int id, char *who, int connect) {
}

void yahoohook::webcam_data_request(int id, int send) {
}

int yahoohook::ylog(char *fmt, ...) {
    if(conf.getdebug()) {
	char buf[512];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	face.log(buf);
    }

    return 0;
}

void yahoohook::get_fd(int id, int fd, int error, void *data) {
    const char *fname = (const char *) data;
    char buf[1024];
    int size = 0;

    if(!error) {
	ifstream f(fname);

	if(f.is_open()) {
	    while(!f.eof()) {
		f.read(buf, 1024);
		write(fd, buf, f.tellg());
		size += f.tellg();
	    }
	    f.close();
	}
    }

    map<const char *, imfile>::iterator ir = yhook.srfiles.find(fname);
    if(ir != yhook.srfiles.end()) {
	face.transferupdate(fname, ir->second,
	    error ? icqface::tsError : icqface::tsFinish,
	    size, size);

	yhook.srfiles.erase(ir);
    }

    vector<char *>::iterator i = find(yhook.sfiles.begin(), yhook.sfiles.end(), fname);
    if(i != yhook.sfiles.end()) {
	delete *i;
	yhook.sfiles.erase(i);
    }
}

void yahoohook::get_url(int id, int fd, int error, const char *filename, unsigned long size, void *data) {
    int rsize = 0;
    const char *localname = (const char *) data;

    if(!error) {
	vector<char *>::iterator i = find(yhook.sfiles.begin(), yhook.sfiles.end(), localname);
	if(i != yhook.sfiles.end()) {
	    ofstream f(localname);

	    if(f.is_open()) {
		int r;
		char buf[1024];
		FILE *fp = fdopen(fd, "r");

		while(!feof(fp)) {
		    r = fread(buf, 1, 1024, fp);
		    if(r > 0) f.write(buf, r);
		    rsize += r;
		}

		fclose(fp);
		f.close();
	    } else {
		error = -1;
	    }

	    delete *i;
	    yhook.sfiles.erase(i);
	}
    }

    map<const char *, imfile>::iterator ir = yhook.srfiles.find(localname);
    if(ir != yhook.srfiles.end()) {
	face.transferupdate(justfname(localname), ir->second,
	    error ? icqface::tsError : icqface::tsFinish,
	    size, rsize);

	yhook.srfiles.erase(ir);
    }

}

#endif
