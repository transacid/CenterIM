/*
*
* centericq core routines
* $Id: centericq.cc,v 1.34 2001/11/16 14:00:17 konst Exp $
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

#include "centericq.h"
#include "icqconf.h"
#include "icqhist.h"
#include "icqhook.h"
#include "yahoohook.h"
#include "icqface.h"
#include "icqoffline.h"
#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqmlist.h"
#include "icqgroups.h"
#include "accountmanager.h"

centericq::centericq() {
    timer_keypress = time(0);
    timer_checkmail = 0;
    regmode = false;
}

centericq::~centericq() {
}

void centericq::commandline(int argc, char **argv) {
    int i;

    for(i = 1; i < argc; i++) {
	string args = argv[i];
	if((args == "-a") || (args == "--ascii")) {
	    kintf_graph = 0;
	} else {
	    kendinterface();
	    cout << "centericq command line parameters:" << endl << endl;
	    cout << "\t--ascii or -a : use characters for windows and UI controls" << endl;
	    cout << "\tanything else : display this stuff" << endl;
	    exit(0);
	}
    }
}

void centericq::exec() {
    struct sigaction sact;

    memset(&sact, 0, sizeof(sact));
    sact.sa_handler = &handlesignal;
    sigaction(SIGINT, &sact, 0);
    sigaction(SIGCHLD, &sact, 0);
    sigaction(SIGALRM, &sact, 0);

    conf.initpairs();
    conf.load();
    face.init();

    if(regmode = !conf.getouridcount()) {
	char *p = getenv("LANG");

	if(p)
	if(((string) p).substr(0, 2) == "ru")
	    icq_Russian = yahoo_Russian = true;

	if(updateconf()) {
	    manager.exec();
	}

	regmode = false;
    }

    if(conf.getouridcount()) {
	conf.checkdir();
	conf.load();
	groups.load();
	clist.load();
	lst.load();
	conf.loadsounds();

	face.done();
	face.init();

	face.draw();
	checkparallel();
	mainloop();

	lst.save();
	clist.save();
	groups.save();
    }

    face.done();
    conf.save();
}

void centericq::mainloop() {
    bool finished = false;
    string text, url;
    int action, old, gid;
    icqcontact *c;
    char buf[512];
    vector<imcontact>::iterator i;

    face.draw();

    while(!finished) {
	face.status(_("F2/M current contact menu, F3/S change icq status, F4/G general actions, Q quit"));
	c = face.mainloop(action);

	switch(action) {
	    case ACT_IGNORELIST:
		face.modelist(csignore);
		break;
	    case ACT_VISIBLELIST:
		face.modelist(csvisible);
		break;
	    case ACT_INVISLIST:
		face.modelist(csinvisible);
		break;
	    case ACT_STATUS:
		changestatus();
		break;
	    case ACT_ORG_GROUPS:
		face.organizegroups();
		break;
	    case ACT_HIDEOFFLINE:
		conf.sethideoffline(!conf.gethideoffline());
		face.update();
		break;
	    case ACT_DETAILS:
		manager.exec();
		break;
	    case ACT_FIND:
		find();
		break;
	    case ACT_CONF:
		updateconf();
		break;
	    case ACT_NONICQ:
		nonicq(0);
		break;
	    case ACT_QUICKFIND:
		face.quickfind();
		break;
	    case ACT_QUIT:
		finished = true;
		break;
	}

	if(!c) continue;

	switch(action) {
	    case ACT_URL:
		url = text = "";
		if(face.editurl(c->getdesc(), url, text))
		    for(i = face.muins.begin(); i != face.muins.end(); i++)
			offl.sendurl(*i, url, text);
		break;
	    case ACT_IGNORE:
		sprintf(buf, _("Ignore all events from %s?"), c->getdesc().totext().c_str());
		if(face.ask(buf, ASK_YES | ASK_NO, ASK_NO) == ASK_YES) {
		    lst.push_back(modelistitem(c->getdispnick(), c->getdesc(), csignore));
		    clist.remove(c->getdesc());
		    face.update();
		}
		break;
	    case ACT_FILE:
		if(c->getstatus() != offline) {
		    sendfiles(c->getdesc());
		}
		break;
	    case ACT_CONTACT:
		sendcontacts(c->getdesc());
		break;
	    case ACT_EDITUSER:
		nonicq(c->getdesc().uin);
		break;
	    case ACT_ADD:
		if(!c->inlist()) {
		    addcontact(c->getdesc());
		}
		break;
	    case ACT_INFO:
		if(c->getdesc() != contactroot)
		    userinfo(c->getdesc());
		break;
	    case ACT_REMOVE:
		sprintf(buf, _("Are you sure want to remove %s?"), c->getdesc().totext().c_str());
		if(face.ask(buf, ASK_YES | ASK_NO, ASK_NO) == ASK_YES) {
		    clist.remove(c->getdesc());
		    face.update();
		}
		break;
	    case ACT_RENAME:
		text = face.inputstr(_("New nickname to show: "), c->getdispnick());
		if(text.size()) {
		    c->setdispnick(text);
		    face.update();
		}
		break;
	    case ACT_GROUPMOVE:
		if(gid = face.selectgroup(_("Select a group to move the user to"))) {
		    c->setgroupid(gid);
		    face.fillcontactlist();
		}
		break;
	    case ACT_HISTORY:
		if(c->getdesc().pname != infocard)
		    face.history(c->getdesc());
		break;
	    case ACT_MSG:
		text = "";
		if(c->getmsgcount()) {
		    face.read(c->getdesc());
		} else if(c->getdesc() != contactroot && c->getdesc().pname != infocard) {
		    message(c->getdesc(), text, scratch);
		}
		break;
	}
    }
}

void centericq::changestatus() {
    imstatus st;
    protocolname pname;

    if(face.changestatus(pname, st)) {
	switch(pname) {
	    case icq: ihook.setstatus(st); break;
	    case yahoo: yhook.setstatus(st); break;
	}

	face.update();
    }
}

void centericq::find() {
    static icqface::searchparameters s;
    bool ret = true;

    while(ret) {
	if(ret = face.finddialog(s)) {
	    switch(s.pname) {
		case icq:
		    if(s.uin) {
			icq_SendSearchUINReq(&icql, s.uin);
		    } else
		    if(s.minage || s.maxage || s.country || s.language
		    || !s.city.empty() || !s.state.empty() || !s.company.empty()
		    || !s.department.empty() || !s.position.empty() || s.onlineonly) {
			icq_SendWhitePagesSearchReq(&icql, s.firstname.c_str(),
			    s.lastname.c_str(), s.nick.c_str(), s.email.c_str(),
			    s.minage, s.maxage, s.gender, s.language, s.city.c_str(),
			    s.state.c_str(), s.country, s.company.c_str(),
			    s.department.c_str(), s.position.c_str(),
			    s.onlineonly ? 1 : 0);

		    } else {
			icq_SendSearchReq(&icql, s.email.c_str(),
			    s.nick.c_str(), s.firstname.c_str(),
			    s.lastname.c_str());
		    }

		    ret = face.findresults();
		    break;

		case yahoo:
		    if(!s.nick.empty()) {
			addcontact(imcontact(s.nick, yahoo));
			ret = false;
		    }
		    break;
	    }
	}
    }
}

void centericq::userinfo(const imcontact cinfo) {
    imcontact realuin = cinfo;
    icqcontact *c = clist.get(cinfo);

    if(!c) {
	c = clist.get(contactroot);
	realuin = contactroot;
	c->clear();

	switch(cinfo.pname) {
	    case icq:
		if(ihook.online()) {
		    c->setseq2(icq_SendMetaInfoReq(&icql, cinfo.uin));
		}
		break;
	    case yahoo:
		break;
	}
    }

    if(c) {
	face.userinfo(cinfo, realuin);
    }
}

void centericq::sendfiles(const imcontact cinfo) {
    int i;
    string msg;
    unsigned long seq;
    icqcontact *c = (icqcontact *) clist.get(cinfo);
    linkedlist flist;

    if(c->getdirect()) {
	if(face.sendfiles(cinfo, msg, flist)) {
	    char **files = 0;

	    for(i = 0; i < flist.count; i++) {
		files = (char **) realloc(files, (i+1)*sizeof(char *));
		files[i] = strdup((char *) flist.at(i));
		files[i+1] = 0;
	    }

	    seq = icq_SendFileRequest(&icql, cinfo.uin, msg.c_str(), files);
	    ihook.addfile(cinfo.uin, seq, files[0], HIST_MSG_OUT);

	    face.log(_("+ sending file %s to %s, %lu"),
		justfname(files[0]).c_str(), c->getdispnick().c_str(), cinfo.uin);
	}
    } else {
	face.log(_("+ remote client doesn't support file transfers"));
    }
}

void centericq::sendcontacts(const imcontact cinfo) {
    icqcontactmsg *m, *n, *cont;
    vector<imcontact>::iterator i;

    if(icql.icq_Status == STATUS_OFFLINE) {
	face.log(_("+ cannot send contacts in offline mode"));
	return;
    }

    face.muins.clear();

    if(face.multicontacts(_("Contacts to send")))
    if(!face.muins.empty()) {
	for(m = 0, i = face.muins.begin(); i != face.muins.end(); i++) {
	    n = new icqcontactmsg;
	    if(m) m->next = n; else cont = n;
	    m = n;

	    m->uin = i->uin;
	    strncpy(m->nick, clist.get(*i)->getnick().c_str(), 12);
	    m->nick[11] = 0;
	    m->next = 0;
	}

	icq_SendContact(&icql, cinfo.uin, cont, ICQ_SEND_THRUSERVER);

	for(m = cont; m; ) {
	    n = (icqcontactmsg *) m->next;
	    delete m;
	    m = n;
	}
    }
}

bool centericq::updateconf() {
    bool r;

    regsound snd = rsdontchange;
    regcolor clr = rcdontchange;

    if(r = face.updateconf(snd, clr)) {
	if(snd != rsdontchange) {
	    conf.setregsound(snd);
	    unlink(conf.getconfigfname("sounds").c_str());
	    conf.loadsounds();
	}

	if(clr != rcdontchange) {
	    conf.setregcolor(clr);
	    unlink(conf.getconfigfname("colorscheme").c_str());
	    conf.loadcolors();
	    face.done();
	    face.init();
	    face.draw();
	}

	face.update();
    }

    return r;
}

void centericq::nonicq(int id) {
    icqcontact *c;

    if(!id) {
	c = clist.addnew(imcontact(0, infocard), true);

	if(face.updatedetails(c)) {
	    c->save();
	} else {
	    clist.remove(c->getdesc());
	}
    } else {
	c = clist.get(imcontact(id, infocard));
	if(face.updatedetails(c)) {
	    c->setdispnick(c->getnick());
	    c->save();
	} else {
	    c->load();
	}
    }

    face.update();
}

void centericq::checkmail() {
    static int fsize = -1;
    const char *fname = getenv("MAIL");
    struct stat st;
    int mcount = 0;
    char buf[512];
    string lastfrom;
    bool prevempty, header;
    FILE *f;

    if(conf.getmailcheck() && fname)
    if(!stat(fname, &st))
    if(st.st_size != fsize) {

	if(fsize != -1) {
	    if(f = fopen(fname, "r")) {
		prevempty = header = true;

		while(!feof(f)) {
		    freads(f, buf, 512);

		    if(prevempty && !strncmp(buf, "From ", 5)) {
			lastfrom = strim(buf+5);
			mcount++;
			header = true;
		    } else if(header && !strncmp(buf, "From: ", 6)) {
			lastfrom = strim(buf+6);
		    }

		    if(prevempty = !buf[0]) header = false;
		}
		fclose(f);
	    }

	    if(mcount) {
		face.log(_("+ new mail arrived, %d messages"), mcount);
		face.log(_("+ last msg from %s"), lastfrom.c_str());
		clist.get(contactroot)->playsound(EVT_EMAIL);
	    }
	}

	fsize = st.st_size;
    }
}

void centericq::handlesignal(int signum) {
    int status;

    switch(signum) {
	case SIGCHLD:
	    while(wait3(&status, WNOHANG, 0) > 0);
	    break;
	case SIGALRM:
	    break;
    }
}

void centericq::checkparallel() {
    string pidfname = (string) getenv("HOME") + "/.centericq/pid", fname;
    int pid = 0;
    char exename[512];

    ifstream f(pidfname.c_str());
    if(f.is_open()) {
	f >> pid;
	f.close();
    }

    if(!(fname = readlink((string) "/proc/" + i2str(pid) + "/exe")).empty() && (pid != getpid())) {
	if(justfname(fname) == "centericq") {
	    face.log(_("! another running copy of centericq detected"));
	    face.log(_("! this may cause problems. pid %lu"), pid);
	}
    } else {
	pid = 0;
    }

    if(!pid) {
	ofstream of(pidfname.c_str());
	if(of.is_open()) {
	    of << getpid() << endl;
	    of.close();
	}
    }
}

bool centericq::message(const imcontact cinfo, const string text, msgmode mode) {
    icqcontact *c = (icqcontact *) clist.get(cinfo);
    vector<imcontact>::iterator i;
    char buf[512];
    string stext;
    bool ret = true;

    face.muins.clear();

    switch(mode) {
	case reply:
	    face.muins.push_back(cinfo);
	    stext = conf.getquote() ? quotemsg(text) : "";
	    break;
	case forward:
	    sprintf(buf, _("%s (%lu) wrote:"),
		c ? c->getdispnick().c_str() : "I", cinfo.uin);

	    stext = (string) buf + "\n\n" + text;

	    if(ret = face.multicontacts()) {
		ret = !face.muins.empty();
	    }
	    break;
	case scratch:
	    face.muins.push_back(cinfo);
	    stext = text;
	    break;
    }

    if(ret) {
	if(ret = face.editmsg(*face.muins.begin(), stext)) {
	    if(face.muins.empty()) {
		face.muins.push_back(cinfo);
	    }

	    if(stext.size() > MAX_TCPMSG_SIZE)
		stext.resize(MAX_TCPMSG_SIZE);

	    for(i = face.muins.begin(); i != face.muins.end(); i++) {
		offl.sendmsg(*i, stext);
	    }
	}
    }

    return ret;
}

const string centericq::quotemsg(const string text) {
    string ret;
    vector<string> lines;
    vector<string>::iterator i;

    breakintolines(text, lines, WORKAREA_X2-WORKAREA_X1-4);

    for(i = lines.begin(); i != lines.end(); i++) {
	if(!i->empty()) ret += (string) "> " + *i;
	ret = trailcut(ret);
	ret += "\n";
    }

    return ret;
}

icqcontact *centericq::addcontact(const imcontact ic) {
    icqcontact *c;
    int groupid = 0;
    bool notify;

    if(c = clist.get(ic)) {
	if(c->inlist()) {
	    face.log(_("+ user %s is already on the list"), ic.totext().c_str());
	    return 0;
	}
    }

    if(conf.getusegroups()) {
	groupid = face.selectgroup(_("Select a group to add the user to"));
	if(!groupid) return 0;
    }

    switch(ic.pname) {
	case icq:
	    notify = ihook.online();
	    break;
	case yahoo:
	default:
	    notify = false;
	    break;
    }

    if(notify) {
	if(face.ask(_("Notify the user he/she has been added?"),
	ASK_YES | ASK_NO, ASK_YES) == ASK_YES) {
	    icq_AlertAddUser(&icql, ic.uin);
	    face.log(_("+ the notification has been sent to %lu"), ic.uin);
	}
    }

    if(!c) {
	c = clist.addnew(ic, false);
    } else {
	c->includeintolist();
    }

    c->setgroupid(groupid);
    face.log(_("+ %s has been added to the list"), ic.totext().c_str());
    face.update();

    return c;
}

bool centericq::idle(int options = 0) {
    bool keypressed;
    fd_set fds;
    struct timeval tv;
    int sockfd;

    for(keypressed = false; !keypressed; ) {
	timer_keypress = lastkeypress();

	FD_ZERO(&fds);
	FD_SET(0, &fds);

	if(!regmode) {
	    exectimers();

	    if(ihook.getsockfd()) {
		FD_SET(ihook.getsockfd(), &fds);
	    }

	    if(yhook.getsockfd()) {
		FD_SET(yhook.getsockfd(), &fds);
	    }

	    if(ihook.online() || yhook.online()) {
		tv.tv_sec = PERIOD_SELECT;
	    } else {
		tv.tv_sec = PERIOD_RECONNECT/3;
	    }
	}

	tv.tv_usec = 0;
	sockfd = max(ihook.getsockfd(), yhook.getsockfd());
	select(sockfd+1, &fds, 0, 0, &tv);

	if(FD_ISSET(0, &fds)) {
	    keypressed = true;
	    time(&timer_keypress);
	} else if(ihook.online() && FD_ISSET(ihook.getsockfd(), &fds)) {
	    icq_HandleServerResponse(&icql);
	    if(options & HIDL_SOCKEXIT) break;
	} else if(yhook.online() && FD_ISSET(yhook.getsockfd(), &fds)) {
	    yhook.main();
	    if(options & HIDL_SOCKEXIT) break;
	}
    }

    return keypressed;
}

void centericq::exectimers() {
    time_t timer_current = time(0);

    if(!conf.getourid(icq).empty()) ihook.exectimers();
    if(!conf.getourid(yahoo).empty()) yhook.exectimers();

    if(timer_current-timer_checkmail > PERIOD_CHECKMAIL) {
	cicq.checkmail();
	time(&timer_checkmail);
    }
}

time_t centericq::getkeypress() const {
    return timer_keypress;
}
