#include "icqgroups.h"

icqgroups::icqgroups() {
}

icqgroups::~icqgroups() {
}

const string icqgroups::getfname() const {
    return (string) getenv("HOME") + "/.centericq/groups";
}

void icqgroups::load() {
    string buf;
    ifstream f;
    int gid;

    clear();
    f.open(getfname().c_str());

    if(f.is_open()) {
	while(!f.eof()) {
	    getline(f, buf);
	    gid = atol(getword(buf).c_str());

	    if(gid && !buf.empty()) {
		push_back(icqgroup(gid, buf));
	    }
	}

	f.close();
    }

    if(find(begin(), end(), 1) == end()) {
	push_back(icqgroup(1, _("General")));
    }
}

void icqgroups::save() {
    ofstream f;
    iterator i;

    f.open(getfname().c_str());

    if(f.is_open()) {
	for(i = begin(); i != end(); i++) {
	    f << i->getid() << "\t" << i->getname() << endl;
	}

	f.close();
    }
}
