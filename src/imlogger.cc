#include "imlogger.h"
#include "icqconf.h"
#include "icqcontacts.h"
#include "icqmlist.h"

imlogger logger;

imlogger::imlogger() {
}

imlogger::~imlogger() {
    if(f.is_open()) {
	f.close();
    }
}

bool imlogger::checkopen() {
    if(conf.getmakelog()) {
	if(!f.is_open()) {
	    f.open((conf.getdirname() + "log").c_str(), ios::app);

	    if(f.is_open())
		putmessage(_("events log started"));
	}

	return f.is_open();
    }

    return false;
}

void imlogger::putmessage(const string &text) {
    string towrite;
    time_t t;

    if(checkopen()) {
	time(&t);
	towrite = ctime(&t);
	towrite.resize(towrite.size()-1);
	towrite += ": " + text;

	f << towrite << endl;
    }
}

void imlogger::putevent(const imevent &ev) {
    char buf[512], *fmt;
    string text, name;
    icqcontact *c;

    if(checkopen()) {
	switch(ev.getdirection()) {
	    case imevent::outgoing: fmt = _("outgoing %s to %s"); break;
	    case imevent::incoming: fmt = _("incoming %s from %s"); break;
	}

	name = ev.getcontact().totext();
	if(c = clist.get(ev.getcontact())) {
	    name += " (" + c->getdispnick() + ")";
	}

	sprintf(buf, fmt, eventnames[ev.gettype()], name.c_str());
	text = buf;

	if(lst.inlist(ev.getcontact(), csignore)) {
	    text += string() + ", " + _("ignored");
	}

	putmessage(text);
    }
}

void imlogger::putonline(const imcontact &cont, const imstatus &oldst, const imstatus &st) {
    char buf[512], *fmt;
    string name;
    icqcontact *c;

    if(checkopen()) {
	if(st != oldst) {
	    if(oldst == offline) {
		fmt = _("%s went online, with status %s");
	    } else if(st == offline) {
		fmt = _("%s went offline");
	    } else {
		fmt = _("%s changed status from %s to %s");
	    }

	    name = cont.totext();
	    if(c = clist.get(cont)) {
		name += " (" + c->getdispnick() + ")";
	    }

	    sprintf(buf, fmt, name.c_str(), imstatus2name[st], imstatus2name[oldst]);
	    putmessage(buf);
	}
    }
}

void imlogger::putourstatus(const protocolname &pname, const imstatus &oldst,
const imstatus &st) {
    char *fmt, buf[512];

    if(checkopen()) {
	if(st != oldst) {
	    if(oldst == offline) {
		fmt = _("%s: went online, with status %s");
	    } else if(st == offline) {
		fmt = _("%s: went offline");
	    } else {
		fmt = _("changed our %s status from %s to %s");
	    }

	    sprintf(buf, fmt, conf.getprotocolname(pname).c_str(), imstatus2name[st], imstatus2name[oldst]);
	    putmessage(buf);
	}
    }
}
