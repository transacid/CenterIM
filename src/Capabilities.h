/*
 * Capabilities
 * Some handling of the mysterious capabilities byte strings
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

#ifndef CAPABILITIES_H
#define CAPABILITIES_H

#include "buffer.h"

#include <set>

namespace ICQ2000 {

  class Capabilities {
   public:

    /*
     * A lot of these flags are AIM specific - so we're unlikely to
     * see/use them unless the other end is a 'libfaim pretending to
     * be ICQ' client.
     */
    enum Flag {
      Chat            = 0,
      Voice           = 1,
      SendFile        = 2,
      ICQ             = 3,
      IMImage         = 4,
      BuddyIcon       = 5,
      SaveStocks      = 6,
      GetFile         = 7,
      ICQServerRelay  = 8,
      Games           = 9,
      Games2          = 10,
      SendBuddyList   = 11,
      ICQRTF          = 12,
      ICQUnknown      = 13,
      Empty           = 14,
      TrillianCrypt   = 15,
      APInfo          = 16,
      ICQUnknown2     = 17
    };

   private:
    static const unsigned int sizeof_cap = 16;

    struct Block {
      Flag flag;
      unsigned char data[sizeof_cap];
    };
    static const Block caps[];

    std::set<Flag> m_flags;

   public:
    Capabilities();
    
    void default_icq2000_capabilities();
    void default_icq2002_capabilities();

    void clear();
    void set_capability_flag(Flag f);
    void clear_capability_flag(Flag f);
    bool has_capability_flag(Flag f) const;

    void Parse(Buffer& b, unsigned short len);
    void Output(Buffer& b) const;

    unsigned short get_length() const;

    bool get_accept_adv_msgs() const;
  };

}

#endif
