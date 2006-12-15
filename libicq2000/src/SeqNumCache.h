/*
 * SeqNumCache
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

#ifndef SEQNUMCACHE_H
#define SEQNUMCACHE_H

#include "Cache.h"
#include "events.h"

#include "libicq2000/sigslot.h"

namespace ICQ2000 {

  class SeqNumCache : public Cache<unsigned short, MessageEvent*> {
   public:
    SeqNumCache() { }
    ~SeqNumCache()
    {
      removeAll();
    }
    
    void expireItem(const SeqNumCache::literator& l) {
      expired.emit( (*l).getValue() );
      Cache<unsigned short, MessageEvent*>::expireItem(l);
    }

    sigslot::signal1<MessageEvent*> expired;
  };
  
}

#endif
