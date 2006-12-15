/*
 * Constants
 * External constants that clients will use
 *
 * Copyright (C) 2001 Barnaby Gray <barnaby@beedesign.co.uk>.
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

#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace ICQ2000
{
  
enum Status
{
  STATUS_ONLINE,
  STATUS_AWAY,
  STATUS_NA,
  STATUS_OCCUPIED,
  STATUS_DND,
  STATUS_FREEFORCHAT,
  STATUS_OFFLINE,
};

static const unsigned int SMS_Max_Length = 160;

static const unsigned int String_Limit = 16384; // A sensible limit
static const unsigned int Incoming_Packet_Limit = 65535;
 
enum RandomChatGroup
 {
  GROUP_GENERALCHAT  = 1,
  GROUP_ROMANCE      = 2,
  GROUP_GAMES        = 3,
  GROUP_STUDENTS     = 4,
  GROUP_20           = 5,
  GROUP_30           = 6,
  GROUP_40           = 7,
  GROUP_50PLUS       = 8,
  GROUP_SEEKINGWOMEN = 9,
  GROUP_SEEKINGMEN   = 10
};

}

#endif
