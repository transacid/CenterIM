/*
*
* centericq core routines
* $Id: centericq.cc,v 1.197 2005/05/02 15:23:58 konst Exp $
*
* Copyright (C) 2001-2003 by Konstantin Klyagin <konst@konst.org.ua>
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
#include "rsshook.h"
#include "ljhook.h"
#include "icqface.h"
#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqmlist.h"
#include "icqgroups.h"
#include "accountmanager.h"
#include "imexternal.h"

centericq::centericq()
    : timer_checkmail(0), timer_update(0), timer_resend(0),
      timer_autosave(0), regmode(false)
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
    sigaction(SIGTERM, &sact, 0);

    kinterface();
    raw();

    conf.load();
    face.init();

    if(regmode = !conf.getouridcount()) {
	bool rus = false;
	char *p = getenv("LANG");

	if(p)
	if(rus = (((string) p).substr(0, 2) == "ru")) {
	    conf.setcharsets("cp1251", "koi8-r");
	    for(protocolname pname = icq; pname != protocolname_size; pname++)
		conf.setcpconvert(pname, true);
	}

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

    for(pname = icq; pname != protocolname_size; pname++) {
	if(gethook(pname).enabled() && !gethook(pname).getCapabs().count(hookcapab::optionalpassword)) {
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

    for(pname = icq; pname != protocolname_size; pname++) {
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
	face.status(_("%s contact menu, %s status, %s general, %s/%s next/prev chat, %s quit"),
	    face.getstatkey(key_user_menu, section_contact).c_str(),
	    face.getstatkey(key_change_status, section_contact).c_str(),
	    face.getstatkey(key_general_menu, section_contact).c_str(),
	    face.getstatkey(key_next_chat, section_contact).c_str(),
	    face.getstatkey(key_prev_chat, section_contact).c_str(),
	    face.getstatkey(key_quit, section_contact).c_str());

	face.xtermtitle();
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
	    case ACT_MASS_MOVE:
		massmove();
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
	    case ACT_JOINDIALOG:
		joindialog();
		break;
	    case ACT_RSS:
		linkfeed();
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

	    case ACT_LJ:
		sendevent(imxmlevent(c->getdesc(),
		    imevent::outgoing, c->getpostponed()), icqface::ok);
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
		if(lst.inlist(c->getdesc(), csignore)) {
		    lst.del(c->getdesc(), csignore);
		} else {
		    sprintf(buf, _("Ignore all events from %s?"), c->getdesc().totext().c_str());
		    if(face.ask(buf, ASK_YES | ASK_NO, ASK_NO) == ASK_YES) {
			lst.push_back(modelistitem(c->getdispnick(), c->getdesc(), csignore));
			if(!ischannel(c)) {
			    sprintf(buf, _("Remove %s from the contact list as well?"), c->getdesc().totext().c_str());
			    if(face.ask(buf, ASK_YES | ASK_NO, ASK_NO) == ASK_YES) {
				clist.remove(c->getdesc());
				face.update();
			    }
			}
		    }
		}
		break;

	    case ACT_EDITUSER:
		c->save();
		if(face.updatedetails(c, c->getdesc().pname)) {
		    if(c->getdesc().pname == infocard)
			c->setdispnick(c->getnick());

		    c->save();
		    face.relaxedupdate();
		} else {
		    c->load();
		}
		break;

	    case ACT_ADD:
		if(!c->inlist())
		if(!ischannel(c) || !gethook(c->getdesc().pname).getCapabs().count(hookcapab::conferencesaretemporary))
		    addcontact(c->getdesc());
		break;

	    case ACT_INFO:
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
		if(face.getlastinputkey() != KEY_ESC) {
		    c->setdispnick(text);
		    gethook(c->getdesc().pname).updatecontact(c);
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

	    case ACT_CONFER:
		if(gethook(c->getdesc().pname).getCapabs().count(hookcapab::conferencesaretemporary))
		    createconference(c);
		break;

	    case ACT_JOIN:
	    case ACT_LEAVE:
		{
		    icqcontact::basicinfo cb = c->getbasicinfo();
		    cb.requiresauth = c->getstatus() == offline;
		    c->setstatus(cb.requiresauth ? available : offline);
		    c->setbasicinfo(cb);
		}
		break;

	    case ACT_EXTERN:
		face.userinfoexternal(c->getdesc());
		break;

	    case ACT_PGPSWITCH:
		c->setusepgpkey(!c->getusepgpkey());
		break;

	    case ACT_PGPKEY:
		text = c->getpgpkey();

		if(!text.empty()) {
		    c->setpgpkey("");
		} else {
		    if(face.selectpgpkey(text, false))
			c->setpgpkey(text);
		}

		break;

	    case ACT_MSG:
	    case 0:
		if(conf.getchatmode(c->getdesc().pname)) {
		    face.chat(c->getdesc());

		} else {
		    if(c->hasevents()) {
			readevents(c->getdesc());
		    } else {
			if(c->getdesc().pname != infocard && c->getdesc().pname != rss) {
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
    bool proceed, setaway;
    string tmp, prompt, awaymsg;
    icqconf::imaccount ia;

    vector<protocolname> pnames;
    vector<protocolname>::const_iterator ipname;

    if(face.changestatus(pnames, st)) {
	if(!conf.enoughdiskspace()) {
	    face.log(_("! cannot connect, free disk space is less than 10k"));

	} else {
	    proceed = true;
	    setaway = false;

	    bool same = true;

	    for(ipname = pnames.begin(); ipname != pnames.end(); ++ipname) {
		if(conf.getaskaway())
		    setaway = setaway || gethook(*ipname).getCapabs().count(hookcapab::setaway);

		if(!conf.getourid(*ipname).empty()) {
		    tmp = conf.getawaymsg(*ipname);
		    if(!tmp.empty()) {
			if(awaymsg.empty()) awaymsg = tmp; else
			    same = same && tmp == awaymsg;
		    }
		}
	    }

	    if(!same)
		awaymsg = "";

	    if(setaway)
	    switch(st) {
		case away:
		case notavail:
		case occupied:
		case dontdisturb:
		    if(pnames.size() == 1)
			prompt = conf.getprotocolname(pnames.front()) + ": ";
		    prompt += _("away message");
		    proceed = setaway = face.edit(tmp = conf.getawaymsg(pnames.front()), prompt);
		    break;
		default:
		    setaway = false;
		    break;
	    }

	    if(proceed)
	    for(ipname = pnames.begin(); ipname != pnames.end(); ++ipname) {
		if(!gethook(*ipname).enabled()) {
		    face.log(_("! support for %s was disabled at build time"),
			conf.getprotocolname(*ipname).c_str());

		} else {
		    abstracthook &hook = gethook(*ipname);

		    if(setaway && !tmp.empty())
			if(hook.getCapabs().count(hookcapab::setaway))
			    conf.setawaymsg(*ipname, tmp);

		    hook.setstatus(st);
		    reconnect[*ipname].timer = timer_current;

		}
	    }

	    face.update();
	}
    }
}

void centericq::joindialog() {
    static imsearchparams s;
    icqcontact *c;

    if(face.finddialog(s, icqface::fschannel)) {
	imcontact ic(s.nick, s.pname);
	abstracthook &h = gethook(s.pname);

	if(ic.nickname.substr(0, 1) != "#")
	    ic.nickname.insert(0, "#");

	if(h.getCapabs().count(hookcapab::groupchatservices)) {
	    if(ic.nickname.find("@") == -1)
		ic.nickname += "@" + s.service;
	}

	if(h.getCapabs().count(hookcapab::conferencesaretemporary)) {
	    createconference(imcontact("", s.pname));

	} else if(c = cicq.addcontact(ic)) {
	    icqcontact::basicinfo cb = c->getbasicinfo();

	    if(h.getCapabs().count(hookcapab::channelpasswords))
		cb.zip = s.password;

	    cb.requiresauth = true;
	    c->setbasicinfo(cb);
	    c->setstatus(available);

	}
    }
}

void centericq::linkfeed() {
    icqcontact *c;
    imsearchparams s;

    s.pname = rss;
    s.checkfrequency = 120;

    if(face.finddialog(s, icqface::fsrss)) {
	if(c = cicq.addcontact(imcontact(0, rss))) {
	    icqcontact::workinfo wi = c->getworkinfo();
	    wi.homepage = s.url; // syndication url
	    c->setworkinfo(wi);

	    icqcontact::moreinfo mi = c->getmoreinfo();
	    mi.checkfreq = s.checkfrequency;
	    c->setmoreinfo(mi);

	    c->setnick(s.nick);
	    c->setdispnick(s.nick);
	    c->save();
	}
    }
}

void centericq::find() {
    static imsearchparams s;
    bool ret = true;
    icqcontact *c;

    while(ret) {
	if(ret = face.finddialog(s, icqface::fsuser)) {
	    switch(s.pname) {
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

		default:
		    ret = face.findresults(s);
		    break;
	    }
	}
    }
}

void centericq::userinfo(const imcontact &cinfo) {
    if(ischannel(cinfo)) {
	imsearchparams s;
	s.pname = cinfo.pname;
	s.room = cinfo.nickname;
	face.findresults(s, true);

    } else {
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
	    configstats.erase("sound");
	}

	if(clr != icqconf::rcdontchange) {
	    conf.setregcolor(clr);
	    unlink(conf.getconfigfname("colorscheme").c_str());
	    conf.loadcolors();
	    face.done();
	    face.init();
	    face.draw();
	    configstats.erase("colorscheme");
	}

	face.update();
    }

    return r;
}

void centericq::checkmail() {
    if(!getenv("MAIL"))
	return;

    struct stat st;
    int mcount = 0;
    char buf[512];
    string lastfrom, fname(getenv("MAIL"));
    bool prevempty, header;
    FILE *f;

    static int fsize = -1;
    static int oldcount = -1;

    bool lts, lo;
    conf.getlogoptions(lts, lo);

    if(conf.getmailcheck())
    if(!stat(fname.c_str(), &st)) 
    if(st.st_size != fsize && (st.st_mode & S_IFREG)) {

	if(fsize != -1 && st.st_size > fsize) {
	    /*
	    *
	    * I.e. only if mailbox has grown after the recent check.
	    *
	    */

	    if(f = fopen(fname.c_str(), "r")) {
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

	    if(mcount && (oldcount == -1 || (mcount > oldcount))) {
		const struct tm *lt;

		lt = localtime(&timer_current);

		if (lts)
		face.log(_("+ new mail arrived, %d messages"), mcount);
		else
		face.log(_("+ [%02d:%02d:%02d] new mail arrived, %d messages"),
		    lt->tm_hour, lt->tm_min, lt->tm_sec, mcount);

		face.log(_("+ last msg from %s"), lastfrom.c_str());

		clist.get(contactroot)->playsound(imevent::email);
	    }

	    oldcount = mcount;
	}

	fsize = st.st_size;

    } else if(st.st_mode & S_IFDIR) {
	DIR *d;
	struct dirent *e;
	string dname;
	time_t last = -1;

	dname = (string) fname + "/new/";

	if(!stat(dname.c_str(), &st) && st.st_mode & S_IFDIR)
	if(d = opendir(dname.c_str())) {
	    fname = "";

	    while((e = readdir(d)))
	    if(!stat((dname+e->d_name).c_str(), &st) && (st.st_mode & S_IFREG)) {
		mcount++;
		if(st.st_mtime >= last) {
		    last = st.st_mtime;
		    fname = dname + e->d_name;
		}
	    }

	    closedir(d);

	    if(f = fopen(fname.c_str(), "r")) {
		header = true;
		while(header && !feof(f)) {
		    freads(f, buf, 512);
		    if(!strncmp(buf, "From: ", 6)) {
			lastfrom = strim(buf+6);
			header = false; // we're done
		    }
		}
		fclose(f);
	    }

	    if(mcount && (oldcount == -1 || (mcount > oldcount)))  {
		const struct tm *lt;
		lt = localtime(&timer_current);

		if (lts)
		face.log(_("+ new mail arrived, %d messages"), mcount);
		else
		face.log(_("+ [%02d:%02d:%02d] new mail arrived, %d messages"),
		    lt->tm_hour, lt->tm_min, lt->tm_sec, mcount);

		face.log(_("+ last msg from %s"), lastfrom.c_str());
		clist.get(contactroot)->playsound(imevent::email);
	    }

	    oldcount = mcount;
	}
    }
}

