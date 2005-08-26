/*
*
* centericq Jabber protocol handling class
* $Id: jabberhook.cc,v 1.85 2005/08/26 11:01:49 konst Exp $
*
* Copyright (C) 2002-2005 by Konstantin Klyagin <konst@konst.org.ua>
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

#ifdef BUILD_JABBER

#include "jabberhook.h"
#include "icqface.h"
#include "imlogger.h"
#include "eventmanager.h"
#include "icqgroups.h"
#include "impgp.h"

#define DEFAULT_CONFSERV "conference.jabber.org"
#define PERIOD_KEEPALIVE 30

static void jidsplit(const string &jid, string &user, string &host, string &rest) {
    int pos;
    user = jid;

    if((pos = user.find("/")) != -1) {
	rest = user.substr(pos+1);
	user.erase(pos);
    }

    if((pos = user.find("@")) != -1) {
	host = user.substr(pos+1);
	user.erase(pos);
    }
}

static string jidtodisp(const string &jid) {
    string user, host, rest;
    jidsplit(jid, user, host, rest);

    if(!host.empty())
	user += (string) "@" + host;

    return user;
}

// ----------------------------------------------------------------------------

jabberhook jhook;

string jabberhook::jidnormalize(const string &jid) const {
    if(find(agents.begin(), agents.end(), jid) != agents.end())
	return jid;

    string user, host, rest;
    jidsplit(jid, user, host, rest);

    if(host.empty())
	host = conf.getourid(proto).server;

    user += (string) "@" + host;
    if(!rest.empty()) user += (string) "/" + rest;
    return user;
}

jabberhook::jabberhook(): abstracthook(jabber), jc(0), flogged(false), fonline(false) {
    fcapabs.insert(hookcapab::setaway);
    fcapabs.insert(hookcapab::fetchaway);
    fcapabs.insert(hookcapab::authrequests);
    fcapabs.insert(hookcapab::directadd);
    fcapabs.insert(hookcapab::flexiblesearch);
    fcapabs.insert(hookcapab::flexiblereg);
    fcapabs.insert(hookcapab::visibility);
    fcapabs.insert(hookcapab::ssl);
    fcapabs.insert(hookcapab::changedetails);
    fcapabs.insert(hookcapab::conferencing);
    fcapabs.insert(hookcapab::groupchatservices);
    fcapabs.insert(hookcapab::changenick);
    fcapabs.insert(hookcapab::changeabout);
    fcapabs.insert(hookcapab::version);
    fcapabs.insert(hookcapab::pgp);
}

jabberhook::~jabberhook() {
}

void jabberhook::init() {
    manualstatus = conf.getstatus(proto);
}

void jabberhook::connect() {
    icqconf::imaccount acc = conf.getourid(proto);
    string jid = getourjid();


    log(logConnecting);

    auto_ptr<char> cjid(strdup(jid.c_str()));
    auto_ptr<char> cpass(strdup(acc.password.c_str()));
		auto_ptr<char> cserver(strdup(acc.server.c_str()));

    regmode = flogged = fonline = false;

    if(jc) delete jc;

    jc = jab_new(cjid.get(), cpass.get(), cserver.get(), acc.port,
	acc.additional["ssl"] == "1" ? 1 : 0);

    jab_packet_handler(jc, &packethandler);
    jab_state_handler(jc, &statehandler);

    if(conf.getdebug())
	jab_logger(jc, &jlogger);

    if(jc->user) {
	fonline = true;
	jstate = STATE_CONNECTING;
	statehandler(0, -1);
	jab_start(jc);
    }
}

void jabberhook::disconnect() {
    // announce it to  everyone else
    setjabberstatus(offline, "");

    // announce it to the user
    statehandler(jc, JCONN_STATE_OFF);

    // close the connection
    jab_stop(jc);
    delete(jc);
    jc = 0;
}

void jabberhook::exectimers() {
    if(logged()) {
	if(timer_current-timer_keepalive > PERIOD_KEEPALIVE) {
	    jab_send_raw(jc, "  \t  ");
	    timer_keepalive = timer_current;
	}
    }
}

void jabberhook::main() {
    xmlnode x, z;
    char *cid;

    if(jc && jc->state == JCONN_STATE_CONNECTING) {
	jab_start(jc);
	return;
    }
    
    jab_poll(jc, 0);

    if(jstate == STATE_CONNECTING) {
	if(jc) {
	    x = jutil_iqnew(JPACKET__GET, NS_AUTH);
	    cid = jab_getid(jc);
	    xmlnode_put_attrib(x, "id", cid);
	    id = atoi(cid);

	    z = xmlnode_insert_tag(xmlnode_get_tag(x, "query"), "username");
	    xmlnode_insert_cdata(z, jc->user->user, (unsigned) -1);
	    jab_send(jc, x);
	    xmlnode_free(x);

	    jstate = STATE_GETAUTH;
	}

	if(!jc || jc->state == JCONN_STATE_OFF) {
	    face.log(_("+ [jab] unable to connect to the server"));
	    fonline = false;
	}
    }

    if(!jc) {
	statehandler(jc, JCONN_STATE_OFF);

    } else if(jc->state == JCONN_STATE_OFF || jc->fd == -1) {
	statehandler(jc, JCONN_STATE_OFF);

    }
}

void jabberhook::getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const {
    if(jc) {
	if (jc->cw_state & CW_CONNECT_WANT_READ)
	    FD_SET(jc->fd, &rfds);
	else if (jc->cw_state & CW_CONNECT_WANT_WRITE)
	    FD_SET(jc->fd, &wfds);
	else
	    FD_SET(jc->fd, &rfds);
	hsocket = max(jc->fd, hsocket);
    }
}

bool jabberhook::isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const {
    if(jc) return FD_ISSET(jc->fd, &rfds) || FD_ISSET(jc->fd, &wfds);
    return false;
}

bool jabberhook::online() const {
    return fonline;
}

bool jabberhook::logged() const {
    return fonline && flogged;
}

bool jabberhook::isconnecting() const {
    return fonline && !flogged;
}

bool jabberhook::enabled() const {
    return true;
}

bool jabberhook::send(const imevent &ev) {
    icqcontact *c = clist.get(ev.getcontact());
    string text, cname, enc;

    if(c) {
	if(ev.gettype() == imevent::message) {
	    const immessage *m = static_cast<const immessage *>(&ev);
	    text = m->gettext();

	} else if(ev.gettype() == imevent::url) {
	    const imurl *m = static_cast<const imurl *>(&ev);
	    text = m->geturl() + "\n\n" + m->getdescription();

	} else if(ev.gettype() == imevent::authorization) {
	    const imauthorization *m = static_cast<const imauthorization *> (&ev);
	    auto_ptr<char> cjid(strdup(jidnormalize(ev.getcontact().nickname).c_str()));
	    xmlnode x = 0;

	    switch(m->getauthtype()) {
		case imauthorization::Granted:
		    x = jutil_presnew(JPACKET__SUBSCRIBED, cjid.get(), 0);
		    break;
		case imauthorization::Rejected:
		    x = jutil_presnew(JPACKET__UNSUBSCRIBED, cjid.get(), 0);
		    break;
		case imauthorization::Request:
		    x = jutil_presnew(JPACKET__SUBSCRIBE, cjid.get(), 0);
		    break;
	    }

	    if(x) {
		jab_send(jc, x);
		xmlnode_free(x);
	    }

	    return true;
	}

	text = rusconv("ku", text);

#ifdef HAVE_GPGME
	if(pgp.enabled(ev.getcontact())) {
	    enc = pgp.encrypt(text, c->getpgpkey(), proto);
	    text = "This message is encrypted.";
	}
#endif

	auto_ptr<char> cjid(strdup(jidnormalize(c->getdesc().nickname).c_str()));
	auto_ptr<char> ctext(strdup(text.c_str()));

	xmlnode x = jutil_msgnew(TMSG_CHAT, cjid.get(), 0, ctext.get());

	if(ischannel(c)) {
	    xmlnode_put_attrib(x, "type", "groupchat");
	    if(!(cname = c->getdesc().nickname.substr(1)).empty())
		xmlnode_put_attrib(x, "to", cname.c_str());
	}

	if(!enc.empty()) {
	    xmlnode xenc = xmlnode_insert_tag(x, "x");
	    xmlnode_put_attrib(xenc, "xmlns", "jabber:x:encrypted");
	    xmlnode_insert_cdata(xenc, enc.c_str(), (unsigned) -1);
	}

	jab_send(jc, x);
	xmlnode_free(x);

	return true;
    }

    return false;
}

void jabberhook::sendnewuser(const imcontact &ic) {
    sendnewuser(ic, true);
}

void jabberhook::removeuser(const imcontact &ic) {
    removeuser(ic, true);
}

void jabberhook::sendnewuser(const imcontact &ic, bool report) {
    xmlnode x, y, z;
    icqcontact *c;
    string cname;

    if(!logged())
	return;

    if(!ischannel(ic)) {
	auto_ptr<char> cjid(strdup(jidnormalize(ic.nickname).c_str()));
	if(roster.find(cjid.get()) != roster.end()) {
	    return;
	}

	if(report) log(logContactAdd, ic.nickname.c_str());

	x = jutil_presnew(JPACKET__SUBSCRIBE, cjid.get(), 0);
	jab_send(jc, x);
	xmlnode_free(x);

	x = jutil_iqnew(JPACKET__SET, NS_ROSTER);
	y = xmlnode_get_tag(x, "query");
	z = xmlnode_insert_tag(y, "item");
	xmlnode_put_attrib(z, "jid", cjid.get());

	roster[cjid.get()] = "";

	if(c = clist.get(ic)) {
	    vector<icqgroup>::const_iterator ig = find(groups.begin(), groups.end(), c->getgroupid());
	    if(ig != groups.end()) {
		z = xmlnode_insert_tag(z, "group");
		xmlnode_insert_cdata(z, ig->getname().c_str(), (unsigned) -1);
		roster[cjid.get()] = ig->getname();
	    }
	}

	jab_send(jc, x);
	xmlnode_free(x);

	if(c = clist.get(ic)) {
	    imcontact icc(jidtodisp(ic.nickname), proto);
	    if(ic.nickname != icc.nickname) {
		c->setdesc(icc);
	    }

	    c->setnick("");

	    string u, h, r;
	    jidsplit(icc.nickname, u, h, r);
	    c->setdispnick(u);
	}

	requestinfo(c);

    } else {
	if(c = clist.get(ic)) {
	    cname = ic.nickname.substr(1);

	    if(!cname.empty()) {
		cname += "/" + conf.getourid(proto).nickname;

		auto_ptr<char> ccname(strdup(cname.c_str()));
		auto_ptr<char> ourjid(strdup(getourjid().c_str()));

		x = jutil_presnew(JPACKET__UNKNOWN, ccname.get(), 0);
		xmlnode_insert_cdata(xmlnode_insert_tag(x, "status"), "Online", (unsigned) -1);

		jab_send(jc, x);
		xmlnode_free(x);
	    }
	}
    }
}

void jabberhook::removeuser(const imcontact &ic, bool report) {
    xmlnode x, y, z;
    icqcontact *c;
    string cname;

    if(!logged())
	return;

    if(!ischannel(ic)) {
	auto_ptr<char> cjid(strdup(jidnormalize(ic.nickname).c_str()));

	map<string, string>::iterator ir = roster.find(cjid.get());

	if(ir == roster.end()) return;
	    else roster.erase(ir);

	if(find(agents.begin(), agents.end(), cjid.get()) != agents.end()) {
	    if(report) face.log(_("+ [jab] unregistering from the %s agent"), cjid.get());

	    x = jutil_iqnew(JPACKET__SET, NS_REGISTER);
	    xmlnode_put_attrib(x, "to", cjid.get());
	    y = xmlnode_get_tag(x, "query");
	    xmlnode_insert_tag(y, "remove");
	    jab_send(jc, x);
	    xmlnode_free(x);

	}

	if(report) log(logContactRemove, ic.nickname.c_str());

	x = jutil_presnew(JPACKET__UNSUBSCRIBE, cjid.get(), 0);
	jab_send(jc, x);
	xmlnode_free(x);

	x = jutil_iqnew(JPACKET__SET, NS_ROSTER);
	y = xmlnode_get_tag(x, "query");
	z = xmlnode_insert_tag(y, "item");
	xmlnode_put_attrib(z, "jid", cjid.get());
	xmlnode_put_attrib(z, "subscription", "remove");
	jab_send(jc, x);
	xmlnode_free(x);

    } else {
	if(c = clist.get(ic)) {
	    cname = ic.nickname.substr(1);

	    if(!cname.empty()) {
		cname += "/" + conf.getourid(proto).nickname;
		auto_ptr<char> ccname(strdup(cname.c_str()));
		x = jutil_presnew(JPACKET__UNKNOWN, ccname.get(), 0);
		xmlnode_put_attrib(x, "type", "unavailable");
		jab_send(jc, x);
		xmlnode_free(x);

		map<string, vector<string> >::iterator icm = chatmembers.find(ic.nickname);
		if(icm != chatmembers.end()) chatmembers.erase(icm);
	    }
	}
    }
}

void jabberhook::setautostatus(imstatus st) {
    if(st != offline) {
	if(getstatus() == offline) {
	    connect();
	} else {
	    string msg;

	    switch(st) {
		case away:
		case dontdisturb:
		case occupied:
		case notavail:
		    msg = conf.getawaymsg(proto);
	    }

	    setjabberstatus(ourstatus = st, msg);
	}
    } else {
	if(getstatus() != offline) {
	    disconnect();
	}
    }
}

void jabberhook::requestinfo(const imcontact &ic) {
    if(isconnecting() || logged()) {
	vector<agent>::const_iterator ia = find(agents.begin(), agents.end(), ic.nickname);

	if(ia != agents.end()) {
	    icqcontact *c = clist.get(imcontact(ic.nickname, proto));
	    if(c) {
		c->setdispnick(ia->name);
		c->setabout(ia->desc);
	    }

	} else {
	    auto_ptr<char> cjid(strdup(jidnormalize(ic.nickname).c_str()));
	    xmlnode x = jutil_iqnew(JPACKET__GET, NS_VCARD);
	    xmlnode_put_attrib(x, "to", cjid.get());
	    xmlnode_put_attrib(x, "id", "VCARDreq");
	    jab_send(jc, x);
	    xmlnode_free(x);

	}
    }
}

void jabberhook::requestawaymsg(const imcontact &ic) {
    icqcontact *c = clist.get(ic);

    if(c) {
	string am = awaymsgs[ic.nickname];

	if(!am.empty()) {
	    em.store(imnotification(ic, (string) _("Away message:") + "\n\n" + am));
	} else {
	    face.log(_("+ [jab] no away message from %s, %s"),
		c->getdispnick().c_str(), ic.totext().c_str());
	}
    }
}

imstatus jabberhook::getstatus() const {
    return online() ? ourstatus : offline;
}

bool jabberhook::regnick(const string &nick, const string &pass,
const string &serv, string &err) {
    int pos, port;
    string jid = nick + "@" + serv;

    if((pos = jid.find(":")) != -1) {
	port = atoi(jid.substr(pos+1).c_str());
	jid.erase(pos);
    } else {
	port = icqconf::defservers[proto].port;
    }

    regmode = true;

    auto_ptr<char> cjid(strdup(jid.c_str()));
    auto_ptr<char> cpass(strdup(pass.c_str()));
    auto_ptr<char> cserver(strdup(serv.c_str()));

    jc = jab_new(cjid.get(), cpass.get(), cserver.get(), port, 0);

    if(!jc->user) {
	err = _("Wrong nickname given, cannot register");
	fonline = false;
	return false;
    }

    jab_packet_handler(jc, &packethandler);
    jab_state_handler(jc, &statehandler);

    if(conf.getdebug())
	jab_logger(jc, &jlogger);

    jc->cw_state = CW_CONNECT_BLOCKING;
    jab_start(jc);

    if(jc) id = atoi(jab_reg(jc));

    if(!online()) {
	err = _("Unable to connect");

    } else {
	regdone = false;
	regerr = "";

	while(online() && !regdone && regerr.empty()) {
	    main();
	}

	disconnect();
	err = regdone ? "" : regerr;
    }

    return regdone;
}

void jabberhook::setjabberstatus(imstatus st, string msg) {
    xmlnode x = jutil_presnew(JPACKET__UNKNOWN, 0, 0);

    switch(st) {
	case away:
	    xmlnode_insert_cdata(xmlnode_insert_tag(x, "show"), "away", (unsigned) -1);
	    break;

	case occupied:
	case dontdisturb:
	    xmlnode_insert_cdata(xmlnode_insert_tag(x, "show"), "dnd", (unsigned) -1);
	    break;

	case freeforchat:
	    xmlnode_insert_cdata(xmlnode_insert_tag(x, "show"), "chat", (unsigned) -1);
	    break;

	case notavail:
	    xmlnode_insert_cdata(xmlnode_insert_tag(x, "show"), "xa", (unsigned) -1);
	    break;

	case invisible:
	    xmlnode_put_attrib(x, "type", "invisible");
	    break;

	case offline:
	    xmlnode_put_attrib(x, "type", "unavailable");
	    break;

    }

    map<string, string> add = conf.getourid(proto).additional;

    if(!add["prio"].empty())
	xmlnode_insert_cdata(xmlnode_insert_tag(x, "priority"),
	    add["prio"].c_str(), (unsigned) -1);

    if(msg.empty())
	msg = imstatus2str(st);

    xmlnode_insert_cdata(xmlnode_insert_tag(x, "status"),
	rusconv("ku", msg).c_str(), (unsigned) -1);

#ifdef HAVE_GPGME

    if(!add["pgpkey"].empty()) {
	pgp.clearphrase(proto);
	xmlnode sign = xmlnode_insert_tag(x, "x");
	xmlnode_put_attrib(sign, "xmlns", "jabber:x:signed");
	xmlnode_insert_cdata(sign, pgp.sign(msg, add["pgpkey"], proto).c_str(), (unsigned) -1);
    }

#endif

    jab_send(jc, x);
    xmlnode_free(x);

    sendvisibility();

    logger.putourstatus(proto, getstatus(), ourstatus = st);
}

void jabberhook::sendvisibility() {
    xmlnode x;
    icqlist::iterator i;

    for(i = lst.begin(); i != lst.end(); ++i)
    if(i->getdesc().pname == proto) {
	x = jutil_presnew(JPACKET__UNKNOWN, 0, 0);
	xmlnode_put_attrib(x, "to", jidnormalize(i->getdesc().nickname).c_str());

	if(i->getstatus() == csvisible && ourstatus == invisible) {

	} else if(i->getstatus() == csvisible && ourstatus != invisible) {

	} else if(i->getstatus() == csinvisible && ourstatus == invisible) {

	} else if(i->getstatus() == csinvisible && ourstatus != invisible) {
	    xmlnode_put_attrib(x, "type", "unavailable");

	}

	jab_send(jc, x);
	xmlnode_free(x);
    }
}

vector<string> jabberhook::getservices(servicetype::enumeration st) const {
    vector<agent>::const_iterator ia = agents.begin();
    vector<string> r;

    agent::param_type pt;

    switch(st) {
	case servicetype::search:
	    pt = agent::ptSearch;
	    break;
	case servicetype::registration:
	    pt = agent::ptRegister;
	    break;
	case servicetype::groupchat:
	    while(ia != agents.end()) {
		if(ia->type == agent::atGroupchat) r.push_back(ia->jid);
		++ia;
	    }
	default:
	    return r;
    }

    while(ia != agents.end()) {
	if(ia->params[pt].enabled) r.push_back(ia->name);
	++ia;
    }

    return r;
}

vector<pair<string, string> > jabberhook::getservparams(const string &agentname, agent::param_type pt) const {
    vector<agent>::const_iterator ia = agents.begin();

    while(ia != agents.end()) {
	if(ia->name == agentname)
	if(ia->params[pt].enabled)
	    return ia->params[pt].paramnames;

	++ia;
    }

    return vector<pair<string, string> >();
}

vector<pair<string, string> > jabberhook::getsearchparameters(const string &agentname) const {
    return getservparams(agentname, agent::ptSearch);
}

vector<pair<string, string> > jabberhook::getregparameters(const string &agentname) const {
    return getservparams(agentname, agent::ptRegister);
}

void jabberhook::gotagentinfo(xmlnode x) {
    xmlnode y;
    string name, data, ns;
    agent::param_type pt;
    vector<agent>::iterator ia = jhook.agents.begin();
    const char *from = xmlnode_get_attrib(x, "from"), *p;

    if(!from) return;

    while(ia != jhook.agents.end()) {
	if(ia->jid == from)
	if(y = xmlnode_get_tag(x, "query")) {
	    p = xmlnode_get_attrib(y, "xmlns"); if(p) ns = p;

	    if(ns == NS_SEARCH) pt = agent::ptSearch; else
	    if(ns == NS_REGISTER) pt = agent::ptRegister; else
		break;

	    ia->params[pt].enabled = true;
	    ia->params[pt].paramnames.clear();

	    for(y = xmlnode_get_firstchild(y); y; y = xmlnode_get_nextsibling(y)) {
		p = xmlnode_get_name(y); name = p ? p : "";
		p = xmlnode_get_data(y); data = p ? p : "";

		if(name == "instructions") ia->params[pt].instruction = data; else
		if(name == "key") ia->params[pt].key = data; else
		if(!name.empty() && name != "registered") {
		    ia->params[pt].paramnames.push_back(make_pair(name, data));
		}
	    }

	    if(ia->params[pt].paramnames.empty()) agents.erase(ia);
	    break;
	}
	++ia;
    }

}

void jabberhook::lookup(const imsearchparams &params, verticalmenu &dest) {
    xmlnode x, y;

    searchdest = &dest;

    while(!foundguys.empty()) {
	delete foundguys.back();
	foundguys.pop_back();
    }

    if(!params.service.empty()) {
	x = jutil_iqnew(JPACKET__SET, NS_SEARCH);
	xmlnode_put_attrib(x, "id", "Lookup");

	y = xmlnode_get_tag(x, "query");

	vector<agent>::const_iterator ia = agents.begin();
	while(ia != agents.end()) {
	    if(ia->name == params.service) {
		xmlnode_put_attrib(x, "to", ia->jid.c_str());
		xmlnode_insert_cdata(xmlnode_insert_tag(y, "key"),
		    ia->params[agent::atSearch].key.c_str(), (unsigned int) -1);
		break;
	    }
	    ++ia;
	}

	vector<pair<string, string> >::const_iterator ip = params.flexparams.begin();
	while(ip != params.flexparams.end()) {
	    xmlnode_insert_cdata(xmlnode_insert_tag(y,
		ip->first.c_str()), ip->second.c_str(), (unsigned int) -1);
	    ++ip;
	}

	jab_send(jc, x);
	xmlnode_free(x);

    } else if(!params.room.empty()) {
	icqcontact *c;
	string room = params.room.substr(1);

	if(c = clist.get(imcontact(params.room, proto))) {
	    vector<string>::const_iterator im = chatmembers[room].begin();
	    while(im != chatmembers[room].end()) {
		foundguys.push_back(c = new icqcontact(imcontact(*im, proto)));
		searchdest->additem(conf.getcolor(cp_clist_jabber), c, (string) " " + *im);
		++im;
	    }
	}

	face.findready();
	log(logConfMembers, foundguys.size());

	searchdest->redraw();
	searchdest = 0;
    }
}

void jabberhook::renamegroup(const string &oldname, const string &newname) {
    map<string, string>::iterator ir = roster.begin();

    while(ir != roster.end()) {
	if(ir->second == oldname) {
	    icqcontact *c = clist.get(imcontact(jidtodisp(ir->first), proto));
	    if(c) {
		updatecontact(c);
		ir->second = newname;
	    }
	}

	++ir;
    }
}

void jabberhook::ouridchanged(const icqconf::imaccount &ia) {
    if(logged()) {
	setautostatus(ourstatus);
	// send a new presence
    }
}

// ----------------------------------------------------------------------------

void jabberhook::gotsearchresults(xmlnode x) {
    xmlnode y, z;
    const char *jid, *nick, *first, *last, *email;
    icqcontact *c;

    if(!searchdest)
	return;

    if(y = xmlnode_get_tag(x, "query"))
    for(y = xmlnode_get_tag(y, "item"); y; y = xmlnode_get_nextsibling(y)) {
	jid = xmlnode_get_attrib(y, "jid");
	nick = first = last = email = 0;

	z = xmlnode_get_tag(y, "nick"); if(z) nick = xmlnode_get_data(z);
	z = xmlnode_get_tag(y, "first"); if(z) first = xmlnode_get_data(z);
	z = xmlnode_get_tag(y, "last"); if(z) last = xmlnode_get_data(z);
	z = xmlnode_get_tag(y, "email"); if(z) email = xmlnode_get_data(z);

	if(jid) {
	    c = new icqcontact(imcontact(jidnormalize(jid), proto));
	    icqcontact::basicinfo cb = c->getbasicinfo();

	    if(nick) {
		c->setnick(nick);
		c->setdispnick(c->getnick());
	    }

	    if(first) cb.fname = first;
	    if(last) cb.lname = last;
	    if(email) cb.email = email;
	    c->setbasicinfo(cb);

	    foundguys.push_back(c);

	    string line = (string) " " + c->getnick();
	    if(line.size() > 12) line.resize(12);
	    else line += string(12-line.size(), ' ');
	    line += " " + cb.fname + " " + cb.lname;
	    if(!cb.email.empty()) line += " <" + cb.email + ">";

	    searchdest->additem(conf.getcolor(cp_clist_jabber), c, line);
	}
    }

    face.findready();

    log(logSearchFinished, foundguys.size());

    searchdest->redraw();
    searchdest = 0;
}

void jabberhook::gotloggedin() {
    xmlnode x;

    x = jutil_iqnew(JPACKET__GET, NS_AGENTS);
    xmlnode_put_attrib(x, "id", "Agent List");
    jab_send(jc, x);
    xmlnode_free(x);

    x = jutil_iqnew(JPACKET__GET, NS_ROSTER);
    xmlnode_put_attrib(x, "id", "Roster");
    jab_send(jc, x);
    xmlnode_free(x);
}

void jabberhook::gotroster(xmlnode x) {
    xmlnode y, z;
    imcontact ic;
    icqcontact *c;

    for(y = xmlnode_get_tag(x, "item"); y; y = xmlnode_get_nextsibling(y)) {
	const char *alias = xmlnode_get_attrib(y, "jid");
	const char *sub = xmlnode_get_attrib(y, "subscription");
	const char *name = xmlnode_get_attrib(y, "name");
	const char *group = 0;

	z = xmlnode_get_tag(y, "group");
	if(z) group = xmlnode_get_data(z);

	if(alias) {
	    ic = imcontact(jidtodisp(alias), proto);
	    clist.updateEntry(ic, group ? group : "");

	    if(c = clist.get(ic)) {
		if(name) c->setdispnick(rusconv("uk", name)); else {
		    string u, h, r;
		    jidsplit(alias, u, h, r);
		    c->setdispnick(u);
		}
	    }

	    roster[jidnormalize(alias)] = group ? group : "";
	}
    }

    postlogin();
}

void jabberhook::postlogin() {
    int i;
    icqcontact *c;

    flogged = true;
    ourstatus = available;
    time(&timer_keepalive);

    log(logLogged);
    setautostatus(jhook.manualstatus);
    face.update();

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == proto)
	if(ischannel(c))
	if(c->getbasicinfo().requiresauth)
	    c->setstatus(available);
    }

    agents.insert(agents.begin(), agent("vcard", "Jabber VCard", "", agent::atStandard));
    agents.begin()->params[agent::ptRegister].enabled = true;

    string buf;
    ifstream f(conf.getconfigfname("jabber-infoset").c_str());

    if(f.is_open()) {
	icqcontact *c = clist.get(contactroot);

	c->clear();
	icqcontact::basicinfo bi = c->getbasicinfo();
	icqcontact::reginfo ri = c->getreginfo();

	ri.service = agents.begin()->name;
	getstring(f, buf); c->setnick(buf);
	getstring(f, buf); bi.email = buf;
	getstring(f, buf); bi.fname = buf;
	getstring(f, buf); bi.lname = buf;
	f.close();

	c->setbasicinfo(bi);
	c->setreginfo(ri);

	sendupdateuserinfo(*c);
	unlink(conf.getconfigfname("jabber-infoset").c_str());
    }
}

void jabberhook::conferencecreate(const imcontact &confid, const vector<imcontact> &lst) {
    auto_ptr<char> jcid(strdup(confid.nickname.substr(1).c_str()));
    xmlnode x = jutil_presnew(JPACKET__UNKNOWN, jcid.get(), 0);
    jab_send(jc, x);
    xmlnode_free(x);
}

void jabberhook::vcput(xmlnode x, const string &name, const string &val) {
    xmlnode_insert_cdata(xmlnode_insert_tag(x, name.c_str()),
	jhook.rusconv("ku", val).c_str(), (unsigned int) -1);
}

void jabberhook::vcputphone(xmlnode x, const string &type, const string &place, const string &number) {
    xmlnode z = xmlnode_insert_tag(x, "TEL");
    vcput(z, type, "");
    vcput(z, place, "");
    vcput(z, "NUMBER", number);
}

void jabberhook::vcputaddr(xmlnode x, const string &place, const string &street,
const string &locality, const string &region, const string &pcode,
unsigned short country) {
    xmlnode z = xmlnode_insert_tag(x, "ADR");
    vcput(z, place, "");
    vcput(z, "STREET", street);
    vcput(z, "LOCALITY", locality);
    vcput(z, "REGION", region);
    vcput(z, "PCODE", pcode);
    vcput(z, "CTRY", getCountryIDtoString(country));
}

void jabberhook::sendupdateuserinfo(const icqcontact &c) {
    xmlnode x, y, z;
    icqcontact::reginfo ri = c.getreginfo();
    string buf;
    char cbuf[64];

    vector<agent>::const_iterator ia = agents.begin();

    while(ia != agents.end()) {
	if(ia->name == ri.service) {
	    if(ia->type == agent::atStandard) {
		x = jutil_iqnew(JPACKET__SET, 0);
		y = xmlnode_insert_tag(x, "vCard");
		xmlnode_put_attrib(y, "xmlns", NS_VCARD);
		xmlnode_put_attrib(y, "version", "3.0");

		icqcontact::basicinfo bi = c.getbasicinfo();
		icqcontact::moreinfo mi = c.getmoreinfo();
		icqcontact::workinfo wi = c.getworkinfo();

		vcput(y, "DESC", c.getabout());
		vcput(y, "EMAIL", bi.email);
		vcput(y, "URL", mi.homepage);
		vcput(y, "TITLE", wi.position);
		vcput(y, "AGE", i2str(mi.age));
		vcput(y, "NICKNAME", c.getnick());

		vcput(y, "GENDER",
		    mi.gender == genderMale ? "Male" :
		    mi.gender == genderFemale ? "Female" : "");

		if(mi.birth_year && mi.birth_month && mi.birth_day) {
		    sprintf(cbuf, "%04d-%02d-%02d", mi.birth_year, mi.birth_month, mi.birth_day);
		    vcput(y, "BDAY", cbuf);
		}

		if(!(buf = bi.fname).empty()) buf += " " + bi.lname;
		vcput(y, "FN", buf);

		z = xmlnode_insert_tag(y, "N");
		vcput(z, "GIVEN", bi.fname);
		vcput(z, "FAMILY", bi.lname);

		z = xmlnode_insert_tag(y, "ORG");
		vcput(z, "ORGNAME", wi.company);
		vcput(z, "ORGUNIT", wi.dept);

		vcputphone(y, "VOICE", "HOME", bi.phone);
		vcputphone(y, "FAX", "HOME", bi.fax);
		vcputphone(y, "VOICE", "WORK", wi.phone);
		vcputphone(y, "FAX", "WORK", wi.fax);

		vcputaddr(y, "HOME", bi.street, bi.city, bi.state, bi.zip, bi.country);
		vcputaddr(y, "WORK", wi.street, wi.city, wi.state, wi.zip, wi.country);

		vcput(y, "HOMECELL", bi.cellular);
		vcput(y, "WORKURL", wi.homepage);

	    } else {
		x = jutil_iqnew(JPACKET__SET, NS_REGISTER);
		xmlnode_put_attrib(x, "id", "Register");
		y = xmlnode_get_tag(x, "query");

		xmlnode_put_attrib(x, "to", ia->jid.c_str());
		xmlnode_insert_cdata(xmlnode_insert_tag(y, "key"),
		    ia->params[agent::ptRegister].key.c_str(), (unsigned int) -1);

		vector<pair<string, string> >::const_iterator ip = ri.params.begin();
		while(ip != ri.params.end()) {
		    xmlnode_insert_cdata(xmlnode_insert_tag(y,
			ip->first.c_str()), ip->second.c_str(), (unsigned int) -1);
		    ++ip;
		}

	    }

	    jab_send(jc, x);
	    xmlnode_free(x);
	    break;
	}
	++ia;
    }
}

void jabberhook::gotmessage(const string &type, const string &from, const string &abody, const string &enc) {
    string u, h, r, body(abody);
    jidsplit(from, u, h, r);

    imcontact ic(jidtodisp(u + "@" + h), proto), chic;

    if(clist.get(chic = imcontact((string) "#" + ic.nickname, proto))) {
	ic = chic;
	if(!r.empty()) body.insert(0, r + ": ");
    }

#ifdef HAVE_GPGME
    icqcontact *c = clist.get(ic);

    if(c) {
	if(!enc.empty()) {
	    c->setusepgpkey(true);
	    if(pgp.enabled(proto)) body = pgp.decrypt(enc, proto);
		else c->setusepgpkey(false);
	} else {
	    c->setusepgpkey(false);
	}
    }
#endif

    em.store(immessage(ic, imevent::incoming, rusconv("uk", body)));
}

void jabberhook::updatecontact(icqcontact *c) {
    xmlnode x, y;

    if(logged()) {
	auto_ptr<char> cjid(strdup(jidnormalize(c->getdesc().nickname).c_str()));
	auto_ptr<char> cname(strdup(rusconv("ku", c->getdispnick()).c_str()));

	x = jutil_iqnew(JPACKET__SET, NS_ROSTER);
	y = xmlnode_insert_tag(xmlnode_get_tag(x, "query"), "item");
	xmlnode_put_attrib(y, "jid", cjid.get());
	xmlnode_put_attrib(y, "name", cname.get());

	vector<icqgroup>::const_iterator ig = find(groups.begin(), groups.end(), c->getgroupid());
	if(ig != groups.end()) {
	    y = xmlnode_insert_tag(y, "group");
	    xmlnode_insert_cdata(y, ig->getname().c_str(), (unsigned) -1);
	}

	jab_send(jc, x);
	xmlnode_free(x);
    }
}

void jabberhook::gotvcard(const imcontact &ic, xmlnode v) {
    xmlnode ad, n;
    char *p;
    string name, data;
    bool wasrole = false;

    icqcontact *c = clist.get(ic);
    if(!c || isourid(ic.nickname)) c = clist.get(contactroot);

    if(c) {
	icqcontact::basicinfo bi = c->getbasicinfo();
	icqcontact::moreinfo mi = c->getmoreinfo();
	icqcontact::workinfo wi = c->getworkinfo();

	for(ad = xmlnode_get_firstchild(v); ad; ad = xmlnode_get_nextsibling(ad)) {
	    p = xmlnode_get_name(ad); name = p ? up(p) : "";
	    p = xmlnode_get_data(ad); data = p ? rusconv("uk", p) : "";

	    if(name == "NICKNAME") c->setnick(data); else
	    if(name == "DESC") c->setabout(data); else
	    if(name == "EMAIL") bi.email = data; else
	    if(name == "URL") mi.homepage = data; else
	    if(name == "AGE") mi.age = atoi(data.c_str()); else
	    if(name == "HOMECELL") bi.cellular = data; else
	    if(name == "WORKURL") wi.homepage = data; else
	    if(name == "GENDER") {
		if(data == "Male") mi.gender = genderMale; else
		if(data == "Female") mi.gender = genderFemale; else
		    mi.gender = genderUnspec;
	    } else
	    if(name == "TITLE" || name == "ROLE") {
		if(!wasrole) {
		    wasrole = true;
		    wi.position = "";
		}

		if(!wi.position.empty()) wi.position += " / ";
		wi.position += data;
	    } else
	    if(name == "FN") {
		bi.fname = getword(data);
		bi.lname = data;
	    } else
	    if(name == "BDAY") {
		mi.birth_year = atoi(getword(data, "-").c_str());
		mi.birth_month = atoi(getword(data, "-").c_str());
		mi.birth_day = atoi(getword(data, "-").c_str());
	    } else
	    if(name == "ORG") {
		if(p = xmlnode_get_tag_data(ad, "ORGNAME")) wi.company = rusconv("uk", p);
		if(p = xmlnode_get_tag_data(ad, "ORGUNIT")) wi.dept = rusconv("uk", p);
	    } else
	    if(name == "N") {
		if(p = xmlnode_get_tag_data(ad, "GIVEN")) bi.fname = rusconv("uk", p);
		if(p = xmlnode_get_tag_data(ad, "FAMILY")) bi.lname = rusconv("uk", p);
	    } else
	    if(name == "ADR") {
		if(xmlnode_get_tag(ad, "HOME")) {
		    if(p = xmlnode_get_tag_data(ad, "STREET")) bi.street = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "LOCALITY")) bi.city = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "REGION")) bi.state = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "PCODE")) bi.zip = rusconv("uk", p);

		    if((p = xmlnode_get_tag_data(ad, "CTRY"))
		    || (p = xmlnode_get_tag_data(ad, "COUNTRY")))
			bi.country = getCountryByName(p);

		} else if(xmlnode_get_tag(ad, "WORK")) {
		    if(p = xmlnode_get_tag_data(ad, "STREET")) wi.street = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "LOCALITY")) wi.city = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "REGION")) wi.state = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "PCODE")) wi.zip = rusconv("uk", p);

		    if((p = xmlnode_get_tag_data(ad, "CTRY"))
		    || (p = xmlnode_get_tag_data(ad, "COUNTRY")))
			wi.country = getCountryByName(p);
		}
	    } else
	    if(name == "TEL") {
		if(p = xmlnode_get_tag_data(ad, "NUMBER")) {
		    if(xmlnode_get_tag(ad, "VOICE")) {
			if(xmlnode_get_tag(ad, "HOME")) bi.phone = rusconv("uk", p); else
			if(xmlnode_get_tag(ad, "WORK")) wi.phone = rusconv("uk", p);

		    } else if(xmlnode_get_tag(ad, "FAX")) {
			if(xmlnode_get_tag(ad, "HOME")) bi.fax = rusconv("uk", p); else
			if(xmlnode_get_tag(ad, "WORK")) wi.fax = rusconv("uk", p);
		    }
		}
	    }
	}

	c->setbasicinfo(bi);
	c->setmoreinfo(mi);
	c->setworkinfo(wi);

	if(isourid(ic.nickname)) {
	    icqcontact *cc = clist.get(ic);
	    if(cc) {
		cc->setnick(c->getnick());
		cc->setabout(c->getabout());
		cc->setbasicinfo(bi);
		cc->setmoreinfo(mi);
		cc->setworkinfo(wi);
	    }
	}
    }
}

void jabberhook::requestversion(const imcontact &ic) {
    auto_ptr<char> cjid(strdup(jidnormalize(ic.nickname).c_str()));
    xmlnode x = jutil_iqnew(JPACKET__GET, NS_VERSION);
    xmlnode_put_attrib(x, "to", cjid.get());
    xmlnode_put_attrib(x, "id", "versionreq");
    jab_send(jc, x);
    xmlnode_free(x);
}

void jabberhook::gotversion(const imcontact &ic, xmlnode x) {
    xmlnode y = xmlnode_get_tag(x, "query"), z;
    char *p;
    string vinfo;

    if(y) {
	if(z = xmlnode_get_tag(y, "name"))
	if(p = xmlnode_get_data(y))
	    if(p) vinfo = rusconv("uk", p);

	if(z = xmlnode_get_tag(y, "version"))
	if(p = xmlnode_get_data(y)) {
	    if(!vinfo.empty()) vinfo += ", ";
	    vinfo += rusconv("uk", p);
	}

	if(z = xmlnode_get_tag(y, "os"))
	if(p = xmlnode_get_data(y)) {
	    if(!vinfo.empty()) vinfo += " / ";
	    vinfo += rusconv("uk", p);
	}

	if(vinfo.size() > 128)
	    vinfo.erase(128);

	char buf[256];
	sprintf(buf, _("The remote is using %s"), vinfo.c_str());
	em.store(imnotification(ic, buf));
    }
}

bool jabberhook::isourid(const string &jid) {
    icqconf::imaccount acc = conf.getourid(jhook.proto);
    int pos;

    string ourjid = acc.nickname;
    if(ourjid.find("@") == -1) ourjid += (string) "@" + acc.server;
    if((pos = ourjid.find(":")) != -1) ourjid.erase(pos);

    return jidnormalize(jid) == ourjid;
}

string jabberhook::getourjid() {
    icqconf::imaccount acc = conf.getourid(jhook.proto);
    string jid = acc.nickname;
    int pos;

    if(jid.find("@") == -1)
	jid += (string) "@" + acc.server;

    if((pos = jid.find(":")) != -1)
	jid.erase(pos);

    if(jid.find("/") == -1)
	jid += "/centericq";

    return jid;
}

// ----------------------------------------------------------------------------

void jabberhook::statehandler(jconn conn, int state) {
    static int previous_state = -1;

    switch(state) {
	case JCONN_STATE_OFF:
	    jhook.flogged = jhook.fonline = false;

	    if(previous_state != JCONN_STATE_OFF) {
		logger.putourstatus(jhook.proto, jhook.getstatus(), jhook.ourstatus = offline);
		jhook.log(logDisconnected);
		jhook.roster.clear();
		jhook.agents.clear();
		clist.setoffline(jhook.proto);
		face.update();
	    }
	    break;

	case JCONN_STATE_CONNECTED:
	    break;

	case JCONN_STATE_AUTH:
	    break;

	case JCONN_STATE_ON:
	    if(jhook.regmode) jhook.fonline = true;
	    break;

	default:
	    break;
    }

    previous_state = state;
}

void jabberhook::packethandler(jconn conn, jpacket packet) {
    char *p;
    xmlnode x, y;
    string from, type, body, enc, ns, id, u, h, s;
    imstatus ust;
    int npos;
    bool isagent;

    jpacket_reset(packet);

    p = xmlnode_get_attrib(packet->x, "from"); if(p) from = p;
    p = xmlnode_get_attrib(packet->x, "type"); if(p) type = p;
    imcontact ic(jidtodisp(from), jhook.proto);

    switch(packet->type) {
	case JPACKET_MESSAGE:
	    x = xmlnode_get_tag(packet->x, "body");
	    p = xmlnode_get_data(x); if(p) body = p;

	    if(x = xmlnode_get_tag(packet->x, "subject"))
	    if(p = xmlnode_get_data(x))
		body = (string) p + ": " + body;

	    /* there can be multiple <x> tags. we're looking for one with
	       xmlns = jabber:x:encrypted */

	    for(x = xmlnode_get_firstchild(packet->x); x; x = xmlnode_get_nextsibling(x)) {
		if((p = xmlnode_get_name(x)) && !strcmp(p, "x"))
		if((p = xmlnode_get_attrib(x, "xmlns")) && !strcasecmp(p, "jabber:x:encrypted"))
		if(p = xmlnode_get_data(x)) {
		    enc = p;
		    break;
		}
	    }

	    if(!body.empty())
		jhook.gotmessage(type, from, body, enc);

	    break;

	case JPACKET_IQ:
	    if(type == "result") {
		if(p = xmlnode_get_attrib(packet->x, "id")) {
		    int iid = atoi(p);

		    if(iid == jhook.id) {
			if(!jhook.regmode) {
			    if(jhook.jstate == STATE_GETAUTH) {
				if(x = xmlnode_get_tag(packet->x, "query"))
				if(!xmlnode_get_tag(x, "digest")) {
				    jhook.jc->sid = 0;
				}

				jhook.id = atoi(jab_auth(jhook.jc));
				jhook.jstate = STATE_SENDAUTH;

			    } else {
				jhook.gotloggedin();
				jhook.jstate = STATE_LOGGED;
			    }

			} else {
			    jhook.regdone = true;

			}
			return;
		    }

		    if(!strcmp(p, "VCARDreq")) {
			x = xmlnode_get_firstchild(packet->x);
			if(!x) x = packet->x;

			jhook.gotvcard(ic, x);
			return;

		    } else if(!strcmp(p, "versionreq")) {
			jhook.gotversion(ic, packet->x);
			return;

		    }
		}

		if(x = xmlnode_get_tag(packet->x, "query")) {
		    p = xmlnode_get_attrib(x, "xmlns"); if(p) ns = p;

		    if(ns == NS_ROSTER) {
			jhook.gotroster(x);

		    } else if(ns == NS_AGENTS) {
			for(y = xmlnode_get_tag(x, "agent"); y; y = xmlnode_get_nextsibling(y)) {
			    const char *alias = xmlnode_get_attrib(y, "jid");

			    if(alias) {
				const char *name = xmlnode_get_tag_data(y, "name");
				const char *desc = xmlnode_get_tag_data(y, "description");
				const char *service = xmlnode_get_tag_data(y, "service");
				agent::agent_type atype = agent::atUnknown;

				if(xmlnode_get_tag(y, "groupchat")) atype = agent::atGroupchat; else
				if(xmlnode_get_tag(y, "transport")) atype = agent::atTransport; else
				if(xmlnode_get_tag(y, "search")) atype = agent::atSearch;

				if(alias && name && desc) {
				    jhook.agents.push_back(agent(alias, name, desc, atype));

				    if(atype == agent::atSearch) {
					x = jutil_iqnew (JPACKET__GET, NS_SEARCH);
					xmlnode_put_attrib(x, "to", alias);
					xmlnode_put_attrib(x, "id", "Agent info");
					jab_send(conn, x);
					xmlnode_free(x);
				    }

				    if(xmlnode_get_tag(y, "register")) {
					x = jutil_iqnew (JPACKET__GET, NS_REGISTER);
					xmlnode_put_attrib(x, "to", alias);
					xmlnode_put_attrib(x, "id", "Agent info");
					jab_send(conn, x);
					xmlnode_free(x);
				    }
				}
			    }
			}

			if(find(jhook.agents.begin(), jhook.agents.end(), DEFAULT_CONFSERV) == jhook.agents.end())
			    jhook.agents.insert(jhook.agents.begin(), agent(DEFAULT_CONFSERV, DEFAULT_CONFSERV,
				_("Default Jabber conference server"), agent::atGroupchat));

		    } else if(ns == NS_SEARCH || ns == NS_REGISTER) {
			p = xmlnode_get_attrib(packet->x, "id"); id = p ? p : "";

			if(id == "Agent info") {
			    jhook.gotagentinfo(packet->x);
			} else if(id == "Lookup") {
			    jhook.gotsearchresults(packet->x);
			} else if(id == "Register") {
			    x = jutil_iqnew(JPACKET__GET, NS_REGISTER);
			    xmlnode_put_attrib(x, "to", from.c_str());
			    xmlnode_put_attrib(x, "id", "Agent info");
			    jab_send(conn, x);
			    xmlnode_free(x);
			}

		    }
		}

	    } else if(type == "set") {
	    } else if(type == "error") {
		string name, desc;
		int code;

		x = xmlnode_get_tag(packet->x, "error");
		p = xmlnode_get_attrib(x, "code"); if(p) code = atoi(p);
		p = xmlnode_get_attrib(x, "id"); if(p) name = p;
		p = xmlnode_get_tag_data(packet->x, "error"); if(p) desc = p;

		switch(code) {
		    case 401: /* Unauthorized */
		    case 302: /* Redirect */
		    case 400: /* Bad request */
		    case 402: /* Payment Required */
		    case 403: /* Forbidden */
		    case 404: /* Not Found */
		    case 405: /* Not Allowed */
		    case 406: /* Not Acceptable */
		    case 407: /* Registration Required */
		    case 408: /* Request Timeout */
		    case 409: /* Conflict */
		    case 500: /* Internal Server Error */
		    case 501: /* Not Implemented */
		    case 502: /* Remote Server Error */
		    case 503: /* Service Unavailable */
		    case 504: /* Remote Server Timeout */
		    default:
			if(!jhook.regmode) {
			    face.log(desc.empty() ?
				_("+ [jab] error %d") :
				_("+ [jab] error %d: %s"),
				code, desc.c_str());

			    if(!jhook.flogged && code != 501) {
				close(jhook.jc->fd);
				jhook.jc->fd = -1;
			    }

			} else {
			    jhook.regerr = desc;

			}
		}

	    }
	    break;

	case JPACKET_PRESENCE:
	    x = xmlnode_get_tag(packet->x, "show");
	    ust = available;

	    if(x) {
		p = xmlnode_get_data(x); if(p) ns = p;

		if(!ns.empty()) {
		    if(ns == "away") ust = away; else
		    if(ns == "dnd") ust = dontdisturb; else
		    if(ns == "xa") ust = notavail; else
		    if(ns == "chat") ust = freeforchat;
		}
	    }

	    if(type == "unavailable")
		ust = offline;

	    jidsplit(from, u, h, s);
	    id = u + "@" + h;

	    if(clist.get(imcontact((string) "#" + id, jhook.proto))) {
		if(ust == offline) {
		    vector<string>::iterator im = find(jhook.chatmembers[id].begin(), jhook.chatmembers[id].end(), s);
		    if(im != jhook.chatmembers[id].end())
			jhook.chatmembers[id].erase(im);

		} else {
		    jhook.chatmembers[id].push_back(s);

		}

	    } else {
		icqcontact *c = clist.get(ic);

		if(c)
		if(c->getstatus() != ust) {
		    if(c->getstatus() == offline)
			jhook.awaymsgs[ic.nickname] = "";

		    logger.putonline(c, c->getstatus(), ust);
		    c->setstatus(ust);

		    if(x = xmlnode_get_tag(packet->x, "status"))
		    if(p = xmlnode_get_data(x))
			jhook.awaymsgs[ic.nickname] = p;

#ifdef HAVE_GPGME
		    if(x = xmlnode_get_tag(packet->x, "x"))
		    if(p = xmlnode_get_attrib(x, "xmlns"))
		    if((string) p == "jabber:x:signed")
		    if(p = xmlnode_get_data(x))
			c->setpgpkey(pgp.verify(p, jhook.awaymsgs[ic.nickname]));
#endif

		}
	    }
	    break;

	case JPACKET_S10N:
	    isagent = find(jhook.agents.begin(), jhook.agents.end(), from) != jhook.agents.end();

	    if(type == "subscribe") {
		if(!isagent) {
		    em.store(imauthorization(ic, imevent::incoming,
			imauthorization::Request, _("The user wants to subscribe to your network presence updates")));

		} else {
		    auto_ptr<char> cfrom(strdup(from.c_str()));
		    x = jutil_presnew(JPACKET__SUBSCRIBED, cfrom.get(), 0);
		    jab_send(jhook.jc, x);
		    xmlnode_free(x);
		}

	    } else if(type == "unsubscribe") {
		auto_ptr<char> cfrom(strdup(from.c_str()));
		x = jutil_presnew(JPACKET__UNSUBSCRIBED, cfrom.get(), 0);
		jab_send(jhook.jc, x);
		xmlnode_free(x);
		em.store(imnotification(ic, _("The user has removed you from his contact list (unsubscribed you, using the Jabber language)")));

	    }

	    break;

	default:
	    break;
    }
}

void jabberhook::jlogger(jconn conn, int inout, const char *p) {
    string tolog = (string) (inout ? "[IN]" : "[OUT]") + "\n";
    tolog += p;
    face.log(tolog);
}

#endif
