/*
 * FTCache
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

#ifndef FTCACHE_H
#define FTCACHE_H

#include "Cache.h"

#include "libicq2000/sigslot.h"

namespace ICQ2000
{
  /* predeclare classes */
  class FileTransferClient;
  class FileTransferEvent;
  
  /* fd -> FileTransferClient cache
   *
   * Where the party is at. Once a FileTransferClient object is added to the
   * cache MM for it is assumed on the cache. Also lookups for a
   * Contact are done in linear time now, it was just too much of a
   * head-ache maintaining the uin map in Client.
   */
  class FTCache : public Cache<int, FileTransferClient*>
  {
   public:
    FTCache() { }
    ~FTCache()
    {
      removeAll();
    }


    FileTransferClient* getByEvent(FileTransferEvent* ev)
    {
	 literator l = m_list.begin();
	 while (l != m_list.end()) {
		 if ((*l).getValue()->getEvent() == ev)
			 return (*l).getValue();
		 ++l;
	 }
	 return NULL;  
    }

    bool exists_listenfd(int fd)
    {
	 literator l = m_list.begin();
	 while (l != m_list.end()) {
		 if ((*l).getValue()->getlistenfd() == fd)
			 return true;
		 ++l;
	 }
	 return false;
    }

    void remove_and_not_delete(const int &k) {
	 literator i = lookup(k);
	 if (i != m_list.end())
	    m_list.erase(i);
    }


    void removeItem(const FTCache::literator& l)
    {
	 delete ((*l).getValue());
      Cache<int, FileTransferClient*>::removeItem(l);
    }

    void expireItem(const FTCache::literator& l)
    {
      expired.emit( (*l).getValue() );
      //Cache<int, FileTransferClient*>::expireItem(l);
      /* this will removeItem(..), which'll delete the DirectClient
       * (see above).
       */
    }

    void removeContact(const ContactRef& c)
    {
      literator curr = m_list.begin();
      literator next = curr;
      while ( curr != m_list.end() ) {
	FileTransferClient *ftc = (*curr).getValue();
	++next;
	if ( ftc->getContact().get() != NULL
	     /* Direct Connections won't have a contact associated
	      * with them initially just after having been accepted as
	      * an incoming connection (we don't know who they are
	      * yet) - so Contact could be a NULL ref.
	      */
	     && ftc->getContact()->getUIN() == c->getUIN() ) {
	  removeItem(curr);
	}
	curr = next;
      }
    }

    FileTransferClient* getByContact(const ContactRef& c)
    {
      // linear lookup
      literator curr = m_list.begin();
      while ( curr != m_list.end() ) {
	FileTransferClient *ftc = (*curr).getValue();
	if (  ftc->getContact().get() != NULL
	     /* Direct Connections won't have a contact associated
	      * with them initially just after having been accepted as
	      * an incoming connection (we don't know who they are
	      * yet) - so Contact could be a NULL ref.
	      */
	     && ftc->getContact()->getUIN() == c->getUIN() )
	  return ftc; // found it

	++curr;
      }

      return NULL; // not found
    }

    void clearoutMessagesPoll()
    {
      literator curr = m_list.begin();
      while ( curr != m_list.end() ) {
	FileTransferClient *ftc = (*curr).getValue();
	ftc->clearoutMessagesPoll();
	++curr;
      }
    }

    sigslot::signal1<FileTransferClient*> expired;
  };
  
}

#endif
