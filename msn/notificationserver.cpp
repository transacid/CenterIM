/*
 * notificationserver.cpp
 * libmsn
 *
 * Created by Mark Rowe on Mon Mar 22 2004.
 * Copyright (c) 2004 Mark Rowe. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <msn/notificationserver.h>
#include <msn/errorcodes.h>
#include <msn/externals.h>
#include "md5.h"
#include <msn/util.h>
#include <curl/curl.h>
#include <algorithm>

#ifdef WIN32
#include <io.h>
#define EINPROGRESS WSAEINPROGRESS
#endif

namespace MSN
{
    std::map<std::string, void (NotificationServerConnection::*)(std::vector<std::string> &)> NotificationServerConnection::commandHandlers;
    
    static size_t msn_handle_curl_write(void *ptr, size_t size, size_t nmemb, void  *stream);
    static size_t msn_handle_curl_header(void *ptr, size_t size, size_t nmemb, void *stream) ;    
	
    NotificationServerConnection::NotificationServerConnection(NotificationServerConnection::AuthData & auth_)
	: Connection(), auth(auth_)
    {
	registerCommandHandlers();
    }
    
    NotificationServerConnection::NotificationServerConnection(std::string username_, std::string password_) 
	: Connection(), auth(username_, password_)
    {
	registerCommandHandlers();
    }
    
    NotificationServerConnection::~NotificationServerConnection()
    {
	ext::closingConnection(this);
	std::list<SwitchboardServerConnection *> list = _switchboardConnections;
	std::list<SwitchboardServerConnection *>::iterator i = list.begin();
	for (; i != list.end(); i++)
	{
	    delete *i;
	}
    }
    
    Connection *NotificationServerConnection::connectionWithSocket(int fd)
    {
	std::list<SwitchboardServerConnection *> & list = _switchboardConnections;
	std::list<SwitchboardServerConnection *>::iterator i = list.begin();
	
	if (this->sock == fd)
	    return this;
	
	for (; i != list.end(); i++)
	{
	    Connection *c = (*i)->connectionWithSocket(fd);
	    if (c)
		return c;
	}
	return NULL;
    }
    
    SwitchboardServerConnection *NotificationServerConnection::switchboardWithOnlyUser(std::string username)
    {
	std::list<SwitchboardServerConnection *> & list = _switchboardConnections;
	std::list<SwitchboardServerConnection *>::iterator i = list.begin();

	for (; i != list.end(); i++)
	{
	    if ((*i)->users.size() == 1 &&
		*((*i)->users.begin()) == username)
		return *i;
	}
	return NULL;
    }
    
    const std::list<SwitchboardServerConnection *> & NotificationServerConnection::switchboardConnections()
    {
	return _switchboardConnections;
    }
    
    void NotificationServerConnection::addSwitchboardConnection(SwitchboardServerConnection *c)
    {
	_switchboardConnections.push_back(c);
    }

    void NotificationServerConnection::removeSwitchboardConnection(SwitchboardServerConnection *c)
    {
	_switchboardConnections.remove(c);
    }
    
    void NotificationServerConnection::addCallback(NotificationServerCallback callback,
						   int trid, void *data)
    {
	this->callbacks[trid] = std::make_pair(callback, data);
    }
    
    void NotificationServerConnection::removeCallback(int trid)
    {
	this->callbacks.erase(trid);
    }
    
    void NotificationServerConnection::registerCommandHandlers()
    {
	if (commandHandlers.size() == 0)
	{
	    commandHandlers["OUT"] = &NotificationServerConnection::handle_OUT;
	    commandHandlers["ADD"] = &NotificationServerConnection::handle_ADD;
	    commandHandlers["REM"] = &NotificationServerConnection::handle_REM;
	    commandHandlers["BLP"] = &NotificationServerConnection::handle_BLP;
	    commandHandlers["GTC"] = &NotificationServerConnection::handle_GTC;
	    commandHandlers["REA"] = &NotificationServerConnection::handle_REA;
	    commandHandlers["CHG"] = &NotificationServerConnection::handle_CHG;
	    commandHandlers["CHL"] = &NotificationServerConnection::handle_CHL;
	    commandHandlers["ILN"] = &NotificationServerConnection::handle_ILN;
	    commandHandlers["NLN"] = &NotificationServerConnection::handle_NLN;
	    commandHandlers["FLN"] = &NotificationServerConnection::handle_FLN;
	    commandHandlers["MSG"] = &NotificationServerConnection::handle_MSG;
	    commandHandlers["ADG"] = &NotificationServerConnection::handle_ADG;
	    commandHandlers["RMG"] = &NotificationServerConnection::handle_RMG;
	    commandHandlers["REG"] = &NotificationServerConnection::handle_REG;
	}
    }
    
    void NotificationServerConnection::dispatchCommand(std::vector<std::string> & args)
    {
	std::map<std::string, void (NotificationServerConnection::*)(std::vector<std::string> &)>::iterator i = commandHandlers.find(args[0]);
	if (i != commandHandlers.end())
	    (this->*commandHandlers[args[0]])(args);
    }
    
    void NotificationServerConnection::sendMessage(std::string & recipient, Message *msg)
    {
	SwitchboardServerConnection *sb = this->switchboardWithOnlyUser(recipient);
	
	if (sb)
	    sb->sendMessage(recipient, msg);
	else
	    this->requestSwitchboardConnection(recipient, msg, NULL);
    }
    
    void NotificationServerConnection::handle_OUT(std::vector<std::string> & args)
    {
	if (args.size() > 1)
	{
	    if (args[1] == "OTH")
	    {
		ext::showError(this, "You have logged onto MSN twice at once. Your MSN session will now terminate.");
	    }
	    else if (args[1] == "SSD")
	    {
		ext::showError(this, "This MSN server is going down for maintenance. Your MSN session will now terminate.");
	    } else {
		ext::showError(this, (std::string("The MSN server has terminated the connection with an unknown reason code. Please report this code: ") + 
				      args[1]).c_str());
	    }
	}
	delete this;
    }
    
    void NotificationServerConnection::handle_ADD(std::vector<std::string> & args)
    {
	if (args[2] == "RL")
	{
	    ext::gotNewReverseListEntry(this, args[4], decodeURL(args[5]));
	}
	else
	{
	    int groupID = -1;
	    if (args.size() > 5)
		groupID = decimalFromString(args[5]);
	    
	    ext::addedListEntry(this, args[2], args[4], groupID);
	}
	ext::gotLatestListSerial(this, decimalFromString(args[3]));
    }

    void NotificationServerConnection::handle_REM(std::vector<std::string> & args)
    {
	int groupID = -1;
	if (args.size() > 4)
	    groupID = decimalFromString(args[4]);
	
	ext::removedListEntry(this, args[2], args[4], groupID);
	ext::gotLatestListSerial(this, decimalFromString(args[3]));
    }
    
    void NotificationServerConnection::handle_BLP(std::vector<std::string> & args)
    {
	ext::gotBLP(this, args[3][0]);
	ext::gotLatestListSerial(this, decimalFromString(args[3]));
    }

    void NotificationServerConnection::handle_GTC(std::vector<std::string> & args)
    {
	ext::gotGTC(this, args[3][0]);
	ext::gotLatestListSerial(this, decimalFromString(args[3]));
    }
    
    void NotificationServerConnection::handle_REA(std::vector<std::string> & args)
    {
	ext::gotFriendlyName(this, decodeURL(args[4]));
	ext::gotLatestListSerial(this, decimalFromString(args[2]));        
    }
    
    void NotificationServerConnection::handle_CHG(std::vector<std::string> & args)
    {
	ext::changedStatus(this, args[2]);       
    }
    
    void NotificationServerConnection::handle_CHL(std::vector<std::string> & args)
    {
	md5_state_t state;
	md5_byte_t digest[16];
	int a;
	
	md5_init(&state);
	md5_append(&state, (md5_byte_t *)(args[2].c_str()), args[2].size());
	md5_append(&state, (md5_byte_t *)"VT6PX?UQTM4WM%YR", 16);
	md5_finish(&state, digest);
	
	std::stringstream buf_;
	buf_ << "QRY " << trid++ << " PROD0038W!61ZTF9 32\r\n";
	write(buf_);
	
	
	char hexdigest[3];
	for (a = 0; a < 16; a++)
	{
	    sprintf(hexdigest, "%02x", digest[a]);
	    write(std::string(hexdigest, 2), false);
	}
    }
    
    void NotificationServerConnection::handle_ILN(std::vector<std::string> & args)
    {
	ext::buddyChangedStatus(this, args[3], decodeURL(args[4]), args[2]);
    }
    
    void NotificationServerConnection::handle_NLN(std::vector<std::string> & args)
    {
	ext::buddyChangedStatus(this, args[2], decodeURL(args[3]), args[1]);
    }
    
    void NotificationServerConnection::handle_FLN(std::vector<std::string> & args)
    {
	ext::buddyOffline(this, args[1]);
    }    
    
    void NotificationServerConnection::handle_MSG(std::vector<std::string> & args)
    {
	Connection::handle_MSG(args);
    }
    
    void NotificationServerConnection::handle_RNG(std::vector<std::string> & args)
    {
	SwitchboardServerConnection::AuthData auth = SwitchboardServerConnection::AuthData(this->auth.username,
											   args[1],
											   args[4]);
	SwitchboardServerConnection *newSBconn = new SwitchboardServerConnection(auth, this);
	this->addSwitchboardConnection(newSBconn);  
	std::pair<std::string, int> server_address = splitServerAddress(args[2]);  
	newSBconn->connect(server_address.first, server_address.second);        
    }
    
    void NotificationServerConnection::handle_ADG(std::vector<std::string> & args)
    {
	ext::addedGroup(this, args[3], decimalFromString(args[4]));
	ext::gotLatestListSerial(this, decimalFromString(args[2]));
    }

    void NotificationServerConnection::handle_RMG(std::vector<std::string> & args)
    {
	ext::removedGroup(this, decimalFromString(args[3]));
	ext::gotLatestListSerial(this, decimalFromString(args[2]));
    }

    void NotificationServerConnection::handle_REG(std::vector<std::string> & args)
    {
	ext::renamedGroup(this, decimalFromString(args[3]), args[4]);
	ext::gotLatestListSerial(this, decimalFromString(args[2]));
    }


    void NotificationServerConnection::setState(std::string state)
    {
	std::stringstream buf_;
	buf_ << "CHG " << trid++ << " " << state << " 0\r\n";
	write(buf_);        
    }
    
    void NotificationServerConnection::setBLP(char setting)
    {
	std::stringstream buf_;
	buf_ << "BLP " << trid++ << " " << setting << "L\r\n";
	write(buf_);        
    }

    void NotificationServerConnection::setGTC(char setting)
    {
	std::stringstream buf_;
	buf_ << "GTC " << trid++ << " " << setting << "\r\n";
	write(buf_);        
    }
    
    void NotificationServerConnection::setFriendlyName(std::string friendlyName) throw (std::runtime_error)
    {
	if (friendlyName.size() > 387)
	    throw std::runtime_error("Friendly name too long!");
	
	std::stringstream buf_;  
	buf_ << "REA " << trid++ << " " << this->auth.username << " " << encodeURL(friendlyName) << "\r\n";
	write(buf_);        
    }

    void NotificationServerConnection::addToList(std::string list, std::string buddyName)
    {
	std::stringstream buf_;
	buf_ << "ADD " << trid++ << " " << list << " " << buddyName << " " << buddyName << "\r\n";
	write(buf_);        
    }
    
    void NotificationServerConnection::removeFromList(std::string list, std::string buddyName)
    {
	std::stringstream buf_;
	buf_ << "REM " << trid++ << " " << list << " " << buddyName << "\r\n";
	write(buf_);        
    }
    
    void NotificationServerConnection::addToGroup(std::string buddyName, int groupID)
    {
	std::stringstream buf_;
	buf_ << "ADD " << trid++ << " " << "FL" << " " << buddyName << " " << buddyName <<  groupID << "\r\n";
	write(buf_);
    }
    
    void NotificationServerConnection::removeFromGroup(std::string buddyName, int groupID)
    {
	std::stringstream buf_;
	buf_ << "REM " << trid++ << " " << "FL" << " " << buddyName << groupID << "\r\n";
	write(buf_);
    }
    
    void NotificationServerConnection::addGroup(std::string groupName)
    {
	std::stringstream buf_;
	buf_ << "ADG " << trid++ << encodeURL(groupName) << " " << 0 << "\r\n";
	write(buf_);        
    }
    
    void NotificationServerConnection::removeGroup(int groupID)
    {
	std::stringstream buf_;
	buf_ << "RMG " << trid++ << groupID << "\r\n";
	write(buf_);
    }
    
    void NotificationServerConnection::renameGroup(int groupID, std::string newGroupName)
    {
	std::stringstream buf_;
	buf_ << "REG " << trid++ << groupID << " " << encodeURL(newGroupName) << " " << 0 << "\r\n";
	write(buf_);
    }
    
    
    void NotificationServerConnection::synchronizeLists(int version)
    {
	ListSyncInfo *info = new ListSyncInfo(version);
	
	std::stringstream buf_;
	buf_ << "SYN " << trid << " " << version << "\r\n";
	write(buf_);
	
	this->addCallback(&NotificationServerConnection::callback_SyncData, trid, (void *)info);
	this->synctrid = trid++;        
    }
    
    void NotificationServerConnection::sendPing()
    {
	write("PNG\r\n");        
    }
    
    void NotificationServerConnection::requestSwitchboardConnection(std::string username, Message *msg, void *tag)
    {
	SwitchboardServerConnection::AuthData *auth = new SwitchboardServerConnection::AuthData(this->auth.username, 
											       username, (msg ? new Message(*msg) : NULL), 
											       tag);
	std::stringstream buf_;
	buf_ << "XFR " << trid << " SB\r\n";
	write(buf_);
	this->addCallback(&NotificationServerConnection::callback_TransferToSwitchboard, trid++, (void *)auth);
    }
    
    void NotificationServerConnection::requestSwitchboardConnection(void *tag)
    {
	requestSwitchboardConnection("", NULL, tag);
    }
    
    template <class _Tp>
    class _sameUserName
    {
	Buddy buddy;
public:
	_sameUserName(const _Tp &__u) : buddy(__u) {};
	bool operator()(const _Tp &__x) { return __x.userName == buddy.userName; }
    };
    
    void NotificationServerConnection::checkReverseList(ListSyncInfo *info)
    {
	std::list<Buddy> & flist = info->reverseList;
	std::list<Buddy> & alist = info->allowList;
	std::list<Buddy> & blist = info->blockList;
	std::list<Buddy>::iterator flist_i;
	std::list<Buddy>::iterator alist_i;
	std::list<Buddy>::iterator blist_i;
	
	for (flist_i = flist.begin(); flist_i != flist.end(); flist_i++)
	{
	    if (std::count_if(alist.begin(), alist.end(), _sameUserName<Buddy>(*flist_i)) == 0 &&
		std::count_if(blist.begin(), blist.end(), _sameUserName<Buddy>(*flist_i)) == 0)
	    {
		ext::gotNewReverseListEntry(this, (*flist_i).userName, (*flist_i).friendlyName);
	    }
	}
    }
    
    void NotificationServerConnection::socketConnectionCompleted()
    {
	Connection::socketConnectionCompleted();
	ext::unregisterSocket(this->sock);
	ext::registerSocket(this->sock, 1, 0);
    }
    
    void NotificationServerConnection::connect(std::string hostname, unsigned int port)
    {
	connectinfo *info = new connectinfo(this->auth.username, this->auth.password);
	
	if ((this->sock = ext::connectToServer(hostname, port, &this->connected)) == -1)
	{
	    ext::showError(this, "Could not connect to MSN server");
	    ext::closingConnection(this);
	    return;
	}
	
	ext::registerSocket(this->sock, 0, 1);
	
	std::stringstream buf_;
	buf_ << "VER " << trid << " MSNP8\r\n";
	this->write(buf_);
	this->addCallback(&NotificationServerConnection::callback_NegotiateCVR, trid++, (void *)info);
    }
    
    void NotificationServerConnection::handleIncomingData()
    {
	while (this->isWholeLineAvailable())
	{
	    std::vector<std::string> args = this->getLine();
	    if (args[0] == "MSG" || args[0] == "NOT")
	    {
		int dataLength = decimalFromString(args[3]);
		if (this->readBuffer.find("\r\n") + 2 + dataLength > this->readBuffer.size())
		    return;
	    }
	    this->readBuffer = this->readBuffer.substr(this->readBuffer.find("\r\n") + 2);
	    int trid = 0;
	    
	    if (args.size() >= 6 && args[0] == "XFR" && args[2] == "NS")
	    {
		// XFR TrID NS NotificationServerIP:Port 0 ThisServerIP:Port
		//  0    1  2             3              4        5    
		this->callbacks.clear(); // delete the callback data
		
		ext::unregisterSocket(this->sock);
		close(this->sock);
		
		std::pair<std::string, int> server_address = splitServerAddress(args[3]);
		this->connect(server_address.first, server_address.second);
		return;
	    }
	    
	    if (args.size() >= 7 && args[0] == "RNG")
	    {
		// RNG SessionID SwitchboardServerIP:Port CKI AuthString InvitingUser InvitingDisplayName
		// 0      1                 2              3       4           5             6
		this->handle_RNG(args);
		return;
	    }
	    
	    if (args.size() >= 1 && args[0] == "QNG")
	    {
		// QNG
		//  0
		
		// ping response, ignore
		return;
	    }
	    
	    if ((args.size() >= 4 && (args[0] == "LST" || args[0] == "LSG")) ||
		(args.size() >= 2 && (args[0] == "GTC" || args[0] == "BLP")) ||
		(args.size() >= 3 && args[0] == "BPR")
		)
	    {
		// LST UserName FriendlyName UserLists [GroupNumbers]
		//  0      1          2          3           4
		//
		// or
		// (GTC|BLP) [TrID] [ListVersion] Setting
		//     0        1        2          4
		
		if (this->synctrid)
		{
		    trid = this->synctrid;
		}
		else
		{
		    trid = decimalFromString(args[1]);
		}
	    } 
	    else if (args.size() > 1)
	    {
		try
		{
		    trid = decimalFromString(args[1]);
		}
		catch (...)
		{
		}
	    }
	    
	    if (!this->callbacks.empty() && trid > 0)
	    {
		if (this->callbacks.find(trid) != this->callbacks.end())
		{
		    (this->*(this->callbacks[trid].first))(args, trid, this->callbacks[trid].second);
		    continue;
		}
	    }
	    
	    if (isdigit(args[0][0]))
		this->showError(decimalFromString(args[0]));
	    else
		this->dispatchCommand(args);
	}
    }
    
    void NotificationServerConnection::callback_SyncData(std::vector<std::string> & args, int trid, void *data) throw (std::runtime_error)
    {
	ListSyncInfo *info = static_cast<ListSyncInfo *>(data);
	
	if (args[0] == "SYN")
	{
	    if (info->listVersion == decimalFromString(args[2]))
	    {
		delete info;
		info = NULL;
		this->removeCallback(trid);
		ext::gotBuddyListInfo(this, NULL);
		return;
	    }
	    else 
	    {
		info->listVersion = decimalFromString(args[2]);
		info->usersRemaining = decimalFromString(args[3]);
		info->groupsRemaining = decimalFromString(args[4]);
		ext::gotLatestListSerial(this, info->listVersion);
	    }
	}
	else if (args[0] == "LST")
	{
	    int list = decimalFromString(args[3]);
	    if ((list & ListSyncInfo::LST_FL) == ListSyncInfo::LST_FL)
	    {
		info->forwardList.push_back(Buddy(args[1], decodeURL(args[2])));
		std::vector<std::string> groups = splitString(args[4], ",");
		std::vector<std::string>::iterator i = groups.begin();
		for (; i != groups.end(); i++)
		{
		    int group = atoi(i->c_str());
		    info->groups[group].buddies.push_back(&(info->forwardList.back()));
		    info->forwardList.back().groups.push_back(&info->groups[group]);
		}
	    }
	    if ((list & ListSyncInfo::LST_RL) == ListSyncInfo::LST_RL)
	    {
		info->reverseList.push_back(Buddy(args[1], decodeURL(args[2])));
	    }
	    if ((list & ListSyncInfo::LST_AL) == ListSyncInfo::LST_AL)
	    {
		info->allowList.push_back(Buddy(args[1], decodeURL(args[2])));
	    }
	    if ((list & ListSyncInfo::LST_BL) == ListSyncInfo::LST_BL)
	    {
		info->blockList.push_back(Buddy(args[1], decodeURL(args[2])));
	    }
	    info->usersRemaining--;
	    if (info->usersRemaining == 0)
	    {
		info->progress |= ListSyncInfo::LST_FL | ListSyncInfo::LST_RL | ListSyncInfo::LST_AL | ListSyncInfo::LST_BL;
	    }
	}
	else if (args[0] == "GTC")
	{
	    info->reverseListPrompting = args[1][0];
	    info->progress |= ListSyncInfo::COMPLETE_GTC;
	    ext::gotGTC(this, info->reverseListPrompting);
	}
	else if (args[0] == "BLP")
	{
	    info->privacySetting = args[1][0];
	    info->progress |= ListSyncInfo::COMPLETE_BLP;
	    ext::gotBLP(this, info->privacySetting);
	}
	else if (args[0] == "LSG")
	{
	    int groupID = decimalFromString(args[1]);
	    Group g(groupID, decodeURL(args[2]));
	    info->groups[groupID] = g;
	}
	else if (args[0] == "BPR")
	{
	    bool enabled = true;
	    if (! std::isdigit(args[2][0]))
		enabled = (decodeURL(args[2]) == "Y");
	    
	    info->forwardList.back().phoneNumbers.push_back(Buddy::PhoneNumber(args[1], decodeURL(args[2]), enabled));
	}
	else
	    throw std::runtime_error("Unexpected sync data");
		
	if (info->progress == (ListSyncInfo::LST_FL | ListSyncInfo::LST_RL |
			       ListSyncInfo::LST_AL | ListSyncInfo::LST_BL | 
			       ListSyncInfo::COMPLETE_BLP | ListSyncInfo::COMPLETE_GTC))
	{
	    this->removeCallback(trid);
	    this->checkReverseList(info);
	    ext::gotBuddyListInfo(this, info);
	    this->synctrid = 0;
	    delete info;
	}
	else if (info->progress > 63 || info->progress < 0)
	    throw std::runtime_error("Corrupt sync progress!");
    }
    
    
    void NotificationServerConnection::callback_NegotiateCVR(std::vector<std::string> & args, int trid, void *data)
    {
	connectinfo * info = (connectinfo *) data;
	this->removeCallback(trid);
	
	if (args.size() >= 3 && args[0] != "VER" || args[2] != "MSNP8") // if either *differs*...
	{
	    ext::showError(NULL, "Protocol negotiation failed");
	    delete info;
	    ext::unregisterSocket(this->sock);
	    close(this->sock);
	    this->sock = -1;
	    return;
	}
	
#ifdef DEBUG
//	fprintf(stderr, "MSN Plugin: Negotiating CVR\n");
#endif
	
	std::stringstream buf_;
	buf_ << "CVR " << trid << " 0x0409 winnt 5.2 i386 MSNMSGR 6.0.0250 MSMSGS " << info->username << "\r\n";
	this->write(buf_);
	this->addCallback(&NotificationServerConnection::callback_RequestUSR, trid++, (void *) data);
    }
    
    void NotificationServerConnection::callback_TransferToSwitchboard(std::vector<std::string> & args, int trid, void *data)
    {
	SwitchboardServerConnection::AuthData *auth = static_cast<SwitchboardServerConnection::AuthData *>(data);
	this->removeCallback(trid);
	
	if (args[0] != "XFR")
	{
	    this->showError(decimalFromString(args[0]));
	    delete auth;
	    return;
	}
	
	auth->cookie = args[5];
	auth->sessionID = "";
	
	SwitchboardServerConnection *newconn = new SwitchboardServerConnection(*auth, this);
	
	this->addSwitchboardConnection(newconn);
	std::pair<std::string, int> server_address = splitServerAddress(args[3]);
	newconn->connect(server_address.first, server_address.second);
	
	delete auth;
    }
    
    void NotificationServerConnection::callback_RequestUSR(std::vector<std::string> & args, int trid, void *data)
    {
	connectinfo *info = (connectinfo *)data;
	this->removeCallback(trid);
	
	if (args.size() > 1 && args[0] != "CVR") // if*differs*...
	{
	    ext::showError(NULL, "Protocol negotiation failed");
	    delete info;
	    ext::unregisterSocket(this->sock);
	    close(this->sock);
	    this->sock = -1;
	    return;
	}
	
#ifdef DEBUG
//	fprintf(stderr, "MSN Plugin: Requesting USR\n");
#endif
	
	std::stringstream buf_;
	buf_ << "USR " << trid << " TWN I " << info->username << "\r\n";
	this->write(buf_);
	
	this->addCallback(&NotificationServerConnection::callback_PassportAuthentication, trid++, (void *) data);
    }    
    
    void NotificationServerConnection::callback_PassportAuthentication(std::vector<std::string> & args, int trid, void * data)
    {
	connectinfo * info;
	
	CURL *curl;
	CURLcode ret;
	curl_slist *slist = NULL;
	std::string auth;
	char *uname, *pword;
	std::string proxy;
	
	info=(connectinfo *)data;
	this->removeCallback(trid);
	
	if (isdigit(args[0][0]))
	{
	    this->showError(decimalFromString(args[0]));
	    delete this;
	    delete info;
	    return;
	}
	
	if (args.size() >= 4 && args[4].empty()) {
	    delete this;
	    delete info;
	    return;
	}
	
	/* Now to fire off some HTTPS login */
	/* args[4] contains the most interesting part we need to send on */
	curl = curl_easy_init();
	if (curl == NULL) {
	    delete this;
	    delete info;
	    return;
	}
	
	proxy = ext::getSecureHTTPProxy();
	if (! proxy.empty()) 
	    ret = curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
	else    
	    ret = CURLE_OK;
	
