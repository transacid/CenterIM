/*
*
* centericq IM protocol abstraction class
* $Id: abstracthook.cc,v 1.58 2004/11/09 23:49:59 konst Exp $
*
* Copyright (C) 2001,2002,2003 by Konstantin Klyagin <konst@konst.org.ua>
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
#include "aimhook.h"
#include "irchook.h"
#include "jabberhook.h"
#include "rsshook.h"
#include "ljhook.h"
#include "gaduhook.h"
#include "msnhook.h"

#include "icqface.h"

#include "md5.h"

#include <time.h>

time_t timer_current = time(0);

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

    for(protocolname pname = icq; pname != protocolname_size; pname++) {
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

void abstracthook::renamegroup(const string &oldname, const string &newname) {
}

string abstracthook::rushtmlconv(const string &tdir, const string &text, bool rus) {
    int pos;
    string r = rus ? rusconv(tdir, text) : text;

    if(tdir == "kw" || tdir == "ku") {
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

    } else if(tdir == "wk" || tdir == "uk") {
	pos = 0;
	while((pos = r.find("&", pos)) != -1) {
	    if(r.substr(pos+1, 4) == "amp;") r.replace(pos, 5, "&"); else
	    if(r.substr(pos+1, 3) == "lt;") r.replace(pos, 4, "<"); else
	    if(r.substr(pos+1, 3) == "gt;") r.replace(pos, 4, ">"); else
	    if(r.substr(pos+1, 5) == "#150;") r.replace(pos, 6, "-"); else
	    if(r.substr(pos+1, 5) == "#151;") r.replace(pos, 6, "--"); else
	    if(r.substr(pos+1, 5) == "#146;") r.replace(pos, 6, "`"); else
	    if(r.substr(pos+1, 4) == "#39;") r.replace(pos, 5, "'"); else
	    if(r.substr(pos+1, 6) == "#8211;") r.replace(pos, 7, "--"); else
	    if(r.substr(pos+1, 6) == "#8230;") r.replace(pos, 7, "..."); else
	    if(r.substr(pos+1, 7) == "hellip;") r.replace(pos, 8, "..."); else
	    if(r.substr(pos+1, 6) == "laquo;") r.replace(pos, 7, "<<"); else
	    if(r.substr(pos+1, 6) == "raquo;") r.replace(pos, 7, ">>"); else
	    if(r.substr(pos+1, 6) == "bdquo;") r.replace(pos, 7, "\""); else
	    if(r.substr(pos+1, 6) == "ldquo;") r.replace(pos, 7, "\""); else
	    if(r.substr(pos+1, 5) == "copy;") r.replace(pos, 8, "(c)");
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
    string r;

    if(!conf.getcpconvert(proto) && tdir.find("u") == -1)
	return text;

#ifdef HAVE_ICONV
    if(tdir == "kw") r = siconv(text, conf.getconvertto(proto), conf.getconvertfrom(proto)); else
    if(tdir == "wk") r = siconv(text, conf.getconvertfrom(proto), conf.getconvertto(proto)); else
    if(tdir == "ku") r = siconv(text, conf.getconvertto(proto), "utf-8"); else
    if(tdir == "uk") r = siconv(text, "utf-8", conf.getconvertto(proto)); else
#endif
	r = text;

#ifndef HAVE_ICONV
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
#endif

    return r;
}

string abstracthook::getmd5(const string &text) {
    md5_state_t state;
    md5_byte_t digest[16];
    string r;
    char buf[3];

    md5_init(&state);
    md5_append(&state, (md5_byte_t *) text.c_str(), text.size());
    md5_finish(&state, digest);

    for(int i = 0; i < 16; i++) {
	sprintf(buf, "%02x", digest[i]);
	r += buf;
    }

    return r;
}

void abstracthook::requestfromfound(const imcontact &ic) {
    vector<icqcontact *>::const_iterator ig = foundguys.begin();

    while(ig != foundguys.end()) {
	if((*ig)->getdesc() == ic) {
	    icqcontact *rc = clist.get(ic);
	    if(!rc) rc = clist.get(contactroot);

	    rc->setnick((*ig)->getnick());
	    rc->setbasicinfo((*ig)->getbasicinfo());
	    rc->setmoreinfo((*ig)->getmoreinfo());
	    rc->setworkinfo((*ig)->getworkinfo());
	    rc->setinterests((*ig)->getinterests());
	    rc->setbackground((*ig)->getbackground());
	    rc->setabout((*ig)->getabout());

	    break;
	}

	++ig;
    }
}

bool abstracthook::regconnect(const string &aserv) {
    return false;
}

bool abstracthook::regattempt(unsigned int &auin, const string &apassword, const string &email) {
    return false;
}

void abstracthook::log(logevent ev, ...) {
    va_list ap;
    char buf[512];
    static map<logevent, string> lst;

    if(lst.empty()) {
	lst[logConnecting] = _("connecting to the server");
	lst[logLogged] = _("logged in");
	lst[logSearchFinished] = _("search finished, %d found");
	lst[logPasswordChanged] = _("password was changed successfully");
	lst[logDisconnected] = _("disconnected");
	lst[logContactAdd] = _("adding %s to the contacts");
	lst[logContactRemove] = _("removing %s from the contacts");
	lst[logConfMembers] = _("members list fetching finished, %d found");
    }

    va_start(ap, ev);
    vsprintf(buf, lst[ev].c_str(), ap);
    va_end(ap);

    face.log((string) "+ [" + conf.getprotocolname(proto)  + "] " + buf);
    face.xtermtitle((string) "[" + conf.getprotocolname(proto)  + "] " + buf);
}

const unsigned short abstracthook::Country_table_size = 243;

const abstracthook::Country_struct abstracthook::Country_table[Country_table_size] = {
    { "Unspecified", 0 },
    { "Afghanistan", 93 },
    { "Albania", 355 },
    { "Algeria", 213 },
    { "American Samoa", 684 },
    { "Andorra", 376 },
    { "Angola", 244 },
    { "Anguilla", 101 },
    { "Antigua", 102 },
    { "Argentina", 54 },
    { "Armenia", 374 },
    { "Aruba", 297 },
    { "Ascension Island", 247 },
    { "Australia", 61 },
    { "Australian Antarctic Territory", 6721 },
    { "Austria", 43 },
    { "Azerbaijan", 994 },
    { "Bahamas", 103 },
    { "Bahrain", 973 },
    { "Bangladesh", 880 },
    { "Barbados", 104 },
    { "Barbuda", 120 },
    { "Belarus", 375 },
    { "Belgium", 32 },
    { "Belize", 501 },
    { "Benin", 229 },
    { "Bermuda", 105 },
    { "Bhutan", 975 },
    { "Bolivia", 591 },
    { "Bosnia and Herzegovina", 387 },
    { "Botswana", 267 },
    { "Brazil", 55 },
    { "British Virgin Islands", 106 },
    { "Brunei", 673 },
    { "Bulgaria", 359 },
    { "Burkina Faso", 226 },
    { "Burundi", 257 },
    { "Cambodia", 855 },
    { "Cameroon", 237 },
    { "Canada", 107 },
    { "Cape Verde Islands", 238 },
    { "Cayman Islands", 108 },
    { "Central African Republic", 236 },
    { "Chad", 235 },
    { "Chile", 56 },
    { "China", 86 },
    { "Christmas Island", 672 },
    { "Cocos-Keeling Islands", 6101 },
    { "Colombia", 57 },
    { "Comoros", 2691 },
    { "Congo", 242 },
    { "Cook Islands", 682 },
    { "Costa Rica", 506 },
    { "Croatia", 385 },
    { "Cuba", 53 },
    { "Cyprus", 357 },
    { "Czech Republic", 42 },
    { "Denmark", 45 },
    { "Diego Garcia", 246 },
    { "Djibouti", 253 },
    { "Dominica", 109 },
    { "Dominican Republic", 110 },
    { "Ecuador", 593 },
    { "Egypt", 20 },
    { "El Salvador", 503 },
    { "Equatorial Guinea", 240 },
    { "Eritrea", 291 },
    { "Estonia", 372 },
    { "Ethiopia", 251 },
    { "Faeroe Islands", 298 },
    { "Falkland Islands", 500 },
    { "Fiji Islands", 679 },
    { "Finland", 358 },
    { "France", 33 },
    { "French Antilles", 5901 },
    { "French Guiana", 594 },
    { "French Polynesia", 689 },
    { "Gabon", 241 },
    { "Gambia", 220 },
    { "Georgia", 995 },
    { "Germany", 49 },
    { "Ghana", 233 },
    { "Gibraltar", 350 },
    { "Greece", 30 },
    { "Greenland", 299 },
    { "Grenada", 111 },
    { "Guadeloupe", 590 },
    { "Guam", 671 },
    { "Guantanamo Bay", 5399 },
    { "Guatemala", 502 },
    { "Guinea", 224 },
    { "Guinea-Bissau", 245 },
    { "Guyana", 592 },
    { "Haiti", 509 },
    { "Honduras", 504 },
    { "Hong Kong", 852 },
    { "Hungary", 36 },
    { "INMARSAT (Atlantic-East)", 871 },
    { "INMARSAT (Atlantic-West)", 874 },
    { "INMARSAT (Indian)", 873 },
    { "INMARSAT (Pacific)", 872 },
    { "INMARSAT", 870 },
    { "Iceland", 354 },
    { "India", 91 },
    { "Indonesia", 62 },
    { "International Freephone Service", 800 },
    { "Iran", 98 },
    { "Iraq", 964 },
    { "Ireland", 353 },
    { "Israel", 972 },
    { "Italy", 39 },
    { "Ivory Coast", 225 },
    { "Jamaica", 112 },
    { "Japan", 81 },
    { "Jordan", 962 },
    { "Kazakhstan", 705 },
    { "Kenya", 254 },
    { "Kiribati Republic", 686 },
    { "Korea (North)", 850 },
    { "Korea (Republic of)", 82 },
    { "Kuwait", 965 },
    { "Kyrgyz Republic", 706 },
    { "Laos", 856 },
    { "Latvia", 371 },
    { "Lebanon", 961 },
    { "Lesotho", 266 },
    { "Liberia", 231 },
    { "Libya", 218 },
    { "Liechtenstein", 4101 },
    { "Lithuania", 370 },
    { "Luxembourg", 352 },
    { "Macau", 853 },
    { "Madagascar", 261 },
    { "Malawi", 265 },
    { "Malaysia", 60 },
    { "Maldives", 960 },
    { "Mali", 223 },
    { "Malta", 356 },
    { "Marshall Islands", 692 },
    { "Martinique", 596 },
    { "Mauritania", 222 },
    { "Mauritius", 230 },
    { "Mayotte Island", 269 },
    { "Mexico", 52 },
    { "Micronesia, Federated States of", 691 },
    { "Moldova", 373 },
    { "Monaco", 377 },
    { "Mongolia", 976 },
    { "Montserrat", 113 },
    { "Morocco", 212 },
    { "Mozambique", 258 },
    { "Myanmar", 95 },
    { "Namibia", 264 },
    { "Nauru", 674 },
    { "Nepal", 977 },
    { "Netherlands Antilles", 599 },
    { "Netherlands", 31 },
    { "Nevis", 114 },
    { "New Caledonia", 687 },
    { "New Zealand", 64 },
    { "Nicaragua", 505 },
    { "Niger", 227 },
    { "Nigeria", 234 },
    { "Niue", 683 },
    { "Norfolk Island", 6722 },
    { "Norway", 47 },
    { "Oman", 968 },
    { "Pakistan", 92 },
    { "Palau", 680 },
    { "Panama", 507 },
    { "Papua New Guinea", 675 },
    { "Paraguay", 595 },
    { "Peru", 51 },
    { "Philippines", 63 },
    { "Poland", 48 },
    { "Portugal", 351 },
    { "Puerto Rico", 121 },
    { "Qatar", 974 },
    { "Republic of Macedonia", 389 },
    { "Reunion Island", 262 },
    { "Romania", 40 },
    { "Rota Island", 6701 },
    { "Russia", 7 },
    { "Rwanda", 250 },
    { "Saint Lucia", 122 },
    { "Saipan Island", 670 },
    { "San Marino", 378 },
    { "Sao Tome and Principe", 239 },
    { "Saudi Arabia", 966 },
    { "Senegal Republic", 221 },
    { "Seychelle Islands", 248 },
    { "Sierra Leone", 232 },
    { "Singapore", 65 },
    { "Slovak Republic", 4201 },
    { "Slovenia", 386 },
    { "Solomon Islands", 677 },
    { "Somalia", 252 },
    { "South Africa", 27 },
    { "Spain", 34 },
    { "Sri Lanka", 94 },
    { "St. Helena", 290 },
    { "St. Kitts", 115 },
    { "St. Pierre and Miquelon", 508 },
    { "St. Vincent and the Grenadines", 116 },
    { "Sudan", 249 },
    { "Suriname", 597 },
    { "Swaziland", 268 },
    { "Sweden", 46 },
    { "Switzerland", 41 },
    { "Syria", 963 },
    { "Taiwan, Republic of China", 886 },
    { "Tajikistan", 708 },
    { "Tanzania", 255 },
    { "Thailand", 66 },
    { "Tinian Island", 6702 },
    { "Togo", 228 },
    { "Tokelau", 690 },
    { "Tonga", 676 },
    { "Trinidad and Tobago", 117 },
    { "Tunisia", 216 },
    { "Turkey", 90 },
    { "Turkmenistan", 709 },
    { "Turks and Caicos Islands", 118 },
    { "Tuvalu", 688 },
    { "USA", 1 },
    { "Uganda", 256 },
    { "Ukraine", 380 },
    { "United Arab Emirates", 971 },
    { "United Kingdom", 44 },
    { "United States Virgin Islands", 123 },
    { "Uruguay", 598 },
    { "Uzbekistan", 711 },
    { "Vanuatu", 678 },
    { "Vatican City", 379 },
    { "Venezuela", 58 },
    { "Vietnam", 84 },
    { "Wallis and Futuna Islands", 681 },
    { "Western Samoa", 685 },
    { "Yemen", 967 },
    { "Yugoslavia", 381 },
    { "Zaire", 243 },
    { "Zambia", 260 },
    { "Zimbabwe", 263 }

};

string abstracthook::getCountryIDtoString(unsigned short id) {
    for(int n = 0; n < Country_table_size; ++n) {
	if(Country_table[n].code == id) {
	    return Country_table[n].name;
	}
    }

    return Country_table[0].name;
}

unsigned short abstracthook::getCountryByName(string name) {
    int i;
    name = up(leadcut(trailcut(name)));    

    for(i = 0; i < Country_table_size; i++)
	if(name == up(Country_table[i].name))
	    return Country_table[i].code;

    return 0;
}

signed char abstracthook::getSystemTimezone() {
    time_t t = time(0);
    struct tm *tzone = localtime(&t);
    int nTimezone = 0;

#ifdef HAVE_TM_ZONE
    nTimezone = -(tzone->tm_gmtoff);
#else
    #ifdef __CYGWIN__
    nTimezone = _timezone;
    #else
    nTimezone = timezone;
    #endif
#endif

    nTimezone += (tzone->tm_isdst == 1 ? 3600 : 0);
    nTimezone /= 1800;

    if(nTimezone > 23) return 23-nTimezone;

    return (signed char) nTimezone;
}

static const unsigned char Interests_table_size = 51;
static const unsigned char Interests_offset = 100;

static const char* const Interests_table[Interests_table_size] = {
    "Art",
    "Cars",
    "Celebrity Fans",
    "Collections",
    "Computers",
    "Culture",
    "Fitness",
    "Games",
    "Hobbies",
    "ICQ - Help",
    "Internet",
    "Lifestyle",
    "Movies and TV",
    "Music",
    "Outdoors",
    "Parenting",
    "Pets and Animals",
    "Religion",
    "Science",
    "Skills",
    "Sports",
    "Web Design",
    "Ecology",
    "News and Media",
    "Government",
    "Business",
    "Mystics",
    "Travel",
    "Astronomy",
    "Space",
    "Clothing",
    "Parties",
    "Women",
    "Social science",
    "60's",
    "70's",
    "80's",
    "50's",
    "Finance and Corporate",
    "Entertainment",
    "Consumer Electronics",
    "Retail Stores",
    "Health and Beauty",
    "Media",
    "Household Products",
    "Mail Order Catalog",
    "Business Services",
    "Audio and Visual",
    "Sporting and Athletics",
    "Publishing",
    "Home Automation"
};

string abstracthook::getInterestsIDtoString(unsigned char id) {
    if(id-Interests_offset >= 0 && id-Interests_offset < Interests_table_size) {
	return Interests_table[id-Interests_offset];
    }

    return "";
}

static const unsigned short Background_table_size = 8;

static const struct {
    char *name;
    unsigned short code;

} Background_table[Background_table_size] = {
    { "University", 303 },
    { "High school", 301 },
    { "College", 302 },
    { "Elementary School", 300 },
    { "Millitary", 304 },
    { "Other", 399 },
    { "Past Organization", 306 },
    { "Past Work Place", 305 }

};

string abstracthook::getBackgroundIDtoString(unsigned short id) {
    for(int n = 0; n < Background_table_size; ++n) {
	if(id == Background_table[n].code) {
	    return Background_table[n].name;
	}
    }
      
    return "";
}

string abstracthook::getTimezoneIDtoString(signed char id) {
    if(id > 24 || id < -24) {
	return "Unspecified";
    } else {
	char buf[32];
	sprintf(buf, "GMT %s%d:%s", id > 0 ? "-" : "+", abs(id/2), id % 2 == 0 ? "00" : "30");
	return buf;
    }
}

string abstracthook::getTimezonetoLocaltime(signed char id) {
    string r;

    if(id <= 24 && id >= -24) {
	time_t rt = time(0) + getSystemTimezone()*1800;
	rt -= id*1800;
	r = ctime(&rt);
    }

    return r;
}

const unsigned char abstracthook::Language_table_size = 60;

const char* const abstracthook::Language_table[Language_table_size] = {
    "Unspecified",
    "Arabic",
    "Bhojpuri",
    "Bulgarian",
    "Burmese",
    "Cantonese",
    "Catalan",
    "Chinese",
    "Croatian",
    "Czech",
    "Danish",
    "Dutch",
    "English",
    "Esperanto",
    "Estonian",
    "Farsi",
    "Finnish",
    "French",
    "Gaelic",
    "German",
    "Greek",
    "Hebrew",
    "Hindi",
    "Hungarian",
    "Icelandic",
    "Indonesian",
    "Italian",
    "Japanese",
    "Khmer",
    "Korean",
    "Lao",
    "Latvian",
    "Lithuanian",
    "Malay",
    "Norwegian",
    "Polish",
    "Portuguese",
    "Romanian",
    "Russian",
    "Serbian",
    "Slovak",
    "Slovenian",
    "Somali",
    "Spanish",
    "Swahili",
    "Swedish",
    "Tagalog",
    "Tatar",
    "Thai",
    "Turkish",
    "Ukrainian",
    "Urdu",
    "Vietnamese",
    "Yiddish",
    "Yoruba",
    "Taiwanese",
    "Afrikaans",
    "Persian",
    "Albanian",
    "Armenian",
};

string abstracthook::getLanguageIDtoString(unsigned char id) {
    if (id >= Language_table_size) {
	return Language_table[0];
    } else {
	return Language_table[id];
    }
}

// ----------------------------------------------------------------------------

abstracthook &gethook(protocolname pname) {
    static abstracthook abshook(infocard);

    switch(pname) {
	case icq: return ihook;
#ifdef BUILD_YAHOO
	case yahoo: return yhook;
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
#ifdef BUILD_RSS
	case rss: return rhook;
#endif
#ifdef BUILD_LJ
	case livejournal: return lhook;
#endif
#ifdef BUILD_GADU
	case gadu: return ghook;
#endif
#ifdef BUILD_MSN
	case msn: return mhook;
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
