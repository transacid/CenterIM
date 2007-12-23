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
#include <msn/md5.h>
#include <msn/util.h>
#include <curl/curl.h>
#include <algorithm>
#include <cctype>
#include <cassert>

#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

#include <stdio.h>
#include <cstring>

#ifndef curl_free
#define curl_free free
#endif

namespace MSN
{
    
    static size_t msn_handle_curl_write(void *ptr, size_t size, size_t nmemb, void  *stream);
    static size_t msn_handle_curl_header(void *ptr, size_t size, size_t nmemb, void *stream) ;    
            
    NotificationServerConnection::NotificationServerConnection(NotificationServerConnection::AuthData & auth_, Callbacks & cb_)
        : Connection(), auth(auth_), externalCallbacks(cb_), _connectionState(NS_DISCONNECTED), _nextPing(50), commandHandlers()
    {
        registerCommandHandlers();
    }
    
    NotificationServerConnection::NotificationServerConnection(Passport username_, std::string password_, Callbacks & cb_) 
        : Connection(), auth(username_, password_), externalCallbacks(cb_), _connectionState(NS_DISCONNECTED), commandHandlers()
    {
        registerCommandHandlers();
    }
    
    NotificationServerConnection::NotificationServerConnection(Callbacks & cb_) : Connection(), auth(Passport(), ""), externalCallbacks(cb_), _connectionState(NS_DISCONNECTED), commandHandlers()
    {
        registerCommandHandlers();
    }
    
    NotificationServerConnection::~NotificationServerConnection()
    {
        if (this->connectionState() != NS_DISCONNECTED)
            this->disconnect();
    }
    
    Connection *NotificationServerConnection::connectionWithSocket(int fd)
    {
        this->assertConnectionStateIsNot(NS_DISCONNECTED);
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
    
    SwitchboardServerConnection *NotificationServerConnection::switchboardWithOnlyUser(Passport username)
    {
        if (this->connectionState() >= NS_CONNECTED)
        {
            std::list<SwitchboardServerConnection *> & list = _switchboardConnections;
            std::list<SwitchboardServerConnection *>::iterator i = list.begin();
            
            for (; i != list.end(); i++)
            {
                if ((*i)->users.size() == 1 &&
                    *((*i)->users.begin()) == username)
                    return *i;
            }
        }
        return NULL;
    }
    
    const std::list<SwitchboardServerConnection *> & NotificationServerConnection::switchboardConnections()
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        return _switchboardConnections;
    }
    
