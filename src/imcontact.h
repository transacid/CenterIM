#ifndef __imcontact_H__
#define __imcontact_H__

#include "icqcommon.h"

enum protocolname {
    icq = 0,
    yahoo,
    infocard
};

struct imcontact {
    imcontact();
    imcontact(unsigned long auin, protocolname apname);
    imcontact(const string anick, protocolname apname);

    string nickname, pass;
    unsigned long uin;
    protocolname pname;

    bool operator == (const imcontact &ainfo) const;
    bool operator != (const imcontact &ainfo) const;

    bool operator == (protocolname pname) const;
    bool operator != (protocolname pname) const;

    bool empty() const;
    const string totext() const;
    const string getshortservicename() const;
};

extern imcontact contactroot;

#endif
