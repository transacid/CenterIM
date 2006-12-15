/*
 * ContactList (model)
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

#ifndef CONTACTLIST_H
#define CONTACTLIST_H

#include <string>
#include <map>

#include <libicq2000/Contact.h>
#include <libicq2000/sigslot.h>

namespace ICQ2000 {

  class ContactListEvent;

  class _ContactList_iterator {
  private:
    std::map<unsigned int,ContactRef>::iterator iter;
  
  public:
    _ContactList_iterator(std::map<unsigned int,ContactRef>::iterator i)
      : iter(i) { }
  
    _ContactList_iterator& operator++() { ++iter; return *this; }
    _ContactList_iterator operator++(int) { return _ContactList_iterator(iter++); }
    bool operator==(const _ContactList_iterator& x) const { return iter == x.iter; }
    bool operator!=(const _ContactList_iterator& x) const { return iter != x.iter; }
    ContactRef& operator*() { return (*iter).second; }
  };

  class _ContactList_const_iterator {
   private:
    std::map<unsigned int,ContactRef>::const_iterator iter;
  
   public:
    _ContactList_const_iterator(std::map<unsigned int,ContactRef>::const_iterator i)
      : iter(i) { }
  
    _ContactList_const_iterator& operator++() { ++iter; return *this; }
    _ContactList_const_iterator operator++(int) { return _ContactList_const_iterator(iter++); }
    bool operator==(const _ContactList_const_iterator& x) const { return iter == x.iter; }
    bool operator!=(const _ContactList_const_iterator& x) const { return iter != x.iter; }
    const ContactRef& operator*() { return (*iter).second; }
  };

  class ContactList {
   private:
    std::map<unsigned int,ContactRef> m_cmap;

    /*
     * Mobile contacts are implemented as
     * Contact's and should still have UINs.
     * Purely Mobile contacts will have imaginary UINs
     * (ones counting down from -1), this is the best I could
     * do to keep this consistent across board.
     *
     * It would be nice to have a hash off mobile# to contact
     * but this was proving tricky to ensure consistency.
     */


   public:
    typedef _ContactList_iterator iterator;
    typedef _ContactList_const_iterator const_iterator;

    ContactList();
    ContactList(const ContactList& cl);
    ContactList(ContactRef ct);

    ContactRef operator[](unsigned int uin);
    ContactRef lookup_uin(unsigned int uin);
    ContactRef lookup_mobile(const std::string& m);
    ContactRef lookup_email(const std::string& em);
    ContactRef add(ContactRef ct);
    void remove(unsigned int uin);

    unsigned int size() const;
    bool empty() const;

    bool exists(unsigned int uin);
    bool mobile_exists(const std::string& m);
    bool email_exists(const std::string& em);

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
  };

}

#endif
