/*
 * SNAC - Server-based lists
 * Mitz Pettel, 2001
 *
 * based on: SNAC - Buddy List
 *
 * Copyright (C) 2001 Barnaby Gray <barnaby@beedesign.co.uk>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "SNAC-SBL.h"
#include "TLV.h"

using std::string;

namespace ICQ2000 {

  // --------------- Server-based Lists (Family 0x0013) SNACs --------------

  // ============================================================================
  //  Request SBL rights
  // ============================================================================

  SBLRequestRightsSNAC::SBLRequestRightsSNAC() { }
  
  void SBLRequestRightsSNAC::OutputBody(Buffer& b) const
  {
    // empty
  }

  // ============================================================================
  //  SBL rights reply
  // ============================================================================
  
  SBLRightsReplySNAC::SBLRightsReplySNAC() { }
  
  void SBLRightsReplySNAC::ParseBody(Buffer& b) 
  {    // several unknown TLVs, skip
    TLVList tlvlist;
    tlvlist.Parse(b, TLV_ParseMode_SBL_Rights, b.remains());
  }
  
  // ============================================================================
  //  Unconditional SBL list request
  // ============================================================================

  SBLRequestListSNAC::SBLRequestListSNAC() { }

  void SBLRequestListSNAC::OutputBody(Buffer& b) const
  {
    // empty
  }

  // ============================================================================
  //  SBL list reply
  // ============================================================================

  SBLListSNAC::SBLListSNAC() : m_privacy_id(0) { }
  
  const unsigned short Entry_UIN        = 0x0000;
  const unsigned short Entry_Group      = 0x0001;
  const unsigned short Entry_VisSetting = 0x0004;
  const unsigned short Entry_ICQTIC     = 0x0009;
  const unsigned short Entry_Invisible  = 0x000e;
  const unsigned short Entry_ImportTime = 0x0013;

  void SBLListSNAC::ParseBody(Buffer& b)
  {
    b.advance(1); // 00

    b >> m_size;  // remember for client

    unsigned short l = m_size;
    while (l--) {
      string name;
      unsigned short group_id, tag_id, type, len, end;

      b >> name
	>> group_id
	>> tag_id
	>> type
	>> len;
      end = b.pos() + len;

      // Parse TLVs
      TLVList tlvlist;
      tlvlist.ParseByLength(b, TLV_ParseMode_SBL, len);
      
      switch(type) {
      case Entry_UIN:
      {
	unsigned int uin;
	string nick;

	uin = Contact::StringtoUIN(name);
	ContactRef ct = ContactRef(new Contact(uin));

	ct->setServerSideInfo(group_id, tag_id);

	if (tlvlist.exists(TLV_SBL_Nick)) {
	  SBLNickTLV *tlv = static_cast<SBLNickTLV*>(tlvlist[TLV_SBL_Nick]);
	  ct->setAlias(tlv->Value()); // translate (?)
	} else {
	  ct->setAlias(name);
	}

	if (tlvlist.exists(TLV_SBL_SMS_No)) {
	  SBLSMSNoTLV *tlv = static_cast<SBLSMSNoTLV*>(tlvlist[TLV_SBL_SMS_No]);
	  ct->setMobileNo(tlv->Value());
	}
	
	if (tlvlist.exists(TLV_SBL_Await_Auth)) {
	  ct->setAuthAwait(true);
	} else {
	  ct->setAuthAwait(false);
	}
	
	// add to contact tree under group
	if (!m_tree.exists_group( group_id )) {
	  //throw ParseException("Contact group_id doesn't match any group");
	  m_unassigned.push_back( ct );
	}
	else {
	ContactTree::Group& gp = m_tree.lookup_group( group_id );
	gp.add(ct);
	}

	break;
      }
      case Entry_Group:
	if (group_id > 0) m_tree.add_group( name, group_id );
	break;
      case Entry_VisSetting:
	m_privacy_id = tag_id;
	break;
/*      case Entry_ICQTIC:
	// ignore
	break;
      case Entry_Invisible:
	// TODO
	break;
      case Entry_ImportTime:
	// ignore
	break;*/
      default:
        m_other_ids.insert(tag_id); // save tag_id
	break;
      }

    }

    b.advance(4);
  }
  
  // ============================================================================
  //  SBL List Received ACK
  // ============================================================================

  SBLListACKSNAC::SBLListACKSNAC() { }
  
  void SBLListACKSNAC::OutputBody(Buffer& b) const
  {
    // empty
  }
  
  // ============================================================================
  //  SBL Begin Edit
  // ============================================================================

  SBLBeginEditSNAC::SBLBeginEditSNAC() { }

  void SBLBeginEditSNAC::OutputBody(Buffer& b) const
  {
    // empty
  }

  // ============================================================================
  //  SBL Commit Edit
  // ============================================================================

  SBLCommitEditSNAC::SBLCommitEditSNAC() { }

  void SBLCommitEditSNAC::OutputBody(Buffer& b) const
  {
    // empty
  }

  // ============================================================================
  //  SBL Add Buddy
  // ============================================================================

  SBLAddBuddySNAC::SBLAddBuddySNAC() { }

  SBLAddBuddySNAC::SBLAddBuddySNAC(const Sbl_item &buddy, unsigned short group_id)
  	: m_buddy(buddy), m_buddy_group(group_id)
  { }

  void SBLAddBuddySNAC::OutputBody(Buffer& b) const
  {
	b << Contact::UINtoString(m_buddy.uin);
	b << (unsigned short) m_buddy_group;
	b << (unsigned short) m_buddy.tag_id;
	b << (unsigned short) 0x0000;

	Buffer::marker m = b.getAutoSizeShortMarker();

	// Contact Nickname TLV
	b << TLV_ContactNickname;
	b << m_buddy.nickname;

	// Auth awaiting TLV
	if(m_buddy.awaitAuth) {
	    b << TLV_AuthAwaited;
	    b << (unsigned short) 0x0000;
	}

	b.setAutoSizeMarker(m);
  }

  // ============================================================================
  //  SBL Add Group
  // ============================================================================

  SBLAddGroupSNAC::SBLAddGroupSNAC() { }

  SBLAddGroupSNAC::SBLAddGroupSNAC(const string& group_name, unsigned short group_id)
    : m_group_name(group_name), m_group_id(group_id)
  { }

  void SBLAddGroupSNAC::OutputBody(Buffer& b) const
  {
      // a Group
      b << m_group_name;
      b << m_group_id;
      b << (unsigned short) 0x0000;
      b << (unsigned short) 0x0001;
      b << (unsigned short) 0x0000;
    }


  // ============================================================================
  //  SBL Update Entry
  // ============================================================================

