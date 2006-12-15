/*
 * TLVs (Type, Length, Value)
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

#ifndef TLV_H
#define TLV_H

#include <string>
#include <map>

#include <string.h>
#include <stdlib.h>

#include "Xml.h"
#include "exceptions.h"
#include "buffer.h"
#include "constants.h"
#include "ICQ.h"
#include "Capabilities.h"

namespace ICQ2000 {
 
  // ------------- TLV numerical constants ------------

  /*
   * TLV types
   * Originally I thought TLV types were distinct within
   * each channel, but in Messages it turns out they are only
   * distinct within each block. To complicate matters you
   * then get TLVs inside TLVs..
   * Solution: the TLV parser must be told what it is expecting
   * to parse so that the correct TLV types are associated
   */

  enum TLV_ParseMode { TLV_ParseMode_Channel01,
		       TLV_ParseMode_Channel02,
		       TLV_ParseMode_Channel04,
		       TLV_ParseMode_MessageBlock,
		       TLV_ParseMode_AdvMsgBlock,
		       TLV_ParseMode_InMessageData,
		       TLV_ParseMode_InAdvMsgData,
		       TLV_ParseMode_SBL,
		       TLV_ParseMode_SBL_Rights
  };

  // Channel 0x0001
  const unsigned short TLV_Screenname = 0x0001;
  const unsigned short TLV_Password = 0x0002;
  const unsigned short TLV_ClientProfile = 0x0003;
  const unsigned short TLV_UserInfo = 0x0005;
  const unsigned short TLV_Cookie = 0x0006;
  const unsigned short TLV_CountryCode = 0x000e;
  const unsigned short TLV_Language = 0x000f;
  const unsigned short TLV_ClientBuildMinor = 0x0014;
  const unsigned short TLV_ClientType = 0x0016;
  const unsigned short TLV_ClientVersionMajor = 0x0017;
  const unsigned short TLV_ClientVersionMinor = 0x0018;
  const unsigned short TLV_ClientICQNumber = 0x0019;
  const unsigned short TLV_ClientBuildMajor = 0x001a;

  // Channel 0x0002
  const unsigned short TLV_UserClass = 0x0001;
  const unsigned short TLV_SignupDate = 0x0002;
  const unsigned short TLV_SignonDate = 0x0003;
  const unsigned short TLV_Port = 0x0004; // ??
  const unsigned short TLV_UserInfoCapabilities = 0x0005;
  const unsigned short TLV_Status = 0x0006;
  const unsigned short TLV_Unknown = 0x0008; // ??
  const unsigned short TLV_IPAddress = 0x000a;
  const unsigned short TLV_WebAddress = 0x000b;
  const unsigned short TLV_LANDetails = 0x000c;
  const unsigned short TLV_Capabilities = 0x000d;
  const unsigned short TLV_TimeOnline = 0x000f;
  const unsigned short TLV_ContactNickname = 0x0131;
  const unsigned short TLV_AuthAwaited = 0x0066;

  // Channel 0x0004
  // const unsigned short TLV_Screenname = 0x0001;
  const unsigned short TLV_ErrorURL = 0x0004;
  const unsigned short TLV_Redirect = 0x0005;
  // const unsigned short TLV_Cookie = 0x0006;
  const unsigned short TLV_ErrorCode = 0x0008;
  const unsigned short TLV_DisconnectReason = 0x0009;
  const unsigned short TLV_DisconnectMessage = 0x000b;
  const unsigned short TLV_Unknown3 = 0x000c;
  const unsigned short TLV_EmailAddress = 0x0011;
  const unsigned short TLV_RegStatus = 0x0013;

  // Message Block
  const unsigned short TLV_MessageData = 0x0002;
  const unsigned short TLV_ServerAckRequested = 0x0003;
  const unsigned short TLV_MessageIsAutoResponse = 0x0004;
  const unsigned short TLV_ICQData = 0x0005;

  // Advanced Message Block
  const unsigned short TLV_AdvMsgData = 0x0005;

  // In Message Data
  const unsigned short TLV_Unknown0501 = 0x0501;
  const unsigned short TLV_MessageText = 0x0101;

  // In Advanced Message Data
  const unsigned short TLV_AdvMsgBody = 0x2711;
  // loads more - but we don't parse them yet

  // In Server-based list data
  const unsigned short TLV_SBL_Await_Auth = 0x0066;
  const unsigned short TLV_SBL_Ids        = 0x00c8;
  const unsigned short TLV_SBL_Visibility = 0x00ca;
  const unsigned short TLV_SBL_ICQTIC     = 0x00cd;
  const unsigned short TLV_SBL_ImportTime = 0x00d4;
  const unsigned short TLV_SBL_Nick       = 0x0131;
  const unsigned short TLV_SBL_SMS_No     = 0x013a;

  // ------------- abstract TLV classes ---------------

  class TLV {
   public:
    virtual ~TLV() { }
    
    virtual unsigned short Type() const = 0;
    virtual unsigned short Length() const = 0;
  };

  // -- Inbound TLV --
  class InTLV : public TLV {
   public:
    virtual void ParseValue(Buffer& b) = 0;

    static InTLV* ParseTLV(Buffer& b, TLV_ParseMode pm);
  };

  // -- Outbound TLV --
  class OutTLV : public TLV {
   protected:
    virtual void OutputHeader(Buffer& b) const;
    virtual void OutputValue(Buffer& b) const = 0;

   public:
    virtual void Output(Buffer& b) const;
  };

  // -------------- base classes ----------------------

  class CharTLV : public OutTLV, public InTLV {
   protected:
    unsigned char m_value;
    
    virtual void OutputValue(Buffer& b) const;

   public:
    CharTLV();
    CharTLV(unsigned char n);

    unsigned short Length() const { return 1; }

    virtual void ParseValue(Buffer& b);
    virtual unsigned char Value() const { return m_value; }
  };

  class ShortTLV : public OutTLV, public InTLV {
   protected:
    unsigned short m_value;
    
    virtual void OutputValue(Buffer& b) const;

   public:
    ShortTLV();
    ShortTLV(unsigned short n);

    unsigned short Length() const { return 2; }

    virtual void ParseValue(Buffer& b);
    virtual unsigned short Value() const { return m_value; }
  };


  class LongTLV : public OutTLV, public InTLV {
   protected:
    unsigned int m_value;
    
    virtual void OutputValue(Buffer& b) const;

   public:
    LongTLV();
    LongTLV(unsigned int n);
    
    unsigned short Length() const { return 4; }

    virtual void ParseValue(Buffer& b);
    virtual unsigned int Value() const { return m_value; }
  };


  class StringTLV : public OutTLV, public InTLV {
   protected:
    std::string m_value;

    virtual void OutputValue(Buffer& b) const;

   public:
    StringTLV();
    StringTLV(const std::string& val);

    unsigned short Length() const { return m_value.size(); }

    virtual void ParseValue(Buffer& b);
    virtual std::string Value() const { return m_value; }
  };


  // --------------- actual classes -------------------

  class ErrorURLTLV : public StringTLV {
   public:
    ErrorURLTLV() { }
    unsigned short Type() const { return TLV_ErrorURL; }
  };

  class ErrorCodeTLV : public ShortTLV {
   public:
    ErrorCodeTLV() { }
    unsigned short Type() const { return TLV_ErrorCode; }
  };

  class DisconnectReasonTLV : public ShortTLV {
   public:
    DisconnectReasonTLV() { }
    unsigned short Type() const { return TLV_DisconnectReason; }
  };

  class DisconnectMessageTLV : public StringTLV {
   public:
    DisconnectMessageTLV() { }
    unsigned short Type() const { return TLV_DisconnectMessage; }
  };

  class ScreenNameTLV : public StringTLV {
   public:
    ScreenNameTLV();
    ScreenNameTLV(const std::string& val);

    unsigned short Type() const { return TLV_Screenname; }
  };

  class PasswordTLV : public OutTLV {
   protected:
    std::string m_password;

    void OutputValue(Buffer& b) const;

   public:
    PasswordTLV(const std::string& pw);

    unsigned short Type() const { return TLV_Password; }
    unsigned short Length() const { return m_password.size(); }
  };

  const unsigned char ALLOWDIRECT_EVERYONE = 0x00;
  const unsigned char ALLOWDIRECT_AUTHORIZATION = 0x10;
  const unsigned char ALLOWDIRECT_CONTACTLIST = 0x20;

  const unsigned char WEBAWARE_NORMAL = 0x02;
  const unsigned char WEBAWARE_WEBAWARE = 0x03;

  class StatusTLV : public OutTLV, public InTLV {
   private:
    unsigned char m_allowDirect;
    unsigned char m_webAware;
    unsigned short m_status;

   protected:
    void OutputValue(Buffer& b) const;
    void ParseValue(Buffer& b);

   public:
    StatusTLV(unsigned char ad, unsigned char wa, unsigned short st)
      : m_allowDirect(ad), m_webAware(wa), m_status(st)
      { }
    StatusTLV() { }

    unsigned short Type() const { return TLV_Status; }
    unsigned short Length() const { return 4; }

    unsigned char getAllowDirect() { return m_allowDirect; }
    unsigned char getWebAware() { return m_webAware; }
    unsigned short getStatus() { return m_status; }
    bool getBirthday() { return ((m_webAware & 0x08) == 0x08); }
    
    void setAllowDirect(unsigned char m) { m_allowDirect = m; }
    void setWebAware(unsigned char m) { m_webAware = m; }
    void setStatus(unsigned short m) { m_status = m; }
  };

  // -- Client*TLVs --

  class ClientProfileTLV : public StringTLV {
   public:
    ClientProfileTLV(const std::string& val) : StringTLV(val) { }
    unsigned short Type() const { return TLV_ClientProfile; }
  };

  class ClientTypeTLV : public ShortTLV {
   public:
    ClientTypeTLV(unsigned short n) : ShortTLV(n) { }
    unsigned short Type() const { return TLV_ClientType; }
  };

  class ClientVersionMajorTLV : public ShortTLV {
   public:
    ClientVersionMajorTLV(unsigned short n) : ShortTLV(n) { }
    unsigned short Type() const { return TLV_ClientVersionMajor; }
  };

  class ClientVersionMinorTLV : public ShortTLV {
   public:
    ClientVersionMinorTLV(unsigned short n) : ShortTLV(n) { }
    unsigned short Type() const { return TLV_ClientVersionMinor; }
  };

  class ClientICQNumberTLV : public ShortTLV {
   public:
    ClientICQNumberTLV(unsigned short n) : ShortTLV(n) { }
    unsigned short Type() const { return TLV_ClientICQNumber; }
  };

  class ClientBuildMajorTLV : public ShortTLV {
   public:
    ClientBuildMajorTLV(unsigned short n) : ShortTLV(n) { }
    unsigned short Type() const { return TLV_ClientBuildMajor; }
  };

  class ClientBuildMinorTLV : public LongTLV {
   public:
    ClientBuildMinorTLV(unsigned int n) : LongTLV(n) { }
    unsigned short Type() const { return TLV_ClientBuildMinor; }
  };

  class CountryCodeTLV : public StringTLV {
   public:
    CountryCodeTLV(std::string val) : StringTLV(val) { }
    unsigned short Type() const { return TLV_CountryCode; }
  };

  class LanguageTLV : public StringTLV {
   public:
    LanguageTLV(const std::string& val) : StringTLV(val) { }
    unsigned short Type() const { return TLV_Language; }
  };

  // --

  class WebAddressTLV : public StringTLV {
   public:
    WebAddressTLV() { }
    unsigned short Type() const { return TLV_WebAddress; }
  };

  class UserClassTLV : public ShortTLV {
   public:
    UserClassTLV() { }
    unsigned short Type() const { return TLV_UserClass; }
  };

  class TimeOnlineTLV : public LongTLV {
   public:
    TimeOnlineTLV() { }
    unsigned short Type() const { return TLV_TimeOnline; }
  };

  class SignupDateTLV : public LongTLV {
   public:
    SignupDateTLV() { }
    unsigned short Type() const { return TLV_SignupDate; }
  };

  class SignonDateTLV : public LongTLV {
   public:
    SignonDateTLV() { }
    unsigned short Type() const { return TLV_SignonDate; }
  };

  class UnknownTLV : public ShortTLV {
   public:
    UnknownTLV() : ShortTLV(0) { }
    unsigned short Type() const { return TLV_Unknown; }
  };

  class IPAddressTLV : public LongTLV {
   public:
    IPAddressTLV() { }
    unsigned short Type() const { return TLV_IPAddress; }
  };

  class PortTLV : public ShortTLV {
   public:
    PortTLV() { }
    unsigned short Type() const { return TLV_Port; }
  };

  class UserInfoCapabilitiesTLV : public OutTLV {
   private:
    Capabilities m_capabilities;

   public:
    UserInfoCapabilitiesTLV();
    unsigned short Type() const { return TLV_UserInfoCapabilities; }
    unsigned short Length() const;

    void OutputValue(Buffer& b) const;
  };

  class CapabilitiesTLV : public InTLV {
   private:
    Capabilities m_capabilities;
    
   public:
    CapabilitiesTLV() { }
    unsigned short Type() const { return TLV_Capabilities; }
    unsigned short Length() const { return m_capabilities.get_length(); }

    Capabilities get_capabilities() const;

    void ParseValue(Buffer& b);
  };

  class RedirectTLV : public InTLV {
   private:
    std::string m_server;
    unsigned short m_port;

   public:
    RedirectTLV() { }

    unsigned short Length() const { return m_server.size(); }
    unsigned short Type() const { return TLV_Redirect; }

    void ParseValue(Buffer& b);

    std::string getHost() { return m_server; }
    unsigned short getPort() { return m_port; }
  };

  class CookieTLV : public InTLV, public OutTLV {
   private:
    unsigned char *m_value;
    unsigned short m_length;

   public:
    CookieTLV() : m_value(NULL), m_length(0) { }
    CookieTLV(const unsigned char *ck, unsigned short len);
    ~CookieTLV();
      
    unsigned short Length() const { return m_length; }
    unsigned short Type() const { return TLV_Cookie; }

    void ParseValue(Buffer& b);
    void OutputValue(Buffer& b) const;

    const unsigned char* Value() { return m_value; }
  };

  // can go out as well
  class LANDetailsTLV : public InTLV, public OutTLV {
   private:
    unsigned int m_lan_ip;
    unsigned short m_lan_port, m_firewall;
    unsigned char m_tcp_version;
    unsigned int m_dc_cookie;
    
   public:
    LANDetailsTLV();
    LANDetailsTLV(unsigned int ip, unsigned short port);

    unsigned short Length() const { return 0; } // varies
    unsigned short Type() const { return TLV_LANDetails; }

    unsigned int getLanIP() const { return m_lan_ip; }
    unsigned short getLanPort() const { return m_lan_port; }
    unsigned short getFirewall() const { return m_firewall; }
    unsigned char getTCPVersion() const { return m_tcp_version; }
    unsigned int getDCCookie() const { return m_dc_cookie; }

    void ParseValue(Buffer& b);
    void OutputValue(Buffer& b) const;
  };

  class RawTLV : public InTLV {
   protected:
    unsigned short m_type;
    unsigned short m_length;

   public:
    RawTLV(unsigned short type);

    unsigned short Type() const { return m_type; }
    unsigned short Length() const { return m_length; }
    void ParseValue(Buffer& b);
  };

  const unsigned short MESSAGETEXT_FLAG1_UCS2      = 0x02;
  const unsigned short MESSAGETEXT_FLAG1_ISO8859_1 = 0x03;

  class MessageTextTLV : public InTLV {
   protected:
    std::string m_message;
    unsigned short m_flag1, m_flag2;
    
   public:
    MessageTextTLV()
      : m_message(), m_flag1(0), m_flag2(0) { }

    std::string getMessage() { return m_message; }
    unsigned short getFlag1() { return m_flag1; }
    unsigned short getFlag2() { return m_flag1; }

    void ParseValue(Buffer& b);
    unsigned short Type() const { return TLV_MessageText; }
    unsigned short Length() const { return 0; }
  };

  class MessageDataTLV : public InTLV {
    MessageTextTLV mttlv;

   public:
    MessageDataTLV();

    std::string getMessage() { return mttlv.getMessage(); }
    unsigned short getFlag1() { return mttlv.getFlag1(); }
    unsigned short getFlag2() { return mttlv.getFlag2(); }

    void ParseValue(Buffer& b);
    unsigned short Type() const { return TLV_MessageData; }
    unsigned short Length() const { return 0; }
  };

  class AdvMsgBodyTLV : public InTLV {
   protected:
    ICQSubType *m_icqsubtype;
    
   public:
    AdvMsgBodyTLV();
    ~AdvMsgBodyTLV();

    ICQSubType* grabICQSubType();

    void ParseValue(Buffer& b);
    unsigned short Type() const { return TLV_AdvMsgBody; }
    unsigned short Length() const { return 0; }
  };

  class AdvMsgDataTLV : public InTLV {
    ICQSubType *m_icqsubtype;

   public:
    AdvMsgDataTLV();
    ~AdvMsgDataTLV();

    ICQSubType* grabICQSubType();

    void ParseValue(Buffer& b);
    unsigned short Type() const { return TLV_AdvMsgData; }
    unsigned short Length() const { return 0; }
  };

  // ============================================================================
  //  SBL TLVs
  // ============================================================================

  class SBLAwaitAuthTLV : public InTLV
  {
   public:
    SBLAwaitAuthTLV() { }

    unsigned short Type() const { return TLV_SBL_Await_Auth; }
    unsigned short Length() const { return 0; }
    void ParseValue(Buffer& b);
  };

  class SBLIdsTLV : public InTLV 
  {
   public:
    SBLIdsTLV() { }

    unsigned short Type() const { return TLV_SBL_Ids; }
    unsigned short Length() const { return 0; }
    void ParseValue(Buffer& b);
  };
  
  class SBLVisibilityTLV : public CharTLV 
  {
   public:
    SBLVisibilityTLV() { }
    unsigned short Type() const { return TLV_SBL_Visibility; }
  };
  
  class SBLICQTICTLV : public StringTLV 
  {
   public:
    SBLICQTICTLV() { }
    unsigned short Type() const { return TLV_SBL_ICQTIC; }
  };
  
  class SBLImportTimeTLV : public LongTLV 
  {
   public:
    SBLImportTimeTLV() { }
    unsigned short Type() const { return TLV_SBL_ImportTime; }
  };

  class SBLNickTLV : public StringTLV 
  {
   public:
    SBLNickTLV() { }
    unsigned short Type() const { return TLV_SBL_Nick; }
  };

  class SBLSMSNoTLV : public StringTLV 
  {
   public:
    SBLSMSNoTLV() { }
    unsigned short Type() const { return TLV_SBL_SMS_No; }
  };

  // --------------- ICQDataTLV ------------------

  class ICQDataTLV : public InTLV {
   private:
    ICQSubType *m_icqsubtype;

   public:
    ICQDataTLV();
    ~ICQDataTLV();

    ICQSubType* getICQSubType() const;
    ICQSubType* grabICQSubType();

    void ParseValue(Buffer& b);
    unsigned short Type() const { return TLV_ICQData; }
    unsigned short Length() const { return 0; }

  };

  // ---------------- TLV List -------------------

  class TLVList {
   private:
    std::map<unsigned short,InTLV*> tlvmap;
   public:
    TLVList();
    ~TLVList();

    void Parse(Buffer& b, TLV_ParseMode pm, unsigned short no_tlvs);
    void ParseByLength(Buffer& b, TLV_ParseMode pm, unsigned int len);

    bool exists(unsigned short type);
    InTLV* & operator[](unsigned short type);

  };

  Buffer& operator<<(Buffer& b, const ICQ2000::OutTLV& t);

}

#endif
