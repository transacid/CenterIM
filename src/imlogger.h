#ifndef __IMLOGGER_H__
#define __IMLOGGER_H__

#include "icqcommon.h"
#include "imevents.h"

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

const char *streventname(imevent::imeventtype type);

#endif
