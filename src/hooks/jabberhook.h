#ifndef __JABBERHOOK_H__
#define __JABBERHOOK_H__

#include "abstracthook.h"

#ifdef BUILD_JABBER

#include "jabber.h"

class jabberhook: public abstracthook {
    protected:
	imstatus ourstatus;
	jconn jc;
	int id;
	string uuid;

	bool flogged, fonline, regmode, regdone;
	string regerr;

	time_t timer_keepalive;

	map<string, pair<imfile, pair<struct send_file *, int > > > srfiles; //transfer queue of files
	map<imfile, string> back_srfiles; //backward compatibility to clean up sending files from overloaded abort function
	map<string, string> roster;
	map<string, string> awaymsgs;
	map<string, string> full_jids; //little trick to store users full JID's, it required in some xmpp packets
	map<imfile, pair<struct send_file *, string> > transferinfo;
	map<string, vector<string> > chatmembers;
	map<string, map<string, pair<char, imstatus> > > statuses;  // <JID, <resource, <prio, status> > >
	set<string> ignore_ids; // set of packet IDs to ignore errors from

	struct agent {
	    string jid, name, desc;

	    struct agent_params {
		bool enabled;
		string instruction, key;
		vector<pair<string, string> > paramnames;
		agent_params(): enabled(false) {}
	    };

	    enum param_type { ptSearch, ptRegister, ptReceipts, param_type_size };
	    enum agent_type { atSearch, atTransport, atGroupchat, atStandard, atUnknown } type;

	    agent_params params[param_type_size];

	    agent(const string &ajid, const string &aname, const string &adesc, agent_type atype):
		jid(ajid), name(aname), desc(adesc), type(atype) {}

	    bool operator == (const string &ajid) const { return jid == ajid; }
	    bool operator != (const string &ajid) const { return jid != ajid; }
	};

	enum {
	    STATE_CONNECTING,
	    STATE_GETAUTH,
	    STATE_SENDAUTH,
	    STATE_LOGGED
	} jstate;

	vector<agent> agents;

	imstatus process_presence(string id, string s, char prio, imstatus ust);

	static void statehandler(jconn conn, int state);
	static void packethandler(jconn conn, jpacket packet);
	static void jlogger(jconn conn, int inout, const char *p);

	vector<pair<string, string> > getservparams(const string &agentname, agent::param_type pt) const;

	void setjabberstatus(imstatus st, string msg);
	void sendvisibility();

	void sendnewuser(const imcontact &c, bool report);
	void removeuser(const imcontact &ic, bool report);

	void gotagentinfo(xmlnode x);
	void gotsearchresults(xmlnode x);
	void gotloggedin();
	void postlogin();
	void gotroster(xmlnode x);
	void gotvcard(const imcontact &ic, xmlnode v);
	void gotmessage(const string &type, const string &from, const string &body, const string &enc);
	void gotversion(const imcontact &ic, xmlnode x);
	void sendversion(const imcontact &ic, xmlnode x);
	void senddiscoinfo(const imcontact &ic, xmlnode x);
	bool get_img_ext(const string &type, string &ext);
	bool url_port_get(const string &full_url, string &url, int &port, string &tail, string &filename);
	bool get_base64_avatar(string &type, string &ava);
	void getfile(const imfile &fr);
	void getfile_http(const imcontact &ic, xmlnode i);
	void getfile_byte(const imcontact &ic, xmlnode i);
	void getfile_result(const imfile &fr);
	void file_transfer_request(const imcontact &ic, xmlnode i);
	void bytenegotiat(const imfile &fr);
	void reject_file(const imfile &fr);
	void recieve_file( const imcontact &ic, xmlnode x, string id, string from, string to );
	void send_file(const string &cjid);

	bool isourid(const string &jid);
	static string getourjid();
	static void progressbar(void *file, long int bytes, long int size, int status, int conn_type );
	string jidnormalize(const string &jid) const;

	void vcput(xmlnode x, const string &name, const string &val);
	void vcputphone(xmlnode x, const string &type, const string &place, const string &number);
	bool get_my_avatar_hash(string &my_hash);	
	void vcputaddr(xmlnode x, const string &place, const string &street,
	    const string &locality, const string &region, const string &pcode,
	    unsigned short country);
	void vcputavatar(xmlnode x, const string &type, const string &val);
	void clean_up_file(const imfile &, int trans_type, char *cjid = NULL );

    public:
	jabberhook();
	virtual ~jabberhook();

	void init();

	void connect();
	void disconnect();
	void exectimers();
	void main();

	void getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const;
	bool isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const;

	bool online() const;
	bool logged() const;
	bool isconnecting() const;
	bool enabled() const;

	bool send(const imevent &ev);

	void sendnewuser(const imcontact &ic);
	void removeuser(const imcontact &ic);

	void setautostatus(imstatus st);
	imstatus getstatus() const;

	void requestinfo(const imcontact &ic);
	void requestawaymsg(const imcontact &ic);

	bool regnick(const string &nick, const string &pass,
	    const string &serv, string &err);

	vector<string> getservices(servicetype::enumeration st) const;
	vector<pair<string, string> > getsearchparameters(const string &agentname) const;
	vector<pair<string, string> > getregparameters(const string &agentname) const;

	map<string,imfile> transferusers; //some hack to quick find transfer file by full JID

	void lookup(const imsearchparams &params, verticalmenu &dest);

	void conferencecreate(const imcontact &confid,
	    const vector<imcontact> &lst);

	void sendupdateuserinfo(const icqcontact &c);
	void updatecontact(icqcontact *c);

	void requestversion(const imcontact &c);
	void renamegroup(const string &oldname, const string &newname);

	void ouridchanged(const icqconf::imaccount &ia);
	bool knowntransfer(const imfile &fr) const;
	void replytransfer(const imfile &fr, bool accept, const string &localpath = string() );
	void aborttransfer(const imfile &fr);
};

extern jabberhook jhook;

#endif

#endif