void centericq::checkconfigs() {
    static const char *configs[] = {
	"sounds", "colorscheme", "actions", "external", "keybindings", 0
    };

    struct stat st;
    const char *p;

    for(int i = 0; p = configs[i]; i++) {
	if(stat(conf.getconfigfname(p).c_str(), &st))
	    st.st_mtime = 0;

	if(configstats.find(p) != configstats.end()) {
	    if(st.st_mtime != configstats[p]) {
		switch(i) {
		    case 0:
			conf.loadsounds();
			break;
		    case 1:
			endwin();
			initscr();
			conf.loadcolors();
			face.redraw();
			break;
		    case 2:
			conf.loadactions();
			break;
		    case 3:
			external.load();
			break;
		    case 4:
			conf.loadkeys();
			break;
		}

		face.log(_("+ the %s configuration was changed, reloaded"), p);
	    }
	}

	configstats[p] = st.st_mtime;
    }
}

void centericq::handlesignal(int signum) {
    int status, pid;

    switch(signum) {
	case SIGCHLD:
	    while((pid = wait3(&status, WNOHANG, 0)) > 0) {
		// In case the child was a nowait external action
		string sname = conf.getdirname() + "centericq-external-tmp." + i2str(pid);
		unlink(sname.c_str());
	    }
	    break;

	case SIGUSR1:
	    cicq.rereadstatus();
	    break;

	case SIGUSR2:
	    clist.load();
	    break;

	case SIGALRM:
	    break;

	case SIGTERM:
	    lst.save();
	    clist.save();
	    groups.save();
	    conf.save();
	    face.done();

	    signal(SIGTERM, 0);
	    kill(0, SIGTERM);
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

    for(pname = icq; pname != protocolname_size; pname++) {
	ia = conf.getourid(pname);

	if(!ia.empty()) {
	    char cst;
	    imstatus st;
	    string fname = conf.getconfigfname((string) "status-" + conf.getprotocolname(pname));
	    ifstream f(fname.c_str());

	    if(f.is_open()) {
		f >> cst, f.close(), f.clear();
		unlink(fname.c_str());

		for(st = offline; st != imstatus_size; st++) {
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
	    if(m->getcontact().pname == rss)
	    if(islivejournal(m->getcontact())) {
		imxmlevent *x = new imxmlevent(m->getcontact(), imevent::outgoing, "");

		string url = clist.get(m->getcontact())->getmoreinfo().homepage;
		int npos = text.rfind(url), id = 0;
		if(npos != -1) id = strtoul(text.c_str()+url.size()+npos, 0, 0);

		x->setfield("_eventkind", "comment");
		x->setfield("replyto", i2str(id));
		sendev = x;
	    }

	    if(conf.getquote()) text = quotemsg(text);
		else text = "";

	} else if(r == icqface::forward) {
	    text = fwdnote + text;
	}

	if(!sendev) {
	    sendev = new immessage(m->getcontact(), imevent::outgoing, text);
	}

    } else if(ev.gettype() == imevent::xml) {
	const imxmlevent *m = dynamic_cast<const imxmlevent *>(&ev);
	text = m->gettext();

	if(r == icqface::reply) {
	    if(conf.getquote()) text = quotemsg(text);
		else text = "";

	} else if(r == icqface::forward) {
	    text = fwdnote + text;
	    sendev = new immessage(m->getcontact(), imevent::outgoing, text);

	}

	if(!sendev) {
	    sendev = new imxmlevent(m->getcontact(), imevent::outgoing, text);
	}

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

    } else if(ev.gettype() == imevent::notification) {
	const imnotification *m = dynamic_cast<const imnotification *>(&ev);
	text = m->gettext();

	if(r == icqface::reply) {
	    if(conf.getquote()) text = quotemsg(text);
		else text = "";

	} else if(r == icqface::forward) {
	    text = fwdnote + text;
	}

	if(!sendev) {
	    sendev = new immessage(m->getcontact(), imevent::outgoing, text);
	}

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
	    if(r == icqface::forward)
		sendev->setcontact(imcontact());

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

    face.update();
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
		conf.execaction("openurl", m->geturl());
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

	while(c->hasevents() && !fin) {
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
	face.status("@");

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

icqcontact *centericq::addcontact(const imcontact &ic, bool reqauth) {
    icqcontact *c;
    int groupid = 0;

    if(c = clist.get(ic)) {
	if(c->inlist()) {
	    face.log(_("+ user %s is already on the list"), ic.totext().c_str());
	    return 0;
	}
    }

    imauthorization auth(ic, imevent::outgoing, imauthorization::Request, "");

    if(reqauth)
    if(!face.eventedit(auth))
	return 0;

    if(conf.getgroupmode() != icqconf::nogroups) {
	groupid = face.selectgroup(_("Select a group to add the user to"));
	if(!groupid) return 0;
    }

    if(!c) {
	if(ic.pname == rss) {
	    c = clist.addnew(imcontact(0, rss), false, groupid);
	    icqcontact::workinfo wi = c->getworkinfo();
	    wi.homepage = ic.nickname;
	    c->setworkinfo(wi);

	    if(!ic.nickname.empty()) {
		string bfeed = ic.nickname;
		getrword(bfeed, "/");
		c->setnick(getrword(bfeed, "/") + "@lj");
		c->setdispnick(c->getnick());
		gethook(ic.pname).sendnewuser(c);
	    }

	} else {
	    c = clist.addnew(ic, false, groupid, reqauth);
	}

    } else {
	c->includeintolist(groupid, reqauth);

    }

    if(reqauth) {
	em.store(auth);
    }

    face.log(_("+ %s has been added to the list"), c->getdesc().totext().c_str());
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

	    for(pname = icq; pname != protocolname_size; pname++) {
		abstracthook &hook = gethook(pname);

		if(hook.online()) {
		    hook.getsockets(rfds, wfds, efds, hsockfd);
		    online = true;
		}
	    }
	}

	tv.tv_sec = face.updaterequested() ? 1 : 20;
	tv.tv_usec = 0;

	select(hsockfd+1, &rfds, &wfds, &efds, &tv);

	if(FD_ISSET(0, &rfds)) {
	    keypressed = true;
	    time(&timer_keypress);
	} else {
	    for(pname = icq; pname != protocolname_size; pname++) {
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

    for(pname = icq; pname != protocolname_size; pname++) {
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
    protocolname pname;
    int paway, pna;
    bool fonline = false;

    time(&timer_current);

    /*
    *
    * Execute regular actions for all the hooks.
    *
    */

    for(pname = icq; pname != protocolname_size; pname++) {
	if(!conf.getourid(pname).empty() || (pname == rss)) {
	    abstracthook &hook = gethook(pname);

	    /*
	    *
	    * A hook's own action goes first.
	    *
	    */

	    hook.exectimers();

	    if(timer_current-reconnect[pname].timer > reconnect[pname].period) {
		/*
		*
		* Any need to try auto re-connecting?
		*
		*/

		if(!hook.logged()) {
		    reconnect[pname].timer = timer_current;

		    if(reconnect[pname].period < 180)
			reconnect[pname].period += reconnect[pname].period/2;

		    if(hook.online()) {
			hook.disconnect();

		    } else if(hook.getmanualstatus() != offline) {
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

	checkconfigs();
	conf.checkdiskspace();

	if(!conf.enoughdiskspace()) {
	    if(fonline) {
		for(pname = icq; pname != protocolname_size; pname++)
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
	checkmail();
	timer_checkmail = timer_current;
    }

    if(timer_current-timer_autosave > PERIOD_AUTOSAVE) {
	clist.save(false);
	timer_autosave = timer_current;
    }

    if(face.updaterequested())
    if(timer_current-timer_update > PERIOD_DISPUPDATE) {
	face.update();
	timer_update = timer_current;
    }
}

void centericq::createconference(const imcontact &ic) {
    int apid = 0;
    imcontact confid("", ic.pname);

    set<protocolname> ps;
    ps.insert(ic.pname);

    face.muins.clear();
    if(!ic.empty())
	face.muins.push_back(ic);

    if(face.multicontacts(_("Invite to conference.."), ps))
    if(!face.muins.empty()) {
	do {
	    confid.nickname = (string) "#" + conf.getourid(ic.pname).nickname + "-" + i2str(getpid() + ++apid);
	} while(clist.get(confid));

	icqcontact *nc = clist.addnew(confid);
	nc->setstatus(available);

	abstracthook &hook = gethook(ic.pname);
	hook.conferencecreate(nc, face.muins);
    }
}

void centericq::massmove() {
    int gid;
    vector<imcontact>::iterator im;
    icqcontact *c;

    face.muins.clear();

    if(face.multicontacts(_("Select contacts to move")))
    if(!face.muins.empty())
    if(gid = face.selectgroup(_("Mass move selected users to.."))) {
	for(im = face.muins.begin(); im != face.muins.end(); ++im) {
	    if(c = clist.get(*im)) {
		c->setgroupid(gid);
	    }
	}

	face.fillcontactlist();
	face.update();
    }
}

// ----------------------------------------------------------------------------

bool ischannel(const imcontact &cont) {
    if(cont.nickname.substr(0, 1) == "#")
    if(gethook(cont.pname).getCapabs().count(hookcapab::conferencing))
	return true;

    return false;
}

bool islivejournal(const icqcontact *c) {
    bool r = false;
#ifdef BUILD_LJ
    if(c->getnick().size() > 3)
	r = c->getnick().substr(c->getnick().size()-3) == "@lj";
#endif
    return r;
}

bool islivejournal(const imcontact &cont) {
    icqcontact *c = clist.get(cont);
    return c ? islivejournal(c) : false;
}

string up(string s) {
    string::iterator is = s.begin();
    while(is != s.end()) {
	*is = toupper(*is);
	++is;
    }

    return s;
}

string lo(string s) {
    string::iterator is = s.begin();
    while(is != s.end()) {
	*is = tolower(*is);
	++is;
    }

    return s;
}
