/*
*
* centericq IM contact basic info class
* $Id: imcontact.cc,v 1.15 2002/11/30 09:30:11 konst Exp $
*
* Copyright (C) 2002 by Konstantin Klyagin <konst@konst.org.ua>
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

#include "imcontact.h"
#include "icqconf.h"

imcontact contactroot(0, icq);

imcontact::imcontact() {
}

imcontact::imcontact(unsigned long auin, protocolname apname) {
    uin = auin;
    pname = apname;
}

imcontact::imcontact(const string &anick, protocolname apname) {
    nickname = anick;
    pname = apname;
    uin = 0;
}

imcontact::imcontact(const icqcontact *c) {
    if(c) *this = c->getdesc();
}

bool imcontact::operator == (const imcontact &ainfo) const {
    int k;
    string nick1, nick2;
    bool r = (ainfo.uin == uin) && (ainfo.pname == pname);

    switch(pname) {
	case irc:
	case yahoo:
	    for(k = 0; k < ainfo.nickname.size(); k++) nick1 += toupper(ainfo.nickname[k]);
	    for(k = 0; k < nickname.size(); k++) nick2 += toupper(nickname[k]);

	    r = r & (nick1 == nick2);
	    break;

	default:
	    r = r & (ainfo.nickname == nickname);
	    break;
    }

    return r;
}

bool imcontact::operator != (const imcontact &ainfo) const {
    return !(*this == ainfo);
}

bool imcontact::operator == (protocolname apname) const {
    return apname == pname;
}

bool imcontact::operator != (protocolname apname) const {
    return !(*this == apname);
}

bool imcontact::empty() const {
    return (!uin && pname == icq) || (nickname.empty() && pname == yahoo);
}

string imcontact::totext() const {
    string r;

    if(*this == contactroot) {
	r = "[self]";
    } else {
	switch(pname) {
	    case icq:
	    case infocard:
		r = "[" + conf.getprotocolname(pname) + "] " + i2str(uin);
		break;

	    default:
		r = "[" + conf.getprotocolname(pname) + "] " + nickname;
		break;
	}
    }

    return r;
}

// ----------------------------------------------------------------------------

bool ischannel(const imcontact &cont) {
    return
	(cont.nickname.substr(0, 1) == "#") &&
	(cont.pname == irc || cont.pname == yahoo || cont.pname == jabber);
}
