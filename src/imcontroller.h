#ifndef __IMCONTROLLER_H__
#define __IMCONTROLLER_H__

#include "icqconf.h"

class imcontroller {
    protected:
	unsigned long ruin;
	string rnick, rfname, rlname, remail, rpasswd;

    protected:
	bool icqregdialog(unsigned int &ruin, string &rpasswd);

	void icqregistration(icqconf::imaccount &account);
	void icqupdatedetails(icqconf::imaccount &account);

    public:
	imcontroller();
	~imcontroller();

	void registration(icqconf::imaccount &account);
	void updateinfo(icqconf::imaccount &account);
};

extern imcontroller imcontrol;

#endif
