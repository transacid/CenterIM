/*
 * SNAC - Buddy List
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

#include "SNAC-BUD.h"

#include "TLV.h"

using std::string;
using std::list;

namespace ICQ2000 {

  // --------------- Buddy List (Family 0x0003) SNACs --------------

  AddBuddySNAC::AddBuddySNAC() { }

  AddBuddySNAC::AddBuddySNAC(const ContactTree& l)
    : m_buddy_list() { 
    ContactTree::const_iterator curr = l.begin();
    while (curr != l.end()) {
      ContactTree::Group::const_iterator gcurr = curr->begin();
      while (gcurr != curr->end()) {
	if ((*gcurr)->isICQContact()) m_buddy_list.push_back((*gcurr)->getStringUIN());
	++gcurr;
      }
      ++curr;
    }
    
  }

  AddBuddySNAC::AddBuddySNAC(const ContactRef& c)
    : m_buddy_list(1, c->getStringUIN()) { }

  void AddBuddySNAC::addBuddy(const ContactRef& c) {
    m_buddy_list.push_back(c->getStringUIN());
  }

  void AddBuddySNAC::OutputBody(Buffer& b) const {
    list<string>::const_iterator curr = m_buddy_list.begin();
    while (curr != m_buddy_list.end()) {
      b << (unsigned char)(*curr).size();
      b.Pack(*curr);
      curr++;
    }
  }

  RemoveBuddySNAC::RemoveBuddySNAC() { }

  RemoveBuddySNAC::RemoveBuddySNAC(const ContactList& l)
    : m_buddy_list() { 
    ContactList::const_iterator curr = l.begin();
    while (curr != l.end()) {
      if ((*curr)->isICQContact()) m_buddy_list.push_back((*curr)->getStringUIN());
      ++curr;
    }
    
  }

  RemoveBuddySNAC::RemoveBuddySNAC(const ContactRef& c)
    : m_buddy_list(1, c->getStringUIN()) { }

  void RemoveBuddySNAC::removeBuddy(const ContactRef& c) {
    m_buddy_list.push_back(c->getStringUIN());
  }

  void RemoveBuddySNAC::OutputBody(Buffer& b) const {
    list<string>::const_iterator curr = m_buddy_list.begin();
    while (curr != m_buddy_list.end()) {
      b << (unsigned char)(*curr).size();
      b.Pack(*curr);
      curr++;
    }
  }

  BuddyOnlineSNAC::BuddyOnlineSNAC() { }

  void BuddyOnlineSNAC::ParseBody(Buffer& b) {
    /* All BuddyOnline consists of is the user info
     * block TLVs
     */
    m_userinfo.Parse(b);
  }

  BuddyOfflineSNAC::BuddyOfflineSNAC() { }

  void BuddyOfflineSNAC::ParseBody(Buffer& b) {
    m_userinfo.Parse(b);
  }

}
