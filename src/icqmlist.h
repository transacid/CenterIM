#ifndef __ICQMLIST_H__
#define __ICQMLIST_H__

#include "kkstrtext.h"
#include "kkfsys.h"
#include "cmenus.h"

#include "icqcommon.h"
#include "imcontact.h"

enum contactstatus {
    csignore = 1,
    csvisible = 2,
    csinvisible = 3
};

class modelistitem {
    private:
	string nick;
	imcontact cdesc;
	contactstatus cs;

    public:
	modelistitem() {}
	modelistitem(const string nnick, const imcontact adesc, contactstatus ncs):
	    nick(nnick), cdesc(adesc), cs(ncs) {}

	const string getnick() const;
	imcontact getdesc() const;
	contactstatus getstatus() const;

	void setstatus(contactstatus ncs);

	bool operator == (const imcontact &cinfo) const;
	bool operator != (const imcontact &cinfo) const;
};

class icqlist : public vector<modelistitem> {
    private:
	vector<modelistitem> menucontents;

    public:
	icqlist();
	~icqlist();

	void load();
	void save();

	void fillmenu(verticalmenu *m, contactstatus ncs);

	bool inlist(const imcontact cinfo, contactstatus ncs) const;
	void del(const imcontact cinfo, contactstatus ncs);

	modelistitem menuat(int pos) const;
};

extern icqlist lst;

#endif
