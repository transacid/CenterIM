#ifndef __ABSTRACTHOOK_H__
#define __ABSTRACTHOOK_H__

#include "imcontact.h"
#include "imevents.h"
#include "imcontroller.h"

struct hookcapab {
    enum enumeration {
	synclist,
	urls,
	files,
	contacts,
	authrequests,
	authreqwithmessages,
	fetchaway,
	setaway,
	changenick,
	changepassword,
	changedetails,
	changeabout,
	optionalpassword,
	visibility,
	version,
	ping,
	conferencing,
	conferencesaretemporary,
	cltemporary,
	directadd,
	flexiblesearch,
	flexiblereg,
	ssl,
	channelpasswords,
	groupchatservices,
	nochat,
    };
};

struct servicetype {
    enum enumeration {
	search,
	registration,
	groupchat
    };
};

struct groupoperation {
    enum enumeration {
	add,
	remove,
	rename
    };
};

class abstracthook {
    protected:
	enum Encoding {
	    encUTF, encKOI, encUnknown
	};

	protocolname proto;
	imstatus manualstatus;
	verticalmenu *searchdest;
	set<hookcapab::enumeration> fcapabs;

	string rusconv(const string &tdir, const string &text);
	string rushtmlconv(const string &tdir, const string &text, bool rus = true);
	string ruscrlfconv(const string &tdir, const string &text);

	string getmd5(const string &text);

    public:
	abstracthook(protocolname aproto);

	virtual void init();

	virtual void connect();
	virtual void disconnect();
	virtual void exectimers();
	virtual void main();

	virtual void getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const;
	virtual bool isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const;

	virtual bool online() const;
	virtual bool logged() const;
	virtual bool isconnecting() const;
	virtual bool enabled() const;

	virtual bool send(const imevent &ev);

	virtual void sendnewuser(const imcontact &c);
	virtual void removeuser(const imcontact &ic);

	virtual void setautostatus(imstatus st);
	virtual void restorestatus();

	virtual void setstatus(imstatus st);
	virtual imstatus getstatus() const;

	virtual bool isdirectopen(const imcontact &c) const;
	virtual void requestinfo(const imcontact &c);

	virtual void lookup(const imsearchparams &params, verticalmenu &dest);
	virtual void stoplookup();

	virtual void requestawaymsg(const imcontact &c);
	virtual void requestversion(const imcontact &c);
	virtual void ping(const imcontact &c);

	set<hookcapab::enumeration> getCapabs() const
	    { return fcapabs; }

	virtual vector<icqcontact *> getneedsync();
	virtual void ouridchanged(const icqconf::imaccount &ia);

	virtual bool knowntransfer(const imfile &fr) const;
	virtual void replytransfer(const imfile &fr, bool accept,
	    const string &localpath = string());
	virtual void aborttransfer(const imfile &fr);

	virtual void conferencecreate(const imcontact &confid,
	    const vector<imcontact> &lst);

	virtual vector<string> getservices(servicetype::enumeration st) const;

	virtual vector<pair<string, string> > getsearchparameters(const string &agentname) const;
	virtual vector<pair<string, string> > getregparameters(const string &agentname) const;

	virtual void updatecontact(icqcontact *c);
	virtual void renamegroup(const string &oldname, const string &newname);
};

abstracthook &gethook(protocolname pname);
struct tm *maketm(int hour, int minute, int day, int month, int year);

extern time_t timer_current;

#endif
