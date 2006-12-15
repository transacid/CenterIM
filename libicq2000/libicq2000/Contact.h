/*
 * Contact (model)
 * A contact on the contact list
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

#ifndef CONTACT_H
#define CONTACT_H

#include <list>
#include <string>

#include <libicq2000/sigslot.h>

#include <libicq2000/constants.h>
#include <libicq2000/userinfoconstants.h>

#include <libicq2000/ref_ptr.h>

namespace ICQ2000
{
  /* predeclare classes */
  class Capabilities;
  class MessageEvent;
  class StatusChangeEvent;
  class UserInfoChangeEvent;

  // -- Status Codes Flags --
  const unsigned short STATUS_FLAG_ONLINE = 0x0000;
  const unsigned short STATUS_FLAG_AWAY = 0x0001;
  const unsigned short STATUS_FLAG_DND = 0x0002;
  const unsigned short STATUS_FLAG_NA = 0x0004;
  const unsigned short STATUS_FLAG_OCCUPIED = 0x0010;
  const unsigned short STATUS_FLAG_FREEFORCHAT = 0x0020;
  const unsigned short STATUS_FLAG_INVISIBLE = 0x0100;

  class Contact
  {
   public:
    // reference count
    unsigned int count;

    // Inner classes for various sections of Contact details

    class MainHomeInfo
    {
      std::string cellular, normalised_cellular;
      // cellular private - access must be through
      // get/setMobileNo for consistency
    
      void normaliseMobileNo();

    public:
      MainHomeInfo();

      std::string alias, firstname, lastname, email, city, state, phone, fax, street, zip;
      Country  country;
      Timezone timezone;

      Country getCountry() const;
      std::string getMobileNo() const;
      void setMobileNo(const std::string& s);

      std::string getNormalisedMobileNo() const;
    };

    class HomepageInfo
    {
    public:
      HomepageInfo();

      unsigned char age;
      Sex sex;
      Language lang1, lang2, lang3;

      std::string homepage;
      unsigned short birth_year;
      unsigned char birth_month, birth_day;

      std::string getBirthDate() const;
      Language getLanguage(int l) const;
    };

    class EmailInfo
    {
    public:
      typedef std::list<std::string> EmailList;
      EmailList emails;

      EmailInfo();

      void addEmailAddress(const std::string&);
    };
  
    class WorkInfo
    {
    public:
      WorkInfo();
    
      std::string city, state, street, zip;
      unsigned short country;
      std::string company_name, company_dept, company_position, company_web;
    };

    class BackgroundInfo
    {
    public:
      typedef std::pair<unsigned short, std::string> School;
      typedef std::list<School> SchoolList;
      SchoolList schools;   // school names

      BackgroundInfo();

      void addSchool(unsigned short cat, const std::string& s);
    };

    class PersonalInterestInfo
    {
     public:
      typedef std::pair<unsigned short, std::string> Interest;
      typedef std::list<Interest> InterestList;
      InterestList interests;

      PersonalInterestInfo();
    
      void addInterest(unsigned short cat, const std::string& s);
    };

  private:
    void Init();
    bool m_icqcontact;
    bool m_virtualcontact;

    // static fields
    unsigned int m_uin;

    // dynamic fields - updated when they come online
    unsigned char m_tcp_version;
    unsigned int m_dc_cookie;
    Status m_status;
    bool m_invisible;
    bool m_authreq, m_authawait;
    bool m_direct;
    bool m_bday;    // Birthday Flag
    bool m_server_based;
    unsigned int m_ext_ip, m_lan_ip;
    unsigned short m_ext_port, m_lan_port, m_group_id, m_tag_id;
    Capabilities * m_capabilities;
    unsigned int m_signon_time, m_last_online_time, m_last_status_change_time;
    unsigned int m_last_message_time, m_last_away_msg_check_time;

    static unsigned int imag_uin;
    
    // other fields
    unsigned short m_seqnum;

    // detailed fields
    MainHomeInfo m_main_home_info;
    HomepageInfo m_homepage_info;
    EmailInfo m_email_info;
    WorkInfo m_work_info;
    PersonalInterestInfo m_personal_interest_info;
    BackgroundInfo m_background_info;
    std::string m_about;

  public:
    Contact();

    Contact(unsigned int uin);
    Contact(const std::string& a);

    ~Contact();

    unsigned int getUIN() const;
    void setUIN(unsigned int uin);
    std::string getStringUIN() const;
    std::string getMobileNo() const;
    std::string getNormalisedMobileNo() const;
    std::string getAlias() const;
    std::string getFirstName() const;
    std::string getLastName() const;
    std::string getEmail() const;

    std::string getNameAlias() const;

    Status getStatus() const;
    bool isInvisible() const;
    bool getAuthReq() const;
    bool getAuthAwait() const;
    bool getServerBased() const;

    unsigned int getExtIP() const;
    unsigned int getLanIP() const;
    unsigned short getExtPort() const;
    unsigned short getLanPort() const;
    unsigned char getTCPVersion() const;
    unsigned int getDCCookie() const;
    bool get_accept_adv_msgs() const;
    Capabilities get_capabilities() const;

    unsigned int get_signon_time() const;
    unsigned int get_last_online_time() const;
    unsigned int get_last_status_change_time() const;
    unsigned int get_last_message_time() const;
    unsigned int get_last_away_msg_check_time() const;

    void setMobileNo(const std::string& mn);
    void setAlias(const std::string& al);
    void setFirstName(const std::string& fn);
    void setLastName(const std::string& ln);
    void setEmail(const std::string& em);
    void setAuthReq(bool b);
    void setAuthAwait(bool b);
    void setServerBased(bool b);

    bool getDirect() const;
    void setDirect(bool b);

    bool getBirthday() const;
    void setBirthday(bool b);
    
    void setStatus(Status st, bool i);
    void setStatus(Status st);
    void setInvisible(bool i);
    void setExtIP(unsigned int ip);
    void setLanIP(unsigned int ip);
    void setExtPort(unsigned short port);
    void setLanPort(unsigned short port);
    void setTCPVersion(unsigned char v);
    void setDCCookie(unsigned int cookie);
    void set_capabilities(const Capabilities& c);

    void setServerSideInfo(unsigned short group_id, unsigned short tag_id);
    unsigned short getServerSideGroupID() const;
    unsigned short getServerSideID() const;

    void set_signon_time(unsigned int t);
    void set_last_online_time(unsigned int t);
    void set_last_status_change_time(unsigned int t);
    void set_last_message_time(unsigned int t);
    void set_last_away_msg_check_time(unsigned int t);

    void setMainHomeInfo(const MainHomeInfo& m);
    void setHomepageInfo(const HomepageInfo& s);
    void setEmailInfo(const EmailInfo &e);
    void setWorkInfo(const WorkInfo &w);
    void setInterestInfo(const PersonalInterestInfo& p);
    void setBackgroundInfo(const BackgroundInfo& b);
    void setAboutInfo(const std::string& about);

    MainHomeInfo& getMainHomeInfo();
    HomepageInfo& getHomepageInfo();
    EmailInfo& getEmailInfo();
    WorkInfo& getWorkInfo();
    BackgroundInfo& getBackgroundInfo();
    PersonalInterestInfo& getPersonalInterestInfo();
    const std::string& getAboutInfo() const;

    bool isICQContact() const;
    bool isVirtualContact() const;

    bool isSMSable() const;

    unsigned short nextSeqNum();

    sigslot::signal1<StatusChangeEvent*> status_change_signal;
    sigslot::signal1<UserInfoChangeEvent*> userinfo_change_signal;

    void userinfo_change_emit();
    void userinfo_change_emit(bool is_transient_detail);

    static std::string UINtoString(unsigned int uin);
    static unsigned int StringtoUIN(const std::string& s);
    
    static unsigned short MapStatusToICQStatus(Status st, bool inv);
    static Status MapICQStatusToStatus(unsigned short st);
    static bool MapICQStatusToInvisible(unsigned short st);

    static unsigned int nextImaginaryUIN();
  };

  typedef ref_ptr<Contact> ContactRef;
}

#endif
