/*
*
* centericq user mode list class
* $Id: icqmlist.cc,v 1.5 2001/11/08 10:26:20 konst Exp $
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

icqlist::icqlist() {
}

icqlist::~icqlist() {
}
	
void icqlist::load() {
    string fname = (string) getenv("HOME") + "/.centericq/modelist";
    char buf[512], *p, *r, *rp;
    unsigned int uin;
    contactstatus st;
    FILE *f;
    
    if(f = fopen(fname.c_str(), "r")) {
	while(!feof(f)) {
	    freads(f, buf, 512);
	    strim(buf);

	    if(buf[0])
	    if(r = strpbrk(buf, " \t")) {
		st = (contactstatus) atoi(buf);
		if(p = strpbrk(r, " \t")) {
		    strim(p);
		    if(rp = strpbrk(p, " \t")) {
			*rp = 0;
			uin = strtoul(rp+1, 0, 0);

			if(uin) {
			    push_back(modelistitem(unmime(p),
				contactinfo(uin, contactinfo::icq), st));
			}
		    }
		}
	    }
	}
	fclose(f);
    }
}

void icqlist::save() {
    string fname = (string) getenv("HOME") + "/.centericq/modelist";
    vector<modelistitem>::iterator i;
    FILE *f;
    char buf[512];

    if(f = fopen(fname.c_str(), "w")) {
	for(i = begin(); i != end(); i++) {
	    fprintf(f, "%d\t%s\t%lu\n",
		i->getstatus(),
		mime(buf, i->getnick().empty() ? " " : i->getnick().c_str()),
		i->getdesc().uin);
	}
	fclose(f);
    }
}

void icqlist::fillmenu(verticalmenu *m, contactstatus ncs) {
    vector<modelistitem>::iterator i;
    m->clear();
    menucontents.clear();

    for(i = begin(); i != end(); i++) {
	if(i->getstatus() == ncs) {
	    m->additemf(" %15lu   %s", i->getdesc().uin, i->getnick().c_str());
	    menucontents.push_back(*i);
	}
    }
}

bool icqlist::inlist(const contactinfo cinfo, contactstatus ncs) const {
    vector<modelistitem>::const_iterator i;

    i = begin();
    while((i = find(i, end(), cinfo)) != end()) {
	if(i->getstatus() == ncs) return true;
	i++;
    }

    return false;
}

void icqlist::del(const contactinfo cinfo, contactstatus ncs) {
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

contactinfo modelistitem::getdesc() const {
    return cdesc;
}

void modelistitem::setstatus(contactstatus ncs) {
    cs = ncs;
}

bool modelistitem::operator == (const contactinfo &cinfo) const {
    return cdesc == cinfo;
}

bool modelistitem::operator != (const contactinfo &cinfo) const {
    return !(*this == cinfo);
}
