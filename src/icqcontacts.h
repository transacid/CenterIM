#ifndef __ICQCONTACTS_H__
#define __ICQCONTACTS_H__

#include "cmenus.h"

/*
*
* SORT_CONTACTS
*
* O Online
*   o Online
*   f Free for chat
*   i Invisible
*   d Do not disturb
*   c Occupied
*   a Away
*   n N/A
*
* _ Offline
* ! Not in list
* N Non-ICQ
* # Unread
*
*/

#define SORT_CONTACTS   "#fidocan_!N"

#include "linkedlist.h"
#include "icqcontact.h"
#include "icqcommon.h"

class icqcontacts: public linkedlist {
    protected:
	static int clistsort(void *p1, void *p2);

    public:
	icqcontacts();
	~icqcontacts();

	void remove(unsigned int uin, bool nonicq = false);
	void load();
	void save();
	void send();
	void nonicq(int id);
	void order();

	icqcontact* addnew(unsigned int uin,
	    bool notinlist = true,
	    bool nonicq = false);

	icqcontact *get(unsigned int uin, bool nonicq = false);
	icqcontact *getseq2(unsigned short seq2);
};

extern icqcontacts clist;

#endif
