/*
*
* centericq IRC protocol handling class
* $Id: irchook.cc,v 1.11 2002/04/08 13:45:45 konst Exp $
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

#include "irchook.h"
#include "icqface.h"
#include "accountmanager.h"
#include "icqcontacts.h"
#include "centericq.h"
#include "imlogger.h"

#ifdef DEBUG
#define DLOG(s) face.log("irc %s", s)
#else
#define DLOG(s)
#endif

irchook irhook;

irchook::irchook()
    : handle(firetalk_create_handle(FP_IRC, 0)),
      fonline(false), flogged(false), ourstatus(offline)
{
    fcapabilities =
	hoptCanSetAwayMsg |
	hoptCanChangeNick |
	hoptNoPasswords |
	hoptChangableServer;
}

irchook::~irchook() {
}

void irchook::init() {
    manualstatus = conf.getstatus(irc);

    loadconfig();

    firetalk_register_callback(handle, FC_CONNECTED, &connected);
    firetalk_register_callback(handle, FC_CONNECTFAILED, &connectfailed);
    firetalk_register_callback(handle, FC_DISCONNECT, &disconnected);
    firetalk_register_callback(handle, FC_NEWNICK, &newnick);
    firetalk_register_callback(handle, FC_IM_GOTINFO, &gotinfo);
    firetalk_register_callback(handle, FC_IM_GOTCHANNELS, &gotchannels);
    firetalk_register_callback(handle, FC_IM_GETMESSAGE, &getmessage);
    firetalk_register_callback(handle, FC_IM_GETACTION, &getmessage);
    firetalk_register_callback(handle, FC_IM_BUDDYONLINE, &buddyonline);
    firetalk_register_callback(handle, FC_IM_BUDDYOFFLINE, &buddyoffline);
    firetalk_register_callback(handle, FC_IM_BUDDYAWAY, &buddyaway);
    firetalk_register_callback(handle, FC_IM_BUDDYUNAWAY, &buddyonline);
    firetalk_register_callback(handle, FC_CHAT_LISTMEMBER, &listmember);
    firetalk_register_callback(handle, FC_CHAT_NAMES, &chatnames);

#ifdef DEBUG
    firetalk_register_callback(handle, FC_LOG, &log);
#endif
}

void irchook::connect() {
    icqconf::imaccount acc = conf.getourid(irc);
    face.log(_("+ [irc] connecting to the server"));

    firetalk_disconnect(handle);

    fonline = firetalk_signon(handle, acc.server.c_str(),
	acc.port, acc.nickname.c_str()) == FE_SUCCESS;

    if(fonline) {
	flogged = false;
    } else {
	face.log(_("+ [irc] unable to connect to the server"));
    }
}

void irchook::disconnect() {
    if(fonline) {
	firetalk_disconnect(handle);
    }
}

void irchook::exectimers() {
}

void irchook::main() {
    firetalk_select();
}

void irchook::getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const {
    int *r, *w, *e, *sr, *sw, *se;

    firetalk_getsockets(FP_IRC, &sr, &sw, &se);

    for(r = sr; *r; r++) {
	if(*r > hsocket) hsocket = *r;
	FD_SET(*r, &rfds);
    }

    for(w = sw; *w; w++) {
	if(*w > hsocket) hsocket = *w;
	FD_SET(*w, &wfds);
    }

    for(e = se; *e; e++) {
	if(*e > hsocket) hsocket = *e;
	FD_SET(*e, &efds);
    }

    delete sr;
    delete sw;
    delete se;
}

bool irchook::isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const {
    bool res = false;
    int *r, *w, *e, *sr, *sw, *se;

    if(online()) {
	firetalk_getsockets(FP_IRC, &sr, &sw, &se);

	for(r = sr; *r; r++) res = res || FD_ISSET(*r, &rfds);
	for(w = sw; *w; w++) res = res || FD_ISSET(*w, &wfds);
	for(e = se; *e; e++) res = res || FD_ISSET(*e, &efds);

	delete sr;
	delete sw;
	delete se;
    }

    return res;
}

bool irchook::online() const {
    return fonline;
}

bool irchook::logged() const {
    return flogged && fonline;
}

bool irchook::enabled() const {
    return true;
}

bool irchook::isconnecting() const {
    return fonline && !flogged;
}

bool irchook::send(const imevent &ev) {
    icqcontact *c = clist.get(ev.getcontact());
    string text;

    if(c) {
	if(ev.gettype() == imevent::message) {
	    const immessage *m = static_cast<const immessage *>(&ev);
	    if(m) text = rusconv("kw", m->gettext());

	} else if(ev.gettype() == imevent::url) {
	    const imurl *m = static_cast<const imurl *>(&ev);
	    if(m) text = rusconv("kw", m->geturl()) + "\n\n" + rusconv("kw", m->getdescription());

	}

	return firetalk_im_send_message(handle, c->getdesc().nickname.c_str(), text.c_str(), 0) == FE_SUCCESS;
    }

    return false;
}

void irchook::sendnewuser(const imcontact &ic) {
    if(online()) {
	face.log(_("+ [irc] adding %s to the contacts"), ic.nickname.c_str());
	firetalk_im_add_buddy(handle, ic.nickname.c_str());
	requestinfo(ic);
    }
}

void irchook::removeuser(const imcontact &ic) {
    if(online()) {
	face.log(_("+ [irc] removing %s from the contacts"), ic.nickname.c_str());
	firetalk_im_remove_buddy(handle, ic.nickname.c_str());
    }
}

void irchook::setautostatus(imstatus st) {
    if(st != offline) {
	if(getstatus() == offline) {
	    connect();
	} else {
	    if(getstatus() != st) {
		logger.putourstatus(irc, getstatus(), ourstatus = st);

		switch(st) {
		    case away:
		    case notavail:
		    case occupied:
			firetalk_set_away(handle, conf.getawaymsg(irc).c_str());
			break;

		    default:
			firetalk_set_away(handle, 0);
			break;
		}
	    }
	}
    } else {
	if(getstatus() != offline) {
	    disconnect();
	}
    }

    face.update();
}

imstatus irchook::getstatus() const {
    return (flogged && fonline) ? ourstatus : offline;
}

void irchook::requestinfo(const imcontact &c) {
    firetalk_im_get_info(handle, c.nickname.c_str());
}

void irchook::sendupdateuserinfo(icqcontact &c, const string &newpass) {
    const icqcontact::basicinfo &b = c.getbasicinfo();

    ircname = b.fname;
    if(!b.lname.empty()) {
	if(!ircname.empty()) ircname += " ";
	ircname += b.lname;
    }

    saveconfig();
}

void irchook::lookup(const imsearchparams &params, verticalmenu &dest) {
    vector<channelInfo>::iterator ic;
    string rooms = params.room, room;
    bool ready = true;

    searchdest = &dest;
    searchchannels.clear();

    while(!userlist.empty()) {
	delete userlist.back();
	userlist.pop_back();
    }

    while(!(room = getword(rooms)).empty()) {
	searchchannels.push_back(room);
	ic = find(channels.begin(), channels.end(), room);

	if(ic == channels.end()) {
	    channels.push_back(channelInfo(room));
	    ic = channels.end()-1;
	}

	if(!ic->joined) {
	    firetalk_chat_join(handle, room.c_str());
	}

	if(ic->fetched) {
	    ic->nicks.clear();
	    firetalk_chat_listmembers(handle, room.c_str());
	} else {
	    ready = false;
	}
    }

    if(ready) {
	processnicks();
    }
}

void irchook::processnicks() {
    char *nick;
    vector<string> foundnicks;
    vector<string>::iterator in, isn;
    vector<channelInfo>::iterator ir;
    bool remove;

    ir = find(channels.begin(), channels.end(), *searchchannels.begin());

    foundnicks = ir->nicks;
    sort(foundnicks.begin(), foundnicks.end());

    if(searchchannels.size() > 1) {
	for(in = foundnicks.begin(); in != foundnicks.end(); ) {
	    remove = false;

	    for(ir = channels.begin(); !remove && (ir != channels.end()); ir++) {
		if(find(searchchannels.begin(), searchchannels.end(), ir->name) == searchchannels.end())
		    continue;
		if(ir->name == *searchchannels.begin())
		    continue;

		if(find(ir->nicks.begin(), ir->nicks.end(), *in) == ir->nicks.end()) {
		    /*
		    *
		    * Not found in one of other channels. Remove it from the
		    * first channel's list.
		    *
		    */
		    remove = true;
		}
	    }

	    if(remove) {
		foundnicks.erase(in);
		in = foundnicks.begin();
	    } else {
		in++;
	    }
	}
    }

    for(in = foundnicks.begin(); in != foundnicks.end(); in++) {
	userlist.push_back(nick = strdup(in->c_str()));
	searchdest->additem(0, nick, (string) " " + nick);
    }

    face.log(_("+ [irc] channel members list fetching finished, %d found"),
	userlist.size());

    searchdest->redraw();
}

