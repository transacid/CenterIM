/*
*
* centericq user mode list class
* $Id: icqmlist.cc,v 1.4 2001/08/17 19:11:59 konst Exp $
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
    morder.freeitem = &nothingfree;
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
			    add(new icqlistitem(unmime(p), uin, st));
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
    FILE *f;
    int i;
    icqlistitem *it;
    char buf[512];
    
    if(f = fopen(fname.c_str(), "w")) {
	for(i = 0; i < count; i++) {
	    it = (icqlistitem *) at(i);
	    fprintf(f, "%d\t%s\t%lu\n",
		it->getstatus(),
		mime(buf, it->getnick().empty() ? " " : it->getnick().c_str()),
		it->getuin());
	}
	fclose(f);
    }
}

void icqlist::fillmenu(verticalmenu *m, contactstatus ncs) {
    int i;
    icqlistitem *it;
    m->clear();
    morder.empty();

    for(i = 0; i < count; i++) {
	it = (icqlistitem *) at(i);

	if(it->getstatus() == ncs) {
	    m->additemf(" %15lu   %s", it->getuin(), it->getnick().c_str());
	    morder.add(it);
	}
    }
}

bool icqlist::inlist(unsigned int uin, contactstatus ncs) {
    int i;
    icqlistitem *it;

    for(i = 0; i < count; i++) {
	it = (icqlistitem *) at(i);
	if(it->getuin() == uin)
	if(it->getstatus() == ncs) return true;
    }

    return false;
}

void icqlist::del(unsigned int uin, contactstatus ncs) {
    int i;
    icqlistitem *it;

    for(i = 0; i < count; i++) {
	it = (icqlistitem *) at(i);

	if(it->getstatus() == ncs)
	if(it->getuin() == uin) {
	    remove(i);
	    break;
	}
    }
}

icqlistitem *icqlist::menuat(int pos) {
    return (icqlistitem *) morder.at(pos);
}
