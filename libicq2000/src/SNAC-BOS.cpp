/*
 * SNAC - Visible/invisible lists
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

#include "SNAC-BOS.h"

#include "TLV.h"

using std::string;
using std::list;

namespace ICQ2000 {

  // --------------- Visible/invisible List (Family 0x0009) SNACs --------------

  BOSListSNAC::BOSListSNAC() { }

  BOSListSNAC::BOSListSNAC(const ContactList& l)
    : m_buddy_list() { 
    ContactList::const_iterator curr = l.begin();
    while (curr != l.end()) {
      if ((*curr)->isICQContact()) m_buddy_list.push_back((*curr)->getStringUIN());
      ++curr;
    }
    
  }

  BOSListSNAC::BOSListSNAC(const ContactRef& c)
    : m_buddy_list(1, c->getStringUIN()) { }

  BOSListSNAC::BOSListSNAC(const string& s)
    : m_buddy_list(1, s) { }

    void BOSListSNAC::addContact(const ContactRef& c) {
    m_buddy_list.push_back(c->getStringUIN());
  }

  void BOSListSNAC::OutputBody(Buffer& b) const {
    list<string>::const_iterator curr = m_buddy_list.begin();
    while (curr != m_buddy_list.end()) {
      b << (unsigned char)(*curr).size();
      b.Pack(*curr);
      curr++;
    }
  }

    AddVisibleSNAC::AddVisibleSNAC() : BOSListSNAC() { }
    AddVisibleSNAC::AddVisibleSNAC(const ContactList& l) : BOSListSNAC(l) { }
    AddVisibleSNAC::AddVisibleSNAC(const ContactRef& c) : BOSListSNAC(c) { }
    AddVisibleSNAC::AddVisibleSNAC(const string& s) : BOSListSNAC(s) { }
    
    AddInvisibleSNAC::AddInvisibleSNAC() : BOSListSNAC() { }
    AddInvisibleSNAC::AddInvisibleSNAC(const ContactList& l) : BOSListSNAC(l) { }
    AddInvisibleSNAC::AddInvisibleSNAC(const ContactRef& c) : BOSListSNAC(c) { }
    AddInvisibleSNAC::AddInvisibleSNAC(const string& s) : BOSListSNAC(s) { }
    
    AddTmpVisibleSNAC::AddTmpVisibleSNAC() : BOSListSNAC() { }
    AddTmpVisibleSNAC::AddTmpVisibleSNAC(const ContactList& l) : BOSListSNAC(l) { }
    AddTmpVisibleSNAC::AddTmpVisibleSNAC(const ContactRef& c) : BOSListSNAC(c) { }
    AddTmpVisibleSNAC::AddTmpVisibleSNAC(const string& s) : BOSListSNAC(s) { }
    
    RemoveVisibleSNAC::RemoveVisibleSNAC() : BOSListSNAC() { }
    RemoveVisibleSNAC::RemoveVisibleSNAC(const ContactList& l) : BOSListSNAC(l) { }
    RemoveVisibleSNAC::RemoveVisibleSNAC(const ContactRef& c) : BOSListSNAC(c) { }
    RemoveVisibleSNAC::RemoveVisibleSNAC(const string& s) : BOSListSNAC(s) { }
    
    RemoveInvisibleSNAC::RemoveInvisibleSNAC() : BOSListSNAC() { }
    RemoveInvisibleSNAC::RemoveInvisibleSNAC(const ContactList& l) : BOSListSNAC(l) { }
    RemoveInvisibleSNAC::RemoveInvisibleSNAC(const ContactRef& c) : BOSListSNAC(c) { }
    RemoveInvisibleSNAC::RemoveInvisibleSNAC(const string& s) : BOSListSNAC(s) { }
    
    RemoveTmpVisibleSNAC::RemoveTmpVisibleSNAC() : BOSListSNAC() { }
    RemoveTmpVisibleSNAC::RemoveTmpVisibleSNAC(const ContactList& l) : BOSListSNAC(l) { }
    RemoveTmpVisibleSNAC::RemoveTmpVisibleSNAC(const ContactRef& c) : BOSListSNAC(c) { }
    RemoveTmpVisibleSNAC::RemoveTmpVisibleSNAC(const string& s) : BOSListSNAC(s) { }

}
