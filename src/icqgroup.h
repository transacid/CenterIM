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
	
    public:
	icqgroup(int aid, const string &aname);
	~icqgroup();

	string getname() { return name; }
	int getid() { return id; }
	
	int getcount(bool countonline, bool countoffline);

	bool iscollapsed() { return collapsed; }
	void changecollapsed();
			
	void rename(const string &aname) { name = aname; }

	bool operator == (int aid) const;
	bool operator != (int aid) const;
};

#endif
