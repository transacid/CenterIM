#ifndef __IMCONTROLLER_H__
#define __IMCONTROLLER_H__

#include "icqconf.h"

#include <libicq2000/userinfoconstants.h>

struct imsearchparams {
    imsearchparams() {
	onlineonly = sincelast = reverse = false;
	uin = checkfrequency = 0;
	minage = maxage = country = language = randomgroup = 0;
	gender = genderUnspec;
	agerange = ICQ2000::range_NoRange;
    };

    imsearchparams(const string &name) {
	load(name);
    };

    protocolname pname;
    bool onlineonly, sincelast, reverse;
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

struct ljparams {
    enum ljsecurity {
	sPublic = 0,
	sPrivate
    };

    string journal, subj, mood, music, picture;
    bool noformat, nocomments, backdated, noemail;
    ljsecurity security;

    ljparams()
	: noformat(false), nocomments(false), backdated(false),
	  noemail(false), security(sPublic) { }

    bool empty() {
	return subj.empty() && mood.empty() && music.empty() && picture.empty();
    }

    string serialize() const {
	string r;
	r += journal + "\t";
	r += mood + "\t";
	r += music + "\t";
	r += picture + "\t";
	r += (string) (noformat ? "1" : "0") + "\t";
	r += (string) (nocomments ? "1" : "0") + "\t";
	r += (string) (backdated ? "1" : "0") + "\t";
	r += (string) (noemail ? "1" : "0") + "\t";
	r += i2str((int) security);
	return r;
    }

    void unserialize(string r) {
	int i;
	string param;

	for(i = 0; i < 9 && !r.empty(); i++) {
	    param = getword(r, "\t");
	    switch(i) {
		case 0: journal = param; break;
		case 1: mood = param; break;
		case 2: music = param; break;
		case 3: picture = param; break;
		case 4: noformat = param == "1"; break;
		case 5: nocomments = param == "1"; break;
		case 6: backdated = param == "1"; break;
		case 7: noemail = param == "1"; break;
		case 8: security = (ljsecurity) atoi(param.c_str()); break;
	    }
	}
    }
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
	void jabberupdateprofile();

	void icqsynclist();
	void yahoosynclist();

    public:
	imcontroller();
	~imcontroller();

	void registration(icqconf::imaccount &account);
	void updateinfo(icqconf::imaccount &account);
	void synclist(icqconf::imaccount &account);
};

extern imcontroller imcontrol;

#endif
