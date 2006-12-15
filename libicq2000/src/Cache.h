/*
 * Cache
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

#ifndef CACHE_H
#define CACHE_H

#include <list>
#include <time.h>

namespace ICQ2000 {
  
  /*
   * This class will cache an id to an object, it's templated
   * since it'll be useful in several places with different sorts of
   * ids and objects.
   */
  
  template <typename Key, typename Value> class CacheItem {
   protected:
    unsigned int m_timeout;
    time_t m_timestamp;
    Key m_key;
    Value m_value;

   public:
    CacheItem(const Key &k, const Value &v, unsigned int timeout);
    
    const Key& getKey() const;
    Value& getValue();
    time_t getTimestamp() const;
    time_t getExpiryTime() const;
    void setTimestamp(time_t t);
    void setTimeout(time_t t);
    void refresh();
  };

  template < typename Key, typename Value >
  class Cache {
   protected:
    typedef typename std::list< CacheItem<Key,Value> >::iterator literator;
    typedef typename std::list< CacheItem<Key,Value> >::const_iterator citerator;

    unsigned int m_timeout;
    
    /*
     * list for storing them in order to timeout
     * a hash could be used as well, but efficiency isn't really an
     * issue - there shouldn't be more than 10 items in here at any one point
     */
    std::list< CacheItem<Key,Value> > m_list;

    citerator lookup(const Key& k) const {
      citerator curr = m_list.begin();
      while (curr != m_list.end()) {
	if ((*curr).getKey() == k) return curr;
	++curr;
      }
      return m_list.end();
    }
    
    literator lookup(const Key& k) {
      literator curr = m_list.begin();
      while (curr != m_list.end()) {
	if ((*curr).getKey() == k) return curr;
	++curr;
      }
      return m_list.end();
    }
    
   public:
    Cache();
    virtual ~Cache();

    bool exists(const Key &k) const {
      citerator i = lookup(k);
      return (i != m_list.end());
    }

    Value& operator[](const Key &k) {
      literator i = lookup(k);
      if (i == m_list.end()) {
	return insert(k, Value());
      } else {
	return (*i).getValue();
      }
    }

    void remove(const Key &k)  {
      literator i = lookup(k);
      if (i != m_list.end()) removeItem(i);
    }

    virtual void removeItem(const literator& l) {
      m_list.erase(l);
    }

    virtual void expireItem(const literator& l) {
      // might want to override to add signalling on expiring of items
      removeItem(l);
    }
    
    void expireAll() {
      while (!m_list.empty()) {
	expireItem(m_list.begin());
      }
    }

    void removeAll() {
      while (!m_list.empty()) {
	removeItem(m_list.begin());
      }
    }

    Value& insert(const Key &k, const Value &v) {
      CacheItem<Key,Value> t(k,v,m_timeout);
      return (*insert(t)).getValue();
    }

    literator insert(const CacheItem<Key,Value>& t) {
      time_t exp_time = t.getExpiryTime();

      literator l = m_list.end();
      while (l != m_list.begin()) {
	--l;
	if ( (*l).getExpiryTime() < exp_time ) {
	  ++l;
	  break;
	}
      }
      return m_list.insert(l, t);
    }

    bool empty() const {
      return m_list.empty();
    }

    const Key& front() const {
      return m_list.front().getKey();
    }

    void refresh(const Key &k) {
      literator i = lookup(k);
      if (i != m_list.end()) {
	CacheItem<Key,Value> t(*i);
	m_list.erase(i);
	insert(t);
      }
    }

    unsigned int getDefaultTimeout() { return m_timeout; }
    void setDefaultTimeout(unsigned int s) { m_timeout = s; }

    void setTimeout(const Key &k, unsigned int s) {
      literator i = lookup(k);
      if (i != m_list.end()) {
	CacheItem<Key,Value> t(*i);
	t.setTimeout(s);
	m_list.erase(i);
	insert(t);
      }
    }

    void clearoutPoll() {
      time_t n = time(NULL);
      while (!m_list.empty() && m_list.front().getExpiryTime() < n)
	expireItem( m_list.begin() );
    }

  };

  template <typename Key, typename Value>
  CacheItem<Key,Value>::CacheItem(const Key &k, const Value &v, unsigned int timeout)
    : m_timeout(timeout), m_timestamp(time(NULL)), 
      m_key(k), m_value(v) { }

  template <typename Key, typename Value>
  void CacheItem<Key,Value>::setTimestamp(time_t t) { m_timestamp = t; }
  
  template <typename Key, typename Value>
  void CacheItem<Key,Value>::setTimeout(time_t t) { m_timeout = t; }
  
  template <typename Key, typename Value>
  time_t CacheItem<Key,Value>::getTimestamp() const { return m_timestamp; }
  
  template <typename Key, typename Value>
  time_t CacheItem<Key,Value>::getExpiryTime() const { return m_timestamp + m_timeout; }
  
  template <typename Key, typename Value>
  void CacheItem<Key,Value>::refresh() { m_timestamp = time(NULL); }
  
  template <typename Key, typename Value>
  const Key& CacheItem<Key,Value>::getKey() const {
    return m_key;
  }

  template <typename Key, typename Value>
  Value& CacheItem<Key,Value>::getValue() {
    return m_value;
  }

  template <typename Key, typename Value>
  Cache<Key,Value>::Cache() {
    setDefaultTimeout(60); // default timeout
  }

  template <typename Key, typename Value>
  Cache<Key,Value>::~Cache() {
    removeAll();
  }
 
}

#endif
