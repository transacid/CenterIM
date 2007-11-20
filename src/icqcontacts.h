#ifndef __ICQCONTACTS_H__
#define __ICQCONTACTS_H__


#include "cmenus.h"

#include "icqconf.h"
#include "linkedlist.h"
#include "icqcontact.h"
#include "icqcommon.h"

// a structure to keep the two methods used for sorting
struct icqcontacts_sort {
    /** the first sort is done by status (sortstatus function)
     * that returns a custom character for each contact and
     * and the order of status is found in icqcontacts.cc
     * see SORT_CONTACTS define
     */
    char (*sortstatus)(const icqcontact& a);
    /** the second sort is done when the contacts have
     * the same status category as returned by sortstatus
     * and they are compared with the "compare" function.
     */
    int (*compare)(const icqcontact& a, const icqcontact& b);
};

class icqcontacts: public linkedlist {
    protected:
	static int clistsort(void *p1, void *p2);
    public:
	static void setsortmode(icqconf::sortmode smode);
	static icqcontacts_sort* sort_order;

    public:
	icqcontacts();
	~icqcontacts();

	void remove(const imcontact &adesc);
	void load();
	void save(bool removenil = true);
	void order();
	void rearrange();

	void setoffline(protocolname pname);

	icqcontact* addnew(const imcontact &adesc,
	    bool notinlist = true, int agroupid = 0,
	    bool reqauth = false);

	icqcontact *get(const imcontact &adesc);
	icqcontact *getmobile(const string &anumber);
	icqcontact *getemail(const string &aemail);

	void updateEntry(const imcontact &ic, const string &groupname);
};

extern icqcontacts clist;

#endif
