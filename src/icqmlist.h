#ifndef __ICQMLIST_H__
#define __ICQMLIST_H__

#include "kkstrtext.h"
#include "cmenus.h"

#include "icqcommon.h"
#include "imcontact.h"

enum contactstatus {
    csnone = -1,
    csignore = 1,
    csvisible,
    csinvisible
};

class modelistitem {
    private:
	string nick;
	imcontact cdesc;
	contactstatus cs;

    public:
	modelistitem() {}
	modelistitem(const string &nnick, const imcontact &adesc, contactstatus ncs):
	    nick(nnick), cdesc(adesc), cs(ncs) {}

	string getnick() const;
	imcontact getdesc() const;
	contactstatus getstatus() const;

	void setstatus(contactstatus ncs);

	bool operator == (const imcontact &cinfo) const;
	bool operator != (const imcontact &cinfo) const;
};

class icqlist : public vector<modelistitem> {
    private:
	vector<modelistitem> menucontents;
	string getfname() const;

    public:
	icqlist();
	~icqlist();

	void load();
	void save();

	void fillmenu(verticalmenu *m, contactstatus ncs);

	bool inlist(const imcontact &cinfo, contactstatus ncs) const;
	void del(const imcontact &cinfo, contactstatus ncs);

	modelistitem menuat(int pos) const;
};

extern icqlist lst;

#endif
