#ifndef __msn_buddy_h__
#define __msn_buddy_h__

/*
 * buddy.h
 * libmsn
 *
 * Created by Mark Rowe on Mon Apr 19 2004.
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

#include <string>
#include <list>
#include <msn/passport.h>

namespace MSN
{
    /** The online state of a buddy.
    */
    
    enum BuddyStatus
    {
        STATUS_AVAILABLE,
        STATUS_BUSY,
        STATUS_IDLE,
        STATUS_BERIGHTBACK,
        STATUS_AWAY,
        STATUS_ONTHEPHONE,
        STATUS_OUTTOLUNCH,
        STATUS_INVISIBLE
    };
    
    std::string buddyStatusToString(BuddyStatus s);
    BuddyStatus buddyStatusFromString(std::string s);
    
    class Group;
    
    /** The Buddy class contains information about a member of a buddy list.
     *
     *  Each Buddy is made up of their passport address (@a userName),
     *  user-visible display name (@a friendlyName), zero or more @a phoneNumbers,
     *  and zero or more @a groups on the buddy list that they belong to.
     *
     *  It is only currently used during MSN::ext::gotBuddyListInfo to
     *  store contact information about a buddy that was retrieved during
     *  the buddy list synchronisation process.
     */
    class Buddy
    {
        /** @todo  BPR's need to be handled at any time, not just when syncing. */
public:
        /** The PhoneNumber class contains information about a single phone number
         *  that is retrieved during the buddy list synchronisation process.
         */
        class PhoneNumber
        {
public:
            /** The name of this phone number.
             *
             *  @todo Should this be an enumeration containing the possible
             *        types of phone number?
             */
            std::string title;
            
            /** The phone number */
            std::string number;
            
            /** @todo Document me! */
            bool enabled;
            
            PhoneNumber(std::string title_, std::string number_, bool enabled_=true)
                : title(title_), number(number_), enabled(enabled_) {};
        };
        
        /** Their passport address */
        Passport userName;
        
        /** Their friendly name */
        std::string friendlyName;
        
        /** A list of phone numbers related to this buddy */
        std::list<Buddy::PhoneNumber> phoneNumbers;
        
        /** A list of Group's that this buddy is a member of */
        std::list<Group *> groups;
        
        Buddy(Passport userName_, std::string friendlyName_) :
            userName(userName_), friendlyName(friendlyName_) {};
        bool const operator==(const Buddy &other) { return userName == other.userName; }
    };
    
    /** The Group class represents a group of contacts on the buddy list.
     *
     *  Each group is represented by a @a groupID and has a user-visible
     *  @a name.
     */
    class Group
    {
public:
        int groupID;
        std::string name;
        std::list<Buddy *> buddies;
        
        Group(int groupID_, std::string name_)
            : groupID(groupID_), name(name_) {};
        
        Group() : groupID(-1), name("INVALID") {};
    };
}

#endif
