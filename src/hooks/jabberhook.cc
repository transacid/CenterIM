/*
*
* centericq Jabber protocol handling class
* $Id: jabberhook.cc,v 1.18 2002/11/30 15:36:11 konst Exp $
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

jabberhook jhook;

jabberhook::jabberhook(): jc(0), flogged(false) {
    fcapabs.insert(hookcapab::setaway);
    fcapabs.insert(hookcapab::fetchaway);
    fcapabs.insert(hookcapab::authrequests);
    fcapabs.insert(hookcapab::directadd);
    fcapabs.insert(hookcapab::flexiblesearch);
    fcapabs.insert(hookcapab::visibility);
//    fcapabs.insert(hookcapab::conferencing);
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

    jc = jab_new(cjid.get(), cpass.get(), 0);

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

    string text;

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
	    xmlnode_put_attrib(x, "to", xmlnode_get_attrib(x, "to")+1);
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
    auto_ptr<char> cjid(strdup(jidnormalize(ic.nickname).c_str()));

    if(find(roster.begin(), roster.end(), cjid.get()) != roster.end())
	return;

    roster.push_back(cjid.get());

    if(report)
	face.log(_("+ [jab] adding %s to the contacts"), ic.nickname.c_str());

    x = jutil_presnew(JPACKET__SUBSCRIBE, cjid.get(), 0);
    jab_send(jc, x);
    xmlnode_free(x);

    x = jutil_iqnew(JPACKET__SET, NS_ROSTER);
    y = xmlnode_get_tag(x, "query");
    z = xmlnode_insert_tag(y, "item");
    xmlnode_put_attrib(z, "jid", cjid.get());
    jab_send(jc, x);
    xmlnode_free(x);
}

void jabberhook::removeuser(const imcontact &ic, bool report) {
    xmlnode x, y, z;
    auto_ptr<char> cjid(strdup(jidnormalize(ic.nickname).c_str()));

    vector<string>::iterator ir = find(roster.begin(), roster.end(), cjid.get());
    if(ir == roster.end()) return;
    roster.erase(ir);

    if(report)
	face.log(_("+ [jab] removing %s from the contacts"), ic.nickname.c_str());

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
    vector<icqcontact *>::const_iterator ifc = foundguys.begin();

    while(ifc != foundguys.end()) {
	if((*ifc)->getdesc() == ic) {
	    icqcontact *c = clist.get(contactroot);
	    c->clear();
	    c->setbasicinfo((*ifc)->getbasicinfo());
	    c->setdispnick((*ifc)->getdispnick());
	    c->setnick(ic.nickname);
	    break;
	}
	++ifc;
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

    jc = jab_new(cjid.get(), cpass.get(), 0);

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

string jabberhook::jidnormalize(const string &jid) {
    string user, host, rest;
    jidsplit(jid, user, host, rest);
    if(host.empty()) host = "jabber.com";
    user += (string) "@" + host;
    if(!rest.empty()) user += (string) "/" + rest;
    return user;
}

string jabberhook::jidtodisp(const string &jid) {
    string user, host, rest;
    jidsplit(jid, user, host, rest);

    if(host != "jabber.com" && !host.empty())
	user += (string) "@" + host;

    return user;
}

void jabberhook::jidsplit(const string &jid, string &user, string &host, string &rest) {
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

vector<string> jabberhook::getsearchservices() const {
    vector<agent>::const_iterator ia = agents.begin();
    vector<string> r;

    while(ia != agents.end()) {
	if(ia->type == agent::search)
	    r.push_back(ia->name);
	++ia;
    }

    return r;
}

vector<pair<string, string> > jabberhook::getsearchparameters(const string &agentname) const {
    vector<agent>::const_iterator ia = agents.begin();
    vector<pair<string, string> > r;

    while(ia != agents.end()) {
	if(ia->name == agentname)
	if(ia->type == agent::search) {
	    vector<string>::const_iterator isp = ia->searchparams.begin();
	    while(isp != ia->searchparams.end()) {
		r.push_back(make_pair(*isp, string()));
		++isp;
	    }
	}

	++ia;
    }

    return r;
}

void jabberhook::gotagentinfo(xmlnode x) {
    xmlnode y;
    string name, data;
    const char *from = xmlnode_get_attrib(x, "from"), *p;
    vector<agent>::iterator ia = jhook.agents.begin();

    if(!from) return;

    while(ia != jhook.agents.end()) {
	if(ia->jid == from)
	if(y = xmlnode_get_tag(x, "query")) {
	    ia->searchparams.clear();

	    for(y = xmlnode_get_firstchild(y); y; y = xmlnode_get_nextsibling(y)) {
		p = xmlnode_get_name(y); name = p ? p : "";
		p = xmlnode_get_data(y); data = p ? p : "";

		if(name == "instructions") ia->instruction = data; else
		if(name == "key") ia->key = data; else
		if(!name.empty()) {
		    ia->searchparams.push_back(name);
		}
	    }
	    break;
	}
	++ia;
    }

}

void jabberhook::lookup(const imsearchparams &params, verticalmenu &dest) {
    xmlnode x, y;

    while(!foundguys.empty()) {
	delete foundguys.back();
	foundguys.pop_back();
    }

    if(!params.service.empty()) {
	searchdest = &dest;

	x = jutil_iqnew(JPACKET__SET, NS_SEARCH);
	xmlnode_put_attrib(x, "id", "Lookup");

	y = xmlnode_get_tag(x, "query");

	vector<agent>::const_iterator ia = agents.begin();
	while(ia != agents.end()) {
	    if(ia->name == params.service) {
		xmlnode_put_attrib(x, "to", ia->jid.c_str());
		xmlnode_insert_cdata(xmlnode_insert_tag(y, "key"),
		    ia->key.c_str(), (unsigned int) -1);
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
	    if(nick) c->setdispnick(nick);
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

    jhook.flogged = true;
    jhook.ourstatus = available;
    jhook.setautostatus(jhook.manualstatus);

    x = jutil_iqnew(JPACKET__GET, NS_ROSTER);
    xmlnode_put_attrib(x, "id", "Roster");
    jab_send(jc, x);
    xmlnode_free(x);

    x = jutil_iqnew(JPACKET__GET, NS_AGENTS);
    xmlnode_put_attrib(x, "id", "Agent List");
    jab_send(jc, x);
    xmlnode_free(x);

    face.log(_("+ [jab] logged in"));
    face.update();
}

void jabberhook::conferencecreate(const imcontact &confid, const vector<imcontact> &lst) {
    auto_ptr<char> jcid(strdup(confid.nickname.substr(1).c_str()));
    xmlnode x = jutil_presnew(JPACKET__UNKNOWN, jcid.get(), 0);
    jab_send(jc, x);
    xmlnode_free(x);
}

// ----------------------------------------------------------------------------

void jabberhook::statehandler(jconn conn, int state) {
    static int previous_state = JCONN_STATE_OFF;

    switch(state) {
	case JCONN_STATE_OFF:
	    if(previous_state != JCONN_STATE_OFF) {
		logger.putourstatus(jabber, jhook.getstatus(), jhook.ourstatus = offline);
		face.log(_("+ [jab] disconnected"));
		clist.setoffline(jabber);
		face.update();

		jhook.jc = 0;
		jhook.flogged = false;
		jhook.roster.clear();
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

	    if(!body.empty()) {
		body = siconv(body, "utf8", conf.getrussian() ? "koi8-u" : DEFAULT_CHARSET);

		if(type == "groupchat")
		    ic = imcontact((string) "#" + ic.nickname, jabber);

		jhook.checkinlist(ic);
		em.store(immessage(ic, imevent::incoming, body));
	    }
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

		x = xmlnode_get_tag(packet->x, "query");

		if(x) {
		    p = xmlnode_get_attrib(x, "xmlns"); if(p) ns = p;

		    if(ns == NS_ROSTER) {
			for(y = xmlnode_get_tag(x, "item"); y; y = xmlnode_get_nextsibling(y)) {
			    const char *alias = xmlnode_get_attrib(y, "jid");
			    const char *sub = xmlnode_get_attrib(y, "subscription");
			    const char *name = xmlnode_get_attrib(y, "name");
			    const char *group = xmlnode_get_attrib(y, "group");

			    if(alias) {
				ic = imcontact(jidtodisp(alias), jabber);
				jhook.roster.push_back(jidnormalize(alias));

				if(!clist.get(ic)) {
				    icqcontact *c = clist.addnew(ic, false);
				    if(name) {
					icqcontact::basicinfo cb = c->getbasicinfo();
					cb.fname = name;
					c->setbasicinfo(cb);
				    }
				}
			    }
			}

		    } else if(ns == NS_AGENTS) {
			for(y = xmlnode_get_tag(x, "agent"); y; y = xmlnode_get_nextsibling(y)) {
			    const char *alias = xmlnode_get_attrib(y, "jid");

			    if(alias) {
				const char *name = xmlnode_get_tag_data(y, "name");
				const char *desc = xmlnode_get_tag_data(y, "description");
				const char *service = xmlnode_get_tag_data(y, "service");
				agent::agent_type atype = agent::unknown;

				if(xmlnode_get_tag(y, "groupchat")) atype = agent::groupchat; else
				if(xmlnode_get_tag(y, "transport")) atype = agent::transport; else
				if(xmlnode_get_tag(y, "search")) atype = agent::search;

				if(alias && name && desc) {
				    jhook.agents.push_back(agent(alias, name, desc, atype));
				    if(atype == agent::search) {
					x = jutil_iqnew (JPACKET__GET, NS_SEARCH);
					xmlnode_put_attrib(x, "to", alias);
					xmlnode_put_attrib(x, "id", "Agent info");
					jab_send(conn, x);
					xmlnode_free(x);
				    }
				}
			    }
			}

		    } else if(ns == NS_SEARCH) {
			p = xmlnode_get_attrib(packet->x, "id"); id = p ? p : "";
			if(id == "Agent info") {
			    jhook.gotagentinfo(packet->x);
			} else if(id == "Lookup") {
			    jhook.gotsearchresults(packet->x);
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