#ifdef DEBUG
//	fprintf(stderr, "MSN Plugin: Will connect to login.passport.com using proxy: %s\n", proxy.empty() ? "None" : proxy);
#endif
	
	if (ret == CURLE_OK)
	    ret = curl_easy_setopt(curl, CURLOPT_URL, "https://login.passport.com/login2.srf");
	
	uname = curl_escape(info->username.c_str(), 0);
	pword = curl_escape(info->password.c_str(), 0);
	auth = std::string("Authorization: Passport1.4 OrgVerb=GET,OrgURL=http%3A%2F%2Fmessenger%2Emsn%2Ecom,sign-in=") + uname + ",pwd=" + pword + ","+ args[4];
	free(uname);
	free(pword);
	slist = curl_slist_append(slist, auth.c_str());
	
	if (ret == CURLE_OK)
	    ret = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
	if (ret == CURLE_OK)
	    ret = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	if (ret == CURLE_OK)
	    ret = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	/*if (ret == CURLE_OK)
	    ret = curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1);*/
	/*  if (ret == CURLE_OK)
	    ret = curl_easy_setopt(curl, CURLOPT_USERAGENT, "msnlib/1.0 (Proteus)");*/
	if (ret == CURLE_OK)
	    ret = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &msn_handle_curl_write);
	/*if (ret == CURLE_OK)
	    ret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, info);*/
	if (ret == CURLE_OK)
	    ret = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &msn_handle_curl_header);
	if (ret == CURLE_OK)
	    ret = curl_easy_setopt(curl, CURLOPT_WRITEHEADER, info);
	
