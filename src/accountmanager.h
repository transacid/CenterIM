#ifndef __ACCOUNTMANAGER_H__
#define __ACCOUNTMANAGER_H__

#include "icqconf.h"

class accountmanager {
    protected:
    public:
	accountmanager();
	~accountmanager();

	void exec();
};

extern accountmanager manager;

#endif
