#ifndef __IMCONTROLLER_H__
#define __IMCONTROLLER_H__

#include "icqconf.h"

class imcontroller {
    protected:
	unsigned long ruin;
	string rnick, rfname, rlname, remail, rpasswd, rserver;

    protected:
	bool icqregdialog();

	bool icqregistration(icqconf::imaccount &account);
	void icqupdatedetails();

    public:
	imcontroller();
	~imcontroller();

	void registration(icqconf::imaccount &account);
	void updateinfo(icqconf::imaccount &account);
};

extern imcontroller imcontrol;

#endif
