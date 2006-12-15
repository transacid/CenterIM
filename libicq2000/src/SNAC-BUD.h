/*
 * SNAC - Buddy (Contact) list management
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

#ifndef SNAC_BUD_H
#define SNAC_BUD_H

#include <string>
#include <list>

#include "SNAC-base.h"
#include "Contact.h"
#include "ContactList.h"
#include "ContactTree.h"
#include "UserInfoBlock.h"

namespace ICQ2000 {

  // Buddy stuff (Family 0x0003)
  const unsigned short SNAC_BUD_Error = 0x0001;
  const unsigned short SNAC_BUD_AddBuddy = 0x0004;
  const unsigned short SNAC_BUD_RemoveBuddy = 0x0005;
  const unsigned short SNAC_BUD_Online = 0x000b;
  const unsigned short SNAC_BUD_Offline = 0x000c;

  // ----------------- Buddy List (Family 0x0003) SNACs -----------

  class BUDFamilySNAC : virtual public SNAC {
   public:
    unsigned short Family() const { return SNAC_FAM_BUD; }
  };

  class AddBuddySNAC : public BUDFamilySNAC, public OutSNAC {
   private:
    std::list<std::string> m_buddy_list;
    
   protected:
    void OutputBody(Buffer& b) const;

   public:
    AddBuddySNAC();
    AddBuddySNAC(const ContactTree& l);
    AddBuddySNAC(const ContactRef& c);

    void addBuddy(const ContactRef& c);

    unsigned short Subtype() const { return SNAC_BUD_AddBuddy; }
  };

  class RemoveBuddySNAC : public BUDFamilySNAC, public OutSNAC {
   private:
    std::list<std::string> m_buddy_list;
    
   protected:
    void OutputBody(Buffer& b) const;

   public:
    RemoveBuddySNAC();
    RemoveBuddySNAC(const ContactList& l);
    RemoveBuddySNAC(const ContactRef& c);

    void removeBuddy(const ContactRef& c);

    unsigned short Subtype() const { return SNAC_BUD_RemoveBuddy; }
  };

  class BuddyOnlineSNAC : public BUDFamilySNAC, public InSNAC {
   private:
    UserInfoBlock m_userinfo;

  protected:
    void ParseBody(Buffer& b);

   public:
    BuddyOnlineSNAC();

    const UserInfoBlock& getUserInfo() const { return m_userinfo; }
    unsigned short Subtype() const { return SNAC_BUD_Online; }
  };
  

  class BuddyOfflineSNAC : public BUDFamilySNAC, public InSNAC {
   private:
    UserInfoBlock m_userinfo;

  protected:
    void ParseBody(Buffer& b);

   public:
    BuddyOfflineSNAC();

    UserInfoBlock getUserInfo() const { return m_userinfo; }
    unsigned short Subtype() const { return SNAC_BUD_Offline; }
  };
  

}

#endif
