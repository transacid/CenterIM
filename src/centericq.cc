/*
*
* centericq core routines
* $Id: centericq.cc,v 1.127 2002/10/04 16:58:52 konst Exp $
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
#include "icqhook.h"
#include "irchook.h"
#include "yahoohook.h"
#include "icqface.h"
#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqmlist.h"
#include "icqgroups.h"
#include "accountmanager.h"

centericq::centericq()
    : timer_checkmail(0), timer_update(0), timer_resend(0),
      regmode(false)
{
}

centericq::~centericq() {
}

void centericq::exec() {
    struct sigaction sact;

    memset(&sact, 0, sizeof(sact));
    sact.sa_handler = &handlesignal;
    sigaction(SIGINT, &sact, 0);
    sigaction(SIGCHLD, &sact, 0);
    sigaction(SIGALRM, &sact, 0);
    sigaction(SIGUSR1, &sact, 0);
    sigaction(SIGUSR2, &sact, 0);

    kinterface();
    raw();

    conf.load();
    face.init();

    if(regmode = !conf.getouridcount()) {
	char *p = getenv("LANG");

	if(p)
	if(((string) p).substr(0, 2) == "ru")
	    conf.setrussian(true);

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
	inithooks();

	if(checkpasswords()) {
	    mainloop();
	}

	lst.save();
	clist.save();
	groups.save();
    }

    face.done();
    conf.save();
}

bool centericq::checkpasswords() {
    protocolname pname;
    icqconf::imaccount ia;
    bool r;

    r = regmode = true;

    for(pname = icq; pname != protocolname_size; (int) pname += 1) {
	if(!gethook(pname).getCapabs().count(hookcapab::optionalpassword)) {
	    if(!(ia = conf.getourid(pname)).empty()) {
		if(ia.password.empty()) {
		    conf.setsavepwd(false);

		    ia.password = face.inputstr("[" +
			conf.getprotocolname(pname) + "] " +
			_("password: "), "", '*');

		    if(ia.password.empty()) {
			r = false;
			break;
		    } else if(face.getlastinputkey() != KEY_ESC) {
			conf.setourid(ia);
		    }
		}
	    }
	}
    }

    regmode = false;
    return r;
}

void centericq::inithooks() {
    protocolname pname;

    for(pname = icq; pname != protocolname_size; (int) pname += 1) {
	gethook(pname).init();
    }
}

void centericq::mainloop() {
    bool finished = false, r;
    string text, url;
    int action, old, gid;
    icqcontact *c;
    char buf[512];
    vector<imcontact>::iterator i;

    face.draw();

    while(!finished) {
	face.status(_("F2/M current contact menu, F3/S change status, F4/G general actions, Q quit"));
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
	    case ACT_TRANSFERS:
		face.transfermonitor();
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
		sendevent(imurl(c->getdesc(), imevent::outgoing, "", ""),
		    icqface::ok);
		break;

	    case ACT_SMS:
		sendevent(imsms(c->getdesc(), imevent::outgoing,
		    c->getpostponed()), icqface::ok);
		break;

	    case ACT_CONTACT:
		sendevent(imcontacts(c->getdesc(), imevent::outgoing, vector< pair<unsigned int, string> >()),
		    icqface::ok);
		break;

	    case ACT_AUTH:
		sendevent(imauthorization(c->getdesc(), imevent::outgoing,
		    imauthorization::Request, ""), icqface::ok);
		break;

	    case ACT_FILE:
		sendevent(imfile(c->getdesc(), imevent::outgoing), icqface::ok);
		break;

	    case ACT_IGNORE:
		sprintf(buf, _("Ignore all events from %s?"), c->getdesc().totext().c_str());
		if(face.ask(buf, ASK_YES | ASK_NO, ASK_NO) == ASK_YES) {
		    lst.push_back(modelistitem(c->getdispnick(), c->getdesc(), csignore));
		    clist.remove(c->getdesc());
		    face.update();
		}
		break;

	    case ACT_EDITUSER:
		c->save();
		if(face.updatedetails(c)) {
		    if(c->getdesc().pname == infocard)
			c->setdispnick(c->getnick());

		    c->save();
		    face.relaxedupdate();
		} else {
		    c->load();
		}
		break;

	    case ACT_ADD:
		if(!c->inlist()) {
		    addcontact(c->getdesc());
		}
		break;

	    case ACT_INFO:
		if(c->getdesc() != contactroot) {
		    if(!ischannel(c)) {
			userinfo(c->getdesc());

		    } else {
			imsearchparams s;
			s.pname = c->getdesc().pname;
			s.room = c->getdesc().nickname;
			face.findresults(s);

		    }
		}
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
		if(face.getlastinputkey() != KEY_ESC) {
		    c->setdispnick(text);
		    face.update();
		}
		break;

	    case ACT_GROUPMOVE:
		if(gid = face.selectgroup(_("Select a group to move the user to"))) {
		    c->setgroupid(gid);
		    face.fillcontactlist();
		    face.update();
		}
		break;

	    case ACT_HISTORY:
		history(c->getdesc());
		break;

	    case ACT_FETCHAWAY:
		{
		    abstracthook &hook = gethook(c->getdesc().pname);

		    if(hook.getstatus() != offline) {
			hook.requestawaymsg(c->getdesc());
		    } else {
			face.log(_("+ cannot fetch away messages being offline"));
		    }
		}
		break;

	    case ACT_VERSION:
		gethook(c->getdesc().pname).requestversion(c->getdesc());
		break;

	    case ACT_PING:
		gethook(c->getdesc().pname).ping(c->getdesc());
		break;

	    case ACT_JOIN:
	    case ACT_LEAVE:
		joinleave(c);
		break;

	    case ACT_MSG:
	    case 0:
		if(conf.getchatmode()) {
		    face.chat(c->getdesc());

		} else {
		    if(c->getmsgcount()) {
			readevents(c->getdesc());
		    } else {
			if(c->getdesc() != contactroot)
			if(c->getdesc().pname != infocard) {
			    sendevent(immessage(c->getdesc(), imevent::outgoing,
				c->getpostponed()), icqface::ok);
			}
		    }
		}
		break;
	}
    }
}

void centericq::changestatus() {
    imstatus st;
    protocolname ipname, pname;
    bool proceed, setaway;
    string tmp, prompt;
    icqconf::imaccount ia;

    if(face.changestatus(pname, st)) {
	if(!conf.enoughdiskspace()) {
	    face.log("! cannot connect, free disk space is less than 10k");

	} else {
	    proceed = true;

	    setaway = conf.getaskaway();
	    if(pname != proto_all) {
		setaway = setaway && gethook(pname).getCapabs().count(hookcapab::setaway);
	    }

	    if(setaway)
	    switch(st) {
		case away:
		case notavail:
		case occupied:
		case dontdisturb:
		    if(pname != proto_all)
			prompt = conf.getprotocolname(pname) + ": ";
		    prompt += _("away message");
		    proceed = setaway = face.edit(tmp = conf.getawaymsg(pname), prompt);
		    break;
		default:
		    setaway = false;
		    break;
	    }

	    if(proceed)
	    for(ipname = icq; ipname != protocolname_size; (int) ipname += 1) {
		if(!(ia = conf.getourid(ipname)).empty())
		if((pname == proto_all) || (ipname == pname)) {
		    abstracthook &hook = gethook(ipname);

		    if(setaway && !tmp.empty())
			if(hook.getCapabs().count(hookcapab::setaway))
			    conf.setawaymsg(ipname, tmp);

		    hook.setstatus(st);
		}
	    }

	    face.update();
	}
    }
}

void centericq::find() {
    static imsearchparams s;
    bool ret = true;
    icqcontact *c;

    while(ret) {
	if(ret = face.finddialog(s)) {
	    switch(s.pname) {
		case icq:
		    if(s.uin) {
			addcontact(imcontact(s.uin, s.pname));
			ret = false;
		    } else {
			ret = face.findresults(s);
		    }

		    break;

		case msn:
		    if(s.nick.size() > 12)
		    if(s.nick.substr(s.nick.size()-12) == "@hotmail.com") {
			s.nick.erase(s.nick.size()-12);
		    }

		case irc:
		    if(!s.nick.empty()) {
			addcontact(imcontact(s.nick, s.pname));
			ret = false;
		    } else {
			ret = face.findresults(s);
		    }
		    break;

		case yahoo:
		case aim:
		    if(!s.nick.empty()) {
			addcontact(imcontact(s.nick, s.pname));
			ret = false;
		    }
		    break;

		case infocard:
		    c = clist.addnew(imcontact(0, infocard), true);
		    c->setnick(s.nick);
		    c->setdispnick(s.nick);

		    if(face.updatedetails(c)) {
			addcontact(c->getdesc());
		    }

		    if(!c->inlist()) {
			clist.remove(c->getdesc());
		    } else {
			c->save();
		    }

		    face.update();
		    ret = false;
		    break;
	    }
	}
    }
}

void centericq::userinfo(const imcontact &cinfo) {
    imcontact realuin = cinfo;
    icqcontact *c = clist.get(cinfo);

    if(!c) {
	c = clist.get(contactroot);
	realuin = contactroot;
	c->clear();
	gethook(cinfo.pname).requestinfo(cinfo);
    }

    if(c) {
	face.userinfo(cinfo, realuin);
    }
}

bool centericq::updateconf() {
    bool r;

    icqconf::regsound snd = icqconf::rsdontchange;
    icqconf::regcolor clr = icqconf::rcdontchange;

    if(r = face.updateconf(snd, clr)) {
	if(snd != icqconf::rsdontchange) {
	    conf.setregsound(snd);
	    unlink(conf.getconfigfname("sounds").c_str());
	    conf.loadsounds();
	}

	if(clr != icqconf::rcdontchange) {
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

	if(fsize != -1 && st.st_size > fsize) {
	    /*
	    *
	    * I.e. only if mailbox has grown after the recent check.
	    *
	    */

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
		time_t tnow;
		const struct tm *lt;

		lt = localtime(&(tnow = time(0)));

		face.log(_("+ [%02d:%02d:%02d] new mail arrived, %d messages"),
		    lt->tm_hour, lt->tm_min, lt->tm_sec, mcount);

		face.log(_("+ last msg from %s"), lastfrom.c_str());

		clist.get(contactroot)->playsound(imevent::email);
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

	case SIGUSR1:
	    cicq.rereadstatus();
	    break;

	case SIGALRM:
	case SIGUSR2:
	    break;
    }
}

