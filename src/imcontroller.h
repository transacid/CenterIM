#ifndef __IMCONTROLLER_H__
#define __IMCONTROLLER_H__

#include "icqconf.h"

#include <libicq2000/userinfoconstants.h>

struct imsearchparams {
    imsearchparams() {
	onlineonly = sincelast = reverse = false;
	uin = 0;
	minage = maxage = country = language = randomgroup = 0;
	gender = genderUnspec;
	agerange = ICQ2000::range_NoRange;
    };

    imsearchparams(const string &name) {
	load(name);
    };

    protocolname pname;
    bool onlineonly, sincelast, reverse;
    unsigned int uin;
    unsigned short minage, maxage, country, language, randomgroup;
    ICQ2000::AgeRange agerange;
    imgender gender;

    string firstname, lastname, nick, city, state, kwords;
    string company, department, position, email, room;

    string service;
    vector<pair<string, string> > flexparams;

    void save(const string &prname) const;
    bool load(const string &prname);
};

class imcontroller {
    protected:
	unsigned int ruin;
	string rnick, rfname, rlname, remail, rpasswd, rserver;

	void synclist(protocolname pname);

    protected:
	bool regdialog(protocolname pname);

	bool icqregistration(icqconf::imaccount &account);
	bool jabberregistration(icqconf::imaccount &account);

	void icqupdatedetails();
	void aimupdateprofile();
	void msnupdateprofile();
	void ircchannels();

	void icqsynclist();
	void yahoosynclist();

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
