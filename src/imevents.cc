#include "imevents.h"
#include "centericq.h"

// -- serialization constants -------------------------------------------------

static const string evlinebreak = "\r\n";

static const string sdirection[imevent::imdirection_size] = {
    "IN", "OUT"
};

static const string seventtype[imevent::imeventtype_size] = {
    "MSG", "URL", "SMS", "AUTH", "", "EMAIL", "NOTE"
};

// -- basic imevent class -----------------------------------------------------

imevent::imevent(): timestamp(time(0)) {
}

imevent::imevent(const imcontact &acont, imdirection adir, imeventtype atype)
: contact(acont), direction(adir), timestamp(time(0)), type(atype) {
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
    f << "\f" << endl <<
	sdirection[direction] << endl <<
	seventtype[type] << endl <<
	timestamp << endl <<
	timestamp << endl;
}

void imevent::read(ifstream &f) {
    string rdbuf;

    getstring(f, rdbuf);
    if(rdbuf == "\f")
	getstring(f, rdbuf);

    for(direction = incoming; direction != imdirection_size; (int) direction += 1)
	if(sdirection[direction] == rdbuf) break;

    getstring(f, rdbuf);
    type = imeventtype_size;

    if(rdbuf != "") {
	for(type = message; type != imeventtype_size; (int) type += 1)
	    if(seventtype[type] == rdbuf) break;
    }

    getstring(f, rdbuf);
    timestamp = strtoul(rdbuf.c_str(), 0, 0);

    getstring(f, rdbuf);
	// skip the second stamp

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
	default:
	    return 0;
    }
}

bool imevent::contains(const string &text) const {
    return false;
}

// -- immessage class ---------------------------------------------------------

immessage::immessage(const imcontact &acont, imdirection adirection, const string &atext)
: imevent(acont, adirection, message), text(atext) {
}

immessage::immessage(const imevent &ev) {
    type = message;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;

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

// -- imrawevent class --------------------------------------------------------

imrawevent::imrawevent(imeventtype atype, const imcontact &acont, imdirection adirection)
: imevent(acont, adirection, atype) {
}
