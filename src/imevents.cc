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
    string rdbuf;

    getstring(f, url);
    description = readblock(f);
}

// -- imauthorization class ---------------------------------------------------

imauthorization::imauthorization() {
    type = authorization;
}

imauthorization::imauthorization(const imcontact acont, imdirection adirection, const string atext) {
    type = authorization;
    contact = acont;
    direction = adirection;
    text = atext;
}

imauthorization::imauthorization(const imevent &ev) {
    type = authorization;
    contact = ev.contact;
    direction = ev.direction;
    timestamp = ev.timestamp;

    const imauthorization *m = dynamic_cast<const imauthorization *>(&ev);

    if(m) {
	text = m->text;
    }
}

const string imauthorization::gettext() const {
    return text;
}

void imauthorization::write(ofstream &f) const {
    imevent::write(f);
    f << text << endl;
}

void imauthorization::read(ifstream &f) {
    text = readblock(f);
}
