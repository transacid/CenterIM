#ifndef __msn_notificationserver_h__
#define __msn_notificationserver_h__

/*
 * notificationserver.h
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
#include <msn/authdata.h>
#include <msn/errorcodes.h>
#include <msn/buddy.h>
#include <msn/passport.h>
#include <stdexcept>

namespace MSN
{    
    class SwitchboardServerConnection;
    
    /** Contains information about synchronising the contact list with the server.
     */
    class ListSyncInfo
    {
public:
        /** Constants specifying the status of the list synchronisation.
         */
        enum SyncProgress
        {
            LST_FL = 1,        /**< Forward list has been received. */
            LST_AL = 2,        /**< Allow list has been received.   */
            LST_BL = 4,        /**< Block list has been received.   */
            LST_RL = 8,        /**< Reverse list has been received. */
            COMPLETE_BLP = 16, /**< @c BLP has been received.       */
            COMPLETE_GTC = 32  /**< @c GTC has been received.       */
        };
        
        
        /** Treat unknown users as if appearing on this list when they attempt
         *  to initiate a switchboard session.
         */
        enum PrivacySetting
        {
            ALLOW = 'A',
            BLOCK = 'B'
        };
        
        /** Action to take when a new user appears on our reverse list.
         */
        enum NewReverseListEntryAction
        {
            PROMPT = 'A',
            DONT_PROMPT = 'N'
        };

        /** A list of people who's statuses we are wish to be notified about.
         */
        std::list<Buddy> forwardList;
        
        /** A list of people that are interested in our status.
         */
        std::list<Buddy> reverseList;
        
        /** A list of people we allow to see our status.
         */
        std::list<Buddy> allowList;
        
        /** A list of people we disallow from seeing our status.
         */
        std::list<Buddy> blockList;
        
        std::map<int, Group> groups;
        
        /** The progress of the sync operation.
         */
        unsigned int progress;
        
        unsigned int usersRemaining, groupsRemaining;
        
        /** The version of the contact list to request.
         */
        unsigned int listVersion;
        
        /** Specifies the default list for non-buddies to be treated as
         *  appearing in when attempting to invite you into a switchboard setting.
         *
         *  This corresponds to the @c BLP command in the MSN protocol.
         */
        char privacySetting;
        
        /** Specifies whether the user should be prompted when a new buddy
         *  appears on their reverse list.
         *
         *  This corresponds to the @c GTC command in the MSN protocol.
         */
        char reverseListPrompting;
        
        ListSyncInfo(int listVersion_) : 
            progress(0), listVersion(listVersion_), 
            privacySetting(ListSyncInfo::ALLOW), reverseListPrompting(ListSyncInfo::PROMPT) {};
    };
    
    // Intermediate steps in connection:
    class connectinfo // : public callback_data
    {
public:
        Passport username;
        std::string password;
        std::string cookie;
        
        connectinfo(const Passport & username_, const std::string & password_) : username(username_), password(password_), cookie("") {};
    };    
        
    /** Represents a connection to a MSN notification server.
     *
     *  The MSN notification server is responsible for dealing with the contact
     *  list, online status, and dispatching message requests or invitations to
     *  or from switchboard servers.
     */
    class NotificationServerConnection : public Connection
    {
private:
        typedef void (NotificationServerConnection::*NotificationServerCallback)(std::vector<std::string> & args, int trid, void *);
        
        class AuthData : public ::MSN::AuthData
        {
public:
            std::string password;
            
            AuthData(const Passport & passport_,
                     const std::string & password_) : 
                ::MSN::AuthData(passport_), password(password_) {} ;
        };
        
        NotificationServerConnection::AuthData auth;
        int synctrid;

public:
        /** Create a NotificationServerConnection with the specified authentication data
         */
        NotificationServerConnection(AuthData & auth_);
        
        /** Create a NotificationServerConnection with the specified @a username and 
         *  @a password.
         */
        NotificationServerConnection(Passport username, std::string password);
        
        /** Create a NotificationServerConnection with no specified username or password. */
        NotificationServerConnection();
        
        virtual ~NotificationServerConnection();
        virtual void dispatchCommand(std::vector<std::string> & args);
        
        /** Return a list of SwitchboardServerConnection's that have been started
         *  from this NotificationServerConnection.
         */
        const std::list<SwitchboardServerConnection *> & switchboardConnections();
        
        /** Add a SwitchboardServerConnection to the list of connections that have
         *  been started from this connection.
         */
        void addSwitchboardConnection(SwitchboardServerConnection *);
        
        /** Remove a SwitchboardServerConnection from the list of connections that have
         *  beep started from this connection.
         */
        void removeSwitchboardConnection(SwitchboardServerConnection *);
        
        /** Return a connection that is associated with @a fd.
         *
         *  If @a fd is equal to @p sock, @c this is returned.  Otherwise
         *  connectionWithSocket is sent to each SwitchboardServerConnection
         *  until a match is found.
         *  
         *  @return  The matching connection, if found.  Otherwise, @c NULL.
         */
        Connection *connectionWithSocket(int fd);
        
        /** Return a SwitchboardServerConnection that has exactly one user whose
         *  username is @a username.
         *
         *  @return  The matching SwitchboardServerConnection, or @c NULL.
         */
        SwitchboardServerConnection *switchboardWithOnlyUser(Passport username);
        
        /** @name Action Methods
         *
         *  These methods all perform actions on the notification server.
         */
        /** @{ */
        
        /** Set our online state to @a state.
         */
        void setState(BuddyStatus state);
        
        void setBLP(char setting);
        void setGTC(char setting);
        
        /** Set our friendly name to @a friendlyName.
         *
         *  @param  friendlyName  Maximum allowed length is 387 characters after URL-encoding.
         */
        void setFriendlyName(std::string friendlyName) throw (std::runtime_error);
        
        /** Add buddy named @a buddyName to the list named @a list.
         */
        void addToList(std::string list, Passport buddyName);
        
        /** Remove buddy named @a budydName from the list named @a list.
         */
        void removeFromList(std::string list, Passport buddyName);
        
        void addToGroup(Passport, int groupID);
        void removeFromGroup(Passport buddyName, int groupID);
        
        void addGroup(std::string groupName);
        void removeGroup(int groupId);
        void renameGroup(int groupId, std::string newGroupName);
        
        /** Request the server side buddy list.
         *
         *  @param  version  if @a version is specified the server will respond with
         *                   the changes necessary to update the list to the latest
         *                   version.  Otherwise the entire list will be sent.
         */
        void synchronizeLists(int version=0);
        
        /** Send a 'keep-alive' ping to the server.
         */
        void sendPing();
        
        /** Request a switchboard connection with @a username.  @a msg will be sent when
         *  @a username joins the switchboard.
         */
        //        void requestSwitchboardConnection(Passport username, Message *msg, void *tag);
        
        /** Request a switchboard connection.
         */
        void requestSwitchboardConnection(const void *tag);
        /** @} */
        
        void checkReverseList(ListSyncInfo *);

        virtual void connect(const std::string & hostname, unsigned int port);
        virtual void connect(const std::string & hostname, unsigned int port, const Passport & username,  const std::string & password);
        virtual void disconnect();
        
        /** Add a callback of @a cb to this connection for response with ID of @a trid.
         *  
         *  @todo  <code>void *</code> is bad.
         */
        virtual void addCallback(NotificationServerCallback cb, int trid, void *data);
        
        /** Remove callbacks assocated with responses with ID of @a trid.
         */
        virtual void removeCallback(int trid);
        
        virtual void socketConnectionCompleted();
        
        
        enum NotificationServerState
        {
            NS_DISCONNECTED,
            NS_CONNECTING,
            NS_CONNECTED,
            NS_SYNCHRONISING,
            NS_ONLINE
        };

        NotificationServerState connectionState() const { return this->connectionStatus; };
protected:
        virtual void handleIncomingData();
        NotificationServerState connectionStatus;

private:
        std::list<SwitchboardServerConnection *> _switchboardConnections;
        std::map<int, std::pair<NotificationServerCallback, void *> > callbacks;
        
        virtual void disconnectForTransfer();        
            
        static std::map<std::string, void (NotificationServerConnection::*)(std::vector<std::string> &)> commandHandlers;
        void registerCommandHandlers();
        void handle_OUT(std::vector<std::string> & args);
        void handle_ADD(std::vector<std::string> & args);
        void handle_REM(std::vector<std::string> & args);
        void handle_BLP(std::vector<std::string> & args);
        void handle_GTC(std::vector<std::string> & args);
        void handle_REA(std::vector<std::string> & args);
        void handle_CHG(std::vector<std::string> & args);
        void handle_CHL(std::vector<std::string> & args);
        void handle_ILN(std::vector<std::string> & args);
        void handle_NLN(std::vector<std::string> & args);
        void handle_FLN(std::vector<std::string> & args);
        void handle_MSG(std::vector<std::string> & args);
        void handle_RNG(std::vector<std::string> & args);
        void handle_ADG(std::vector<std::string> & args);
        void handle_RMG(std::vector<std::string> & args);
        void handle_REG(std::vector<std::string> & args);
        
        void callback_SyncData(std::vector<std::string> & args, int trid, void *data) throw (std::runtime_error);
        void callback_NegotiateCVR(std::vector<std::string> & args, int trid, void *data);
        void callback_TransferToSwitchboard(std::vector<std::string> & args, int trid, void *data);
        void callback_RequestUSR(std::vector<std::string> & args, int trid, void *data);
        void callback_PassportAuthentication(std::vector<std::string> & args, int trid, void * data);        
        void callback_AuthenticationComplete(std::vector<std::string> & args, int trid, void * data);
    };
    
}


#endif
