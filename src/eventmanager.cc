#include "eventmanager.h"
#include "icqmlist.h"
#include "icqconf.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "abstracthook.h"
#include "imlogger.h"
#include "imexternal.h"

imeventmanager em;

imeventmanager::imeventmanager() {
    unsent = 0;
}

imeventmanager::~imeventmanager() {
}

void imeventmanager::store(const imevent &ev) {
    icqcontact *c;
    bool proceed;

    logger.putevent(ev);

    if(ev.getdirection() == imevent::incoming) {
	if(!lst.inlist(ev.getcontact(), csignore)) {
	    c = clist.get(ev.getcontact());

	    if(!c) {
		proceed = !conf.getantispam();
		proceed = (ev.gettype() == imevent::authorization) || proceed;

		if(proceed)
		    c = clist.addnew(ev.getcontact());
	    }

	    if(c) {
		eventwrite(ev, history);

		c->setmsgcount(c->getmsgcount()+1);
		c->playsound(ev.gettype());

		external.exec(ev);

		face.relaxedupdate();
	    }
	}
    } else if(ev.getdirection() == imevent::outgoing) {
	abstracthook &hook = gethook(ev.getcontact().pname);

	if(hook.logged()) {
	    if(hook.send(ev)) {
		eventwrite(ev, history);
	    } else {
		eventwrite(ev, offline);
	    }
	} else {
	    eventwrite(ev, offline);
	}
    }
}

// if incoming, store to the history immediately
// if outgoing, return id

// void accept(const imcontact cont);

vector<imevent *> imeventmanager::getevents(const imcontact &cont, time_t lastread) const {
    ifstream fhist;
    imevent *rev;
    vector<imevent *> r;
    icqcontact *c = clist.get(cont);

    if(c) {
	fhist.open((c->getdirname() + "history").c_str());

	if(fhist.is_open()) {
	    if(lastread) {
		fhist.seekg(c->gethistoffset(), ios::beg);
	    }

	    while(!fhist.eof()) {
		rev = eventread(fhist);

		if(rev) {
		    if(rev->gettimestamp() > lastread) {
			rev->setcontact(cont);
			r.push_back(rev);
		    } else {
			delete rev;
		    }
		}
	    }

	    fhist.close();
	} else {
	}
    }

    return r;
}

void imeventmanager::eventwrite(const imevent &ev, eventwritemode mode) {
    ofstream fhist;
    string fname, lockfname;
    icqcontact *c;

    if(c = clist.get(ev.getcontact())) {
	switch(mode) {
	    case history:
		fname = c->getdirname() + "history";
		break;
	    case offline:
		fname = c->getdirname() + "offline";
		unsent++;
		face.relaxedupdate();
		break;
	}

	setlock(fname);

	fhist.open(fname.c_str(), ios::app);

	if(fhist.is_open()) {
	    if(ev.gettype() == imevent::message) {
		const immessage *m = static_cast<const immessage *>(&ev);
		m->write(fhist);
	    } else if(ev.gettype() == imevent::url) {
		const imurl *m = static_cast<const imurl *>(&ev);
		m->write(fhist);
	    } else if(ev.gettype() == imevent::sms) {
		const imsms *m = static_cast<const imsms *>(&ev);
		m->write(fhist);
	    } else if(ev.gettype() == imevent::authorization) {
		const imauthorization *m = static_cast<const imauthorization *>(&ev);
		m->write(fhist);
	    } else if(ev.gettype() == imevent::email) {
		const imemail *m = static_cast<const imemail *>(&ev);
		m->write(fhist);
	    }

	    fhist.close();
	}

	releaselock(fname);
    }
}

imevent *imeventmanager::eventread(ifstream &f) const {
    imevent *rev, ev;

    ev.read(f);

    if(rev = ev.getevent()) {
	rev->read(f);
    }

    return rev;
}

void imeventmanager::resend() {
    icqcontact *c;
    imevent *rev;
    string fname, tfname;
    ifstream f;
    int i;

    unsent = 0;

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	fname = c->getdirname() + "offline";
	tfname = fname + "_";

	if(!access(fname.c_str(), F_OK))
	if(!rename(fname.c_str(), tfname.c_str())) {
	    f.open(tfname.c_str());

	    if(f.is_open()) {
		while(!f.eof()) {
		    rev = eventread(f);
		    rev->setcontact(c->getdesc());

		    if(rev) {
			if(rev->gettimestamp() > PERIOD_RESEND) {
			    store(*rev);
			} else {
			    eventwrite(*rev, offline);
			}

			delete rev;
		    }
		}

		f.close();
		f.clear();
	    }

	    unlink(tfname.c_str());
	}
    }
}

int imeventmanager::getunsentcount() const {
    return unsent;
}

void imeventmanager::setlock(const string fname) const {
    string lockfname = fname + ".lock";
    int ntries = 0;
    ofstream f;

    while(!access(lockfname.c_str(), F_OK) && (ntries < 10)) {
	usleep(10);
	ntries++;
    }

    f.open(lockfname.c_str());
    if(f.is_open()) f.close();
}

void imeventmanager::releaselock(const string fname) const {
    string lockfname = fname + ".lock";
    unlink(lockfname.c_str());
}
