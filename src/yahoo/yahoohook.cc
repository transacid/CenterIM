#include "yahoohook.h"
#include "icqmlist.h"
#include "icqface.h"
#include "icqcontacts.h"

yahoohook::yahoohook() {
    fonline = false;
    timer_reconnect = 0;
    yahoo = 0;
    manualstatus = conf.getstatus(::yahoo);
}

yahoohook::~yahoohook() {
    conf.savestatus(::yahoo, manualstatus);
}

void yahoohook::init(const icqconf::imaccount account) {
    yahoo_options options;

    memset(&options, 0, sizeof(options));
    options.connect_mode = YAHOO_CONNECT_NORMAL;

    face.log("+ initializing %s engine",
	conf.getprotocolname(account.pname).c_str());

    yahoo = yahoo_init(account.nickname.c_str(),
	account.password.c_str(), &options);

    yahoo->yahoo_Disconnected = &disconnected;
    yahoo->yahoo_UserLogon = &userlogon;
    yahoo->yahoo_UserLogoff = &userlogoff;
    yahoo->yahoo_UserStatus = &userstatus;
    yahoo->yahoo_RecvBounced = &recvbounced;
    yahoo->yahoo_RecvMessage = &recvmessage;

#ifdef DEBUG
    yahoo->yahoo_Log = &log;
#else
    yahoo->yahoo_Log = 0;
#endif
}

void yahoohook::connect() {
    icqconf::imaccount acc = conf.getourid(::yahoo);

    if(!enabled()) init(acc);

    if(yahoo) {
	face.log(_("+ [yahoo] connecting to the server"));

	alarm(5);

	if(!yahoo_connect(yahoo)) {
	    face.log(_("+ [yahoo] unable to connect to the server"));
	    disconnected(yahoo);
	} else {
	    fonline = true;
	    yahoo_get_config(yahoo);
	    yahoo_cmd_logon(yahoo, ourstatus = YAHOO_STATUS_AVAILABLE);
	    face.log(_("+ [yahoo] logged in"));
	    clist.send();
	}
    }
}

void yahoohook::main() {
    if(yahoo) {
	yahoo_main(yahoo);
    }
}

int yahoohook::getsockfd() const {
    return (yahoo && fonline) ? yahoo->sockfd : 0;
}

void yahoohook::disconnect() {
    if(yahoo) {
	yahoo_cmd_logoff(yahoo);
	disconnected(yahoo);
    }
}

void yahoohook::exectimers() {
    time_t tcurrent = time(0);

    if(logged()) {
    } else {
	if(tcurrent-timer_reconnect > PERIOD_RECONNECT) {
	    if(online() && !logged()) {
		disconnect();
	    } else if(manualstatus != offline) {
		connect();
	    }
	}
    }
}

struct tm *yahoohook::timestamp() {
    time_t t = time(0);
    return localtime(&t);
}

unsigned long yahoohook::sendmessage(const icqcontact *c, const string atext) {
    string text = atext;
    string::iterator is;

    for(is = text.begin(); is != text.end(); is++)
	if(*is < 32) *is = ' ';

    yahoo_cmd_msg(yahoo,
	conf.getourid(::yahoo).nickname.c_str(),
	c->getdesc().nickname.c_str(),
	text.c_str());

    return 1;
}

bool yahoohook::online() const {
    return fonline;
}

bool yahoohook::logged() const {
    return fonline;
}

void yahoohook::sendnewuser(const imcontact ic) {
    char *group;

    if(yahoo)
    if(online()) {
	if(yahoo->buddies && yahoo->buddies[0]) {
	    group = yahoo->buddies[0]->group;
	} else {
	    group = "group";
	}

	if(!yahoo_isbuddy(yahoo, ic.nickname.c_str())) {
	    yahoo_add_buddy(yahoo, ic.nickname.c_str(),
		conf.getourid(::yahoo).nickname.c_str(), group, "");
	}
    }
}

void yahoohook::removeuser(const imcontact ic) {
    if(yahoo)
    if(online()) {
	yahoo_remove_buddy(yahoo, ic.nickname.c_str(),
	    conf.getourid(::yahoo).nickname.c_str(), "group", "");
    }
}

imstatus yahoohook::yahoo2imstatus(int status) const {
    imstatus st = offline;

    switch(status) {
	case YAHOO_STATUS_AVAILABLE:
	case YAHOO_STATUS_CUSTOM:
	    st = available;
	    break;
	case YAHOO_STATUS_BUSY:
	    st = occupied;
	    break;
	case YAHOO_STATUS_BRB:
	case YAHOO_STATUS_IDLE:
	case YAHOO_STATUS_ONPHONE:
	    st = away;
	    break;
	case YAHOO_STATUS_NOTATHOME:
	case YAHOO_STATUS_NOTATDESK:
	case YAHOO_STATUS_NOTINOFFICE:
	case YAHOO_STATUS_ONVACATION:
	case YAHOO_STATUS_OUTTOLUNCH:
	case YAHOO_STATUS_STEPPEDOUT:
	    st = notavail;
	    break;
	case YAHOO_STATUS_INVISIBLE:
	    st = invisible;
	    break;
	case -1:
	    st = offline;
	    break;
    }

    return st;
}

bool yahoohook::enabled() const {
    return (bool) yahoo;
}

void yahoohook::setstatus(imstatus st) {
    if(st == offline) {
	if(getstatus() != offline) {
	    disconnect();
	}
    } else {
	if(getstatus() == offline) {
	    connect();
	} else {
	}
    }

    manualstatus = st;
}

imstatus yahoohook::getstatus() const {
    return online() ? yahoo2imstatus(ourstatus) : offline;
}

// ----------------------------------------------------------------------------

void yahoohook::disconnected(yahoo_context *y) {
    int i;
    icqcontact *c;

    if(yhook.fonline) {
	face.log(_("+ [yahoo] disconnected from the network"));
	yhook.fonline = false;
    }

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);
	if(c->getdesc().pname == ::yahoo) {
	    c->setstatus(offline);
	}
    }

    time(&yhook.timer_reconnect);
}

void yahoohook::userlogon(yahoo_context *y, const char *nick) {
    yhook.userstatus(yhook.yahoo, nick, YAHOO_STATUS_AVAILABLE);
}

void yahoohook::userlogoff(yahoo_context *y, const char *nick) {
    yhook.userstatus(yhook.yahoo, nick, -1);
}

void yahoohook::userstatus(yahoo_context *y, const char *nick, int status) {
    imcontact ic(nick, ::yahoo);
    icqcontact *c = clist.get(ic);

    if(!c) {
	c = clist.addnew(ic, false);
    }

    c->setstatus(yhook.yahoo2imstatus(status));

    if(c->getstatus() != offline) {
	c->setlastseen();
    }

    face.update();
}

void yahoohook::recvbounced(yahoo_context *y, const char *nick) {
}

void yahoohook::recvmessage(yahoo_context *y, const char *nick, const char *msg) {
    imcontact ic(nick, ::yahoo);

    if(strlen(msg))
    if(!lst.inlist(ic, csignore)) {
	hist.putmessage(ic, msg, HIST_MSG_IN, timestamp());
	icqcontact *c = clist.get(ic);

	if(c) {
	    c->setmsgcount(c->getmsgcount()+1);
	    c->playsound(EVT_MSG);
	    face.update();
	}
    }
}

void yahoohook::log(yahoo_context *y, const char *msg) {
    face.log(msg);
}
