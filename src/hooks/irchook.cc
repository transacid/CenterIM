/*
*
* centericq IRC protocol handling class
* $Id: irchook.cc,v 1.80 2004/12/20 00:54:02 konst Exp $
*
* Copyright (C) 2001-2004 by Konstantin Klyagin <konst@konst.org.ua>
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

#ifdef BUILD_IRC

#include "icqface.h"
#include "icqcontacts.h"
#include "imlogger.h"

#include "accountmanager.h"
#include "eventmanager.h"

#include <iterator>

// ----------------------------------------------------------------------------

irchook irhook;

irchook::irchook()
    : abstracthook(irc), handle(firetalk_create_handle(FP_IRC, 0)),
      fonline(false), flogged(false), ourstatus(offline)
{
    fcapabs.insert(hookcapab::setaway);
    fcapabs.insert(hookcapab::fetchaway);
    fcapabs.insert(hookcapab::changenick);
    fcapabs.insert(hookcapab::optionalpassword);
    fcapabs.insert(hookcapab::ping);
    fcapabs.insert(hookcapab::version);
    fcapabs.insert(hookcapab::files);
    fcapabs.insert(hookcapab::cltemporary);
    fcapabs.insert(hookcapab::directadd);
    fcapabs.insert(hookcapab::conferencing);
    fcapabs.insert(hookcapab::channelpasswords);
}

irchook::~irchook() {
}

void irchook::init() {
    int i;

    manualstatus = conf.getstatus(irc);

    for(i = 0; i < clist.count; i++) {
	icqcontact *c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == irc)
	if(ischannel(c))
	if(c->getbasicinfo().requiresauth) {
	    channels.push_back(channelInfo(c->getdesc().nickname));
	    channels.back().joined = true;
	    channels.back().passwd = c->getbasicinfo().zip;
	}
    }

    firetalk_register_callback(handle, FC_CONNECTED, &connected);
    firetalk_register_callback(handle, FC_CONNECTFAILED, &connectfailed);
    firetalk_register_callback(handle, FC_DISCONNECT, &disconnected);
    firetalk_register_callback(handle, FC_NEWNICK, &newnick);
    firetalk_register_callback(handle, FC_IM_GOTINFO, &gotinfo);
    firetalk_register_callback(handle, FC_IM_GOTCHANNELS, &gotchannels);
    firetalk_register_callback(handle, FC_IM_GETMESSAGE, &getmessage);
    firetalk_register_callback(handle, FC_IM_GETACTION, &getaction);
    firetalk_register_callback(handle, FC_IM_BUDDYONLINE, &buddyonline);
    firetalk_register_callback(handle, FC_IM_BUDDYOFFLINE, &buddyoffline);
    firetalk_register_callback(handle, FC_IM_BUDDYAWAY, &buddyaway);
    firetalk_register_callback(handle, FC_IM_BUDDYUNAWAY, &buddyonline);
    firetalk_register_callback(handle, FC_CHAT_LISTMEMBER, &listmember);
    firetalk_register_callback(handle, FC_CHAT_LIST_EXTENDED, &listextended);
    firetalk_register_callback(handle, FC_CHAT_END_EXTENDED, &endextended);
    firetalk_register_callback(handle, FC_CHAT_NAMES, &chatnames);
    firetalk_register_callback(handle, FC_CHAT_GETMESSAGE, &chatmessage);
    firetalk_register_callback(handle, FC_CHAT_GETACTION, &chataction);
    firetalk_register_callback(handle, FC_CHAT_JOINED, &chatjoined);
    firetalk_register_callback(handle, FC_CHAT_LEFT, &chatleft);
    firetalk_register_callback(handle, FC_CHAT_KICKED, &chatkicked);
    firetalk_register_callback(handle, FC_CHAT_OPPED, &chatopped);
    firetalk_register_callback(handle, FC_CHAT_DEOPPED, &chatdeopped);
    firetalk_register_callback(handle, FC_ERROR, &errorhandler);
    firetalk_register_callback(handle, FC_IM_USER_NICKCHANGED, &nickchanged);
    firetalk_register_callback(handle, FC_NEEDPASS, &needpass);
    firetalk_register_callback(handle, FC_FILE_OFFER, &fileoffer);
    firetalk_register_callback(handle, FC_FILE_START, &filestart);
    firetalk_register_callback(handle, FC_FILE_PROGRESS, &fileprogress);
    firetalk_register_callback(handle, FC_FILE_FINISH, &filefinish);
    firetalk_register_callback(handle, FC_FILE_ERROR, &fileerror);
    firetalk_register_callback(handle, FC_CHAT_USER_JOINED, &chatuserjoined);
    firetalk_register_callback(handle, FC_CHAT_USER_LEFT, &chatuserleft);
    firetalk_register_callback(handle, FC_CHAT_USER_KICKED, &chatuserkicked);
    firetalk_register_callback(handle, FC_CHAT_GOTTOPIC, &chatgottopic);
    firetalk_register_callback(handle, FC_CHAT_USER_OPPED, &chatuseropped);
    firetalk_register_callback(handle, FC_CHAT_USER_DEOPPED, &chatuserdeopped);

    firetalk_subcode_register_request_callback(handle, "VERSION", &subrequest);

    firetalk_subcode_register_reply_callback(handle, "PING", &subreply);
    firetalk_subcode_register_reply_callback(handle, "VERSION", &subreply);

    if(conf.getdebug())
	firetalk_register_callback(handle, FC_LOG, &fclog);
}

void irchook::connect() {
    icqconf::imaccount acc = conf.getourid(irc);
    log(logConnecting);

    firetalk_register_callback(handle, FC_DISCONNECT, 0);
    firetalk_disconnect(handle);
    firetalk_register_callback(handle, FC_DISCONNECT, &disconnected);

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
    string text, cmd;

    if(c) {
	if(ev.gettype() == imevent::message) {
	    const immessage *m = static_cast<const immessage *>(&ev);
	    text = rushtmlconv("kw", m->gettext());

	} else if(ev.gettype() == imevent::url) {
	    const imurl *m = static_cast<const imurl *>(&ev);
	    text = rushtmlconv("kw", m->geturl()) + "\n\n" + rushtmlconv("kw", m->getdescription());

	} else if(ev.gettype() == imevent::file) {
	    const imfile *m = static_cast<const imfile *>(&ev);
	    vector<imfile::record> files = m->getfiles();
	    vector<imfile::record>::const_iterator ir;

	    for(ir = files.begin(); ir != files.end(); ++ir) {
		imfile::record r;
		r.fname = ir->fname;
		r.size = ir->size;

		imfile fr(c->getdesc(), imevent::outgoing, "",
		    vector<imfile::record>(1, r));

		firetalk_file_offer(handle, c->getdesc().nickname.c_str(),
		    ir->fname.c_str(), &transferinfo[fr].first);
	    }

	    return true;
	}

	if(text.substr(0, 1) == "/") {
	    text.erase(0, 1);
	    cmd = up(getword(text));

	    if(cmd == "ME") {
		if(ischannel(c)) return firetalk_chat_send_action(handle,
		    c->getdesc().nickname.c_str(), text.c_str(), 0) == FE_SUCCESS;
		else return firetalk_im_send_action(handle,
		    c->getdesc().nickname.c_str(), text.c_str(), 0) == FE_SUCCESS;

	    } else if(cmd == "KICK") {
		text = cmd + " " + c->getdesc().nickname + " " + text;

	    } else if(cmd == "OP") {
		text = (string) "mode " + c->getdesc().nickname + " +o " + text;

	    } else if(cmd == "DEOP") {
		text = (string) "mode " + c->getdesc().nickname + " -o " + text;

	    } else if(cmd == "RAW") {
		// leave text intact

	    } else {
		text.insert(0, cmd + " ");

	    }

	    rawcommand(text);
	}

	if(ischannel(c)) {
	    return firetalk_chat_send_message(handle,
		c->getdesc().nickname.c_str(), text.c_str(), 0) == FE_SUCCESS;

	} else {
	    return firetalk_im_send_message(handle,
		c->getdesc().nickname.c_str(), text.c_str(), 0) == FE_SUCCESS;

	}
    }

    return false;
}

void irchook::sendnewuser(const imcontact &ic) {
    icqcontact *c;

    if(online()) {
	if(!ischannel(ic)) {
	    firetalk_im_add_buddy(handle, ic.nickname.c_str());
	    requestinfo(ic);
	} else {
	    vector<channelInfo> ch = getautochannels();
	    if(find(ch.begin(), ch.end(), ic.nickname) == ch.end()) {
		ch.push_back(ic.nickname);
		ch.back().joined = true;
		if(c = clist.get(ic)) {
		    ch.back().passwd = c->getbasicinfo().zip;
		}
		setautochannels(ch);
	    }
	}
    }
}

void irchook::removeuser(const imcontact &ic) {
    if(online()) {
	if(!ischannel(ic)) {
	    firetalk_im_remove_buddy(handle, ic.nickname.c_str());
	} else {
	    vector<channelInfo> ch = getautochannels();
	    vector<channelInfo>::iterator ich = find(ch.begin(), ch.end(), ic.nickname);
	    if(ich != ch.end()) {
		ch.erase(ich);
		setautochannels(ch);
	    }
	}
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
}

void irchook::lookup(const imsearchparams &params, verticalmenu &dest) {
    string rooms = params.room, room;
    bool ready = true;
    vector<channelInfo>::iterator ic;

    searchdest = &dest;

    emailsub = params.email;
    namesub = params.firstname;
    searchsincelast = params.sincelast && !params.room.empty();

    searchchannels.clear();
    extlisted.clear();

    while(!foundguys.empty()) {
	delete foundguys.back();
	foundguys.pop_back();
    }

    if(!params.room.empty()) {
	smode = Channel;

	while(!(room = getword(rooms)).empty()) {
	    if(room[0] != '#') room.insert(0, "#");
	    searchchannels.push_back(room);
	    ic = find(channels.begin(), channels.end(), room);

	    if(ic == channels.end()) {
		channels.push_back(channelInfo(room));
		ic = channels.end()-1;
	    }

	    if(!ic->joined) {
		firetalk_chat_join(handle, room.c_str(), ic->passwd.c_str());
		ready = false;
	    }

	    if(emailsub.empty() && namesub.empty()) {
		if(ic->fetched) {
		    ic->nicks.clear();
		    firetalk_chat_listmembers(handle, room.c_str());
		} else {
		    ready = false;
		}

	    } else {
		ready = false;
		if(ic->joined) {
		    ic->nicks.clear();
		    firetalk_chat_requestextended(handle, params.room.c_str());
		}

	    }
	}

	if(ready)
	    processnicks();

    } else if(!params.email.empty()) {
	smode = Email;
	searchchannels.push_back("");
	firetalk_im_searchemail(handle, ("*" + params.email + "*").c_str());

    }
}

void irchook::processnicks() {
    string dnick;
    char *nick;
    vector<string> foundnicks;
    static vector<string> lastfoundnicks;
    vector<string>::iterator in, isn;
    vector<channelInfo>::iterator ir;
    bool remove;
    icqcontact *c;
    int npos;

    if(!searchdest)
	return;

    ir = find(channels.begin(), channels.end(), *searchchannels.begin());
    if(ir != channels.end())
	foundnicks = ir->nicks;

    sort(foundnicks.begin(), foundnicks.end());

    if(searchchannels.size() > 1) {
	for(in = foundnicks.begin(); in != foundnicks.end(); ) {
	    remove = false;

	    for(ir = channels.begin(); !remove && (ir != channels.end()); ++ir) {
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

    if(searchsincelast) {
	vector<string> tnicks;
	insert_iterator< vector<string> > tins(tnicks, tnicks.begin());

	set_difference(foundnicks.begin(), foundnicks.end(),
	    lastfoundnicks.begin(), lastfoundnicks.end(), tins);

	lastfoundnicks = foundnicks;
	foundnicks = tnicks;

    } else {
	lastfoundnicks = foundnicks;

    }

    for(in = foundnicks.begin(); in != foundnicks.end(); ++in) {
	dnick = *in;

	npos = 0;

	if(!emailsub.empty()) npos = dnick.rfind("<"); else
	if(!namesub.empty()) npos =  dnick.rfind("[");

	if(npos > 0) dnick.erase(npos-1);

	c = new icqcontact(imcontact(dnick, irc));
	c->setdispnick(dnick);

	searchdest->additem(conf.getcolor(cp_clist_irc), c, (string) " " + *in);
	foundguys.push_back(c);
    }

    face.findready();
    log(logConfMembers, foundguys.size());

    searchdest->redraw();
    searchdest = 0;
}

vector<irchook::channelInfo> irchook::getautochannels() const {
    vector<channelInfo> r;
    vector<channelInfo>::const_iterator ic;

    for(ic = channels.begin(); ic != channels.end(); ++ic) {
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

    for(ic = channels.begin(); ic != channels.end(); ++ic) {
	iac = find(achannels.begin(), achannels.end(), ic->name);

	if(ic->joined) {
	    r = iac == achannels.end();
	    if(!r) r = !iac->joined;
	    if(r) firetalk_chat_part(irhook.handle, ic->name.c_str());
	}
    }

    /*
    *
    * Now, let's see if there are any new channels we
    * gotta join to.
    *
    */

    for(iac = achannels.begin(); iac != achannels.end(); ++iac) {
	ic = find(channels.begin(), channels.end(), iac->name);

	if(iac->joined) {
	    r = ic == channels.end();
	    if(!r) r = !ic->joined;
	    if(r) firetalk_chat_join(irhook.handle, iac->name.c_str(), iac->passwd.c_str());
	}
    }

    channels = achannels;
}

