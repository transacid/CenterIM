#include "imcontact.h"

imcontact contactroot(0, icq);

imcontact::imcontact() {
}

imcontact::imcontact(unsigned long auin, protocolname apname) {
    uin = auin;
    pname = apname;
}

imcontact::imcontact(const string anick, protocolname apname) {
    nickname = anick;
    pname = apname;
}

bool imcontact::operator == (const imcontact &ainfo) const {
    return
	(ainfo.uin == uin) &&
	(ainfo.pname == pname) &&
	(ainfo.nickname == nickname);
}

bool imcontact::operator != (const imcontact &ainfo) const {
    return !(*this == ainfo);
}

bool imcontact::empty() const {
    return (!uin && pname == icq) || (nickname.empty() && pname == yahoo);
}
