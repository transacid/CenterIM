#include "yahoohook.h"
#include "icqmlist.h"
#include "icqface.h"
#include "icqcontacts.h"

yahoohook::yahoohook() {
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
    if(!yahoo_connect(yahoo)) {
	disconnected(yahoo);
    } else {
	yahoo_get_config(yahoo);
	yahoo_cmd_logon(yahoo, YAHOO_STATUS_AVAILABLE);
    }
}

void yahoohook::sendmessage(const string nick, const string msg) {
    yahoo_cmd_msg(yahoo, username.c_str(), nick.c_str(), msg.c_str());
}

void yahoohook::main() {
    yahoo_main(yahoo);
}

int yahoohook::getsockfd() const {
    return yahoo->sockfd;
}

void yahoohook::disconnect() {
    yahoo_cmd_logoff(yahoo);
}

void yahoohook::exectimers() {
}

struct tm *yahoohook::timestamp() {
    time_t t = time(0);
    return localtime(&t);
}

// ----------------------------------------------------------------------------

void yahoohook::disconnected(yahoo_context *y) {
    face.log(_("+ disconnected from yahoo network"));
}

void yahoohook::userlogon(yahoo_context *y, const char *nick) {
}

void yahoohook::userlogoff(yahoo_context *y, const char *nick) {
}

void yahoohook::userstatus(yahoo_context *y, const char *nick) {
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
