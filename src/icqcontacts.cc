/*
*
* centericq contact list class
* $Id: icqcontacts.cc,v 1.16 2001/11/21 14:35:56 konst Exp $
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

#include "icqhook.h"
#include "yahoohook.h"

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
	    break;

	case yahoo:
	    c->setnick(cinfo.nickname);
	    c->setdispnick(cinfo.nickname);
	    c->save();
	    add(c);
	    break;

	case infocard:
	    c->save();
	    add(c);
	    break;
    }

    if(c)
    if(notinlist) {
	c->excludefromlist();
    } else {
	c->includeintolist();
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
	while(ent = readdir(d)) {
	    tuname = tname + ent->d_name;
	    stat(tuname.c_str(), &st);

	    if(S_ISDIR(st.st_mode)) {
		c = 0;

		switch(ent->d_name[0]) {
		    case 'n':
			c = new icqcontact(imcontact(atol(ent->d_name+1),
			    infocard));
			break;
		    case 'y':
			c = new icqcontact(imcontact(ent->d_name+1, yahoo));
			c->setnick(ent->d_name+1);
			break;
		    case '0':
		    case '1':
		    case '2':
		    case '3':
		    case '4':
		    case '5':
		    case '6':
		    case '7':
		    case '8':
		    case '9':
			c = new icqcontact(imcontact(atol(ent->d_name), icq));
			break;
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

	tuname = tname + "ythekonst";
	mkdir(tuname.c_str(), S_IREAD | S_IWRITE | S_IEXEC);
	add(new icqcontact(imcontact("thekonst", yahoo)));
    }

    if(!get(contactroot)) {
	add(new icqcontact(contactroot));
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

    if(ihook.online()) {
	icq_ContactClear(&icql);
    }

    for(i = 0; i < count; i++) {
	c = (icqcontact *) at(i);

	if(c->getdesc() != contactroot)
	switch(c->getdesc().pname) {
	    case icq:
		if(ihook.online()) {
		    icq_ContactAdd(&icql, c->getdesc().uin);
		    icq_ContactSetVis(&icql, c->getdesc().uin,
			lst.inlist(c->getdesc(), csvisible) ? ICQ_CONT_VISIBLE :
			lst.inlist(c->getdesc(), csinvisible) ? ICQ_CONT_INVISIBLE :
			ICQ_CONT_NORMAL);
		}
		break;
	    case yahoo:
//                yhook.sendnewuser(c->getdesc());
		break;
	}
    }

    if(ihook.online()) {
	icq_SendContactList(&icql);
	icq_SendVisibleList(&icql);
	icq_SendInvisibleList(&icql);
    }
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
