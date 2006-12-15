/*
 * switchboardserver.cpp
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

#include <msn/switchboardserver.h>
#include <msn/notificationserver.h>
#include <msn/errorcodes.h>
#include <msn/externals.h>
#include <msn/filetransfer.h>

#include <sys/stat.h>
#include <algorithm>
#include <cassert>

namespace MSN
{
    std::map<std::string, void (SwitchboardServerConnection::*)(std::vector<std::string> &)> SwitchboardServerConnection::commandHandlers;
    
    SwitchboardServerConnection::SwitchboardServerConnection(AuthData & auth_, NotificationServerConnection & n)
        : Connection(), auth(auth_), connectionStatus(SB_DISCONNECTED), notificationServer(n)
    {
        registerCommandHandlers();
    }
    
    SwitchboardServerConnection::~SwitchboardServerConnection()
    {
        if (connectionStatus != SB_DISCONNECTED)
            this->disconnect();
    }
        
    Connection *SwitchboardServerConnection::connectionWithSocket(int fd)
    {
        assert(connectionStatus >= SB_CONNECTING);
        std::list<FileTransferConnection *> & list = _fileTransferConnections;
        std::list<FileTransferConnection *>::iterator i = list.begin();
        
        if (this->sock == fd)
            return this;
        
        for (; i != list.end(); i++)
        {
            if ((*i)->sock == fd)
                return *i;
        }
        return NULL;
    }
    
    void SwitchboardServerConnection::addFileTransferConnection(FileTransferConnection *c)
    {
        assert(connectionStatus >= SB_CONNECTED);        
        _fileTransferConnections.push_back(c);
    }

    void SwitchboardServerConnection::removeFileTransferConnection(FileTransferConnection *c)
    {
        assert(connectionStatus >= SB_CONNECTED);        
        _fileTransferConnections.remove(c);
    }

    /**
     *  @todo  FIX THIS.  Delete shouldn't be here.
     */    
    void SwitchboardServerConnection::removeFileTransferConnection(FileTransferInvitation *inv)
    {
        assert(connectionStatus >= SB_CONNECTED);        
        std::list<FileTransferConnection *> & list = _fileTransferConnections;
        std::list<FileTransferConnection *>::iterator i = list.begin();        
        for (i = list.begin(); i != list.end(); i++)
        {
            if ((*i)->auth.inv == inv)
                break;
        }
        if (i != list.end())
            (*i)->disconnect();
    }
    
    template <class _Tp>
    class hasCookieOf
    {
        const std::string & cookie;
public:
        hasCookieOf(const std::string & __i) : cookie(__i) {};
        bool operator()(const _Tp &__x) { return __x->cookie == cookie; }
    };
    
    Invitation *SwitchboardServerConnection::invitationWithCookie(const std::string & cookie)
    {
        std::list<Invitation *> & receivedList = this->invitationsReceived;
        std::list<Invitation *> & sentList = this->invitationsSent;
        std::list<Invitation *>::iterator i;
        
        i = std::find_if(receivedList.begin(), receivedList.end(), hasCookieOf<Invitation *>(cookie));
        if (i != receivedList.end())
            return *i;
        
        i = std::find_if(sentList.begin(), sentList.end(), hasCookieOf<Invitation *>(cookie));
        if (i != sentList.end())
            return *i;
        return NULL;
    }
    
    void SwitchboardServerConnection::addCallback(SwitchboardServerCallback callback,
                                                   int trid, void *data)
    {
        assert(connectionStatus >= SB_CONNECTING);        
        this->callbacks[trid] = std::make_pair(callback, data);
    }

    void SwitchboardServerConnection::removeCallback(int trid)
    {
        assert(connectionStatus >= SB_CONNECTING);        
        this->callbacks.erase(trid);
    }
    
    void SwitchboardServerConnection::registerCommandHandlers()
    {
        if (commandHandlers.size() == 0)
        {
            commandHandlers["BYE"] = &SwitchboardServerConnection::handle_BYE;
            commandHandlers["JOI"] = &SwitchboardServerConnection::handle_JOI;
            commandHandlers["NAK"] = &SwitchboardServerConnection::handle_NAK;
            commandHandlers["MSG"] = &SwitchboardServerConnection::handle_MSG;
        }
    }
    
    void SwitchboardServerConnection::dispatchCommand(std::vector<std::string> & args)
    {
        assert(connectionStatus >= SB_CONNECTED);        
        std::map<std::string, void (SwitchboardServerConnection::*)(std::vector<std::string> &)>::iterator i = commandHandlers.find(args[0]);
        if (i != commandHandlers.end())
            (this->*commandHandlers[args[0]])(args);
    }
    
    void SwitchboardServerConnection::sendMessage(const Message *msg)
    {
        assert(connectionStatus >= SB_READY);        
        std::string s = msg->asString();
        
        std::ostringstream buf_;
        buf_ << "MSG " << trid++ << " N " << (int) s.size() << "\r\n" << s;
        this->write(buf_);
    }

    void SwitchboardServerConnection::sendMessage(const std::string & body)
    { 
        Message msg(body, "MIME-Version: 1.0\r\nContent-Type: text/plain; charset=UTF-8\r\n\r\n");
        this->sendMessage(&msg);
    }    
    
    void SwitchboardServerConnection::handleInvite(Passport from, const std::string & friendly, const std::string & mime, const std::string & body)
    {
        assert(connectionStatus >= SB_READY);        
        Message::Headers headers = Message::Headers(body);
        std::string command = headers["Invitation-Command"];
        std::string cookie = headers["Invitation-Cookie"];
        std::string guid = headers["Application-GUID"];
        
        Invitation * inv = this->invitationWithCookie(cookie);
        
        if (command == "INVITE" && guid == "{5D3E02AB-6190-11d3-BBBB-00C04F795683}")
        {
            this->handleNewInvite(from, friendly, mime, body);
        } 
        else if (inv == NULL)
        {
//            printf("Very odd - just got a %s out of mid-air...\n", command.c_str()); 
        }
        else if (command == "ACCEPT")
        {
            inv->invitationWasAccepted(body);
        } 
        else if (command == "CANCEL" || command == "REJECT")
        {
            inv->invitationWasCanceled(body);
        } else {
//            printf("Argh, don't support %s yet!\n", command.c_str());
        }
    }
    
    void SwitchboardServerConnection::handleNewInvite(Passport & from, const std::string & friendly, const std::string & mime, const std::string & body)
    {
        assert(connectionStatus >= SB_READY);        
        Message::Headers headers = Message::Headers(body);
        std::string appname;
        std::string filename;
        std::string filesize;
        
        appname = headers["Application-Name"];
        Invitation *invg = NULL;
        
        if (!((filename = headers["Application-File"]).empty() ||
              (filesize = headers["Application-FileSize"]).empty()))
        {
            invg = new FileTransferInvitation(Invitation::MSNFTP, headers["Invitation-Cookie"], from, 
                                              this, filename, atol(filesize.c_str()));
            ext::gotFileTransferInvitation(this, from, friendly, static_cast<FileTransferInvitation *>(invg));
        }
        
        if (invg == NULL)
        {
            ext::showError(this, "Unknown invitation type!");
            return;
        }
        
        this->invitationsReceived.push_back(invg);
    }
    
    void SwitchboardServerConnection::handle_BYE(std::vector<std::string> & args)
    {
        assert(connectionStatus >= SB_CONNECTED);        
        std::list<Passport> & list = this->users;
        std::list<Passport>::iterator i;
        
        ext::buddyLeftConversation(this, args[1]);
        
        for (i = list.begin(); i != list.end(); i++)
        {
            if (*i == args[1])
            {
                list.remove(*i);
                break;
            }
        }    
        
        if (this->users.empty() || (args.size() > 3 && args[3] == "1"))
        {
            this->disconnect();
        }
    }
    
    
    void SwitchboardServerConnection::handle_JOI(std::vector<std::string> & args)
    {
        assert(connectionStatus >= SB_CONNECTED);        
        if (args[1] == this->auth.username)
            return;
        
        if (this->auth.sessionID.empty() && connectionStatus == SB_WAITING_FOR_USERS)
            connectionStatus = SB_READY;
        
        this->users.push_back(args[1]);
        ext::buddyJoinedConversation(this, args[1], decodeURL(args[2]), 0);
    }
    
    void SwitchboardServerConnection::handle_NAK(std::vector<std::string> & args)
    {
        assert(connectionStatus >= SB_CONNECTED);        
        ext::failedSendingMessage(this);
    }
    
    void SwitchboardServerConnection::handle_MSG(std::vector<std::string> & args)
    {
        assert(connectionStatus >= SB_CONNECTED);        
        Connection::handle_MSG(args);
    }
    
    void SwitchboardServerConnection::sendTypingNotification()
    {
        assert(connectionStatus >= SB_READY);        
        std::ostringstream buf_, msg_;
        msg_ << "MIME-Version: 1.0\r\n";
        msg_ << "Content-Type: text/x-msmsgscontrol\r\n";
        msg_ << "TypingUser: " << this->auth.username << "\r\n";
        msg_ << "\r\n";
        msg_ << "\r\n";
        size_t msg_length = msg_.str().size();
        
        buf_ << "MSG " << trid++ << " U " << (int) msg_length << "\r\n" << msg_.str();
        
        write(buf_);        
    }
    
    void SwitchboardServerConnection::inviteUser(Passport userName)
    {
        assert(connectionStatus >= SB_WAITING_FOR_USERS);        
        std::ostringstream buf_;
        buf_ << "CAL " << trid++ << " " << userName << "\r\n";
        write(buf_);        
    }
    
    void SwitchboardServerConnection::connect(const std::string & hostname, unsigned int port)
    {
        assert(connectionStatus == SB_DISCONNECTED);

        if ((this->sock = ext::connectToServer(hostname, port, &this->connected)) == -1)
        {
            ext::showError(this, "Could not connect to switchboard server");
            return;
        }
        
        ext::registerSocket(this->sock, 1, 1);
        connectionStatus = SB_CONNECTING;
        std::ostringstream buf_;
        if (this->auth.sessionID.empty())
        {
            buf_ << "USR " << trid << " " << this->auth.username << " " << this->auth.cookie << "\r\n";
            this->write(buf_);
            this->addCallback(&SwitchboardServerConnection::callback_InviteUsers, trid, NULL);
        } 
        else
        {
            buf_ << "ANS " << trid << " " << this->auth.username << " " << this->auth.cookie << " " << this->auth.sessionID << "\r\n";
            this->write(buf_);
            ext::gotNewConnection(this);
            this->addCallback(&SwitchboardServerConnection::callback_AnsweredCall, trid, NULL);
        }
        
        trid++;    
    }
    
    void SwitchboardServerConnection::disconnect()
    {
        assert(connectionStatus != SB_DISCONNECTED);
        notificationServer.removeSwitchboardConnection(this);
        ext::closingConnection(this);
        
        std::list<FileTransferConnection *> list = _fileTransferConnections;
        std::list<FileTransferConnection *>::iterator i = list.begin();
        for (; i != list.end(); i++)
        {
            removeFileTransferConnection(*i);
        }
        Connection::disconnect();
        connectionStatus = SB_DISCONNECTED;
    }    
    
    void SwitchboardServerConnection::socketConnectionCompleted()
    {
        assert(connectionStatus == SB_CONNECTING);
        Connection::socketConnectionCompleted();
        ext::unregisterSocket(this->sock);
        ext::registerSocket(this->sock, 1, 0);
        connectionStatus = SB_WAITING_FOR_USERS;
    }
    
    void SwitchboardServerConnection::handleIncomingData()
    {
        assert(connectionStatus >= SB_CONNECTED);        
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
            
            if (args.size() > 1)
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
    
    
    void SwitchboardServerConnection::callback_InviteUsers(std::vector<std::string> & args, int trid, void *)
    {    
        assert(connectionStatus >= SB_CONNECTED);
        this->removeCallback(trid);
        
        if (args[2] != "OK")
        {
            this->showError(decimalFromString(args[0]));
            this->disconnect();
            return;
        }
        
        ext::gotSwitchboard(this, this->auth.tag);
        ext::gotNewConnection(this);
    }
    
    void SwitchboardServerConnection::callback_AnsweredCall(std::vector<std::string> & args, int trid, void * data)
    {
        assert(connectionStatus == SB_WAITING_FOR_USERS);        
        if (args.size() >= 3 && args[0] == "ANS" && args[2] == "OK")
            return;
        
        if (isdigit(args[0][0]))
        {
            this->removeCallback(trid);
            this->showError(decimalFromString(args[0]));
            this->disconnect();
            return;
        }
        
        if (args.size() >= 6 && args[0] == "IRO")
        {
            if (args[4] == this->auth.username)
                return;
            
            this->users.push_back(args[4]);
            ext::buddyJoinedConversation(this, args[4], decodeURL(args[5]), 1);
            if (args[2] == args[3])
            {
                this->removeCallback(trid);
                connectionStatus = SB_READY;
            }
        }
    }
    
    FileTransferInvitation *SwitchboardServerConnection::sendFile(const std::string path)
    {
        assert(connectionStatus == SB_READY);        
        struct stat st_info;
        
        if (stat(path.c_str(), &st_info) < 0)
        { 
            ext::showError(this, "Could not open file"); 
            return NULL; 
        }
        
        char tmp[64];
        sprintf(tmp, "%d", rand());
        FileTransferInvitation *inv = new FileTransferInvitation(Invitation::MSNFTP, std::string(tmp), *(users.begin()), 
                                                                 this, path, st_info.st_size);
        
        std::string basename = inv->fileName;
        
        size_t forward_slash_pos = inv->fileName.rfind('/');
        size_t backward_slash_pos = inv->fileName.rfind('\\');
        size_t basename_pos = 0;
        if (forward_slash_pos != std::string::npos)
            basename_pos = forward_slash_pos + 1;
        if (backward_slash_pos != std::string::npos && backward_slash_pos > forward_slash_pos)
            basename_pos = backward_slash_pos + 1;
        
        basename = basename.substr(basename_pos);
        
        std::ostringstream buf_;
        buf_ << "Application-Name: File Transfer\r\n";
        buf_ << "Application-GUID: {5D3E02AB-6190-11d3-BBBB-00C04F795683}\r\n";
        buf_ << "Invitation-Command: INVITE\r\n";
        buf_ << "Invitation-Cookie: " << inv->cookie << "\r\n";
        buf_ << "Application-File: " << basename << "\r\n";
        buf_ << "Application-FileSize: " << inv->fileSize << "\r\n";
        buf_ << "\r\n";
        
        Message *msg = new Message(buf_.str());
        msg->setHeader("Content-Type", "text/x-msmsgsinvite; charset=UTF-8");
        
        this->sendMessage(msg);
        
        this->invitationsSent.push_back(inv);
        
        delete msg;
        
        ext::fileTransferProgress(inv, "Negotiating connection", 0, 0);
        
        return inv;
    }
}
