#ifndef __ABSTRACTHOOK_H__
#define __ABSTRACTHOOK_H__

#include "icqcontact.h"

class abstracthook {
    protected:
    public:
	virtual void connect();
	virtual void disconnect();
	virtual void exectimers();
	virtual void main();

	virtual int getsockfd() const;
	virtual bool online() const;
	virtual bool logged() const;
	virtual bool isconnecting() const;
	virtual bool enabled() const;

	virtual unsigned long sendmessage(const icqcontact *c,
	    const string text);

	virtual void setautostatus(imstatus st);
	virtual void setstatus(imstatus st);
	virtual imstatus getstatus() const;
};

abstracthook &gethook(protocolname pname);

#endif
