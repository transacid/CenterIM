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

namespace MSN
{
    class Group;
    class Buddy
    {
	/** @todo  BPR's need to be handled at @i any time, not just when syncing. */
public:
	class PhoneNumber
	{
public:
	    std::string title;
	    std::string number;
	    bool enabled;
	    
	    PhoneNumber(std::string title_, std::string number_, bool enabled_=true)
		: title(title_), number(number_), enabled(enabled_) {};
	};
	
	std::string userName;
	std::string friendlyName;
	
	std::list<Buddy::PhoneNumber> phoneNumbers;
	std::list<Group *> groups;
	
	Buddy(std::string userName_, std::string friendlyName_) :
	    userName(userName_), friendlyName(friendlyName_) {};
	bool const operator==(const Buddy &other) { return userName == other.userName; }
    };
    
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
