/*
 * SNAC - Server
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

#include "SNAC-SRV.h"

#include <memory>
#include "sstream_fix.h"

#include "TLV.h"
#include "Xml.h"

#include "time_extra.h"

using std::string;
using std::auto_ptr;
using std::istringstream;

namespace ICQ2000 {

  // --------------------- Server (Family 0x0015) SNACs ---------

  SrvSendSNAC::SrvSendSNAC(const string& text, const string& destination,
			   unsigned int senders_UIN, const string& senders_name, bool delrpt)
    : m_text(text), m_destination(destination), m_senders_name(senders_name),
       m_senders_UIN(senders_UIN), m_delivery_receipt(delrpt) { }

  void SrvSendSNAC::OutputBody(Buffer& b) const {

    /*
     * Sending SMS messages
     * This is the biggest hodge-podge of a mess you
     * could imagine in a protocol, a mix of Big and Little Endian,
     * AIM TLVs and ICQ binary data and add in some XML
     * to top it all off. :-)
     */
    
    XmlBranch xmltree("icq_sms_message");
    xmltree.pushnode(new XmlLeaf("destination",m_destination));
    xmltree.pushnode(new XmlLeaf("text",m_text));
    xmltree.pushnode(new XmlLeaf("codepage","1252"));
    xmltree.pushnode(new XmlLeaf("senders_UIN",Contact::UINtoString(m_senders_UIN)));
    xmltree.pushnode(new XmlLeaf("senders_name",m_senders_name));
    xmltree.pushnode(new XmlLeaf("delivery_receipt",(m_delivery_receipt ? "Yes" : "No")));

    /* Time string, format: Wkd, DD Mnm YYYY HH:MM:SS TMZ */
    char timestr[30];
    time_t t;
    struct tm *tm;
    time(&t);
    tm = gmtime(&t);
    strftime(timestr, 30, "%a, %d %b %Y %T %Z", tm);
    xmltree.pushnode(new XmlLeaf("time",string(timestr)));

    string xmlstr = xmltree.toString(0);

    // this is a TLV header
    b << (unsigned short)0x0001;
    b << (unsigned short)(xmlstr.size()+37);

    b.setLittleEndian();
    b << (unsigned short)(xmlstr.size()+35);
    b << m_senders_UIN;

    // think this is the message type
    b << (unsigned short)2000;

    // think this is a request id of sorts
    b << (unsigned short)m_requestID; /* low word of the request ID */

    b.setBigEndian();

    // SMS send subtype
    b << (unsigned short)0x8214;

    // not sure about what this means
    b << (unsigned short)0x0001;
    b << (unsigned short)0x0016;
    for(int a = 0; a < 16; a++)
      b << (unsigned char)0x00;

    b << (unsigned short)0x0000;
    b.PackUint16StringNull(xmlstr);
  }

  SrvRequestOfflineSNAC::SrvRequestOfflineSNAC(unsigned int uin)
    : m_uin(uin) { }

  void SrvRequestOfflineSNAC::OutputBody(Buffer& b) const {
    // this is a TLV header
    b << (unsigned short)0x0001
      << (unsigned short)0x000a;

    b.setLittleEndian();
    b << (unsigned short)0x0008;
    b << m_uin;

    // message type
    b << (unsigned short)60;
    // a request id
    b << (unsigned short)m_requestID; /* low word of the request ID */

  }

  SrvAckOfflineSNAC::SrvAckOfflineSNAC(unsigned int uin)
    : m_uin(uin) { }

  void SrvAckOfflineSNAC::OutputBody(Buffer& b) const {
    // this is a TLV header
    b << (unsigned short)0x0001
      << (unsigned short)0x000a;

    b.setLittleEndian();
    b << (unsigned short)0x0008;
    b << m_uin;

    // message type
    b << (unsigned short)62;
    // a request id
    b << (unsigned short)m_requestID; /* low word of the request ID */

  }

  SrvRequestSimpleUserInfo::SrvRequestSimpleUserInfo(unsigned int my_uin, unsigned int user_uin)
    : m_my_uin(my_uin), m_user_uin(user_uin) { }

  void SrvRequestSimpleUserInfo::OutputBody(Buffer& b) const {
    b << (unsigned short)0x0001
      << (unsigned short)0x0010;

    b.setLittleEndian();
    b << (unsigned short)0x000e;
    b << m_my_uin;

    b << (unsigned short)2000	/* type 9808 */
      << (unsigned short)m_requestID /* low word of the request ID */
      << (unsigned short)1311	/* subtype (unsigned short)1311 */
      << m_user_uin;
    
  }

  SrvRequestShortWP::SrvRequestShortWP
  (unsigned int my_uin, const string& nickname,
   const string& firstname, const string& lastname)

    : m_my_uin(my_uin), m_nickname(nickname),
      m_firstname(firstname), m_lastname(lastname)
  { }


  void SrvRequestShortWP::OutputBody(Buffer& b) const {
    b << (unsigned short)0x0001;

    Buffer::marker m1 = b.getAutoSizeShortMarker();

    b.setLittleEndian();
    Buffer::marker m2 = b.getAutoSizeShortMarker();

    b << m_my_uin;

    b << (unsigned short)2000	/* type 9808 */
      << (unsigned short)m_requestID /* low word of the request ID */
      << (unsigned short)0x0515;	/* subtype wp-short-request */
    b.PackUint16StringNull(m_firstname);
    b.PackUint16StringNull(m_lastname);
    b.PackUint16StringNull(m_nickname);

    b.setAutoSizeMarker(m1);
    b.setAutoSizeMarker(m2);
  }


  SrvRequestFullWP::SrvRequestFullWP
  (unsigned int my_uin, const string& nickname,
   const string& firstname,  const string& lastname,
   const string& email, unsigned short min_age, unsigned short max_age,
   unsigned char sex, unsigned char language, const string& city, const string& state,
   unsigned short country, const string& company_name, const string& department,
   const string& position, bool only_online)
  
    : m_my_uin(my_uin), m_nickname(nickname),
      m_firstname(firstname), m_lastname(lastname),
      m_email(email), m_min_age(min_age), m_max_age(max_age),
      m_sex(sex), m_language(language), m_city(city), m_state(state),
      m_company_name(company_name), m_department(department), m_position(position),
      m_country(country), m_only_online(only_online)

  { }

  void SrvRequestFullWP::OutputBody(Buffer& b) const {
    b << (unsigned short)0x0001;

    Buffer::marker m1 = b.getAutoSizeShortMarker();

    b.setLittleEndian();
    Buffer::marker m2 = b.getAutoSizeShortMarker();

    b << m_my_uin;

    b << (unsigned short)2000	/* type 9808 */
      << (unsigned short)m_requestID /* low word of the request ID */
      << (unsigned short)0x0533;	/* subtype wp-full-request */
    b.PackUint16StringNull(m_firstname);
    b.PackUint16StringNull(m_lastname);
    b.PackUint16StringNull(m_nickname);
    b.PackUint16StringNull(m_email);
    b << (unsigned short)m_min_age;		// minimum age
    b << (unsigned short)m_max_age;		// maximum age
    b << (unsigned char)m_sex;			// sex
    b << (unsigned char)m_language;             // language
    b.PackUint16StringNull(m_city);             // city
    b.PackUint16StringNull(m_state);            // state
    b << (unsigned short)m_country;             // country
    b.PackUint16StringNull(m_company_name);	// company name
    b.PackUint16StringNull(m_department);	// department
    b.PackUint16StringNull(m_position);		// position
    b << (unsigned char)0x00;			// occupation
    b << (unsigned short)0x0000;		// past info category
    b.PackUint16StringNull("");			//           description
    b << (unsigned short)0x0000;		// interests category
    b.PackUint16StringNull("");			//           description
    b << (unsigned short)0x0000;		// affiliation/organization
    b.PackUint16StringNull("");			//           description
    b << (unsigned short)0x0000;		// homepage category
    b.PackUint16StringNull("");			//           description
    b << (unsigned char)(m_only_online ? 0x01 : 0x00);
                                                // only-online flag
    b.setAutoSizeMarker(m1);
    b.setAutoSizeMarker(m2);
  }

  SrvRequestKeywordSearch::SrvRequestKeywordSearch(unsigned int my_uin, const string& keyword)
    : m_my_uin(my_uin), m_keyword(keyword)
  { }
  
  void SrvRequestKeywordSearch::OutputBody(Buffer& b) const
  { 
    b << (unsigned short)0x0001;

    Buffer::marker m1 = b.getAutoSizeShortMarker();

    b.setLittleEndian();
    Buffer::marker m2 = b.getAutoSizeShortMarker();

    b << m_my_uin;

    b << (unsigned short)2000	     /* type 9808 */
      << (unsigned short)m_requestID /* low word of the request ID */
      << (unsigned short)0x055F      /* subtype keyword-search */
      << (unsigned short)0x0226;     /* unknown */

    Buffer::marker m3 = b.getAutoSizeShortMarker();
    b.PackUint16StringNull(m_keyword);
    b.setAutoSizeMarker(m3);
    
    b.setAutoSizeMarker(m1);
    b.setAutoSizeMarker(m2);
  }
  
  SrvRequestDetailUserInfo::SrvRequestDetailUserInfo(unsigned int my_uin, unsigned int user_uin)
    : m_my_uin(my_uin), m_user_uin(user_uin) { }

  void SrvRequestDetailUserInfo::OutputBody(Buffer& b) const {
    b << (unsigned short)0x0001
      << (unsigned short)0x0010;

    b.setLittleEndian();
    b << (unsigned short)0x000e;
    b << m_my_uin;

    b << (unsigned short)2000   /* type 9808 */
      << (unsigned short)m_requestID /* low word of the request ID */
      << (unsigned short)0x04b2 /* subtype (unsigned short)1311 */
      << m_user_uin;
   
  }

  SrvUpdateMainHomeInfo::SrvUpdateMainHomeInfo(unsigned int my_uin, const Contact::MainHomeInfo& main_home_info)
    : m_my_uin(my_uin), m_main_home_info(main_home_info) { }
    
  void SrvUpdateMainHomeInfo::OutputBody(Buffer& b) const {
        b << (unsigned short)0x0001;

        Buffer::marker m1 = b.getAutoSizeShortMarker();
    
        b.setLittleEndian();
        Buffer::marker m2 = b.getAutoSizeShortMarker();

        b << m_my_uin;

        b << (unsigned short)2000   /* type 9808 */
            << (unsigned short)m_requestID /* low word of the request ID */
            << (unsigned short)0x03ea; /* subtype */
        b.PackUint16StringNull(m_main_home_info.alias);     // alias
        b.PackUint16StringNull(m_main_home_info.firstname); // first name
        b.PackUint16StringNull(m_main_home_info.lastname);  // last name
        b.PackUint16StringNull(m_main_home_info.email);	    // email
        b.PackUint16StringNull(m_main_home_info.city);	    // city
        b.PackUint16StringNull(m_main_home_info.state);     // state
        b.PackUint16StringNull(m_main_home_info.phone);     // phone
        b.PackUint16StringNull(m_main_home_info.fax);       // fax
        b.PackUint16StringNull(m_main_home_info.street);    // street
        b.PackUint16StringNull(m_main_home_info.getMobileNo());  // cellular
        b.PackUint16StringNull(m_main_home_info.zip);       // zip
        b << m_main_home_info.country;
        b << m_main_home_info.timezone;
        unsigned char publish_email = 0;
        b << publish_email;
        b.setAutoSizeMarker(m1);
        b.setAutoSizeMarker(m2);
    }
    
  SrvUpdateWorkInfo::SrvUpdateWorkInfo(unsigned int my_uin, const Contact::WorkInfo& work_info)
    : m_my_uin(my_uin), m_work_info(work_info) { }
    
  void SrvUpdateWorkInfo::OutputBody(Buffer& b) const {
        b << (unsigned short)0x0001;

        Buffer::marker m1 = b.getAutoSizeShortMarker();
    
        b.setLittleEndian();
        Buffer::marker m2 = b.getAutoSizeShortMarker();

        b << m_my_uin;

        b << (unsigned short)2000   /* type 9808 */
            << (unsigned short)m_requestID /* low word of the request ID */
            << (unsigned short)0x03f3; /* subtype */
        b.PackUint16StringNull(m_work_info.city);     	    // city
        b.PackUint16StringNull(m_work_info.state);     	    // state
        b << (unsigned short)0x0000
          << (unsigned short)0x0000;
        b.PackUint16StringNull(m_work_info.street);         // street
        b.PackUint16StringNull(m_work_info.zip);     	    // zip
        b << m_work_info.country;			    // country-code
        b.PackUint16StringNull(m_work_info.company_name);   // company: name
        b.PackUint16StringNull(m_work_info.company_dept);   // company: department
        b.PackUint16StringNull(m_work_info.company_position); // company: position
        b << (unsigned short)0x0000;
        b.PackUint16StringNull(m_work_info.company_web);    // company: homepage
        b.setAutoSizeMarker(m1);
        b.setAutoSizeMarker(m2);
    }
    
  SrvUpdateHomepageInfo::SrvUpdateHomepageInfo(unsigned int my_uin, const Contact::HomepageInfo& homepage_info)
    : m_my_uin(my_uin), m_homepage_info(homepage_info) { }
    
  void SrvUpdateHomepageInfo::OutputBody(Buffer& b) const {
        b << (unsigned short)0x0001;

        Buffer::marker m1 = b.getAutoSizeShortMarker();
    
        b.setLittleEndian();
        Buffer::marker m2 = b.getAutoSizeShortMarker();

        b << m_my_uin;

        b << (unsigned short)2000   /* type 9808 */
            << (unsigned short)m_requestID /* low word of the request ID */
            << (unsigned short)0x03fd; /* subtype */
        b << (unsigned char)m_homepage_info.age;	    // age
        b << (unsigned char)0x00;
        b << (unsigned char)m_homepage_info.sex;	    // sex-code
        b.PackUint16StringNull(m_homepage_info.homepage);
        b << (unsigned short)m_homepage_info.birth_year;
        b << (unsigned char)m_homepage_info.birth_month;
        b << (unsigned char)m_homepage_info.birth_day;
        b << (unsigned char)m_homepage_info.lang1;
        b << (unsigned char)m_homepage_info.lang2;
        b << (unsigned char)m_homepage_info.lang3;
        b.setAutoSizeMarker(m1);
        b.setAutoSizeMarker(m2);
    }
    
  SrvUpdateAboutInfo::SrvUpdateAboutInfo(unsigned int my_uin, const string& about_info)
    : m_my_uin(my_uin), m_about_info(about_info) { }
    
  void SrvUpdateAboutInfo::OutputBody(Buffer& b) const {
        b << (unsigned short)0x0001;

        Buffer::marker m1 = b.getAutoSizeShortMarker();
    
        b.setLittleEndian();
        Buffer::marker m2 = b.getAutoSizeShortMarker();

        b << m_my_uin;

        b << (unsigned short)2000   /* type 9808 */
            << (unsigned short)m_requestID /* low word of the request ID */
            << (unsigned short)0x0406; /* subtype */
        b.PackUint16StringNull(m_about_info);
        b.setAutoSizeMarker(m1);
        b.setAutoSizeMarker(m2);
    }
    
  SrvRequestRandomChat::SrvRequestRandomChat(unsigned int my_uin, unsigned short random_group)
    : m_my_uin(my_uin), m_random_group(random_group) { }
    
  void SrvRequestRandomChat::OutputBody(Buffer& b) const {
	b << (unsigned short)0x0001;

	Buffer::marker m1 = b.getAutoSizeShortMarker();
    
	b.setLittleEndian();
	Buffer::marker m2 = b.getAutoSizeShortMarker();

	b << m_my_uin;

	b << (unsigned short)2000   /* type 9808 */
	    << (unsigned short)m_requestID /* low word of the request ID */
	    << (unsigned short)0x074E; /* subtype */

	b << (unsigned short)m_random_group;
	b.setAutoSizeMarker(m1);
	b.setAutoSizeMarker(m2);
    }
    
  SrvSetRandomChatGroup::SrvSetRandomChatGroup(unsigned int my_uin, unsigned short random_group)
    : m_my_uin(my_uin), m_random_group(random_group) { }
    
  void SrvSetRandomChatGroup::OutputBody(Buffer& b) const {
	int i;
	b << (unsigned short)0x0001;

	Buffer::marker m1 = b.getAutoSizeShortMarker();
    
	b.setLittleEndian();
	Buffer::marker m2 = b.getAutoSizeShortMarker();

	b << m_my_uin;

	b << (unsigned short)2000   /* type 9808 */
	    << (unsigned short)m_requestID /* low word of the request ID */
	    << (unsigned short)0x0758; /* subtype */

	b << (unsigned short)m_random_group;

	b << (unsigned short) 0x0000;
	b << (unsigned short) 0x0220;
	for(i = 0; i < 6; i++) b << (unsigned short) 0x0000;
	b << (char) 0x0;
	for(i = 0; i < 3; i++) b << (unsigned short) 0x0000;
	b << (char) 0x50;
	for(i = 0; i < 3; i++) b << (char) 0x0;
	b << (char) 0x03;
	for(i = 0; i < 3; i++) b << (char) 0x0;

	b.setAutoSizeMarker(m1);
	b.setAutoSizeMarker(m2);
    }
    
  SrvResponseSNAC::SrvResponseSNAC() : m_icqsubtype(NULL) { }

  SrvResponseSNAC::~SrvResponseSNAC() {
    if (m_icqsubtype != NULL) delete m_icqsubtype;
  }

  void SrvResponseSNAC::ParseBody(Buffer& b) {

    /* It is worth making the distinction between
     * sms responses and sms delivery responses
     * - an sms response is sent on this channel always
     *   after an sms is sent
     * - an sms delivery response is sent from the mobile
     *   if requested and arrives as SNAC 0x0004 0x0007
     */

    // a TLV header
    unsigned short type, length;
    b >> type;
    b >> length;
    
    b.setLittleEndian();
    // the length again in little endian
    b >> length;

    unsigned int uin;
    b >> uin;

    /* Command type:
     * 65 (dec) = An Offline message
     * 66 (dec) = Offline Messages Finish
     * 2010 (dec) = ICQ Extended response
     * others.. ??
     */
    unsigned short command_type;
    b >> command_type;

    unsigned short request_id;
    b >> request_id;

    if (command_type == 65) {
      ParseOfflineMessage(b);
    } else if (command_type == 66) {
      m_type = OfflineMessagesComplete;
      unsigned char waste_char;
      b >> waste_char;
    } else if (command_type == 2010) {
      ParseICQResponse(b);
    } else {
      throw ParseException("Unknown command type for Server Response SNAC");
    }

  }

  void SrvResponseSNAC::ParseOfflineMessage(Buffer& b) {
    b >> m_sender_UIN;
    unsigned short year;
    unsigned char month, day, hour, minute;
    b >> year
      >> month
      >> day
      >> hour
      >> minute;

    struct tm timetm;
    timetm.tm_sec = 0;
    timetm.tm_min = minute;
    timetm.tm_hour = hour;
    timetm.tm_mday = day;
    timetm.tm_mon = month-1;
    timetm.tm_year = year-1900;
    timetm.tm_isdst = 0;

    m_time = gmt_mktime(&timetm);

    m_type = OfflineMessage;
    m_icqsubtype = ICQSubType::ParseICQSubType(b, false, false);
    /* offline message is non-advanced, and not an ack */
    b.advance(2); // unknown

    if (m_icqsubtype != NULL && dynamic_cast<UINICQSubType*>(m_icqsubtype) != NULL) {
      UINICQSubType *ust = dynamic_cast<UINICQSubType*>(m_icqsubtype);
      ust->setSource( m_sender_UIN );
    }
  }

  void SrvResponseSNAC::ParseICQResponse(Buffer& b) {

    unsigned short subtype;
    b >> subtype;

    switch(subtype) {
    case SrvResponse_Error:
      ParseSMSError(b); // fix
      break;
    case SrvResponse_SMS_Done:
      ParseSMSResponse(b);
      break;
    case SrvResponse_SimpleUI:
    case SrvResponse_SimpleUI_Done:
    case SrvResponse_SearchUI:
    case SrvResponse_SearchUI_Done:
      ParseSimpleUserInfo(b, subtype);
      break;
    case SrvResponse_MainHomeInfo:
    case SrvResponse_WorkInfo:
    case SrvResponse_HomePageInfo:
    case SrvResponse_AboutInfo:
    case SrvResponse_EmailInfo:
    case SrvResponse_InterestInfo:
    case SrvResponse_BackgroundInfo:
    case SrvResponse_Unknown:
      ParseDetailedUserInfo(b, subtype);
      break;
    case SrvResponse_AckMainHomeInfoChange:
    case SrvResponse_AckWorkInfoChange:
    case SrvResponse_AckHomepageInfoChange:
    case SrvResponse_AckAboutInfoChange:
      ParseInfoChangeAck(b, subtype);
      break;
    case SrvResponse_RandomChatFound:
      ParseRandomChatFound(b);
      break;
    case SrvResponse_AckRandomGroupSet:
      ParseRandomGroupSetAck(b);
      break;
    default:
      throw ParseException("Unknown ICQ subtype for Server response SNAC");
    }
    

  }

  void SrvResponseSNAC::ParseInfoChangeAck(Buffer &b, unsigned short subtype) {
    switch(subtype)
    {
    case SrvResponse_AckMainHomeInfoChange:
      m_type = AckMainHomeInfoChange;
      break;
    case SrvResponse_AckWorkInfoChange:
      m_type = AckWorkInfoChange;
      break;
    case SrvResponse_AckHomepageInfoChange:
      m_type = AckHomepageInfoChange;
      break;
    case SrvResponse_AckAboutInfoChange:
      m_type = AckAboutInfoChange;
      break;
    default:
      throw ParseException("Unknown info change acknowledgment");
    }
    
    b.advance(1); // there's a 0x0a there
    
    if (b.beforeEnd())
      throw ParseException("Extra data at info change acknowledgment (could be an SMS response)");
  }

  void SrvResponseSNAC::ParseSMSError(Buffer& b) {
    m_type = SMS_Error;
    // to do - maybe?
  }

  void SrvResponseSNAC::ParseSMSResponse(Buffer& b) {
    /* Not sure what the difference between 100 and 150 is
     * when successful it sends the erroneous 100 and then 150
     * otherwise only 150 I think
     */
    m_type = SMS_Response;

    /* Don't know what the next lot of data
       * means:
       * 0a 00 01 00 08 00 01
       */
    unsigned char waste_char;
    for (int a = 0; a < 7; a++)
      b >> waste_char;

    b.setBigEndian();
    string tag;
    b >> tag;

    string xmlstr;
    b >> xmlstr;

    string::iterator s = xmlstr.begin();
    auto_ptr<XmlNode> top(XmlNode::parse(s, xmlstr.end()));
    
    if (top.get() == NULL) throw ParseException("Couldn't parse xml data in Server Response SNAC");

    if (top->getTag() != "sms_response") throw ParseException("No <sms_response> tag found in xml data");
    XmlBranch *sms_response = dynamic_cast<XmlBranch*>(top.get());
    if (sms_response == NULL) throw ParseException("No tags found in xml data");

    XmlLeaf *source = sms_response->getLeaf("source");
    if (source != NULL) m_source = source->getValue();

    XmlLeaf *deliverable = sms_response->getLeaf("deliverable");
    m_deliverable = false;
    m_smtp_deliverable = false;
    
    if (deliverable != NULL) {
      if (deliverable->getValue() == "Yes") m_deliverable = true;
      if (deliverable->getValue() == "SMTP") {
	m_deliverable = false;
	m_smtp_deliverable = true;
      }
    }

    if (m_deliverable) {
      // -- deliverable = Yes --

      XmlLeaf *network = sms_response->getLeaf("network");
      if (network != NULL) m_network = network->getValue();

      XmlLeaf *message_id = sms_response->getLeaf("message_id");
      if (message_id != NULL) m_message_id = message_id->getValue();

      XmlLeaf *messages_left = sms_response->getLeaf("messages_left");
      if (messages_left != NULL) m_messages_left = messages_left->getValue();
      // always 0, unsurprisingly

    } else if (m_smtp_deliverable) {
      // -- deliverable = SMTP --
      
      XmlLeaf *smtp_from = sms_response->getLeaf("from");
      if (smtp_from != NULL) m_smtp_from = smtp_from->getValue();

      XmlLeaf *smtp_to = sms_response->getLeaf("to");
      if (smtp_to != NULL) m_smtp_to = smtp_to->getValue();

      XmlLeaf *smtp_subject = sms_response->getLeaf("subject");
      if (smtp_subject != NULL) m_smtp_subject = smtp_subject->getValue();

    } else {
      // -- deliverable = No --

      // should be an <error> tag
      XmlBranch *error = sms_response->getBranch("error");
      if (error != NULL) {
	// should be an <id> tag
	XmlLeaf *error_id = error->getLeaf("id");
	if (error_id != NULL) {
	  // convert error id to a number
	  istringstream istr(error_id->getValue());
	  m_error_id = 0;
	  istr >> m_error_id;
	}

	// should also be a <params> branch
	XmlBranch *params = error->getBranch("params");
	if (params != NULL) {
	  // assume only one <param> tag
	  XmlLeaf *param = params->getLeaf("param");
	  if (param != NULL) m_error_param = param->getValue();
	}
      } // end <error> tag


    } // end deliverable = No


  }

  void SrvResponseSNAC::ParseDetailedUserInfo(Buffer& b, unsigned short subtype) {
    unsigned char wb;
    b >> wb; // status code ?

    switch(subtype) {
    case SrvResponse_MainHomeInfo:
    {
      string s;
      b.UnpackUint16StringNull(m_main_home_info.alias);     // alias
      b.UnpackUint16StringNull(m_main_home_info.firstname); // first name
      b.UnpackUint16StringNull(m_main_home_info.lastname);  // last name
      b.UnpackUint16StringNull(m_main_home_info.email);	    // email
      b.UnpackUint16StringNull(m_main_home_info.city);	    // city
      b.UnpackUint16StringNull(m_main_home_info.state);     // state
      b.UnpackUint16StringNull(m_main_home_info.phone);     // phone
      b.UnpackUint16StringNull(m_main_home_info.fax);       // fax
      b.UnpackUint16StringNull(m_main_home_info.street);    // street
      b.UnpackUint16StringNull(s);  // cellular
      m_main_home_info.setMobileNo(s);
      b.UnpackUint16StringNull(m_main_home_info.zip);       // zip
      unsigned short country;
      b >> country;
      m_main_home_info.country = Country(country);
      signed char timezone;
      b >> timezone;
      m_main_home_info.timezone = Timezone(timezone);
      b.advance(1);

      // some end marker?
      unsigned short wi;
      b >> wi;

      m_type = RMainHomeInfo;
      break;
    }
    case SrvResponse_HomePageInfo:
    {
      b >> m_homepage_info.age;
      unsigned char unk;
      b >> unk;
      unsigned char sex;
      b >> sex;
      m_homepage_info.sex = Sex(sex);
      b.UnpackUint16StringNull(m_homepage_info.homepage);
      b >> m_homepage_info.birth_year;
      b >> m_homepage_info.birth_month;
      b >> m_homepage_info.birth_day;
      unsigned char lang1, lang2, lang3;
      b >> lang1;
      m_homepage_info.lang1 = Language(lang1);
      b >> lang2;
      m_homepage_info.lang2 = Language(lang2);
      b >> lang3;
      m_homepage_info.lang3 = Language(lang3);
      b >> wb;
      b >> wb;
      m_type = RHomepageInfo;
      break;
    }
    case SrvResponse_EmailInfo:
    {
      unsigned char n;
      b >> n;
      while(n > 0) {
	string s;
	b.UnpackUint16StringNull(s);
	m_email_info.addEmailAddress(s);
	--n;
      }
      m_type = REmailInfo;
      break;
    }
    case SrvResponse_WorkInfo:
    {
      b.UnpackUint16StringNull(m_work_info.city);
      b.UnpackUint16StringNull(m_work_info.state);
      string s;	// these fields are incorrect in the spec
      b.UnpackUint16StringNull(s);
      b.UnpackUint16StringNull(s);
      b.UnpackUint16StringNull(m_work_info.street);
      b.UnpackUint16StringNull(m_work_info.zip);
      b >> m_work_info.country;
      b.UnpackUint16StringNull(m_work_info.company_name);
      b.UnpackUint16StringNull(m_work_info.company_dept);
      b.UnpackUint16StringNull(m_work_info.company_position);
      b.advance(2);
      b.UnpackUint16StringNull(m_work_info.company_web);
      m_type = RWorkInfo;
      break;
    }
    case SrvResponse_AboutInfo:
      b.UnpackUint16StringNull(m_about);
      m_type = RAboutInfo;
      break;
    case SrvResponse_InterestInfo:
    {
      unsigned char n;
      b >> n;
      while (n > 0)
      {
	unsigned short cat;
	string spec;
	b >> cat;
	b.UnpackUint16StringNull(spec);
	m_personal_interest_info.addInterest(cat, spec);
	--n;
      }
      m_type = RInterestInfo;
      break;
    }
    case SrvResponse_BackgroundInfo:
    {
      unsigned char n;
      b >> n;
      while (n > 0)
      {
	unsigned short cat;
	string s;
	b >> cat;
	b.UnpackUint16StringNull(s);
	m_background_info.addSchool(cat, s);
	--n;
      }

      unsigned char junk;
      b >> junk;
      m_type = RBackgroundInfo;
      break;
    }
    case SrvResponse_Unknown:
    {
      unsigned short ws;
      b >> ws;
      m_type = RUnknown;
      break;
    }
    default:
      throw ParseException("Unknown mode for Detailed user info parsing");
    } 
  }

  void SrvResponseSNAC::ParseSimpleUserInfo(Buffer& b, unsigned short subtype) {
    if (subtype == SrvResponse_SimpleUI || subtype == SrvResponse_SimpleUI_Done) m_type = SimpleUserInfo;
    if (subtype == SrvResponse_SearchUI || subtype == SrvResponse_SearchUI_Done) m_type = SearchSimpleUserInfo;
    
    if (subtype == SrvResponse_SimpleUI_Done || subtype == SrvResponse_SearchUI_Done) m_last_in_search = true;
    else m_last_in_search = false;

    unsigned char wb;
    b >> wb;
    /*
     * status code
     * = 0a usually
     * = 32 on returning no contacts
     */
    if (wb == 0x32 || wb == 0x14) {
      m_empty_contact = true;
      return;
    }
    
    m_empty_contact = false;

    unsigned short ws;
    b >> ws; // unknown

    b >> m_uin;

    b.UnpackUint16StringNull(m_alias);
    b.UnpackUint16StringNull(m_firstname);
    b.UnpackUint16StringNull(m_lastname);
    b.UnpackUint16StringNull(m_email);

    // Auth required
    b >> wb;
    if (wb == 0) m_authreq = true;
    else m_authreq = false;

    // Status
    unsigned char st;
    b >> st;
    switch(st) {
    case 0:
      m_status = STATUS_OFFLINE;
      break;
    case 1:
      m_status = STATUS_ONLINE;
      break;
    case 2:
      // status disabled
      m_status = STATUS_OFFLINE;
      break;
    default:
      m_status = STATUS_OFFLINE;
    }

    b >> wb; // unknown

    if (b.remains() == 3 || b.remains() == 7) {
      b >> m_sex;
      b >> m_age;
      b >> wb;       // unknown
    }

    if (subtype == SrvResponse_SimpleUI_Done || subtype == SrvResponse_SearchUI_Done) {
      b >> m_more_results;
    }

  }

  void SrvResponseSNAC::ParseRandomChatFound(Buffer& b) {
    unsigned char wb;

    m_empty_contact = false;
    m_type = RandomChatFound;

    b >> wb;
    b >> m_uin;
    b >> wb;
  }

  void SrvResponseSNAC::ParseRandomGroupSetAck(Buffer& b) {
    m_type = RandomGroupSet;
  }

}