void irchook::requestawaymsg(const imcontact &ic) {
    em.store(imnotification(ic, string() +
	_("Away message:") + "\n\n" + awaymessages[ic.nickname]));
}

void irchook::rawcommand(const string &cmd) {
    int *r, *w, *e, sock;
    if(online()) {
	firetalk_getsockets(FP_IRC, &r, &w, &e);

	if(*r) {
	    write(*r, cmd.c_str(), cmd.size());
	    write(*r, "\n", 1);
	}

	delete r;
	delete w;
	delete e;
    }
}

void irchook::channelfatal(string room, const char *fmt, ...) {
    va_list ap;
    char buf[1024];
    vector<channelInfo>::iterator i;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    if(room.substr(0, 1) != "#")
	room.insert(0, "#");

    i = find(channels.begin(), channels.end(), room);

    if(i != channels.end()) {
	imcontact cont(room, irc);
	icqcontact *c = clist.get(cont);
	if(!c) c = clist.addnew(cont);
	c->setstatus(offline);
	i->joined = i->fetched = false;
	em.store(imnotification(cont, buf));
    }
}

void irchook::ouridchanged(const icqconf::imaccount &ia) {
    if(logged()) {
	firetalk_set_nickname(handle, ia.nickname.c_str());
    }
}

void irchook::requestversion(const imcontact &c) {
    if(logged()) {
	firetalk_subcode_send_request(handle, c.nickname.c_str(), "VERSION", 0);
    }
}

