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

class msncallbacks : public MSN::Callbacks {
    public:
        void registerSocket(int s, int read, int write);
        void unregisterSocket(int s);
        void showError(MSN::Connection * conn, std::string msg);
        void buddyChangedStatus(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname, MSN::BuddyStatus state);
        void buddyOffline(MSN::Connection * conn, MSN::Passport buddy);
        void log(int writing, const char* buf);
        void gotFriendlyName(MSN::Connection * conn, std::string friendlyname);
        void gotBuddyListInfo(MSN::NotificationServerConnection * conn, MSN::ListSyncInfo * data);
        void gotLatestListSerial(MSN::Connection * conn, int serial);
        void gotGTC(MSN::Connection * conn, char c);
        void gotBLP(MSN::Connection * conn, char c);
        void gotNewReverseListEntry(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname);
        void addedListEntry(MSN::Connection * conn, std::string list, MSN::Passport buddy, int groupID);
        void removedListEntry(MSN::Connection * conn, std::string list, MSN::Passport buddy, int groupID);
        void addedGroup(MSN::Connection * conn, std::string groupName, int groupID);
        void removedGroup(MSN::Connection * conn, int groupID);
        void renamedGroup(MSN::Connection * conn, int groupID, std::string newGroupName);
        void gotSwitchboard(MSN::SwitchboardServerConnection * conn, const void * tag);
        void buddyJoinedConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, int is_initial);
        void buddyLeftConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy);
        void gotInstantMessage(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, MSN::Message * msg);
        void failedSendingMessage(MSN::Connection * conn);
        void buddyTyping(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname);
        void gotInitialEmailNotification(MSN::Connection * conn, int unread_inbox, int unread_folders);
        void gotNewEmailNotification(MSN::Connection * conn, std::string from, std::string subject);
        void gotFileTransferInvitation(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname, MSN::FileTransferInvitation * inv);
        void fileTransferProgress(MSN::FileTransferInvitation * inv, std::string status, unsigned long recv, unsigned long total);
        void fileTransferFailed(MSN::FileTransferInvitation * inv, int error, std::string message);
        void fileTransferSucceeded(MSN::FileTransferInvitation * inv);
        void gotNewConnection(MSN::Connection * conn);
        void closingConnection(MSN::Connection * conn);
        void changedStatus(MSN::Connection * conn, MSN::BuddyStatus state);
        int connectToServer(std::string server, int port, bool *connected);
        int listenOnPort(int port);
        std::string getOurIP();
        std::string getSecureHTTPProxy();
};

class msnhook : public abstracthook {

    friend void msncallbacks::registerSocket(int s, int reading, int writing);
    friend void msncallbacks::unregisterSocket(int s);
    friend void msncallbacks::showError(MSN::Connection * conn, string msg);
    friend void msncallbacks::buddyChangedStatus(MSN::Connection * conn, MSN::Passport buddy, string friendlyname, MSN::BuddyStatus state);
    friend void msncallbacks::buddyOffline(MSN::Connection * conn, MSN::Passport buddy);
    friend void msncallbacks::log(int writing, const char* buf);
    friend void msncallbacks::gotFriendlyName(MSN::Connection * conn, string friendlyname);
    friend void msncallbacks::gotBuddyListInfo(MSN::NotificationServerConnection * conn, MSN::ListSyncInfo * data);
    friend void msncallbacks::gotLatestListSerial(MSN::Connection * conn, int serial);
    friend void msncallbacks::gotGTC(MSN::Connection * conn, char c);
    friend void msncallbacks::gotBLP(MSN::Connection * conn, char c);
    friend void msncallbacks::gotNewReverseListEntry(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname);
    friend void msncallbacks::addedListEntry(MSN::Connection * conn, std::string list, MSN::Passport buddy, int groupID);
    friend void msncallbacks::removedListEntry(MSN::Connection * conn, std::string list, MSN::Passport buddy, int groupID);
    friend void msncallbacks::addedGroup(MSN::Connection * conn, string groupName, int groupID);
    friend void msncallbacks::removedGroup(MSN::Connection * conn, int groupID);
    friend void msncallbacks::renamedGroup(MSN::Connection * conn, int groupID, string newGroupName);
    friend void msncallbacks::gotSwitchboard(MSN::SwitchboardServerConnection * conn, const void * tag);
    friend void msncallbacks::buddyJoinedConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, int is_initial);
    friend void msncallbacks::buddyLeftConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy);
    friend void msncallbacks::gotInstantMessage(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, MSN::Message * msg);
    friend void msncallbacks::failedSendingMessage(MSN::Connection * conn);
    friend void msncallbacks::buddyTyping(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname);
    friend void msncallbacks::gotInitialEmailNotification(MSN::Connection * conn, int unread_inbox, int unread_folders);
    friend void msncallbacks::gotNewEmailNotification(MSN::Connection * conn, string from, string subject);
    friend void msncallbacks::gotFileTransferInvitation(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname, MSN::FileTransferInvitation * inv);
    friend void msncallbacks::fileTransferProgress(MSN::FileTransferInvitation * inv, string status, unsigned long recv, unsigned long total);
    friend void msncallbacks::fileTransferFailed(MSN::FileTransferInvitation * inv, int error, string message);
    friend void msncallbacks::fileTransferSucceeded(MSN::FileTransferInvitation * inv);
    friend void msncallbacks::gotNewConnection(MSN::Connection * conn);
    friend void msncallbacks::closingConnection(MSN::Connection * conn);
    friend void msncallbacks::changedStatus(MSN::Connection * conn, MSN::BuddyStatus state);
    friend int msncallbacks::connectToServer(string server, int port, bool *connected);
    friend int msncallbacks::listenOnPort(int port);
    friend string msncallbacks::getOurIP();
    friend string msncallbacks::getSecureHTTPProxy();

    protected:
	imstatus ourstatus;
	bool fonline, flogged, readinfo, destroying;
	MSN::NotificationServerConnection conn;
	msncallbacks cb;
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
	virtual ~msnhook();

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
