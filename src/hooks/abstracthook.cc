#include "abstracthook.h"

#include "icqhook.h"
#include "yahoohook.h"
#include "msnhook.h"

#include <time.h>

abstracthook::abstracthook() {
    fcapabilities = 0;
    searchdest = 0;
}

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

void abstracthook::sendnewuser(const imcontact c) {
}

imstatus abstracthook::getstatus() const {
    return offline;
}

bool abstracthook::isdirectopen(const imcontact c) const {
    return false;
}

void abstracthook::removeuser(const imcontact ic) {
}

int abstracthook::getcapabilities() const {
    return fcapabilities;
}

void abstracthook::requestinfo(const imcontact c) {
}

void abstracthook::lookup(const imsearchparams &params, verticalmenu &dest) {
}

void abstracthook::stoplookup() {
    searchdest = 0;
}

// ----------------------------------------------------------------------------

abstracthook &gethook(protocolname pname) {
    static abstracthook ahook;

    switch(pname) {
	case icq:
	    return ihook;
	case yahoo:
	    return yhook;
	case msn:
	    return mhook;
    }

    return ahook;
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

/*
*
* Timezone related function
* The code was taken from licq
*
*/

char getsystemtimezone() {
    time_t t = time(NULL);
    struct tm *tzone = localtime(&t);
    int nTimezone = 0;

    nTimezone = timezone + (tzone->tm_isdst == 1 ? 3600 : 0);
    nTimezone /= 900;
    if(nTimezone > 23) return 23-nTimezone;

    return nTimezone;
}