void irchook::ping(const imcontact &c) {
    if(logged()) {
	firetalk_subcode_send_request(handle, c.nickname.c_str(), "PING", 0);
	time(&pingtime[up(c.nickname)]);
    }
}

bool irchook::knowntransfer(const imfile &fr) const {
    return transferinfo.find(fr) != transferinfo.end();
}

void irchook::replytransfer(const imfile &fr, bool accept, const string &localpath) {
    if(accept) {
	transferinfo[fr].second = localpath;

	if(transferinfo[fr].second.substr(transferinfo[fr].second.size()-1) != "/")
	    transferinfo[fr].second += "/";

	transferinfo[fr].second += justfname(fr.getfiles().begin()->fname);

	firetalk_file_accept(handle, transferinfo[fr].first, 0,
	    transferinfo[fr].second.c_str());

    } else {
	firetalk_file_refuse(handle, transferinfo[fr].first);
	transferinfo.erase(fr);

    }
}

void irchook::aborttransfer(const imfile &fr) {
    firetalk_file_cancel(handle, transferinfo[fr].first);

    face.transferupdate(fr.getfiles().begin()->fname, fr,
	icqface::tsCancel, 0, 0);

    irhook.transferinfo.erase(fr);
}

bool irchook::getfevent(void *fhandle, imfile &fr) {
    map<imfile, pair<void *, string> >::const_iterator i = transferinfo.begin();

    while(i != transferinfo.end()) {
	if(i->second.first == fhandle) {
	    fr = i->first;
	    return true;
	}
	++i;
    }

    return false;
}

