#include "imlogger.h"
#include "icqconf.h"
#include "icqcontacts.h"
#include "icqmlist.h"

static const char* imstatus2name(imstatus st) {
    switch(st) {
	case offline: return _("Offline");
	case available: return _("Online");
	case invisible: return _("Invisible");
	case freeforchat: return _("Free for chat");
	case dontdisturb: return _("DND");
	case occupied: return _("Occupied");
	case notavail: return _("N/A");
	case away: return _("Away");
    }

    return "";
};

const char *streventname(imevent::imeventtype type) {
    switch(type) {
	case imevent::message: return _("message");
	case imevent::url: return _("URL");
	case imevent::sms: return _("SMS");
	case imevent::authorization: return _("authorization");
	case imevent::email: return _("e-mail");
	case imevent::notification: return _("notification");
	case imevent::contacts: return _("contacts");
	case imevent::file: return _("files");
    }
    return "";
};

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

	sprintf(buf, fmt, streventname(ev.gettype()), name.c_str());
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
		fmt = _("%s changed status to %s from %s");
	    }

	    name = cont.totext();
	    if(c = clist.get(cont)) {
		name += " (" + c->getdispnick() + ")";
	    }

	    sprintf(buf, fmt, name.c_str(), imstatus2name(st), imstatus2name(oldst));
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
		fmt = _("changed our %s status to %s from %s");
	    }

	    sprintf(buf, fmt, conf.getprotocolname(pname).c_str(), imstatus2name(st), imstatus2name(oldst));
	    putmessage(buf);
	}
    }
}
