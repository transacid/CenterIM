/*
*
* centericq user mode list class
* $Id: icqmlist.cc,v 1.8 2001/12/05 17:13:48 konst Exp $
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

#include "icqmlist.h"
#include "icqconf.h"

icqlist::icqlist() {
}

icqlist::~icqlist() {
}

const string icqlist::getfname() const {
    return conf.getdirname() + "/modelist";
}
	
void icqlist::load() {
    string buf, tok, nick;
    ifstream f;
    int i;

    f.open(getfname().c_str());

    if(f.is_open()) {
	while(!f.eof()) {
	    getstring(f, buf);
	    tok = getword(buf);

	    if(!tok.empty())
	    if(i = atoi(tok.c_str())) {
		nick = unmime(getword(buf));
		if(!nick.empty()) {
		    if(buf == "y") {
			lst.push_back(modelistitem(nick,
			    imcontact(nick, yahoo), (contactstatus) i));
		    } else {
			lst.push_back(modelistitem(nick,
			    imcontact(strtoul(buf.c_str(), 0, 0), icq), (contactstatus) i));
		    }
		}
	    }
	}
	f.close();
    }
}

void icqlist::save() {
    vector<modelistitem>::iterator i;
    ofstream f;

    f.open(getfname().c_str());

    if(f.is_open()) {
	for(i = begin(); i != end(); i++) {
	    f << (int) i->getstatus() << "\t";
	    f << mime(i->getnick()) << "\t";

	    switch(i->getdesc().pname) {
		case icq: f << i->getdesc().uin; break;
		case yahoo: f << "y"; break;
	    }

	    f << endl;
	}

	f.close();
    }
}

void icqlist::fillmenu(verticalmenu *m, contactstatus ncs) {
    vector<modelistitem>::iterator i;
    m->clear();
    menucontents.clear();

    for(i = begin(); i != end(); i++) {
	if(i->getstatus() == ncs) {
	    m->additem(" " + i->getnick());
	    menucontents.push_back(*i);
	}
    }
}

bool icqlist::inlist(const imcontact cinfo, contactstatus ncs) const {
    vector<modelistitem>::const_iterator i;

    i = begin();
    while((i = find(i, end(), cinfo)) != end()) {
	if(i->getstatus() == ncs) return true;
	i++;
    }

    return false;
}

void icqlist::del(const imcontact cinfo, contactstatus ncs) {
    vector<modelistitem>::iterator i;

    i = begin();
    while((i = find(i, end(), cinfo)) != end()) {
	if(i->getstatus() == ncs) {
	    erase(i);
	    break;
	}
	i++;
    }
}

modelistitem icqlist::menuat(int pos) const {
    return menucontents[pos];
}

// ----------------------------------------------------------------------------

const string modelistitem::getnick() const {
    return nick;
}

contactstatus modelistitem::getstatus() const {
    return cs;
}

imcontact modelistitem::getdesc() const {
    return cdesc;
}

void modelistitem::setstatus(contactstatus ncs) {
    cs = ncs;
}

bool modelistitem::operator == (const imcontact &cinfo) const {
    return cdesc == cinfo;
}

bool modelistitem::operator != (const imcontact &cinfo) const {
    return !(*this == cinfo);
}
