/*
*
* centericq livejournal protocol handling class (sick)
* $Id: ljhook.cc,v 1.11 2003/10/12 23:28:20 konst Exp $
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

#ifdef BUILD_LJ

#include "ljhook.h"
#include "rsshook.h"
#include "icqface.h"
#include "eventmanager.h"

#include <sys/utsname.h>

ljhook lhook;

#define KOI2UTF(x) siconv(x, conf.getrussian(proto) ? "koi8-u" : conf.getdefcharset(), "utf8")
#define UTF2KOI(x) siconv(x, "utf8", conf.getrussian(proto) ? "koi8-u" : conf.getdefcharset())

ljhook::ljhook(): abstracthook(livejournal), fonline(false), sdest(0) {
    fcapabs.insert(hookcapab::nochat);
}

ljhook::~ljhook() {
}

void ljhook::init() {
    manualstatus = conf.getstatus(proto);
    httpcli.messageack.connect(this, &ljhook::messageack_cb);
    httpcli.socket.connect(this, &ljhook::socket_cb);
#ifdef DEBUG
    httpcli.logger.connect(this, &ljhook::logger_cb);
#endif
}

void ljhook::connect() {
    icqconf::imaccount acc = conf.getourid(proto);

    baseurl = acc.server + ":" + i2str(acc.port) + "/interface/flat";
    md5pass = getmd5(acc.password);

    face.log(_("+ [lj] connecting to the server"));

    httpcli.setProxyServerHost(conf.gethttpproxyhost());
    httpcli.setProxyServerPort(conf.gethttpproxyport());

    HTTPRequestEvent *ev = new HTTPRequestEvent(baseurl, HTTPRequestEvent::POST);
    ev->addParam("mode", "login");
    ev->addParam("user", username = acc.nickname);
    ev->addParam("hpassword", md5pass);

    struct utsname un;
    string clientver;

    if(!uname(&un)) clientver = un.sysname;
	else clientver = "GNU";

    clientver += string("-") + PACKAGE + "/" + VERSION;

    ev->addParam("clientversion", clientver);

    self = imcontact(username + "@lj", livejournal);

    fonline = true;
    httpcli.SendEvent(ev);
    sent[ev] = reqLogin;
}

void ljhook::disconnect() {
    if(fonline) {
	fonline = false;
	face.log(_("+ [lj] disconnected"));
	if(flogged) {
	    flogged = false;
	    icqcontact *c = clist.get(self);
	    if(c) c->setstatus(offline);
	}
    }
}

void ljhook::exectimers() {
    if(logged())
    if(timer_current-timer_getfriends >= 120) {
	requestfriends();
	timer_getfriends = timer_current;
    }
}

void ljhook::main() {
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
		httpcli.socket_cb(*i, SocketEvent::READ);
		break;
	    }
	}

	for(i = wfds.begin(); i != wfds.end(); ++i) {
	    if(FD_ISSET(*i, &ws)) {
		httpcli.socket_cb(*i, SocketEvent::WRITE);
		break;
	    }
	}

	for(i = efds.begin(); i != efds.end(); ++i) {
	    if(FD_ISSET(*i, &es)) {
		httpcli.socket_cb(*i, SocketEvent::EXCEPTION);
		break;
	    }
	}
    }
}

void ljhook::getsockets(fd_set &rs, fd_set &ws, fd_set &es, int &hsocket) const {
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

bool ljhook::isoursocket(fd_set &rs, fd_set &ws, fd_set &es) const {
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

bool ljhook::online() const {
    return fonline;
}

bool ljhook::logged() const {
    return fonline && flogged;
}

bool ljhook::isconnecting() const {
    return fonline && !flogged;
}

bool ljhook::enabled() const {
    return true;
}

bool ljhook::send(const imevent &ev) {
    if(ev.gettype() == imevent::message) {
	const immessage *m = static_cast<const immessage *> (&ev);
	HTTPRequestEvent *ev = new HTTPRequestEvent(baseurl, HTTPRequestEvent::POST);

	ev->addParam("mode", "postevent");
	ev->addParam("user", username);
	ev->addParam("hpassword", md5pass);
	ev->addParam("event", KOI2UTF(m->gettext()));

	time_t t = m->gettimestamp();
	struct tm *tm = localtime(&t);

	ev->addParam("year", i2str(tm->tm_year+1900));
	ev->addParam("mon", i2str(tm->tm_mon+1));
	ev->addParam("day", i2str(tm->tm_mday));
	ev->addParam("hour", i2str(tm->tm_hour));
	ev->addParam("min", i2str(tm->tm_min));

	ev->addParam("lineendings", "unix");
	ev->addParam("subject", "");

	httpcli.SendEvent(ev);
	sent[ev] = reqPost;
	return true;
    }

    return false;
}

void ljhook::sendnewuser(const imcontact &ic) {
    int npos;
    icqcontact *c;

    if(logged())
    if(c = clist.get(ic))
    if((npos = c->getnick().find("@lj")) != -1) {
	HTTPRequestEvent *ev = new HTTPRequestEvent(baseurl, HTTPRequestEvent::POST);

	ev->addParam("mode", "editfriends");
	ev->addParam("user", username);
	ev->addParam("hpassword", md5pass);

	ev->addParam("editfriend_add_1_user", c->getnick().substr(0, npos));
	httpcli.SendEvent(ev);
	sent[ev] = reqAddFriend;
    }
}

void ljhook::removeuser(const imcontact &ic) {
    int npos;
    icqcontact *c;

    if(logged())
    if(c = clist.get(ic))
    if((npos = c->getnick().find("@lj")) != -1) {
	string nick = c->getnick().substr(0, npos);

	if(c->getworkinfo().homepage == getfeedurl(nick)) {
	    HTTPRequestEvent *ev = new HTTPRequestEvent(baseurl, HTTPRequestEvent::POST);

	    ev->addParam("mode", "editfriends");
	    ev->addParam("user", username);
	    ev->addParam("hpassword", md5pass);

	    ev->addParam((string) "editfriend_delete_" + nick, "1");

	    httpcli.SendEvent(ev);
	    sent[ev] = reqDelFriend;
	}
    }
}

void ljhook::setautostatus(imstatus st) {
    if(st == offline) {
	if(getstatus() != offline) disconnect();

    } else {
	if(getstatus() == offline) connect();

    }
}

imstatus ljhook::getstatus() const {
    return flogged ? available : offline;
}

void ljhook::requestinfo(const imcontact &ic) {
    int npos;
    icqcontact *c = clist.get(ic);

    if(c) {
	icqcontact::moreinfo m = c->getmoreinfo();
	if((npos = c->getnick().find("@lj")) != -1) {
	    m.homepage = getfeedurl(c->getnick().substr(0, npos));
	    c->setmoreinfo(m);
	}
    }
}

void ljhook::lookup(const imsearchparams &params, verticalmenu &dest) {
    while(!foundguys.empty()) {
	delete foundguys.back();
	foundguys.pop_back();
    }

    HTTPRequestEvent *ev = new HTTPRequestEvent(getfeedurl(params.nick));
    httpcli.SendEvent(ev);
    sent[ev] = reqLookup;

    sdest = &dest;
    lookfor = params.nick;
}

void ljhook::stoplookup() {
    sdest = 0;
}

void ljhook::requestawaymsg(const imcontact &c) {
}

void ljhook::requestversion(const imcontact &c) {
}

void ljhook::ping(const imcontact &c) {
}

void ljhook::ouridchanged(const icqconf::imaccount &ia) {
}

void ljhook::updatecontact(icqcontact *c) {
}

// ----------------------------------------------------------------------------

void ljhook::requestfriends() {
    HTTPRequestEvent *ev = new HTTPRequestEvent(baseurl, HTTPRequestEvent::POST);

    ev->addParam("mode", "getfriends");
    ev->addParam("user", username);
    ev->addParam("hpassword", md5pass);
    ev->addParam("includefriendof", "1");
    ev->addParam("includebdays", "1");

    httpcli.SendEvent(ev);
    sent[ev] = reqGetFriends;
}

// ----------------------------------------------------------------------------

void ljhook::socket_cb(SocketEvent *ev) {
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

void ljhook::messageack_cb(MessageEvent *ev) {
    HTTPRequestEvent *rev = dynamic_cast<HTTPRequestEvent*>(ev);

    if(!rev) return;

    map<HTTPRequestEvent *, RequestType>::iterator ie = sent.find(rev);

    if(ie == sent.end()) return;

    int npos;
    string content = rev->getContent(), pname;
    map<string, string> params;
    icqcontact *c;

    if(ie->second != reqLookup) {
	if(!ev->isDelivered() || content.empty()) {
	    pname = rev->getHTTPResp();
	    if(pname.empty()) pname = _("cannot connect");
	    face.log(_("+ [lj] HTTP failed: %s"), pname.c_str());
	    disconnect();
	    return;
	}

	while((npos = content.find("\r")) != -1)
	    content.erase(npos, 1);

	while((npos = content.find("\n")) != -1) {
	    pname = content.substr(0, npos);
	    content.erase(0, npos+1);

	    if((npos = content.find("\n")) == -1)
		npos = content.size();

	    params[pname] = content.substr(0, npos);

	    if(npos != content.size()) content.erase(0, npos+1);
		else content = "";
	}
    }

    if(isconnecting() && ie->second == reqLogin) {
	fonline = true;

	if(params["success"] == "OK") {
	    flogged = true;
	    face.log(_("+ [lj] logged in"));
	    face.update();
	    setautostatus(manualstatus);

	    if(!(c = clist.get(self))) {
		c = clist.addnew(self, false);
		requestinfo(self);
	    }

	    icqcontact::basicinfo bi = c->getbasicinfo();

	    string fullname = params["name"];
	    bi.fname = getword(fullname);
	    bi.lname = fullname;

	    c->setbasicinfo(bi);
	    c->setstatus(available);

	    string buf;
	    friendof.clear();
	    while(!(buf = getword(bi.zip)).empty())
		friendof.push_back(buf);

	    if(!params["message"].empty())
		em.store(imnotification(self, _("Message from the server: ") + params["message"]));

	    timer_getfriends = 0;

	} else {
	    face.log(_("+ [lj] login failed: %s"), params["errmsg"].c_str());
	}
    } else if(ie->second == reqPost) {
	if(params["success"] == "OK") {
	    face.log(_("+ [lj] posted successully, the id is %s"), params["itemid"].c_str());
	} else {
	    face.log(_("+ [lj] post error: %s"), params["errmsg"].c_str());
	}

    } else if(ie->second == reqGetFriends) {
	if(params["success"] == "OK") {
	    int fcount = atoi(params["friend_count"].c_str()), i, k;
	    string username, name, birthday, bd;
	    icqcontact::basicinfo bi;
	    icqcontact::moreinfo mi;

	    for(i = 1; i <= fcount; i++) {
		username = params[(string) "friend_" + i2str(i) + "_user"];
		name = UTF2KOI(params[(string) "friend_" + i2str(i) + "_name"]);
		birthday = params[(string) "friend_" + i2str(i) + "_birthday"];

		bool found = false;

		for(k = 0; k < clist.count && !found; k++) {
		    c = (icqcontact *) clist.at(k);
		    if(c->getdesc().pname == rss) {
			found = (c->getworkinfo().homepage == getfeedurl(username));
		    }
		}

		if(!found)
		if(c = clist.addnew(imcontact(0, rss), false)) {
		    icqcontact::workinfo wi = c->getworkinfo();
		    wi.homepage = getfeedurl(username);
		    c->setworkinfo(wi);

		    icqcontact::moreinfo mi = c->getmoreinfo();
		    mi.checkfreq = 120;
		    c->setmoreinfo(mi);

		    c->setnick(username + "@lj");
		    c->setdispnick(c->getnick());
		    c->save();
		}

		bi = c->getbasicinfo();
		bi.fname = getword(name);
		bi.lname = name;
		c->setbasicinfo(bi);

		if(!birthday.empty()) {
		    mi = c->getmoreinfo();
		    k = 0;

		    while(!(bd = getrword(birthday, "-")).empty())
		    switch(k++) {
			case 0: mi.birth_day = atoi(bd.c_str()); break;
			case 1: mi.birth_month = atoi(bd.c_str()); break;
			case 2: mi.birth_year = atoi(bd.c_str()); break;
		    }

		    c->setmoreinfo(mi);
		}
	    }

	    fcount = atoi(params["friendof_count"].c_str());
	    bool foempty = friendof.empty();

	    map<string, string> nfriendof;
	    map<string, string>::const_iterator in;
	    vector<string>::iterator il;
	    char buf[512];

	    for(i = 1; i <= fcount; i++) {
		username = params[(string) "friendof_" + i2str(i) + "_user"];
		name = UTF2KOI(params[(string) "friendof_" + i2str(i) + "_name"]);
		nfriendof[username] = name;
	    }

	    for(in = nfriendof.begin(); in != nfriendof.end(); ++in)
	    if(find(friendof.begin(), friendof.end(), in->first) == friendof.end()) {
		friendof.push_back(in->first);

		if(!foempty) {
		    bd = (string) "http://" + conf.getourid(proto).server + "/users/" + in->first;

		    sprintf(buf, _("The user %s (%s) has added you to his/her friend list\n\nJournal address: %s"),
			in->first.c_str(), in->second.c_str(), bd.c_str());

		    em.store(imnotification(self, buf));
		}
	    }

	    for(il = friendof.begin(); il != friendof.end(); ) {
		if(nfriendof.find(*il) == nfriendof.end()) {
		    bd = (string) "http://" + conf.getourid(proto).server + "/users/" + *il;
		    sprintf(buf, _("The user %s has removed you from his/her friend list\n\nJournal address: %s"),
			il->c_str(), bd.c_str());
		    em.store(imnotification(self, buf));
		    friendof.erase(il);
		    il = friendof.begin();
		} else {
		    ++il;
		}
	    }

	    if(c = clist.get(self)) {
		bi = c->getbasicinfo();
		vector<string>::const_iterator ifo = friendof.begin();
		bi.zip = "";

		while(ifo != friendof.end()) {
		    bi.zip += *ifo + " ";
		    ++ifo;
		}

		c->setbasicinfo(bi);
	    }
	}

    } else if(ie->second == reqDelFriend) {
	if(params["success"] != "OK") {
	    face.log(_("+ [lj] error deleting friend"));
	} else {
	    face.log(_("+ [lj] the user has been removed from your friend list"));
	}

    } else if(ie->second == reqAddFriend) {
	if(params["success"] != "OK" || !atoi(params["friends_added"].c_str())) {
	    face.log(_("+ [lj] couldn't add friend"));
	} else {
	    face.log(_("+ [lj] %s was added to friends"), params["friend_1_user"].c_str());
	}

    } else if(ie->second == reqLookup && sdest) {
	if(ev->isDelivered() && !content.empty()) {
	    imcontact ic(0, rss);
	    ic.nickname = rev->getURL();
	    c = new icqcontact(ic);

	    c->setnick(lookfor + "@lj");
	    c->setdispnick(c->getnick());
	    c->excludefromlist();
	    rsshook::parsedocument(rev, c);

	    sdest->additem(conf.getcolor(cp_clist_rss), c, (string) " " + c->getnick());
	    foundguys.push_back(c);
	}

	face.findready();
	face.log(_("+ [lj] user lookup finished"));
	sdest->redraw();
	sdest = 0;

    }

    if(ie != sent.end()) sent.erase(ie);
}

void ljhook::logger_cb(LogEvent *ev) {
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

#endif

string ljhook::getfeedurl(const string &nick) const {
    return (string) "http://" + conf.getourid(proto).server + "/users/" + nick + "/rss/";
}
