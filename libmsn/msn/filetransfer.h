#ifndef __msn_filetransfer_h__
#define __msn_filetransfer_h__

/*
 * filetransfer.h
 * libmsn
 *
 * Created by Mark Rowe on Wed Mar 17 2004.
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

#include <msn/util.h>
#include <msn/switchboardserver.h>
#include <msn/invitation.h>
#include <msn/passport.h>

#include <string>
#include <stdio.h>

namespace MSN 
{
    /** Contains information about the invitation that was sent or received when
     *  a file transfer was initiated.
     */
    class FileTransferInvitation : public Invitation
    {
public:
        /** The name of the file that is being transferred.
         */
        std::string fileName;
        
        /** The size in bytes of the file that is being transferred.
         */
        long unsigned fileSize;
        
        /** A pointer to user-specified data that can be used to identify this
         *  transfer.
         */
        void * userData;
        
        /** Create a new FileTransferInvitation.
         *
         *  @todo  Do all of these parameters really need to be in the constructor?
         */
        FileTransferInvitation(Invitation::ApplicationType application_, std::string cookie_,
                       Passport otherUser_, SwitchboardServerConnection * switchboardConnection_,
                       std::string fileName_, long unsigned fileSize_, void *userData_ = NULL) :
            Invitation(application_, cookie_, otherUser_, switchboardConnection_),
            fileName(fileName_), fileSize(fileSize_), userData(userData_) {};

        /** The remote side accepted our file transfer.  Notify calling code.
         */
        virtual void invitationWasAccepted(const std::string & body);
        
        /** The remote side canceled our file transfer.  Notify calling code.
         */
        virtual void invitationWasCanceled(const std::string & body);
        
        /** The user wishes to accept the file transfer, and have the file
         *  written to @a destinationFile.
         *
         *  @todo Should the received data be passed back to the user to store
         *        in the file?  This would remove some of the complexity in the
         *        file transfer code.
         */
        void acceptTransfer(const std::string & destinationFile);
        
        /** The user wishes to decline the file transfer.
         */
        void rejectTransfer();
        
        /** The user wishes to cancel the file transfer.
         */
        void cancelTransfer();
private:        
        void sendFile(const std::string & msg_body);
        void receiveFile(const std::string & msg_body);
    };    
    
    
    /** Represents a connection to another users computer for the purpose of
     *  transferring a file.
     *
     *  @todo Finish refactoring to store progress explicitly rather than in
     *        a complex combination of member variables.
     */
    class FileTransferConnection : public Connection
    {
public:
        
        /** Are we sending or receiving the file?
         */
        typedef enum 
        {
            MSNFTP_SEND,
            MSNFTP_RECV
        } Direction;
        
        /** Are we being connected to or connecting to them?
         */
        typedef enum
        {
            MSNFTP_SERVER,
            MSNFTP_CLIENT
        } Perspective;
        
        /** How far through the process are we?
         */
        typedef enum
        {
            WAITING_FOR_CONNECTION,
            NEGOTIATING,
            TRANSFERRING
        } Progress;
        
        /** AuthData contains authentication information
         *  relating to a FileTransferConnection.
         */
        class AuthData : public ::MSN::AuthData
        {
public:
            std::string cookie;
            Direction direction;
            Perspective perspective;
            FileTransferInvitation *inv;
            FILE *fd;
            bool connected;
            unsigned long bytes_done;
            int num_ignore;
            
            AuthData(Passport username_, std::string cookie_, Direction direction_, 
                                 FileTransferInvitation *inv_=NULL, FILE *fd_=NULL, bool connected_=false,
                                 unsigned long bytes_done_=0) :
                ::MSN::AuthData(username_), cookie(cookie_), direction(direction_),
                perspective((direction == MSNFTP_RECV ? MSNFTP_CLIENT : MSNFTP_SERVER)), inv(inv_), 
                fd(fd_), connected(connected_), bytes_done(bytes_done_), num_ignore(0) {};
            virtual ~AuthData() { if (fd) fclose(fd); };
        };
        
        FileTransferConnection::AuthData auth;
        
        FileTransferConnection(AuthData & auth_) : Connection(), auth(auth_) {};
        virtual ~FileTransferConnection();
        
        /** @todo Should this really be an empty function? */
        virtual void connect(const std::string & hostname, unsigned int port) {};
        virtual void disconnect();
        virtual void dispatchCommand(std::vector<std::string> & args) {};

        virtual void socketIsWritable();
        virtual void socketConnectionCompleted();
        virtual void dataArrivedOnSocket();
protected:
        virtual void handleIncomingData();

private:
        void handleSend();
        void handleReceive();
        
        void handleSend_WaitingForConnection();
        void handleSend_Negotiating();
        void handleSend_Transferring();
        void handleSend_Bye();
        
        void handleReceive_Negotiating();
        void handleReceive_Transferring();
        
        SwitchboardServerConnection *switchboardConnection() { return this->auth.inv->switchboardConnection; };
    };
}
#endif
