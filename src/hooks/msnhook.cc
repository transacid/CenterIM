/*
*
* centericq MSN protocol handling class
* $Id: msnhook.cc,v 1.91 2005/02/03 02:02:37 konst Exp $
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

#include "icqcommon.h"

#ifdef BUILD_MSN

#include "msnhook.h"
#include "icqconf.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "accountmanager.h"
#include "eventmanager.h"
#include "imlogger.h"
#include "connwrap.h"
#include "icqgroups.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PERIOD_PING     60

msnhook mhook;

static string nicknormalize(const string &nick) {
    if(nick.find("@") == -1) return nick + "@hotmail.com";
    return nick;
}

static string nicktodisp(const string &nick) {
    int pos;
    string r = nick;

    if((pos = r.find("@")) != -1)
    if(r.substr(pos+1) == "hotmail.com")
	r.erase(pos);

    return r;
}

static map<MSN::BuddyStatus, imstatus> convstat;

static imstatus buddy2stat(MSN::BuddyStatus bst) {
    map<MSN::BuddyStatus, imstatus>::const_iterator i = convstat.find(bst);
    if(i != convstat.end())
	return i->second;

    return offline;
}

static MSN::BuddyStatus stat2buddy(imstatus st) {
    map<MSN::BuddyStatus, imstatus>::const_iterator i = convstat.begin();
    while(i != convstat.end()) {
	if(st == i->second)
	    return i->first;
	++i;
    }

    return MSN::STATUS_AVAILABLE;
}

// ----------------------------------------------------------------------------

msnhook::msnhook(): abstracthook(msn), conn() {
    ourstatus = offline;
    fonline = false;
    destroying = false;

    fcapabs.insert(hookcapab::changedetails);
    fcapabs.insert(hookcapab::directadd);
}

msnhook::~msnhook() {
    if(conn.connectionState() != MSN::NotificationServerConnection::NS_DISCONNECTED) {
	destroying = true;
	conn.disconnect();
    }
}

void msnhook::init() {
    manualstatus = conf.getstatus(msn);

    convstat[MSN::STATUS_AVAILABLE] = available;
    convstat[MSN::STATUS_INVISIBLE] = invisible;
    convstat[MSN::STATUS_BUSY] = dontdisturb;
    convstat[MSN::STATUS_ONTHEPHONE] = occupied;
    convstat[MSN::STATUS_AWAY] = away;
    convstat[MSN::STATUS_BERIGHTBACK] = away;
    convstat[MSN::STATUS_OUTTOLUNCH] = notavail;
    convstat[MSN::STATUS_IDLE] = away;
}

void msnhook::connect() {
    icqconf::imaccount account = conf.getourid(msn);

    log(logConnecting);

    readinfo = flogged = false;
    fonline = true;

    try {
	if(conn.connectionState() != MSN::NotificationServerConnection::NS_DISCONNECTED)
	    conn.disconnect();

	rfds.clear();
	wfds.clear();

	conn.connect(account.server, account.port, nicknormalize(account.nickname), account.password);
    } catch(...) {
    }
}

void msnhook::disconnect() {
    fonline = false;
    if(conn.connectionState() != MSN::NotificationServerConnection::NS_DISCONNECTED)
	conn.disconnect();
}

void msnhook::exectimers() {
    if(logged()) {
	if(timer_current-timer_ping > PERIOD_PING) {
	    try {
		conn.sendPing();
		timer_ping = timer_current;
	    } catch(...) {
	    }
	}
    }
}

void msnhook::main() {
    vector<int>::const_iterator i;
    fd_set rs, ws, es;
    struct timeval tv;
    int hsock = 0;
    MSN::Connection *c;

    FD_ZERO(&rs);
    FD_ZERO(&ws);
    FD_ZERO(&es);

    getsockets(rs, ws, es, hsock);
    tv.tv_sec = tv.tv_usec = 0;

    try {
	if(select(hsock+1, &rs, &ws, &es, &tv) > 0) {
	    for(i = rfds.begin(); i != rfds.end(); ++i)
		if(FD_ISSET(*i, &rs)) {
		    c = conn.connectionWithSocket(*i);
		    if(c) c->dataArrivedOnSocket();
		    return;
		}

	    for(i = wfds.begin(); i != wfds.end(); ++i)
		if(FD_ISSET(*i, &ws)) {
		    c = conn.connectionWithSocket(*i);

		    if(c) {
			if(!c->isConnected()) {
			    c->socketConnectionCompleted();
			} else {
			    c->socketIsWritable();
			}
		    }

		    return;
		}
	}
    } catch(...) {
    }
}

void msnhook::getsockets(fd_set &rf, fd_set &wf, fd_set &efds, int &hsocket) const {
    vector<int>::const_iterator i;

    for(i = rfds.begin(); i != rfds.end(); ++i) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &rf);
    }

    for(i = wfds.begin(); i != wfds.end(); ++i) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &wf);
    }
}

bool msnhook::isoursocket(fd_set &rf, fd_set &wf, fd_set &efds) const {
    vector<int>::const_iterator i;

    for(i = rfds.begin(); i != rfds.end(); ++i)
	if(FD_ISSET(*i, &rf))
	    return true;

    for(i = wfds.begin(); i != wfds.end(); ++i)
	if(FD_ISSET(*i, &wf))
	    return true;

    return false;
}

bool msnhook::online() const {
    return fonline;
}

bool msnhook::logged() const {
    return fonline && flogged;
}

bool msnhook::isconnecting() const {
    return fonline && !flogged;
}

bool msnhook::enabled() const {
    return true;
}

bool msnhook::send(const imevent &ev) {
    string text;
    string rcpt = nicknormalize(ev.getcontact().nickname);

    if(ev.gettype() == imevent::message) {
	const immessage *m = static_cast<const immessage *>(&ev);
	if(m) text = m->gettext();

    } else if(ev.gettype() == imevent::url) {
	const imurl *m = static_cast<const imurl *>(&ev);
	if(m) text = m->geturl() + "\n\n" + m->getdescription();

    } else if(ev.gettype() == imevent::file) {
	const imfile *m = static_cast<const imfile *>(&ev);
	vector<imfile::record> files = m->getfiles();
	vector<imfile::record>::const_iterator ir;

	for(ir = files.begin(); ir != files.end(); ++ir) {
	    imfile::record r;

	    r.fname = ir->fname;
	    r.size = ir->size;

	    imfile fr(ev.getcontact(), imevent::outgoing, "", vector<imfile::record>(1, r));

	    try {
		qevent *ctx = new qevent(qevent::qeFile, rcpt, ir->fname);

		if(lconn.find(rcpt) != lconn.end()) sendmsn(lconn[rcpt], ctx);
		    else conn.requestSwitchboardConnection(ctx);

	    } catch(...) {
	    }
	}

	return true;
    }

    icqcontact *c = clist.get(ev.getcontact());
    text = rusconv("ku", text);

    if(c)
    if(c->getstatus() != offline || !c->inlist()) {
	try {
	    qevent *ctx = new qevent(qevent::qeMsg, rcpt, text);

	    if(lconn.find(rcpt) != lconn.end()) sendmsn(lconn[rcpt], ctx);
		else conn.requestSwitchboardConnection(ctx);

	} catch(...) {
	}

	return true;
    }

    return false;
}

int msnhook::findgroup(const imcontact &ic, string &gname) const {
    int gid = -1;
    icqcontact *c;

    if(c = clist.get(ic)) {
	vector<icqgroup>::const_iterator ig = find(groups.begin(), groups.end(), c->getgroupid());
	if(ig != groups.end()) {
	    gname = ig->getname();
	    map<int, string>::const_iterator im = mgroups.begin();
	    while(im != mgroups.end() && gid == -1) {
		if(im->second == ig->getname())
		    gid = im->first;
		++im;
	    }
	}
    }

    return gid;
}

void msnhook::sendnewuser(const imcontact &ic) {
    if(logged() && !readinfo) {
	icqcontact *c;
	imcontact icc(nicktodisp(ic.nickname), msn);

	if(icc.nickname != ic.nickname)
	if(c = clist.get(ic)) {
	    c->setdesc(icc);
	    c->setnick(icc.nickname);
	    c->setdispnick(icc.nickname);
	}

	int gid;
	string gname;

	gid = findgroup(ic, gname);

	try {
	    if(gid != -1) {
		conn.addToGroup(nicknormalize(ic.nickname), gid);
	    } else {
		conn.addGroup(gname);
	    }
	} catch(...) {
	}
    }

    requestinfo(ic);
}

void msnhook::setautostatus(imstatus st) {
    if(st != offline) {
	if(!logged()) {
	    connect();
	} else {
	    logger.putourstatus(msn, ourstatus, st);
	    try {
		conn.setState(stat2buddy(ourstatus = st));
	    } catch(...) {
	    }
	}
    } else {
	if(getstatus() != offline) {
	    disconnect();
	}
    }
}

imstatus msnhook::getstatus() const {
    return online() ? ourstatus : offline;
}

void msnhook::removeuser(const imcontact &ic) {
    removeuser(ic, true);
}

void msnhook::removeuser(const imcontact &ic, bool report) {
    int i;
    bool found;
    vector<msnbuddy>::const_iterator ib = find(slst["FL"].begin(), slst["FL"].end(), nicknormalize(ic.nickname));

    if(online() && ib != slst["FL"].end()) {
	if(report)
	    log(logContactRemove, ic.nickname.c_str());

	try {
	    conn.removeFromGroup(nicknormalize(ic.nickname), ib->gid);

	    for(i = 0, found = false; i < clist.count && !found; i++) {
		icqcontact *c = (icqcontact *) clist.at(i);
		found = c->getdesc().pname == msn
		    && groups.getname(c->getgroupid()) == mgroups[ib->gid];
	    }

	    if(!found)
		conn.removeGroup(ib->gid);
	} catch(...) {
	}
    }
}

void msnhook::requestinfo(const imcontact &ic) {
    icqcontact *c = clist.get(ic);
    if(!c) c = clist.get(contactroot);

    icqcontact::moreinfo m = c->getmoreinfo();
    icqcontact::basicinfo b = c->getbasicinfo();

    b.email = nicknormalize(ic.nickname);
    m.homepage = "http://members.msn.com/" + b.email;

    if(ic.nickname == conf.getourid(msn).nickname)
	c->setnick(friendlynicks[ic.nickname]);

    c->setmoreinfo(m);
    c->setbasicinfo(b);
}

void msnhook::lookup(const imsearchparams &params, verticalmenu &dest) {
    if(params.reverse) {
	vector<msnbuddy>::const_iterator i = slst["RL"].begin();

	while(i != slst["RL"].end()) {
	    icqcontact *c = new icqcontact(imcontact(nicktodisp(i->nick), msn));
	    c->setnick(i->friendly);

	    dest.additem(conf.getcolor(cp_clist_msn), c, (string) " " + i->nick);
	    ++i;
	}
	face.findready();

	face.log(_("+ [msn] reverse users listing finished, %d found"),
	    slst["RL"].size());

	dest.redraw();
    }
}

vector<icqcontact *> msnhook::getneedsync() {
    int i;
    vector<icqcontact *> r;
    bool found;

    for(i = 0; i < clist.count; i++) {
	icqcontact *c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == msn) {
	    vector<msnbuddy>::const_iterator fi = slst["FL"].begin();

	    for(found = false; fi != slst["FL"].end() && !found; ++fi)
		found = c->getdesc().nickname == fi->nick;

	    if(!found)
		r.push_back(c);
	}
    }

    return r;
}

void msnhook::sendupdateuserinfo(const icqcontact &c) {
    try {
	conn.setFriendlyName(c.getnick());
    } catch(...) {
    }
}

void msnhook::checkfriendly(icqcontact *c, const string friendlynick, bool forcefetch) {
    string oldnick = c->getnick();
    string newnick = unmime(friendlynick);

    c->setnick(newnick);

    if(forcefetch || (oldnick != newnick && c->getdispnick() == oldnick) || oldnick.empty()) {
	c->setdispnick(newnick);
	face.relaxedupdate();
    }
}

void msnhook::checkinlist(imcontact ic) {
    icqcontact *c = clist.get(ic);
    vector<icqcontact *> notremote = getneedsync();

    if(c)
    if(c->inlist())
    if(find(notremote.begin(), notremote.end(), c) != notremote.end())
	mhook.sendnewuser(ic);
}

bool msnhook::knowntransfer(const imfile &fr) const {
    return transferinfo.find(fr) != transferinfo.end();
}

void msnhook::replytransfer(const imfile &fr, bool accept, const string &localpath) {
    if(accept) {
	transferinfo[fr].second = localpath;

	if(transferinfo[fr].second.substr(transferinfo[fr].second.size()-1) != "/")
	    transferinfo[fr].second += "/";

	transferinfo[fr].second += justfname(fr.getfiles().begin()->fname);
//        msn_filetrans_accept(transferinfo[fr].first, transferinfo[fr].second.c_str());

    } else {
//        msn_filetrans_reject(transferinfo[fr].first);
	transferinfo.erase(fr);

    }
}

void msnhook::aborttransfer(const imfile &fr) {
//    msn_filetrans_reject(transferinfo[fr].first);

    face.transferupdate(fr.getfiles().begin()->fname, fr,
	icqface::tsCancel, 0, 0);

    transferinfo.erase(fr);
}

bool msnhook::getfevent(MSN::FileTransferInvitation *fhandle, imfile &fr) {
    map<imfile, pair<MSN::FileTransferInvitation *, string> >::const_iterator i = transferinfo.begin();

    while(i != transferinfo.end()) {
	if(i->second.first == fhandle) {
	    fr = i->first;
	    return true;
	}
	++i;
    }

    return false;
}

void msnhook::updatecontact(icqcontact *c) {
    string gname, nick = nicknormalize(c->getdesc().nickname);
    vector<msnbuddy>::const_iterator ib = find(slst["FL"].begin(), slst["FL"].end(), nick);

    if(ib != slst["FL"].end() && logged() && conf.getgroupmode() != icqconf::nogroups)
    if(mhook.findgroup(c->getdesc(), gname) != ib->gid) {
	try {
	    conn.removeFromList("FL", nick.c_str());
	} catch(...) {
	}

	sendnewuser(c->getdesc());
    }
}

void msnhook::renamegroup(const string &oldname, const string &newname) {
    if(logged()) {
	map<int, string>::const_iterator im = mgroups.begin();
	while(im != mgroups.end()) {
	    if(im->second == oldname) {
		try {
		    conn.renameGroup(im->first, newname);
		} catch(...) {
		}

		break;
	    }
	    ++im;
	}
    }
}

// ----------------------------------------------------------------------------

void msnhook::statusupdate(string buddy, string friendlyname, imstatus status) {
    imcontact ic(nicktodisp(buddy), msn);
    icqcontact *c = clist.get(ic);
    bool forcefetch;

    if(forcefetch = !c)
	c = clist.addnew(ic, false);

    if(!friendlyname.empty())
	checkfriendly(c, friendlyname, forcefetch);

    logger.putonline(ic, c->getstatus(), status);
    c->setstatus(status);
}

// ----------------------------------------------------------------------------

void msnhook::sendmsn(MSN::SwitchboardServerConnection *conn, const qevent *ctx) {
    MSN::FileTransferInvitation *inv;

    switch(ctx->type) {
	case msnhook::qevent::qeMsg:
	    conn->sendMessage(ctx->text);
	    break;

	case msnhook::qevent::qeFile:
	    inv = conn->sendFile(ctx->text);
//                if(inv) mhook.transferinfo[] = inv;
	    break;
    }

    delete ctx;
}

// ----------------------------------------------------------------------------

static void log(const string &s) {
    if(conf.getdebug())
	face.log(s);
}

void MSN::ext::log(int writing, const char* buf) {
    string pref = writing ? "OUT" : "IN";
    ::log((string) "[" + pref + "] " + buf);
}

void MSN::ext::registerSocket(int s, int reading, int writing) {
    ::log("MSN::ext::registerSocket");
    if(reading) mhook.rfds.push_back(s);
    if(writing) mhook.wfds.push_back(s);
}

void MSN::ext::unregisterSocket(int s) {
    ::log("MSN::ext::unregisterSocket");
    vector<int>::iterator i;

    i = find(mhook.rfds.begin(), mhook.rfds.end(), s);
    if(i != mhook.rfds.end()) mhook.rfds.erase(i);

    i = find(mhook.wfds.begin(), mhook.wfds.end(), s);
    if(i != mhook.wfds.end()) mhook.wfds.erase(i);
}

void MSN::ext::gotFriendlyName(MSN::Connection * conn, string friendlyname) {
    ::log("MSN::ext::gotFriendlyName");
    if(!friendlyname.empty())
	mhook.friendlynicks[conf.getourid(msn).nickname] = friendlyname;
}

void MSN::ext::gotBuddyListInfo(MSN::NotificationServerConnection * conn, MSN::ListSyncInfo * data) {
    ::log("MSN::ext::gotBuddyListInfo");
    imcontact ic;
    bool found;

    mhook.readinfo = true;

    std::map<int, MSN::Group>::iterator ig;

    for(ig = data->groups.begin(); ig != data->groups.end(); ig++) {
	mhook.mgroups[ig->second.groupID] = ig->second.name;
    }

    std::list<MSN::Buddy> &lst = data->forwardList;
    std::list<MSN::Buddy>::iterator i;

    for(i = lst.begin(); i != lst.end(); i++) {
	int gid = 0;
	if(!i->groups.empty()) gid = (*i->groups.begin())->groupID;
	mhook.slst["FL"].push_back(msnbuddy(i->userName, i->friendlyName, gid));

	ic = imcontact(nicktodisp(i->userName), msn);
	icqcontact *c = clist.get(ic);
	if(!c) c = clist.addnew(ic, false);

	icqcontact::basicinfo bi = c->getbasicinfo();
	icqcontact::workinfo wi = c->getworkinfo();

	list<MSN::Buddy::PhoneNumber>::iterator ip = i->phoneNumbers.begin();
	for(; ip != i->phoneNumbers.end(); ip++) {
	    if(ip->title == "PHH") bi.phone = ip->number; else
	    if(ip->title == "PHW") wi.phone = ip->number; else
	    if(ip->title == "PHM") bi.cellular = ip->number;
	}

	c->setbasicinfo(bi);
	c->setworkinfo(wi);

	for(found = false, ig = data->groups.begin(); ig != data->groups.end() && !found; ++ig) {
	    found = ig->second.groupID == gid;
	    if(found) clist.updateEntry(ic, ig->second.name);
	}
    }

    for(lst = data->reverseList, i = lst.begin(); i != lst.end(); ++i)
	mhook.slst["RL"].push_back(msnbuddy(i->userName, i->friendlyName));

    mhook.readinfo = false;
    mhook.flogged = true;

    mhook.setautostatus(mhook.manualstatus);
    mhook.timer_ping = timer_current;
    mhook.log(abstracthook::logLogged);
    face.update();
}

void MSN::ext::gotLatestListSerial(MSN::Connection * conn, int serial) {
    ::log("MSN::ext::gotLatestListSerial");
}

void MSN::ext::gotGTC(MSN::Connection * conn, char c) {
    ::log("MSN::ext::gotGTC");
}

void MSN::ext::gotBLP(MSN::Connection * conn, char c) {
    ::log("MSN::ext::gotBLP");
}

void MSN::ext::gotNewReverseListEntry(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname) {
    ::log("MSN::ext::gotNewReverseListEntry");

    try {
	mhook.conn.addToList("AL", buddy);
    } catch(...) {
    }

    imcontact ic(nicktodisp(buddy), msn);
    mhook.checkinlist(ic);
    em.store(imnotification(ic, _("The user has added you to his/her contact list")));
}

void MSN::ext::addedListEntry(MSN::Connection * conn, string lst, MSN::Passport buddy, int groupID) {
    ::log("MSN::ext::addedListEntry");
    mhook.slst[lst].push_back(msnbuddy(buddy, "", groupID));
}

void MSN::ext::removedListEntry(MSN::Connection * conn, string lst, MSN::Passport buddy, int groupID) {
    ::log("MSN::ext::removedListEntry");
    vector<msnbuddy>::iterator i = mhook.slst[lst].begin();
    while(i != mhook.slst[lst].end()) {
	if(i->nick == buddy) {
	    mhook.slst[lst].erase(i);
	    i = mhook.slst[lst].begin();
	} else {
	    ++i;
	}
    }
}

void MSN::ext::showError(MSN::Connection * conn, string msg) {
    ::log(msg);
}

void MSN::ext::buddyChangedStatus(MSN::Connection * conn, MSN::Passport buddy, string friendlyname, MSN::BuddyStatus state) {
    ::log("MSN::ext::buddyChangedStatus");
    mhook.statusupdate(buddy, friendlyname, buddy2stat(state));
}

void MSN::ext::buddyOffline(MSN::Connection * conn, MSN::Passport buddy) {
    ::log("MSN::ext::buddyOffline");
    mhook.statusupdate(buddy, "", offline);
}

void MSN::ext::gotSwitchboard(MSN::SwitchboardServerConnection * conn, const void * tag) {
    ::log("MSN::ext::gotSwitchboard");

    if(tag) {
	const msnhook::qevent *ctx = static_cast<const msnhook::qevent *>(tag);
	conn->inviteUser(ctx->nick);
    }
}

void MSN::ext::buddyJoinedConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, int is_initial) {
    ::log("MSN::ext::buddyJoinedConversation");
    if(conn->auth.tag) {
	const msnhook::qevent *ctx = static_cast<const msnhook::qevent *>(conn->auth.tag);
	mhook.lconn[ctx->nick] = conn;
	mhook.sendmsn(conn, ctx);
	conn->auth.tag = 0;
    }
}

void MSN::ext::buddyLeftConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy) {
    ::log("MSN::ext::buddyLeftConversation");
}

void MSN::ext::gotInstantMessage(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, MSN::Message * msg) {
    ::log("MSN::ext::gotInstantMessage");
    imcontact ic(nicktodisp(buddy), msn);

    mhook.checkinlist(ic);

    string text = mhook.rusconv("uk", msg->getBody());
    em.store(immessage(ic, imevent::incoming, text));
}

void MSN::ext::failedSendingMessage(MSN::Connection * conn) {
    ::log("MSN::ext::failedSendingMessage");
}

void MSN::ext::buddyTyping(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname) {
    ::log("MSN::ext::buddyTyping");
    icqcontact *c = clist.get(imcontact(nicktodisp(buddy), msn));
    if(c) c->setlasttyping(timer_current);
}

void MSN::ext::gotInitialEmailNotification(MSN::Connection * conn, int unread_inbox, int unread_folders) {
    ::log("MSN::ext::gotInitialEmailNotification");
    face.log(_("+ [msn] unread e-mail: %d in inbox, %d in folders"),
	unread_inbox, unread_folders);
}

void MSN::ext::gotNewEmailNotification(MSN::Connection * conn, string from, string subject) {
    ::log("MSN::ext::gotNewEmailNotification");
    face.log(_("+ [msn] e-mail from %s, %s"), from.c_str(), subject.c_str());
    clist.get(contactroot)->playsound(imevent::email);
}

void MSN::ext::gotFileTransferInvitation(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname, MSN::FileTransferInvitation * inv) {
    ::log("MSN::ext::gotFileTransferInvitation");
    if(!mhook.fcapabs.count(hookcapab::files))
	return;

    imfile::record r;
    r.fname = inv->fileName;
    r.size = inv->fileSize;

    imcontact ic(nicktodisp(buddy), msn);
    mhook.checkinlist(ic);

    imfile fr(ic, imevent::incoming, "", vector<imfile::record>(1, r));

    mhook.transferinfo[fr].first = inv;
    em.store(fr);

    face.transferupdate(inv->fileName, fr, icqface::tsInit, inv->fileSize, 0);
}

void MSN::ext::fileTransferProgress(MSN::FileTransferInvitation * inv, string status, unsigned long recv, unsigned long total) {
    ::log("MSN::ext::fileTransferProgress");
    imfile fr;

    if(mhook.getfevent(inv, fr)) {
	face.transferupdate(fr.getfiles().begin()->fname, fr,
	    icqface::tsProgress, total, recv);
    }
}

void MSN::ext::fileTransferFailed(MSN::FileTransferInvitation * inv, int error, string message) {
    ::log("MSN::ext::fileTransferFailed");
    imfile fr;

    if(mhook.getfevent(inv, fr)) {
	face.transferupdate(fr.getfiles().begin()->fname, fr, icqface::tsError, 0, 0);
	mhook.transferinfo.erase(fr);
    }
}

void MSN::ext::fileTransferSucceeded(MSN::FileTransferInvitation * inv) {
    ::log("MSN::ext::fileTransferSucceeded");
    imfile fr;

    if(mhook.getfevent(inv, fr)) {
	face.transferupdate(fr.getfiles().begin()->fname, fr, icqface::tsFinish, 0, 0);
	mhook.transferinfo.erase(fr);
    }
}

void MSN::ext::gotNewConnection(MSN::Connection * conn) {
    ::log("MSN::ext::gotNewConnection");
    if(dynamic_cast<MSN::NotificationServerConnection *>(conn))
	dynamic_cast<MSN::NotificationServerConnection *>(conn)->synchronizeLists();
}

void MSN::ext::closingConnection(MSN::Connection * conn) {
    ::log("MSN::ext::closingConnection");

    MSN::SwitchboardServerConnection *swc;

    if(swc = dynamic_cast<MSN::SwitchboardServerConnection *>(conn)) {
	map<string, MSN::SwitchboardServerConnection *>::const_iterator ic = mhook.lconn.begin();
	while(ic != mhook.lconn.end()) {
	    if(swc == ic->second) {
		mhook.lconn.erase(ic->first);
		break;
	    }
	    ++ic;
	}

    } else if(conn == &mhook.conn) {
	if(!mhook.destroying) {
	    mhook.rfds.clear();
	    mhook.wfds.clear();
	    mhook.lconn.clear();
	    mhook.slst.clear();

	    if(mhook.logged()) {
		logger.putourstatus(msn, mhook.getstatus(), mhook.ourstatus = offline);
		clist.setoffline(msn);

		mhook.fonline = false;
		mhook.log(abstracthook::logDisconnected);

		face.update();
	    }
	}
    }

    unregisterSocket(conn->sock);
}

void MSN::ext::changedStatus(MSN::Connection * conn, MSN::BuddyStatus state) {
    ::log("MSN::ext::changedStatus");
}

int MSN::ext::connectToServer(string server, int port, bool *connected) {
    ::log("MSN::ext::connectToServer");
    struct sockaddr_in sa;
    struct hostent *hp;
    int a, s;
    string msgerr = _("+ [msn] cannot connect: ");

    hp = gethostbyname(server.c_str());
    if(!hp) {
	face.log(msgerr + _("could not resolve hostname"));
	errno = ECONNREFUSED;
	return -1;
    }

    memset(&sa, 0, sizeof(sa));
    memcpy((char *) &sa.sin_addr, hp->h_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short) port);

    if((s = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0)
	return -1;

    int oldfdArgs = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, oldfdArgs | O_NONBLOCK);

    *connected = false;

    if(cw_connect(s, (struct sockaddr *) &sa, sizeof sa, 0) < 0) {
	if(errno != EINPROGRESS) { 
	    face.log(msgerr + _("verify the hostname and port"));
	    close(s);
	    return -1;
	}
    }

    return s;
}

int MSN::ext::listenOnPort(int port) {
    ::log("MSN::ext::listenOnPort");
    int s;
    struct sockaddr_in addr;

    if((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if(bind(s, (sockaddr *) &addr, sizeof(addr)) < 0 || listen(s, 1) < 0) {
	close(s);
	return -1;
    }

    return s;
}

string MSN::ext::getOurIP() {
    ::log("MSN::ext::getOurIP");
    struct hostent *hn;
    char buf2[1024];

    gethostname(buf2, 1024);
    hn = gethostbyname(buf2);

    return inet_ntoa(*((struct in_addr*) hn->h_addr));
}

string MSN::ext::getSecureHTTPProxy() {
    ::log("MSN::ext::getSecureHTTPProxy");
    return "";
}

void MSN::ext::addedGroup(MSN::Connection * conn, string groupName, int groupID) {
    ::log("MSN::ext::addedGroup");
    int i;
    icqcontact *c;

    mhook.mgroups[groupID] = groupName;

    vector<icqgroup>::const_iterator ig = groups.begin();

    while(ig != groups.end()) {
	if(ig->getname() == groupName) {
	    for(i = 0; i < clist.count; i++) {
		c = (icqcontact *) clist.at(i);
		if(c->getgroupid() == ig->getid())
		    mhook.sendnewuser(c->getdesc());
	    }
	    break;
	}
	++ig;
    }
}

void MSN::ext::renamedGroup(MSN::Connection * conn, int groupID, string newGroupName) {
    ::log("MSN::ext::renamedGroup");
    mhook.mgroups[groupID] = newGroupName;
}

void MSN::ext::removedGroup(MSN::Connection * conn, int groupID) {
    ::log("MSN::ext::removedGroup");
    mhook.mgroups.erase(groupID);
}

#endif
