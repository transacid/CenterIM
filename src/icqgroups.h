#ifndef __ICQGROUPS_H__
#define __ICQGROUPS_H__

#include "icqgroup.h"

class icqgroups: private vector<icqgroup> {
    private:
    public:
	icqgroups();
	~icqgroups();

	const string getfname() const;

	void load();
	void save();

	int add(const string aname);
	void remove(int gid);

	iterator begin()
	    { return vector<icqgroup>::begin(); }
	iterator end()
	    { return vector<icqgroup>::end(); }
};

extern icqgroups groups;

#endif