vector<irchook::channelInfo> irchook::getautochannels() const {
    vector<channelInfo> r;
    vector<channelInfo>::const_iterator ic;

    for(ic = channels.begin(); ic != channels.end(); ic++) {
	if(find(searchchannels.begin(), searchchannels.end(), ic->name) != searchchannels.end())
	    if(!ic->joined)
		// A channel used for search
		continue;

	r.push_back(*ic);
    }

    return r;
}

void irchook::setautochannels(vector<channelInfo> &achannels) {
    bool r;
    vector<channelInfo>::iterator ic, iac;

    /*
    *
    * First, let's leave the channels we are not on anymore.
    *
    */

    for(ic = channels.begin(); ic != channels.end(); ic++) {
	if(ic->joined) {
	    iac = find(achannels.begin(), achannels.end(), ic->name);
	    r = iac == achannels.end();
	    if(!r) r = !iac->joined;

	    if(r) {
		firetalk_chat_part(irhook.handle, ic->name.c_str());
	    }
	}
    }

    /*
    *
    * Now, let's see if there are any new channels we
    * gotta join to.
    *
    */

    for(iac = achannels.begin(); iac != achannels.end(); iac++) {
	if(iac->joined) {
	    ic = find(channels.begin(), channels.end(), iac->name);
	    r = ic == channels.end();
	    if(!r) r = !ic->joined;

	    if(r) {
		firetalk_chat_join(irhook.handle, iac->name.c_str());
	    }
	}
    }

    channels = achannels;
    saveconfig();
}

