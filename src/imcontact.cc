#include "imcontact.h"
#include "icqconf.h"

imcontact contactroot(0, icq);

imcontact::imcontact() {
}

imcontact::imcontact(unsigned long auin, protocolname apname) {
    uin = auin;
    pname = apname;
}

imcontact::imcontact(const string &anick, protocolname apname) {
    nickname = anick;
    pname = apname;
    uin = 0;
}

imcontact::imcontact(const icqcontact *c) {
    if(c) *this = c->getdesc();
}

bool imcontact::operator == (const imcontact &ainfo) const {
    int k;
    string nick1, nick2;
    bool r = (ainfo.uin == uin) && (ainfo.pname == pname);

    switch(pname) {
	case irc:
	case yahoo:
	    for(k = 0; k < ainfo.nickname.size(); k++) nick1 += toupper(ainfo.nickname[k]);
	    for(k = 0; k < nickname.size(); k++) nick2 += toupper(nickname[k]);

	    r = r & (nick1 == nick2);
	    break;

	default:
	    r = r & (ainfo.nickname == nickname);
	    break;
    }

    return r;
}

bool imcontact::operator != (const imcontact &ainfo) const {
    return !(*this == ainfo);
}

bool imcontact::operator == (protocolname apname) const {
    return apname == pname;
}

bool imcontact::operator != (protocolname apname) const {
    return !(*this == apname);
}

bool imcontact::empty() const {
    return (!uin && pname == icq) || (nickname.empty() && pname == yahoo);
}

string imcontact::totext() const {
    string r;

    if(*this == contactroot) {
	r = "[self]";
    } else {
	switch(pname) {
	    case icq:
	    case infocard:
		r = "[" + conf.getprotocolname(pname) + "] " + i2str(uin);
		break;

	    default:
		r = "[" + conf.getprotocolname(pname) + "] " + nickname;
		break;
	}
    }

    return r;
}
