#include "imevents.h"
#include "centericq.h"

// -- serialization constants -------------------------------------------------

static const string evlinebreak = "\r\n";

static const string sdirection[imevent::imdirection_size] = {
    "IN", "OUT"
};

static const string seventtype[imevent::imeventtype_size] = {
    "MSG", "URL", "SMS", "AUTH", "", ""
};

// -- basic imevent class -----------------------------------------------------

imevent::imevent() {
    time(&timestamp);
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

	read(f);
    }
}

const string imevent::readblock(ifstream &f) {
    string rdbuf, r;

    while(!f.eof()) {
	getstring(f, rdbuf);

	if(rdbuf == "\f") break;
	if(f.eof() && rdbuf.empty()) break;

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
	default:
	    return 0;
    }
}

bool imevent::contains(const string text) const {
    return false;
}

// -- immessage class ---------------------------------------------------------

immessage::immessage() {
    type = message;
}

immessage::immessage(const imcontact acont, imdirection adirection,
const string atext) {
    type = message;
    contact = acont;
    direction = adirection;
    text = atext;
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

const string immessage::gettext() const {
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

bool immessage::contains(const string atext) const {
    return text.find(atext) != -1;
}

// -- imurl class -------------------------------------------------------------

imurl::imurl() {
    type = imevent::url;
}

imurl::imurl(const imcontact acont, imdirection adirection, const string aurl,
const string adescription) {
    type = imevent::url;
    contact = acont;
    direction = adirection;
    url = aurl;
    description = adescription;
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

const string imurl::geturl() const {
    return url;
}

const string imurl::getdescription() const {
    return description;
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

bool imurl::contains(const string atext) const {
    return (url.find(atext) != -1) || (description.find(atext) != -1);
}

// -- imauthorization class ---------------------------------------------------

imauthorization::imauthorization() {
    type = authorization;
}

imauthorization::imauthorization(const imcontact acont, imdirection adirection,
bool agranted, const string atext) {
    type = authorization;
    contact = acont;
    direction = adirection;
    text = atext;
    granted = agranted;
}

imauthorization::imauthorization(const imevent &ev) {
    type = authorization;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;

    const imauthorization *m = dynamic_cast<const imauthorization *>(&ev);

    if(m) {
	text = m->text;
	granted = m->granted;
    }
}

const string imauthorization::gettext() const {
    return text;
}

void imauthorization::write(ofstream &f) const {
    imevent::write(f);

    f << (granted ? "1" : "0") << endl <<
	text << endl;
}

void imauthorization::read(ifstream &f) {
    string rdbuf;

    getstring(f, rdbuf);
    text = readblock(f);

    granted = rdbuf == "1";
}

bool imauthorization::empty() const {
    return false;
}

bool imauthorization::getgranted() const {
    return granted;
}

bool imauthorization::contains(const string atext) const {
    return text.find(atext) != -1;
}

// -- imsms class -------------------------------------------------------------

imsms::imsms() {
    type = sms;
}

imsms::imsms(const imcontact acont, imdirection adirection,
const string atext) {
    type = sms;
    contact = acont;
    direction = adirection;
    text = atext;
}

imsms::imsms(const imevent &ev) {
    type = sms;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;

    const imsms *m = dynamic_cast<const imsms *>(&ev);
    if(m) {
	text = m->text;
    }
}

const string imsms::gettext() const {
    return text;
}

void imsms::write(ofstream &f) const {
    imevent::write(f);
    f << text << endl;
}

void imsms::read(ifstream &f) {
    text = readblock(f);
}

bool imsms::empty() const {
    return text.empty();
}

bool imsms::contains(const string atext) const {
    return text.find(atext) != -1;
}
