#ifndef __ICQHOOK_H__
#define __ICQHOOK_H__

#include "icq.h"

#include "kkiproc.h"
#include "cmenus.h"

#define PERIOD_KEEPALIVE        100
#define PERIOD_SELECT           1
#define PERIOD_TCP              1
#define PERIOD_RESOLVE          40
#define PERIOD_RECONNECT        30
#define PERIOD_DISCONNECT       130
#define PERIOD_RESEND           45
#define PERIOD_CHECKMAIL        30
#define PERIOD_WAITNEWUIN       20
#define PERIOD_SOCKSALIVE       30
#define PERIOD_WAIT_KEEPALIVE   10

#define MAX_UDPMSG_SIZE         440
#define MAX_TCPMSG_SIZE         7000

#define HIDL_SOCKEXIT   2

class icqfileassociation {
    public:
	icqfileassociation(unsigned int fuin, unsigned long fseq,
	string ffname, int fdir): uin(fuin), seq(fseq), fname(ffname), dir(fdir) {};

	int dir;
	unsigned int uin;
	unsigned long seq;
	string fname;
};

class icqhook {
    protected:
	bool flogged, connecting;
	int newuin, n_keepalive, manualstatus;
	unsigned long seq_keepalive;
	time_t timer_keepalive, timer_tcp, timer_resolve, timer_keypress;
	time_t timer_offline, timer_reconnect, timer_ack, timer_checkmail;
	time_t logontime;
	verticalmenu *finddest;

	vector<unsigned long> founduins;
	vector<icqfileassociation> files;

    public:
	icqhook();
	~icqhook();

	void init(struct icq_link *link);
	void reginit(struct icq_link *link);

	unsigned int getreguin();
	void setreguin(unsigned int ruin);
	void postreg();
	void connect(int status = -2);

	bool logged();
	bool isconnecting();
	struct tm *maketm(int hour, int minute, int day, int month, int year);
	void exectimers();
	bool idle(int options = 0);
	void setfinddest(verticalmenu *m);
	unsigned int getfinduin(int pos);
	void clearfindresults();

	void addfile(unsigned int uin, unsigned long seq, string fname, int dir);
	int getmanualstatus();
	void setmanualstatus(int st);

	static void loggedin(struct icq_link *link);
	static void disconnected(struct icq_link *link);
	static void regdisconnected(struct icq_link *link);
	static void message(struct icq_link *link, unsigned long uin,
	    unsigned char hour, unsigned char minute, unsigned char day,
	    unsigned char month, unsigned short year, const char *msg);
	static void contact(struct icq_link *link, unsigned long uin,
	    unsigned char hour, unsigned char minute, unsigned char day,
	    unsigned char month, unsigned short year, icqcontactmsg *c);
	static void url(struct icq_link *link, unsigned long uin,
	    unsigned char hour, unsigned char minute, unsigned char day,
	    unsigned char month, unsigned short year, const char *url,
	    const char *descr);
	static void webpager(struct icq_link *link,unsigned char hour,
	    unsigned char minute, unsigned char day, unsigned char month,
	    unsigned short year, const char *nick, const char *email,
	    const char *msg);
	static void mailexpress(struct icq_link *link,unsigned char hour,
	    unsigned char minute, unsigned char day, unsigned char month,
	    unsigned short year, const char *nick, const char *email,
	    const char *msg);
	static void chat(struct icq_link *link, unsigned long uin,
	    unsigned char hour, unsigned char minute, unsigned char day,
	    unsigned char month, unsigned short year, const char *descr,
	    unsigned long seq/*, const char *session, unsigned long port*/);
	static void file(struct icq_link *link, unsigned long uin,
	    unsigned char hour, unsigned char minute, unsigned char day,
	    unsigned char month, unsigned short year, const char *descr,
	    const char *filename, unsigned long filesize, unsigned long seq);
	static void added(struct icq_link *link, unsigned long uin,
	    unsigned char hour, unsigned char minute, unsigned char day,
	    unsigned char month, unsigned short year, const char *nick,
	    const char *first, const char *last, const char *email);
	static void auth(struct icq_link *link, unsigned long uin,
	    unsigned char hour, unsigned char minute, unsigned char day,
	    unsigned char month, unsigned short year, const char *nick,
	    const char *first, const char *last, const char *email,
	    const char *reason);
	static void useronline(struct icq_link *link, unsigned long uin,
	    unsigned long status, unsigned long ip, unsigned short port,
	    unsigned long real_ip, unsigned char tcp_flag);
	static void useroffline(struct icq_link *link, unsigned long uin);
	static void userstatus(struct icq_link *link, unsigned long uin,
	    unsigned long status);
	static void wrongpass(struct icq_link *link);
	static void invaliduin(struct icq_link *link);
	static void regnewuin(struct icq_link *link, unsigned long uin);
	static void log(struct icq_link *link, time_t time, unsigned char level,
	    const char *str);
	static void metauserinfo(struct icq_link *link, unsigned short seq2,
	    const char *nick, const char *first, const char *last,
	    const char *pri_eml, const char *sec_eml, const char *old_eml,
	    const char *city, const char *state, const char *phone,
	    const char *fax, const char *street, const char *cellular,
	    unsigned long zip, unsigned short country, unsigned char timezone,
	    unsigned char auth);
	static void metauserabout(struct icq_link *link, unsigned short seq2,
	    const char *about);
	static void metauserwork(struct icq_link *link, unsigned short seq2,
	    const char *fwcity, const char *fwstate, const char *fwphone,
	    const char *fwfax, const char *fwaddress, unsigned long fwzip,
	    unsigned short fwcountry, const char *fcompany,
	    const char *fdepartment, const char *fjob,
	    unsigned short foccupation, const char *fwhomepage);
	static void metausermore(struct icq_link *link, unsigned short seq2,
	    unsigned short fage, unsigned char fgender, const char *fhomepage,
	    unsigned char byear, unsigned char bmonth, unsigned char bday,
	    unsigned char flang1, unsigned char flang2, unsigned char flang3);
	static void metauserinterests(struct icq_link *link,
	    unsigned short seq2, unsigned char num, unsigned short icat1,
	    const char *int1, unsigned short icat2, const char *int2,
	    unsigned short icat3, const char *int3, unsigned short icat4,
	    const char *int4);
	static void metauseraffiliations(struct icq_link *link,
	    unsigned short seq2, unsigned char anum, unsigned short acat1,
	    const char *aff1, unsigned short acat2, const char *aff2,
	    unsigned short acat3, const char *aff3, unsigned short acat4,
	    const char *aff4, unsigned char bnum, unsigned short bcat1,
	    const char *back1, unsigned short bcat2, const char *back2,
	    unsigned short bcat3, const char *back3, unsigned short bcat4,
	    const char *back4);
	static void userfound(struct icq_link *link, unsigned long uin,
	    const char *nick, const char *first, const char *last,
	    const char *email, char auth);
	static void searchdone(struct icq_link *link);
	static void requestnotify(struct icq_link *link, unsigned long id,
	    int result, unsigned int length, void *data);
};

extern icqhook ihook;

#endif
