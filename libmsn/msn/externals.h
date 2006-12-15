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
    
    namespace ext
    {
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
        
        std::string getOurIP(void);
        
        std::string getSecureHTTPProxy(void);
    }
}
#endif
