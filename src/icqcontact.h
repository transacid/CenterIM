#ifndef __ICQCONTACT_H_
#define __ICQCONTACT_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <dirent.h>

#include "konst.list.h"
#include "konst.string.h"
#include "konst.fs.h"
#include "konst.ui.func.h"
#include "konst.ui.menu.h"

#include "icq.h"

#define SOUND_COUNT     7

class icqcontact {
    protected:
	unsigned int uin;
	unsigned short seq2;
	int status, nmsgs, fupdated, infotryn;
	bool finlist, reqauth, webaware, pubip, nonicq, direct;
	time_t lastread, lastseen;

	string sound[SOUND_COUNT];

	string nick, firstname, lastname, primemail, secemail, oldemail;
	string city, state, phone, fax, street, cellular, wcity, wstate;
	string wphone, wfax, waddress, company, department, job, whomepage;
	string homepage, lastip, about, dispnick;
	string faf[4], fbg[4], fint[4];

	unsigned char lang1, lang2, lang3, bday, bmonth, byear, age, gender;
	unsigned short country, wcountry, occupation;
	unsigned long zip, wzip;

	void scanhistory();
	string tosane(string p);

    public:
	icqcontact(unsigned int fuin, bool nonicq = false);
	~icqcontact();

	bool operator > (const icqcontact &acontact) const;

	void setstatus(int fstatus);
	void setlastread(time_t flastread);
	void setlastseen();

	void setseq2(unsigned short fseq2);
	void setnick(string fnick);
	void setdispnick(string fnick);
	void setdirect(bool flag);

	void setinfo(string fname, string lname, string fprimemail,
	    string fsecemail, string foldemail, string fcity,
	    string fstate, string fphone, string ffax, string fstreet,
	    string fcellular, unsigned long fzip, unsigned short fcountry);

	void setmoreinfo(unsigned char fage, unsigned char fgender,
	    string fhomepage, unsigned char flang1, unsigned char flang2,
	    unsigned char flang3, unsigned char fbday, unsigned char fbmonth,
	    unsigned char fbyear);

	void setworkinfo(string fwcity, string fwstate, string fwphone,
	    string fwfax, string fwaddress, unsigned long fwzip,
	    unsigned short fwcountry, string fcompany, string fdepartment,
	    string fjob, unsigned short foccupation, string fwhomepage);

	void setinterests(string nint[]);
	void setaffiliations(string naf[]);
	void setbackground(string nbg[]);

	void setsecurity(bool freqauth, bool fwebaware, bool fpubip);
	void setabout(string data);
	void setlastip(string flastip);

	void getinfo(string &fname, string &lname, string &fprimemail,
	    string &fsecemail, string &foldemail, string &fcity,
	    string &fstate, string &fphone, string &ffax, string &fstreet,
	    string &fcellular, unsigned long &fzip, unsigned short &fcountry);

	void getmoreinfo(unsigned char &fage, unsigned char &fgender,
	    string &fhomepage, unsigned char &flang1, unsigned char &flang2,
	    unsigned char &flang3, unsigned char &fbday,
	    unsigned char &fbmonth, unsigned char &fbyear);

	void getworkinfo(string &fwcity, string &fwstate, string &fwphone,
	    string &fwfax, string &fwaddress, unsigned long &fwzip,
	    unsigned short &fwcountry, string &fcompany, string &fdepartment,
	    string &fjob, unsigned short &foccupation, string &fwhomepage);

	void getinterests(string &int1, string &int2, string &int3, string &int4);
	void getaffiliations(string &naf1, string &naf2, string &naf3, string &naf4);
	void getbackground(string &nbg1, string &nbg2, string &nbg3, string &nbg4);

	void getsecurity(bool &freqauth, bool &fwebaware, bool &fpubip);

	string getabout();
	string getlastip();

	time_t getlastread();
	time_t getlastseen();
	
	int getstatus();
	unsigned int getuin();
	unsigned short getseq2();
	int getmsgcount();

	string getnick();
	string getdispnick();

	int updated();
	void unsetupdated();
	void setmsgcount(int n);

	void setsound(int event, string sf);
	void playsound(int event);

	void clear();
	void load();
	void save();
	void remove();
	void excludefromlist();
	void includeintolist();
	bool inlist();
	bool getdirect();
	
	bool isnonicq();
	bool isbirthday();
	string getdirname();

	char getshortstatus();
	char getsortchar();

	int getinfotryn();
	void incinfotryn();
};

#endif
