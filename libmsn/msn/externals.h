#ifndef __msn_externals_h__
#define __msn_externals_h__

/*
 * externals.h
 * libmsn
 *
 * Created by Meredydd Luff.
 * Copyright (c) 2004 Meredydd Luff. All rights reserved.
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

#include <msn/buddy.h>

namespace MSN
{
    class ListSyncInfo;
    
    class Callbacks
    {
    public:
        virtual void registerSocket(int s, int read, int write) = 0;
        virtual void unregisterSocket(int s) = 0;
        
        virtual void showError(MSN::Connection * conn, std::string msg) = 0;
        
        virtual void buddyChangedStatus(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname, MSN::BuddyStatus state) = 0;
        virtual void buddyOffline(MSN::Connection * conn, MSN::Passport buddy) = 0;
        
        virtual void log(int writing, const char* buf) = 0;
        
        virtual void gotFriendlyName(MSN::Connection * conn, std::string friendlyname) = 0;
        virtual void gotBuddyListInfo(MSN::NotificationServerConnection * conn, MSN::ListSyncInfo * data) = 0;
        virtual void gotLatestListSerial(MSN::Connection * conn, int serial) = 0;
        virtual void gotGTC(MSN::Connection * conn, char c) = 0;
        virtual void gotBLP(MSN::Connection * conn, char c) = 0;
        
        virtual void gotNewReverseListEntry(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname) = 0;
        
        virtual void addedListEntry(MSN::Connection * conn, std::string list, MSN::Passport buddy, int groupID) = 0;
        
        virtual void removedListEntry(MSN::Connection * conn, std::string list, MSN::Passport buddy, int groupID) = 0;
        
        virtual void addedGroup(MSN::Connection * conn, std::string groupName, int groupID) = 0;
        virtual void removedGroup(MSN::Connection * conn, int groupID) = 0;
        virtual void renamedGroup(MSN::Connection * conn, int groupID, std::string newGroupName) = 0;
        
        virtual void gotSwitchboard(MSN::SwitchboardServerConnection * conn, const void * tag) = 0;
        
        virtual void buddyJoinedConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, int is_initial) = 0;
        
        virtual void buddyLeftConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy) = 0;
        
        virtual void gotInstantMessage(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, MSN::Message * msg) = 0;
        
        virtual void failedSendingMessage(MSN::Connection * conn) = 0;
        
        virtual void buddyTyping(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname) = 0;
        
        virtual void gotInitialEmailNotification(MSN::Connection * conn, int unread_inbox, int unread_folders) = 0;
        
        virtual void gotNewEmailNotification(MSN::Connection * conn, std::string from, std::string subject) = 0;
        
        virtual void gotFileTransferInvitation(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname, MSN::FileTransferInvitation * inv) = 0;
        
        virtual void fileTransferProgress(MSN::FileTransferInvitation * inv, std::string status, unsigned long recv, unsigned long total) = 0;
        
        virtual void fileTransferFailed(MSN::FileTransferInvitation * inv, int error, std::string message) = 0;
        
        virtual void fileTransferSucceeded(MSN::FileTransferInvitation * inv) = 0;
        
        virtual void gotNewConnection(MSN::Connection * conn) = 0;
        
        virtual void closingConnection(MSN::Connection * conn) = 0;
        
        virtual void changedStatus(MSN::Connection * conn, MSN::BuddyStatus state) = 0;
        
        virtual int connectToServer(std::string server, int port, bool *connected) = 0;
        
        virtual int listenOnPort(int port) = 0;
        
        virtual std::string getOurIP() = 0;
        
        virtual std::string getSecureHTTPProxy() = 0;
    };
}
#endif
