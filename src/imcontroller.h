#ifndef __IMCONTROLLER_H__
#define __IMCONTROLLER_H__

#include "icqconf.h"

struct imsearchparams {
    imsearchparams() {
	onlineonly = false;
	uin = 0;
	minage = maxage = country = language = 0;
	gender = genderUnspec;
    };

    bool onlineonly;
    unsigned int uin;
    unsigned short minage, maxage, country, language;
    imgender gender;
    string firstname, lastname, nick, city, state;
    string company, department, position, email;
    protocolname pname;
};

class imcontroller {
    protected:
	unsigned int ruin;
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
