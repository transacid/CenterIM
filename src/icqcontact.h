#ifndef __ICQCONTACT_H_
#define __ICQCONTACT_H_

#include "icqcommon.h"

#include <sys/stat.h>
#include <dirent.h>

#include "kkstrtext.h"
#include "kkfsys.h"
#include "cmenus.h"

#include "imcontact.h"
#include "imevents.h"

enum imgender {
    genderUnspec,
    genderMale,
    genderFemale,
    imgender_size
};

class icqcontact {
    public:
	struct basicinfo {
	    basicinfo(): zip(0) {};

	    string fname, lname, email, city, state, phone, fax, street, cellular;
	    unsigned int zip;
	    unsigned short country;
	};

	struct moreinfo {
	    moreinfo(): gender(genderUnspec), age(0) {};

	    unsigned char age;
	    imgender gender;
	    string homepage;
	    unsigned int birth_day, birth_month, birth_year;
	    unsigned short lang1, lang2, lang3;

	    const string strbirthdate() const;
	};

	struct workinfo {
	    workinfo(): zip(0) {};

	    string city, state, phone, fax, street, company;
	    string dept, position, homepage;
	    unsigned int zip;
	    unsigned short country;
	};

    protected:
	imcontact cdesc;
	imstatus status;

	int nmsgs, fupdated, groupid;
	bool finlist;
	time_t lastread, lastseen;

	string sound[imevent::imeventtype_size];
	string nick, about, dispnick, postponed, lastip;

	basicinfo binfo;
	moreinfo minfo;
	workinfo winfo;
	vector<string> interests, background;

	void scanhistory();
	const string tosane(const string p) const;

    public:
	icqcontact(imcontact adesc);
	~icqcontact();

	void setstatus(imstatus fstatus);
	void setlastread(time_t flastread);
	void setlastseen();

	void setnick(const string fnick);
	void setdispnick(const string fnick);

	void setbasicinfo(const basicinfo &ainfo);
	void setmoreinfo(const moreinfo &ainfo);
	void setworkinfo(const workinfo &ainfo);
	void setinterests(const vector<string> &ainterests);
	void setbackground(const vector<string> &abackground);
	void setabout(const string data);
	void setlastip(const string flastip);

	const basicinfo &getbasicinfo() const;
	const moreinfo &getmoreinfo() const;
	const workinfo &getworkinfo() const;
	const vector<string> &getinterests() const;
	const vector<string> &getbackground() const;
	const string getabout() const;
	const string getlastip() const;

	time_t getlastread() const;
	time_t getlastseen() const;
	
	imstatus getstatus() const;
	int getmsgcount() const;

	const string getnick() const;
	const string getdispnick() const;

	int updated() const;
	void unsetupdated();
	void setmsgcount(int n);

	void setsound(imevent::imeventtype event, const string sf);
	void playsound(imevent::imeventtype event) const;

	void clear();
	void load();
	void save();
	void remove();
	void excludefromlist();
	void includeintolist();
	bool inlist() const;

	bool isbirthday() const;
	const string getdirname() const;

	char getshortstatus() const;

	void setpostponed(const string apostponed);
	const string getpostponed() const;

	void setgroupid(int agroupid);
	int getgroupid() const;

	const imcontact getdesc() const;

	bool operator > (const icqcontact &acontact) const;
};

#endif
