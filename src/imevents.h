#ifndef __IMEVENTS_H__
#define __IMEVENTS_H__

#include "imcontact.h"

class imevent {
    friend class immessage;
    friend class imurl;
    friend class imsms;
    friend class imauthorization;

    public:
	enum imeventtype {
	    message, url, sms, authorization, online, email, imeventtype_size
	};

	enum imdirection {
	    incoming, outgoing, imdirection_size
	};

    protected:
	imcontact contact;
	imeventtype type;
	imdirection direction;
	time_t timestamp;

    public:
	imeventtype gettype() const;
	imdirection getdirection() const;
	imcontact getcontact() const;
	time_t gettimestamp() const;

	void settimestamp(time_t atimestamp);
	void setcontact(const imcontact &acontact);

	virtual void write(ofstream &f) const;
	virtual void read(ifstream &f);
};

class immessage: public imevent {
    protected:
	string text;

    public:
	immessage();
	immessage(const imevent &ev);
	immessage(const imcontact acont, imdirection adirection,
	    const string atext);

	const string gettext() const;

	void write(ofstream &f) const;
	void read(ifstream &f);
};

class imurl: public imevent {
    protected:
	string url, description;

    public:
	imurl();
	imurl(const imevent &ev);
	imurl(const imcontact acont, imdirection adirection,
	    const string aurl, const string adescription);

	const string geturl() const;
	const string getdescription() const;

	void write(ofstream &f) const;
	void read(ifstream &f);
};

class imsms: public immessage {
};

class imauthorization: public imevent {
    protected:
	string text;

    public:
	imauthorization();
	imauthorization(const imevent &ev);
	imauthorization(const imcontact acont, imdirection adirection,
	    const string atext);

	const string gettext() const;

	void write(ofstream &f) const;
	void read(ifstream &f);
};

#endif
