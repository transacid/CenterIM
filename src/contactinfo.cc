#include "contactinfo.h"

contactinfo contactroot(0, contactinfo::icq);

contactinfo::contactinfo() {
}

contactinfo::contactinfo(unsigned long auin, idtype atype) {
    uin = auin;
    type = atype;
}

bool contactinfo::operator == (const contactinfo &ainfo) const {
    return (ainfo.uin == uin) && (ainfo.type == type);
}

bool contactinfo::operator != (const contactinfo &ainfo) const {
    return !(*this == ainfo);
}

bool contactinfo::empty() const {
    return uin == 0;
}
