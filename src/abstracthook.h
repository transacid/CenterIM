#ifndef __ABSTRACTHOOK_H__
#define __ABSTRACTHOOK_H__

class abstracthook {
    protected:
    public:
	virtual void connect() = 0;
	virtual void disconnect() = 0;
	virtual void exectimers() = 0;

	virtual int getsockfd() const;
	virtual bool online() const;
	virtual bool logged() const;
	virtual bool isconnecting() const;
	virtual bool enabled() const;

	virtual unsigned long sendmessage(const icqcontact *c,
	    const string text);

	virtual void setautostatus(imstatus st) = 0;
	virtual void setstatus(imstatus st) = 0;
	virtual imstatus getstatus() const;
};

abstracthook &gethook(protocolname pname);

#endif
