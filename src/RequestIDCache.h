/*
 * RequestIDCache
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

#ifndef REQUESTIDCACHE_H
#define REQUESTIDCACHE_H

#include "Cache.h"

#include "libicq2000/sigslot.h"

namespace ICQ2000 {

  class RequestIDCacheValue {
   public:
    enum Type {
      UserInfo,
      SMSMessage,
      Search,
      ServerBasedContact
    };

    virtual ~RequestIDCacheValue() { }

    virtual Type getType() const = 0;
  };

  class UserInfoCacheValue : public RequestIDCacheValue {
   private:
    ContactRef m_contact;

   public:
    UserInfoCacheValue(ContactRef c) : m_contact(c) { }
    unsigned int getUIN() const { return m_contact->getUIN(); }
    ContactRef getContact() const { return m_contact; }

    Type getType() const { return UserInfo; }
  };

  class SMSEventCacheValue : public RequestIDCacheValue {
   private:
    SMSMessageEvent *m_ev;

   public:
    SMSEventCacheValue( SMSMessageEvent *ev ) : m_ev(ev) { }
    virtual ~SMSEventCacheValue() { delete m_ev; }
    SMSMessageEvent* getEvent() const { return m_ev; }
    ContactRef getContact() const { return m_ev->getContact(); }

    Type getType() const { return SMSMessage; }
  };

  class SearchCacheValue : public RequestIDCacheValue {
   private:
    SearchResultEvent *m_ev;

   public:
    SearchCacheValue( SearchResultEvent *ev ) : m_ev(ev) { }
    SearchResultEvent* getEvent() const { return m_ev; }
    Type getType() const { return Search; }
  };

  /*  class ServerBasedContactCacheValue : public RequestIDCacheValue {
   private:
    ServerBasedContactEvent *m_ev;

   public:
    ServerBasedContactCacheValue( ServerBasedContactEvent *ev ) : m_ev(ev) { }
    ServerBasedContactEvent *getEvent() const { return m_ev; }
    Type getType() const { return ServerBasedContact; }
  };
  */

  class RequestIDCache : public Cache<unsigned int, RequestIDCacheValue*> {
   public:
    RequestIDCache() { }
    ~RequestIDCache()
    {
      removeAll();
    }

    sigslot::signal1<RequestIDCacheValue*> expired;
    
    void expireItem(const RequestIDCache::literator& l) {
      expired.emit( (*l).getValue() );
      Cache<unsigned int, RequestIDCacheValue*>::expireItem(l);
    }

    void removeItem(const RequestIDCache::literator& l) {
      delete ((*l).getValue());
      Cache<unsigned int, RequestIDCacheValue*>::removeItem(l);
    }

  };
  
}

#endif
