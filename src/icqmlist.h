#ifndef __ICQMLIST_H__
#define __ICQMLIST_H__

#include "kkstrtext.h"
#include "kkfsys.h"
#include "cmenus.h"
#include "linkedlist.h"

#include "icqcommon.h"

enum contactstatus {csignore = 1, csvisible = 2, csinvisible = 3};

class icqlistitem {
    private:
	string nick;
	unsigned int uin;
	contactstatus cs;

    public:
	icqlistitem(string nnick, unsigned int nuin, contactstatus ncs):
	    nick(nnick), uin(nuin), cs(ncs) { }
	string getnick()                    { return nick; }
	unsigned int getuin()               { return uin; }
	contactstatus getstatus()           { return cs; }
	void setstatus(contactstatus ncs)   { cs = ncs; }
};

class icqlist : public linkedlist {
    private:
	linkedlist morder;

    public:
	icqlist();
	~icqlist();
	
	void load();
	void save();
	void fillmenu(verticalmenu *m, contactstatus ncs);

	bool inlist(unsigned int uin, contactstatus ncs);
	void del(unsigned int uin, contactstatus ncs);
	
	icqlistitem *menuat(int pos);
};

extern icqlist lst;

#endif
