#ifndef __IMCONTROLLER_H__
#define __IMCONTROLLER_H__

#include "icqconf.h"

#include <libicq2000/userinfoconstants.h>

struct imsearchparams {
    imsearchparams() {
	onlineonly = false;
	uin = 0;
	minage = maxage = country = language = 0;
	gender = genderUnspec;
	agerange = ICQ2000::range_NoRange;
    };

    bool onlineonly;
    unsigned int uin;
    unsigned short minage, maxage, country, language;
    ICQ2000::AgeRange agerange;
    imgender gender;
    string firstname, lastname, nick, city, state;
    string company, department, position, email, room;
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
	void aimupdateprofile();
	void ircchannels();

    public:
	imcontroller();
	~imcontroller();

	void registration(icqconf::imaccount &account);
	void updateinfo(icqconf::imaccount &account);
	void channels(icqconf::imaccount &account);
};

extern imcontroller imcontrol;

#endif
