/*
 * SNAC - General
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

#ifndef SNAC_GEN_H
#define SNAC_GEN_H

#include <string>

#include "SNAC-base.h"
#include "UserInfoBlock.h"

namespace ICQ2000 {

  // Generic (Family 0x0001)
  const unsigned short SNAC_GEN_Error = 0x0001;
  const unsigned short SNAC_GEN_ClientReady = 0x0002;
  const unsigned short SNAC_GEN_ServerReady = 0x0003;
  const unsigned short SNAC_GEN_NewService = 0x0004;
  const unsigned short SNAC_GEN_Redirect = 0x0005;
  const unsigned short SNAC_GEN_RequestRateInfo = 0x0006;
  const unsigned short SNAC_GEN_RateInfo = 0x0007;
  const unsigned short SNAC_GEN_RateInfoAck = 0x0008;
  const unsigned short SNAC_GEN_RateInfoChange = 0x000a;
  const unsigned short SNAC_GEN_ServerPause = 0x000b;
  const unsigned short SNAC_GEN_ServerResume = 0x000d;

  const unsigned short SNAC_GEN_UserInfoRequest = 0x000e;
  const unsigned short SNAC_GEN_UserInfo = 0x000f;
  const unsigned short SNAC_GEN_Evil = 0x0010; // ??
  const unsigned short SNAC_GEN_SetIdle = 0x0011; // maybe AIM specific?
  const unsigned short SNAC_GEN_MigrationRequest = 0x0012; // maybe AIM specific?
  const unsigned short SNAC_GEN_MOTD = 0x0013;
  const unsigned short SNAC_GEN_SetPrivFlags = 0x0014;
  const unsigned short SNAC_GEN_WellKnownURL = 0x0015; // ??
  const unsigned short SNAC_GEN_NOP = 0x0016;

  // The next lot seem to be ICQ specific
  const unsigned short SNAC_GEN_Capabilities = 0x0017;
  const unsigned short SNAC_GEN_CapAck = 0x0018;
  const unsigned short SNAC_GEN_SetStatus = 0x001e;

  // ------------ Generic (Family 0x0001) SNACs -------------
  
  class GenericSNAC : virtual public SNAC {
   public:
    unsigned short Family() const { return SNAC_FAM_GEN; }
  };

  class ServerReadySNAC : public GenericSNAC, public InSNAC {
   protected:
    void ParseBody(Buffer& b);
    
   public:
    ServerReadySNAC() { }

    unsigned short Subtype() const { return SNAC_GEN_ServerReady; }
  };

  class RequestRateInfoSNAC : public GenericSNAC, public OutSNAC {
   protected:
    void OutputBody(Buffer& b) const;

   public:
    RequestRateInfoSNAC() { }
    unsigned short Subtype() const { return SNAC_GEN_RequestRateInfo; }
  };

  class RateInfoSNAC : public GenericSNAC, public InSNAC {
   protected:
    void ParseBody(Buffer& b);

   public:
    RateInfoSNAC() { }
    unsigned short Subtype() const { return SNAC_GEN_RateInfo; }
  };

  class RateInfoAckSNAC : public GenericSNAC, public OutSNAC {
   protected:
    void OutputBody(Buffer& b) const;

   public:
    RateInfoAckSNAC() { }
    unsigned short Subtype() const { return SNAC_GEN_RateInfoAck; }
  };

  class CapabilitiesSNAC : public GenericSNAC, public OutSNAC {
   protected:
    void OutputBody(Buffer& b) const;

   public:
    CapabilitiesSNAC() { }
    unsigned short Subtype() const { return SNAC_GEN_Capabilities; }
  };

  class CapAckSNAC : public GenericSNAC, public InSNAC {
   protected:
    void ParseBody(Buffer& b);
    
   public:
    CapAckSNAC() { }

    unsigned short Subtype() const { return SNAC_GEN_CapAck; }
  };

  class SetStatusSNAC : public GenericSNAC, public OutSNAC {
   private:
    unsigned short m_status, m_port;
    unsigned int m_ip;
    bool m_sendextra, m_web_aware;

   protected:
    void OutputBody(Buffer& b) const;

   public:
    SetStatusSNAC(unsigned short status, bool web_aware);
    unsigned short Subtype() const { return SNAC_GEN_SetStatus; }

    void setSendExtra(bool b);
    void setIP(unsigned int ip);
    void setPort(unsigned short port);
  };

  class SetIdleSNAC : public GenericSNAC, public OutSNAC {
   protected:
    void OutputBody(Buffer& b) const;

   public:
    SetIdleSNAC() { }
    unsigned short Subtype() const { return SNAC_GEN_SetIdle; }
  };

  class ClientReadySNAC : public GenericSNAC, public OutSNAC {
   protected:
    void OutputBody(Buffer& b) const;

   public:
    ClientReadySNAC() { }
    unsigned short Subtype() const { return SNAC_GEN_ClientReady; }
  };

  class PersonalInfoRequestSNAC : public GenericSNAC, public OutSNAC {
   protected:
    void OutputBody(Buffer& b) const;

   public:
    PersonalInfoRequestSNAC() { }
    unsigned short Subtype() const { return SNAC_GEN_UserInfoRequest; }
  };

  class UserInfoSNAC : public GenericSNAC, public InSNAC {
   private:
    UserInfoBlock m_userinfo;

   protected:
    void ParseBody(Buffer& b);
    
   public:
    UserInfoSNAC() { }

    unsigned short Subtype() const { return SNAC_GEN_UserInfo; }
    const UserInfoBlock& getUserInfo() const { return m_userinfo; }
  };

  const unsigned char MOTD_MANDATORY_UPGRADE = 0x01; // hehe - like we're going to obey this :-)
  const unsigned char MOTD_ADVISORY_UPGRADE = 0x02;
  const unsigned char MOTD_SYSTEM_BULLETIN = 0x03;
  const unsigned char MOTD_NORMAL = 0x04;

  class MOTDSNAC : public GenericSNAC, public InSNAC {
   private:
    unsigned char m_status;
    std::string m_url;

   protected:
    void ParseBody(Buffer& b);
    
   public:
    MOTDSNAC() { }

    unsigned short Subtype() const { return SNAC_GEN_MOTD; }
  };
  
  class RateInfoChangeSNAC : public GenericSNAC, public InSNAC {
   private:
    unsigned short m_code;	
    unsigned short m_rateclass;	
    unsigned int m_windowsize;
    unsigned int m_clear;
    unsigned int m_alert;
    unsigned int m_limit;
    unsigned int m_disconnect;
    unsigned int m_currentavg;
    unsigned int m_maxavg;
    
   protected:
    void ParseBody(Buffer& b);
    
   public:
    RateInfoChangeSNAC() { }
    unsigned short getCode() const { return m_code; }	
    unsigned short getRateClass() const { return m_rateclass; }	
    unsigned int getWindowSize() const { return m_windowsize; }
    unsigned int getClear() const { return m_clear; }
    unsigned int getAlert() const { return m_alert; }
    unsigned int getLimit() const { return m_limit; }
    unsigned int getDisconnect() const { return m_disconnect; }
    unsigned int getCurrentAvg() const { return m_currentavg; }
    unsigned int getMaxAvg() const { return m_maxavg; }
    
    unsigned short Subtype() const { return SNAC_GEN_RateInfoChange; }
  };
   
}

#endif
