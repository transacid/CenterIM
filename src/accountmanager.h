#ifndef __ACCOUNTMANAGER_H__
#define __ACCOUNTMANAGER_H__

#include "icqconf.h"

class accountmanager {
    protected:
	icqconf::imaccount addcontact();
	bool fopen;

    public:
	accountmanager();
	~accountmanager();

	void exec();

	bool isopen() const;
};

extern accountmanager manager;

#endif