void centericq::checkparallel() {
    string pidfname = conf.getdirname() + "pid", fname;
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

void centericq::rereadstatus() {
    protocolname pname;
    icqconf::imaccount ia;

    for(pname = icq; pname != protocolname_size; (int) pname += 1) {
	ia = conf.getourid(pname);

	if(!ia.empty()) {
	    char cst;
	    imstatus st;
	    string fname = conf.getconfigfname((string) "status-" + conf.getprotocolname(pname));
	    ifstream f(fname.c_str());

	    if(f.is_open()) {
		f >> cst, f.close(), f.clear();
		unlink(fname.c_str());

		for(st = offline; st != imstatus_size; (int) st += 1) {
		    if(imstatus2char[st] == cst) {
			gethook(pname).setstatus(st);
			break;
		    }
		}
	    }
	}
    }
}

bool centericq::sendevent(const imevent &ev, icqface::eventviewresult r) {
    bool proceed;
    string text, fwdnote;
    imevent *sendev;
    icqcontact *c;
    vector<imcontact>::iterator i;

    sendev = 0;
    face.muins.clear();

    if(ev.getdirection() == imevent::incoming) {
	fwdnote = ev.getcontact().totext();
    } else {
	fwdnote = "I";
    }

    fwdnote += " wrote:\n\n";

    if(ev.gettype() == imevent::message) {
	const immessage *m = dynamic_cast<const immessage *>(&ev);

	text = m->gettext();

	if(r == icqface::reply) {
	    if(conf.getquote()) {
		text = quotemsg(text);
	    } else {
		text = "";
	    }
	} else if(r == icqface::forward) {
	    text = fwdnote + text;
	}

	sendev = new immessage(m->getcontact(), imevent::outgoing, text);

    } else if(ev.gettype() == imevent::url) {
	const imurl *m = dynamic_cast<const imurl *>(&ev);

	text = m->getdescription();

	switch(r) {
	    case icqface::forward:
		text = fwdnote + text;
	    case icqface::ok:
		sendev = new imurl(m->getcontact(), imevent::outgoing,
		    m->geturl(), text);
		break;

	    case icqface::reply:
		/* reply to an URL is a message */
		sendev = new immessage(m->getcontact(), imevent::outgoing, "");
		break;
	}

    } else if(ev.gettype() == imevent::authorization) {
	const imauthorization *m = dynamic_cast<const imauthorization *>(&ev);

	text = m->getmessage();

	switch(r) {
	    case icqface::ok:
		sendev = new imauthorization(m->getcontact(), imevent::outgoing,
		    m->getauthtype(), text);
		break;

	    case icqface::forward:
		text = fwdnote + m->gettext();
	    case icqface::reply:
		/* reply to an URL is a message */
		sendev = new immessage(m->getcontact(), imevent::outgoing, text);
		break;
	}

    } else if(ev.gettype() == imevent::sms) {
	const imsms *m = dynamic_cast<const imsms *>(&ev);
	string phone = m->getphone();

	if(phone.empty() && (c = clist.get(ev.getcontact()))) {
	    icqcontact::basicinfo b = c->getbasicinfo();

	    if(b.cellular.find_first_of("0123456789") == -1) {
		b.cellular = face.inputstr(_("Mobile number: "), b.cellular);

		if((b.cellular.find_first_of("0123456789") == -1)
		|| (face.getlastinputkey() == KEY_ESC))
		    return false;

		c->setbasicinfo(b);
		c->save();
	    }

	    phone = b.cellular;
	}

	text = m->getmessage();

	if(r == icqface::reply) {
	    if(conf.getquote()) {
		text = quotemsg(text);
	    } else {
		text = "";
	    }
	} else if(r == icqface::forward) {
	    text = fwdnote + text;
	}

	sendev = new imsms(m->getcontact(), imevent::outgoing, text, phone);

    } else if(ev.gettype() == imevent::contacts) {
	const imcontacts *m = dynamic_cast<const imcontacts *>(&ev);

	switch(r) {
	    case icqface::forward:
	    case icqface::ok:
		sendev = new imcontacts(m->getcontact(), imevent::outgoing,
		    m->getcontacts());
		break;

	    case icqface::reply:
		/* reply to contacts is a message */
		sendev = new immessage(m->getcontact(), imevent::outgoing, "");
		break;
	}

    } else if(ev.gettype() == imevent::file) {
	sendev = new imfile(ev);

    }

    if(proceed = sendev) {
	switch(r) {
	    case icqface::forward:
		if(proceed = face.multicontacts())
		    proceed = !face.muins.empty();
		break;
	    case icqface::reply:
	    case icqface::ok:
		face.muins.push_back(ev.getcontact());
		break;
	}

	if(proceed) {
	    if(proceed = face.eventedit(*sendev))
	    if(proceed = !sendev->empty()) {
		for(i = face.muins.begin(); i != face.muins.end(); ++i) {
		    sendev->setcontact(*i);
		    em.store(*sendev);
		}

		face.muins.clear();
		face.muins.push_back(ev.getcontact());

	    }
	}
    }

    if(sendev) {
	delete sendev;
    }

    return proceed;
}

icqface::eventviewresult centericq::readevent(const imevent &ev, const vector<icqface::eventviewresult> &buttons) {
    bool b1, b2;
    return readevent(ev, b1, b2, buttons);
}

icqface::eventviewresult centericq::readevent(const imevent &ev, bool &enough, bool &fin, const vector<icqface::eventviewresult> &buttons) {
    icqface::eventviewresult r = face.eventview(&ev, buttons);

    imcontact cont = ev.getcontact();
    string nickname, tmp;

    if(ev.gettype() == imevent::contacts) {
	const imcontacts *m = static_cast<const imcontacts *>(&ev);

	if(face.extk <= m->getcontacts().size() && face.extk > 0) {
	    vector< pair<unsigned int, string> >::const_iterator icc =
		m->getcontacts().begin()+face.extk-1;

	    nickname = icc->second;
	    cont = imcontact();
	    cont.pname = ev.getcontact().pname;

	    if(!(cont.uin = icc->first))
		cont.nickname = nickname;
	}
    }

    switch(r) {
	case icqface::forward:
	    sendevent(ev, r);
	    break;

	case icqface::reply:
	    sendevent(ev, r);
	    enough = true;
	    break;

	case icqface::ok:
	    enough = true;
	    break;

	case icqface::cancel:
	    fin = true;
	    break;

	case icqface::open:
	    if(const imurl *m = static_cast<const imurl *>(&ev))
		conf.openurl(m->geturl());
	    break;

	case icqface::accept:
	    switch(ev.gettype()) {
		case imevent::authorization:
		    em.store(imauthorization(ev.getcontact(),
			imevent::outgoing, imauthorization::Granted, "accepted"));
		    enough = true;
		    break;

		case imevent::file:
		    tmp = face.inputdir(_("directory to save the file(s) to: "), getenv("HOME"));

		    if(enough = (!tmp.empty() && !access(tmp.c_str(), X_OK))) {
			if(enough = !access(tmp.c_str(), W_OK)) {
			    const imfile *f = static_cast<const imfile *>(&ev);
			    gethook(ev.getcontact().pname).replytransfer(*f, true, tmp);
			} else {
			    face.status(_("The specified directory is not writable"));
			}

		    } else {
			face.status(_("The specified directory does not exist"));

		    }
		    break;
	    }

	    break;

	case icqface::reject:
	    switch(ev.gettype()) {
		case imevent::authorization:
		    em.store(imauthorization(ev.getcontact(),
			imevent::outgoing, imauthorization::Rejected, "rejected"));
		    break;

		case imevent::file:
		    gethook(ev.getcontact().pname).replytransfer(*static_cast<const imfile *>(&ev), false);
		    break;
	    }

	    enough = true;
	    break;

	case icqface::info:
	    cicq.userinfo(cont);
	    break;

	case icqface::add:
	    icqcontact *c = cicq.addcontact(cont);
	    if(c) c->setdispnick(nickname);
	    break;
    }

    return r;
}

void centericq::readevents(const imcontact cont) {
    vector<imevent *> events;
    vector<imevent *>::iterator iev;
    icqcontact *c = clist.get(cont);
    bool fin, enough;

    if(c) {
	fin = false;
	face.saveworkarea();

	while(c->getmsgcount() && !fin) {
	    events = em.getevents(cont, c->getlastread());
	    fin = events.empty();

	    for(iev = events.begin(); (iev != events.end()) && !fin; ++iev) {
		if((*iev)->getdirection() == imevent::incoming) {
		    enough = false;

		    while(!enough && !fin) {
			readevent(**iev, enough, fin);
		    }
		}

		c->setlastread((*iev)->gettimestamp());
	    }

	    while(!events.empty()) {
		delete events.back();
		events.pop_back();
	    }
	}

	c->save();
	face.restoreworkarea();
	face.update();
    }
}

void centericq::history(const imcontact &cont) {
    icqface::eventviewresult r;
    imevent *im;
    bool enough;
    vector<imevent *> events;
    vector<imevent *>::iterator i;
    vector<icqface::eventviewresult> buttons;

    buttons.push_back(icqface::prev);
    buttons.push_back(icqface::next);

    events = em.getevents(cont, 0);

    if(!events.empty()) {
	face.saveworkarea();
	face.histmake(events);

	while(face.histexec(im)) {
	    enough = false;

	    while(!enough) {
		r = readevent(*im, buttons);

		switch(r) {
		    case icqface::cancel:
		    case icqface::ok:
			enough = true;
			break;

		    case icqface::next:
			i = ::find(events.begin(), events.end(), im);
			enough = i == events.begin();

			if(!enough) {
			    i--;
			    im = *i;
			}
			break;

		    case icqface::prev:
			i = ::find(events.begin(), events.end(), im);
			enough = i == events.end()-1;

			if(!enough) {
			    i++;
			    im = *i;
			}
			break;
		}
	    }
	}

	face.restoreworkarea();
	face.status("");

	while(!events.empty()) {
	    delete events.back();
	    events.pop_back();
	}

    } else {
	face.log(_("+ no history items for %s"), cont.totext().c_str());
    }
}

string centericq::quotemsg(const string &text) {
    string ret;
    vector<string> lines;
    vector<string>::iterator i;

    breakintolines(text, lines, 50/*face.sizeWArea.x2-face.sizeWArea.x1-4*/);

    for(i = lines.begin(); i != lines.end(); ++i) {
	if(!i->empty()) ret += (string) "> " + *i;
	ret = trailcut(ret);
	ret += "\n";
    }

    return ret;
}

icqcontact *centericq::addcontact(const imcontact &ic) {
    icqcontact *c;
    int groupid = 0;

    if(c = clist.get(ic)) {
	if(c->inlist()) {
	    face.log(_("+ user %s is already on the list"), ic.totext().c_str());
	    return 0;
	}
    }

    if(conf.getgroupmode() != icqconf::nogroups) {
	groupid = face.selectgroup(_("Select a group to add the user to"));
	if(!groupid) return 0;
    }

    if(!c) {
	c = clist.addnew(ic, false);
    } else {
	c->includeintolist();
    }

    c->setgroupid(groupid);
    face.log(_("+ %s has been added to the list"), ic.totext().c_str());
    face.relaxedupdate();

    return c;
}

bool centericq::idle(int options ) {
    bool keypressed, online, fin;
    fd_set rfds, wfds, efds;
    struct timeval tv;
    int hsockfd;
    protocolname pname;

    for(keypressed = fin = false; !keypressed && !fin; ) {
	timer_keypress = lastkeypress();

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&efds);

	FD_SET(hsockfd = 0, &rfds);
	online = false;

	if(!regmode) {
	    exectimers();

	    for(pname = icq; pname != protocolname_size; (int) pname += 1) {
		abstracthook &hook = gethook(pname);

		if(hook.online()) {
		    hook.getsockets(rfds, wfds, efds, hsockfd);
		    online = true;
		}
	    }
	}

	tv.tv_sec = face.updaterequested() ? 1 : 30;
	tv.tv_usec = 0;

	select(hsockfd+1, &rfds, &wfds, &efds, &tv);

	if(FD_ISSET(0, &rfds)) {
	    keypressed = true;
	    time(&timer_keypress);
	} else {
	    for(pname = icq; pname != protocolname_size; (int) pname += 1) {
		abstracthook &hook = gethook(pname);

		if(hook.online())
		if(hook.isoursocket(rfds, wfds, efds)) {
		    hook.main();
		    fin = fin || (options & HIDL_SOCKEXIT);
		}
	    }
	}
    }

    return keypressed;
}

