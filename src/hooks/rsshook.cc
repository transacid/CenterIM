/*
*
* centericq rss handling class
* $Id: rsshook.cc,v 1.1 2003/07/12 17:16:41 konst Exp $
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

#include <libicq2000/Xml.h>

rsshook rhook;

rsshook::rsshook(): abstracthook(rss), timer_check(time(0)-PERIOD_CHECK) {
    fcapabs.insert(hookcapab::optionalpassword);
    fcapabs.insert(hookcapab::ping);
    fcapabs.insert(hookcapab::changeabout);
}

rsshook::~rsshook() {
}

void rsshook::init() {
    httpcli.messageack.connect(this, &rsshook::messageack_cb);
    httpcli.socket.connect(this, &rsshook::socket_cb);
#ifdef DEBUG
    httpcli.logger.connect(this, &rsshook::logger_cb);
#endif
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
		freq = mi.birth_day;
		lastcheck = mi.birth_month;

		if(freq)
		if(tcurrent-lastcheck > freq*3600) {
		    ping(c);
		}
	    }
	}

	timer_check = tcurrent;
    }

    httpcli.clearoutMessagesPoll();
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
}

void rsshook::requestversion(const imcontact &c) {
}

void rsshook::ping(const imcontact &ic) {
    icqcontact *c = clist.get(ic);
    if(c) {
	httpcli.SendEvent(new HTTPRequestEvent(c->getworkinfo().homepage));
    }
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

static void fetchRSSParam(string &base, XmlBranch *i, const string &enc,
const string &name, const string &title, const string &postfix = "\n") {
    XmlLeaf *d = i->getLeaf(name);
    string val;

    if(d)
    if(!d->getValue().empty()) {
	val = d->getValue();
	if(!enc.empty()) val = siconv(val, enc, conf.getrussian(rss) ? "koi8-u" : DEFAULT_CHARSET);
	base += title + val + postfix;
    }
}

void rsshook::messageack_cb(MessageEvent *ev) {
    HTTPRequestEvent *rev = dynamic_cast<HTTPRequestEvent*>(ev);

    if(!rev) return;

    int i, k;
    icqcontact *c;
    time_t tcurrent = time(0);

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);
	if(c->getdesc().pname == rss) {
	    if(c->getworkinfo().homepage == rev->getURL()) {
		icqcontact::basicinfo bi = c->getbasicinfo();
		icqcontact::moreinfo mi = c->getmoreinfo();

		string content = rev->getContent(), enc;

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
		    }
		}

		string::iterator is = content.begin();

		auto_ptr<XmlNode> top(XmlNode::parse(is, content.end()));
		if(!top.get()) throw ParseException("Couldn't parse xml data in RSS");

		if(top->getTag() != "rss") throw ParseException("No <rss> tag found in RSS data");

		XmlBranch *rss = dynamic_cast<XmlBranch*>(top.get());
		if(rss == NULL || !rss->exists("channel")) throw ParseException("No <channel> tag found in RSS data");

		XmlBranch *channel = rss->getBranch("channel");
		if(!channel) throw ParseException("<channel> tag is not a leaf in RSS data");

		string text;

		text = ""; fetchRSSParam(text, channel, enc, "title", "");
		if(!text.empty()) bi.fname = text;

		text = ""; fetchRSSParam(text, channel, enc, "link", "");
		if(!text.empty()) mi.homepage = text;

		text = ""; fetchRSSParam(text, channel, enc, "description", "");
		if(!text.empty()) c->setabout(text);

		XmlBranch *item;

		vector<string> items;
		vector<string>::iterator ii;

		for(k = 0; item = channel->getBranch("item", k); k++) {
		    text = "";

		    fetchRSSParam(text, item, enc, "title", _("Title: "));
		    fetchRSSParam(text, item, enc, "pubDate", _("Published on: "));
		    fetchRSSParam(text, item, enc, "category", _("Category: "));
		    fetchRSSParam(text, item, enc, "author", _("Author: "));
		    text += "\n";
		    fetchRSSParam(text, item, enc, "description", _("Description: "), "\n\n");
		    fetchRSSParam(text, item, enc, "link", _("Link: "));
		    fetchRSSParam(text, item, enc, "comments", _("Comments: "));

		    items.push_back(text);
		}

		imevent *ev;
		vector<imevent *> events = em.getevents(c->getdesc(), mi.birth_month-1);

		if(!events.empty())
		if(ev = dynamic_cast<immessage*>(events.back())) {
		    for(ii = items.begin(); ii != items.end(); ++ii) {
			string evtext = ev->gettext();
			while((k = evtext.find("\r")) != -1)
			    evtext.erase(k, 1);

			if(*ii == evtext) {
			    items.erase(ii, items.end());
			    break;
			}
		    }
		}

		if(!items.empty()) {
		    vector<string>::reverse_iterator ir;

		    for(ir = items.rbegin(); ir != items.rend(); ++ir)
			em.store(immessage(c->getdesc(), imevent::incoming, *ir));
		}

		mi.birth_month = tcurrent;
		c->setbasicinfo(bi);
		c->setmoreinfo(mi);
	    }
	}
    }
}

void rsshook::logger_cb(LogEvent *ev) {
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
