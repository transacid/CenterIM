#ifndef __ABSTRACTHOOK_H__
#define __ABSTRACTHOOK_H__

#include "imcontact.h"
#include "imevents.h"
#include "imcontroller.h"

enum hookcapabilities {
	    hoptCanSyncList = 2,
	     hoptCanSendURL = 4,
	    hoptCanSendFile = 8,
	     hoptCanSendSMS = 16,
	hoptCanFetchAwayMsg = 32,
	  hoptCanSetAwayMsg = 64,
	  hoptCanChangeNick = 128,
	hoptChangableServer = 256,
      hoptCanChangePassword = 512,
       hoptCanUpdateDetails = 1024,
	    hoptNoPasswords = 2048,
  hoptControlableVisibility = 4096,
	    hoptAuthReqSend = 8192
};

class abstracthook {
    protected:
	imstatus manualstatus;
	int fcapabilities;
	verticalmenu *searchdest;

    public:
	abstracthook();

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

	int getcapabilities() const;
};

abstracthook &gethook(protocolname pname);
struct tm *maketm(int hour, int minute, int day, int month, int year);

#endif
