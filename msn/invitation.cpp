/*
 * invitation.cpp
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

#include <msn/invitation.h>
#include <msn/switchboardserver.h>
#include <list>
#include <algorithm>

namespace MSN
{
    template <class _Tp>
    class hasCookieOf
    {
        const std::string & cookie;
public:
        hasCookieOf(const std::string & __i) : cookie(__i) {};
        bool operator()(const _Tp &__x) { return __x->cookie == cookie; }
    };

    bool Invitation::invitationWasSent()
    {
        std::list<Invitation *> & in_list = this->switchboardConnection->invitationsReceived;
        std::list<Invitation *> & out_list = this->switchboardConnection->invitationsSent;
        std::list<Invitation *>::iterator i;
        
        i = std::find_if(in_list.begin(), in_list.end(), hasCookieOf<Invitation *>(cookie));
        if (i != in_list.end())
            return false;
        
        i = std::find_if(out_list.begin(), out_list.end(), hasCookieOf<Invitation *>(cookie));
        if (i != out_list.end())
            return true;
    
        return false;
    }
}