void centericq::setauto(imstatus astatus) {
    protocolname pname;
    imstatus stcurrent;
    static bool autoset = false;
    bool nautoset, changed = false;

    if(astatus == available && !autoset) return;

    nautoset = autoset;

    if(autoset && (astatus == available)) {
	face.log(_("+ the user is back"));
	nautoset = false;
    }

    for(pname = icq; pname != protocolname_size; (int) pname += 1) {
	abstracthook &hook = gethook(pname);
	stcurrent = hook.getstatus();

	if(hook.logged())
	switch(stcurrent) {
	    case invisible:
	    case offline:
		break;

	    default:
		if(autoset && (astatus == available)) {
		    face.log(_("+ [%s] status restored"),
			conf.getprotocolname(pname).c_str());

		    hook.restorestatus();
		    nautoset = false;
		    changed = true;
		} else {
		    if(astatus == away && stcurrent == notavail)
			break;

		    if(astatus != stcurrent)
		    if(!autoset || (autoset && (stcurrent == away))) {
			hook.setautostatus(astatus);
			nautoset = changed = true;

			face.log(_("+ [%s] automatically set %s"),
			    conf.getprotocolname(pname).c_str(),
			    astatus == away ? _("away") : _("n/a"));
		    }
		}
		break;
	}
    }

    if(changed) {
	autoset = nautoset;
	face.relaxedupdate();
    }
}

