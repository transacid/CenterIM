/*
 * SNAC - Visible/invisible list management
 * Mitz Pettel, 2001
 *
 * based on: SNAC - Buddy (Contact) list management
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

#ifndef SNAC_BOS_H
#define SNAC_BOS_H

#include <string>
#include <list>

#include "SNAC-base.h"
#include "Contact.h"
#include "ContactList.h"
#include "UserInfoBlock.h"

namespace ICQ2000 {

  // List stuff (Family 0x0009)
  const unsigned short SNAC_BOS_Add_Visible = 0x0005;
  const unsigned short SNAC_BOS_Remove_Visible = 0x0006;
  const unsigned short SNAC_BOS_Add_Invisible = 0x0007;
  const unsigned short SNAC_BOS_Remove_Invisible = 0x0008;
  const unsigned short SNAC_BOS_Add_Tmp_Visible = 0x000A;
  const unsigned short SNAC_BOS_Remove_Tmp_Visible = 0x000B;

  // ----------------- Visible/invisible List (Family 0x0009) SNACs -----------

  class BOSFamilySNAC : virtual public SNAC {
   public:
    unsigned short Family() const { return SNAC_FAM_BOS; }
  };

  class BOSListSNAC : virtual public BOSFamilySNAC, public OutSNAC {
    protected:
      std::list<std::string> m_buddy_list;
      void OutputBody(Buffer& b) const;

    public:
      BOSListSNAC();
      BOSListSNAC(const ContactList& l);
      BOSListSNAC(const ContactRef& c);
      BOSListSNAC(const std::string& s);
      void addContact(const ContactRef& c);
  };


  class AddVisibleSNAC : public BOSListSNAC {
    public:
      unsigned short Subtype() const { return SNAC_BOS_Add_Visible; }
      AddVisibleSNAC();
      AddVisibleSNAC(const ContactList& l);
      AddVisibleSNAC(const ContactRef& c);
      AddVisibleSNAC(const std::string& s);
  };

  class AddInvisibleSNAC : public BOSListSNAC {
    public:
      unsigned short Subtype() const { return SNAC_BOS_Add_Invisible; }
      AddInvisibleSNAC();
      AddInvisibleSNAC(const ContactList& l);
      AddInvisibleSNAC(const ContactRef& c);
      AddInvisibleSNAC(const std::string& s);
  };

  class AddTmpVisibleSNAC : public BOSListSNAC {
    public:
      unsigned short Subtype() const { return SNAC_BOS_Add_Tmp_Visible; }
      AddTmpVisibleSNAC();
      AddTmpVisibleSNAC(const ContactList& l);
      AddTmpVisibleSNAC(const ContactRef& c);
      AddTmpVisibleSNAC(const std::string& s);
  };

  class RemoveVisibleSNAC : public BOSListSNAC {
    public:
      unsigned short Subtype() const { return SNAC_BOS_Remove_Visible; }
      RemoveVisibleSNAC();
      RemoveVisibleSNAC(const ContactList& l);
      RemoveVisibleSNAC(const ContactRef& c);
      RemoveVisibleSNAC(const std::string& s);
  };

  class RemoveInvisibleSNAC : public BOSListSNAC {
    public:
      unsigned short Subtype() const { return SNAC_BOS_Remove_Invisible; }
      RemoveInvisibleSNAC();
      RemoveInvisibleSNAC(const ContactList& l);
      RemoveInvisibleSNAC(const ContactRef& c);
      RemoveInvisibleSNAC(const std::string& s);
  };

  class RemoveTmpVisibleSNAC : public BOSListSNAC {
    public:
      unsigned short Subtype() const { return SNAC_BOS_Remove_Tmp_Visible; }
      RemoveTmpVisibleSNAC();
      RemoveTmpVisibleSNAC(const ContactList& l);
      RemoveTmpVisibleSNAC(const ContactRef& c);
      RemoveTmpVisibleSNAC(const std::string& s);
  };
  
}

#endif
