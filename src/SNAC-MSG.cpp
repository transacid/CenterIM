/*
 * SNAC - Messaging services
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

#include "SNAC-MSG.h"

#include "sstream_fix.h"

#include "TLV.h"
#include "Contact.h"

using std::string;
using std::ostringstream;

namespace ICQ2000 {

  // --------------- Message (Family 0x0004) SNACs -----------------

  const unsigned short MESSAGEFLAG_ALLOWED       = 0x00000001;
  const unsigned short MESSAGEFLAG_MISSEDCALLS   = 0x00000002;
  const unsigned short MESSAGEFLAG_TYPING        = 0x00000008;

  void MsgAddICBMParameterSNAC::OutputBody(Buffer& b) const
  {
    b << (unsigned short)0x0000       // channel
      << (unsigned int)  (MESSAGEFLAG_ALLOWED
			  | MESSAGEFLAG_MISSEDCALLS
			  | (m_typing_notif ? MESSAGEFLAG_TYPING : 0))
                                      // message flags
      << (unsigned short)0x1f40       // max message snac size
      << (unsigned short)0x03e7       // max sender warning level
      << (unsigned short)0x03e7       // max receiver warning level
      << (unsigned short)0x0000       // min message interval
      << (unsigned short)0x0000;      // unknown
  }

  MsgSendSNAC::MsgSendSNAC(ICQSubType *icqsubtype, bool ad)
    : m_icqsubtype(icqsubtype), m_advanced(ad) { } 

  void MsgSendSNAC::setSeqNum(unsigned short seqnum)
  {
    m_seqnum = seqnum;
  }

  void MsgSendSNAC::setAdvanced(bool ad)
  {
    m_advanced = ad;
  }

  void MsgSendSNAC::setICBMCookie(const ICBMCookie& c)
  {
    m_cookie = c;
  }

  void MsgSendSNAC::set_capabilities(const Capabilities& c)
  {
    m_dest_capabilities = c;
  }
  
  void MsgSendSNAC::OutputBody(Buffer& b) const
  {
    // ICBM Cookie
    b << m_cookie;
    
    /* There is no consistency in the protocol here,
     * Messages are sent on channel 1
     * Advanced messages are sent on channel 2
     * Everything else on channel 4
     */
    if (m_advanced) {
      b << (unsigned short)0x0002;
      
      UINICQSubType *ust = dynamic_cast<UINICQSubType*>(m_icqsubtype);
      if (ust == NULL) return;
      // should fix
      
      // Destination UIN (screenname)
      b.PackByteString( Contact::UINtoString(ust->getDestination()) );
      
      b << (unsigned short)0x0005;
      Buffer::marker m1 = b.getAutoSizeShortMarker();

      b << (unsigned short)0x0000       // status
	<< m_cookie;

      Capabilities c;
      c.set_capability_flag( Capabilities::ICQServerRelay );
      c.Output(b);

      b << (unsigned short)0x000a       // TLV
	<< (unsigned short)0x0002
	<< (unsigned short)0x0001;

      b << (unsigned short)0x000f       // TLV
	<< (unsigned short)0x0000;

      b << (unsigned short)0x2711;      // TLV
      Buffer::marker m2 = b.getAutoSizeShortMarker();

      b.setLittleEndian();

      // -- start of first subsection
      Buffer::marker m3 = b.getAutoSizeShortMarker();
      
      // unknown..
      b << (unsigned short)0x0007;      // Protocol version

      b << (unsigned int)  0x00000000   // Unknown
	<< (unsigned int)  0x00000000
	<< (unsigned int)  0x00000000
	<< (unsigned int)  0x00000000
	<< (unsigned short)0x0000;

      b << (unsigned int)  0x00000003;  // Client features (?)
      b << (unsigned char) 0x00;        // Unknown

      b << m_seqnum;

      b.setAutoSizeMarker(m3);
      // -- end of first subsection

      // -- start of second subsection
      Buffer::marker m4 = b.getAutoSizeShortMarker();
      b	<< m_seqnum;

      // unknown
      b << (unsigned int)0x00000000
	<< (unsigned int)0x00000000
	<< (unsigned int)0x00000000;

      b.setAutoSizeMarker(m4);
      // -- end of second subsection

      m_icqsubtype->Output(b);
      
      b.setAutoSizeMarker(m1);
      b.setAutoSizeMarker(m2);

      b.setBigEndian();
      b << (unsigned short)0x0003 // TLV
	<< (unsigned short)0x0000;

      return;
    }

    // non-advanced

    if (m_icqsubtype->getType() == MSG_Type_Normal) {
      NormalICQSubType *nst = static_cast<NormalICQSubType*>(m_icqsubtype);

      b << (unsigned short)0x0001;

      // Destination UIN (screenname)
      b.PackByteString( Contact::UINtoString(nst->getDestination()) );
      
      string text = nst->getMessage();

      /*
       * Message Block TLV
       * - contains two TLVs
       * 0x0501 - don't know what this is
       * 0x0101 - the message
       */
      b << (unsigned short)0x0002;
      Buffer::marker m1 = b.getAutoSizeShortMarker();
      
      // TLV 0x0501 - unknown
      b << (unsigned short)0x0501
	<< (unsigned short)0x0001
	<< (unsigned char) 0x01;
      
      // TLV 0x0101 - flags + the message
      b << (unsigned short)0x0101;
      Buffer::marker m2 = b.getAutoSizeShortMarker();
      
      // flags - for what?
      b << (unsigned short)0x0000
	<< (unsigned short)0x0000;
      
      b.Pack(text);

      b.setAutoSizeMarker(m1);
      b.setAutoSizeMarker(m2);
      
    } else if (m_icqsubtype->getType() == MSG_Type_URL
	       || m_icqsubtype->getType() == MSG_Type_AuthReq
	       || m_icqsubtype->getType() == MSG_Type_AuthAcc
	       || m_icqsubtype->getType() == MSG_Type_AuthRej
	       || m_icqsubtype->getType() == MSG_Type_UserAdd) {
      UINICQSubType *ust = dynamic_cast<UINICQSubType*>(m_icqsubtype);
      if (ust == NULL) return;
      
      b << (unsigned short)0x0004;

      // Destination UIN (screenname)
      b.PackByteString( Contact::UINtoString(ust->getDestination()) );
      
      /*
       * Data Block TLV
       */
      b << (unsigned short)0x0005;
      Buffer::marker m1 = b.getAutoSizeShortMarker();

      b.setLittleEndian();
      b << (unsigned int)ust->getSource();
      ust->Output(b);
      b.setAutoSizeMarker(m1);

    }

    /*
     * Another TLV - dunno what it means
     * - it doesn't seem to matter if I take this out
     */
    b.setBigEndian();
    b << (unsigned short)0x0006
      << (unsigned short)0x0000;

  }

  MessageSNAC::MessageSNAC() : m_icqsubtype(NULL) { }

  MessageSNAC::~MessageSNAC() {
    if (m_icqsubtype != NULL) delete m_icqsubtype;
  }

  ICQSubType* MessageSNAC::grabICQSubType() {
    ICQSubType *ret = m_icqsubtype;
    m_icqsubtype = NULL;
    return ret;
  }

  void MessageSNAC::ParseBody(Buffer& b) {
    // ICBM Cookie
    b >> m_cookie;

    /*
     * Channel 0x0001 = Normal message
     * Channel 0x0002 = Advanced messages
     * Channel 0x0004 = ICQ specific features (URLs, Added to Contact List, SMS, etc.)
     */
    unsigned short channel;
    b >> channel;
    if (channel != 0x0001 && channel != 0x0002 && channel != 0x0004)
      throw ParseException("Message SNAC 0x0004 0x0007 received on unknown channel");

    /*
     * the UserInfo block comes next
     * this is a screenname, then some tlvs
     */
    m_userinfo.Parse(b);

    /*
     * the Message block comes now
     * this is one (maybe more TLVs) with the message garbled
     * up inside
     */
    if (channel == 0x0001) {
      TLVList tlvlist;
      tlvlist.Parse(b, TLV_ParseMode_MessageBlock, (unsigned int)-1);

      // Normal message
      if (!tlvlist.exists(TLV_MessageData))
	throw ParseException("No message data in SNAC");

      MessageDataTLV *t = static_cast<MessageDataTLV*>(tlvlist[TLV_MessageData]);
      // coerce this into the NormalICQSubType
      NormalICQSubType *nst = new NormalICQSubType(false);
      nst->setAdvanced(false);
      nst->setMessage( t->getMessage() );
      m_icqsubtype = nst;

    } else if (channel == 0x0002) {
      TLVList tlvlist;
      tlvlist.Parse(b, TLV_ParseMode_AdvMsgBlock, (unsigned int)-1);
      
      if (!tlvlist.exists(TLV_AdvMsgData))
	throw ParseException("No Advanced Message TLV in SNAC 0x0004 0x0007 on channel 2");

      AdvMsgDataTLV *t = static_cast<AdvMsgDataTLV*>(tlvlist[TLV_AdvMsgData]);
      m_icqsubtype = t->grabICQSubType();

    } else if (channel == 0x0004) {
      TLVList tlvlist;
      tlvlist.Parse(b, TLV_ParseMode_MessageBlock, (unsigned int)-1);

      /* ICQ hacked in messages
       * - SMS message
       * - URLs
       * - Added to contactlist
       * ..
       */

      if (!tlvlist.exists(TLV_ICQData))
	throw ParseException("No ICQ data TLV in SNAC 0x0004 0x0007 on channel 4");
      
      ICQDataTLV *t = static_cast<ICQDataTLV*>(tlvlist[TLV_ICQData]);
      m_icqsubtype = t->grabICQSubType();

    } else {

      ostringstream ostr;
      ostr << "Message SNAC on unsupported channel: 0x" << std::hex << channel;
      throw ParseException(ostr.str());

    }

    if (m_icqsubtype != NULL && dynamic_cast<UINICQSubType*>(m_icqsubtype) != NULL) {
      UINICQSubType *ust = dynamic_cast<UINICQSubType*>(m_icqsubtype);
      ust->setSource( m_userinfo.getUIN() );
    }
   
  }

  MessageACKSNAC::MessageACKSNAC()
    : m_icqsubtype(NULL) { }

  MessageACKSNAC::MessageACKSNAC(ICBMCookie c, UINICQSubType *icq)
    : m_cookie(c), m_icqsubtype(icq) { }

  MessageACKSNAC::~MessageACKSNAC() {
    if (m_icqsubtype != NULL) delete m_icqsubtype;
  }

  void MessageACKSNAC::OutputBody(Buffer& b) const {
    b << m_cookie
      << (unsigned short)0x0002;

    b.PackByteString( Contact::UINtoString( m_icqsubtype->getSource() ) );

    b << (unsigned short)0x0003; // unknown

    b.setLittleEndian();
    
    // -- start of first subsection
    Buffer::marker m1 = b.getAutoSizeShortMarker();
      
    // unknown..
    b << (unsigned short)0x0007;      // Protocol version

    b << (unsigned int)  0x00000000   // Unknown
      << (unsigned int)  0x00000000
      << (unsigned int)  0x00000000
      << (unsigned int)  0x00000000
      << (unsigned short)0x0000;

    b << (unsigned int)  0x00000003;  // Client features (?)
    b << (unsigned char) 0x00;        // Unknown

    b << m_icqsubtype->getSeqNum();

    b.setAutoSizeMarker(m1);
    // -- end of first subsection

    // -- start of second subsection
    Buffer::marker m2 = b.getAutoSizeShortMarker();
    b << m_icqsubtype->getSeqNum();

    // unknown
    b << (unsigned int)0x00000000
      << (unsigned int)0x00000000
      << (unsigned int)0x00000000;

    b.setAutoSizeMarker(m2);
    // -- end of second subsection

    m_icqsubtype->Output(b);
  }

  void MessageACKSNAC::ParseBody(Buffer& b) {
    b >> m_cookie;

    unsigned short channel;
    b >> channel;

    string sn;
    unsigned int uin;
    b.UnpackByteString(sn);
    uin = Contact::StringtoUIN(sn);

    b.advance(2); // 0x0003 unknown
    
    unsigned short len;
    b.setLittleEndian();

    // -- first section, mostly unknown/useless
    b >> len;
    b.advance(len);
    // ----

    // -- second section
    unsigned short seqnum;
    b >> len;
    b >> seqnum;
    b.advance(len - 2); // 12 zeroes usually
    // ----

    ICQSubType *t = ICQSubType::ParseICQSubType(b, true, true);
    if (t != NULL) {
      m_icqsubtype = dynamic_cast<UINICQSubType*>(t);
      if (m_icqsubtype != NULL) {
	m_icqsubtype->setSource(uin);
	m_icqsubtype->setSeqNum(seqnum);
      } else {
	delete t;
      }

    }

    /* trailing GUID ? */
    if (b.remains() > 0) {
      unsigned int len;
      b >> len;
      b.advance(len);
    }
    
  }

  void MessageOfflineUserSNAC::ParseBody(Buffer& b) {
    /**
     *  The server sends this packet to you if you try sending a
     *  message to a user who is offline.  If it was an advanced
     *  message you sent, then it needs to be retried as a
     *  non-advanced message.
     */
    b >> m_cookie
      >> m_channel;

    unsigned char len;
    string sn;
    b >> len;
    b.Unpack(sn, len);
    m_uin = Contact::StringtoUIN(sn);
  }

}
