#include "icqgroup.h"
#include "icqgroups.h"

icqgroup::icqgroup(int aid, const string &aname) {
    id = aid;
    name = aname;
    collapsed = false;
}

icqgroup::~icqgroup() {
}

int icqgroup::getcount(bool countonline, bool countoffline) {
    int i, counter;

    for(i = counter = 0; i < clist.count; i++) {
	icqcontact *c = (icqcontact *) clist.at(i);

	if(c->getgroupid() == id) {
	    if(countoffline && c->getstatus() == offline) counter++;
	    if(countonline && c->getstatus() != offline) counter++;
	}
    }

    return counter;
}

void icqgroup::moveup() {
    if(id > 1) exchange(id-1);
}

void icqgroup::movedown() {
    if(id < groups.size()) exchange(id+1);
}

void icqgroup::exchange(int nid) {
    int i;
    icqcontact *c;
    vector<icqgroup>::iterator ig;

    ig = find(groups.begin(), groups.end(), nid);

    if(ig != groups.end()) {
	for(i = 0; i < clist.count; i++) {
	    c = (icqcontact *) clist.at(i);

	    if(c->getgroupid() == id) c->setgroupid(nid); else
	    if(c->getgroupid() == nid) c->setgroupid(id);
	}

	ig->id = id;
	id = nid;
    }
}
