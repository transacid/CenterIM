/*
*
* centericq contact list class
* $Id: icqcontacts.cc,v 1.39 2002/11/22 16:08:17 konst Exp $
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

#include "icqcontacts.h"
#include "icqmlist.h"
#include "icqconf.h"
#include "icqgroups.h"
#include "abstracthook.h"

icqcontacts::icqcontacts() {
}

icqcontacts::~icqcontacts() {
}

icqcontact *icqcontacts::addnew(const imcontact &cinfo, bool notinlist) {
    icqcontact *c = new icqcontact(cinfo);

    switch(cinfo.pname) {
	case icq:
	    c->setnick(i2str(cinfo.uin));
	    c->setdispnick(i2str(cinfo.uin));
	    break;

	case yahoo:
	case msn:
	case aim:
	case irc:
	    c->setnick(cinfo.nickname);
	    c->setdispnick(cinfo.nickname);
	    break;
    }

    c->save();
    add(c);

    if(c)
    if(notinlist) {
	c->excludefromlist();
	if(cinfo.pname == icq)
	    gethook(cinfo.pname).sendnewuser(cinfo);
    } else {
	c->includeintolist();
    }

    return c;
}

void icqcontacts::load() {
    string tuname, fname;
    struct dirent *ent;
    struct stat st;
    DIR *d;
    FILE *f;
    icqcontact *c;
    imcontact cinfo;
    protocolname pname;

    empty();

    if(d = opendir(conf.getdirname().c_str())) {
	while(ent = readdir(d)) {
	    tuname = conf.getdirname() + ent->d_name;
	    stat(tuname.c_str(), &st);

	    if(S_ISDIR(st.st_mode)) {
		c = 0;
		pname = conf.getprotocolbyletter(ent->d_name[0]);

		switch(pname) {
		    case icq:
			c = new icqcontact(imcontact(atol(ent->d_name), pname));
			break;

		    case infocard:
			c = new icqcontact(imcontact(atol(ent->d_name+1), pname));
			break;

		    case yahoo:
		    case msn:
		    case aim:
		    case irc:
			c = new icqcontact(imcontact(ent->d_name+1, pname));
			c->setnick(ent->d_name+1);
			break;
		}

		if(c) {
		    clist.add(c);
		}
	    }
	}

	closedir(d);
    }

    checkdefault();

    if(!get(contactroot)) {
	add(new icqcontact(contactroot));
    }
}

void icqcontacts::checkdefault() {
    protocolname pname;
    icqcontact *c;
    int i;
    bool found;

    for(pname = icq; pname != protocolname_size; (int) pname += 1) {
	if(!conf.getourid(pname).empty()) {
	    for(i = 0, found = false; i < count && !found; i++) {
		c = (icqcontact *) at(i);
		if(c->getdesc() != contactroot) {
		    found = (c->getdesc().pname == pname);
		}
	    }

	    if(!found)
	    switch(pname) {
		case icq:
		    addnew(imcontact(17502151, pname), false);
		    break;
	    }
	}
    }
}

void icqcontacts::save() {
    int i;

    for(i = 0; i < count; i++) {
	icqcontact *c = (icqcontact *) at(i);

	if(c->inlist()) c->save(); else
	if(!c->getmsgcount()) {
	    string fname = c->getdirname() + "offline";
	    if(access(fname.c_str(), F_OK)) c->remove();
	}
    }
}

void icqcontacts::remove(const imcontact &cinfo) {
    int i;
    icqcontact *c;

    for(i = 0; i < count; i++) {
	c = (icqcontact *) at(i);

	if(cinfo == c->getdesc()) {
	    c->remove();
	    linkedlist::remove(i);
	    break;
	}
    }
}

icqcontact *icqcontacts::get(const imcontact &cinfo) {
    int i;
    icqcontact *c;

    for(i = 0; i < count; i++) {
	c = (icqcontact *) at(i);

	if(c->getdesc() == cinfo)
	    return (icqcontact *) at(i);
    }

    return 0;
}

int icqcontacts::clistsort(void *p1, void *p2) {
    icqcontact *c1 = (icqcontact *) p1, *c2 = (icqcontact *) p2;
    static char *sorder = SORT_CONTACTS;
    char s1, s2;
    bool makegroup;

    s1 = SORTCHAR(c1);
    s2 = SORTCHAR(c2);

    switch(conf.getgroupmode()) {
	case icqconf::group1:
	    makegroup = true;
	    break;
	case icqconf::group2:
	    makegroup = (s1 != '_' && s2 != '_') || (s1 == '_' && s2 == '_');
	    break;
	default:
	    makegroup = false;
	    break;
    }

    if(makegroup)
    if(!strchr("!N", s1) && !strchr("!N", s2))
    if(!c1->getmsgcount() && !c2->getmsgcount()) {
	if(c1->getgroupid() > c2->getgroupid()) return -1; else
	if(c1->getgroupid() < c2->getgroupid()) return 1;
    }

    if(s1 == s2) {
	if(*c1 > *c2) return -1; else return 1;
    } else {
	if(strchr(sorder, s1) > strchr(sorder, s2)) return -1; else return 1;
    }
}

void icqcontacts::order() {
    sort(&clistsort);
}

void icqcontacts::rearrange() {
    int i;
    icqcontact *c;

    for(i = 0; i < count; i++) {
	c = (icqcontact *) at(i);
	if(::find(groups.begin(), groups.end(), c->getgroupid()) == groups.end()) {
	    c->setgroupid(1);
	}
    }
}

void icqcontacts::setoffline(protocolname pname) {
    int i;
    icqcontact *c;

    for(i = 0; i < count; i++) {
	c = (icqcontact *) at(i);
	if(c->getdesc().pname == pname)
	    c->setstatus(offline);
    }
}

icqcontact *icqcontacts::getmobile(const string &anumber) {
    int i, pos;
    icqcontact *c;
    string cnumber, number;

    if(!anumber.empty()) {
	number = anumber;

	while((pos = number.find_first_not_of("0123456789")) != -1)
	    number.erase(pos, 1);

	for(i = 0; i < count; i++) {
	    c = (icqcontact *) at(i);
	    cnumber = c->getbasicinfo().cellular;

	    while((pos = cnumber.find_first_not_of("0123456789")) != -1)
		cnumber.erase(pos, 1);

	    if(number == cnumber)
		return c;
	}
    }

    return 0;
}

icqcontact *icqcontacts::getemail(const string &aemail) {
    int i;
    icqcontact *c;

    if(!aemail.empty()) {
	for(i = 0; i < count; i++) {
	    c = (icqcontact *) at(i);

	    if(aemail == c->getbasicinfo().email)
		return c;
	}
    }

    return 0;
}
