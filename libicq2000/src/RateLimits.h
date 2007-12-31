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

#ifndef __LIBICQ2000_RATE_LIMITS_H
#define __LIBICQ2000_RATE_LIMITS_H

#include "buffer.h"
#include <map>

namespace ICQ2000 {
    class RateClass {
        public:
            RateClass();
            RateClass(unsigned int win, unsigned int clr,
                    unsigned int alrt, unsigned int lim,
                    unsigned int disc, unsigned int cur,
                    unsigned int max, unsigned int last,
                    unsigned char cur_st);
            ~RateClass() {}
            unsigned int getWindow() const { return window; }
            unsigned int getClearLevel() const { return clear_l; }
            unsigned int getAlertLevel() const { return alert_l; }
            unsigned int getLimitedLevel() const { return limited_l; }
            unsigned int getDisconnectedLevel() const { return disconnected_l; }
            unsigned int getCurrentLevel() const { return current_l; }
            unsigned int getMaxLevel() const { return max_l; }
            unsigned int getLastTime() const { return last_time; }
            unsigned int getCurrentState() const { return current_state; }
            void setMembers( unsigned short n, Buffer& b );
            std::multimap<unsigned short, unsigned short> getMembers() const { return members; }
            RateClass& operator<<(Buffer& b);
        private:
            unsigned int window;
            unsigned int clear_l;
            unsigned int alert_l;
            unsigned int limited_l;
            unsigned int disconnected_l;
            unsigned int current_l;
            unsigned int max_l;
            unsigned int last_time;
            unsigned char current_state;
            std::multimap<unsigned short, unsigned short> members;
    };
}
#endif
