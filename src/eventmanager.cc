/*
*
* centerim event manager class
* $Id: eventmanager.cc,v 1.27 2005/08/28 01:33:21 konst Exp $
*
* Copyright (C) 2001,2002 by Konstantin Klyagin <k@thekonst.net>
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

#include "eventmanager.h"
#include "icqmlist.h"
#include "icqconf.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "abstracthook.h"
#include "imlogger.h"
#include "imexternal.h"
#include "captcha.h"

imeventmanager em;

imeventmanager::imeventmanager(): unsent(0), lastevent(0), recentlysent(0) {
}

imeventmanager::~imeventmanager() {
}

void imeventmanager::store(const imevent &cev) {
    icqcontact *c;
    bool proceed;

    auto_ptr<imevent> icev(cev.getevent());
    imevent &ev = *icev.get();

    static int preoptions[imevent::imdirection_size] = {
	imexternal::aoprereceive,
	imexternal::aopresend
    };

    external.exec(&ev, proceed, preoptions[ev.getdirection()]);

    if(!proceed)
	return;

    if(ev.getdirection() == imevent::incoming) {
	if(!lst.inlist(ev.getcontact(), csignore)) {
	    logger.putevent(ev);

	    c = clist.get(ev.getcontact());

	    if(!c) {
		proceed = !conf->getantispam() || 
			((ev.gettype() == imevent::authorization) && !conf->getdropauthreq()) ||
			conf->getusingcaptcha();

		if(proceed) {
		    c = clist.addnew(ev.getcontact());
		    /* turing test */
		    if (conf->getdebug()) face.log("captcha: enabled = %i", conf->getusingcaptcha());
		    if (conf->getusingcaptcha()) {
			if (conf->getdebug()) face.log("captcha: start");
		        /* If the turing test was failed or not completed, remove
			 * contact (WE DONT WANT YOU!)
			 * Note tat below, we always remove the contact if it is not verified */
			if (!conf->thecaptcha.docaptcha(ev.getcontact())) {
			    if (conf->getdebug()) face.log("captcha: starting a new turing test with: %s", ev.getcontact().totext().c_str());
			    /* have not started the turing test, so send our greeter and message */
			    store(immessage(
				c->getdesc(), imevent::outgoing, 
				conf->getcaptchagreet() + "\n" +
				conf->thecaptcha.getcaptchaquestion(ev.getcontact())));
			    clist.remove(c->getdesc());
			    c = 0;
			} else {
			    if(conf->getdebug()) face.log("captcha: got an answer: %s", ev.gettext().c_str());
			    /* have started the test, now must verify the answer */
			    if (conf->thecaptcha.checkcaptcha(ev.getcontact(), ev.gettext())) {
			        if (conf->getdebug()) face.log("captcha: received correct answer");
			        /* good, a human */
				store(immessage(
				    c->getdesc(), imevent::outgoing, 
				    conf->getcaptchasuccess()));
				conf->thecaptcha.donecaptcha(ev.getcontact());
			    } else {
			        if (conf->getdebug()) face.log("captcha: received intorrent answer");
				/* bad, a computer (or very dumb human :) */
				store(immessage(
				    c->getdesc(), imevent::outgoing, 
				    conf->getcaptchafailure() + "\n" +
				    conf->thecaptcha.getcaptchaquestion(ev.getcontact())));
			        clist.remove(c->getdesc());
			        c = 0;
			    }
			}
			if (conf->getdebug()) face.log("captcha: end");
		    }
		} else {
		    face.log("Dropped an authorization request from %s", ev.getcontact().totext().c_str());
		}
	    }
	    
	    if (c) {
	    	face.xtermtitle(_("event from %s (%s)"), cev.getcontact().totext().c_str(), c->getdispnick().c_str());
	        eventwrite(ev, history);
		
	        c->sethasevents(true);
	        c->playsound(ev.gettype());

	        external.exec(ev);

	        face.relaxedupdate();
	    }
	}
    } else if(ev.getdirection() == imevent::outgoing) {
	abstracthook *hook;

	// All the SMSes are to be sent through the icqhook class..

	if(ev.gettype() == imevent::sms) {
	    hook = &gethook(icq);
	} else {
	    hook = &gethook(ev.getcontact().pname);
	}

	// Some self-flood protection..

	if(time(0)-lastevent > PERIOD_ATONCE)
	    recentlysent = 0;

	proceed = (hook->logged() || ev.getcontact().pname == rss)
	    && recentlysent < MAX_ATONCE;

	// 3.. 2.. 1.. LAUNCH!

	if(proceed) {
	    if(hook->send(ev)) {
		eventwrite(cev, history);
		logger.putevent(cev);
		face.xtermtitle();
		time(&lastevent);
		recentlysent++;
	    } else {
		eventwrite(cev, offline);
	    }
	} else {
	    eventwrite(cev, offline);
	}
    }
}

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
			if(rev->getdirection() == imevent::outgoing && c->receivedAcks.count(rev->gettimestamp()))
				rev->setack(true);
			else if(rev->getdirection() == imevent::incoming && c->sentAcks.count(rev->gettimestamp()))
				rev->setack(true);
			else
				rev->setack(false);
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
	    ev.write(fhist);
	    fhist.close();
	}

	releaselock(fname);
    }
}

imevent *imeventmanager::eventread(ifstream &f) const {
    imevent *rev, ev(f);

    if(rev = ev.getevent()) {
	rev->read(f);

	if(rev->gettype() == imevent::imeventtype_size) {
	    delete rev;
	    rev = 0;
	}
    }

    return rev;
}

void imeventmanager::resend() {
    icqcontact *c;
    imevent *rev;
    string fname, tfname;
    ifstream f;
    int i;

    for(unsent = i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	fname = c->getdirname() + "offline";
	tfname = fname + "_";

	if(!access(fname.c_str(), F_OK))
	if(!rename(fname.c_str(), tfname.c_str())) {
	    f.open(tfname.c_str());

	    if(f.is_open()) {
		while(!f.eof()) {
		    rev = eventread(f);

		    if(rev) {
			rev->setcontact(c->getdesc());

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

void imeventmanager::setlock(const string &fname) const {
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

void imeventmanager::releaselock(const string &fname) const {
    string lockfname = fname + ".lock";
    unlink(lockfname.c_str());
}
