/*
 * connection.cpp
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

#include <msn/connection.h>
#include <msn/errorcodes.h>
#include <msn/util.h>
#include <msn/passport.h>
#include <msn/externals.h>
#include <msn/notificationserver.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#else
#include <winsock.h>
#include <io.h>
#endif

#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <cerrno>
#include <time.h>
#include <cassert>

namespace MSN
{
    static std::vector<std::string> errors;

    Connection::Connection() 
        : sock(0), connected(false), trID(1), user_data(NULL), messageHandlers()
    {
        srand((unsigned int) time(NULL));

        if (errors.size() != 0)
        {
            assert(errors.size() == 1000);
        }
        else
        {
            errors.resize(1000);
            for (int a = 0; a < 1000; a++)
            {
                errors[a] = "Unknown error code";
            }
            
            errors[200] = "Syntax error";
            errors[201] = "Invalid parameter";
            errors[205] = "Invalid user";
            errors[206] = "Domain name missing from username";
            errors[207] = "Already logged in";
            errors[208] = "Invalid username";
            errors[209] = "Invalid friendly name";
            errors[210] = "List full";
            errors[215] = "This user is already on this list or in this session";
            errors[216] = "Not on list";
            errors[218] = "Already in this mode";
            errors[219] = "This user is already in the opposite list";
            errors[280] = "Switchboard server failed";
            errors[281] = "Transfer notification failed";
            errors[300] = "Required fields missing";
            errors[302] = "Not logged in";
            errors[500] = "Internal server error";
            errors[501] = "Database server error";
            errors[510] = "File operation failed at server";
            errors[520] = "Memory allocation failed on server";
            errors[600] = "The server is too busy";
            errors[601] = "The server is unavailable";
            errors[602] = "A Peer Notification Server is down";
            errors[603] = "Database connection failed";
            errors[604] = "Server going down for maintenance";
            errors[707] = "Server failed to create connection";
            errors[711] = "Blocking write failed on server";
            errors[712] = "Session overload on server";
            errors[713] = "You have been too active recently. Slow down!";
            errors[714] = "Too many sessions open";
            errors[715] = "Not expected";
            errors[717] = "Bad friend file on server";
            errors[911] = "Authentication failed. Check that you typed your username and password correctly.";
            errors[913] = "This action is not allowed while you are offline";
            errors[920] = "This server is not accepting new users";            
        }

        messageHandlers["text/plain"]                            = &Connection::message_plain;
        messageHandlers["text/x-msmsgsinitialemailnotification"] = &Connection::message_initial_email_notification;
        messageHandlers["text/x-msmsgsemailnotification"]        = &Connection::message_email_notification;
        messageHandlers["text/x-msmsgsinvite"]                   = &Connection::message_invitation;
        messageHandlers["text/x-msmsgscontrol"]                  = &Connection::message_typing_user;            
    }
    
    Connection::~Connection() {}
    
    void Connection::disconnect()
    {
        this->myNotificationServer()->externalCallbacks.unregisterSocket(this->sock);
        ::close(this->sock);
        this->sock = -1;
        this->writeBuffer.erase();
        this->readBuffer.erase();
        this->trID = 1;
    }
    
    std::vector<std::string> Connection::getLine()
    {
        assert(this->isWholeLineAvailable());
        std::string s = this->readBuffer.substr(0, this->readBuffer.find("\r\n"));
        this->myNotificationServer()->externalCallbacks.log(0, (s + "\n").c_str());
        return splitString(s, " ");
    }
    
    bool Connection::isWholeLineAvailable()
    {
        return this->readBuffer.find("\r\n") != std::string::npos;
    }
    
    void Connection::errorOnSocket(int errno_)
    {
        this->myNotificationServer()->externalCallbacks.showError(this, strerror(errno_));
        this->disconnect();
    }
    
    void Connection::socketConnectionCompleted()
    {
        this->connected = true;
        
        // We know that we are connected, so this will try writing to the network.
        size_t writtenLength = this->write(this->writeBuffer, 1);
        this->writeBuffer = this->writeBuffer.substr(writtenLength);
    }
        
    size_t Connection::write(std::string s, bool log) throw (std::runtime_error)
    {
        if (! this->connected)
            this->writeBuffer.append(s);
        else
        {
            if (log)
                this->myNotificationServer()->externalCallbacks.log(1, s.c_str());
            
            size_t written = 0;
            while (written < s.size())
            {
                size_t newWritten = ::send(this->sock, s.substr(written).c_str(), (int) (s.size() - written), 0);
                if (newWritten <= 0)
                {
                    if (errno == EAGAIN)
                        continue;
                    else
                        break;
                }
                written += newWritten;
            }
            if (written != s.size())
            {
                this->errorOnSocket(errno);
                return written;
            }
        }
        return s.size();
    }
    
    size_t Connection::write(std::ostringstream & ss, bool log) throw (std::runtime_error)
    {
        std::string s = ss.str();
        size_t result = write(s, log);
        return result;        
    }
        
    void Connection::dataArrivedOnSocket()
    {
        char tempReadBuffer[8192];
        int amountRead = ::recv(this->sock, &tempReadBuffer, 8192, 0);
        if (amountRead < 0)
        {
            if (errno == EAGAIN)
                return;
            
            // We shouldn't get EAGAIN here because dataArrivedOnSocket
            // is only called when select/poll etc has told us that
            // the socket is readable.
            // assert(errno != EAGAIN);
            
            this->myNotificationServer()->externalCallbacks.showError(this, strerror(errno));                
            this->disconnect();            
        }
        else if (amountRead == 0)
        {
            this->myNotificationServer()->externalCallbacks.showError(this, "Connection closed by remote endpoint.");
            this->disconnect();
        }
        else
        {
            this->readBuffer += std::string(tempReadBuffer, amountRead);
            try
            {
                handleIncomingData();
            }
            catch (std::exception & e)
            {
                this->myNotificationServer()->externalCallbacks.showError(this, e.what());
            }
        }
    }
    
    void Connection::handle_MSG(std::vector<std::string> & args)
    {
        int msglen;
        std::string msg;
        std::string mime;
        std::string body;
        size_t tmp;
        
        msglen = decimalFromString(args[3]);
        msg = this->readBuffer.substr(0, msglen);
        this->readBuffer = this->readBuffer.substr(msglen);
        
        body = msg.substr(msg.find("\r\n\r\n") + 4);
        mime = msg.substr(0, msg.size() - body.size());  
        
        std::string contentType;
        Message::Headers headers = Message::Headers(mime);
        contentType = headers["Content-Type"];
        
        if ((tmp = contentType.find("; charset")) != std::string::npos)
            contentType = contentType.substr(0, tmp);
        
        std::map<std::string, void (Connection::*)(std::vector<std::string> &, std::string, std::string)>::iterator i = messageHandlers.find(contentType);
        if (i != messageHandlers.end())
            (this->*(messageHandlers[contentType]))(args, mime, body);
    }
    
    void Connection::message_plain(std::vector<std::string> & args, std::string mime, std::string body)
    {
        Message msg = Message(body, mime);
        
        this->myNotificationServer()->externalCallbacks.gotInstantMessage(static_cast<SwitchboardServerConnection *>(this),
                               args[1], decodeURL(args[2]), &msg);
    }
    
    void Connection::message_initial_email_notification(std::vector<std::string> & args, std::string mime, std::string body)
    {
        std::string unreadInbox;
        std::string unreadFolder;
        int unreadInboxCount = 0, unreadFolderCount = 0;
        
        // Initial email notifications body is a set of MIME headers        
        Message::Headers headers = Message::Headers(body);
        
        unreadInbox = headers["Inbox-Unread"];
        unreadFolder = headers["Folders-Unread"];
        if (! unreadInbox.empty())
            unreadInboxCount = decimalFromString(unreadInbox);
        
        if (! unreadFolder.empty())
            unreadFolderCount = decimalFromString(unreadFolder);
        
        this->myNotificationServer()->externalCallbacks.gotInitialEmailNotification(this, unreadInboxCount, unreadFolderCount);
    }

    
    void Connection::message_email_notification(std::vector<std::string> & args, std::string mime, std::string body)
    {
        // New email notifications body is a set of MIME headers
        Message::Headers headers = Message::Headers(body);
        
        std::string from = headers["From-Addr"];
        std::string subject = headers["Subject"];
        
        this->myNotificationServer()->externalCallbacks.gotNewEmailNotification(this, from, subject);
    }

    
    void Connection::message_invitation(std::vector<std::string> & args, std::string mime, std::string body)
    {
        static_cast<SwitchboardServerConnection *>(this)->handleInvite(args[1], decodeURL(args[2]), mime, body);
    }

    void Connection::message_typing_user(std::vector<std::string> & args, std::string mime, std::string body)
    {
        this->myNotificationServer()->externalCallbacks.buddyTyping(this, args[1], decodeURL(args[2]));        
    }   
    
    void Connection::showError(int errorCode)
    {
        std::ostringstream buf_;
        buf_ << "Error code: " << errorCode << " (" << errors[errorCode] << ")";
        this->myNotificationServer()->externalCallbacks.showError(this, buf_.str());        
    }
}
