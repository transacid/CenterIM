/*
 * RateLimits - Limitation of rate of messages between server and client
 *
 * Copyright (C) 2007 Stéphane Bisinger <stephane.bisinger@gmail.com>
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

#include "RateLimits.h"

namespace ICQ2000 {
    RateClass::RateClass() : members() {}
    RateClass::RateClass(unsigned int win, unsigned int clr,
            unsigned int alrt, unsigned int lim,
            unsigned int disc, unsigned int cur,
            unsigned int max, unsigned int last,
            unsigned char cur_st) : window(win), clear_l(clr),
    alert_l(alrt), limited_l(lim), disconnected_l(disc), current_l(cur),
    max_l(max), last_time(last), current_state(cur_st), members() {}

    void RateClass::setMembers( unsigned short n, Buffer& b ) {
        for( int i = 0; i < n; i++ ) {
            unsigned short f, s;
            b >> f;
            b >> s;
            members.insert( std::pair<unsigned short, unsigned short>(f, s) );
        }
    }

    RateClass& RateClass::operator<<(Buffer& b) {
        b >> window;
        b >> clear_l;
        b >> alert_l;
        b >> limited_l;
        b >> disconnected_l;
        b >> current_l;
        b >> max_l;
        b >> last_time;
        b >> current_state;
        return (*this);
    }
}