// ----------------------------------------------------------------------------

void irchook::userstatus(const string &nickname, imstatus st) {
    if(!nickname.empty()) {
	imcontact ic(nickname, irc);
	icqcontact *c = clist.get(ic);

	if(c)
	if(st != c->getstatus()) {
	    logger.putonline(ic, c->getstatus(), st);
	    c->setstatus(st);
	}
    }
}

void irchook::connected(void *conn, void *cli, ...) {
    irhook.flogged = true;
    irhook.log(logLogged);
    face.update();

    int i;
    vector<channelInfo>::iterator ic;
    icqcontact *c;

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == irc)
	if(!ischannel(c)) {
	    firetalk_im_add_buddy(irhook.handle, c->getdesc().nickname.c_str());
	}
    }

    for(ic = irhook.channels.begin(); ic != irhook.channels.end(); ++ic) {
	if(ic->joined) {
	    firetalk_chat_join(irhook.handle, ic->name.c_str(), ic->passwd.c_str());
	}
    }

    irhook.ourstatus = available;
    irhook.setautostatus(irhook.manualstatus);
    irhook.awaymessages.clear();
    irhook.sentpass = false;
}

void irchook::disconnected(void *conn, void *cli, ...) {
    irhook.fonline = false;

    logger.putourstatus(irc, irhook.getstatus(), offline);
    clist.setoffline(irc);

    vector<channelInfo>::iterator ic;
    for(ic = irhook.channels.begin(); ic != irhook.channels.end(); ++ic)
	ic->fetched = false;

    irhook.log(logDisconnected);
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

    if(nick)
    if(strlen(nick)) {
	icqconf::imaccount acc = conf.getourid(irc);
	acc.nickname = nick;
	conf.setourid(acc);

	face.log(_("+ [irc] nickname was changed successfully"));
    }
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

	cbinfo.lname = "";
	cbinfo.email = irhook.rushtmlconv("wk", email);

	if((pos = about.find(" ")) != -1) {
	    cbinfo.lname = irhook.rushtmlconv("wk", about.substr(pos+1));
	    about.erase(pos);
	}

	cbinfo.fname = irhook.rushtmlconv("wk", about);

	c->setnick(nick);
	c->setbasicinfo(cbinfo);

	if(c->getstatus() == offline)
	    c->setstatus(available);
    }
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

	if(c->getstatus() == offline)
	    c->setstatus(available);
    }
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
	if(!irhook.sentpass)
	if(up(sender) == "NICKSERV") {
	    firetalk_im_send_message(irhook.handle, "NickServ",
		((string) "identify " + conf.getourid(irc).additional["nickpass"]).c_str(), 0);

	    irhook.sentpass = true;
	}

	em.store(immessage(imcontact(sender, irc),
	    imevent::incoming, irhook.rushtmlconv("wk", cuthtml(message, chCutBR | chLeaveLinks))));
    }
}