#if 0
  SBLUpdateEntrySNAC::SBLUpdateEntrySNAC(const string &group_name,
					 unsigned short group_id,
					 const std::vector<unsigned short> &ids)
    : m_group_name(group_name), m_group_id(group_id), m_ids(ids), m_cont(0)
  { }

  SBLUpdateEntrySNAC::SBLUpdateEntrySNAC(const ContactRef &c)
    : m_cont(c), m_group_id(0)
  { }
  
  void SBLUpdateEntrySNAC::OutputBody(Buffer& b) const {
    if(!m_cont.get()) {
      b << (unsigned short) m_group_name.size();
      b.Pack(m_group_name);
      b << m_group_id;
      b << (unsigned short) 0x0000;
      b << (unsigned short) 0x0001;

      if(m_ids.empty()) {
	b << (unsigned short) 0x0000;

      } else {
	b << (unsigned short) (4 + m_ids.size()*2);
	b << (unsigned short) 0x00c8;
	b << (unsigned short) (m_ids.size()*2);
	// raw-coded TLV, found no suitable data structures in TLV.h

	std::vector<unsigned short>::const_iterator curr = m_ids.begin();
	while (curr != m_ids.end()) {
	  b << (unsigned short) *curr;
	  ++curr;
	}

      }
    } else {
      string suin = m_cont->getStringUIN();

      b << (unsigned short) suin.size();
      b.Pack(suin);
      b << m_cont->getServerSideGroupID();
      b << m_cont->getServerSideID();
      b << (unsigned short) 0x0000;

      Buffer::marker m = b.getAutoSizeShortMarker();

      // Auth awaiting TLV
      if(m_cont->getAuthAwait()) {
	b << TLV_AuthAwaited;
	b << (unsigned short) 0x0000;
      }

      b.setAutoSizeMarker(m);
    }
  }
