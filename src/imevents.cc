/*
*
* centericq events serialization classes
* $Id: imevents.cc,v 1.36 2005/01/18 23:20:17 konst Exp $
*
* Copyright (C) 2001-2004 by Konstantin Klyagin <konst@konst.org.ua>
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

#include "imevents.h"
#include "src/Xml.h"
#include "hooks/abstracthook.h"

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif

// -- serialization constants -------------------------------------------------

static const string evlinebreak = "\r\n";

static const string sdirection[imevent::imdirection_size] = {
    "IN", "OUT"
};

static const string seventtype[imevent::imeventtype_size] = {
    "MSG", "URL", "SMS", "AUTH", "", "", "EMAIL", "NOTE", "CONT", "FILE", "XML"
};

// -- basic imevent class -----------------------------------------------------

imevent::imevent() {
}

imevent::imevent(const imcontact &acont, imdirection adir, imeventtype atype, time_t asenttimestamp)
: contact(acont), direction(adir), type(atype), timestamp(timer_current),
  senttimestamp(asenttimestamp ? asenttimestamp : timer_current) {
}

imevent::imevent(ifstream &f) {
    read(f);
}

imevent::~imevent() {
}

imevent::imeventtype imevent::gettype() const {
    return type;
}

imevent::imdirection imevent::getdirection() const {
    return direction;
}

imcontact imevent::getcontact() const {
    return contact;
}

time_t imevent::getsenttimestamp() const {
    return senttimestamp;
}

time_t imevent::gettimestamp() const {
    return timestamp;
}

string imevent::gettext() const {
    return "";
}

void imevent::settimestamp(time_t atimestamp) {
    timestamp = atimestamp;
}

void imevent::setcontact(const imcontact &acontact) {
    contact = acontact;
}

void imevent::write(ofstream &f) const {
    if(type == imeventtype_size)
	return;

    f << "\f" << endl <<
	sdirection[direction] << endl <<
	seventtype[type] << endl <<
	timestamp << endl <<
	senttimestamp << endl;
}

void imevent::read(ifstream &f) {
    string rdbuf;

    getstring(f, rdbuf);
    if(rdbuf == "\f")
	getstring(f, rdbuf);

    for(direction = incoming; direction != imdirection_size; direction++)
	if(sdirection[direction] == rdbuf) break;

    getstring(f, rdbuf);
    type = imeventtype_size;

    if(rdbuf != "") {
	for(type = message; type != imeventtype_size; type++)
	    if(seventtype[type] == rdbuf) break;
    }

    getstring(f, rdbuf);
    timestamp = strtoul(rdbuf.c_str(), 0, 0);

    getstring(f, rdbuf);
    senttimestamp = strtoul(rdbuf.c_str(), 0, 0);

    if(direction == imdirection_size || type == imeventtype_size) {
	while(!f.eof() && rdbuf != "\f")
	    getstring(f, rdbuf);

	if(!f.eof())
	    read(f);
    }
}

string imevent::readblock(ifstream &f) {
    int pos;
    string rdbuf, r;

    while(!f.eof()) {
	getstring(f, rdbuf);

	if(rdbuf == "\f") break;
	if(f.eof() && rdbuf.empty()) break;

	while((pos = rdbuf.find_first_of(evlinebreak)) != -1)
	    rdbuf.erase(pos, 1);

	if(!r.empty()) r += evlinebreak;
	r += rdbuf;
    }

    return r;
}

bool imevent::empty() const {
    return false;
}

imevent *imevent::getevent() const {
    switch(type) {
	case message:
	    return new immessage(*this);
	case url:
	    return new imurl(*this);
	case sms:
	    return new imsms(*this);
	case authorization:
	    return new imauthorization(*this);
	case email:
	    return new imemail(*this);
	case notification:
	    return new imnotification(*this);
	case contacts:
	    return new imcontacts(*this);
	case file:
	    return new imfile(*this);
	case xml:
	    return new imxmlevent(*this);
	default:
	    return new imrawevent(*this);
    }
}

bool imevent::contains(const string &text) const {
    return false;
}

// -- immessage class ---------------------------------------------------------

immessage::immessage(const imcontact &acont, imdirection adirection, const string &atext, const time_t asenttime)
: imevent(acont, adirection, message, asenttime), text(atext) {
}

immessage::immessage(const imevent &ev) {
    type = message;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;
    senttimestamp = ev.senttimestamp;

    const immessage *m = dynamic_cast<const immessage *>(&ev);

    if(m) {
	text = m->text;
    }
}

string immessage::gettext() const {
    return text;
}

void immessage::write(ofstream &f) const {
    imevent::write(f);
    f << text << endl;
}

void immessage::read(ifstream &f) {
    text = readblock(f);
}

bool immessage::empty() const {
    return text.empty();
}

bool immessage::contains(const string &atext) const {
    return text.find(atext) != -1;
}

// -- imurl class -------------------------------------------------------------

imurl::imurl(const imcontact &acont, imdirection adirection, const string &aurl, const string &adescription)
: imevent(acont, adirection, imevent::url), url(aurl), description(adescription) {
}

imurl::imurl(const imevent &ev) {
    type = imevent::url;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;
    senttimestamp = ev.senttimestamp;

    const imurl *m = dynamic_cast<const imurl *>(&ev);

    if(m) {
	url = m->url;
	description = m->description;
    }
}

string imurl::geturl() const {
    return url;
}

string imurl::getdescription() const {
    return description;
}

string imurl::gettext() const {
    return url + "\n----\n" + description;
}

void imurl::write(ofstream &f) const {
    imevent::write(f);
    f << url << endl <<
	description << endl;
}

void imurl::read(ifstream &f) {
    getstring(f, url);
    description = readblock(f);
}

bool imurl::empty() const {
    return url.empty() && description.empty();
}

bool imurl::contains(const string &atext) const {
    return (url.find(atext) != -1) || (description.find(atext) != -1);
}

// -- imauthorization class ---------------------------------------------------

imauthorization::imauthorization(const imcontact &acont, imdirection adirection,
    AuthType atype, const string &atext) : imevent(acont, adirection,
    authorization), authtype(atype)
{
    text = atext;
    if(text.empty() && authtype == Request)
    switch(adirection) {
	case outgoing: text = "Please accept my authorization to add you to my contact list."; break;
	case incoming: text = _("Empty authorization request message"); break;
    }

}

imauthorization::imauthorization(const imevent &ev) {
    type = authorization;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;
    senttimestamp = ev.senttimestamp;

    const imauthorization *m = dynamic_cast<const imauthorization *>(&ev);

    if(m) {
	text = m->text;
	authtype = m->authtype;
    }
}

string imauthorization::gettext() const {
    string prefix;

    prefix = (authtype == Request) ?
	_("* Authorization request : ") :
	_("* Authorization : ");

    return prefix + text;
}

string imauthorization::getmessage() const {
    return text;
}

void imauthorization::write(ofstream &f) const {
    imevent::write(f);

    f << i2str((int) authtype) << endl <<
	text << endl;
}

void imauthorization::read(ifstream &f) {
    string rdbuf;

    getstring(f, rdbuf);
    text = readblock(f);

    authtype = (AuthType) strtoul(rdbuf.c_str(), 0, 0);
}

bool imauthorization::empty() const {
    return false;
}

imauthorization::AuthType imauthorization::getauthtype() const {
    return authtype;
}

bool imauthorization::contains(const string &atext) const {
    return gettext().find(atext) != -1;
}

// -- imsms class -------------------------------------------------------------

imsms::imsms(const imcontact &acont, imdirection adirection, const string &atext, const string &aphone)
: imevent(acont, adirection, sms), text(atext), phone(aphone) {
}

imsms::imsms(const imevent &ev) {
    type = sms;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;
    senttimestamp = ev.senttimestamp;

    const imsms *m = dynamic_cast<const imsms *>(&ev);

    if(m) {
	text = m->text;
	phone = m->phone;
    }
}

string imsms::gettext() const {
    return (string) _("* SMS : ")  + text;
}

void imsms::write(ofstream &f) const {
    imevent::write(f);

    if(!phone.empty()) {
	if(phone.substr(0, 1) != "+") f << "+";
	f << phone << endl;
    }

    f << text << endl;
}

void imsms::read(ifstream &f) {
    getstring(f, phone);
    text = readblock(f);

    if(!phone.empty() && phone.substr(0, 1) != "+") {
	if(!text.empty()) phone += "\n" + text;
	text = phone;
	phone = "";
    }
}

bool imsms::empty() const {
    return text.empty();
}

bool imsms::contains(const string &atext) const {
    return text.find(atext) != -1;
}

// -- imemail class -----------------------------------------------------------

imemail::imemail(const imcontact &acont, imdirection adirection, const string &atext)
: imevent(acont, adirection, email), text(atext) {
}

imemail::imemail(const imevent &ev) {
    type = email;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;
    senttimestamp = ev.senttimestamp;

    const imemail *m = dynamic_cast<const imemail *>(&ev);

    if(m) {
	text = m->text;
    }
}

string imemail::gettext() const {
    return text;
}

bool imemail::empty() const {
    return text.empty();
}

bool imemail::contains(const string &atext) const {
    return text.find(atext) != -1;
}

void imemail::write(ofstream &f) const {
    imevent::write(f);
    f << text << endl;
}

void imemail::read(ifstream &f) {
    text = readblock(f);
}

// -- imnotification class ----------------------------------------------------

imnotification::imnotification(const imcontact &acont, const string &atext)
: imevent(acont, incoming, notification), text(atext) {
}

imnotification::imnotification(const imevent &ev) {
    type = notification;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;
    senttimestamp = ev.senttimestamp;

    const imnotification *m = dynamic_cast<const imnotification *>(&ev);

    if(m) {
	text = m->text;
    }
}

string imnotification::gettext() const {
    return (string) "* " + text;
}

bool imnotification::empty() const {
    return text.empty();
}

bool imnotification::contains(const string &atext) const {
    return text.find(atext) != -1;
}

void imnotification::write(ofstream &f) const {
    imevent::write(f);
    f << text << endl;
}

void imnotification::read(ifstream &f) {
    text = readblock(f);
}

// -- imcontacts class --------------------------------------------------------

imcontacts::imcontacts(const imcontact &acont, imdirection adirection, const vector< pair<unsigned int, string> > &lst)
: imevent(acont, adirection, imevent::contacts), contacts(lst) {
}

imcontacts::imcontacts(const imevent &ev) {
    type = imevent::contacts;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;
    senttimestamp = ev.senttimestamp;

    const imcontacts *m = dynamic_cast<const imcontacts *>(&ev);

    if(m) {
	contacts = m->contacts;
    }
}

string imcontacts::gettext() const {
    string text = _("* Contacts : ");
    vector< pair<unsigned int, string> >::const_iterator ic;

    for(ic = contacts.begin(); ic != contacts.end(); ++ic) {
	if(ic->first) {
	    text += (ic->second.empty() ? _("<no nick>") : ic->second)
		+ " (" + i2str(ic->first) + ")";
	} else {
	    text += ic->second;
	}

	if(ic+1 != contacts.end())
	    text += ", ";
    }

    return text;
}

const vector< pair<unsigned int, string> > &imcontacts::getcontacts() const {
    return contacts;
}

bool imcontacts::empty() const {
    return contacts.empty();
}

bool imcontacts::contains(const string &atext) const {
    return gettext().find(atext) != -1;
}

void imcontacts::write(ofstream &f) const {
    vector< pair<unsigned int, string> >::const_iterator ic;

    imevent::write(f);

    for(ic = contacts.begin(); ic != contacts.end(); ++ic) {
	f
	    << ic->first << "\t"
	    << ic->second << "\n";
    }
}

void imcontacts::read(ifstream &f) {
    string buf;
    int pos;

#ifdef HAVE_SSTREAM
    ostringstream ost;
    ost << readblock(f);
    istringstream st(ost.str());
#else
    strstream st;
    st << readblock(f);
#endif

    while(getline(st, buf)) {
	while((pos = buf.find_first_of("\r\n")) != -1)
	    buf.erase(pos, 1);

	if((pos = buf.find("\t")) != -1) {
	    contacts.push_back(make_pair(atoi(buf.substr(0, pos).c_str()), buf.substr(pos+1)));
	}
    }
}

// -- imfile class ------------------------------------------------------------

imfile::imfile(const imevent &ev) {
    type = imevent::file;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;
    senttimestamp = ev.senttimestamp;

    const imfile *m = dynamic_cast<const imfile *>(&ev);

    if(m) {
	files = m->files;
    }
}

imfile::imfile(const imcontact &acont, imdirection adirection, const string &amsg, const vector<record> &afiles)
: imevent(acont, adirection, imevent::file), msg(amsg), files(afiles) {
}

string imfile::gettext() const {
    string r;
    r = _("* File transfer");

    if(!msg.empty())
	r += (string) " <" + msg + ">";

    r += (string) " : ";

    for(vector<record>::const_iterator i = files.begin(); i != files.end(); ++i) {
	r += i->fname;

	if(i->size != -1)       // -1 means the size is unknown
	    r += (string) " (" + i2str(i->size) + ")";

	if(i+1 != files.end())
	    r += ", ";
    }

    return r;
}

string imfile::getmessage() const {
    return msg;
}

const vector<imfile::record> &imfile::getfiles() const {
    return files;
}

void imfile::setfiles(const vector<imfile::record> &lst) {
    files = lst;
}

bool imfile::empty() const {
    return files.empty();
}

bool imfile::contains(const string &atext) const {
    return gettext().find(atext) != -1;
}

void imfile::write(ofstream &f) const {
    imevent::write(f);

    f << "|" << mime(msg) << endl;

    for(vector<record>::const_iterator i = files.begin(); i != files.end(); ++i)
	f << mime(i->fname) << "\t" << i->size << endl;
}

void imfile::read(ifstream &f) {
    string buf;
    int pos, line = 0;

#ifdef HAVE_SSTREAM
    ostringstream ost;
    ost << readblock(f);
    istringstream st(ost.str());
#else
    strstream st;
    st << readblock(f);
#endif

    while(getline(st, buf)) {
	while((pos = buf.find_first_of("\r\n")) != -1)
	    buf.erase(pos, 1);

	if(!line++) {
	    msg = unmime(buf.substr(1));

	} else if((pos = buf.find("\t")) != -1) {
	    record r;
	    r.fname = unmime(buf.substr(0, pos));
	    r.size = strtoul(buf.substr(pos+1).c_str(), 0, 0);
	    files.push_back(r);

	}
    }
}

// -- imrawevent class --------------------------------------------------------

imrawevent::imrawevent(imeventtype atype, const imcontact &acont, imdirection adirection)
: imevent(acont, adirection, atype) {
}

imrawevent::imrawevent(const imevent &ev) {
    type = ev.type;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;
    senttimestamp = ev.senttimestamp;
}

// -- imxmlevent class --------------------------------------------------------

imxmlevent::imxmlevent(const imcontact &acont, imdirection adirection, const string &atext)
: imevent(acont, adirection, imevent::xml) {
    data["text"] = atext;
}

imxmlevent::imxmlevent(const imevent &ev) {
    type = ev.type;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;
    senttimestamp = ev.senttimestamp;

    const imxmlevent *m = dynamic_cast<const imxmlevent *>(&ev);

    if(m) {
	data = m->data;
    }
}

string imxmlevent::gettext() const {
    return getfield("text");
}

bool imxmlevent::field_empty(const string &name) const {
    return getfield(name).empty();
}

string imxmlevent::getfield(const string &name) const {
    string r;
    map<string, string>::const_iterator ic = data.find(name);

    if(ic != data.end()) {
	r = ic->second;
    }

    return r;
}

void imxmlevent::setfield(const string &name, const string &val) {
    data[name] = val;
}

bool imxmlevent::empty() const {
    return getfield("text").empty();
}

bool imxmlevent::contains(const string &atext) const {
    return gettext().find(atext) != -1;
}

void imxmlevent::write(ofstream &f) const {
    XmlBranch x("xml");

    map<string, string>::const_iterator id = data.begin();
    while(id != data.end()) {
	if(!id->second.empty())
	    x.pushnode(new XmlLeaf(id->first, id->second));
	++id;
    }

    imevent::write(f);
    f << x.toString(0) << endl;
}

void imxmlevent::read(ifstream &f) {
    string content = readblock(f);
    string::iterator ic = content.begin();

    auto_ptr<XmlNode> top(XmlNode::parse(ic, content.end()));
    XmlBranch *x = dynamic_cast<XmlBranch *>(top.get());

    list<string> tags = x->getChildren();
    list<string>::const_iterator it = tags.begin();

    while(it != tags.end()) {
	data[*it] = x->getLeaf(*it)->getValue();
	++it;
    }

}
