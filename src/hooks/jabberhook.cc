/*
*
* centericq Jabber protocol handling class
* $Id: jabberhook.cc,v 1.31 2002/12/11 19:10:03 konst Exp $
*
* Copyright (C) 2002 by Konstantin Klyagin <konst@konst.org.ua>
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

#include "jabberhook.h"
#include "icqface.h"
#include "imlogger.h"
#include "eventmanager.h"

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

static string jidnormalize(const string &jid) {
    string user, host, rest;
    jidsplit(jid, user, host, rest);
    if(host.empty()) host = "jabber.com";
    user += (string) "@" + host;
    if(!rest.empty()) user += (string) "/" + rest;
    return user;
}

static string jidtodisp(const string &jid) {
    string user, host, rest;
    jidsplit(jid, user, host, rest);

    if(host != "jabber.com" && !host.empty())
	user += (string) "@" + host;

    return user;
}

// ----------------------------------------------------------------------------

jabberhook jhook;

jabberhook::jabberhook(): jc(0), flogged(false) {
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
}

jabberhook::~jabberhook() {
}

void jabberhook::init() {
    manualstatus = conf.getstatus(jabber);
}

void jabberhook::connect() {
    icqconf::imaccount acc = conf.getourid(jabber);
    string jid;
    int pos;

    face.log(_("+ [jab] connecting to the server"));

    jid = acc.nickname;

    if(jid.find("@") == -1)
	jid += (string) "@" + acc.server;

    if((pos = jid.find(":")) != -1)
	jid.erase(pos);

    if(jid.find("/") == -1)
	jid += "/centericq";

    auto_ptr<char> cjid(strdup(jid.c_str()));
    auto_ptr<char> cpass(strdup(acc.password.c_str()));

    regmode = false;

    jc = jab_new(cjid.get(), cpass.get(), acc.port,
	acc.additional["ssl"] == "1" ? 1 : 0);

    jab_packet_handler(jc, &packethandler);
    jab_state_handler(jc, &statehandler);

#if PACKETDEBUG
    jab_logger(jc, &jlogger);
#endif

    if(jc->user) {
	jab_start(jc);
	id = atoi(jab_auth(jc));
    } else {
	face.log(_("+ [jab] incorrect jabber id"));
    }

    if(!jc || jc->state == JCONN_STATE_OFF) {
	face.log(_("+ [jab] unable to connect to the server"));
	jab_delete(jc);
	jc = 0;
    }
}

void jabberhook::disconnect() {
    jab_stop(jc);
    jab_delete(jc);
    statehandler(jc, JCONN_STATE_OFF);
}

void jabberhook::main() {
    jab_poll(jc, 0);

    if(!jc) {
	statehandler(jc, JCONN_STATE_OFF);

    } else if(jc->state == JCONN_STATE_OFF || jc->fd == -1) {
	statehandler(jc, JCONN_STATE_OFF);
	jab_delete(jc);
	jc = 0;
    }
}

void jabberhook::getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const {
    if(jc) {
	FD_SET(jc->fd, &rfds);
	hsocket = max(jc->fd, hsocket);
    }
}

bool jabberhook::isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const {
    if(jc) {
	return FD_ISSET(jc->fd, &rfds);
    } else {
	return false;
    }
}

bool jabberhook::online() const {
    return (bool) jc;
}

bool jabberhook::logged() const {
    return jc && flogged;
}

bool jabberhook::isconnecting() const {
    return jc && !flogged;
}

bool jabberhook::enabled() const {
    return true;
}

bool jabberhook::send(const imevent &ev) {
    icqcontact *c = clist.get(ev.getcontact());

    string text, cname;

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

	    if(m->getauthtype() == imauthorization::Granted) {
		x = jutil_presnew(JPACKET__SUBSCRIBED, cjid.get(), 0);

	    } else if(m->getauthtype() == imauthorization::Request) {
		x = jutil_presnew(JPACKET__SUBSCRIBE, cjid.get(), 0);

	    }

	    if(x) {
		jab_send(jc, x);
		xmlnode_free(x);
	    }

	    return true;
	}

	text = siconv(text, conf.getrussian() ? "koi8-u" : DEFAULT_CHARSET, "utf8");

	auto_ptr<char> cjid(strdup(jidnormalize(c->getdesc().nickname).c_str()));
	auto_ptr<char> ctext(strdup(text.c_str()));

	xmlnode x;
	x = jutil_msgnew(TMSG_CHAT, cjid.get(), 0, ctext.get());

	if(ischannel(c)) {
	    xmlnode_put_attrib(x, "type", "groupchat");
	    if(!(cname = getchanneljid(c)).empty())
		xmlnode_put_attrib(x, "to", cname.c_str());
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
	if(find(roster.begin(), roster.end(), cjid.get()) != roster.end())
	    return;

	if(report)
	    face.log(_("+ [jab] adding %s to the contacts"), ic.nickname.c_str());

	roster.push_back(cjid.get());
	x = jutil_presnew(JPACKET__SUBSCRIBE, cjid.get(), 0);
	jab_send(jc, x);
	xmlnode_free(x);

	x = jutil_iqnew(JPACKET__SET, NS_ROSTER);
	y = xmlnode_get_tag(x, "query");
	z = xmlnode_insert_tag(y, "item");
	xmlnode_put_attrib(z, "jid", cjid.get());
	jab_send(jc, x);
	xmlnode_free(x);

	if(c = clist.get(ic)) {
	    imcontact icc(jidtodisp(ic.nickname), jabber);
	    if(ic.nickname != icc.nickname) {
		c->setdesc(icc);
		c->setdispnick(icc.nickname);
	    }
	    c->setnick("");
	}

	requestinfo(c);

    } else {
	if(c = clist.get(ic)) {
	    cname = getchanneljid(c);
	    if(!cname.empty()) {
		cname += "/" + conf.getourid(jabber).nickname;
		auto_ptr<char> ccname(strdup(cname.c_str()));
		x = jutil_presnew(JPACKET__UNKNOWN, ccname.get(), 0);
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
	vector<string>::iterator ir = find(roster.begin(), roster.end(), cjid.get());

	if(ir == roster.end())
	    return;

	if(report)
	    face.log(_("+ [jab] removing %s from the contacts"), ic.nickname.c_str());

	roster.erase(ir);
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
	    cname = getchanneljid(c);
	    if(!cname.empty()) {
		cname += "/" + conf.getourid(jabber).nickname;
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
		    msg = conf.getawaymsg(jabber);
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
    if(logged()) {
	auto_ptr<char> cjid(strdup(jidnormalize(ic.nickname).c_str()));
	xmlnode x = jutil_iqnew(JPACKET__GET, NS_VCARD);
	xmlnode_put_attrib(x, "to", cjid.get());
	jab_send(jc, x);
	xmlnode_free(x);
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
    int pos;
    string jid = nick + "@" + serv;
    if((pos = jid.find(":")) != -1) jid.erase(pos);

    auto_ptr<char> cjid(strdup(jid.c_str()));
    auto_ptr<char> cpass(strdup(pass.c_str()));

    jc = jab_new(cjid.get(), cpass.get(), icqconf::defservers[jabber].port, 0);

    jab_packet_handler(jc, &packethandler);
    jab_state_handler(jc, &statehandler);

#if PACKETDEBUG
    jab_logger(jc, &jlogger);
#endif

    jab_start(jc);
    id = atoi(jab_reg(jc));

    if(!online()) {
	err = _("Unable to connect");

    } else {
	regmode = true;
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

void jabberhook::setjabberstatus(imstatus st, const string &msg) {
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
    }

    if(!msg.empty()) {
	xmlnode_insert_cdata(xmlnode_insert_tag(x, "status"),
	    msg.c_str(), (unsigned) -1);
    }

    jab_send(jc, x);
    xmlnode_free(x);

    sendvisibility();

    logger.putourstatus(jabber, getstatus(), ourstatus = st);
}

void jabberhook::sendvisibility() {
    xmlnode x;
    icqlist::iterator i;

    for(i = lst.begin(); i != lst.end(); ++i)
    if(i->getdesc().pname == jabber) {
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

string jabberhook::getchanneljid(icqcontact *c) {
    string r;
    vector<agent>::const_iterator ia = agents.begin();

    while(ia != agents.end() && r.empty()) {
	if(ia->name == c->getbasicinfo().street)
	    r = (string) c->getdesc().nickname.substr(1) + "@" +
		ia->jid;
	++ia;
    }

    return r;
}

void jabberhook::checkinlist(imcontact ic) {
    icqcontact *c = clist.get(ic);

    if(c) {
	if(c->inlist()) {
	    if(find(roster.begin(), roster.end(), jidnormalize(ic.nickname)) != roster.end())
		sendnewuser(ic, false);
	}
    } else {
	clist.addnew(ic);
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
		if(ia->type == agent::atGroupchat) r.push_back(ia->name);
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
		if(!name.empty()) {
		    ia->params[pt].paramnames.push_back(make_pair(name, data));
		}
	    }
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

	if(c = clist.get(imcontact(params.room, jabber)))
	if(!(room = getchanneljid(c)).empty()) {
	    vector<string>::const_iterator im = chatmembers[room].begin();
	    while(im != chatmembers[room].end()) {
		foundguys.push_back(c = new icqcontact(imcontact(*im, jabber)));
		searchdest->additem(conf.getcolor(cp_clist_jabber), c, (string) " " + *im);
		++im;
	    }
	}

	face.findready();
	face.log(_("+ [jab] conference list fetching finished, %d found"),
	    foundguys.size());

	searchdest->redraw();
	searchdest = 0;
    }
}

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
	    c = new icqcontact(imcontact(jidnormalize(jid), jabber));
	    icqcontact::basicinfo cb = c->getbasicinfo();

	    if(nick) {
		c->setnick(nick);
		c->setdispnick(nick);
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

    face.log(_("+ [jab] search finished, %d found"),
	foundguys.size());

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
    xmlnode y;
    imcontact ic;
    icqcontact *c;

    for(y = xmlnode_get_tag(x, "item"); y; y = xmlnode_get_nextsibling(y)) {
	const char *alias = xmlnode_get_attrib(y, "jid");
	const char *sub = xmlnode_get_attrib(y, "subscription");
	const char *name = xmlnode_get_attrib(y, "name");
	const char *group = xmlnode_get_attrib(y, "group");

	if(alias) {
	    ic = imcontact(jidtodisp(alias), jabber);
	    roster.push_back(jidnormalize(alias));

	    if(!(c = clist.get(ic)))
		c = clist.addnew(ic, false);

	    if(name) {
		c->setnick(name);
		c->setdispnick(name);
	    }
	}
    }

    postlogin();
}

void jabberhook::postlogin() {
    int i;
    icqcontact *c;

    flogged = true;
    ourstatus = available;
    setautostatus(jhook.manualstatus);
    face.log(_("+ [jab] logged in"));
    face.update();

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == jabber)
	if(ischannel(c))
	if(c->getbasicinfo().requiresauth)
	    c->setstatus(available);
    }
}

void jabberhook::conferencecreate(const imcontact &confid, const vector<imcontact> &lst) {
    auto_ptr<char> jcid(strdup(confid.nickname.substr(1).c_str()));
    xmlnode x = jutil_presnew(JPACKET__UNKNOWN, jcid.get(), 0);
    jab_send(jc, x);
    xmlnode_free(x);
}

void jabberhook::sendupdateuserinfo(const icqcontact &c) {
    xmlnode x, y;
    icqcontact::reginfo ri = c.getreginfo();

    x = jutil_iqnew(JPACKET__SET, NS_REGISTER);
    xmlnode_put_attrib(x, "id", "Register");

    y = xmlnode_get_tag(x, "query");

    vector<agent>::const_iterator ia = agents.begin();
    while(ia != agents.end()) {
	if(ia->name == ri.service) {
	    xmlnode_put_attrib(x, "to", ia->jid.c_str());
	    xmlnode_insert_cdata(xmlnode_insert_tag(y, "key"),
		ia->params[agent::ptRegister].key.c_str(), (unsigned int) -1);
	    break;
	}
	++ia;
    }

    vector<pair<string, string> >::const_iterator ip = ri.params.begin();
    while(ip != ri.params.end()) {
	xmlnode_insert_cdata(xmlnode_insert_tag(y,
	    ip->first.c_str()), ip->second.c_str(), (unsigned int) -1);
	++ip;
    }

    jab_send(jc, x);
    xmlnode_free(x);
}

void jabberhook::gotmessage(const string &type, const string &from, const string &abody) {
    int pos;
    string u, h, r, body(abody), ournick;
    imcontact ic(jidtodisp(from), jabber);
    icqcontact *c;

    if(type == "groupchat") {
	ournick = conf.getourid(jabber).nickname;
	jidsplit(from, u, h, r);

	if(r == ournick) return; else {
	    ic = imcontact((string) "#" + u, jabber);

	    if(!r.empty()) body.insert(0, r + ": "); else {
		if((pos = body.find(" ")) != -1) {
		    u = body.substr(0, pos);
		    if(c = clist.get(ic)) h = getchanneljid(c);

		    if(body.substr(pos+1) == "has arrived.") {
			if(u == ournick) return; else
			    chatmembers[h].push_back(u);

		    } else if(body.substr(pos+1) == "has left.") {
			vector<string>::iterator im = find(chatmembers[h].begin(), chatmembers[h].end(), u);
			if(im != chatmembers[h].end())
			    chatmembers[h].erase(im);

		    }
		}
	    }
	}
    }

    checkinlist(ic);
    body = siconv(body, "utf8", conf.getrussian() ? "koi8-u" : DEFAULT_CHARSET);
    em.store(immessage(ic, imevent::incoming, body));
}

void jabberhook::updatecontact(icqcontact *c) {
    xmlnode x, y;

    if(logged()) {
	auto_ptr<char> cjid(strdup(jidnormalize(c->getdesc().nickname).c_str()));
	auto_ptr<char> cname(strdup(c->getdispnick().c_str()));

	x = jutil_iqnew(JPACKET__SET, NS_ROSTER);
	y = xmlnode_insert_tag(xmlnode_get_tag(x, "query"), "item");
	xmlnode_put_attrib(y, "jid", cjid.get());
	xmlnode_put_attrib(y, "name", cname.get());
	jab_send(jc, x);
	xmlnode_free(x);

	c->setnick(c->getdispnick());
    }
}

void jabberhook::gotvcard(const imcontact &ic, xmlnode v) {
    xmlnode ad, n;
    icqcontact *c;
    char *p;

    if(c = clist.get(ic)) {
	icqcontact::basicinfo bi = c->getbasicinfo();
	icqcontact::moreinfo mi = c->getmoreinfo();
	icqcontact::workinfo wi = c->getworkinfo();

	if(p = xmlnode_get_tag_data(v, "NICKNAME")) c->setnick(p);
	if(p = xmlnode_get_tag_data(v, "DESC")) c->setabout(p);
	if(p = xmlnode_get_tag_data(v, "EMAIL")) bi.email = p;
	if(p = xmlnode_get_tag_data(v, "URL")) mi.homepage = p;
	if(p = xmlnode_get_tag_data(v, "TITLE")) wi.position = p;
	if(p = xmlnode_get_tag_data(v, "AGE")) mi.age = atoi(p);
	if(p = xmlnode_get_tag_data(v, "HOMECELL")) bi.cellular = p;

	if(p = xmlnode_get_tag_data(v, "ROLE"))
	if(!wi.position.empty()) wi.position += (string) " / " + p;

	if(p = xmlnode_get_tag_data(v, "FN")) {
	    string buf = p;
	    bi.fname = getword(buf);
	    bi.lname = buf;
	}

	if(ad = xmlnode_get_tag(v, "ORG")) {
	    if(p = xmlnode_get_tag_data(ad, "ORGNAME")) wi.company = p;
	    if(p = xmlnode_get_tag_data(ad, "ORGUNIT")) wi.dept = p;
	}

	if(ad = xmlnode_get_tag(v, "TEL")) {
	    if(p = xmlnode_get_tag_data(ad, "VOICE")) bi.phone = p;
	    if(p = xmlnode_get_tag_data(ad, "HOME")) bi.fax = p;
	}

	if(ad = xmlnode_get_tag(v, "ADR")) {
	    if(p = xmlnode_get_tag_data(ad, "STREET")) bi.street = p;
	    if(p = xmlnode_get_tag_data(ad, "LOCALITY")) bi.state = p;
	}

	c->setbasicinfo(bi);
	c->setmoreinfo(mi);
	c->setworkinfo(wi);
    }
}

// ----------------------------------------------------------------------------

void jabberhook::statehandler(jconn conn, int state) {
    static int previous_state = JCONN_STATE_OFF;

    switch(state) {
	case JCONN_STATE_OFF:
	    if(previous_state != JCONN_STATE_OFF) {
		logger.putourstatus(jabber, jhook.getstatus(), jhook.ourstatus = offline);
		face.log(_("+ [jab] disconnected"));
		jhook.jc = 0;
		jhook.flogged = false;
		jhook.roster.clear();
		clist.setoffline(jabber);
		face.update();
	    }
	    break;

	case JCONN_STATE_CONNECTED:
	    break;

	case JCONN_STATE_AUTH:
	    break;

	case JCONN_STATE_ON:
	    break;

	default:
	    break;
    }

    previous_state = state;
}

void jabberhook::packethandler(jconn conn, jpacket packet) {
    char *p;
    xmlnode x, y;
    string from, type, body, ns, id;
    imstatus ust;

    jpacket_reset(packet);

    p = xmlnode_get_attrib(packet->x, "from"); if(p) from = p;
    p = xmlnode_get_attrib(packet->x, "type"); if(p) type = p;
    imcontact ic(jidtodisp(from), jabber);

    switch(packet->type) {
	case JPACKET_MESSAGE:
	    x = xmlnode_get_tag(packet->x, "body");
	    p = xmlnode_get_data(x); if(p) body = p;

	    if(x = xmlnode_get_tag(packet->x, "subject")) {
		body = (string) xmlnode_get_data (x) + ": " + body;
	    }

	    if(!body.empty())
		jhook.gotmessage(type, from, body);
	    break;

	case JPACKET_IQ:
	    if(type == "result") {
		if(p = xmlnode_get_attrib(packet->x, "id")) {
		    int iid = atoi(p);

		    if(iid == jhook.id) {
			if(!jhook.regmode) {
			    jhook.gotloggedin();

			} else {
			    jhook.regdone = true;

			}
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

		if(x = xmlnode_get_firstchild(packet->x))
		if(p = xmlnode_get_name(x))
		if(!strcasecmp(p, "vcard"))
		    jhook.gotvcard(ic, x);

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
			    face.log(_("[jab] error %d: %s"), code, desc.c_str());
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

	    if(0/*ischatroom(from)*/) {
		/*JABBERChatRoomBuddyStatus(room, user, status);*/
	    } else {
		icqcontact *c = clist.get(ic);

		if(c) {
		    if(c->getstatus() != ust) {
			if(c->getstatus() == offline)
			    jhook.awaymsgs[ic.nickname] = "";

			c->setstatus(ust);

			if(x = xmlnode_get_tag(packet->x, "status"))
			if(p = xmlnode_get_data(x))
			    jhook.awaymsgs[ic.nickname] = p;
		    }
		}
	    }
	    break;

	case JPACKET_S10N:
	    if(type == "subscribe") {
		em.store(imauthorization(ic, imevent::incoming,
		    imauthorization::Request, _("The user wants to subscribe to your network presence updates")));

	    } else if(type == "unsubscribe") {
		auto_ptr<char> cfrom(strdup(from.c_str()));

		x = jutil_presnew(JPACKET__UNSUBSCRIBED, cfrom.get(), 0);
		jab_send(jhook.jc, x);
		xmlnode_free(x);

		em.store(imnotification(ic, (string)
		    _("The user has removed you from his contact list (unsubscribed you, using the Jabber language)")));
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
