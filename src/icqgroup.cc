/*
*
* centericq IM contacts group class
* $Id: icqgroup.cc,v 1.9 2005/01/30 22:21:34 konst Exp $
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

#include "icqgroup.h"
#include "icqgroups.h"
#include "abstracthook.h"

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
    int nid = id-1;

    while(nid > 0) {
	if(find(groups.begin(), groups.end(), nid) != groups.end()) break;
	nid--;
    }

    if(nid > 0) exchange(nid);
}

void icqgroup::movedown() {
    int nid = id+1;
    int fid = (groups.end()-1)->id;

    while(nid <= fid) {
	if(find(groups.begin(), groups.end(), nid) != groups.end()) break;
	nid++;
    }

    if(nid <= fid) exchange(nid);
}

void icqgroup::exchange(int nid) {
    int i;
    icqcontact *c;
    vector<icqgroup>::iterator ig;

    ig = find(groups.begin(), groups.end(), nid);

    if(ig != groups.end()) {
	for(i = 0; i < clist.count; i++) {
	    c = (icqcontact *) clist.at(i);

	    if(c->getgroupid() == id) c->setgroupid(nid, false); else
	    if(c->getgroupid() == nid) c->setgroupid(id, false);
	}

	ig->id = id;
	id = nid;
    }
}

void icqgroup::rename(const string &aname) {
    string oldname = name;
    name = aname;

    for(protocolname pname = icq; pname != protocolname_size; pname++)
	gethook(pname).renamegroup(oldname, name);
}
