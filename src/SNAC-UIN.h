/*
 * SNAC - UIN
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

#ifndef SNAC_UIN_H
#define SNAC_UIN_H

#include <string>

#include "SNAC-base.h"

namespace ICQ2000 {

  // UIN Registration (Family 0x0017)
  const unsigned short SNAC_UIN_RequestError = 0x0001;
  const unsigned short SNAC_UIN_Request = 0x0004;
  const unsigned short SNAC_UIN_Response = 0x0005;

  // --------------------- UIN Registration (Family 0x0017) SNACs ---------

  class UINFamilySNAC : virtual public SNAC {
   public:
    unsigned short Family() const { return SNAC_FAM_UIN; }
  };

  class UINRequestSNAC : public UINFamilySNAC, public OutSNAC {
   protected:
    std::string m_password;
    
    void OutputBody(Buffer& b) const;

   public:
    UINRequestSNAC(const std::string& p);

    unsigned short Subtype() const { return SNAC_UIN_Request; }
   };
 
   class UINRequestErrorSNAC : public UINFamilySNAC, public InSNAC {
    protected:
     void ParseBody(Buffer& b);
     
    public:
     UINRequestErrorSNAC();
     
     unsigned short Subtype() const { return SNAC_UIN_RequestError; }
  };

  class UINResponseSNAC : public UINFamilySNAC, public InSNAC {
   protected:
    unsigned int m_uin;
    
    void ParseBody(Buffer& b);
    
   public:
    UINResponseSNAC();

    unsigned short Subtype() const { return SNAC_UIN_Response; }
    unsigned int getUIN() const { return m_uin; }
  };

}

#endif
