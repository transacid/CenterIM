#ifndef __ICQGROUP_H__
#define __ICQGROUP_H__

#include "icqcommon.h"

class icqgroup {
    private:
	int id;
	string name;

    public:
	icqgroup(int aid, const string &aname);
	~icqgroup();

	string getname() { return name; }
	int getid() { return id; }

	void rename(const string &aname) { name = aname; }

	bool operator == (int aid) const;
	bool operator != (int aid) const;
};

#endif
