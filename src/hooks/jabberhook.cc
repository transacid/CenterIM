/*
*
* centericq icq protocol handling class
* $Id: jabberhook.cc,v 1.3 2002/11/20 14:41:13 konst Exp $
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
#include "eventmanager.h"

jabberhook jhook;

jabberhook::jabberhook(): jc(0), flogged(false) {
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

    jab_packet_handler(jc, &packethandler);
    jab_state_handler(jc, &statehandler);

    jab_start(jc);

    if(!jc || jc->state == JCONN_STATE_OFF) {
	face.log(_("+ [jab] unable to connect to the server"));
	jab_delete(jc);
	jc = 0;
    } else {
	status = manualstatus;
    }
}

void jabberhook::disconnect() {
    statehandler(jc, JCONN_STATE_OFF);
    jab_delete(jc);
    jc = 0;
}

void jabberhook::main() {
    jab_poll(jc, 0);

    if(!jc) {
	statehandler(jc, JCONN_STATE_OFF);

    } else if(jc->state == JCONN_STATE_OFF || jc->fd == -1) {
	statehandler(jc, JCONN_STATE_OFF);
	jab_delete(jc);
	jc = 0;
    }
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
    return jc && flogged;
}

bool jabberhook::isconnecting() const {
    return jc && !flogged;
}

bool jabberhook::enabled() const {
    return true;
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

// ----------------------------------------------------------------------------

void jabberhook::statehandler(jconn conn, int state) {
    static int previous_state = JCONN_STATE_OFF;

    switch(state) {
	case JCONN_STATE_OFF:
	    if(previous_state != JCONN_STATE_OFF) {
		face.log(_("+ [jab] disconnected"));
		clist.setoffline(jabber);
		face.update();
		jhook.jc = 0;
		jhook.flogged = false;
	    }
	    break;

	case JCONN_STATE_CONNECTED:
	    break;

	case JCONN_STATE_AUTH:
	    break;

	case JCONN_STATE_ON:
	    face.log(_("+ [jab] logged in"));
	    jhook.flogged = true;
	    break;

	default:
	    break;
    }

    previous_state = state;
}

void jabberhook::packethandler(jconn conn, jpacket packet) {
    char *p;
    xmlnode x;
    string from, type, body;

    jpacket_reset(packet);

    switch(packet->type) {
	case JPACKET_MESSAGE:
	    p = xmlnode_get_attrib(packet->x, "from"); if(p) from = p;
	    p = xmlnode_get_attrib(packet->x, "type"); if(p) type = p;

	    x = xmlnode_get_tag(packet->x, "body");
	    p = xmlnode_get_data(x); if(p) body = p;

	    if(x = xmlnode_get_tag(packet->x, "subject")) {
		body = (string) xmlnode_get_data (x) + ": " + body;
	    }

	    if(type == "groupchat") {
	    } else {
		imcontact ic(from, jabber);
		em.store(immessage(ic, imevent::incoming, body));
	    }
	    break;

	case JPACKET_IQ:
	    p = xmlnode_get_attrib(packet->x, "from"); if(p) from = p;
	    p = xmlnode_get_attrib(packet->x, "type"); if(p) type = p;

	    if(type == "result") {
	    } else if(type == "set") {
	    } else if(type == "error") {
		x = xmlnode_get_tag(packet->x, "error");
		int code = atoi(xmlnode_get_attrib(x, "code"));
		string name = xmlnode_get_attrib(x, "id");
		string desc = xmlnode_get_tag_data(packet->x, "error");

		switch(code) {
		    case 401: /* Unauthorized */
		    case 302: /* Redirect */
		    case 400: /* Bad request */
		    case 402: /* Payment Required */
		    case 403: /* Forbidden */
		    case 404: /* Not Found */
		    case 405: /* Not Allowed */
		    case 406: /* Not Acceptable */
		    case 407: /* Registration Required */
		    case 408: /* Request Timeout */
		    case 409: /* Conflict */
		    case 500: /* Internal Server Error */
		    case 501: /* Not Implemented */
		    case 502: /* Remote Server Error */
		    case 503: /* Service Unavailable */
		    case 504: /* Remote Server Timeout */
		    default:
			face.log(_("[jab] error %d: %s"), code, desc.c_str());
		}

	    }
	    break;

	case JPACKET_PRESENCE:
	    break;

	case JPACKET_S10N:
	    break;

	default:
	    break;
    }
}