#endif

  // ============================================================================
  //  SBL Update Group
  // ============================================================================

  SBLUpdateGroupSNAC::SBLUpdateGroupSNAC(const string &group_name,
					 unsigned short group_id,
					 const std::set<unsigned short> &ids)
    : m_group_name(group_name), m_group_id(group_id), m_ids(ids)
  { }
  
  SBLUpdateGroupSNAC::SBLUpdateGroupSNAC(const Sbl_group& group)
  	: m_group_name(group.name), m_group_id(group.group_id), m_ids(group.buddies)
  { }
  

  void SBLUpdateGroupSNAC::OutputBody(Buffer& b) const {
      b << (unsigned short) m_group_name.size();
      b.Pack(m_group_name);
      b << m_group_id;
      b << (unsigned short) 0x0000;
      b << (unsigned short) 0x0001;

      if(m_ids.empty()) {
		b << (unsigned short) 0x0000;

      } else {
		b << (unsigned short) (4 + m_ids.size()*2);
		b << (unsigned short) 0x00c8;
		b << (unsigned short) (m_ids.size()*2);
	// raw-coded TLV, found no suitable data structures in TLV.h

		std::set<unsigned short>::const_iterator curr = m_ids.begin();
		while (curr != m_ids.end()) {
	  	b << (unsigned short) *curr;
	  	++curr;
		}
	  }
  }

  
  // ============================================================================
  //  SBL Update Privacy Settings
  // ============================================================================
  
  SBLUpdatePrivacySNAC::SBLUpdatePrivacySNAC(unsigned short buddy_id, unsigned char privacy)
  {
	m_buddy_id = buddy_id;
	m_privacy = privacy;
  }
  
  void SBLUpdatePrivacySNAC::OutputBody(Buffer& b) const
  {
	b << (unsigned short) 0x0000; // no buddy name
	b << (unsigned short) 0x0000; // master group
	b << m_buddy_id; // buddy ID
	b << (unsigned short) 0x0004; // Permit/Deny record
	
	Buffer::marker m = b.getAutoSizeShortMarker();
	
	b << (unsigned short) 0x00ca; // Privacy TLV
	b << (unsigned short) 0x0001;
	b << m_privacy;
	
	b << (unsigned short) 0x00cb; // Class visibility TLV
	b << (unsigned short) 0x0004;
	b << (unsigned short) 0xFFFF; // All classes
	b << (unsigned short) 0xFFFF;
	
	b.setAutoSizeMarker(m);
  }
  
  // ============================================================================
  //  SBL Remove Entry
  // ============================================================================
#if 0
  SBLRemoveEntrySNAC::SBLRemoveEntrySNAC() { }

  SBLRemoveEntrySNAC::SBLRemoveEntrySNAC(const ContactList& l)
    : m_buddy_list(), m_group_id(0)
  { 
    ContactList::const_iterator curr = l.begin();
    while (curr != l.end()) {
      if ((*curr)->isICQContact()) m_buddy_list.push_back(*curr);
      ++curr;
    }
  }

  SBLRemoveEntrySNAC::SBLRemoveEntrySNAC(const ContactRef& c)
    : m_buddy_list(1, c), m_group_id(0) { }

  SBLRemoveEntrySNAC::SBLRemoveEntrySNAC(const string &group_name, unsigned short group_id)
    : m_group_name(group_name), m_group_id(group_id) { }

  void SBLRemoveEntrySNAC::OutputBody(Buffer& b) const
  {
    if(m_group_id) {
      b << (unsigned short) m_group_name.size();
      b.Pack(m_group_name);
      b << m_group_id;
      b << (unsigned short) 0x0000;
      b << (unsigned short) 0x0001;
      b << (unsigned short) 0x0000;

    } else {
      std::list<ContactRef>::const_iterator curr = m_buddy_list.begin();
      while (curr != m_buddy_list.end()) {
	string suin = (*curr)->getStringUIN();
	string alias = (*curr)->getAlias();

	b << (unsigned short) suin.size();
	b.Pack(suin);
	b << (unsigned short) (*curr)->getServerSideGroupID();
	b << (unsigned short) (*curr)->getServerSideID();
	b << (unsigned short) 0x0000;

	int tlvlen = 4 + alias.size();
	if((*curr)->getAuthAwait()) tlvlen += 4;

	b << (unsigned short) tlvlen;

	b << TLV_ContactNickname;
	b << (unsigned short) alias.size();
	b.Pack(alias);

	if((*curr)->getAuthAwait()) {
	    b << TLV_AuthAwaited;
	    b << (unsigned short) 0x0000;
	}

	++curr;
      }

    }
  }
