/*
 * msntest.cpp
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

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#include <msn/msn.h>
#include <string>
#include <iostream>

class Callbacks : public MSN::Callbacks
{
    
    virtual void registerSocket(int s, int read, int write);
    virtual void unregisterSocket(int s);
    
    virtual void showError(MSN::Connection * conn, std::string msg);
    
    virtual void buddyChangedStatus(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname, MSN::BuddyStatus state);
    virtual void buddyOffline(MSN::Connection * conn, MSN::Passport buddy);
    
    virtual void log(int writing, const char* buf);
    
    virtual void gotFriendlyName(MSN::Connection * conn, std::string friendlyname);
    virtual void gotBuddyListInfo(MSN::NotificationServerConnection * conn, MSN::ListSyncInfo * data);
    virtual void gotLatestListSerial(MSN::Connection * conn, int serial);
    virtual void gotGTC(MSN::Connection * conn, char c);
    virtual void gotBLP(MSN::Connection * conn, char c);
    
    virtual void gotNewReverseListEntry(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname);
    
    virtual void addedListEntry(MSN::Connection * conn, std::string list, MSN::Passport buddy, int groupID);
    
    virtual void removedListEntry(MSN::Connection * conn, std::string list, MSN::Passport buddy, int groupID);
    
    virtual void addedGroup(MSN::Connection * conn, std::string groupName, int groupID);
    virtual void removedGroup(MSN::Connection * conn, int groupID);
    virtual void renamedGroup(MSN::Connection * conn, int groupID, std::string newGroupName);
    
    virtual void gotSwitchboard(MSN::SwitchboardServerConnection * conn, const void * tag);
    
    virtual void buddyJoinedConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, int is_initial);
    
    virtual void buddyLeftConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy);
    
    virtual void gotInstantMessage(MSN::SwitchboardServerConnection * conn, MSN::Passport buddy, std::string friendlyname, MSN::Message * msg);
    
    virtual void failedSendingMessage(MSN::Connection * conn);
    
    virtual void buddyTyping(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname);
    
    virtual void gotInitialEmailNotification(MSN::Connection * conn, int unread_inbox, int unread_folders);
    
    virtual void gotNewEmailNotification(MSN::Connection * conn, std::string from, std::string subject);
    
    virtual void gotFileTransferInvitation(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname, MSN::FileTransferInvitation * inv);
    
    virtual void fileTransferProgress(MSN::FileTransferInvitation * inv, std::string status, unsigned long recv, unsigned long total);
    
    virtual void fileTransferFailed(MSN::FileTransferInvitation * inv, int error, std::string message);
    
    virtual void fileTransferSucceeded(MSN::FileTransferInvitation * inv);
    
    virtual void gotNewConnection(MSN::Connection * conn);
    
    virtual void closingConnection(MSN::Connection * conn);
    
    virtual void changedStatus(MSN::Connection * conn, MSN::BuddyStatus state);
    
    virtual int connectToServer(std::string server, int port, bool *connected);
    
    virtual int listenOnPort(int port);
    
    virtual std::string getOurIP();
    
    virtual std::string getSecureHTTPProxy();
};

struct pollfd mySockets[21];

void handle_command(MSN::NotificationServerConnection &);
int countsocks(void);

int main()
{
    for (int i = 1; i < 20; i++)
    {
        mySockets[i].fd = -1;
        mySockets[i].events = POLLIN; 
        mySockets[i].revents = 0;
    }
    
    mySockets[0].fd = 0;
    mySockets[0].events = POLLIN;
    mySockets[0].revents = 0;
    
    Callbacks cb;
    MSN::Passport uname;
    char *pass = NULL;
    while (1)
    {
        fprintf(stderr, "Enter your login name: ");
        fflush(stdout);
        try
        {
            std::cin >> uname;
            break;
        }
        catch (MSN::InvalidPassport & e)
        {
            std::cout << e.what() << std::endl;
        }
    }
    
    pass = getpass("Enter your password: ");
    
    fprintf(stderr, "Connecting to the MSN Messenger service...\n");
    
    MSN::NotificationServerConnection mainConnection(uname, pass, cb);
    mainConnection.connect("messenger.hotmail.com", 1863);
        
    fprintf(stderr, "> ");
    fflush(stderr);
    while (1)
    {
        poll(mySockets, 20, -1);
        for (int i = 1; i < 20; i++)
        {
            if (mySockets[i].fd == -1)
                break; 
            
            if (mySockets[i].revents & POLLHUP) 
	    {
                mySockets[i].revents = 0; 
		continue; 
	    }
            
            if (mySockets[i].revents & (POLLERR | POLLNVAL))
            {
                printf("Dud socket (%d)! Code %x (ERR=%x, INVAL=%x)\n", mySockets[i].fd, mySockets[i].revents, POLLERR, POLLNVAL);
                
                MSN::Connection *c;
                
                // Retrieve the connection associated with the
                // socket's file handle on which the event has
                // occurred.
                c = mainConnection.connectionWithSocket(mySockets[i].fd);
                
                // if this is a libmsn socket
                if (c != NULL)
                {
                    // Delete the connection.  This will cause the resources
                    // that are being used to be freed.
                    delete c;
                }
                
                mySockets[i].fd = -1;
                mySockets[i].revents = 0;
                continue;
            }
            
            if (mySockets[i].revents & (POLLIN | POLLOUT | POLLPRI))
            {
                MSN::Connection *c;
                
                // Retrieve the connection associated with the
                // socket's file handle on which the event has
                // occurred.
                c = mainConnection.connectionWithSocket(mySockets[i].fd);
                
                // if this is a libmsn socket
                if (c != NULL)
                {
                    // If we aren't yet connected, a socket event means that
                    // our connection attempt has completed.
                    if (c->isConnected() == false)
                        c->socketConnectionCompleted();
                    
                    // If this event is due to new data becoming available 
                    if (mySockets[i].revents & POLLIN)
                        c->dataArrivedOnSocket();
                    
                    // If this event is due to the socket becoming writable
                    if (mySockets[i].revents & POLLOUT)
                        c->socketIsWritable();
                }          
            }
            mySockets[i].revents = 0;
        }

        if (mySockets[0].revents & POLLIN)
        {
            handle_command(mainConnection);
            mySockets[0].revents = 0;
        }
    }
}

void handle_command(MSN::NotificationServerConnection & mainConnection)
{
    char command[40];
    
    if (scanf(" %s", command) == EOF)
    {
        printf("\n");
        exit(0);
    }
    
    if (!strcmp(command, "quit"))
    {
        exit(0);
    } else if (!strcmp(command, "msg")) {
        char rcpt[80];
        char msg[1024];
        
        scanf(" %s", rcpt);
        
        fgets(msg, 1024, stdin);
        
        msg[strlen(msg)-1] = '\0';
        
        const std::string rcpt_ = rcpt;
        const std::string msg_ = msg;
        const std::pair<std::string, std::string> *ctx = new std::pair<std::string, std::string>(rcpt_, msg_);
        mainConnection.requestSwitchboardConnection(ctx);
    } else if (!strcmp(command, "status")) {
        char state[10];
        
        scanf(" %s", state);
        
        mainConnection.setState(MSN::buddyStatusFromString(state));
    } else if (!strcmp(command, "friendlyname")) {
        char fn[256];
        
        fgets(fn, 256, stdin);
        fn[strlen(fn)-1] = '\0';
        
        mainConnection.setFriendlyName(fn);
    } else if (!strcmp(command, "add")) {
        char list[10];
        char user[128];
        
        scanf(" %s %s", list, user);
        
        mainConnection.addToList(list, user);
    } else if (!strcmp(command, "del")) {
        char list[10];
        char user[128];
        
        scanf(" %s %s", list, user);
        
        mainConnection.removeFromList(list, user);
    } else if (!strcmp(command, "reconnect")) {
        if (mainConnection.connectionState() != MSN::NotificationServerConnection::NS_DISCONNECTED)
            mainConnection.disconnect();

        mainConnection.connect("messenger.hotmail.com", 1863);
    } else if (!strcmp(command, "disconnect")) {
        mainConnection.disconnect();
    } else {
        fprintf(stderr, "\nBad command \"%s\"", command);
    }
    
    fprintf(stderr, "\n> ");
    fflush(stderr);
}

int countsocks(void)
{
    int retval = 0;
    
    for (int a = 0; a < 20; a++)
    {
        if (mySockets[a].fd == -1) 
            break; 
        
        retval++;
    }
    return retval;
}

void Callbacks::registerSocket(int s, int reading, int writing)
{
    for (int a = 1; a < 20; a++)
    {
        if (mySockets[a].fd == -1)
        {
            mySockets[a].events = 0;
            if (reading)
                mySockets[a].events |= POLLIN;  
            
            if (writing)
                mySockets[a].events |= POLLOUT;  
            
            mySockets[a].fd = s;
            return;
        }
    }
}

void Callbacks::unregisterSocket(int s)
{
    for (int a = 1; a < 20; a++)
    {
        if (mySockets[a].fd == s)
        {
            for (int b = a; b < 19; b++)
                mySockets[b].fd = mySockets[b + 1].fd;
            
            mySockets[19].fd = -1;
        }
    }
}

void Callbacks::gotFriendlyName(MSN::Connection * conn, std::string friendlyname)
{
    printf("Your friendlyname is now: %s\n", friendlyname.c_str());
}

void Callbacks::gotBuddyListInfo(MSN::NotificationServerConnection * conn, MSN::ListSyncInfo * info)
{
    int a;
    std::list<MSN::Buddy> & list = info->forwardList;
    std::list<MSN::Buddy>::iterator i;
        
    for (a = 0; a < 4; a++)
    {
        switch (a)
        {
            case 0:
            {
                list = info->forwardList; 
                printf("Forward list:\n"); 
                break; 
            }
                
            case 1:
            { 
                list = info->reverseList; 
                printf("Reverse list:\n"); 
                break; 
            }
                
            case 2:
            {
                list = info->allowList; 
                printf("Allow list:\n");
                break;
            }
                
            case 3:
            { 
                list = info->blockList; 
                printf("Block list:\n"); 
                break; 
            }
        }
        
        for (i = list.begin(); i != list.end(); i++)
        {
            printf("-  %s (%s)\n", (*i).friendlyName.c_str(), (*i).userName.c_str());
            std::list<MSN::Buddy::PhoneNumber>::iterator pns = (*i).phoneNumbers.begin();
            std::list<MSN::Group *>::iterator g = (*i).groups.begin();
            for (; g != (*i).groups.end(); g++)
            {
                printf("    G: %s\n", (*g)->name.c_str());
            }
            
            for (; pns != (*i).phoneNumbers.end(); pns++)
            {
                printf("    %s: %s (%d)\n", (*pns).title.c_str(), (*pns).number.c_str(), (*pns).enabled);
            }
        }
    }
    
    printf("Available Groups:\n");
    std::map<int, MSN::Group>::iterator g = info->groups.begin();
    for (; g != info->groups.end(); g++)
    {
        printf("    %d: %s\n", (*g).second.groupID, (*g).second.name.c_str());
    }
}

void Callbacks::gotLatestListSerial(MSN::Connection * conn, int serial)
{
    printf("The latest serial number is: %d\n", serial);
}

void Callbacks::gotGTC(MSN::Connection * conn, char c)
{
    printf("Your GTC value is now %c\n", c);
}

void Callbacks::gotBLP(MSN::Connection * conn, char c)
{
    printf("Your BLP value is now %cL\n", c);
}

void Callbacks::gotNewReverseListEntry(MSN::Connection * conn, MSN::Passport username, std::string friendlyname)
{
    printf("%s (%s) has added you to their contact list.\nYou might want to add them to your Allow or Block list\n", username.c_str(), friendlyname.c_str());
}

void Callbacks::addedListEntry(MSN::Connection * conn, std::string list, MSN::Passport username, int groupID)
{
    printf("%s is now on your %s, group %d\n", username.c_str(), list.c_str(), groupID);
}

void Callbacks::removedListEntry(MSN::Connection * conn, std::string list, MSN::Passport username, int groupID)
{
    printf("%s has been removed from your %s, group %d\n", username.c_str(), list.c_str(), groupID);
}

void Callbacks::addedGroup(MSN::Connection *conn, std::string groupName, int groupID)
{
    printf("A group named %s (%d) was added\n", groupName.c_str(), groupID);
}

void Callbacks::removedGroup(MSN::Connection *conn, int groupID)
{
    printf("A group with ID %d was removed\n", groupID);
}

void Callbacks::renamedGroup(MSN::Connection *conn, int groupID, std::string newGroupName)
{
    printf("A group with ID %d was renamed to %s\n", groupID, newGroupName.c_str());
}

void Callbacks::showError(MSN::Connection * conn, std::string msg)
{
    printf("MSN: Error: %s\n", msg.c_str());
}

void Callbacks::buddyChangedStatus(MSN::Connection * conn, MSN::Passport buddy, std::string friendlyname, MSN::BuddyStatus status)
{
    printf("%s (%s) is now %s\n", friendlyname.c_str(), buddy.c_str(), MSN::buddyStatusToString(status).c_str());
}

void Callbacks::buddyOffline(MSN::Connection *conn, MSN::Passport buddy)
{
    printf("%s is now offline\n", buddy.c_str());
}

void Callbacks::gotSwitchboard(MSN::SwitchboardServerConnection * conn, const void * tag)
{
    printf("Got switchboard connection\n");
    if (tag)
    {
        const std::pair<std::string, std::string> *ctx = static_cast<const std::pair<std::string, std::string> *>(tag);
        conn->inviteUser(ctx->first);
    }
}

void Callbacks::buddyJoinedConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport username, std::string friendlyname, int is_initial)
{
    printf("%s (%s) is now in the session\n", friendlyname.c_str(), username.c_str());
    if (conn->auth.tag)
    {
        const std::pair<std::string, std::string> *ctx = static_cast<const std::pair<std::string, std::string> *>(conn->auth.tag);
        conn->sendMessage(ctx->second);
        delete ctx;
        conn->auth.tag = NULL;
    }
}

void Callbacks::buddyLeftConversation(MSN::SwitchboardServerConnection * conn, MSN::Passport username)
{
    printf("%s has now left the session\n", username.c_str());
}

void Callbacks::gotInstantMessage(MSN::SwitchboardServerConnection * conn, MSN::Passport username, std::string friendlyname, MSN::Message * msg)
{
    printf("--- Message from %s (%s) in font %s:\n%s\n", friendlyname.c_str(), username.c_str(), msg->getFontName().c_str(), msg->getBody().c_str());
}

void Callbacks::failedSendingMessage(MSN::Connection * conn)
{
    printf("**************************************************\n");
    printf("ERROR:  Your last message failed to send correctly\n");
    printf("**************************************************\n");
}

void Callbacks::buddyTyping(MSN::Connection * conn, MSN::Passport username, std::string friendlyname)
{
    printf("\t%s (%s) is typing...\n", friendlyname.c_str(), username.c_str());
}

void Callbacks::gotInitialEmailNotification(MSN::Connection * conn, int unread_inbox, int unread_folders)
{
    if (unread_inbox > 0) 
        printf("You have %d new messages in your Inbox\n", unread_inbox); 
    
    if (unread_folders > 0) 
        printf("You have %d new messages in other folders.\n", unread_folders); 
}

void Callbacks::gotNewEmailNotification(MSN::Connection * conn, std::string from, std::string subject)
{
    printf("New e-mail has arrived from %s.\nSubject: %s\n", from.c_str(), subject.c_str());
}

void Callbacks::gotFileTransferInvitation(MSN::Connection *conn, MSN::Passport username, std::string friendlyname, MSN::FileTransferInvitation * inv)
{
    printf("Got file transfer invitation from %s (%s)\n", friendlyname.c_str(), username.c_str());
    inv->acceptTransfer("tmp.out");
}

void Callbacks::fileTransferProgress(MSN::FileTransferInvitation * inv, std::string status, unsigned long sent, unsigned long total)
{
    printf("File transfer: %s\t(%lu/%lu bytes sent)\n", status.c_str(), sent, total);
}

void Callbacks::fileTransferFailed(MSN::FileTransferInvitation * inv, int error, std::string message)
{
    printf("File transfer failed: %s\n", message.c_str());
}

void Callbacks::fileTransferSucceeded(MSN::FileTransferInvitation * inv)
{
    printf("File transfer successfully completed\n");
}

void Callbacks::gotNewConnection(MSN::Connection * conn)
{
    if (dynamic_cast<MSN::NotificationServerConnection *>(conn))
        dynamic_cast<MSN::NotificationServerConnection *>(conn)->synchronizeLists();
}

void Callbacks::closingConnection(MSN::Connection * conn)
{
    printf("Closed connection with socket %d\n", conn->sock);
}

void Callbacks::changedStatus(MSN::Connection * conn, MSN::BuddyStatus state)
{
    printf("Your state is now: %s\n", MSN::buddyStatusToString(state).c_str());
}

int Callbacks::connectToServer(std::string hostname, int port, bool *connected)
{
    struct sockaddr_in sa;
    struct hostent     *hp;
    int s;
    
    if ((hp = gethostbyname(hostname.c_str())) == NULL) {
        errno = ECONNREFUSED;                       
        return -1;
    }
    
    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr,hp->h_addr,hp->h_length);     /* set address */
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)port);
    
    if ((s = socket(hp->h_addrtype,SOCK_STREAM,0)) < 0)     /* get socket */
        return -1;

    int oldfdArgs = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, oldfdArgs | O_NONBLOCK);
    
    if (connect(s,(struct sockaddr *)&sa,sizeof sa) < 0)
    {
        if (errno != EINPROGRESS) 
        { 
            close(s);
            return -1;
        }
        *connected = false;
    }
    else
        *connected = true;
    
    return s;
}

int Callbacks::listenOnPort(int port)
{
    int s;
    struct sockaddr_in addr;
    
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return -1;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (bind(s, (sockaddr *)(&addr), sizeof(addr)) < 0 || listen(s, 1) < 0)
    {
        close(s);
        return -1;
    }
    
    return s;
}

std::string Callbacks::getOurIP(void)
{
    struct hostent * hn;
    char buf2[1024];
    
    gethostname(buf2,1024);
    hn = gethostbyname(buf2);
    
    return inet_ntoa( *((struct in_addr*)hn->h_addr));
}

void Callbacks::log(int i, const char *s)
{
    
}

std::string Callbacks::getSecureHTTPProxy()
{
    return "";
}
