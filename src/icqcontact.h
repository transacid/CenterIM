#ifndef __ICQCONTACT_H_
#define __ICQCONTACT_H_

#include "icqcommon.h"

#include <sys/stat.h>
#include <dirent.h>

#include "cmenus.h"

#include "imcontact.h"
#include "imevents.h"

enum imgender {
    genderUnspec = 0,
    genderMale,
    genderFemale,
    imgender_size
};

ENUM_PLUSPLUS(imgender)

class icqcontact {
    public:
	struct basicinfo {
	    basicinfo()
		: country(0), requiresauth(false), webaware(false),
		  authawait(false), randomgroup(0) { };

	    string fname, lname, email, city, state, phone, fax;
	    string street, cellular, zip;
	    unsigned short country, randomgroup;
	    bool requiresauth, webaware, authawait;
	};

	struct moreinfo {
	    moreinfo():
		gender(genderUnspec), age(0), birth_day(0),
		birth_month(0), birth_year(0), lang1(0), lang2(0), lang3(0),
		timezone(0), checkfreq(0), checklast(0) {};

	    unsigned char age;
	    imgender gender;
	    string homepage;
	    unsigned int birth_day, birth_month, birth_year;
	    unsigned short lang1, lang2, lang3;
	    signed char timezone;

	    string strbirthdate() const;
	    string strtimezone() const;

	    int checkfreq, checklast;
	};

	struct workinfo {
	    workinfo(): country(0) {};

	    string city, state, phone, fax, street, company;
	    string dept, position, homepage, zip;
	    unsigned short country;
	};

	struct reginfo {
	    string service, password;
	    vector<pair<string, string> > params;
	};

    protected:
	imcontact cdesc;
	imstatus status;

	int fupdated, groupid, fhistoffset;
	bool finlist, congratulated, modified, fhasevents, usepgpkey, openedforchat;
	time_t lastread, lastseen, lasttyping;

	string sound[imevent::imeventtype_size];
	string nick, about, dispnick, postponed, lastip, pgpkey;

	basicinfo binfo;
	moreinfo minfo;
	workinfo winfo;
	reginfo rinfo;
	vector<string> interests, background;

	void scanhistory();
	string tosane(const string &p) const;

    public:
	icqcontact(imcontact adesc);
	~icqcontact();

	void setstatus(imstatus fstatus, bool reflect = true);
	void setlastread(time_t flastread);
	void setlastseen();

	void setnick(const string &fnick);
	void setdispnick(const string &fnick);

	void setbasicinfo(const basicinfo &ainfo);
	void setmoreinfo(const moreinfo &ainfo);
	void setworkinfo(const workinfo &ainfo);
	void setreginfo(const reginfo &ainfo);
	void setinterests(const vector<string> &ainterests);
	void setbackground(const vector<string> &abackground);
	void setabout(const string &data);
	void setlastip(const string &flastip);

	const basicinfo &getbasicinfo() const { return binfo; }
	const moreinfo &getmoreinfo() const { return minfo; }
	const workinfo &getworkinfo() const { return winfo; }
	const reginfo &getreginfo() const { return rinfo; }
	const vector<string> &getinterests() const { return interests; }
	const vector<string> &getbackground() const { return background; }

	string getabout() const;
	string getlastip() const;

	time_t getlastread() const;
	time_t getlastseen() const;
	
	imstatus getstatus() const;

	bool hasevents() const { return fhasevents; }
	void sethasevents(bool n) { fhasevents = n; }
	bool isopenedforchat() const { return openedforchat; }
	bool setopenedforchat(bool n) { openedforchat = n; }
	bool toggleopenedforchat() { openedforchat = !openedforchat; }

	string getnick() const;
	string getdispnick() const;

	int updated() const;
	void unsetupdated();

	void setsound(imevent::imeventtype event, const string &sf);
	void playsound(imevent::imeventtype event) const;

	void clear();
	void load();
	void save();
	void remove();
	void excludefromlist();
	void includeintolist(int agroupid, bool reqauth);
	void includeintolist();
	bool inlist() const;

	bool isbirthday() const;
	string getdirname() const;

	char getshortstatus() const;

	void setpostponed(const string &apostponed);
	string getpostponed() const;

	int gethistoffset() const;
	void sethistoffset(int aoffset);

	void setgroupid(int agroupid, bool reflect = true);
	int getgroupid() const;

	const imcontact getdesc() const;
	void setdesc(const imcontact &ic);

	void remindbirthday(bool r);

	bool operator > (const icqcontact &acontact) const;

	time_t getlasttyping() const { return lasttyping; }
	void setlasttyping(time_t t) { lasttyping = t; }

	string getpgpkey() const;
	void setpgpkey(const string &key);

	bool getusepgpkey() const;
	void setusepgpkey(bool usekey);
};

#endif
