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

bool imcontact::operator == (const imcontact &ainfo) const {
    bool r = (ainfo.uin == uin) && (ainfo.pname == pname);

    if(pname == irc) {
	int k;
	string nick1, nick2;

	for(k = 0; k < ainfo.nickname.size(); k++) nick1 += toupper(ainfo.nickname[k]);
	for(k = 0; k < nickname.size(); k++) nick2 += toupper(nickname[k]);

	r = r & (nick1 == nick2);
    } else {
	r = r & (ainfo.nickname == nickname);
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

string imcontact::getshortservicename() const {
    string r;

    switch(pname) {
	case icq: r = "I"; break;
	case yahoo: r = "Y"; break;
	case infocard: r = "C"; break;
    }

    return r;
}
