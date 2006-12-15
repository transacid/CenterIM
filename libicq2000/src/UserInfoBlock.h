/*
 * UserInfoBlock
 *
 * Copyright (C) 2001 Barnaby Gray <barnaby@beedesign.co.uk>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#ifndef USERINFOBLOCK_H
#define USERINFOBLOCK_H

#include <string>

#include "buffer.h"
#include "Capabilities.h"

namespace ICQ2000 {
 
  /* the user information block of screenname and then TLVs
   * of user info appears in several different SNACs so
   * encapsulate it here
   */
  class UserInfoBlock {
   protected:
    std::string m_screenname;
    unsigned short m_warninglevel, m_userClass;
    unsigned char m_allowDirect, m_webAware;
    unsigned short m_status;
    bool m_birthday;
    unsigned int m_timeOnline;
    unsigned int m_signupDate, m_signonDate;
    unsigned int m_lan_ip, m_ext_ip;
    unsigned int m_dc_cookie;
    unsigned short m_lan_port, m_ext_port, m_firewall;
    unsigned char m_tcp_version;
    bool m_contains_capabilities;
    Capabilities m_capabilities;
    
   public:
    UserInfoBlock();

    std::string getScreenName() const;
    unsigned int getUIN() const;
    unsigned int getTimeOnline() const;
    unsigned int getSignupDate() const;
    unsigned int getSignonDate() const;
    unsigned int getLanIP() const;
    unsigned int getExtIP() const;
    unsigned short getLanPort() const;
    unsigned short getExtPort() const;
    unsigned short getFirewall() const;
    unsigned int getDCCookie() const;
    unsigned char getTCPVersion() const;
    unsigned short getStatus() const;
    bool getBirthday() const;
    
    bool contains_capabilities() const;
    Capabilities get_capabilities() const;

    void Parse(Buffer& b);
  };

}

#endif
