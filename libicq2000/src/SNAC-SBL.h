/*
 * SNAC - Server-based list management
 * Mitz Pettel, 2001
 * Copyright (C) 2002 Barnaby Gray <barnaby@beedesign.co.uk>.
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

#ifndef SNAC_SBL_H
#define SNAC_SBL_H

#include <string>
#include <list>

#include "SNAC-base.h"
#include "Contact.h"
#include "ContactList.h"
#include "ContactTree.h"
#include "UserInfoBlock.h"

namespace ICQ2000 {

  // Server-based list stuff (Family 0x0013)
  const unsigned short SNAC_SBL_Req_Rights       = 0x0002; // Out
  const unsigned short SNAC_SBL_Rights_Reply     = 0x0003; // In
  const unsigned short SNAC_SBL_Request_List     = 0x0004; // Out 
  const unsigned short SNAC_SBL_Check_List       = 0x0005; // Out
  const unsigned short SNAC_SBL_List_From_Server = 0x0006; // In
  const unsigned short SNAC_SBL_List_ACK         = 0x0007; // Out
  const unsigned short SNAC_SBL_Add_Entry        = 0x0008; // Out
  const unsigned short SNAC_SBL_Update_Entry     = 0x0009; // Out
  const unsigned short SNAC_SBL_Remove_Entry     = 0x000a; // Out
  const unsigned short SNAC_SBL_Edit_ACK         = 0x000e; // In
  const unsigned short SNAC_SBL_List_Unchanged   = 0x000f; // In
  const unsigned short SNAC_SBL_Begin_Edit       = 0x0011; // Out
  const unsigned short SNAC_SBL_Commit_Edit      = 0x0012; // Out
  const unsigned short SNAC_SBL_Request_Auth     = 0x0018; // Out
  const unsigned short SNAC_SBL_Auth_Request     = 0x0019; // In
  const unsigned short SNAC_SBL_Authorise        = 0x001a; // Out 
  const unsigned short SNAC_SBL_Auth_Granted     = 0x001b; // In
  const unsigned short SNAC_SBL_User_Added_You   = 0x001c; // In
  
  // ----------------- Server-based Lists (Family 0x0013) SNACs -----------

  class SBLFamilySNAC : virtual public SNAC
  {
   public:
    unsigned short Family() const { return SNAC_FAM_SBL; }
  };

  // ============================================================================
  //  Request SBL rights
  // ============================================================================

  class SBLRequestRightsSNAC : public SBLFamilySNAC, public OutSNAC 
  {
   protected:
    void OutputBody(Buffer& b) const;

  public:
    SBLRequestRightsSNAC();

    unsigned short Subtype() const { return SNAC_SBL_Req_Rights; }
  };

  // ============================================================================
  //  SBL rights reply
  // ============================================================================
  
  class SBLRightsReplySNAC : public SBLFamilySNAC, public InSNAC 
  {
   protected:
    void ParseBody(Buffer& b);

  public:
    SBLRightsReplySNAC();

    unsigned short Subtype() const { return SNAC_SBL_Rights_Reply; }
  };

  // ============================================================================
  //  Unconditional SBL list request
  // ============================================================================

  class SBLRequestListSNAC : public SBLFamilySNAC, public OutSNAC
  {
   protected:
    void OutputBody(Buffer& b) const;

   public:
    SBLRequestListSNAC();

    unsigned short Subtype() const { return SNAC_SBL_Request_List; }
  };
  
  // ============================================================================
  //  SBL list reply
  // ============================================================================

  class SBLListSNAC : public SBLFamilySNAC, public InSNAC
  {
   private:
    ContactTree m_tree;
    unsigned short m_size;
     
   protected:
    void ParseBody(Buffer& b);

   public:
    SBLListSNAC();
    
    ContactTree& getContactTree() { return m_tree; }
    unsigned short get_size() const { return m_size; }

    unsigned short Subtype() const { return SNAC_SBL_List_From_Server; }
  };
  
  // ============================================================================
  //  SBL list received ACK
  // ============================================================================

  class SBLListACKSNAC : public SBLFamilySNAC, public OutSNAC
  {
   protected:
    void OutputBody(Buffer& b) const;

   public:
    SBLListACKSNAC();
    
    unsigned short Subtype() const { return SNAC_SBL_List_ACK; }
  };
  
  // ============================================================================
  //  SBL Begin Edit
  // ============================================================================

  class SBLBeginEditSNAC : public SBLFamilySNAC, public OutSNAC
  {
   protected:
    void OutputBody(Buffer& b) const;

   public:
    SBLBeginEditSNAC();

    unsigned short Subtype() const { return SNAC_SBL_Begin_Edit; }
  };

  // ============================================================================
  //  SBL Add Entry
  // ============================================================================

  class SBLAddEntrySNAC : public SBLFamilySNAC, public OutSNAC
  {
   private:
    std::string m_group_name;
    std::list<ContactRef> m_buddy_list;
    unsigned short m_group_id;
    
   protected:
    void OutputBody(Buffer& b) const;

   public:
    SBLAddEntrySNAC();
    SBLAddEntrySNAC(const ContactList& l);
    SBLAddEntrySNAC(const ContactRef& c);
    SBLAddEntrySNAC(const std::string &group_name, unsigned short group_id);

    void addBuddy(const ContactRef& c);

    unsigned short Subtype() const { return SNAC_SBL_Add_Entry; }
  };

  // ============================================================================
  //  SBL Update Entry
  // ============================================================================

  class SBLUpdateEntrySNAC : public SBLFamilySNAC, public OutSNAC
  {
   private:
     std::string m_group_name;
     unsigned short m_group_id;
     std::vector<unsigned short> m_ids;
     const ContactRef &m_cont;

   protected:
    void OutputBody(Buffer& b) const;

   public:
    SBLUpdateEntrySNAC(const std::string &group_name,
		       unsigned short group_id, const std::vector<unsigned short> &ids);

    SBLUpdateEntrySNAC(const ContactRef &c);

    unsigned short Subtype() const { return SNAC_SBL_Update_Entry; }
  };
  
  // ============================================================================
  //  SBL Remove Entry
  // ============================================================================

  class SBLRemoveEntrySNAC : public SBLFamilySNAC, public OutSNAC
  {
   private:
    std::string m_group_name;
    std::list<ContactRef> m_buddy_list;
    unsigned short m_group_id;
    
   protected:
    void OutputBody(Buffer& b) const;

   public:
    SBLRemoveEntrySNAC();
    SBLRemoveEntrySNAC(const ContactList& l);
    SBLRemoveEntrySNAC(const ContactRef& c);
    SBLRemoveEntrySNAC(const std::string &group_name, unsigned short group_id);

    unsigned short Subtype() const { return SNAC_SBL_Remove_Entry; }
  };

  // ============================================================================
  //  SBL Commit Edit
  // ============================================================================

  class SBLCommitEditSNAC : public SBLFamilySNAC, public OutSNAC
  {
   protected:
    void OutputBody(Buffer& b) const;

   public:
    SBLCommitEditSNAC();

    unsigned short Subtype() const { return SNAC_SBL_Commit_Edit; }
  };

  // ============================================================================
  //  SBL Edit ACK
  // ============================================================================

  class SBLEditACKSNAC : public SBLFamilySNAC, public InSNAC
  {
   public:
     enum Result {
       Success,
       Failed,
       AuthRequired,
       AlreadyExists
     };

   protected:
    std::vector<Result> m_results;
    void ParseBody(Buffer& b);

   public:
    SBLEditACKSNAC();

    std::vector<Result> getResults() const { return m_results; }
    unsigned short Subtype() const { return SNAC_SBL_Edit_ACK; }
  };

  // ============================================================================
  //  SBL List Unchanged reply
  // ============================================================================

  class SBLListUnchangedSNAC : public SBLFamilySNAC, public InSNAC 
  {
   protected:
    void ParseBody(Buffer& b);

   public:
    SBLListUnchangedSNAC();
    unsigned short Subtype() const { return SNAC_SBL_List_Unchanged; }
  };
  
}

#endif
