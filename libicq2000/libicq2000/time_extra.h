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

#ifndef TIME_EXTRA_H
#define TIME_EXTRA_H

#include <time.h>

namespace ICQ2000 {

  /*
   * a function to reliably produce a time_t from a struct tm that is
   * known to be stored in GMT. The problem lies in that BSDs have
   * tm_gmtoff member of struct tm, whereas other POSIX systems use
   * the global timezone instead, and heavens knows for other more bizarre
   * unixes.
   */
  time_t gmt_mktime(struct tm *tm);
}

#endif
