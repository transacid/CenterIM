/*
*
* centericq IM protocol abstraction class
* $Id: abstracthook.cc,v 1.33 2002/12/11 10:46:22 konst Exp $
*
* Copyright (C) 2001,2002 by Konstantin Klyagin <konst@konst.org.ua>
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

#include "abstracthook.h"

#include "icqhook.h"
#include "yahoohook.h"
#include "msnhook.h"
#include "aimhook.h"
#include "irchook.h"
#include "jabberhook.h"

#include <time.h>

abstracthook::abstracthook()
    : searchdest(0)
{ }

void abstracthook::init() {
}

void abstracthook::connect() {
}

void abstracthook::disconnect() {
}

void abstracthook::exectimers() {
}

void abstracthook::main() {
}

void abstracthook::setautostatus(imstatus st) {
}

void abstracthook::restorestatus() {
    setautostatus(manualstatus);
}

void abstracthook::setstatus(imstatus st) {
    setautostatus(manualstatus = st);

    for(protocolname pname = icq; pname != protocolname_size; (int) pname += 1) {
	if(&gethook(pname) == this) {
	    conf.savestatus(pname, manualstatus);
	    break;
	}
    }
}

void abstracthook::getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const {
}

bool abstracthook::isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const {
    return false;
}

bool abstracthook::online() const {
    return false;
}

bool abstracthook::logged() const {
    return false;
}

bool abstracthook::isconnecting() const {
    return false;
}

bool abstracthook::enabled() const {
    return false;
}

bool abstracthook::send(const imevent &ev) {
    return false;
}

void abstracthook::sendnewuser(const imcontact &c) {
}

imstatus abstracthook::getstatus() const {
    return offline;
}

bool abstracthook::isdirectopen(const imcontact &c) const {
    return false;
}

void abstracthook::removeuser(const imcontact &ic) {
}

void abstracthook::requestinfo(const imcontact &c) {
}

void abstracthook::lookup(const imsearchparams &params, verticalmenu &dest) {
}

void abstracthook::stoplookup() {
    searchdest = 0;
}

void abstracthook::requestawaymsg(const imcontact &c) {
}

vector<icqcontact *> abstracthook::getneedsync() {
    return vector<icqcontact *>();
}

void abstracthook::ouridchanged(const icqconf::imaccount &ia) {
}

void abstracthook::requestversion(const imcontact &c) {
}

void abstracthook::ping(const imcontact &c) {
}

bool abstracthook::knowntransfer(const imfile &fr) const {
    return false;
}

void abstracthook::replytransfer(const imfile &fr, bool accept, const string &localpath) {
}

void abstracthook::aborttransfer(const imfile &fr) {
}

void abstracthook::conferencecreate(const imcontact &confid,
const vector<imcontact> &lst) {
}

vector<string> abstracthook::getservices(servicetype::enumeration st) const {
    return vector<string>();
}

vector<pair< string, string> > abstracthook::getsearchparameters(const string &agentname) const {
    return vector<pair<string, string> >();
}

vector<pair<string, string> > abstracthook::getregparameters(const string &agentname) const {
    return vector<pair<string, string> >();
}

void abstracthook::updatecontact(icqcontact *c) {
}

// ----------------------------------------------------------------------------

abstracthook &gethook(protocolname pname) {
    static abstracthook abshook;

    switch(pname) {
	case icq: return ihook;
	case yahoo: return yhook;
	case msn: return mhook;
	case aim: return ahook;
	case irc: return irhook;
	case jabber: return jhook;
    }

    return abshook;
}

struct tm *maketm(int hour, int minute, int day, int month, int year) {
    static struct tm msgtm;
    memset(&msgtm, 0, sizeof(msgtm));
    msgtm.tm_min = minute;
    msgtm.tm_hour = hour;
    msgtm.tm_mday = day;
    msgtm.tm_mon = month-1;
    msgtm.tm_year = year-1900;
    return &msgtm;
}
