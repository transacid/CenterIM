#ifndef __imcontact_H__
#define __imcontact_H__

#include "icqcommon.h"

enum protocolname {
    icq,
    yahoo,
    infocard
};

struct imcontact {
    imcontact();
    imcontact(unsigned long auin, protocolname apname);
    imcontact(const string anick, protocolname apname);

    string nickname;
    unsigned long uin;
    protocolname pname;

    bool operator == (const imcontact &ainfo) const;
    bool operator != (const imcontact &ainfo) const;

    bool empty() const;
};

extern imcontact contactroot;

#endif
