/*
 * ICBMCookieCache
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

#ifndef ICBMCOOKIECACHE_H
#define ICBMCOOKIECACHE_H

#include "Cache.h"
#include "ICBMCookie.h"
#include "events.h"

#include "libicq2000/sigslot.h"

namespace ICQ2000 {

  class ICBMCookieCache : public Cache<ICBMCookie, MessageEvent*> {
   public:
    ICBMCookieCache() { }
    ~ICBMCookieCache()
    {
      removeAll();
    }

    void removeItem(const ICBMCookieCache::literator& l)
    {
      if (((*l).getValue())->getType() != MessageEvent::FileTransfer)
	delete ((*l).getValue());
      Cache<ICBMCookie, MessageEvent*>::removeItem(l);
    }

    void expireItem(const ICBMCookieCache::literator& l) {
      expired.emit( (*l).getValue() );
      Cache<ICBMCookie, MessageEvent*>::expireItem(l);
    }

    ICBMCookie generateUnique() const {
      ICBMCookie c;
      c.generate();
      while (exists(c)) c.generate();
      return c;
    }

    sigslot::signal1<MessageEvent*> expired;

  };
}

#endif
