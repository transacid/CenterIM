#ifndef __ICQGROUP_H__
#define __ICQGROUP_H__

#include "icqcommon.h"
#include "icqcontacts.h"
#include "imcontact.h"

class icqgroup {
    private:
	int id;
	string name;
	bool collapsed;

	void exchange(int nid);

    public:
	icqgroup(int aid, const string &aname);
	~icqgroup();

	string getname() { return name; }
	int getid() { return id; }

	int getcount(bool countonline, bool countoffline);

	bool iscollapsed() { return collapsed; }
	void changecollapsed() { collapsed = !collapsed; }

	void moveup();
	void movedown();

	void rename(const string &aname) { name = aname; }

	bool operator == (int aid) const { return id == aid; }
	bool operator != (int aid) const { return !(*this == aid); }

	bool operator < (const icqgroup &ag) const { return id < ag.id; }
	bool operator > (const icqgroup &ag) const { return id > ag.id; }
};

#endif