void irchook::saveconfig() const {
    vector<channelInfo>::iterator ic;
    vector<channelInfo> savech = getautochannels();
    ofstream f(conf.getconfigfname("irc-profile").c_str());

    if(f.is_open()) {
	f << "name\t" << ircname << endl;

	for(ic = savech.begin(); ic != savech.end(); ic++)
	    f << "channel\t" << ic->name << "\t" << (ic->joined ? "1" : "0") << endl;

	f.close();
    }
}

void irchook::loadconfig() {
    string buf, param;
    vector<channelInfo>::iterator ic;

    ifstream f(conf.getconfigfname("irc-profile").c_str());

    if(f.is_open()) {
	channels.clear();

	while(!f.eof()) {
	    getstring(f, buf);
	    param = getword(buf);

	    if(param == "name") ircname = buf; else
	    if(param == "channel") {
		channels.push_back(channelInfo(getword(buf)));
		channels.back().joined = (buf == "1");
	    }
	}

	f.close();
    }
}

// ----------------------------------------------------------------------------

void irchook::userstatus(const string &nickname, imstatus st) {
    if(!nickname.empty()) {
	imcontact ic(nickname, irc);
	icqcontact *c = clist.get(ic);

	if(!c) {
	    c = clist.addnew(ic, false);
	}

	if(st != c->getstatus()) {
	    logger.putonline(ic, c->getstatus(), st);
	    c->setstatus(st);

	    if(c->getstatus() != offline) {
		c->setlastseen();
	    }
	}
    }
}

void irchook::connected(void *conn, void *cli, ...) {
    irhook.flogged = true;
    face.log(_("+ [irc] logged in"));
    face.update();

    int i;
    vector<channelInfo>::iterator ic;
    icqcontact *c;

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == irc) {
	    firetalk_im_add_buddy(irhook.handle, c->getdesc().nickname.c_str());
	}
    }

    for(ic = irhook.channels.begin(); ic != irhook.channels.end(); ic++) {
	firetalk_chat_join(irhook.handle, ic->name.c_str());
    }

    irhook.ourstatus = available;
    irhook.setautostatus(irhook.manualstatus);
}

void irchook::disconnected(void *conn, void *cli, ...) {
    irhook.fonline = false;
    logger.putourstatus(irc, irhook.getstatus(), offline);
    clist.setoffline(irc);

    face.log(_("+ [irc] disconnected from the network"));
    face.update();
}

void irchook::connectfailed(void *connection, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    int err = va_arg(ap, int);
    char *reason = va_arg(ap, char *);
    va_end(ap);

    irhook.fonline = false;

    face.log(_("+ [irc] connect failed: %s"), reason);
    face.update();
}

void irchook::newnick(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *nick = va_arg(ap, char *);
    va_end(ap);
/*
    if(nick)
    if(strlen(nick)) {
	icqconf::imaccount acc = conf.getourid(irc);
	acc.nickname = nick;
	conf.setourid(acc);

	face.log(_("+ [irc] nickname was changed successfully"));
    }
*/
    DLOG("newnick");
}