#ifdef DEBUG
//	fprintf(stderr, "MSN Plugin: Connecting to login.passport.com\n");
#endif
	
	if (ret == CURLE_OK)
	    ret = curl_easy_perform(curl);
	
	curl_easy_cleanup(curl);
	curl_slist_free_all(slist);
	
	/* ok, if auth succeeded info->cookie is now the cookie to pass back */
	if (info->cookie.empty())
	{
	    // No authentication cookie /usually/ means that authentication failed.
	    this->showError(911);
	    
	    delete this;
	    delete info;
	    return;
	}
	
#ifdef DEBUG
//	fprintf(stderr, "MSN Plugin: Returning login cookie to MSN\n");
//	fprintf(stderr, "MSN Plugin: Cookie: %s\n", info->cookie);
#endif
	
	std::stringstream buf_;
	buf_ << "USR " << trid << " TWN S " << info->cookie << "\r\n";
	this->write(buf_);
	this->addCallback(&NotificationServerConnection::callback_AuthenticationComplete, trid++, (void *) data);
    }
    
    void NotificationServerConnection::callback_AuthenticationComplete(std::vector<std::string> & args, int trid, void * data)
    {
	connectinfo * info = (connectinfo *) data;
	this->removeCallback(trid);
	
	if (isdigit(args[0][0]))
	{
	    this->showError(decimalFromString(args[0]));
	    delete info;
	    delete this;
	    return;
	}
	
	ext::gotFriendlyName(this, decodeURL(args[4]));
	
	delete info;
	
	trid++;
	
	ext::gotNewConnection(this);
    }    
    
    
    static size_t msn_handle_curl_write(void *ptr, size_t size, size_t nmemb, void  *stream) {
	return size * nmemb;
    }
    
    static size_t msn_handle_curl_header(void *ptr, size_t size, size_t nmemb, void *stream) 
    {
	connectinfo * info;
	std::string cookiedata;
	
	info = (connectinfo *)stream;
	
	if ((size * nmemb) < strlen("Authentication-Info:"))
	    return (size * nmemb);
	
	std::string headers_ = std::string((char *)ptr, size * nmemb);
	Message::Headers headers = Message::Headers(headers_);
	cookiedata = headers["Authentication-Info:"];
#ifdef DEBUG
//	printf("MSN Plugin: Authentication-Info header: %s\n", cookiedata.empty() ? "Not found" : "Found");
#endif
	if (! cookiedata.empty()) 
	{
	    unsigned int pos = cookiedata.find(",from-PP='");
	    if (pos == std::string::npos) {
		info->cookie = "";
	    } else {
		info->cookie = cookiedata.substr(pos + strlen(",from-PP='"));
		pos = info->cookie.find("'");
		if (pos != std::string::npos)
		    info->cookie = info->cookie.substr(0, pos);
	    }
	}
	
	return (size * nmemb);
    }    
}