    void NotificationServerConnection::addSwitchboardConnection(SwitchboardServerConnection *c)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);        
        _switchboardConnections.push_back(c);
    }

    void NotificationServerConnection::removeSwitchboardConnection(SwitchboardServerConnection *c)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);        
        _switchboardConnections.remove(c);
    }
    
    void NotificationServerConnection::addCallback(NotificationServerCallback callback,
                                                   int trid, void *data)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTING);        
        this->callbacks[trid] = std::make_pair(callback, data);
    }
    
    void NotificationServerConnection::removeCallback(int trid)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTING);        
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
            commandHandlers["NOT"] = &NotificationServerConnection::handle_NOT;
            commandHandlers["FLN"] = &NotificationServerConnection::handle_FLN;
            commandHandlers["MSG"] = &NotificationServerConnection::handle_MSG;
            commandHandlers["ADG"] = &NotificationServerConnection::handle_ADG;
            commandHandlers["RMG"] = &NotificationServerConnection::handle_RMG;
            commandHandlers["REG"] = &NotificationServerConnection::handle_REG;
        }
    }
    
    void NotificationServerConnection::dispatchCommand(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);        
        std::map<std::string, void (NotificationServerConnection::*)(std::vector<std::string> &)>::iterator i = commandHandlers.find(args[0]);
        if (i != commandHandlers.end())
            (this->*commandHandlers[args[0]])(args);
    }
    
    void NotificationServerConnection::handle_OUT(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);        
        if (args.size() > 1)
        {
            if (args[1] == "OTH")
            {
                this->myNotificationServer()->externalCallbacks.showError(this, "You have logged onto MSN twice at once. Your MSN session will now terminate.");
            }
            else if (args[1] == "SSD")
            {
                this->myNotificationServer()->externalCallbacks.showError(this, "This MSN server is going down for maintenance. Your MSN session will now terminate.");
            } else {
                this->myNotificationServer()->externalCallbacks.showError(this, (std::string("The MSN server has terminated the connection with an unknown reason code. Please report this code: ") + 
                                      args[1]).c_str());
            }
        }
        this->disconnect();
    }
    
    void NotificationServerConnection::handle_ADD(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);        
        if (args[2] == "RL")
        {
            this->myNotificationServer()->externalCallbacks.gotNewReverseListEntry(this, args[4], decodeURL(args[5]));
        }
        else
        {
            int groupID = -1;
            if (args.size() > 5)
                groupID = decimalFromString(args[5]);
            
            this->myNotificationServer()->externalCallbacks.addedListEntry(this, args[2], args[4], groupID);
        }
        this->myNotificationServer()->externalCallbacks.gotLatestListSerial(this, decimalFromString(args[3]));
    }

    void NotificationServerConnection::handle_REM(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        int groupID = -1;
        if (args.size() > 4)
            groupID = decimalFromString(args[4]);
        
        this->myNotificationServer()->externalCallbacks.removedListEntry(this, args[2], args[4], groupID);
        this->myNotificationServer()->externalCallbacks.gotLatestListSerial(this, decimalFromString(args[3]));
    }
    
    void NotificationServerConnection::handle_BLP(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        this->myNotificationServer()->externalCallbacks.gotBLP(this, args[3][0]);
        this->myNotificationServer()->externalCallbacks.gotLatestListSerial(this, decimalFromString(args[3]));
    }

    void NotificationServerConnection::handle_GTC(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        this->myNotificationServer()->externalCallbacks.gotGTC(this, args[3][0]);
        this->myNotificationServer()->externalCallbacks.gotLatestListSerial(this, decimalFromString(args[3]));
    }
    
    void NotificationServerConnection::handle_REA(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        this->myNotificationServer()->externalCallbacks.gotFriendlyName(this, decodeURL(args[4]));
        this->myNotificationServer()->externalCallbacks.gotLatestListSerial(this, decimalFromString(args[2]));        
    }
    
    void NotificationServerConnection::handle_CHG(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        this->myNotificationServer()->externalCallbacks.changedStatus(this, buddyStatusFromString(args[2]));
    }
    
    void NotificationServerConnection::handle_CHL(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        md5_state_t state;
        md5_byte_t digest[16];
        int a;
        
        md5_init(&state);
        md5_append(&state, (md5_byte_t *)(args[2].c_str()), (int) args[2].size());
        md5_append(&state, (md5_byte_t *)"VT6PX?UQTM4WM%YR", 16);
        md5_finish(&state, digest);
        
        std::ostringstream buf_;
        buf_ << "QRY " << this->trID++ << " PROD0038W!61ZTF9 32\r\n";
        if (write(buf_) != buf_.str().size())
            return;
        
        
        char hexdigest[3];
        for (a = 0; a < 16; a++)
        {
            sprintf(hexdigest, "%02x", digest[a]);
            write(std::string(hexdigest, 2), false);
        }
    }
    
    void NotificationServerConnection::handle_ILN(std::vector<std::string> & args)
    {
        this->assertConnectionStateIs(NS_CONNECTED);
        this->myNotificationServer()->externalCallbacks.buddyChangedStatus(this, args[3], decodeURL(args[4]), buddyStatusFromString(args[2]));
    }
    
    void NotificationServerConnection::handle_NLN(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        this->myNotificationServer()->externalCallbacks.buddyChangedStatus(this, args[2], decodeURL(args[3]), buddyStatusFromString(args[1]));
    }
    
    void NotificationServerConnection::handle_NOT(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        int notlen = decimalFromString(args[1]);
        this->readBuffer = this->readBuffer.substr(notlen);
    }
    
    void NotificationServerConnection::handle_FLN(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        this->myNotificationServer()->externalCallbacks.buddyOffline(this, args[1]);
    }    
    
    void NotificationServerConnection::handle_MSG(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        Connection::handle_MSG(args);
    }
    
    void NotificationServerConnection::handle_RNG(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        SwitchboardServerConnection::AuthData auth = SwitchboardServerConnection::AuthData(this->auth.username,
                                                                                           args[1],
                                                                                           args[4]);
        SwitchboardServerConnection *newSBconn = new SwitchboardServerConnection(auth, *this);
        this->addSwitchboardConnection(newSBconn);  
        std::pair<std::string, int> server_address = splitServerAddress(args[2]);  
        newSBconn->connect(server_address.first, server_address.second);        
    }
    
    void NotificationServerConnection::handle_ADG(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);        
        this->myNotificationServer()->externalCallbacks.addedGroup(this, decodeURL(args[3]), decimalFromString(args[4]));
        this->myNotificationServer()->externalCallbacks.gotLatestListSerial(this, decimalFromString(args[2]));
    }

    void NotificationServerConnection::handle_RMG(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        this->myNotificationServer()->externalCallbacks.removedGroup(this, decimalFromString(args[3]));
        this->myNotificationServer()->externalCallbacks.gotLatestListSerial(this, decimalFromString(args[2]));
    }

    void NotificationServerConnection::handle_REG(std::vector<std::string> & args)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        this->myNotificationServer()->externalCallbacks.renamedGroup(this, decimalFromString(args[3]), decodeURL(args[4]));
        this->myNotificationServer()->externalCallbacks.gotLatestListSerial(this, decimalFromString(args[2]));
    }


    void NotificationServerConnection::setState(BuddyStatus state)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        std::ostringstream buf_;
        buf_ << "CHG " << this->trID++ << " " << buddyStatusToString(state) << " 0\r\n";
        write(buf_);        
    }
    
    void NotificationServerConnection::setBLP(char setting)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        std::ostringstream buf_;
        buf_ << "BLP " << this->trID++ << " " << setting << "L\r\n";
        write(buf_);        
    }

    void NotificationServerConnection::setGTC(char setting)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        std::ostringstream buf_;
        buf_ << "GTC " << this->trID++ << " " << setting << "\r\n";
        write(buf_);        
    }
    
    void NotificationServerConnection::setFriendlyName(std::string friendlyName) throw (std::runtime_error)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        if (friendlyName.size() > 387)
            throw std::runtime_error("Friendly name too long!");
        
        std::ostringstream buf_;  
        buf_ << "REA " << this->trID++ << " " << this->auth.username << " " << encodeURL(friendlyName) << "\r\n";
        write(buf_);        
    }

    void NotificationServerConnection::addToList(std::string list, Passport buddyName)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        std::ostringstream buf_;
        buf_ << "ADD " << this->trID++ << " " << list << " " << buddyName << " " << buddyName << "\r\n";
        write(buf_);        
    }
    
    void NotificationServerConnection::removeFromList(std::string list, Passport buddyName)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        std::ostringstream buf_;
        buf_ << "REM " << this->trID++ << " " << list << " " << buddyName << "\r\n";
        write(buf_);        
    }
    
    void NotificationServerConnection::addToGroup(Passport buddyName, int groupID)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        std::ostringstream buf_;
        buf_ << "ADD " << this->trID++ << " " << "FL" << " " << buddyName << " " << buddyName <<  " " << groupID << "\r\n";
        write(buf_);
    }
    
    void NotificationServerConnection::removeFromGroup(Passport buddyName, int groupID)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        std::ostringstream buf_;
        buf_ << "REM " << this->trID++ << " " << "FL" << " " << buddyName;
        if( groupID > 0 ) {
            buf_ << " " << groupID;
        }
        buf_ << "\r\n";
        write(buf_);
    }
    
    void NotificationServerConnection::addGroup(std::string groupName)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        std::ostringstream buf_;
        buf_ << "ADG " << this->trID++ << " " << encodeURL(groupName) << " " << 0 << "\r\n";
        write(buf_);        
    }
    
    void NotificationServerConnection::removeGroup(int groupID)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        std::ostringstream buf_;
        buf_ << "RMG " << this->trID++ << " " << groupID << "\r\n";
        write(buf_);
    }
    
    void NotificationServerConnection::renameGroup(int groupID, std::string newGroupName)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        std::ostringstream buf_;
        buf_ << "REG " << this->trID++ << " " << groupID << " " << encodeURL(newGroupName) << " " << 0 << "\r\n";
        write(buf_);
    }
    
    
    void NotificationServerConnection::synchronizeLists(int version)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        this->assertConnectionStateIsNot(NS_SYNCHRONISING);
        ListSyncInfo *info = new ListSyncInfo(version);
        
        std::ostringstream buf_;
        buf_ << "SYN " << this->trID << " " << version << "\r\n";
        if (write(buf_) != buf_.str().size())
            return;
        
        this->addCallback(&NotificationServerConnection::callback_SyncData, this->trID, (void *)info);
        this->synctrid = this->trID++;
        this->setConnectionState(NS_SYNCHRONISING);
    }
    
    void NotificationServerConnection::sendPing()
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        write("PNG\r\n");        
    }
    
    void NotificationServerConnection::requestSwitchboardConnection(const void *tag)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        SwitchboardServerConnection::AuthData *auth = new SwitchboardServerConnection::AuthData(this->auth.username, tag);
        std::ostringstream buf_;
        buf_ << "XFR " << this->trID << " SB\r\n";
        if (write(buf_) != buf_.str().size())
            return;
        
        this->addCallback(&NotificationServerConnection::callback_TransferToSwitchboard, this->trID++, (void *)auth);        
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
                this->myNotificationServer()->externalCallbacks.gotNewReverseListEntry(this, (*flist_i).userName, (*flist_i).friendlyName);
            }
        }
    }
    
    void NotificationServerConnection::socketConnectionCompleted()
    {
        this->assertConnectionStateIs(NS_CONNECTING);
        this->setConnectionState(NS_CONNECTED);
        
        Connection::socketConnectionCompleted();
        
        // If an error occurs in Connection::socketConnectionCompleted, we
        // will be disconnected before we get here.
        if (this->connectionState() != NS_DISCONNECTED)
        {
            this->myNotificationServer()->externalCallbacks.unregisterSocket(this->sock);
            this->myNotificationServer()->externalCallbacks.registerSocket(this->sock, 1, 0);
        }
    }
    
    void NotificationServerConnection::connect(const std::string & hostname, unsigned int port)
    {
        this->assertConnectionStateIs(NS_DISCONNECTED);
        connectinfo *info = new connectinfo(this->auth.username, this->auth.password);
        
        if ((this->sock = this->myNotificationServer()->externalCallbacks.connectToServer(hostname, port, &this->connected)) == -1)
        {
            this->myNotificationServer()->externalCallbacks.showError(this, "Could not connect to MSN server");
            this->myNotificationServer()->externalCallbacks.closingConnection(this);
            return;
        }
        this->setConnectionState(NS_CONNECTING);
        this->myNotificationServer()->externalCallbacks.registerSocket(this->sock, 0, 1);
        if (this->connected)
            this->socketConnectionCompleted();
        
        std::ostringstream buf_;
        buf_ << "VER " << this->trID << " MSNP9\r\n";
        if (this->write(buf_) != buf_.str().size())
            return;
        
        this->addCallback(&NotificationServerConnection::callback_NegotiateCVR, this->trID++, (void *)info);
    }
    
    void NotificationServerConnection::connect(const std::string & hostname, unsigned int port, const Passport & username, const std::string & password)
    {
        this->auth.username = username;
        this->auth.password = password;
        this->connect(hostname, port);
    }
    
    void NotificationServerConnection::disconnect()
    {
        this->assertConnectionStateIsNot(NS_DISCONNECTED);
        
        std::list<SwitchboardServerConnection *> list = _switchboardConnections;
        std::list<SwitchboardServerConnection *>::iterator i = list.begin();
        for (; i != list.end(); i++)
        {
            delete *i;
        }
        
        this->callbacks.clear();
        
        this->setConnectionState(NS_DISCONNECTED);        
        this->myNotificationServer()->externalCallbacks.closingConnection(this);        
        Connection::disconnect();
    }
    
    void NotificationServerConnection::disconnectForTransfer()
    {
        this->assertConnectionStateIsNot(NS_DISCONNECTED);
        this->myNotificationServer()->externalCallbacks.unregisterSocket(this->sock);
        ::close(this->sock);
        this->setConnectionState(NS_DISCONNECTED);
    }
    
    void NotificationServerConnection::handleIncomingData()
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        while (this->isWholeLineAvailable())
        {
            std::vector<std::string> args = this->getLine();
            if ((args.size() >= 4 && args[0] == "MSG") ||
                (args.size() >= 2 && (args[0] == "NOT" || args[0] == "IPG")))
            {
                int dataLength;
                if (args[0] == "MSG")
                    dataLength = decimalFromString(args[3]);
                else
                    dataLength = decimalFromString(args[1]);

                if (this->readBuffer.find("\r\n", 0) + 2 + dataLength > this->readBuffer.size()) {
                    this->myNotificationServer()->externalCallbacks.showError(this, "Could not read buffer: too small");
                    return;
                }
            }
            this->readBuffer = this->readBuffer.substr(this->readBuffer.find("\r\n", 0) + 2);
            int trid = 0;
            
            if (args.size() >= 6 && args[0] == "XFR" && args[2] == "NS")
            {
                // XFR TrID NS NotificationServerIP:Port 0 ThisServerIP:Port
                //  0    1  2             3              4        5    
                this->callbacks.clear(); // delete the callback data
                
                this->disconnectForTransfer();
                
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
            
            if (args.size() >= 2 && args[0] == "QNG")
            {
                // QNG
                //  0
                
                this->_nextPing = decimalFromString(args[1]);
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
            
            if (!this->callbacks.empty() && trid >= 0)
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
        this->assertConnectionStateIs(NS_SYNCHRONISING);
        ListSyncInfo *info = static_cast<ListSyncInfo *>(data);
        
        if (args[0] == "SYN")
        {
            if (info->listVersion == decimalFromString(args[2]))
            {
                delete info;
                info = NULL;
                this->removeCallback(trid);
                this->myNotificationServer()->externalCallbacks.gotBuddyListInfo(this, NULL);
                this->setConnectionState(NS_CONNECTED);                
                return;
            }
            else 
            {
                info->listVersion = decimalFromString(args[2]);
                info->usersRemaining = decimalFromString(args[3]);
                info->groupsRemaining = decimalFromString(args[4]);
                this->myNotificationServer()->externalCallbacks.gotLatestListSerial(this, info->listVersion);
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
            this->myNotificationServer()->externalCallbacks.gotGTC(this, info->reverseListPrompting);
        }
        else if (args[0] == "BLP")
        {
            info->privacySetting = args[1][0];
            info->progress |= ListSyncInfo::COMPLETE_BLP;
            this->myNotificationServer()->externalCallbacks.gotBLP(this, info->privacySetting);
        }
        else if (args[0] == "LSG")
        {
            int groupID = decimalFromString(args[1]);
            Group g(groupID, decodeURL(args[2]));
            info->groups[groupID] = g;
			info->groupsRemaining--;
            if (info->groupsRemaining == 0 && info->usersRemaining == 0)
            {
                info->progress |= ListSyncInfo::LST_FL | ListSyncInfo::LST_RL | ListSyncInfo::LST_AL | ListSyncInfo::LST_BL;
            }
        }
        else if (args[0] == "BPR")
        {
            bool enabled;
            if (decodeURL(args[2])[0] == 'Y')
                enabled = true;
            else
                enabled = false;
            info->forwardList.back().phoneNumbers.push_back(Buddy::PhoneNumber(args[1], 
                                                                               decodeURL(args[2]), 
                                                                               enabled));
        }
        else
            throw std::runtime_error("Unexpected sync data");
                
        if (info->progress == (ListSyncInfo::LST_FL | ListSyncInfo::LST_RL |
                               ListSyncInfo::LST_AL | ListSyncInfo::LST_BL | 
                               ListSyncInfo::COMPLETE_BLP | ListSyncInfo::COMPLETE_GTC))
        {
            this->removeCallback(trid);
            this->checkReverseList(info);
            this->myNotificationServer()->externalCallbacks.gotBuddyListInfo(this, info);
            this->synctrid = 0;
            delete info;
            this->setConnectionState(NS_CONNECTED);
        }
        else if (info->progress > 63 || info->progress < 0)
            throw std::runtime_error("Corrupt sync progress!");
    }
    
    
    void NotificationServerConnection::callback_NegotiateCVR(std::vector<std::string> & args, int trid, void *data)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        connectinfo * info = (connectinfo *) data;
        this->removeCallback(trid);
        
        if (args.size() >= 3 && args[0] != "VER" || args[2] != "MSNP9") // if either *differs*...
        {
            this->myNotificationServer()->externalCallbacks.showError(NULL, "Protocol negotiation failed");
            delete info;
            this->disconnect();
            return;
        }
        
        std::ostringstream buf_;
        buf_ << "CVR " << this->trID << " 0x0409 winnt 5.2 i386 MSNMSGR 7.5.0324 MSMSGS " << info->username << "\r\n";
        if (this->write(buf_) != buf_.str().size())
            return;
        this->addCallback(&NotificationServerConnection::callback_RequestUSR, this->trID++, (void *) data);
    }
    
    void NotificationServerConnection::callback_TransferToSwitchboard(std::vector<std::string> & args, int trid, void *data)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        SwitchboardServerConnection::AuthData *auth = static_cast<SwitchboardServerConnection::AuthData *>(data);
        this->removeCallback(trid);
        
        if (args[0] != "XFR")
        {
            this->showError(decimalFromString(args[0]));
            this->disconnect();
            delete auth;
            return;
        }
        
        auth->cookie = args[5];
        auth->sessionID = "";
        
        SwitchboardServerConnection *newconn = new SwitchboardServerConnection(*auth, *this);
        
        this->addSwitchboardConnection(newconn);
        std::pair<std::string, int> server_address = splitServerAddress(args[3]);
        newconn->connect(server_address.first, server_address.second);
        
        delete auth;
    }
    
    void NotificationServerConnection::callback_RequestUSR(std::vector<std::string> & args, int trid, void *data)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        connectinfo *info = (connectinfo *)data;
        this->removeCallback(trid);
        
        if (args.size() > 1 && args[0] != "CVR") // if*differs*...
        {
            this->myNotificationServer()->externalCallbacks.showError(NULL, "Protocol negotiation failed");
            delete info;
            this->disconnect();
            return;
        }
        
        std::ostringstream buf_;
        buf_ << "USR " << this->trID << " TWN I " << info->username << "\r\n";
        if (this->write(buf_) != buf_.str().size())
            return;
        
        this->addCallback(&NotificationServerConnection::callback_PassportAuthentication, this->trID++, (void *) data);
    }    
    
    void NotificationServerConnection::callback_PassportAuthentication(std::vector<std::string> & args, int trid, void * data)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
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
            delete info;
            this->disconnect();
            return;
        }
        
        if (args.size() >= 4 && args[4].empty()) {
            this->disconnect();
            delete info;
            return;
        }
        
        /* Now to fire off some HTTPS login */
        /* args[4] contains the most interesting part we need to send on */
        curl = curl_easy_init();
        if (curl == NULL) {
            this->disconnect();
            delete info;
            return;
        }
        
        proxy = this->myNotificationServer()->externalCallbacks.getSecureHTTPProxy();
        if (! proxy.empty()) 
            ret = curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
        else	
            ret = CURLE_OK;
        
        if (ret == CURLE_OK)
            ret = curl_easy_setopt(curl, CURLOPT_URL, "https://login.passport.com/login2.srf");
        
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        uname = curl_escape(const_cast<char *>(info->username.c_str()), 0);
        pword = curl_escape(const_cast<char *>(info->password.c_str()), 0);
        auth = std::string("Authorization: Passport1.4 OrgVerb=GET,OrgURL=http%3A%2F%2Fmessenger%2Emsn%2Ecom,sign-in=") + uname + ",pwd=" + pword + ","+ args[4];
        curl_free(uname);
        curl_free(pword);
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
        
        if (ret == CURLE_OK)
            ret = curl_easy_perform(curl);
        
        curl_easy_cleanup(curl);
        curl_slist_free_all(slist);
        
        /* ok, if auth succeeded info->cookie is now the cookie to pass back */
        if (info->cookie.empty())
        {
            // No authentication cookie /usually/ means that authentication failed.
            this->showError(911);
            
            this->disconnect();
            delete info;
            return;
        }
        
        std::ostringstream buf_;
        buf_ << "USR " << this->trID << " TWN S " << info->cookie << "\r\n";
        if (this->write(buf_) != buf_.str().size())
            return;
        this->addCallback(&NotificationServerConnection::callback_AuthenticationComplete, this->trID++, (void *) data);
    }
    
    void NotificationServerConnection::callback_AuthenticationComplete(std::vector<std::string> & args, int trid, void * data)
    {
        this->assertConnectionStateIsAtLeast(NS_CONNECTED);
        connectinfo * info = (connectinfo *) data;
        this->removeCallback(trid);
        
        if (isdigit(args[0][0]))
        {
            this->showError(decimalFromString(args[0]));
            delete info;
            this->disconnect();
            return;
        }
        
        this->myNotificationServer()->externalCallbacks.gotFriendlyName(this, decodeURL(args[4]));
        
        delete info;
        
        this->myNotificationServer()->externalCallbacks.gotNewConnection(this);
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

        if (! cookiedata.empty()) 
        {
            size_t pos = cookiedata.find(",from-PP='");
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
