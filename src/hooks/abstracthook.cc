#include "abstracthook.h"
#include "icqhook.h"
#include "yahoohook.h"

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

void abstracthook::setstatus(imstatus st) {
    setautostatus(manualstatus = st);
}

void abstracthook::getsockets(fd_set &fds, int &hsocket) const {
}

bool abstracthook::isoursocket(fd_set &fds) const {
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

unsigned long abstracthook::sendmessage(const icqcontact *c,
const string text) {
    return 0;
}

void abstracthook::sendnewuser(const imcontact c) {
}

imstatus abstracthook::getstatus() const {
    return offline;
}

bool abstracthook::isdirectopen(const imcontact c) const {
    return false;
}

// ----------------------------------------------------------------------------

abstracthook &gethook(protocolname pname) {
    static abstracthook ahook;

    switch(pname) {
	case icq:
	    return ihook;
	case yahoo:
	    return yhook;
	default:
	    return ahook;
    }
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
