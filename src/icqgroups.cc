#include "icqgroups.h"
#include "icqconf.h"

icqgroups::icqgroups() {
}

icqgroups::~icqgroups() {
}

string icqgroups::getfname() const {
    return conf.getdirname() + "groups";
}

void icqgroups::load() {
    string buf;
    ifstream f;
    int gid;

    clear();
    f.open(getfname().c_str());

    if(f.is_open()) {
	while(!f.eof()) {
	    getstring(f, buf);
	    gid = atol(getword(buf).c_str());

	    if(gid && !buf.empty()) {
		push_back(icqgroup(gid, buf));
	    }
	    else if(gid) {
	        icqgroup *g = find(begin(), end(), gid);
	        if(g) g->changecollapsed();
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

    if(conf.enoughdiskspace()) {
	f.open(getfname().c_str());

	if(f.is_open()) {
	    for(i = begin(); i != end(); i++) {
		f << i->getid() << "\t" << i->getname() << endl;
	    }
            for(i = begin(); i != end(); i++)
                if(i->iscollapsed()) f << i->getid() << endl;
	    f.close();
	}
    }
}

int icqgroups::add(const string &aname) {
    int i;

    for(i = 1; find(begin(), end(), i) != end(); i++);
    push_back(icqgroup(i, aname));

    return i;
}

void icqgroups::remove(int gid) {
    iterator i = find(begin(), end(), gid);
    if(i != end()) {
	erase(i);
    }
}
