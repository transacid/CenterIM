#include "yahoohook.h"
#include "icqmlist.h"
#include "icqface.h"
#include "icqcontacts.h"

yahoohook::yahoohook() {
    fonline = false;
    timer_reconnect = 0;
    yahoo = 0;
}

yahoohook::~yahoohook() {
}

void yahoohook::init(const string id, const string pass) {
    yahoo_options options;

    memset(&options, 0, sizeof(options));
    options.connect_mode = YAHOO_CONNECT_NORMAL;

    yahoo = yahoo_init((username = id).c_str(), pass.c_str(), &options);

    yahoo->yahoo_Disconnected = &disconnected;
    yahoo->yahoo_UserLogon = &userlogon;
    yahoo->yahoo_UserLogoff = &userlogoff;
    yahoo->yahoo_UserStatus = &userstatus;
    yahoo->yahoo_RecvBounced = &recvbounced;
    yahoo->yahoo_RecvMessage = &recvmessage;
}

void yahoohook::connect() {
    if(yahoo) {
	fonline = true;

	if(!yahoo_connect(yahoo)) {
	    face.log(_("+ unable to connect to the yahoo server"));
	    disconnected(yahoo);
	} else {
	    yahoo_get_config(yahoo);
	    yahoo_cmd_logon(yahoo, YAHOO_STATUS_AVAILABLE);
	    face.log(_("+ connected to the yahoo server"));
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
	fonline = false;
    }
}

void yahoohook::exectimers() {
    if(yahoo) {
	time_t tcurrent = time(0);

	if(logged()) {
	} else {
	    if(tcurrent-timer_reconnect > PERIOD_RECONNECT) {
		if(online() && !logged()) {
		    disconnect();
		} else {
		    connect();
		}
	    }
	}
    }
}

struct tm *yahoohook::timestamp() {
    time_t t = time(0);
    return localtime(&t);
}

unsigned long yahoohook::sendmessage(const icqcontact *c, const string text) {
    yahoo_cmd_msg(yahoo, username.c_str(), c->getdesc().nickname.c_str(), text.c_str());
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

    if(yahoo->buddies && yahoo->buddies[0]) {
	group = yahoo->buddies[0]->group;
    } else {
	group = "group";
    }

    yahoo_add_buddy(yahoo, ic.nickname.c_str(), username.c_str(), group, "");
}

void yahoohook::removeuser(const imcontact ic) {
    yahoo_remove_buddy(yahoo, ic.nickname.c_str(), username.c_str(), "group", "");
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
    }

    return st;
}

// ----------------------------------------------------------------------------

void yahoohook::disconnected(yahoo_context *y) {
    face.log(_("+ disconnected from yahoo network"));
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

bool yahoohook::enabled() const {
    return (bool) yahoo;
}
