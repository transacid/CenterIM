#include "icqgroup.h"

icqgroup::icqgroup(int aid, const string &aname) {
    id = aid;
    name = aname;
    collapsed = false;
}

icqgroup::~icqgroup() {
}

bool icqgroup::operator == (int aid) const {
    return id == aid;
}

bool icqgroup::operator != (int aid) const {
    return !(*this == aid);
}

void icqgroup::changecollapsed() {
    if(collapsed == false) { collapsed = true; }
    else { collapsed = false; }
}

int icqgroup::getcount(bool countonline, bool countoffline) {
    int counter = 0;
    for(int i = 0; i < clist.count; i++) {
        icqcontact *c = (icqcontact *) clist.at(i);
        if(c->getgroupid() == id) {
            if(countoffline && c->getstatus() == offline) counter++;
            if(countonline && c->getstatus() != offline) counter++;
        }
    }
    return counter;
}
