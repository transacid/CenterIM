/*
*
* centericq contact list class
* $Id: icqcontacts.cc,v 1.12 2001/11/11 14:30:14 konst Exp $
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
#include "icqhook.h"
#include "icqgroups.h"

icqcontacts::icqcontacts() {
}

icqcontacts::~icqcontacts() {
}

icqcontact *icqcontacts::addnew(const imcontact cinfo, bool notinlist = true) {
    icqcontact *c = new icqcontact(cinfo);

    switch(cinfo.pname) {
	case icq:
	    c->setnick(i2str(cinfo.uin));
	    c->setdispnick(i2str(cinfo.uin));
	    c->save();
	    add(c);

	    if(notinlist) {
		c->excludefromlist();
	    } else {
		c->includeintolist();
	    }
	    break;

	case infocard:
	    c->save();
	    add(c);
	    break;
    }

    return c;
}

void icqcontacts::load() {
    string tname = (string) getenv("HOME") + "/.centericq/", tuname, fname;
    struct dirent *ent;
    struct stat st;
    DIR *d;
    FILE *f;
    icqcontact *c;
    imcontact cinfo;

    empty();

    if(d = opendir(tname.c_str())) {
	while(ent = readdir(d))
	if(strspn(ent->d_name+1, "0123456789") == strlen(ent->d_name)-1) {
	    tuname = tname + ent->d_name;
	    stat(tuname.c_str(), &st);

	    if(S_ISDIR(st.st_mode)) {
		c = 0;

		if(ent->d_name[0] == 'n') {
		    c = new icqcontact(imcontact(atol(ent->d_name+1),
			infocard));
		} else if(strchr("0123456789", ent->d_name[0])) {
		    c = new icqcontact(imcontact(atol(ent->d_name),
			icq));
		}

		if(c) clist.add(c);
	    }
	}

	closedir(d);
    }

    if(!count) {
	tuname = tname + "17502151";
	mkdir(tuname.c_str(), S_IREAD | S_IWRITE | S_IEXEC);
	add(new icqcontact(imcontact(17502151, icq)));
    }

    cinfo = contactroot;

    if(!get(cinfo)) {
	add(new icqcontact(cinfo));
    }
}

void icqcontacts::save() {
    int i;

    for(i = 0; i < count; i++) {
	icqcontact *c = (icqcontact *) at(i);

	if(c->inlist()) c->save(); else
	if(!c->getmsgcount()) {
	    string fname = c->getdirname() + "/offline";
	    if(access(fname.c_str(), F_OK)) c->remove();
	}
    }
}

void icqcontacts::send() {
    int i;
    icqcontact *c;

    icq_ContactClear(&icql);

    for(i = 0; i < count; i++) {
	c = (icqcontact *) at(i);

	if(c->getdesc().uin)
	switch(c->getdesc().pname) {
	    case icq:
		icq_ContactAdd(&icql, c->getdesc().uin);
		icq_ContactSetVis(&icql, c->getdesc().uin,
		    lst.inlist(c->getdesc(), csvisible) ? ICQ_CONT_VISIBLE :
		    lst.inlist(c->getdesc(), csinvisible) ? ICQ_CONT_INVISIBLE :
		    ICQ_CONT_NORMAL);
		break;
	}
    }

    icq_SendContactList(&icql);
    icq_SendVisibleList(&icql);
    icq_SendInvisibleList(&icql);
}

void icqcontacts::remove(const imcontact cinfo) {
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

icqcontact *icqcontacts::get(const imcontact cinfo) {
    int i;
    icqcontact *c;

    for(i = 0; i < count; i++) {
	c = (icqcontact *) at(i);

	if(c->getdesc() == cinfo)
	    return (icqcontact *) at(i);
    }

    return 0;
}

icqcontact *icqcontacts::getseq2(unsigned short seq2) {
    int i;
    icqcontact *c;

    for(i = 0; i < count; i++) {
	c = (icqcontact *) at(i);
	if(c->getseq2() == seq2) {
	    return (icqcontact *) at(i);
	}
    }

    return 0;
}

int icqcontacts::clistsort(void *p1, void *p2) {
    icqcontact *c1 = (icqcontact *) p1, *c2 = (icqcontact *) p2;
    static char *sorder = SORT_CONTACTS;
    char s1, s2;

    s1 = SORTCHAR(c1);
    s2 = SORTCHAR(c2);

    if(!strchr("!N", s1) && !strchr("!N", s2))
    if(!c1->getmsgcount() && !c2->getmsgcount())
    if(conf.getusegroups()) {
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
