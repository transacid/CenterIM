#ifndef __IMCONTROLLER_H__
#define __IMCONTROLLER_H__

#include "icqconf.h"

#include <libicq2000/userinfoconstants.h>

struct imsearchparams {
    imsearchparams() {
	onlineonly = sincelast = false;
	uin = 0;
	minage = maxage = country = language = randomgroup = 0;
	gender = genderUnspec;
	agerange = ICQ2000::range_NoRange;
    };

    imsearchparams(const string &name) {
	load(name);
    };

    protocolname pname;
    bool onlineonly, sincelast;
    unsigned int uin;
    unsigned short minage, maxage, country, language, randomgroup;
    ICQ2000::AgeRange agerange;
    imgender gender;
    string firstname, lastname, nick, city, state, kwords;
    string company, department, position, email, room;

    void save(const string &prname) const;
    bool load(const string &prname);
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

	void icqsynclist();
	void sendauthorization(vector<icqcontact *> &needauth);

    public:
	imcontroller();
	~imcontroller();

	void registration(icqconf::imaccount &account);
	void updateinfo(icqconf::imaccount &account);
	void channels(icqconf::imaccount &account);
	void synclist(icqconf::imaccount &account);
};

extern imcontroller imcontrol;

#endif