#define MINCK0(x, y)       (x ? (y ? (x > y ? y : x) : x) : y)

void centericq::exectimers() {
    time_t timer_current = time(0);
    protocolname pname;
    int paway, pna;
    bool fonline = false;

    /*
    *
    * Execute regular actions for all the hooks.
    *
    */

    for(pname = icq; pname != protocolname_size; (int) pname += 1) {
	if(!conf.getourid(pname).empty()) {
	    abstracthook &hook = gethook(pname);

	    /*
	    *
	    * A hook's own action goes first.
	    *
	    */

	    hook.exectimers();

	    static map<protocolname, reconnectInfo> reconnect;

	    if(timer_current-reconnect[pname].timer > reconnect[pname].period) {
		/*
		*
		* Any need to try auto re-connecting?
		*
		*/

		if(!hook.logged()) {
		    time(&reconnect[pname].timer);

		    if(reconnect[pname].period < 180)
			reconnect[pname].period += reconnect[pname].period/2;

		    if(hook.online()) {
			hook.disconnect();

		    } else if(conf.getstatus(pname) != offline) {
			if(conf.enoughdiskspace() && !manager.isopen()) {
			    hook.connect();
			}

		    }
		} else {
		    fonline = true;
		    reconnect[pname] = reconnectInfo();
		}
	    }
	}
    }

    /*
    *
    * How let's find out how are the auto-away mode is doing.
    *
    */

    if(fonline) {
	conf.getauto(paway, pna);

	if(paway || pna) {
	    imstatus toset = offline;
	    static map<imstatus, bool> autostat;

	    if(autostat.empty()) {
		autostat[available] = false;
		autostat[notavail] = false;
		autostat[away] = false;
	    }

	    if(paway && timer_current-timer_keypress > paway*60)
		toset = away;

	    if(pna && timer_current-timer_keypress > pna*60)
		toset = notavail;

	    if(timer_current-timer_keypress < MINCK0(paway, pna)*60)
		toset = available;

	    if(toset != offline && !autostat[toset]) {
		setauto(toset);

		if(toset == available) {
		    autostat[away] = autostat[notavail] = false;
		} else {
		    autostat[toset] = true;
		}
	    }
	}
    }

    if(timer_current-timer_resend > PERIOD_RESEND) {
	/*
	*
	* We'll check for free disk space here too.
	*
	*/

	conf.checkdiskspace();

	if(!conf.enoughdiskspace()) {
	    if(fonline) {
		for(pname = icq; pname != protocolname_size; (int) pname += 1)
		    gethook(pname).disconnect();

		face.log(_("! free disk space is less than 10k, going offline"));
		face.log(_("! otherwise we can lose events and configuration"));
	    }
	} else {
	    /*
	    *
	    * Re-send the idle IM events.
	    *
	    */

	    em.resend();
	    face.relaxedupdate();
	}

	timer_resend = timer_current;
    }

    if(timer_current-timer_checkmail > PERIOD_CHECKMAIL) {
	cicq.checkmail();
	timer_checkmail = timer_current;
    }

    if(face.updaterequested())
    if(timer_current-timer_update > PERIOD_DISPUPDATE) {
	face.update();
	timer_update = timer_current;
    }
}

