/*
*
* centericq icq protocol handling class
* $Id: jabberhook.cc,v 1.2 2002/11/19 18:13:36 konst Exp $
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

#include "jabberhook.h"
#include "icqface.h"
#include "imlogger.h"

jabberhook jhook;

jabberhook::jabberhook(): jc(0) {
}

jabberhook::~jabberhook() {
}

void jabberhook::init() {
    manualstatus = conf.getstatus(jabber);
}

void jabberhook::connect() {
    icqconf::imaccount acc = conf.getourid(icq);
    string jid;

    face.log(_("+ [jab] connecting to the server"));

    jid = acc.nickname + "@" + acc.server + "/centericq";

    auto_ptr<char> cjid(strdup(jid.c_str()));
    auto_ptr<char> cpass(strdup(acc.password.c_str()));

    jc = jab_new(cjid.get(), cpass.get());
/*
    jab_packet_handler(jc, j_on_packet_handler);
    jab_state_handler(jc, j_on_state_handler);
*/
    jab_start(jc);

    if(!jc || jc->state == JCONN_STATE_OFF) {
	face.log(_("+ [jab] unable to connect to the server"));
	jab_delete(jc);
	jc = 0;
    } else {
	status = manualstatus;
/*
	jc->id = atoi(jab_auth(jc));
	jc->reg_flag = 0;
*/
    }
}

void jabberhook::disconnect() {
}

void jabberhook::main() {
}

void jabberhook::getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const {
    if(jc) {
	FD_SET(jc->fd, &rfds);
    }
}

bool jabberhook::isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const {
    if(jc) {
	return FD_ISSET(jc->fd, &rfds);
    } else {
	return false;
    }
}

bool jabberhook::online() const {
    return (bool) jc;
}

bool jabberhook::logged() const {
}

bool jabberhook::isconnecting() const {
}

bool jabberhook::enabled() const {
    return false; //true;
}

bool jabberhook::send(const imevent &ev) {
}

void jabberhook::sendnewuser(const imcontact &c) {
}

void jabberhook::removeuser(const imcontact &ic) {
}

void jabberhook::setautostatus(imstatus st) {
    if(st != offline) {
	if(getstatus() == offline) {
	    connect();
	} else {
	    logger.putourstatus(jabber, status, st);
//            MSN_ChangeState(stat2int[status = st]);
	}
    } else {
	if(getstatus() != offline) {
	    disconnect();
	}
    }
}

void jabberhook::requestinfo(const imcontact &c) {
}
