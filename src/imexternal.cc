/*
*
* centericq external actions handling class
* $Id: imexternal.cc,v 1.29 2005/01/18 23:20:17 konst Exp $
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

#include "imexternal.h"
#include "icqconf.h"
#include "abstracthook.h"
#include "eventmanager.h"
#include "imlogger.h"
#include "icqcontacts.h"
#include <sys/wait.h>

imexternal external;

imexternal::imexternal() {
}

imexternal::~imexternal() {
}

void imexternal::load() {
    ifstream f(conf.getconfigfname("external").c_str());
    action a;

    actions.clear();

    if(f.is_open()) {
	while(!f.eof()) {
	    a = action();

	    if(a.load(f)) {
		actions.push_back(a);
	    }
	}

	f.close();
    }
}

int imexternal::exec(const imevent &ev) {
    bool r;
    auto_ptr<imevent> p(ev.getevent());
    int n = exec(p.get(), r);
    return n;
}

int imexternal::exec(imevent *ev, bool &eaccept, int option) {
    vector<action>::iterator i;
    int r = 0, rl;
    imcontact c;

    eaccept = true;

    for(i = actions.begin(); i != actions.end(); ++i) {
	if(i->exec(ev, rl, option)) {
	    eaccept = eaccept && !rl;
	    r++;
	}
    }

    return r;
}

bool imexternal::execmanual(const imcontact &ic, int id, string &outbuf) {
    int res;
    bool r;

    if(r = (id < actions.size() && id >= 0)) {
	vector<action>::iterator ia = actions.begin()+id;
	r = ia->exec(ic, outbuf);
    }

    return r;
}

vector<pair<int, string> > imexternal::getlist(int options, protocolname pname) const {
    vector<pair<int, string> > r;
    vector<action>::const_iterator i;

    for(i = actions.begin(); i != actions.end(); ++i) {
	if(i->matches(options, pname))
	    r.push_back(make_pair(i-actions.begin(), i->getname()));
    }

    return r;
}

// ----------------------------------------------------------------------------

imexternal::action::action(): options(0), enabled(true) {
}

imexternal::action::~action() {
}

bool imexternal::action::exec(const imcontact &ic, string &outbuf) {
    bool r;
    char buf[512];
    abstracthook &hook = gethook(ic.pname);

    if(r = enabled)
    if(r = (options & aomanual)) {
	currentev = new immessage(ic, imevent::incoming, "");

	writescript();
	int result = execscript();

	sprintf(buf, _("executed external manual action %s, return code = %d"),
	    name.c_str(), result);

	outbuf = output;
	logger.putmessage(buf);
	delete currentev;
    }

    return r;
}

bool imexternal::action::exec(imevent *ev, int &result, int option) {
    bool r;
    char buf[512];
    abstracthook &hook = gethook(ev->getcontact().pname);

    r = enabled &&
	(find(event.begin(), event.end(), ev->gettype()) != event.end()) &&
	(find(proto.begin(), proto.end(), ev->getcontact().pname) != proto.end()) &&
	(find(status.begin(), status.end(), hook.getstatus()) != status.end())
    && ((option && (options & option)) || !option);

    if(r && !option) {
	r = !(options & aoprereceive) && !(options & aopresend) && !(options & aomanual);
    }

    if(r) {
	currentev = ev;

	writescript();
	result = execscript();

	sprintf(buf, _("executed external action %s, return code = %d"),
	    name.c_str(), result);

	logger.putmessage(buf);

	if(!option) {
	    respond();

	} else {
	    substitute();

	}
    }

    return r;
}

void imexternal::action::respond() {
    imevent *ev;

    if((options & aostdout) && !output.empty()) {
	ev = 0;

	switch(currentev->gettype()) {
	    case imevent::sms:
		ev = new imsms(currentev->getcontact(),
		    imevent::outgoing, output);
		break;
	    default:
		ev = new immessage(currentev->getcontact(),
		    imevent::outgoing, output);
		break;
	}

	if(ev) {
	    em.store(*ev);
	    delete ev;
	}
    }
}

void imexternal::action::substitute() {
    if((options & aostdout) && !output.empty()) {
	if(currentev->gettype() == imevent::message) {
	    immessage *m = static_cast<immessage *>(currentev);
	    *m = immessage(m->getcontact(), m->getdirection(), output);

	} else if(currentev->gettype() == imevent::url) {
	    imurl *m = static_cast<imurl *>(currentev);
	    *m = imurl(m->getcontact(), m->getdirection(), m->geturl(), output);

	} else if(currentev->gettype() == imevent::sms) {
	    imsms *m = static_cast<imsms *>(currentev);
	    *m = imsms(m->getcontact(), m->getdirection(), output, m->getphone());

	} else if(currentev->gettype() == imevent::authorization) {
	    imauthorization *m = static_cast<imauthorization *>(currentev);
	    *m = imauthorization(m->getcontact(), m->getdirection(), m->getauthtype(), output);

	}
    }
}

void imexternal::action::writescript() {
#ifdef HAVE_SSTREAM
    std::ostringstream tfname;
#else
    std::ostrstream tfname;
#endif
    ofstream f;
    int i = 0;

    do {
	tfname.clear();
	tfname << conf.getdirname() << "centericq-external-tmp." << dec << time(0)+i++;
#ifdef HAVE_SSTREAM
	sname = tfname.str();
#else
	sname = string(tfname.str(), tfname.pcount());
	tfname.freeze(false);
#endif
    } while(!access(sname.c_str(), F_OK));

    f.open(sname.c_str());

    if(f.is_open()) {
	if(code.substr(0, 2) != "#!")
	    f << "#!/bin/sh" << endl << endl;

	f << code << endl;
	f.close();
	chmod(sname.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
    }
}

int imexternal::action::execscript() {
    int inpipe[2], outpipe[2], pid;
    string text;
    char ch;
    icqcontact *c;
    fd_set rfds;
    int r = -1;
    struct sigaction sact, osact;

    memset(&sact, 0, sizeof(sact));
    sigaction(SIGCHLD, &sact, &osact);

    if(!pipe(inpipe) && !pipe(outpipe)) {
	pid = fork();

	if(!pid) {
	    if(c = clist.get(currentev->getcontact())) {
		setenv("EVENT_TYPE", geteventname(currentev->gettype()).c_str(), 1);
		setenv("EVENT_NETWORK", conf.getprotocolname(c->getdesc().pname).c_str(), 1);

		setenv("CONTACT_INFODIR", c->getdirname().c_str(), 1);
		setenv("CONTACT_UIN", i2str(c->getdesc().uin).c_str(), 1);

		if(c->getdesc().uin) setenv("CONTACT_NICK", c->getnick().c_str(), 1);
		    else setenv("CONTACT_NICK", c->getdesc().nickname.c_str(), 1);

		setenv("SENDER_UIN", getenv("CONTACT_UIN"), 1);
		setenv("SENDER_NICK", getenv("CONTACT_NICK"), 1);
		setenv("SENDER_INFODIR", getenv("CONTACT_INFODIR"), 1);

		c->save();
	    }

	    if(options & aonowait) {
		string nsname = conf.getdirname() + "centericq-external-tmp." + i2str(getpid());
		rename(sname.c_str(), nsname.c_str());
		sname = nsname;

		if(r = open("/dev/null", 0))
		    inpipe[1] = outpipe[0] = r;
	    }

	    dup2(inpipe[1], STDOUT_FILENO);
	    dup2(outpipe[0], STDIN_FILENO);

	    close(inpipe[1]);
	    close(inpipe[0]);
	    close(outpipe[0]);
	    close(outpipe[1]);

	    execl(sname.c_str(), sname.c_str(), 0);
	    _exit(0);
	} else {
	    close(outpipe[0]);
	    close(inpipe[1]);

	    if(options & aostdin) {
		text = currentev->gettext();
		write(outpipe[1], text.c_str(), text.size());
		close(outpipe[1]);
	    }

	    r = 0;
	    text = "";

	    if(!(options & aonowait)) {
		if(options & aostdout) {
		    while(1) {
			FD_ZERO(&rfds);
			FD_SET(inpipe[0], &rfds);

			if(select(inpipe[0]+1, &rfds, 0, 0, 0) < 0) break; else {
			    if(FD_ISSET(inpipe[0], &rfds)) {
				if(read(inpipe[0], &ch, 1) != 1) break; else {
				    text += ch;
				}
			    }
			}
		    }
		}

		waitpid(pid, &r, 0);
		unlink(sname.c_str());
	    }

	    close(inpipe[0]);
	    close(outpipe[1]);
	    output = text;
	}
    }

    sigaction(SIGCHLD, &osact, 0);
    return r;
}

bool imexternal::action::load(ifstream &f) {
    int pos, npos, i;
    string buf, sect, param, aname;
    bool action, wasexec = false;

    static const struct {
	const char *name;
	int option;
    } actions[] = {
	{ "pre-receive", aoprereceive },
	{ "pre-send", aopresend },
	{ "manual", aomanual },
	{ 0, 0 }
    };

    while(getconf(sect, buf, f, sect == "exec")) {
	if((npos = sect.find_first_of(" \t")) == -1)
	    npos = sect.size();

	aname = sect.substr(0, npos);

	if(!(action = aname == "action"))
	    for(i = 0; actions[i].name && !action; i++)
		action = aname == actions[i].name;

	if(action) {
	    if(wasexec) {
		f.seekg(pos, ios::beg);
		break;
	    }

	    if(npos != sect.size() && name.empty())
		name = leadcut(sect.substr(npos+1));

	    for(i = 0; actions[i].name; i++) {
		if(aname == actions[i].name) {
		    if(!(options & actions[i].option))
			options |= actions[i].option;
		    break;
		}
	    }

	    param = getword(buf);

	    if(param == "event") {
		while(!(param = getword(buf)).empty()) {
		    for(imevent::imeventtype et = imevent::message; et != imevent::imeventtype_size; et++) {
			if((param == geteventname(et))
			|| (param == "all")) {
			    event.insert(et);
			}
		    }
		}

	    } else if(param == "proto") {
		while(!(param = getword(buf)).empty()) {
		    for(protocolname pname = icq; pname != protocolname_size; pname++) {
			if((param == conf.getprotocolname(pname))
			|| (param == "all")) {
			    proto.insert(pname);
			}
		    }
		}

	    } else if(param == "status") {
		while(!(param = getword(buf)).empty()) {
		    if(param == "online") status.insert(available); else
		    if(param == "away") status.insert(away); else
		    if(param == "dnd") status.insert(dontdisturb); else
		    if(param == "na") status.insert(notavail); else
		    if(param == "occupied") status.insert(occupied); else
		    if(param == "ffc") status.insert(freeforchat); else
		    if(param == "invisible") status.insert(invisible); else
		    if(param == "all") {
			status.insert(available);
			status.insert(away);
			status.insert(dontdisturb);
			status.insert(notavail);
			status.insert(occupied);
			status.insert(freeforchat);
			status.insert(invisible);
		    }
		}

	    } else if(param == "options") {
		while(!(param = getword(buf)).empty()) {
		    if(param == "stdin") options |= aostdin; else
		    if(param == "stdout") options |= aostdout; else
		    if(param == "nowait") options |= aonowait;
		}
	    }
	} else if(sect == "exec") {
	    wasexec = true;
	    if(!code.empty()) code += "\n";
	    code += buf;
	}

	pos = f.tellg();
    }

    if(options & aomanual)
    if(!(options & aostdout))
	options |= aostdout;

    return wasexec;
}

bool imexternal::action::matches(int aoptions, protocolname apname) const {
    return (options & aoptions) && proto.count(apname);
}

string imexternal::action::geteventname(imevent::imeventtype et) {
    switch(et) {
	case imevent::message:
	    return "msg";
	case imevent::url:
	    return "url";
	case imevent::sms:
	    return "sms";
	case imevent::online:
	    return "online";
	case imevent::offline:
	    return "offline";
	case imevent::notification:
	    return "notification";
	case imevent::authorization:
	    return "auth";
	case imevent::contacts:
	    return "contacts";
	case imevent::file:
	    return "file";
    }

    return "";
}
