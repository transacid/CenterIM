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
#include "TLV.h"

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

  unsigned short InSNAC::Version() const {
    return m_version;
  }
  
  void InSNAC::Parse(Buffer& b) {
    b >> m_flags
      >> m_requestID;
    m_version = 0;
    if (m_flags & 0x8000) { // contains version TLV
	unsigned short dataLen = 0;	
	b >> dataLen;
	unsigned int pos = b.pos();
	
	if (dataLen >= 2) {
	    unsigned short tlvType = 0;
	    b >> tlvType;
	    if (tlvType == 1) {
		unsigned short tlvLen = 0;
    		b >> tlvLen;
		if (tlvLen >= 2) {
		    b >> m_version;
		}
	    }
	}
	b.setPos(pos + dataLen);
    }
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
  
  void ErrorSNAC::initCodeDescriptions() {
    codeDescriptions[0x01] = "Invalid SNAC header";
    codeDescriptions[0x02] = "Server rate limit exceeded";
    codeDescriptions[0x03] = "Client rate limit exceeded";
    codeDescriptions[0x04] = "Recipient is not logged in";
    codeDescriptions[0x05] = "Requested service unavailable";
    codeDescriptions[0x06] = "Requested service not defined";
    codeDescriptions[0x07] = "You sent obsolete SNAC";
    codeDescriptions[0x08] = "Not supported by server";
    codeDescriptions[0x09] = "Not supported by client";
    codeDescriptions[0x0A] = "Refused by client";
    codeDescriptions[0x0B] = "Reply too big";
    codeDescriptions[0x0C] = "Responses lost";
    codeDescriptions[0x0D] = "Request denied";
    codeDescriptions[0x0E] = "Incorrect SNAC format";
    codeDescriptions[0x0F] = "Insufficient rights";
    codeDescriptions[0x10] = "In local permit/deny (recipient blocked)";
    codeDescriptions[0x11] = "Sender too evil";
    codeDescriptions[0x12] = "Receiver too evil";
    codeDescriptions[0x13] = "User temporarily unavailable";
    codeDescriptions[0x14] = "No match";
    codeDescriptions[0x15] = "List overflow";
    codeDescriptions[0x16] = "Request ambiguous";
    codeDescriptions[0x17] = "Server queue full";
    codeDescriptions[0x18] = "Not while on AOL";
  }

  ErrorInSNAC::ErrorInSNAC(unsigned short f, unsigned short t)
    : m_family(f), m_subtype(t) { }

  std::string ErrorSNAC::getErrorDescription() {
    if( codeDescriptions.empty() ) {
      initCodeDescriptions();
    }
    std::string desc;
    if( codeDescriptions.count(m_error_code) == 0 ) {
      desc = "Unknown error";
    } else {
      desc = codeDescriptions[m_error_code];
    }
    return desc;
  }

  void ErrorInSNAC::ParseBody(Buffer& b) {
  	b >> m_error_code;
	TLVList list;
	list.Parse(b, TLV_ParseMode_Channel02, 1);
  }

  Buffer& operator<<(Buffer& b, const ICQ2000::OutSNAC& snac) { snac.Output(b); return b; }

}

