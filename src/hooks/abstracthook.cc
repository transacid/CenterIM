/*
*
* centericq IM protocol abstraction class
* $Id: abstracthook.cc,v 1.35 2003/07/07 18:51:00 konst Exp $
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

abstracthook::abstracthook(protocolname aproto)
    : proto(aproto), searchdest(0)
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

string abstracthook::rushtmlconv(const string &tdir, const string &text) {
    int pos;
    string r = rusconv(tdir, text);

    if(tdir == "kw") {
	pos = 0;
	while((pos = r.find_first_of("&<>", pos)) != -1) {
	    switch(r[pos]) {
		case '&':
		    if(r.substr(pos, 4) != "&lt;"
		    && r.substr(pos, 4) != "&gt;")
			r.replace(pos, 1, "&amp;");
		    break;
		case '<': r.replace(pos, 1, "&lt;"); break;
		case '>': r.replace(pos, 1, "&gt;"); break;
	    }
	    pos++;
	}

    } else if(tdir == "wk") {
	pos = 0;
	while((pos = r.find("&", pos)) != -1) {
	    if(r.substr(pos+1, 4) == "amp;") r.replace(pos, 5, "&"); else
	    if(r.substr(pos+1, 3) == "lt;") r.replace(pos, 4, "<"); else
	    if(r.substr(pos+1, 3) == "gt;") r.replace(pos, 4, ">"); 
	    pos++;
	}
    }

    return r;
}

string abstracthook::ruscrlfconv(const string &tdir, const string &text) {
    string r = rusconv(tdir, text);
    int pos;

    for(pos = 0; (pos = r.find("\r")) != -1; pos++) {
	r.erase(pos, 1);
    }

    return r;
}

string abstracthook::rusconv(const string &tdir, const string &text) {
    if(!conf.getrussian(proto))
	return text;

    string r;

    static unsigned char kw[] = {
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
	160,161,162,184,164,165,166,167,168,169,170,171,172,173,174,175,
	176,177,178,168,180,181,182,183,184,185,186,187,188,189,190,191,
	254,224,225,246,228,229,244,227,245,232,233,234,235,236,237,238,
	239,255,240,241,242,243,230,226,252,251,231,248,253,249,247,250,
	222,192,193,214,196,197,212,195,213,200,201,202,203,204,205,206,
	207,223,208,209,210,211,198,194,220,219,199,216,221,217,215,218
    };

    static unsigned char wk[] = {
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
	160,161,162,163,164,165,166,167,179,169,170,171,172,173,174,175,
	176,177,178,179,180,181,182,183,163,185,186,187,188,189,190,191,
	225,226,247,231,228,229,246,250,233,234,235,236,237,238,239,240,
	242,243,244,245,230,232,227,254,251,253,255,249,248,252,224,241,
	193,194,215,199,196,197,214,218,201,202,203,204,205,206,207,208,
	210,211,212,213,198,200,195,222,219,221,223,217,216,220,192,209
    };

    unsigned char c;
    string::const_iterator i;
    unsigned char *table = 0;

#ifdef HAVE_ICONV_H
    if(tdir == "kw") r = siconv(text, "koi8-u", "cp1251"); else
    if(tdir == "wk") r = siconv(text, "cp1251", "koi8-u"); else
#endif
	r = text;

    if(text == r) {
	if(tdir == "kw") table = kw; else
	if(tdir == "wk") table = wk;

	if(table) {
	    r = "";

	    for(i = text.begin(); i != text.end(); ++i) {
		c = (unsigned char) *i;
		c &= 0377;
		if(c & 0200) c = table[c & 0177];
		r += c;
	    }
	}
    }

    return r;
}

abstracthook::Encoding abstracthook::guessencoding(const string &text) {
    if(!conf.getrussian(proto))
	return encKOI;

    string::const_iterator ic = text.begin();
    int utf, latin, third;

    utf = latin = 0;

    while(ic != text.end()) {
	unsigned char c = (unsigned) *ic;

	if(c > 32 && c < 127) latin++; else
	if(c > 127 && c < 192) utf++;

	++ic;
    }

    third = (int) text.size()/3;

    if(utf > third) return encUTF; else
    if(latin > third) return encUnknown; else
	return encKOI;
}

// ----------------------------------------------------------------------------

abstracthook &gethook(protocolname pname) {
    static abstracthook abshook(infocard);

    switch(pname) {
	case icq: return ihook;
#ifdef BUILD_YAHOO
	case yahoo: return yhook;
#endif
#ifdef BUILD_MSN
	case msn: return mhook;
#endif
#ifdef BUILD_AIM
	case aim: return ahook;
#endif
#ifdef BUILD_IRC
	case irc: return irhook;
#endif
#ifdef BUILD_JABBER
	case jabber: return jhook;
#endif
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
