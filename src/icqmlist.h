#ifndef __ICQMLIST_H__
#define __ICQMLIST_H__

#include "kkstrtext.h"
#include "kkfsys.h"
#include "cmenus.h"

#include "icqcommon.h"
#include "contactinfo.h"

enum contactstatus {
    csignore = 1,
    csvisible = 2,
    csinvisible = 3
};

class modelistitem {
    private:
	string nick;
	contactinfo cdesc;
	contactstatus cs;

    public:
	modelistitem() {}
	modelistitem(const string nnick, const contactinfo adesc, contactstatus ncs):
	    nick(nnick), cdesc(adesc), cs(ncs) {}

	const string getnick() const;
	contactinfo getdesc() const;
	contactstatus getstatus() const;

	void setstatus(contactstatus ncs);

	bool operator == (const contactinfo &cinfo) const;
	bool operator != (const contactinfo &cinfo) const;
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

	bool inlist(const contactinfo cinfo, contactstatus ncs) const;
	void del(const contactinfo cinfo, contactstatus ncs);

	modelistitem menuat(int pos) const;
};

extern icqlist lst;

#endif
