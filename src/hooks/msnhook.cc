#include "msnhook.h"
#include "icqconf.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "accountmanager.h"

msnhook mhook;

#define TIMESTAMP maketm(d->hour-1, d->minute, d->day, d->month, d->year)

msnhook::msnhook() {
    manualstatus = conf.getstatus(msn);
    status = offline;
    fonline = false;
    timer_reconnect = 0;

    for(int i = MSN_RNG; i != MSN_NUM_EVENTS; i++) {
	msn_event[i] = 0;
    }

    MSN_RegisterCallback(MSN_MSG, &messaged);
    MSN_RegisterCallback(MSN_ILN, &statuschanged);
    MSN_RegisterCallback(MSN_NLN, &statuschanged);
    MSN_RegisterCallback(MSN_FLN, &statuschanged);
    MSN_RegisterCallback(MSN_AUTH, &authrequested);
    MSN_RegisterCallback(MSN_OUT, &disconnected);
//    MSN_RegisterCallback(MSN_MAIL, &mail_callback);

#ifdef DEBUG
    MSN_RegisterErrorOutput(&log);
#endif
}

msnhook::~msnhook() {
}

void msnhook::connect() {
    icqconf::imaccount account = conf.getourid(msn);

    face.log(_("+ [msn] connecting to the server"));

    if(!MSN_Login(account.nickname.c_str(), account.password.c_str(),
    DEFAULT_HOST, DEFAULT_PORT)) {
	face.log(_("+ [msn] logged in"));
	fonline = true;
	status = available;
//        setautostatus(manualstatus);
    } else {
	face.log(_("+ [msn] unable to connect to the server"));
    }

    time(&mhook.timer_reconnect);
}

void msnhook::disconnect() {
    if(online()) {
	disconnected(0);
    }
}

void msnhook::exectimers() {
    time_t tcurrent = time(0);

    if(logged()) {
    } else {
	if(tcurrent-timer_reconnect > PERIOD_RECONNECT) {
	    if(online() && !logged()) {
		disconnect();
	    } else if(manualstatus != offline) {
		if(!manager.isopen()) {
		    connect();
		}
	    }
	}
    }
}

void msnhook::main() {
    if(online()) {
	MSN_Main();
    }
}

void msnhook::getsockets(fd_set &fds, int &hsocket) const {
    vector<int> sockets = MSN_GetSockets();
    vector<int>::iterator i;

    for(i = sockets.begin(); i != sockets.end(); i++) {
	hsocket = max(hsocket, *i);
	FD_SET(*i, &fds);
    }
}

bool msnhook::isoursocket(fd_set &fds) const {
    vector<int> sockets = MSN_GetSockets();
    vector<int>::iterator i;

    for(i = sockets.begin(); i != sockets.end(); i++) {
	if(FD_ISSET(*i, &fds))
	    return true;
    }

    return false;
}

bool msnhook::online() const {
    return fonline;
}

bool msnhook::logged() const {
    return fonline;
}

bool msnhook::isconnecting() const {
    return false;
}

bool msnhook::enabled() const {
    return true;
}

unsigned long msnhook::sendmessage(const icqcontact *c, const string text) {
    return MSN_SendMessage(c->getnick().c_str(), text.c_str());
}

void msnhook::sendnewuser(const imcontact c) {
    if(online()) {
	MSN_AddContact(c.nickname.c_str());
    }
}

void msnhook::setautostatus(imstatus st) {
    static int stat2int[imstatus_size] = {
	USER_NLN,
	USER_FLN,
	USER_HDN,
	USER_FLN,
	USER_BSY,
	USER_BSY,
	USER_AWY,
	USER_IDL
    };

    if(st != offline) {
	if(getstatus() == offline) {
	    connect();
	} else {
	    MSN_ChangeState(stat2int[status = st]);
	}
    } else {
	if(getstatus() != offline) {
	    disconnect();
	}
    }
}

imstatus msnhook::getstatus() const {
    return status;
}

imstatus msnhook::msn2imstatus(int st) const {
    switch(st) {
	case USER_NLN:
	    return offline;

	case USER_HDN:
	    return invisible;

	case USER_BSY:
	case USER_PHN:
	    return occupied;

	case USER_IDL:
	case USER_BRB:
	case USER_AWY:
	    return away;

	case USER_LUN:
	    return notavail;

	case USER_FLN:
	    return offline;

	default:
	    return available;
    }
}

void msnhook::removeuser(const imcontact ic) {
    face.log(_("+ [msn] removing %s from the contacts"), ic.nickname.c_str());
    MSN_RemoveContact(ic.nickname.c_str());
}

// ----------------------------------------------------------------------------

void msnhook::messaged(void *data) {
    MSN_InstantMessage *d = (MSN_InstantMessage *) data;
    imcontact ic(d->sender, msn);

    if(strlen(d->msg))
    if(!lst.inlist(ic, csignore)) {
	hist.putmessage(ic, d->msg, HIST_MSG_IN, TIMESTAMP);
	icqcontact *c = clist.get(ic);

	if(c) {
	    if(c->getstatus() == offline) {
		c->setstatus(available);
	    }

	    c->setmsgcount(c->getmsgcount()+1);
	    c->playsound(EVT_MSG);
	    face.update();
	}
    }
}

void msnhook::statuschanged(void *data) {
    MSN_StatusChange *d = (MSN_StatusChange *) data;
    imcontact ic(d->handle, msn);
    icqcontact *c = clist.get(ic);

    if(!c) {
	c = clist.addnew(ic, false);
    }

    c->setstatus(mhook.msn2imstatus(d->newStatus));

    if(c->getstatus() != offline) {
	c->setlastseen();
    }

    face.update();
}

void msnhook::authrequested(void *data) {
}

void msnhook::disconnected(void *data) {
    int i;
    icqcontact *c;

    if(mhook.online()) {
	face.log(_("+ [msn] disconnected"));
	mhook.fonline = false;
    }

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);
	if(c->getdesc().pname == msn) {
	    c->setstatus(offline);
	}
    }

    time(&mhook.timer_reconnect);
    face.update();
}

void msnhook::log(const char *event, const char *cause) {
    face.log("%s: %s", event, cause);
}
