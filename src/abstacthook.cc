#include "abstracthook.h"

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

imstatus abstracthook::getstatus() const {
    return offline;
}

// ----------------------------------------------------------------------------

abstracthook &gethook(protocolname pname) {
    switch(pname) {
	case icq: return ihook; break;
	case yahoo: return yhook; break;
    }

    return abstracthook();
}
