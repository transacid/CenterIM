#ifndef __IMCONTROLLER_H__
#define __IMCONTROLLER_H__

#include "icqconf.h"

#include <libicq2000/userinfoconstants.h>

struct imsearchparams {
    imsearchparams() {
	onlineonly = sincelast = reverse = photo = false;
	uin = checkfrequency = 0;
	minage = maxage = country = language = randomgroup = 0;
	gender = genderUnspec;
	agerange = ICQ2000::RANGE_NORANGE;
    };

    imsearchparams(const string &name) {
	load(name);
    };

    protocolname pname;
    bool onlineonly, sincelast, reverse, photo;
    unsigned int uin, checkfrequency;
    unsigned short minage, maxage, country, language, randomgroup;
    ICQ2000::AgeRange agerange;
    imgender gender;

    string firstname, lastname, nick, city, state, kwords, url;
    string company, department, position, email, room, password;

    string service;
    vector<pair<string, string> > flexparams;

    void save(const string &prname) const;
    bool load(const string &prname);
};

class imcontroller {
    protected:
	unsigned int ruin;
	string rnick, rfname, rlname, remail, rpasswd, rserver;

    protected:
	bool regdialog(protocolname pname);

	bool uinreg(icqconf::imaccount &account);
	bool jabberregistration(icqconf::imaccount &account);

	void icqupdatedetails();
	void aimupdateprofile();
	void msnupdateprofile();
	void jabberupdateprofile();
	void gaduupdateprofile();

    public:
	imcontroller();
	~imcontroller();

	void registration(icqconf::imaccount &account);
	void updateinfo(icqconf::imaccount &account);
};

extern imcontroller imcontrol;

#endif
