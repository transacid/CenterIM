#ifndef __ICQCONTACT_H_
#define __ICQCONTACT_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <dirent.h>

#include "kkstrtext.h"
#include "kkfsys.h"
#include "cmenus.h"

#include "icq.h"

#include "icqcommon.h"

#define SOUND_COUNT     7

class icqcontact {
    protected:
	unsigned int uin;
	unsigned short seq2;
	int status, nmsgs, fupdated, infotryn, groupid;
	bool finlist, reqauth, webaware, pubip, nonicq, direct, msgdirect;
	time_t lastread, lastseen;

	string sound[SOUND_COUNT];

	string nick, firstname, lastname, primemail, secemail, oldemail;
	string city, state, phone, fax, street, cellular, wcity, wstate;
	string wphone, wfax, waddress, company, department, job, whomepage;
	string homepage, lastip, about, dispnick, postponed;
	string faf[4], fbg[4], fint[4];

	unsigned char lang1, lang2, lang3, bday, bmonth, byear, age, gender;
	unsigned short country, wcountry, occupation;
	unsigned long zip, wzip;

	void scanhistory();
	const string tosane(const string p) const;

    public:
	icqcontact(unsigned int fuin, bool nonicq = false);
	~icqcontact();

	bool operator > (const icqcontact &acontact) const;

	void setstatus(int fstatus);
	void setlastread(time_t flastread);
	void setlastseen();

	void setseq2(unsigned short fseq2);
	void setnick(const string fnick);
	void setdispnick(const string fnick);

	void setinfo(const string fname, const string lname, const string fprimemail,
	    const string fsecemail, const string foldemail, const string fcity,
	    const string fstate, const string fphone, const string ffax, const string fstreet,
	    const string fcellular, unsigned long fzip, unsigned short fcountry);

	void setmoreinfo(unsigned char fage, unsigned char fgender,
	    const string fhomepage, unsigned char flang1, unsigned char flang2,
	    unsigned char flang3, unsigned char fbday, unsigned char fbmonth,
	    unsigned char fbyear);

	void setworkinfo(const string fwcity, const string fwstate, const string fwphone,
	    const string fwfax, const string fwaddress, unsigned long fwzip,
	    unsigned short fwcountry, const string fcompany, const string fdepartment,
	    const string fjob, unsigned short foccupation, const string fwhomepage);

	void setinterests(const string nint[]);
	void setaffiliations(const string naf[]);
	void setbackground(const string nbg[]);

	void setsecurity(bool freqauth, bool fwebaware, bool fpubip);
	void setabout(const string data);
	void setlastip(const string flastip);

	void getinfo(string &fname, string &lname, string &fprimemail,
	    string &fsecemail, string &foldemail, string &fcity,
	    string &fstate, string &fphone, string &ffax, string &fstreet,
	    string &fcellular, unsigned long &fzip, unsigned short &fcountry) const;

	void getmoreinfo(unsigned char &fage, unsigned char &fgender,
	    string &fhomepage, unsigned char &flang1, unsigned char &flang2,
	    unsigned char &flang3, unsigned char &fbday,
	    unsigned char &fbmonth, unsigned char &fbyear) const;

	void getworkinfo(string &fwcity, string &fwstate, string &fwphone,
	    string &fwfax, string &fwaddress, unsigned long &fwzip,
	    unsigned short &fwcountry, string &fcompany, string &fdepartment,
	    string &fjob, unsigned short &foccupation, string &fwhomepage) const;

	void getinterests(string &int1, string &int2, string &int3, string &int4) const;
	void getaffiliations(string &naf1, string &naf2, string &naf3, string &naf4) const;
	void getbackground(string &nbg1, string &nbg2, string &nbg3, string &nbg4) const;

	void getsecurity(bool &freqauth, bool &fwebaware, bool &fpubip) const;

	const string getabout() const;
	const string getlastip() const;

	time_t getlastread() const;
	time_t getlastseen() const;
	
	int getstatus() const;
	unsigned int getuin() const;
	unsigned short getseq2() const;
	int getmsgcount() const;

	const string getnick() const;
	const string getdispnick() const;

	int updated() const;
	void unsetupdated();
	void setmsgcount(int n);

	void setsound(int event, const string sf);
	void playsound(int event) const;

	void clear();
	void load();
	void save();
	void remove();
	void excludefromlist();
	void includeintolist();
	bool inlist() const;

	void setdirect(bool flag);
	bool getdirect() const;

	void setmsgdirect(bool flag);
	bool getmsgdirect() const;
	
	bool isnonicq() const;
	bool isbirthday() const;
	const string getdirname() const;

	char getshortstatus() const;

	int getinfotryn() const;
	void incinfotryn();

	void setpostponed(const string apostponed);
	const string getpostponed() const;

	void setgroupid(int agroupid);
	int getgroupid() const;
};

#endif
