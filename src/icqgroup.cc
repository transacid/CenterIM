#include "icqgroup.h"

icqgroup::icqgroup(int aid, const string aname) {
    id = aid;
    name = aname;
}

icqgroup::~icqgroup() {
}

bool icqgroup::operator==(int aid) const {
    return id == aid;
}

bool icqgroup::operator!=(int aid) const {
    return !(*this == aid);
}
