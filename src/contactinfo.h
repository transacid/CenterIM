#ifndef __CONTACTINFO_H__
#define __CONTACTINFO_H__

struct contactinfo {
    enum idtype {
	icq,
	yahoo,
	infocard
    };

    contactinfo();
    contactinfo(unsigned long auin, idtype atype);

    unsigned long uin;
    idtype type;

    bool operator == (const contactinfo &ainfo) const;
    bool operator != (const contactinfo &ainfo) const;

    bool empty() const;
};

extern contactinfo contactroot;

#endif
