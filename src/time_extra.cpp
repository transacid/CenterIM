/*
 * Extra time functions
 * (Mainly to make up for deficiencies/quirks in libc implementations)
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

#include "time_extra.h"

namespace ICQ2000 
{

  time_t gmt_mktime (struct tm *tm)
  {
    static const int epoch = 1970;
    
    time_t when;
    int day, year;
    static const int days[] = {
      0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
    };
    
    year = tm->tm_year + 1900;
    day = days[tm->tm_mon] + tm->tm_mday - 1;
    
    /* adjust for leap years */
    day += ((year - (epoch - (epoch % 4)))-1) / 4;
    day -= (year - (epoch - (epoch % 100))) / 100;
    day += (year - (epoch - (epoch % 400))) / 400;
    
    when = ((year - epoch) * 365 + day) * 24 + tm->tm_hour;
    when = (when * 60 + tm->tm_min) * 60 + tm->tm_sec;
    return when;
  }

}


