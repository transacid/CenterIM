/*
 * UserInfoBlock
 *
 * Copyright (C) 2001 Barnaby Gray <barnaby@beedesign.co.uk>
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

#include "UserInfoBlock.h"

#include "Contact.h"
#include "TLV.h"

using std::string;

namespace ICQ2000 {

  UserInfoBlock::UserInfoBlock()
    : m_contains_capabilities(false)
  { }

  string UserInfoBlock::getScreenName() const { return m_screenname; }

  unsigned int UserInfoBlock::getUIN() const {
    return Contact::StringtoUIN(m_screenname);
  }
  
  unsigned int UserInfoBlock::getTimeOnline() const { return m_timeOnline; }
  unsigned int UserInfoBlock::getSignupDate() const { return m_signupDate; }
  unsigned int UserInfoBlock::getSignonDate() const { return m_signonDate; }
  unsigned int UserInfoBlock::getLanIP() const { return m_lan_ip; }
  unsigned int UserInfoBlock::getExtIP() const { return m_ext_ip; }
  unsigned short UserInfoBlock::getLanPort() const { return m_lan_port; }
  unsigned short UserInfoBlock::getExtPort() const { return m_ext_port; }
  unsigned short UserInfoBlock::getFirewall() const { return m_firewall; }
  unsigned int UserInfoBlock::getDCCookie() const { return m_dc_cookie; }
  unsigned char UserInfoBlock::getTCPVersion() const { return m_tcp_version; }
  unsigned short UserInfoBlock::getStatus() const { return m_status; }
  bool UserInfoBlock::getBirthday() const { return m_birthday; }
  bool UserInfoBlock::contains_capabilities() const { return m_contains_capabilities; }
  Capabilities UserInfoBlock::get_capabilities() const { return m_capabilities; }
  
  void UserInfoBlock::Parse(Buffer& b) {
    // (byte)length, string screenname
    b.UnpackByteString(m_screenname);

    b >> m_warninglevel;
    unsigned short no_tlvs;
    b >> no_tlvs;
    
    TLVList tlvlist;
    tlvlist.Parse(b, TLV_ParseMode_Channel02, no_tlvs);

    m_userClass = 0;
    if (tlvlist.exists(TLV_UserClass)) {
      UserClassTLV *t = (UserClassTLV*)tlvlist[TLV_UserClass];
      m_userClass = t->Value();
    }

    m_status = 0;
    m_allowDirect = 0;
    m_webAware = 0;
    if (tlvlist.exists(TLV_Status)) {
      StatusTLV *t = (StatusTLV*)tlvlist[TLV_Status];
      m_allowDirect = t->getAllowDirect();
      m_webAware = t->getWebAware();
      m_status = t->getStatus();
      m_birthday = t->getBirthday();
    }

    m_timeOnline = 0;
    if (tlvlist.exists(TLV_TimeOnline)) {
      TimeOnlineTLV *t = (TimeOnlineTLV*)tlvlist[TLV_TimeOnline];
      m_timeOnline = t->Value();
    }

    m_signupDate = 0;
    if (tlvlist.exists(TLV_SignupDate)) {
      SignupDateTLV *t = (SignupDateTLV*)tlvlist[TLV_SignupDate];
      m_signupDate = t->Value();
    }

    m_signonDate = 0;
    if (tlvlist.exists(TLV_SignonDate)) {
      SignonDateTLV *t = (SignonDateTLV*)tlvlist[TLV_SignonDate];
      m_signonDate = t->Value();
    }

    m_lan_ip = 0;
    m_lan_port = 0;
    m_firewall = 0;
    m_tcp_version = 0;
    if (tlvlist.exists(TLV_LANDetails)) {
      LANDetailsTLV *t = (LANDetailsTLV*)tlvlist[TLV_LANDetails];
      m_lan_ip = t->getLanIP();
      m_lan_port = t->getLanPort();
      m_firewall = t->getFirewall();
      m_tcp_version = t->getTCPVersion();
      m_dc_cookie = t->getDCCookie();
    }

    m_ext_ip = 0;
    if (tlvlist.exists(TLV_IPAddress)) {
      IPAddressTLV *t = (IPAddressTLV*)tlvlist[TLV_IPAddress];
      m_ext_ip = t->Value();
    }

    m_ext_port = 0;
    if (tlvlist.exists(TLV_Port)) {
      PortTLV *t = (PortTLV*)tlvlist[TLV_Port];
      m_ext_port = t->Value();
    }

    if (tlvlist.exists(TLV_Capabilities)) {
      CapabilitiesTLV *t = (CapabilitiesTLV*)tlvlist[TLV_Capabilities];
      m_contains_capabilities = true;
      m_capabilities = t->get_capabilities();
    }

  }

}