void irchook::gotinfo(void *conn, void *cli, ...) {
    int pos;
    va_list ap;
    string about, email;
    icqcontact::basicinfo cbinfo;

    va_start(ap, cli);
    char *nick = va_arg(ap, char *);
    char *info = va_arg(ap, char *);
    int warning = va_arg(ap, int);
    int idle = va_arg(ap, int);
    va_end(ap);

    if(nick && info)
    if(strlen(nick) && strlen(info)) {
	icqcontact *c = clist.get(imcontact(nick, irc));

	if(!c) c = clist.get(contactroot);

	about = info;
	cbinfo = c->getbasicinfo();

	if((pos = about.find(":")) != -1) {
	    email = about.substr(0, pos);
	    about.erase(0, pos);

	    if(email.substr(0, 1) == "~") email.erase(0, 1);
	    if((pos = about.find_first_not_of(" :")) > 0) about.erase(0, pos);
	}

	cbinfo.email = email;

	if((pos = about.find(" ")) != -1) {
	    cbinfo.lname = about.substr(pos+1);
	    about.erase(pos);
	}

	cbinfo.fname = about;

	c->setnick(nick);
	c->setbasicinfo(cbinfo);
    }

    DLOG("gotinfo");
}

void irchook::gotchannels(void *conn, void *cli, ...) {
    va_list ap;
    string info;
    icqcontact *c;

    va_start(ap, cli);
    char *nick = va_arg(ap, char *);
    char *channels = va_arg(ap, char *);
    va_end(ap);

    if(nick && channels)
    if(strlen(nick) && strlen(channels)) {
	c = clist.get(imcontact(nick, irc));
	if(!c) c = clist.get(contactroot);

	c->setabout((string) _("On channels: ") + channels);
    }

    DLOG("gotchannels");
}

void irchook::getmessage(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *sender = va_arg(ap, char *);
    int automessage_flag = va_arg(ap, int);
    char *message = va_arg(ap, char *);
    va_end(ap);

    if(sender && message)
    if(strlen(sender) && strlen(message)) {
	em.store(immessage(imcontact(sender, irc),
	    imevent::incoming, rusconv("wk", message)));
    }

    DLOG("getmessage");
}

void irchook::buddyonline(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *nick = va_arg(ap, char *);
    va_end(ap);

    if(nick)
    if(strlen(nick)) {
	irhook.userstatus(nick, available);
    }

    DLOG("buddyonline");
}

void irchook::buddyoffline(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *nick = va_arg(ap, char *);
    va_end(ap);

    if(nick)
    if(strlen(nick)) {
	irhook.userstatus(nick, offline);
    }

    DLOG("buddyoffline");
}

void irchook::buddyaway(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *nick = va_arg(ap, char *);
    va_end(ap);

    if(nick)
    if(strlen(nick)) {
	irhook.userstatus(nick, away);
    }

    DLOG("buddyaway");
}

void irchook::listmember(void *connection, void *cli, ...) {
    va_list ap;
    vector<channelInfo>::iterator ir;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *membername = va_arg(ap, char *);
    int opped = va_arg(ap, int);
    va_end(ap);

    if(membername)
    if(strlen(membername)) {
	ir = find(irhook.channels.begin(), irhook.channels.end(), room);

	if(ir != irhook.channels.end()) {
	    ir->nicks.push_back(/*(string) (opped ? "@": "") +*/ membername);
	}
    }
}

void irchook::log(void *connection, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *msg = va_arg(ap, char *);
    va_end(ap);

    face.log("irc: %s", msg);
}

void irchook::chatnames(void *connection, void *cli, ...) {
    va_list ap;
    vector<channelInfo>::iterator ic;
    vector<string>::iterator is;
    bool ready = true;

    DLOG("chatnames");

    va_start(ap, cli);
    char *croom = va_arg(ap, char *);
    va_end(ap);

    ic = find(irhook.channels.begin(), irhook.channels.end(), croom);

    if(ic != irhook.channels.end()) {
	ic->fetched = true;
	ic->nicks.clear();

	firetalk_chat_listmembers(irhook.handle, croom);

	if(!ic->joined) {
	    firetalk_chat_part(irhook.handle, croom);
	}
    }

    if(irhook.searchdest) {
	for(is = irhook.searchchannels.begin(); is != irhook.searchchannels.end(); is++) {
	    ic = find(irhook.channels.begin(), irhook.channels.end(), *is);

	    if(ic != irhook.channels.end())
	    if(!ic->fetched) {
		ready = false;
		break;
	    }
	}

	if(ready) {
	    irhook.processnicks();
	}
    }
}

// ----------------------------------------------------------------------------

static string up(string s) {
    int k;
    string r;

    for(k = 0; k < s.size(); k++)
	r += toupper(s[k]);

    return r;
}

bool irchook::channelInfo::operator != (const string &aname) const {
    return up(aname) != up(name);
}

bool irchook::channelInfo::operator == (const string &aname) const {
    return up(aname) == up(name);
}
