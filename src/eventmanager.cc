#include "eventmanager.h"
#include "icqmlist.h"
#include "icqconf.h"
#include "icqface.h"
#include "icqcontacts.h"

imeventmanager em;

void imeventmanager::store(const imevent &ev) {
    icqcontact *c;
    ofstream fhist;

    if(!lst.inlist(ev.getcontact(), csignore)) {
	c = clist.get(ev.getcontact());

	if(!c && !conf.getantispam()) {
	    c = clist.addnew(ev.getcontact());
	}

	if(c) {
	    fhist.open((c->getdirname() + "/history").c_str(), ios::app);

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
		}

		fhist.close();
	    }

	    c->setmsgcount(c->getmsgcount()+1);
	    c->playsound(ev.gettype());
	    face.relaxedupdate();
	}
    }
}

// if incoming, store to the history immediately
// if outgoing, return id

// void accept(const imcontact cont);

vector<imevent *> imeventmanager::getevents(const imcontact &cont, time_t lastread) const {
    imevent ev;
    ifstream fhist;
    vector<imevent *> r;
    icqcontact *c = clist.get(cont);

    if(c) {
	ev.setcontact(cont);
	fhist.open((c->getdirname() + "/history").c_str());

	if(fhist.is_open()) {
	    while(!fhist.eof()) {
		ev.read(fhist);

		if(ev.gettimestamp() >= lastread) {
		    if(ev.gettype() == imevent::message) {
			immessage *m = new immessage(ev);
			m->read(fhist);
			r.push_back(m);
		    } else if(ev.gettype() == imevent::url) {
			imurl *m = new imurl(ev);
			m->read(fhist);
			r.push_back(m);
		    } else if(ev.gettype() == imevent::sms) {
		    } else if(ev.gettype() == imevent::authorization) {
		    }
		}
	    }

	    fhist.close();
	} else {
	}
    }

    return r;
}

// for history, event viewing, etc

void imeventmanager::resend() {
}

// call every certain period of time

int imeventmanager::getunsentcount() const {
    return 0;
}
