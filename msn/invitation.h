#ifndef __msn_invitation_h__
#define __msn_invitation_h__

/*
 * msn_invitation.h
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


#include <string>
#include <msn/passport.h>

namespace MSN
{

    class SwitchboardServerConnection;

    class Invitation
    {
public:
        enum ApplicationType
        {
            MSNFTP = 1
        };
        ApplicationType application;
        std::string cookie;
        Passport otherUser;
        SwitchboardServerConnection *switchboardConnection;
        
        Invitation(ApplicationType application_, const std::string & cookie_, 
                   Passport otherUser_, SwitchboardServerConnection * switchboardConnection_) :
            application(application_), cookie(cookie_),
            otherUser(otherUser_), switchboardConnection(switchboardConnection_) {};
        virtual ~Invitation() {};
        
        virtual void invitationWasAccepted(const std::string & body) = 0;
        virtual void invitationWasCanceled(const std::string & body) = 0;
        
        bool invitationWasSent();
    };
}

#endif
