#ifndef __IMEVENTS_H__
#define __IMEVENTS_H__

#include "imcontact.h"

#undef gettext

class imevent {
    friend class immessage;
    friend class imurl;
    friend class imsms;
    friend class imauthorization;
    friend class imemailexpress;
    friend class imemail;
    friend class imnotification;
    friend class imcontacts;
    friend class imfile;
    friend class imrawevent;
    friend class imxmlevent;

    public:
	enum imeventtype {
	    message, url, sms, authorization, online, offline, email,
	    notification, contacts, file, xml, imeventtype_size
	};

	enum imdirection {
	    incoming, outgoing, imdirection_size
	};

    protected:
	imcontact contact;
	imeventtype type;
	imdirection direction;
	time_t senttimestamp;
	time_t timestamp;

	string readblock(ifstream &f);

    public:
	imevent();
	imevent(const imcontact &acont, imdirection adir, imeventtype atype, time_t asenttimestamp = 0);
	imevent(ifstream &f);
	virtual ~imevent();

	imeventtype gettype() const;
	imdirection getdirection() const;
	imcontact getcontact() const;
	time_t getsenttimestamp() const;
	time_t gettimestamp() const;

	void settimestamp(time_t atimestamp);
	void setcontact(const imcontact &acontact);

	imevent *getevent() const;

	virtual string gettext() const;

	virtual bool empty() const;
	virtual bool contains(const string &atext) const;

	virtual void write(ofstream &f) const;
	virtual void read(ifstream &f);
};

ENUM_PLUSPLUS(imevent::imeventtype)
ENUM_PLUSPLUS(imevent::imdirection)

class immessage: public imevent {
    protected:
	string text;

    public:
	immessage(const imevent &ev);
	immessage(const imcontact &acont, imdirection adirection,
	    const string &atext, const time_t asenttime = 0);

	string gettext() const;

	bool empty() const;
	bool contains(const string &atext) const;

	void write(ofstream &f) const;
	void read(ifstream &f);
};

class imurl: public imevent {
    protected:
	string url, description;

    public:
	imurl(const imevent &ev);
	imurl(const imcontact &acont, imdirection adirection,
	    const string &aurl, const string &adescription);

	string geturl() const;
	string getdescription() const;

	string gettext() const;

	bool empty() const;
	bool contains(const string &atext) const;

	void write(ofstream &f) const;
	void read(ifstream &f);
};

class imsms: public imevent {
    protected:
	string text, phone;

    public:
	imsms(const imevent &ev);
	imsms(const imcontact &acont, imdirection adirection,
	    const string &atext, const string &aphone = "");

	string gettext() const;
	string getmessage() const { return text; }
	string getphone() const { return phone; }

	bool empty() const;
	bool contains(const string &atext) const;

	void write(ofstream &f) const;
	void read(ifstream &f);
};

class imauthorization: public imevent {
    public:
	enum AuthType {
	    Rejected = 0,
	    Granted = 1,
	    Request = 2,
	    AuthType_size
	};

    protected:
	string text;
	AuthType authtype;

    public:
	imauthorization(const imevent &ev);
	imauthorization(const imcontact &acont, imdirection adirection,
	    AuthType aatype, const string &atext = "");

	string gettext() const;
	string getmessage() const;

	AuthType getauthtype() const;

	bool empty() const;
	bool contains(const string &atext) const;

	void write(ofstream &f) const;
	void read(ifstream &f);
};

class imemail: public imevent {
    protected:
	string text;

    public:
	imemail(const imevent &ev);
	imemail(const imcontact &acont, imdirection adirection, const string &atext);

	string gettext() const;

	bool empty() const;
	bool contains(const string &atext) const;

	void write(ofstream &f) const;
	void read(ifstream &f);
};

class imnotification: public imevent {
    protected:
	string text;

    public:
	imnotification(const imevent &ev);
	imnotification(const imcontact &acont, const string &atext);

	string gettext() const;

	bool empty() const;
	bool contains(const string &atext) const;

	void write(ofstream &f) const;
	void read(ifstream &f);
};

class imcontacts: public imevent {
    protected:
	vector< pair<unsigned int, string> > contacts;

    public:
	imcontacts(const imevent &ev);
	imcontacts(const imcontact &acont, imdirection adirection,
	    const vector< pair<unsigned int, string> > &lst);

	string gettext() const;
	const vector< pair<unsigned int, string> > &getcontacts() const;

	bool empty() const;
	bool contains(const string &atext) const;

	void write(ofstream &f) const;
	void read(ifstream &f);
};

class imfile: public imevent {
    public:
	struct record {
	    string fname;
	    long size;
	};

    protected:
	vector<record> files;
	string msg;

    public:
	imfile() { };
	imfile(const imevent &ev);
	imfile(const imcontact &acont, imdirection adirection,
	    const string &amsg = "",
	    const vector<record> &afiles = vector<record>());

	string gettext() const;
	string getmessage() const;

	const vector<record> &getfiles() const;
	void setfiles(const vector<record> &lst);

	bool empty() const;
	bool contains(const string &atext) const;

	void write(ofstream &f) const;
	void read(ifstream &f);

	bool operator == (const imfile &fr) const   { return this->gettext() == fr.gettext(); }
	bool operator != (const imfile &fr) const   { return !(*this == fr); }
	bool operator < (const imfile &fr) const    { return this->gettext() < fr.gettext(); }
};

class imrawevent: public imevent {
    public:
	imrawevent(const imevent &ev);
	imrawevent(imeventtype atype, const imcontact &acont, imdirection adirection);
};

class imxmlevent: public imevent {
    protected:
	map<string, string> data;

    public:
	imxmlevent(const imevent &ev);
	imxmlevent(const imcontact &acont, imdirection adirection, const string &atext);

	string gettext() const;

	bool field_empty(const string &name) const;
	string getfield(const string &name) const;

	void setfield(const string &name, const string &val);

	bool empty() const;
	bool contains(const string &atext) const;

	void write(ofstream &f) const;
	void read(ifstream &f);
};

#endif
