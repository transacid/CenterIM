/*
*
* centericq IM contacts group listing class
* $Id: icqgroups.cc,v 1.12 2003/11/05 14:54:27 konst Exp $
*
* Copyright (C) 2001 by Konstantin Klyagin <konst@konst.org.ua>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at
* your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
* USA
*
*/

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
    vector<icqgroup>::iterator ig;

    clear();
    f.open(getfname().c_str());

    if(f.is_open()) {
	while(!f.eof()) {
	    getstring(f, buf);
	    gid = atol(getword(buf).c_str());

	    if(gid) {
		if(!buf.empty()) {
		    push_back(icqgroup(gid, buf));
		} else {
		    ig = find(begin(), end(), gid);
		    if(ig != end()) ig->changecollapsed();
		}
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
	    for(i = begin(); i != end(); ++i) {
		f << i->getid() << "\t" << i->getname() << endl;
	    }
	    for(i = begin(); i != end(); ++i)
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

string icqgroups::getname(int gid) const {
    string r;
    const_iterator i = find(vector<icqgroup>::begin(), vector<icqgroup>::end(), gid);
    if(i != vector<icqgroup>::end()) r = i->getname();

    return r;
}