void centericq::joinleave(icqcontact *c) {
    vector<irchook::channelInfo> channels = irhook.getautochannels();
    vector<irchook::channelInfo>::iterator ic = ::find(channels.begin(), channels.end(), c->getdesc().nickname);

    if(ic != channels.end()) {
	ic->joined = !ic->joined;
	irhook.setautochannels(channels);
    }
}

// ----------------------------------------------------------------------------

string rushtmlconv(const string &tdir, const string &text) {
    int pos;
    string r = rusconv(tdir, text);

    if(tdir == "kw") {
	pos = 0;
	while((pos = r.find_first_of("&<>", pos)) != -1) {
	    switch(r[pos]) {
		case '&':
		    if(r.substr(pos, 4) != "&lt;"
		    && r.substr(pos, 4) != "&gt;")
			r.replace(pos, 1, "&amp;");
		    break;
		case '<': r.replace(pos, 1, "&lt;"); break;
		case '>': r.replace(pos, 1, "&gt;"); break;
	    }
	    pos++;
	}

    } else if(tdir == "wk") {
	pos = 0;
	while((pos = r.find("&", pos)) != -1) {
	    if(r.substr(pos+1, 4) == "amp;") r.replace(pos, 5, "&"); else
	    if(r.substr(pos+1, 3) == "lt;") r.replace(pos, 4, "<"); else
	    if(r.substr(pos+1, 3) == "gt;") r.replace(pos, 4, ">"); 
	    pos++;
	}
    }

    return r;
}

