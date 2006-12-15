/*
 * SNAC - New UIN services
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

#include "SNAC-UIN.h"

#include "buffer.h"

using std::string;

namespace ICQ2000 {

  // -------------- New UIN (0x0017) Family -----------------------

  UINRequestSNAC::UINRequestSNAC(const string& p)
    : m_password(p) { }

  void UINRequestSNAC::OutputBody(Buffer& b) const{
    b<<(unsigned int)0x00010039;
    b<<(unsigned int)0x0;
    b<<(unsigned int)0x28000300;
    b<<(unsigned int)0x0;
    b<<(unsigned int)0x0;
    b<<(unsigned int)0x624e0000;
    b<<(unsigned int)0x624e0000;
    b<<(unsigned int)0x0;
    b<<(unsigned int)0x0;
    b<<(unsigned int)0x0;
    b<<(unsigned int)0x0;
    b.setLittleEndian();
    b.PackUint16StringNull(m_password);
    b.setBigEndian();
    b<<(unsigned int)0x624e0000;
    b<<(unsigned int)0x0000d601;
  }

  UINResponseSNAC::UINResponseSNAC() { }

  void UINResponseSNAC::ParseBody(Buffer& b){
    b.advance(46);
    b.setLittleEndian();
    b >> m_uin;
    b.advance(10);
  }
  
  UINRequestErrorSNAC::UINRequestErrorSNAC() { }
  
  void UINRequestErrorSNAC::ParseBody(Buffer& b){
    b.advance(32);
  }

}
