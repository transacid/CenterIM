#ifndef __imcontact_H__
#define __imcontact_H__

#include "icqcommon.h"

enum protocolname {
    icq = 0,
    yahoo,
    msn,
    aim,
    irc,
    jabber,
    rss,
    livejournal,
    gadu,
    infocard,

    protocolname_size
};

ENUM_PLUSPLUS(protocolname)

enum imstatus {
    offline = 0,
    available,
    invisible,
    freeforchat,
    dontdisturb,
    occupied,
    notavail,
    away,
    imstatus_size
};

ENUM_PLUSPLUS(imstatus)

static char imstatus2char[imstatus_size] = {
    '_', 'o', 'i', 'f', 'd', 'c', 'n', 'a'
};

class icqcontact;

struct imcontact {
    imcontact();
    imcontact(unsigned long auin, protocolname apname);
    imcontact(const string &anick, protocolname apname);
    imcontact(const icqcontact *c);

    string nickname;
    unsigned long uin;
    protocolname pname;

    bool operator == (const imcontact &ainfo) const;
    bool operator != (const imcontact &ainfo) const;

    bool operator == (protocolname pname) const;
    bool operator != (protocolname pname) const;

    bool empty() const;
    string totext() const;
};

extern imcontact contactroot;

bool ischannel(const imcontact &cont);
bool islivejournal(const icqcontact *c);
bool islivejournal(const imcontact &cont);
string imstatus2str(imstatus st);

#endif
