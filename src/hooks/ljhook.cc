/*
*
* centericq livejournal protocol handling class (sick)
* $Id: ljhook.cc,v 1.28 2004/11/11 13:42:05 konst Exp $
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

#ifdef BUILD_LJ

#include "ljhook.h"
#include "rsshook.h"
#include "icqface.h"
#include "eventmanager.h"

#include <sys/utsname.h>

ljhook lhook;

#define PERIOD_FRIENDS  3600

ljhook::ljhook(): abstracthook(livejournal), fonline(false), sdest(0) {
    fcapabs.insert(hookcapab::nochat);
}

ljhook::~ljhook() {
}

void ljhook::init() {
    manualstatus = conf.getstatus(proto);
    httpcli.messageack.connect(this, &ljhook::messageack_cb);
    httpcli.socket.connect(this, &ljhook::socket_cb);

    if(conf.getdebug())
	httpcli.logger.connect(this, &ljhook::logger_cb);

    journals = vector<string>(1, conf.getourid(proto).nickname);
}

void ljhook::connect() {
    icqconf::imaccount acc = conf.getourid(proto);

    baseurl = acc.server + ":" + i2str(acc.port) + "/interface/flat";
    md5pass = getmd5(acc.password);

    log(logConnecting);

    httpcli.setProxyServerHost(conf.gethttpproxyhost());
    httpcli.setProxyServerPort(conf.gethttpproxyport());

    if(!conf.gethttpproxyuser().empty()) {
	httpcli.setProxyServerUser(conf.gethttpproxyuser());
	httpcli.setProxyServerPasswd(conf.gethttpproxypasswd());
    }

    HTTPRequestEvent *ev = new HTTPRequestEvent(baseurl, HTTPRequestEvent::POST);
    ev->addParam("mode", "login");
    ev->addParam("user", username = acc.nickname);
    ev->addParam("hpassword", md5pass);
    ev->addParam("getmoods", "1");
    ev->addParam("getpickws", "1");

    struct utsname un;
    string clientver;

    if(!uname(&un)) clientver = un.sysname;
	else clientver = "GNU";

    clientver += string("-") + PACKAGE + "/" + VERSION;

    ev->addParam("clientversion", clientver);

    self = imcontact(username + "@lj", livejournal);

    fonline = true;

    sent[ev] = reqLogin;
    httpcli.SendEvent(ev);

    moods = vector<string>(1, "");
    pictures = vector<string>(1, "");
}

void ljhook::disconnect() {
    if(fonline) {
	fonline = false;
	log(logDisconnected);
	if(flogged) {
	    flogged = false;
	    icqcontact *c = clist.get(self);
	    if(c) c->setstatus(offline);
	}
    }
}

void ljhook::exectimers() {
    if(logged())
    if(timer_current-timer_getfriends >= PERIOD_FRIENDS) {
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

bool ljhook::send(const imevent &asev) {
    if(!logged()) return false;

    bool ret = false;
    imevent *sev;

    if(asev.gettype() == imevent::xml) sev = asev.getevent(); else {
	sev = new imxmlevent(asev.getcontact(), asev.getdirection(), asev.gettext());
	static_cast<imxmlevent *>(sev)->setfield("_eventkind", "posting");

    }

    if(sev->gettype() == imevent::xml) {
	const imxmlevent *m = static_cast<const imxmlevent *> (sev);
	HTTPRequestEvent *ev = 0;

	if(m->getfield("_eventkind") == "posting") {
	    ev = new HTTPRequestEvent(baseurl, HTTPRequestEvent::POST);

	    ev->addParam("mode", "postevent");
	    ev->addParam("user", username);
	    ev->addParam("hpassword", md5pass);
	    ev->addParam("event", rusconv("ku", m->gettext()));
	    ev->addParam("ver", "1");

	    time_t t = m->gettimestamp();
	    struct tm *tm = localtime(&t);

	    ev->addParam("year", i2str(tm->tm_year+1900));
	    ev->addParam("mon", i2str(tm->tm_mon+1));
	    ev->addParam("day", i2str(tm->tm_mday));
	    ev->addParam("hour", i2str(tm->tm_hour));
	    ev->addParam("min", i2str(tm->tm_min));

	    ev->addParam("lineendings", "unix");
	    ev->addParam("security", m->getfield("security"));
	    ev->addParam("allowmask", "0");

	    if(!m->field_empty("journal")) ev->addParam("usejournal", m->getfield("journal"));
	    if(!m->field_empty("subject")) ev->addParam("subject", rusconv("ku", m->getfield("subject")));
	    if(!m->field_empty("mood")) ev->addParam("prop_current_mood", rusconv("ku", m->getfield("mood")));
	    if(!m->field_empty("music")) ev->addParam("prop_current_music", rusconv("ku", m->getfield("music")));
	    if(!m->field_empty("picture")) ev->addParam("prop_picture_keyword", m->getfield("picture"));

	    if(!m->field_empty("preformatted")) ev->addParam("prop_opt_preformatted", "1");
	    if(!m->field_empty("nocomments")) ev->addParam("prop_opt_nocomments", "1");
	    if(!m->field_empty("backdated")) ev->addParam("prop_opt_backdated", "1");
	    if(!m->field_empty("noemail")) ev->addParam("prop_opt_noemail", "1");

	    sent[ev] = reqPost;

	} else if(m->getfield("_eventkind") == "comment") {
	    icqconf::imaccount acc = conf.getourid(proto);

	    string journal = clist.get(sev->getcontact())->getnick();
	    journal.erase(journal.find("@"));

	    ev = new HTTPRequestEvent(acc.server + ":" + i2str(acc.port) + "/talkpost_do.bml", HTTPRequestEvent::POST);

	    ev->addParam("parenttalkid", "0");
	    ev->addParam("itemid", m->getfield("replyto"));
	    ev->addParam("journal", journal);
	    ev->addParam("usertype", "user");
	    ev->addParam("userpost", acc.nickname);
	    ev->addParam("password", acc.password);
	    ev->addParam("do_login", "0");
	    ev->addParam("subject", "");
	    ev->addParam("body", rusconv("ku", sev->gettext()));
	    ev->addParam("subjecticon", "none");
	    ev->addParam("prop_opt_preformatted", "1");
	    ev->addParam("submitpost", "1");
	    ev->addParam("do_spellcheck", "0");

	    sent[ev] = reqPostComment;
	}

	if(ev) {
	    httpcli.SendEvent(ev);
	    ret = true;
	}
    }

    delete sev;
    return ret;
}

void ljhook::sendnewuser(const imcontact &ic) {
    int npos;
    icqcontact *c;

    if(logged())
    if(ic.pname == rss)
    if(c = clist.get(ic))
    if((npos = c->getnick().find("@lj")) != -1) {
	HTTPRequestEvent *ev = new HTTPRequestEvent(baseurl, HTTPRequestEvent::POST);

	ev->addParam("mode", "editfriends");
	ev->addParam("user", username);
	ev->addParam("hpassword", md5pass);

	ev->addParam("editfriend_add_1_user", c->getnick().substr(0, npos));

	sent[ev] = reqAddFriend;
	httpcli.SendEvent(ev);
    }
}

void ljhook::removeuser(const imcontact &ic) {
    int npos;
    icqcontact *c;

    if(logged())
    if(conf.getourid(proto).additional["importfriends"] == "1")
    if(c = clist.get(ic))
    if((npos = c->getnick().find("@lj")) != -1) {
	string nick = c->getnick().substr(0, npos);

	if(c->getworkinfo().homepage == getfeedurl(nick)) {
	    HTTPRequestEvent *ev = new HTTPRequestEvent(baseurl, HTTPRequestEvent::POST);

	    ev->addParam("mode", "editfriends");
	    ev->addParam("user", username);
	    ev->addParam("hpassword", md5pass);

	    ev->addParam((string) "editfriend_delete_" + nick, "1");

	    sent[ev] = reqDelFriend;
	    httpcli.SendEvent(ev);
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

    if(params.reverse) {
	icqcontact *c = clist.get(self);

	if(c) {
	    string lst = c->getbasicinfo().zip, buf;
	    while(!(buf = getword(lst)).empty()) {
		imcontact ic(0, rss);
		ic.nickname = getfeedurl(buf);
		c = new icqcontact(ic);

		c->setnick(buf + "@lj");
		c->setdispnick(c->getnick());
		c->excludefromlist();

		dest.additem(conf.getcolor(cp_clist_rss), c, (string) " " + c->getnick());
		foundguys.push_back(c);
	    }

	    face.findready();
	    face.log(_("+ [lj] user lookup finished"));
	    dest.redraw();
	}

    } else {
	HTTPRequestEvent *ev = new HTTPRequestEvent(getfeedurl(params.nick));

	sent[ev] = reqLookup;
	httpcli.SendEvent(ev);

	sdest = &dest;
	lookfor = params.nick;
    }
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

    sent[ev] = reqGetFriends;
    httpcli.SendEvent(ev);
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

    int npos, count, i, k;
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
	    log(logLogged);
	    face.update();
	    setautostatus(manualstatus);

	    if(!(c = clist.get(self))) {
		c = clist.addnew(self, false);
		requestinfo(self);
	    }

	    count = atoi(params["mood_count"].c_str());
	    for(i = 1; i < count; i++)
		moods.push_back(params[(string) "mood_" + i2str(i) + "_name"]);

	    count = atoi(params["pickw_count"].c_str());
	    for(i = 1; i < count; i++)
		pictures.push_back(params[(string) "pickw_" + i2str(i)]);

	    sort(moods.begin()+1, moods.end());

	    icqcontact::basicinfo bi = c->getbasicinfo();
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

    } else if(ie->second == reqPostComment) {

    } else if(ie->second == reqGetFriends) {
	if(params["success"] == "OK") {
	    journals = vector<string>(1, username);

	    if(conf.getourid(proto).additional["importfriends"] == "1") {
		count = atoi(params["friend_count"].c_str());
	    } else {
		count = 0;
	    }

	    string username, name, birthday, bd;
	    icqcontact::moreinfo mi;
	    icqcontact::basicinfo bi;

	    for(i = 1; i <= count; i++) {
		username = params[(string) "friend_" + i2str(i) + "_user"];
		name = rusconv("uk", params[(string) "friend_" + i2str(i) + "_name"]);
		birthday = params[(string) "friend_" + i2str(i) + "_birthday"];

		bool found = false;

		for(k = 0; k < clist.count && !found; k++) {
		    c = (icqcontact *) clist.at(k);
		    if(c->getdesc().pname == rss) {
			icqcontact::workinfo wi = c->getworkinfo();

			if(wi.homepage == getoldfeedurl(username)) {
			    wi.homepage = getfeedurl(username);
			    c->setworkinfo(wi);
			}

			found = (wi.homepage == getfeedurl(username));
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
		mi = c->getmoreinfo();
		bi.email = name;

		if(!birthday.empty()) {
		    k = 0;

		    while(!(bd = getrword(birthday, "-")).empty())
		    switch(k++) {
			case 0: mi.birth_day = atoi(bd.c_str()); break;
			case 1: mi.birth_month = atoi(bd.c_str()); break;
			case 2: mi.birth_year = atoi(bd.c_str()); break;
		    }

		}

		c->setmoreinfo(mi);
		c->setbasicinfo(bi);

		if(params[(string) "friend_" + i2str(i) + "_type"] == "community")
		    journals.push_back(username);
	    }

	    count = atoi(params["friendof_count"].c_str());
	    bool foempty = friendof.empty();

	    map<string, string> nfriendof;
	    map<string, string>::const_iterator in;
	    vector<string>::iterator il;
	    char buf[512];

	    for(i = 1; i <= count; i++) {
		username = params[(string) "friendof_" + i2str(i) + "_user"];
		name = rusconv("uk", params[(string) "friendof_" + i2str(i) + "_name"]);
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
		icqcontact::basicinfo bi = c->getbasicinfo();
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
	    face.log(ev->getMessage());
	    break;

	default:
	    face.log(ev->getMessage());
	    break;
    }
}

string ljhook::getfeedurl(const string &nick) const {
    return (string) "http://" + conf.getourid(proto).server + "/users/" + nick + "/data/rss?auth=digest";
}

string ljhook::getoldfeedurl(const string &nick) const {
    return (string) "http://" + conf.getourid(proto).server + "/users/" + nick + "/rss/";
}

#endif
