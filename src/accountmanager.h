#ifndef __ACCOUNTMANAGER_H__
#define __ACCOUNTMANAGER_H__

#include "icqconf.h"

class accountmanager {
    protected:
	icqconf::imaccount addcontact();

    public:
	accountmanager();
	~accountmanager();

	void exec();
};

extern accountmanager manager;

#endif
