#ifndef __ABSTRACTHOOK_H__
#define __ABSTRACTHOOK_H__

#include "imcontact.h"
#include "imevents.h"
#include "imcontroller.h"

struct hookcapab {
    enum enumeration {
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
	pgp
    };
};

struct servicetype {
    enum enumeration {
	search,
	registration,
	groupchat
    };
};

class abstracthook {
    protected:
	enum Encoding {
	    encUTF, encKOI, encUnknown
	};

	enum logevent {
	    logConnecting,
	    logLogged,
	    logSearchFinished,
	    logPasswordChanged,
	    logDisconnected,
	    logContactAdd,
	    logContactRemove,
	    logConfMembers
	};

	protocolname proto;
	imstatus manualstatus;
	verticalmenu *searchdest;
	set<hookcapab::enumeration> fcapabs;
	vector<icqcontact *> foundguys;

	string rusconv(const string &tdir, const string &text);
	string rushtmlconv(const string &tdir, const string &text, bool rus = true);
	string ruscrlfconv(const string &tdir, const string &text);

	string getmd5(const string &text);

	void requestfromfound(const imcontact &ic);
	void log(logevent ev, ...);

	struct Country_struct {
	    char *name;
	    unsigned short code;
	};

    public:
	static const unsigned char Language_table_size;
	static const char* const Language_table[];
	static const unsigned short Country_table_size;
	static const Country_struct Country_table[];

	static string getCountryIDtoString(unsigned short id);
	static unsigned short getCountryByName(string name);
	static signed char getSystemTimezone();
	static string getInterestsIDtoString(unsigned char id);
	static string getBackgroundIDtoString(unsigned short id);
	static string getTimezoneIDtoString(signed char id);
	static string getTimezonetoLocaltime(signed char id);
	static string getLanguageIDtoString(unsigned char id);

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

	virtual imstatus getmanualstatus() const
	    { return manualstatus; }

	virtual bool isdirectopen(const imcontact &c) const;
	virtual void requestinfo(const imcontact &c);

	virtual void lookup(const imsearchparams &params, verticalmenu &dest);
	virtual void stoplookup();

	virtual void requestawaymsg(const imcontact &c);
	virtual void requestversion(const imcontact &c);
	virtual void ping(const imcontact &c);

	set<hookcapab::enumeration> getCapabs() const
	    { return fcapabs; }

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

	virtual bool regconnect(const string &aserv);
	virtual bool regattempt(unsigned int &auin, const string &apassword, const string &email);
};

abstracthook &gethook(protocolname pname);
struct tm *maketm(int hour, int minute, int day, int month, int year);

extern time_t timer_current;

#endif
