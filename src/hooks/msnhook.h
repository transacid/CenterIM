#ifndef __MSNHOOK_H__
#define __MSNHOOK_H__

#include "abstracthook.h"

#ifdef BUILD_MSN

#include "msn/msn.h"

struct msnbuddy {
    msnbuddy(const string &anick, const string &afriendly = "", int agid = -1)
	: nick(anick), friendly(afriendly), gid(agid) { }

    string nick, friendly;
    int gid;

    bool operator == (const string &anick) const
	{ return nick == anick; }
    bool operator != (const string &anick) const
	{ return nick != anick; }
};

class msnhook : public abstracthook {

    friend void MSN::ext::registerSocket(int s, int reading, int writing);
    friend void MSN::ext::unregisterSocket(int s);
    friend void MSN::ext::showError(MSN::Connection * conn, string msg);
    friend void MSN::ext::buddyChangedStatus(MSN::Connection * conn, MSN::Passport buddy, string friendlyname, MSN::BuddyStatus state);
    friend void MSN::ext::buddyOffline(MSN::Connection * conn, MSN::Passport buddy);
    friend void MSN::ext::log(int writing, const char* buf);
    friend void MSN::ext::gotFriendlyName(MSN::Connection * conn, string friendlyname);
    friend void MSN::ext::gotBuddyListInfo(MSN::NotificationServerConnection * conn, MSN::ListSyncInfo * data);
    friend void MSN::ext::gotLatestListSerial(MSN::Connection * conn, int serial);
    friend void MSN::ext::gotGTC(MSN::Connection * conn, char c);
    friend void MSN::ext::gotBLP(MSN::Connection * conn, char c);
    friend void MSN::ext::gotNewReverseListEntry(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname);
    friend void MSN::ext::addedListEntry(MSN::Connection * conn, std::string list, MSN::Passport buddy, int groupID);
    friend void MSN::ext::removedListEntry(MSN::Connection * conn, std::string list, MSN::Passport buddy, int groupID);
    friend void MSN::ext::addedGroup(MSN::Connection * conn, string groupName, int groupID);
    friend void MSN::ext::removedGroup(MSN::Connection * conn, int groupID);
    friend void MSN::ext::renamedGroup(MSN::Connection * conn, int groupID, string newGroupName);
    friend void MSN::ext::gotSwitchboard(MSN::SwitchboardServerConnection * conn, const void * tag);
    friend void MSN::ext::buddyJoinedConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, int is_initial);
    friend void MSN::ext::buddyLeftConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy);
    friend void MSN::ext::gotInstantMessage(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, MSN::Message * msg);
    friend void MSN::ext::failedSendingMessage(MSN::Connection * conn);
    friend void MSN::ext::buddyTyping(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname);
    friend void MSN::ext::gotInitialEmailNotification(MSN::Connection * conn, int unread_inbox, int unread_folders);
    friend void MSN::ext::gotNewEmailNotification(MSN::Connection * conn, string from, string subject);
    friend void MSN::ext::gotFileTransferInvitation(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname, MSN::FileTransferInvitation * inv);
    friend void MSN::ext::fileTransferProgress(MSN::FileTransferInvitation * inv, string status, unsigned long recv, unsigned long total);
    friend void MSN::ext::fileTransferFailed(MSN::FileTransferInvitation * inv, int error, string message);
    friend void MSN::ext::fileTransferSucceeded(MSN::FileTransferInvitation * inv);
    friend void MSN::ext::gotNewConnection(MSN::Connection * conn);
    friend void MSN::ext::closingConnection(MSN::Connection * conn);
    friend void MSN::ext::changedStatus(MSN::Connection * conn, MSN::BuddyStatus state);
    friend int MSN::ext::connectToServer(string server, int port, bool *connected);
    friend int MSN::ext::listenOnPort(int port);
    friend string MSN::ext::getOurIP();
    friend string MSN::ext::getSecureHTTPProxy();

    protected:
	imstatus ourstatus;
	bool fonline, flogged, readinfo, destroying;
	MSN::NotificationServerConnection conn;
	time_t timer_ping;

	struct qevent {
	    enum qetype { qeMsg, qeFile };

	    qetype type;
	    string nick, text;

	    qevent(qetype atype, const string &anick, const string &atext):
		type(atype), nick(anick), text(atext) { }
	};

	map<string, vector<msnbuddy> > slst;
	map<string, MSN::SwitchboardServerConnection *> lconn;

	vector<int> rfds, wfds;
	map<int, string> mgroups;
	map<string, string> friendlynicks;
	map<imfile, pair<MSN::FileTransferInvitation *, string> > transferinfo;

	void checkfriendly(icqcontact *c, const string friendlynick,
	    bool forcefetch = false);

	void checkinlist(imcontact ic);
	int findgroup(const imcontact &ic, string &gname) const;

	void removeuser(const imcontact &ic, bool report);
	bool getfevent(MSN::FileTransferInvitation *fhandle, imfile &fr);
	void statusupdate(string buddy, string friendlyname, imstatus status);

	void sendmsn(MSN::SwitchboardServerConnection *conn, const qevent *ctx);

    public:
	msnhook();
	~msnhook();

	void init();

	void connect();
	void disconnect();
	void exectimers();
	void main();

	void getsockets(fd_set &rf, fd_set &wf, fd_set &ef, int &hsocket) const;
	bool isoursocket(fd_set &rf, fd_set &wf, fd_set &ef) const;

	bool online() const;
	bool logged() const;
	bool isconnecting() const;
	bool enabled() const;

	bool send(const imevent &ev);

	void sendnewuser(const imcontact &c);
	void removeuser(const imcontact &ic);
	void requestinfo(const imcontact &ic);

	void sendupdateuserinfo(const icqcontact &c);

	void setautostatus(imstatus st);
	imstatus getstatus() const;

	void lookup(const imsearchparams &params, verticalmenu &dest);
	vector<icqcontact *> getneedsync();

	bool knowntransfer(const imfile &fr) const;
	void replytransfer(const imfile &fr, bool accept, const string &localpath = string());
	void aborttransfer(const imfile &fr);

	void updatecontact(icqcontact *c);
	void renamegroup(const string &oldname, const string &newname);
};

extern msnhook mhook;

#endif

#endif