string ruscrlfconv(const string &tdir, const string &text) {
    string r = rusconv(tdir, text);
    int pos;

    for(pos = 0; (pos = r.find("\r")) != -1; pos++) {
	r.erase(pos, 1);
    }

    return r;
}

string rusconv(const string &tdir, const string &text) {
    string r;

#ifndef HAVE_ICONV_H
    static unsigned char kw[] = {
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
	160,161,162,184,164,165,166,167,168,169,170,171,172,173,174,175,
	176,177,178,168,180,181,182,183,184,185,186,187,188,189,190,191,
	254,224,225,246,228,229,244,227,245,232,233,234,235,236,237,238,
	239,255,240,241,242,243,230,226,252,251,231,248,253,249,247,250,
	222,192,193,214,196,197,212,195,213,200,201,202,203,204,205,206,
	207,223,208,209,210,211,198,194,220,219,199,216,221,217,215,218
    };

    static unsigned char wk[] = {
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
	160,161,162,163,164,165,166,167,179,169,170,171,172,173,174,175,
	176,177,178,179,180,181,182,183,163,185,186,187,188,189,190,191,
	225,226,247,231,228,229,246,250,233,234,235,236,237,238,239,240,
	242,243,244,245,230,232,227,254,251,253,255,249,248,252,224,241,
	193,194,215,199,196,197,214,218,201,202,203,204,205,206,207,208,
	210,211,212,213,198,200,195,222,219,221,223,217,216,220,192,209
    };

    unsigned char c;
    string::const_iterator i;
    unsigned char *table = 0;

    if(tdir == "kw") table = kw; else
    if(tdir == "wk") table = wk;

    if(conf.getrussian() && table) {
	for(i = text.begin(); i != text.end(); ++i) {
	    c = (unsigned char) *i;
	    c &= 0377;
	    if(c & 0200) c = table[c & 0177];
	    r += c;
	}
    } else {
	r = text;
    }
#else

    if(conf.getrussian()) {
	if(tdir == "kw") r = siconv(text, "koi8-r", "cp1251"); else
	if(tdir == "wk") r = siconv(text, "cp1251", "koi8-r"); else
	    r = text;
    } else {
	r = text;
    }

#endif

    return r;
}

bool ischannel(const imcontact &cont) {
    return
	(cont.nickname.substr(0, 1) == "#") &&
	(cont.pname == irc || cont.pname == yahoo);
}
