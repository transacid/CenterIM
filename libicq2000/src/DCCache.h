/*
 * DCCache
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

#ifndef DCCACHE_H
#define DCCACHE_H

#include "Cache.h"

#include "libicq2000/sigslot.h"

namespace ICQ2000
{
  /* predeclare classes */
  class DirectClient;
  
  /* fd -> DirectClient cache
   *
   * Where the party is at. Once a DirectClient object is added to the
   * cache MM for it is assumed on the cache. Also lookups for a
   * Contact are done in linear time now, it was just too much of a
   * head-ache maintaining the uin map in Client.
   */
  class DCCache : public Cache<int, DirectClient*>
  {
   public:
    DCCache() { }
    ~DCCache()
    {
      removeAll();
    }

    void removeItem(const DCCache::literator& l)
    {
      delete ((*l).getValue());
      Cache<int, DirectClient*>::removeItem(l);
    }

    void expireItem(const DCCache::literator& l)
    {
      expired.emit( (*l).getValue() );
      Cache<int, DirectClient*>::expireItem(l);
      /* this will removeItem(..), which'll delete the DirectClient
       * (see above).
       */
    }

    void removeContact(const ContactRef& c)
    {
      literator curr = m_list.begin();
      literator next = curr;
      while ( curr != m_list.end() ) {
	DirectClient *dc = (*curr).getValue();
	++next;
	if ( dc->getContact().get() != NULL
	     /* Direct Connections won't have a contact associated
	      * with them initially just after having been accepted as
	      * an incoming connection (we don't know who they are
	      * yet) - so Contact could be a NULL ref.
	      */
	     && dc->getContact()->getUIN() == c->getUIN() ) {
	  removeItem(curr);
	}
	curr = next;
      }
    }

    DirectClient* getByContact(const ContactRef& c)
    {
      // linear lookup
      literator curr = m_list.begin();
      while ( curr != m_list.end() ) {
	DirectClient *dc = (*curr).getValue();
	if (  dc->getContact().get() != NULL
	     /* Direct Connections won't have a contact associated
	      * with them initially just after having been accepted as
	      * an incoming connection (we don't know who they are
	      * yet) - so Contact could be a NULL ref.
	      */
	     && dc->getContact()->getUIN() == c->getUIN() )
	  return dc; // found it

	++curr;
      }

      return NULL; // not found
    }

    void clearoutMessagesPoll()
    {
      literator curr = m_list.begin();
      while ( curr != m_list.end() ) {
	DirectClient *dc = (*curr).getValue();
	dc->clearoutMessagesPoll();
	++curr;
      }
    }

    sigslot::signal1<DirectClient*> expired;
  };
  
}

#endif
