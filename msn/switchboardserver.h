#ifndef __msn_switchboardserver_h__
#define __msn_switchboardserver_h__

/*
 * switchboardserver.h
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

#include <msn/message.h>
#include <msn/authdata.h>
#include <msn/connection.h>
#include <msn/passport.h>
#include <string>

namespace MSN
{
    class NotificationServerConnection;
    class FileTransferConnection;
    class FileTransferInvitation;
    class Invitation;
    
    /** Represents a connection to a MSN switchboard.
     */
    class SwitchboardServerConnection : public Connection
    {
private:
        typedef void (SwitchboardServerConnection::*SwitchboardServerCallback)(std::vector<std::string> & args, int trid, void *);
public:
        class AuthData : public ::MSN::AuthData
        {
public:
            std::string sessionID;
            std::string cookie;
            const void *tag;
            
            AuthData(Passport & username_, const std::string & sessionID_, 
                     const std::string & cookie_, const void *tag_=NULL) : 
                ::MSN::AuthData(username_), sessionID(sessionID_), cookie(cookie_), tag(tag_) {};
            
            AuthData(Passport & username_, const void *tag_=NULL) :
                ::MSN::AuthData(username_), sessionID(""), cookie(""), tag(tag_) {};        
        };
        
        SwitchboardServerConnection::AuthData auth;
        
        /** A list of the users in this switchboard session.
         */
        std::list<Passport> users;

        /** Invitations extended but not responded to.
        */
        std::list<Invitation *> invitationsSent;         
        
        /** Invitations received but not responded to.
        */
        std::list<Invitation *> invitationsReceived;
        
        SwitchboardServerConnection(AuthData & auth_, NotificationServerConnection &);
        virtual ~SwitchboardServerConnection();
        virtual void dispatchCommand(std::vector<std::string> & args);
        
        /** Return a connection that is associated with @a fd.
         *
         *  If @a fd is equal to @p sock, @c this is returned.  Otherwise
         *  connectionWithSocket is sent to each FileTransferConnection
         *  until a match is found.
         *  
         *  @return  The matching connection, if found.  Otherwise, @c NULL.
         */        
        Connection *connectionWithSocket(int fd);
        
        /** Return a list of all FileTransferConnection's associated with this
         *  connection.
         */
        std::list<FileTransferConnection *> & fileTransferConnections() const;
        
        /** Add a FileTransferConnection to the list of associated connections.
         */
        void addFileTransferConnection(FileTransferConnection *);
        
        /** Remove a FileTransferConnection from the list of associated connections.
         */
        void removeFileTransferConnection(FileTransferConnection *);
        
        /** Remove the FileTransferConnection that is associated with FileTransferInvitation
         *  @a inv from the list of associated connections.
         */
        void removeFileTransferConnection(FileTransferInvitation *inv);
        
        /** Send a typing notification to the switchboard server.
         */
        void sendTypingNotification();
        
        /** Invite @a userName into this conversation.
         */
        void inviteUser(Passport userName);

        virtual void connect(const std::string & hostname, unsigned int port);
        virtual void disconnect();
        virtual void sendMessage(const Message *msg);
        virtual void sendMessage(const std::string & s);
        
        FileTransferInvitation *sendFile(const std::string path);

        /** Add @a cb as a callback that will be called when a response is received
         *  a transaction ID of @a trid.
         */
        virtual void addCallback(SwitchboardServerCallback, int trid, void *data);
        
        /** Remove callbacks for transaction ID @a trid.
         */
        virtual void removeCallback(int trid);
        
        Invitation *invitationWithCookie(const std::string & cookie);
        
        virtual void socketConnectionCompleted();
        
        enum SwitchboardServerState
        {
            SB_DISCONNECTED,
            SB_CONNECTING,
            SB_CONNECTED,            
            SB_WAITING_FOR_USERS,
            SB_READY
        };
        
        SwitchboardServerState connectionState() const { return this->connectionStatus; };
protected:
        virtual void handleIncomingData();
        SwitchboardServerState connectionStatus;
private:
        NotificationServerConnection & notificationServer;
        std::list<FileTransferConnection *> _fileTransferConnections;
        std::map<int, std::pair<SwitchboardServerCallback, void *> > callbacks;        
        
        static std::map<std::string, void (SwitchboardServerConnection::*)(std::vector<std::string> &)> commandHandlers;
        void registerCommandHandlers(); 
        void handle_BYE(std::vector<std::string> & args);
        void handle_JOI(std::vector<std::string> & args);
        void handle_NAK(std::vector<std::string> & args);
        void handle_MSG(std::vector<std::string> & args);
        
        void callback_InviteUsers(std::vector<std::string> & args, int trid, void * data);
        void callback_AnsweredCall(std::vector<std::string> & args, int trid, void * data);
        
        void handleInvite(Passport from, const std::string & friendly, const std::string & mime, const std::string & body);
        void handleNewInvite(Passport & from, const std::string & friendly, const std::string & mime, const std::string & body);
        friend class Connection;
    };
}
#endif
