/*
 * ContactTree
 * a list of groups, then list of contacts within each group
 *
 * Copyright (C) 2002 Barnaby Gray <barnaby@beedesign.co.uk>
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

#ifndef CONTACTTREE_H
#define CONTACTTREE_H

#include <list>
#include <string>
#include <map>

#include <libicq2000/Contact.h>
#include <libicq2000/sigslot.h>

namespace ICQ2000 {

  class ContactListEvent;

  // ---------------------------------------------------------------------------
  //  Group object
  //  (typedef'd into ContactTree::Group)
  // ---------------------------------------------------------------------------
  class _ContactTree_Group 
  {
   private:
    std::map<unsigned int, ContactRef> m_crefs;

    // unique id, used by server
    unsigned short m_id;
    
    std::string m_label;

   public:
    // iterators
    class iterator {
     private:
      std::map<unsigned int,ContactRef>::iterator iter;
  
     public:
      iterator(std::map<unsigned int,ContactRef>::iterator i)
      : iter(i) { }
  
      iterator& operator++() { ++iter; return *this; }
      iterator operator++(int) { return iterator(iter++); }
      bool operator==(const iterator& x) const { return iter == x.iter; }
      bool operator!=(const iterator& x) const { return iter != x.iter; }
      ContactRef& operator*() { return (*iter).second; }
    };

    class const_iterator {
     private:
      std::map<unsigned int,ContactRef>::const_iterator iter;
  
     public:
      const_iterator(std::map<unsigned int,ContactRef>::const_iterator i)
      : iter(i) { }
  
      const_iterator& operator++() { ++iter; return *this; }
      const_iterator operator++(int) { return const_iterator(iter++); }
      bool operator==(const const_iterator& x) const { return iter == x.iter; }
      bool operator!=(const const_iterator& x) const { return iter != x.iter; }
      const ContactRef& operator*() { return (*iter).second; }
    };

    _ContactTree_Group(const std::string& l, unsigned short id);
    _ContactTree_Group();
    _ContactTree_Group(const _ContactTree_Group& gp);

    unsigned short get_id() const;
    void set_id(unsigned short i) { m_id = i; }

    std::string get_label() const;
    void set_label(const std::string& l);

    ContactRef operator[](unsigned int uin);
    ContactRef lookup_uin(unsigned int uin);
    ContactRef lookup_mobile(const std::string& m);
    ContactRef lookup_email(const std::string& em);

    ContactRef add(ContactRef ct);
    void remove(unsigned int uin);

    void relocate_from(ContactRef ct);
    void relocate_to(ContactRef ct);
    
    unsigned int size() const;
    bool empty() const;

    bool exists(unsigned int uin);
    bool mobile_exists(const std::string& m);
    bool email_exists(const std::string& em);

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    // aggregation of contact's
    sigslot::signal1<StatusChangeEvent*> contact_status_change_signal;
    sigslot::signal1<UserInfoChangeEvent*> contact_userinfo_change_signal;

    sigslot::signal1<ContactListEvent*> contactlist_signal;
  };

  // ---------------------------------------------------------------------------
  //  ContactTree
  // ---------------------------------------------------------------------------
  class ContactTree {
   public:
    // Group typedef's
    typedef _ContactTree_Group Group;
    typedef std::list<Group>::iterator iterator;
    typedef std::list<Group>::const_iterator const_iterator;

   private:
    // Group list
    std::list<Group> m_groups;

    unsigned short get_unique_group_id() const;

   public:
    ContactTree();
    ContactTree(const ContactTree& ct);

    Group& add_group(const std::string& l);
    Group& add_group(const std::string& l, unsigned short group_id);
    void remove_group(Group& gp);
    void remove_group(unsigned short group_id);
    bool exists_group(unsigned short group_id);
    Group& lookup_group(unsigned short group_id);
    Group& lookup_group_containing_contact(ContactRef ct);
    
    void relocate_contact(ContactRef ct, Group& from, Group& to);

    ContactRef operator[](unsigned int uin);
    ContactRef lookup_uin(unsigned int uin);
    ContactRef lookup_mobile(const std::string& m);
    ContactRef lookup_email(const std::string& em);
    void remove(unsigned int uin);

    unsigned int size() const;
    unsigned int group_size() const;
    bool empty() const;

    bool exists(unsigned int uin);
    bool mobile_exists(const std::string& m);
    bool email_exists(const std::string& em);

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    // aggregation of contact's
    sigslot::signal1<StatusChangeEvent*> contact_status_change_signal;
    sigslot::signal1<UserInfoChangeEvent*> contact_userinfo_change_signal;

    sigslot::signal1<ContactListEvent*> contactlist_signal;
  };

}

#endif
