/*
 * ContactTree
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

#include "ContactTree.h"
#include "events.h"

namespace ICQ2000 {

  using std::string;

#define ITERATE_CONTACTS_BEGIN \
  iterator curr = begin();     \
  while (curr != end()) {
  
#define ITERATE_CONTACTS_END \
    ++curr;                  \
  }

#define ITERATE_GROUPS_BEGIN        \
  iterator curr = m_groups.begin(); \
  while (curr != m_groups.end()) {

#define ITERATE_GROUPS_END \
    ++curr;                \
  }

#define CONST_ITERATE_GROUPS_BEGIN        \
  const_iterator curr = m_groups.begin(); \
  while (curr != m_groups.end()) {

#define CONST_ITERATE_GROUPS_END \
    ++curr;                      \
  }

  // ============================================================================
  //  ContactTree Group object
  // ============================================================================

  _ContactTree_Group::_ContactTree_Group(const std::string& l, unsigned short id)
    : m_id(id), m_label(l)
  { }
  
  _ContactTree_Group::_ContactTree_Group()
    : m_id(0), m_label("")
  { }
  
  _ContactTree_Group::_ContactTree_Group(const _ContactTree_Group& gp)
    : m_crefs( gp.m_crefs ), m_id( gp.m_id ), m_label( gp.m_label )
  { }
  
  unsigned short _ContactTree_Group::get_id() const
  {
    return m_id;
  }
  
  string _ContactTree_Group::get_label() const
  {
    return m_label;
  }
  
  void _ContactTree_Group::set_label(const string& l) 
  {
    if (m_label == l) return;
    m_label = l;
    GroupChangeEvent ev( *this );
    contactlist_signal.emit( &ev );
  }

  ContactRef _ContactTree_Group::operator[](unsigned int uin)
  {
    return lookup_uin(uin);
  }

  ContactRef _ContactTree_Group::lookup_uin(unsigned int uin)
  {
    if (m_crefs.count(uin) != 0) return (*(m_crefs.find(uin))).second;
    return NULL;
  }
  
  ContactRef _ContactTree_Group::lookup_mobile(const string& m)
  {
    ITERATE_CONTACTS_BEGIN
    if ((*curr)->getNormalisedMobileNo() == m) return (*curr);
    ITERATE_CONTACTS_END

    return NULL;
  }

  ContactRef _ContactTree_Group::lookup_email(const string& em)
  {
    ITERATE_CONTACTS_BEGIN
    if ((*curr)->getEmail() == em) return (*curr);
    ITERATE_CONTACTS_END

    return NULL;
  }

  ContactRef _ContactTree_Group::add(ContactRef ct) {
    m_crefs.insert( std::make_pair(ct->getUIN(), ct) );

    // fire off signal
    UserAddedEvent uev( ct, *this );
    contactlist_signal.emit( &uev );

    // connect up contact's status signals to propagate up group
    ct->status_change_signal.connect( contact_status_change_signal );
    ct->userinfo_change_signal.connect( contact_userinfo_change_signal );

    return ct;
  }

  void _ContactTree_Group::relocate_from(ContactRef ct) {
    if (m_crefs.count(ct->getUIN()) != 0)
      m_crefs.erase(ct->getUIN());
  }

  void _ContactTree_Group::relocate_to(ContactRef ct) {
    m_crefs.insert( std::make_pair(ct->getUIN(), ct) );
  }

  void _ContactTree_Group::remove(unsigned int uin) {
    if (m_crefs.count(uin) != 0) {
      // first fire off signal
      UserRemovedEvent uev( m_crefs[uin], *this );
      contactlist_signal.emit( &uev );

      m_crefs.erase(uin);
    }
  }

  bool _ContactTree_Group::empty() const
  {
    return m_crefs.empty();
  }

  unsigned int _ContactTree_Group::size() const
  {
    return m_crefs.size();
  }

  bool _ContactTree_Group::exists(unsigned int uin)
  {
    return (m_crefs.count(uin) != 0);
  }
  
  bool _ContactTree_Group::mobile_exists(const string& m)
  {
    ITERATE_CONTACTS_BEGIN
    if ((*curr)->getNormalisedMobileNo() == m) return true;
    ITERATE_CONTACTS_END
    return false;
  }

  bool _ContactTree_Group::email_exists(const string& em)
  {
    ITERATE_CONTACTS_BEGIN
    if ((*curr)->getEmail() == em) return true;
    ITERATE_CONTACTS_END
    return false;
  }

  _ContactTree_Group::iterator _ContactTree_Group::begin()
  {
    return iterator(m_crefs.begin());
  }

  _ContactTree_Group::iterator _ContactTree_Group::end()
  {
    return iterator(m_crefs.end());
  }

  _ContactTree_Group::const_iterator _ContactTree_Group::begin() const
  {
    return const_iterator(m_crefs.begin());
  }

  _ContactTree_Group::const_iterator _ContactTree_Group::end() const
  {
    return const_iterator(m_crefs.end());
  }

  // ============================================================================
  //  ContactTree
  // ============================================================================

  ContactTree::ContactTree() { }
  
  ContactTree::ContactTree(const ContactTree& ct)
    : m_groups( ct.m_groups ) 
  { }
  
  ContactRef ContactTree::operator[](unsigned int uin)
  {
    return lookup_uin(uin);
  }
  
  ContactRef ContactTree::lookup_uin(unsigned int uin)
  {
    ITERATE_GROUPS_BEGIN
    if ((*curr).exists(uin)) return (*curr).lookup_uin(uin);
    ITERATE_GROUPS_END
    return NULL;
  }
  
  ContactRef ContactTree::lookup_mobile(const std::string& m)
  {
    ITERATE_GROUPS_BEGIN
    if ((*curr).mobile_exists(m)) return (*curr).lookup_mobile(m);
    ITERATE_GROUPS_END
    return NULL;
  }
  
  ContactRef ContactTree::lookup_email(const std::string& em)
  {
    ITERATE_GROUPS_BEGIN
    if ((*curr).email_exists(em)) return (*curr).lookup_email(em);
    ITERATE_GROUPS_END
    return NULL;
  }
  
  ContactTree::Group& ContactTree::add_group(const string& l)
  {
    return add_group(l, get_unique_group_id() );
  }

  ContactTree::Group& ContactTree::add_group(const string& l, unsigned short group_id)
  {
    if (exists_group(group_id)) {
      // oops.. trying to add one with a duplicate group_id
      group_id = get_unique_group_id();
    }

    Group gp( l, group_id );
    m_groups.push_back(gp);

    // propagate signals up to ContactTree object
    m_groups.back().contactlist_signal.connect( contactlist_signal );
    m_groups.back().contact_status_change_signal.connect( contact_status_change_signal );
    m_groups.back().contact_userinfo_change_signal.connect( contact_userinfo_change_signal );

    // fireoff event
    GroupAddedEvent ev(m_groups.back());
    contactlist_signal.emit( &ev );
    
    return m_groups.back();
  }

  void ContactTree::remove_group(Group& gp)
  {
    remove_group(gp.get_id());
  }
  
  void ContactTree::remove_group(unsigned short group_id)
  {
    ITERATE_GROUPS_BEGIN
    if ((*curr).get_id() == group_id) {
      // emit event
      GroupRemovedEvent ev(*curr);
      contactlist_signal.emit( &ev );

      // remove from list
      m_groups.erase(curr);

      break;
    }
    ITERATE_GROUPS_END
  }

  bool ContactTree::exists_group(unsigned short group_id)
  {
    ITERATE_GROUPS_BEGIN
    if ((*curr).get_id() == group_id) return true;
    ITERATE_GROUPS_END
    return false;
  }
  
  ContactTree::Group& ContactTree::lookup_group(unsigned short group_id)
  {
    ITERATE_GROUPS_BEGIN
    if ((*curr).get_id() == group_id) return (*curr);
    ITERATE_GROUPS_END
    return add_group("");
  }

  ContactTree::Group& ContactTree::lookup_group_containing_contact(ContactRef ct)
  {
    unsigned int uin = ct->getUIN();
    ITERATE_GROUPS_BEGIN
    if ((*curr).exists(uin)) return (*curr);
    ITERATE_GROUPS_END
    return add_group("");
  }

  void ContactTree::relocate_contact(ContactRef ct, Group& from, Group& to)
  {
    from.relocate_from(ct);
    to.relocate_to(ct);
    
    UserRelocatedEvent ev(ct, to, from);
    contactlist_signal.emit( &ev );
  }
  
  void ContactTree::remove(unsigned int uin)
  {
    ITERATE_GROUPS_BEGIN
    if ((*curr).exists(uin)) {
      (*curr).remove(uin);
      break;
    }
    ITERATE_GROUPS_END
  }
  
  unsigned int ContactTree::size() const
  {
    unsigned int ret = 0;
    CONST_ITERATE_GROUPS_BEGIN
    ret += (*curr).size();
    CONST_ITERATE_GROUPS_END
    return ret;
  }
  
  unsigned int ContactTree::group_size() const 
  {
    return m_groups.size();
  }
  
  bool ContactTree::empty() const
  {
    return m_groups.empty();
  }
  
  bool ContactTree::exists(unsigned int uin)
  {
    ITERATE_GROUPS_BEGIN
    if ((*curr).exists(uin)) return true;
    ITERATE_GROUPS_END
    return false;
  }
  
  bool ContactTree::mobile_exists(const std::string& m)
  {
    ITERATE_GROUPS_BEGIN
    if ((*curr).mobile_exists(m)) return true;
    ITERATE_GROUPS_END
    return false;
  }
  
  bool ContactTree::email_exists(const std::string& em)
  {
    ITERATE_GROUPS_BEGIN
    if ((*curr).email_exists(em)) return true;
    ITERATE_GROUPS_END
    return false;
  }
  
  ContactTree::iterator ContactTree::begin()
  {
    return m_groups.begin();
  }
  
  ContactTree::iterator ContactTree::end()
  {
    return m_groups.end();
  }
  
  ContactTree::const_iterator ContactTree::begin() const
  {
    return m_groups.begin();
  }
  
  ContactTree::const_iterator ContactTree::end() const
  {
    return m_groups.end();
  }

  unsigned short ContactTree::get_unique_group_id() const
  {
    unsigned short ret;
    while(1) {
      ret = (unsigned short) rand();
      // this is fine for our simple purposes..

      bool uniq = true;
      CONST_ITERATE_GROUPS_BEGIN
      if ((*curr).get_id() == ret) {
	uniq = false;
	break;
      }
      CONST_ITERATE_GROUPS_END
      
      if (uniq) break;
    }

    return ret;
  }
}  
