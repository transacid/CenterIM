/*
 * Capabilities
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

#include "Capabilities.h"

using std::set;

namespace ICQ2000 {

  /*
   * Capability blocks.  (Thanks libfaim for figuring out these - many
   * are only relevant to AIM, but included for completeness)
   *
   * These are CLSIDs. They should actually be of the form:
   *
   * {0x0946134b, 0x4c7f, 0x11d1,
   *  {0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
   *
   * But, eh.
   */
  
  const struct Capabilities::Block Capabilities::caps[] = {
    /*
     * Chat is oddball.
     */
    {Chat,
     {0x74, 0x8f, 0x24, 0x20, 0x62, 0x87, 0x11, 0xd1, 
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    /*
     * These are mostly in order.
     */
    {Voice,
     {0x09, 0x46, 0x13, 0x41, 0x4c, 0x7f, 0x11, 0xd1, 
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    {SendFile,
     {0x09, 0x46, 0x13, 0x43, 0x4c, 0x7f, 0x11, 0xd1, 
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    /*
     * Advertised by the EveryBuddy client.
     */
    {ICQ,
     {0x09, 0x46, 0x13, 0x44, 0x4c, 0x7f, 0x11, 0xd1, 
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    {IMImage,
     {0x09, 0x46, 0x13, 0x45, 0x4c, 0x7f, 0x11, 0xd1, 
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    {BuddyIcon,
     {0x09, 0x46, 0x13, 0x46, 0x4c, 0x7f, 0x11, 0xd1, 
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    {SaveStocks,
     {0x09, 0x46, 0x13, 0x47, 0x4c, 0x7f, 0x11, 0xd1,
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    {GetFile,
     {0x09, 0x46, 0x13, 0x48, 0x4c, 0x7f, 0x11, 0xd1,
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    {ICQServerRelay,
     {0x09, 0x46, 0x13, 0x49, 0x4c, 0x7f, 0x11, 0xd1,
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    /*
     * Indeed, there are two of these.  The former appears to be correct, 
     * but in some versions of winaim, the second one is set.  Either they 
     * forgot to fix endianness, or they made a typo. It really doesn't 
     * matter which.
     */
    {Games,
     {0x09, 0x46, 0x13, 0x4a, 0x4c, 0x7f, 0x11, 0xd1,
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    {Games2,
     {0x09, 0x46, 0x13, 0x4a, 0x4c, 0x7f, 0x11, 0xd1,
      0x22, 0x82, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    {SendBuddyList,
     {0x09, 0x46, 0x13, 0x4b, 0x4c, 0x7f, 0x11, 0xd1,
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    {ICQUnknown2,
     {0x09, 0x46, 0x13, 0x4e, 0x4c, 0x7f, 0x11, 0xd1,
      0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}},
    
    {ICQRTF,
     {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 
      0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x92}},
    
    {ICQUnknown,
     {0x2e, 0x7a, 0x64, 0x75, 0xfa, 0xdf, 0x4d, 0xc8,
      0x88, 0x6f, 0xea, 0x35, 0x95, 0xfd, 0xb6, 0xdf}},
    
    {Empty,
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    
    {TrillianCrypt,
     {0xf2, 0xe7, 0xc7, 0xf4, 0xfe, 0xad, 0x4d, 0xfb,
      0xb2, 0x35, 0x36, 0x79, 0x8b, 0xdf, 0x00, 0x00}},
    
    {APInfo, 
     {0xaa, 0x4a, 0x32, 0xb5, 0xf8, 0x84, 0x48, 0xc6,
      0xa3, 0xd7, 0x8c, 0x50, 0x97, 0x19, 0xfd, 0x5b}}
  };

  Capabilities::Capabilities()
  { }
  
  void Capabilities::default_icq2000_capabilities()
  {
    clear();
    set_capability_flag( ICQ );
    set_capability_flag( ICQServerRelay );
  }

  void Capabilities::default_icq2002_capabilities()
  {
    clear();
    set_capability_flag( ICQ );
    set_capability_flag( ICQServerRelay );
    set_capability_flag( ICQRTF );
//    set_capability_flag( ICQUnknown2 );
  }

  void Capabilities::clear()
  {
    m_flags.clear();
  }
  
  void Capabilities::set_capability_flag(Flag f)
  {
    m_flags.insert(f);
  }
  
  void Capabilities::clear_capability_flag(Flag f)
  {
    m_flags.erase(f);
  }

  bool Capabilities::has_capability_flag(Flag f) const
  {
    return (m_flags.count(f) > 0);
  }

  void Capabilities::Parse(Buffer& b, unsigned short len)
  {
    int i = len / sizeof_cap;
    unsigned char cap[sizeof_cap];
    for (int j = 0; j < i; ++j) {
      b.Unpack(cap, sizeof_cap);

      // search for capability in list
      for (unsigned int k = 0; k < sizeof(caps) / sizeof(Block); ++k)
	if ( memcmp( caps[k].data, cap, sizeof_cap ) == 0 ) {
	  set_capability_flag( caps[k].flag );
	  break;
	}
      // might be nice to catch unknown capabilities
    }

    // any remainder (there shouldn't be any, but..)
    b.advance( len - i * sizeof_cap );
  }

  void Capabilities::Output(Buffer& b) const
  {
    for (set<Flag>::const_iterator curr = m_flags.begin();
	 curr != m_flags.end(); ++curr)
      for (unsigned int i = 0; i < sizeof(caps) / sizeof(Block); ++i)
	if ( caps[i].flag == *curr ) {
	  b.Pack( caps[i].data, sizeof_cap );
	  break;
	}
    
  }
  
  unsigned short Capabilities::get_length() const
  {
    return sizeof_cap * m_flags.size();
  }

  /*
   * This can be done properly based on the actual capability now - as
   * opposed to the dodgy method based on length that was used before.
   */
  bool Capabilities::get_accept_adv_msgs() const
  {
    return (has_capability_flag(ICQ) && has_capability_flag(ICQServerRelay));
  }

}