#endif

  // ============================================================================
  //  SBL Remove Buddy
  // ============================================================================

  SBLRemoveBuddySNAC::SBLRemoveBuddySNAC() { }

  SBLRemoveBuddySNAC::SBLRemoveBuddySNAC(const Sbl_item &buddy, unsigned short group_id)
  	: m_buddy(buddy), m_buddy_group(group_id)
  { }

  void SBLRemoveBuddySNAC::OutputBody(Buffer& b) const
  {
	b << Contact::UINtoString(m_buddy.uin);
	b << (unsigned short) m_buddy_group;
	b << (unsigned short) m_buddy.tag_id;
	b << (unsigned short) 0x0000;

	Buffer::marker m = b.getAutoSizeShortMarker();

	// Contact Nickname TLV
	b << TLV_ContactNickname;
	b << m_buddy.nickname;

	// Auth awaiting TLV
	if(m_buddy.awaitAuth) {
	    b << TLV_AuthAwaited;
	    b << (unsigned short) 0x0000;
	}

	b.setAutoSizeMarker(m);
  }

  // ============================================================================
  //  SBL Remove Group
  // ============================================================================

  SBLRemoveGroupSNAC::SBLRemoveGroupSNAC() { }

  SBLRemoveGroupSNAC::SBLRemoveGroupSNAC(const string& group_name, unsigned short group_id)
    : m_group_name(group_name), m_group_id(group_id)
  { }

  void SBLRemoveGroupSNAC::OutputBody(Buffer& b) const
  {
      // a Group
      b << m_group_name;
      b << m_group_id;
      b << (unsigned short) 0x0000;
      b << (unsigned short) 0x0001;
      b << (unsigned short) 0x0000;
  }

  // ============================================================================
  //  SBL Edit ACK
  // ============================================================================

  SBLEditACKSNAC::SBLEditACKSNAC() { }

  void SBLEditACKSNAC::ParseBody(Buffer& b) {
    unsigned short errcode;

    while(b.remains() >= 2) {
      b >> errcode;

      switch(errcode) {
	case 0x0000: m_results.push_back(Success); break;
	case 0x0003: m_results.push_back(AlreadyExists); break;
	case 0x000a: m_results.push_back(Failed); break;
	case 0x000e: m_results.push_back(AuthRequired); break;
      }
    }
  }

  // ============================================================================
  //  SBL List Unchanged reply
  // ============================================================================

  SBLListUnchangedSNAC::SBLListUnchangedSNAC()
  { }

  void SBLListUnchangedSNAC::ParseBody(Buffer& b)
  {
    // anything?
  }
  
  // ============================================================================
  //  SBL Request authorisation
  // ============================================================================
  
  SBLRequestAuthSNAC::SBLRequestAuthSNAC(const ContactRef &c, const std::string &message)
    : m_cont(c), m_msg(message)
  { }
  
  void SBLRequestAuthSNAC::OutputBody(Buffer& b) const
  {
    b.PackByteString(m_cont->getStringUIN());
    b << m_msg;
    b << (unsigned short) 0x0000;
  }
  
  // ============================================================================
  //  SBL Incoming request authorisation
  // ============================================================================

  SBLAuthRequestSNAC::SBLAuthRequestSNAC()
  { }

  void SBLAuthRequestSNAC::ParseBody(Buffer& b) 
  {
	unsigned short unk;
	b.UnpackByteString(m_uin);
	b >> m_msg;
	b >> unk;
  }

  // ============================================================================
  //  SBL Authorisation request reply (outgoing)
  // ============================================================================

  SBLAuthoriseSNAC::SBLAuthoriseSNAC(const ContactRef &c, const std::string reason, const bool grant)
   : m_cont(c), m_reason(reason), m_grant(grant)
  { }
  
  void SBLAuthoriseSNAC::OutputBody(Buffer &b) const
  {
    b.PackByteString(m_cont->getStringUIN());
    b << (unsigned char)(m_grant ? 1 : 0);
    b << m_reason;
  }

  // ============================================================================
  //  SBL Authorisation request reply (incoming)
  // ============================================================================
  SBLAuthReceivedSNAC::SBLAuthReceivedSNAC()
  { }
  
  void SBLAuthReceivedSNAC::ParseBody(Buffer& b) 
  {
	unsigned char res;
	b.UnpackByteString(m_uin);
	b >> res;
	m_grant = res;
	b >> m_msg;
  }
  
  // ============================================================================
  //  SBL "User added you" incoming message
  // ============================================================================
  SBLUserAddedYouSNAC::SBLUserAddedYouSNAC()
  { }
  
  void SBLUserAddedYouSNAC::ParseBody(Buffer& b) 
  {
	b.UnpackByteString(m_uin);
  }
}
