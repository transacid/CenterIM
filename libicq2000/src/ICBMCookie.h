/*
 * ICBM Cookie
 * Simple 8 byte message cookie
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

#ifndef ICBMCOOKIE_H
#define ICBMCOOKIE_H

#include "buffer.h"

namespace ICQ2000 {

  class ICBMCookie {
   private:
    unsigned int m_c1, m_c2;

   public:
    ICBMCookie();
    
    void generate();

    void Parse(Buffer& b);
    void Output(Buffer& b) const;

    bool operator==(const ICBMCookie& c) const;
    ICBMCookie& operator=(const ICBMCookie& c);
  };

  Buffer& operator<<(Buffer& b, const ICQ2000::ICBMCookie& c);
  Buffer& operator>>(Buffer& b, ICQ2000::ICBMCookie& c);

}

#endif
