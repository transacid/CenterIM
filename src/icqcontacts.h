#ifndef __ICQCONTACTS_H__
#define __ICQCONTACTS_H__

/*
*
* SORT_CONTACTS
*
* o Online
* f Free for chat
* i Invisible
* d Do not disturb
* c Occupied
* a Away
* n N/A
*
* _ Offline
* ! Not in list
* N Non-ICQ
* # Unread
*
*/

#define SORT_CONTACTS   "#fidocan_!N"

#define SORTCHAR(c) ( \
    c->getmsgcount() ? '#' : \
    !c->inlist() ? '!' : \
    c->getshortstatus() \
)

#include "cmenus.h"

#include "linkedlist.h"
#include "icqcontact.h"
#include "icqcommon.h"

class icqcontacts: public linkedlist {
    protected:
	static int clistsort(void *p1, void *p2);

    public:
	icqcontacts();
	~icqcontacts();

	void remove(const imcontact adesc);
	void load();
	void save();
	void nonicq(int id);
	void order();
	void rearrange();
	void checkdefault();

	void setoffline(protocolname pname);

	icqcontact* addnew(const imcontact adesc, bool notinlist = true);

	icqcontact *get(const imcontact adesc);
};

extern icqcontacts clist;

#endif
