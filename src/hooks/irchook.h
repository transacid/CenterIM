#ifndef __IRCHOOK_H__
#define __IRCHOOK_H__

#include "abstracthook.h"

#ifdef BUILD_IRC

extern "C" {
#include "firetalk.h"
}

class irchook: public abstracthook {
    protected:
	struct channelInfo {
	    string name;
	    string passwd;
	    bool joined, fetched;

	    vector<string> nicks;

	    channelInfo(const string &aname):
		name(aname), joined(false), fetched(false), passwd("") {}

	    bool operator != (const string &aname) const;
	    bool operator == (const string &aname) const;
	};

	enum searchMode {
	    Channel,
	    Email
	};

	searchMode smode;

	bool fonline, flogged, searchsincelast, sentpass;
	firetalk_t handle;
	imstatus ourstatus;
	string ircname, emailsub, namesub;

	vector<channelInfo> channels;
	vector<string> searchchannels, extlisted;

	map<string, string> awaymessages;
	map<string, time_t> pingtime;
	map<imfile, pair<void *, string> > transferinfo;

	void userstatus(const string &nickname, imstatus st);
	void processnicks();

	bool getfevent(void *fhandle, imfile &fr);

	static void connected(void *conn, void *cli, ...);
	static void disconnected(void *conn, void *cli, ...);
	static void newnick(void *conn, void *cli, ...);
	static void gotinfo(void *conn, void *cli, ...);
	static void gotchannels(void *conn, void *cli, ...);
	static void getmessage(void *conn, void *cli, ...);
	static void getaction(void *conn, void *cli, ...);
	static void buddyonline(void *conn, void *cli, ...);
	static void buddyoffline(void *conn, void *cli, ...);
	static void buddyaway(void *conn, void *cli, ...);
	static void connectfailed(void *connection, void *cli, ...);
	static void listmember(void *connection, void *cli, ...);
	static void fclog(void *connection, void *cli, ...);
	static void chatnames(void *connection, void *cli, ...);
	static void listextended(void *connection, void *cli, ...);
	static void endextended(void *connection, void *cli, ...);
	static void chatmessage(void *connection, void *cli, ...);
	static void chataction(void *connection, void *cli, ...);
	static void chatjoined(void *connection, void *cli, ...);
	static void chatleft(void *connection, void *cli, ...);
	static void chatkicked(void *connection, void *cli, ...);
	static void errorhandler(void *connection, void *cli, ...);
	static void nickchanged(void *connection, void *cli, ...);
	static void needpass(void *conn, void *cli, ...);
	static void fileoffer(void *conn, void *cli, ...);
	static void filestart(void *conn, void *cli, ...); 
	static void fileprogress(void *conn, void *cli, ...);
	static void filefinish(void *conn, void *cli, ...);
	static void fileerror(void *conn, void *cli, ...);
	static void chatuserjoined(void *conn, void *cli, ...);
	static void chatuserleft(void *conn, void *cli, ...);
	static void chatuserkicked(void *conn, void *cli, ...);
	static void chatgottopic(void *conn, void *cli, ...);
	static void chatuseropped(void *conn, void *cli, ...);
	static void chatuserdeopped(void *conn, void *cli, ...);
	static void chatopped(void *conn, void *cli, ...);
	static void chatdeopped(void *conn, void *cli, ...);

	static void subrequest(void *conn, void *cli,
	    const char * const nick, const char * const command,
	    const char * const args);

	static void subreply(void *conn, void *cli,
	    const char * const nick, const char * const command,
	    const char * const args);

	void rawcommand(const string &cmd);
	void channelfatal(string room, const char *fmt, ...);

	vector<channelInfo> getautochannels() const;
	void setautochannels(vector<channelInfo> &achannels);

    public:
	irchook();
	~irchook();

	void init();

	void connect();
	void disconnect();
	void exectimers();
	void main();

	void getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const;
	bool isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const;

	bool online() const;
	bool logged() const;
	bool enabled() const;
	bool isconnecting() const;

	bool send(const imevent &ev);

	void sendnewuser(const imcontact &c);
	void removeuser(const imcontact &ic);

	void setautostatus(imstatus st);

	imstatus getstatus() const;

	void requestversion(const imcontact &c);
	void ping(const imcontact &c);

	void requestinfo(const imcontact &c);
	void requestawaymsg(const imcontact &c);
	void sendupdateuserinfo(icqcontact &c, const string &newpass);

	void lookup(const imsearchparams &params, verticalmenu &dest);

	void ouridchanged(const icqconf::imaccount &ia);

	bool knowntransfer(const imfile &fr) const;
	void replytransfer(const imfile &fr, bool accept, const string &localpath = string());
	void aborttransfer(const imfile &fr);
};

extern irchook irhook;

#endif

#endif
