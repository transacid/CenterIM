#include "imevents.h"
#include "centericq.h"

// -- serialization constants -------------------------------------------------

static const char *sdirection[imevent::imdirection_size] = {
    "IN", "OUT"
};

static const char *convdirection[imevent::imdirection_size] = {
    "wk", "kw"
};

static const char *seventtype[imevent::imeventtype_size] = {
    "MSG", "SMS", "AUTH", "", ""
};

// -- basic imevent class -----------------------------------------------------

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

    getline(f, rdbuf);
    if(rdbuf == "\f")
	getline(f, rdbuf);

    for(direction = incoming; direction != imdirection_size; (int) direction += 1)
	if(sdirection[direction] == rdbuf) break;

    getline(f, rdbuf);
    for(type = message; type != imeventtype_size; (int) type += 1)
	if(seventtype[type] == rdbuf) break;

    getline(f, rdbuf);
    timestamp = strtoul(rdbuf.c_str(), 0, 0);

    getline(f, rdbuf);
	// skip the second stamp
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
}

const string immessage::gettext() const {
    return text;
}

void immessage::write(ofstream &f) const {
    imevent::write(f);
    f << rusconv(convdirection[direction], text) << endl;
}

void immessage::read(ifstream &f) {
    string rdbuf;

    while(!f.eof()) {
	getline(f, rdbuf);
	if(rdbuf != "\f") {
	    if(!text.empty()) text += "\n";
	    text += rdbuf;
	} else {
	    break;
	}
    }
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
}

const string imurl::geturl() const {
    return url;
}

const string imurl::getdescription() const {
    return description;
}

void imurl::write(ofstream &f) const {
    imevent::write(f);
    f << rusconv(convdirection[direction], url) << endl <<
	rusconv(convdirection[direction], description) << endl;
}

void imurl::read(ifstream &f) {
    string rdbuf;

    getline(f, url);

    while(!f.eof()) {
	getline(f, rdbuf);
	if(rdbuf != "\f") {
	    if(!description.empty()) description += "\n";
	    description += rdbuf;
	} else {
	    break;
	}
    }
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
}

const string imauthorization::gettext() const {
    return text;
}

void imauthorization::write(ofstream &f) const {
    imevent::write(f);
    f << rusconv(convdirection[direction], text) << endl;
}

void imauthorization::read(ifstream &f) {
    string rdbuf;

    while(!f.eof()) {
	getline(f, rdbuf);
	if(rdbuf != "\f") {
	    if(!text.empty()) text += "\n";
	    text += rdbuf;
	} else {
	    break;
	}
    }
}
