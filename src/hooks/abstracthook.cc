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
}

int abstracthook::getsockfd() const {
    return 0;
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