void irchook::getaction(void *conn, void *cli, ...) {
    va_list ap;
  
    va_start(ap, cli);
    char *sender = va_arg(ap, char *);
    int automessage_flag = va_arg(ap, int);
    char *message = va_arg(ap, char *);
    va_end(ap);
    
    if(sender && message)
    if(strlen(sender) && strlen(message)) {
	em.store(immessage(imcontact(sender, irc), imevent::incoming, 
	    ((string) "* " + sender + " " + 
		irhook.rushtmlconv("wk", cuthtml(message, chCutBR | chLeaveLinks))).c_str()));
    }
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
}

void irchook::buddyaway(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *nick = va_arg(ap, char *);
    char *msg = va_arg(ap, char *);
    va_end(ap);

    if(nick)
    if(strlen(nick)) {
	irhook.userstatus(nick, away);
	if(msg) irhook.awaymessages[nick] = irhook.rushtmlconv("wk", msg);
    }
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

void irchook::fclog(void *connection, void *cli, ...) {
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

    va_start(ap, cli);
    char *croom = va_arg(ap, char *);
    va_end(ap);

    ic = find(irhook.channels.begin(), irhook.channels.end(), croom);

    if(ic != irhook.channels.end()) {
	ic->fetched = true;
	ic->nicks.clear();

	if(irhook.searchdest) {
	    if(irhook.emailsub.empty() && irhook.namesub.empty()) {
		firetalk_chat_listmembers(irhook.handle, croom);
		if(!ic->joined) firetalk_chat_part(irhook.handle, croom);
	    } else {
		firetalk_chat_requestextended(irhook.handle, croom);
	    }
	}
    }

    if(irhook.searchdest && irhook.emailsub.empty() && irhook.namesub.empty()) {
	for(is = irhook.searchchannels.begin(); is != irhook.searchchannels.end(); ++is) {
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

void irchook::listextended(void *connection, void *cli, ...) {
    va_list ap;
    string text, email, name;
    vector<channelInfo>::iterator ir;
    vector<string>::iterator is;
    int npos;

    va_start(ap, cli);
    char *nickname = va_arg(ap, char *);
    char *room = va_arg(ap, char *);
    char *login = va_arg(ap, char *);
    char *hostname = va_arg(ap, char *);
    char *net = va_arg(ap, char *);
    char *description = va_arg(ap, char *);
    va_end(ap);

    ir = find(irhook.channels.begin(), irhook.channels.end(), room);

    if(irhook.smode == Email) {
	ir = find(irhook.channels.begin(), irhook.channels.end(), "");

	if(ir == irhook.channels.end()) {
	    irhook.channels.push_back(channelInfo(""));
	    ir = irhook.channels.end()-1;
	}
    }

    if(ir != irhook.channels.end()) {
	name = description;
	if((npos = name.find(" ")) != -1)
	    name.erase(0, npos+1);

	email = (string) login + "@" + hostname;

	if(irhook.emailsub.empty() || email.find(irhook.emailsub) != -1 || irhook.smode == Email) {
	    if(irhook.namesub.empty() || name.find(irhook.namesub) != -1) {
		text = nickname;

		if(!irhook.emailsub.empty()) {
		    text += " <" + email + ">";
		} else if(!irhook.namesub.empty()) {
		    text += " [" + name + "]";
		}

		ir->nicks.push_back(text);
	    }
	}
    }

    if(find(irhook.extlisted.begin(), irhook.extlisted.end(), room) == irhook.extlisted.end())
	irhook.extlisted.push_back(room);
}

void irchook::endextended(void *connection, void *cli, ...) {
    bool ready = true;
    vector<string>::iterator is;
    vector<channelInfo>::iterator ic;

    if(irhook.smode == Channel && !irhook.extlisted.empty()) {
	ic = find(irhook.channels.begin(), irhook.channels.end(), irhook.extlisted.back());

	if(ic != irhook.channels.end()) {
	    if(!ic->joined)
		firetalk_chat_part(irhook.handle, irhook.extlisted.back().c_str());

	    for(is = irhook.searchchannels.begin(); ready && is != irhook.searchchannels.end(); ++is)
		ready = find(irhook.extlisted.begin(), irhook.extlisted.end(), *is) != irhook.extlisted.end();
	}
    }

    ready = ready || irhook.smode == Email;

    if(ready) {
	irhook.processnicks();

	if(irhook.smode == Email) {
	    ic = find(irhook.channels.begin(), irhook.channels.end(), "");
	    if(ic != irhook.channels.end()) irhook.channels.erase(ic);
	}
    }
}

void irchook::chatmessage(void *connection, void *cli, ...) {
    va_list ap;
    string imsg;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *from = va_arg(ap, char *);
    int automessage = va_arg(ap, int);
    char *msg = va_arg(ap, char *);
    va_end(ap);

    if(clist.get(imcontact(room, irc))) {
	imsg = (string) from + ": " + msg;
	getmessage(connection, cli, room, automessage, imsg.c_str());
    }
}

void irchook::chataction(void *connection, void *cli, ...) {
    va_list ap;
    string imsg;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *from = va_arg(ap, char *);
    int automessage = va_arg(ap, int);
    char *msg = va_arg(ap, char *);
    va_end(ap);

    if(from && msg)
    if(strlen(from) && strlen(msg))
    if(clist.get(imcontact(room, irc))) {
	em.store(immessage(imcontact(room, irc), imevent::incoming,
	    ((string) "* " + from + " " +
		irhook.rushtmlconv("wk", cuthtml(msg, chCutBR | chLeaveLinks))).c_str()));
    }
}


void irchook::chatjoined(void *connection, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    va_end(ap);

    icqcontact *c = clist.get(imcontact(room, irc));
    if(c) c->setstatus(available, false);
}

void irchook::chatleft(void *connection, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    va_end(ap);

    icqcontact *c = clist.get(imcontact(room, irc));
    if(c) c->setstatus(offline, false);
}

void irchook::chatkicked(void *connection, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *by = va_arg(ap, char *);
    char *reason = va_arg(ap, char *);
    va_end(ap);

    irhook.channelfatal(room, _("Kicked by %s; reason: %s"),
	by, irhook.rushtmlconv("wk", reason).c_str());
}

void irchook::errorhandler(void *connection, void *cli, ...) {
    va_list ap;
    icqcontact *c;

    va_start(ap, cli);
    int error = va_arg(ap, int);
    char *subject = va_arg(ap, char *);
    char *description = va_arg(ap, char *);
    va_end(ap);

    switch(error) {
	case FE_ROOMUNAVAILABLE:
	    // Cannot join channel
	    if(subject)
	    if(strlen(subject))
		irhook.channelfatal(subject, "%s", description);
	    break;
	case FE_BADUSER:
	    // Cannot fetch user's info
	    if(subject)
	    if(strlen(subject))
	    if(c = clist.get(imcontact(subject, irc)))
		c->setstatus(offline);
	    break;
    }
}

void irchook::nickchanged(void *connection, void *cli, ...) {
    va_list ap;
    icqcontact *c;
    char buf[100];

    va_start(ap, cli);
    char *oldnick = va_arg(ap, char *);
    char *newnick = va_arg(ap, char *);
    va_end(ap);

    if(oldnick && newnick)
    if(strcmp(oldnick, newnick)) {
	if(c = clist.get(imcontact(oldnick, irc))) {
	    if(!clist.get(imcontact(newnick, irc))) {
		if(!c->inlist()) {
		    if(c->getdispnick() == oldnick) c->setdispnick(newnick);
		    c->setdesc(imcontact(newnick, irc));
		    c->setnick(newnick);
		}

	    } else {
		c->setstatus(offline);

	    }

	    sprintf(buf, _("The user has changed their nick from %s to %s"), oldnick, newnick);
	    em.store(imnotification(c, buf));
	}
    }
}

void irchook::needpass(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *pass = va_arg(ap, char *);
    int size = va_arg(ap, int);
    va_end(ap);

    if(pass) {
	icqconf::imaccount acc = conf.getourid(irc);

	if(!acc.password.empty()) {
	    strncpy(pass, acc.password.c_str(), size-1);
	    pass[size-1] = 0;
	    face.log(_("+ [irc] password sent"));
	}
    }
}

void irchook::subrequest(void *conn, void *cli, const char * const nick,
const char * const command, const char * const args) {

    if(!strcmp(command, "VERSION")) {
	firetalk_subcode_send_reply(conn, nick, "VERSION", PACKAGE " " VERSION);

    }
}

void irchook::subreply(void *conn, void *cli, const char * const nick,
const char * const command, const char * const args) {
    char buf[512];

    if(!strcmp(command, "PING")) {
	map<string, time_t>::iterator i = irhook.pingtime.find(up(nick));

	if(i != irhook.pingtime.end()) {
	    sprintf(buf, _("PING reply from the user: %d second(s)"), time(0)-i->second);
	    em.store(imnotification(imcontact(nick, irc), buf));
	}

    } else if(!strcmp(command, "VERSION")) {
	sprintf(buf, _("The remote is using %s"), args);
	em.store(imnotification(imcontact(nick, irc), buf));

    }
}

void irchook::fileoffer(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    void *filehandle = va_arg(ap, void *);
    char *from = va_arg(ap, char *);
    char *filename = va_arg(ap, char *);
    long size = va_arg(ap, long);
    va_end(ap);

    imfile::record r;
    r.fname = filename;
    r.size = size;

    imfile fr(imcontact(from, irc), imevent::incoming, "",
	vector<imfile::record>(1, r));

    irhook.transferinfo[fr].first = filehandle;
    em.store(fr);

    face.transferupdate(filename, fr, icqface::tsInit, size, 0);
}

void irchook::filestart(void *conn, void *cli, ...) {
    va_list ap;
    imfile fr;

    va_start(ap, cli);
    void *filehandle = va_arg(ap, void *);
    void *clientfilestruct = va_arg(ap, void *);
    va_end(ap);

    if(irhook.getfevent(filehandle, fr)) {
	face.transferupdate(fr.getfiles().begin()->fname, fr,
	    icqface::tsStart, 0, 0);
    }
}

void irchook::fileprogress(void *conn, void *cli, ...) {
    va_list ap;
    imfile fr;

    va_start(ap, cli);
    void *filehandle = va_arg(ap, void *);
    void *clientfilestruct = va_arg(ap, void *);
    long bytes = va_arg(ap, long);
    long size = va_arg(ap, long);
    va_end(ap);

    if(irhook.getfevent(filehandle, fr)) {
	face.transferupdate(fr.getfiles().begin()->fname, fr,
	    icqface::tsProgress, size, bytes);
    }
}

void irchook::filefinish(void *conn, void *cli, ...) {
    va_list ap;
    imfile fr;

    va_start(ap, cli);
    void *filehandle = va_arg(ap, void *);
    void *clientfilestruct = va_arg(ap, void *);
    long size = va_arg(ap, long);
    va_end(ap);

    if(irhook.getfevent(filehandle, fr)) {
	face.transferupdate(fr.getfiles().begin()->fname, fr,
	    icqface::tsFinish, size, 0);

	irhook.transferinfo.erase(fr);
    }
}

void irchook::fileerror(void *conn, void *cli, ...) {
    va_list ap;
    imfile fr;

    va_start(ap, cli);
    void *filehandle = va_arg(ap, void *);
    void *clientfilestruct = va_arg(ap, void *);
    int error = va_arg(ap, int);
    va_end(ap);

    if(irhook.getfevent(filehandle, fr)) {
	face.transferupdate(fr.getfiles().begin()->fname, fr,
	    icqface::tsError, 0, 0);

	irhook.transferinfo.erase(fr);
    }
}

void irchook::chatuserjoined(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *who = va_arg(ap, char *);
    char *email = va_arg(ap, char *);
    va_end(ap);

    if(conf.getourid(irc).nickname != who) {
	string uname = who;

	if(email)
	if(strlen(email))
	    uname += (string) " (" + email + ")";

	char buf[512];
	sprintf(buf, _("%s has joined."), uname.c_str());
	em.store(imnotification(imcontact(room, irc), buf));
    }
}

void irchook::chatuserleft(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *who = va_arg(ap, char *);
    char *reason = va_arg(ap, char *);
    va_end(ap);

    if(conf.getourid(irc).nickname != who) {
	string text;
	char buf[512];

	sprintf(buf, _("%s has left"), who); text = buf;

	if(reason)
	if(strlen(reason)) {
	    if(strlen(reason) > 450) reason[450] = 0;
	    sprintf(buf, _("reason: %s"), reason);
	    text += (string) "; " + buf + ".";
	}

	em.store(imnotification(imcontact(room, irc), text));
    }
}

void irchook::chatuserkicked(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *who = va_arg(ap, char *);
    char *by = va_arg(ap, char *);
    char *reason = va_arg(ap, char *);
    va_end(ap);

    if(conf.getourid(irc).nickname != who) {
	string text;
	char buf[512];

	sprintf(buf, _("%s has been kicked by %s"), who, by); text = buf;

	if(reason)
	if(strlen(reason)) {
	    sprintf(buf, _("reason: %s"), reason);
	    text += (string) "; " + buf + ".";
	}

	em.store(imnotification(imcontact(room, irc), text));
    }
}

void irchook::chatgottopic(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *topic = va_arg(ap, char *);
    char *author = va_arg(ap, char *);
    va_end(ap);

    vector<channelInfo>::const_iterator ic = find(irhook.channels.begin(), irhook.channels.end(), room);

    if(ic == irhook.channels.end() || !ic->joined)
	return;

    string text;
    char buf[1024];
    sprintf(buf, _("Channel topic now is: %s"), topic);
    text = buf;

    if(author)
    if(strlen(author)) {
	sprintf(buf, _("set by %s"), author);
	text += (string) "; " + buf + ".";
    }

    em.store(imnotification(imcontact(room, irc), text));
}

void irchook::chatuseropped(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *who = va_arg(ap, char *);
    char *by = va_arg(ap, char *);
    va_end(ap);

    if(by) {
	char buf[512];
	sprintf(buf, _("%s has been opped by %s."), who, by);
	em.store(imnotification(imcontact(room, irc), buf));
    }
}

void irchook::chatuserdeopped(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *who = va_arg(ap, char *);
    char *by = va_arg(ap, char *);
    va_end(ap);

    if(by) {
	char buf[512];
	sprintf(buf, _("%s has been deopped by %s."), who, by);
	em.store(imnotification(imcontact(room, irc), buf));
    }
}

void irchook::chatopped(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *by = va_arg(ap, char *);
    va_end(ap);

    char buf[512];
    if(by) sprintf(buf, _("%s has opped us."), by);
	else strcpy(buf, _("you are an op here"));

    em.store(imnotification(imcontact(room, irc), buf));
}

void irchook::chatdeopped(void *conn, void *cli, ...) {
    va_list ap;

    va_start(ap, cli);
    char *room = va_arg(ap, char *);
    char *by = va_arg(ap, char *);
    va_end(ap);

    char buf[512];
    sprintf(buf, _("%s has deopped us."), by);
    em.store(imnotification(imcontact(room, irc), buf));
}

// ----------------------------------------------------------------------------

bool irchook::channelInfo::operator != (const string &aname) const {
    return up(aname) != up(name);
}

bool irchook::channelInfo::operator == (const string &aname) const {
    return up(aname) == up(name);
}

#endif
