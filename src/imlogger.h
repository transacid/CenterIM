#ifndef __IMLOGGER_H__
#define __IMLOGGER_H__

#include "icqcommon.h"
#include "imevents.h"

static const char* imstatus2name[imstatus_size] = {
    _("Offline"),
    _("Online"),
    _("Invisible"),
    _("Free for chat"),
    _("DND"),
    _("Occupied"),
    _("N/A"),
    _("Away")
};

class imlogger {
    protected:
	ofstream f;

	bool checkopen();

    public:
	imlogger();
	~imlogger();

	void putmessage(const string &text);
	void putevent(const imevent &event);

	void putonline(const imcontact &cont,
	    const imstatus &oldst, const imstatus &st);

	void putourstatus(const protocolname &pname,
	    const imstatus &oldst, const imstatus &st);
};

extern imlogger logger;

static const char *eventnames[imevent::imeventtype_size] = {
    _("message"), _("URL"), _("SMS"), _("authorization"), "", _("e-mail")
};

#endif
