/*
*
* centericq rss handling class
* $Id: rsshook.cc,v 1.23 2004/12/24 18:33:07 konst Exp $
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

#ifdef BUILD_RSS

#define PERIOD_CHECK 30

#include "rsshook.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "imlogger.h"
#include "icqconf.h"
#include "eventmanager.h"

rsshook rhook;

rsshook::rsshook(): abstracthook(rss), timer_check(0) {
    fcapabs.insert(hookcapab::optionalpassword);
    fcapabs.insert(hookcapab::ping);
    fcapabs.insert(hookcapab::changeabout);
    fcapabs.insert(hookcapab::nochat);
}

rsshook::~rsshook() {
}

void rsshook::init() {
    httpcli.messageack.connect(this, &rsshook::messageack_cb);
    httpcli.socket.connect(this, &rsshook::socket_cb);

    if(conf.getdebug())
	httpcli.logger.connect(this, &rsshook::logger_cb);
}

void rsshook::exectimers() {
    int i, lastcheck, freq;
    icqcontact *c;
    time_t tcurrent = time(0);

    if(tcurrent-timer_check > PERIOD_CHECK) {
	for(i = 0; i < clist.count; i++) {
	    c = (icqcontact *) clist.at(i);

	    if(c->getdesc().pname == rss) {
		icqcontact::moreinfo mi = c->getmoreinfo();

		freq = mi.checkfreq;
		lastcheck = mi.checklast;

		if(freq <= 0) {
		    mi.checkfreq = 120;
		    c->setmoreinfo(mi);
		}

		if(tcurrent-lastcheck > freq*60) {
		    ping(c);
		}
	    }
	}

	timer_check = tcurrent;
    }

    httpcli.clearoutMessagesPoll();
}

bool rsshook::send(const imevent &ev) {
    if(ev.gettype() == imevent::xml && islivejournal(ev.getcontact()))
	return gethook(livejournal).send(ev);

    return false;
}

void rsshook::main() {
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

void rsshook::getsockets(fd_set &rs, fd_set &ws, fd_set &es, int &hsocket) const {
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

bool rsshook::isoursocket(fd_set &rs, fd_set &ws, fd_set &es) const {
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

bool rsshook::online() const {
    return true;
}

bool rsshook::logged() const {
    return false;
}

bool rsshook::isconnecting() const {
    return false;
}

bool rsshook::enabled() const {
    return true;
}

void rsshook::requestinfo(const imcontact &c) {
    ping(c);
}

void rsshook::requestversion(const imcontact &c) {
}

void rsshook::ping(const imcontact &ic) {
    string url;
    icqcontact *c = clist.get(ic);

    if(c) url = c->getworkinfo().homepage;
	else url = ic.nickname;

    if(!url.empty()) {
	httpcli.setProxyServerHost(conf.gethttpproxyhost());
	httpcli.setProxyServerPort(conf.gethttpproxyport());

	if(!conf.gethttpproxyuser().empty()) {
	    httpcli.setProxyServerUser(conf.gethttpproxyuser());
	    httpcli.setProxyServerPasswd(conf.gethttpproxypasswd());
	}

	HTTPRequestEvent *ev = new HTTPRequestEvent(url);
	if(islivejournal(ic)) {
	    icqconf::imaccount acc = conf.getourid(livejournal);
	    ev->setAuth(HTTPRequestEvent::Digest, acc.nickname, acc.password);
	}

	httpcli.SendEvent(ev);
    }
}

void rsshook::sendnewuser(const imcontact &ic) {
    gethook(livejournal).sendnewuser(ic);
}

void rsshook::removeuser(const imcontact &ic) {
    gethook(livejournal).removeuser(ic);
}

// ----------------------------------------------------------------------------

void rsshook::socket_cb(SocketEvent *ev) {
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

void rsshook::fetchRSSParam(string &base, XmlBranch *i, const string &enc,
const string &name, const string &title, const string &postfix) {

    XmlLeaf *d = i->getLeaf(name);
    string val;
    int pos, n;

    if(d)
    if(!d->getValue().empty()) {
	val = rushtmlconv("wk", cuthtml(d->getValue(), chCutBR | chLeaveLinks), false);
	if(!enc.empty()) val = siconv(val, enc, conf.getconvertto(rss));

	while((pos = val.find("<br")) != -1 || (pos = val.find("<BR")) != -1) {
	    if((n = val.substr(pos).find(">")) != -1) {
		val.replace(pos, n+1, "\n");
	    }
	}

	while((pos = val.find("<p>")) != -1 || (pos = val.find("<P>")) != -1) {
	    val.replace(pos, 3, "\n\n");
	}

	base += title + val + postfix;
    }
}

void rsshook::parsedocument(const HTTPRequestEvent *rev, icqcontact *c) {
    int k, n;
    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();
    icqcontact::workinfo wi = c->getworkinfo();

    string content = rev->getContent(), enc;

    bi.city = bi.state = bi.lname = "";

    if(!rev->isDelivered() || content.empty()) {
	bi.lname = rev->getHTTPResp();
	if(bi.lname.empty()) bi.lname = _("couldn't fetch");

	content = "";
    }

    if(bi.lname.empty())
    if((k = content.find("<?xml")) != -1) {
	string xmlspec = content.substr(k+6);

	if((k = xmlspec.find("?>")) != -1) {
	    xmlspec.erase(k);
	    xmlspec.insert(0, "<x ");
	    xmlspec += "/>";

	    string::iterator ix = xmlspec.begin();
	    auto_ptr<XmlNode> x(XmlNode::parse(ix, xmlspec.end()));
	    XmlLeaf *xx = dynamic_cast<XmlLeaf*>(x.get());

	    enc = xx->getAttrib("encoding");
	    bi.state = enc;
	}
    }

    XmlBranch *rss, *channel, *item;
    rss = channel = item = 0;

    string::iterator is = content.begin();
    auto_ptr<XmlNode> top(XmlNode::parse(is, content.end()));

    if(bi.lname.empty()) {
	if(!top.get()) {
	    if(!content.empty()) bi.lname = _("wrong XML");

	} else if(up(top->getTag().substr(0, 3)) != "RSS" && up(top->getTag().substr(0, 3)) != "RDF") {
	    if(!content.empty()) bi.lname = _("no <rss> tag found");

	}
    }

    if(bi.lname.empty())
    if(top.get()) {
	rss = dynamic_cast<XmlBranch*>(top.get());

	if(rss == NULL || !rss->exists("channel")) {
	    if(!content.empty()) bi.lname = _("no <channel> tag found");

	} else {
	    bi.city = top.get()->getAttrib("version");
	    channel = rss->getBranch("channel");
	    if(!channel) {
		if(!content.empty()) bi.lname = _("wrong <channel> tag");
	    }

	}
    }

    vector<string> items;
    vector<string>::iterator ii;
    string text;

    if(bi.lname.empty())
    if(channel) {
	bi.lname = _("success");

	text = ""; rhook.fetchRSSParam(text, channel, enc, "title", "");
	if(!text.empty()) bi.fname = text;

	text = ""; rhook.fetchRSSParam(text, channel, enc, "link", "");
	while((k = text.find_first_of(" \t\r\n")) != -1) text.erase(k, 1);
	if(!text.empty()) mi.homepage = text;

	text = ""; rhook.fetchRSSParam(text, channel, enc, "description", "");
	if(!text.empty()) c->setabout(text);

	if(!channel->getBranch("item")) {
	    channel = rss;
	    if(bi.city.empty()) bi.city = "rdf";
	}

	for(k = 0; item = channel->getBranch("item", k); k++) {
	    text = "";

	    rhook.fetchRSSParam(text, item, enc, "title", _("Title: "));
	    rhook.fetchRSSParam(text, item, enc, "pubDate", _("Published on: "));
	    rhook.fetchRSSParam(text, item, enc, "category", _("Category: "));
	    rhook.fetchRSSParam(text, item, enc, "author", _("Author: "));
	    text += "\n";
	    rhook.fetchRSSParam(text, item, enc, "description", _("Description: "), "\n\n");
	    rhook.fetchRSSParam(text, item, enc, "link", _("Link: "));
	    rhook.fetchRSSParam(text, item, enc, "comments", _("Comments: "));

	    items.push_back(text);
	}

	imevent *ev;
	vector<imevent *> events = em.getevents(c->getdesc(), 0);

	if(!events.empty())
	if(ev = dynamic_cast<immessage*>(events.back())) {
	    for(ii = items.begin(); ii != items.end(); ++ii) {
		string evtext = ev->gettext();

		while((k = evtext.find("\r")) != -1) evtext.erase(k, 1);
		while((k = ii->find("\r")) != -1) ii->erase(k, 1);

		if(*ii == evtext) {
		    items.erase(ii, items.end());
		    break;
		}
	    }
	}

	while(!events.empty()) {
	    delete events.back();
	    events.pop_back();
	}
    }

    if(c->inlist() && c->getdesc() != contactroot) {
	if(!items.empty()) {
	    vector<string>::reverse_iterator ir;

	    for(ir = items.rbegin(); ir != items.rend(); ++ir)
		em.store(immessage(c->getdesc(), imevent::incoming, *ir));
	}
    } else {
	string bfeed = rev->getURL();
	getrword(bfeed, "/");
	c->setnick(getrword(bfeed, "/"));
	wi.homepage = rev->getURL();
    }

    mi.checklast = timer_current;

    c->setbasicinfo(bi);
    c->setmoreinfo(mi);
    c->setworkinfo(wi);
}

void rsshook::messageack_cb(MessageEvent *ev) {
    HTTPRequestEvent *rev = dynamic_cast<HTTPRequestEvent*>(ev);

    if(!rev) return;

    int i;
    icqcontact *c;
    bool found;

    for(i = 0, found = false; i < clist.count && !found; i++) {
	c = (icqcontact *) clist.at(i);

	found = c->getdesc().pname == rss &&
	    c->getworkinfo().homepage == rev->getURL();
    }

    if(!found) c = clist.get(contactroot);

    parsedocument(rev, c);
}

void rsshook::logger_cb(LogEvent *ev) {
    switch(ev->getType()) {
	case LogEvent::PACKET:
	case LogEvent::DIRECTPACKET:
	    if(conf.getdebug())
		face.log(ev->getMessage());
	    break;

	default:
	    face.log(ev->getMessage());
	    break;
    }
}

#endif
