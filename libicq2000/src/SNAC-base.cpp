/*
 * SNAC base classes
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

#include "SNAC-base.h"

#include "buffer.h"

namespace ICQ2000 {

  // ------------------ Abstract SNACs ---------------

  SNAC::SNAC()
    : m_flags(0x0000), m_requestID(0x00000000) { }

  unsigned short SNAC::Flags() const {
    return m_flags;
  }

  unsigned int SNAC::RequestID() const {
    return m_requestID;
  }

  void SNAC::setRequestID(unsigned int id) {
    m_requestID = id;
  }

  void SNAC::setFlags(unsigned short fl) {
    m_flags = fl;
  }

  void InSNAC::Parse(Buffer& b) {
    b >> m_flags
      >> m_requestID;
    ParseBody(b);
  }

  void OutSNAC::Output(Buffer& b) const {
    OutputHeader(b);
    OutputBody(b);
  }

  void OutSNAC::OutputHeader(Buffer& b) const {
    b << Family();
    b << Subtype();
    b << Flags();
    b << RequestID();
  }

  // --------------- Raw SNAC ---------------------------

  RawSNAC::RawSNAC(unsigned short f, unsigned short t)
    : m_family(f), m_subtype(t) { }

  void RawSNAC::ParseBody(Buffer& b) {
    b.advance(b.size());
  }

  Buffer& operator<<(Buffer& b, const ICQ2000::OutSNAC& snac) { snac.Output(b); return b; }

}

