/*
 * SNAC - base classes
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

#ifndef SNAC_BASE_H
#define SNAC_BASE_H

namespace ICQ2000 {
 
  /* predeclare classes */
  class Buffer;

  // ------------- SNAC numerical constants ------------

  // SNAC Families
  const unsigned short SNAC_FAM_GEN = 0x0001;
  const unsigned short SNAC_FAM_LOC = 0x0002;
  const unsigned short SNAC_FAM_BUD = 0x0003;
  const unsigned short SNAC_FAM_MSG = 0x0004;
  const unsigned short SNAC_FAM_ADS = 0x0005;
  const unsigned short SNAC_FAM_INV = 0x0006;
  const unsigned short SNAC_FAM_ADM = 0x0007;
  const unsigned short SNAC_FAM_POP = 0x0008;
  const unsigned short SNAC_FAM_BOS = 0x0009;
  const unsigned short SNAC_FAM_LUP = 0x000a;
  const unsigned short SNAC_FAM_STS = 0x000b;
  const unsigned short SNAC_FAM_TRT = 0x000c;
  const unsigned short SNAC_FAM_CNV = 0x000d;
  const unsigned short SNAC_FAM_CHT = 0x000e;

  const unsigned short SNAC_FAM_SBL = 0x0013; // Server-based lists

  const unsigned short SNAC_FAM_SRV = 0x0015; // Server messages
  const unsigned short SNAC_FAM_UIN = 0x0017; // UIN registration

  // ------------- abstract SNAC classes ---------------

  class SNAC {
   protected:
    unsigned short m_flags;
    unsigned int m_requestID;
    
   public:
    SNAC();
    virtual ~SNAC() { }
    
    virtual unsigned short Family() const = 0;
    virtual unsigned short Subtype() const = 0;

    virtual unsigned short Flags() const;
    virtual unsigned int RequestID() const;
    void setRequestID(unsigned int id);
    void setFlags(unsigned short fl);
  };

  // -- Inbound SNAC --
  class InSNAC : virtual public SNAC {
   protected:
    virtual void ParseBody(Buffer& b) = 0;

   public:
    virtual void Parse(Buffer& b);
  };

  // -- Outbound SNAC --
  class OutSNAC : virtual public SNAC {
   protected:
    virtual void OutputHeader(Buffer& b) const;
    virtual void OutputBody(Buffer& b) const = 0;

   public:
    virtual void Output(Buffer& b) const;
  };

  // ------------ Raw SNAC ----------------------------------
  
  class RawSNAC : public InSNAC {
   protected:
    unsigned short m_family, m_subtype;

    void ParseBody(Buffer& b);
    
   public:
    RawSNAC(unsigned short f, unsigned short t);

    unsigned short Family() const { return m_family; }
    unsigned short Subtype() const { return m_subtype; }
  };
  
  Buffer& operator<<(Buffer& b, const ICQ2000::OutSNAC& t);
 
}

#endif
