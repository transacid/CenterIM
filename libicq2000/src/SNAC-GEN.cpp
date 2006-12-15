/*
 * SNAC - General Family
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

#include "SNAC-GEN.h"

#include "TLV.h"
#include "buffer.h"

namespace ICQ2000 {

  // --------------- Generic (Family 0x0001) ------------

  void ServerReadySNAC::ParseBody(Buffer& b) {
    /* The body of the server ready SNAC seems
     * to be a list of the SNAC families the server
     * will accept - the client is then expected
     * to send back a list of those it wants
     * - basically ignore this for the moment :-)
     */
    unsigned short cap;
    while(b.beforeEnd())
      b >> cap;

  }

  void RequestRateInfoSNAC::OutputBody(Buffer& b) const {
    // empty
  }

  void RateInfoSNAC::ParseBody(Buffer& b) {

    // shamelessly not parsing any of this :-(
    b.advance(179);
    unsigned short n;
    b >> n;
    for (unsigned short a = 0; a < n; a++) {
      unsigned short major, minor;
      b >> major
	>> minor;
    }

    b.advance(68);
    
  }

  void RateInfoAckSNAC::OutputBody(Buffer& b) const {
    b << (unsigned short)0x0001
      << (unsigned short)0x0002
      << (unsigned short)0x0003
      << (unsigned short)0x0004
      << (unsigned short)0x0005;
  }

  void RateInfoChangeSNAC::ParseBody(Buffer& b) {
    b >> m_code;
    b >> m_rateclass;
    b >> m_windowsize;
    b >> m_clear;
    b >> m_alert;
    b >> m_limit;
    b >> m_disconnect;
    b >> m_currentavg;
    b >> m_maxavg;
    b.advance(5);
  }

  void CapabilitiesSNAC::OutputBody(Buffer& b) const {
    /* doesn't seem any need currently to do more
     * than copy the official client
     */
    unsigned short v1 = 0x0001, v3 = 0x0003;
    b << SNAC_FAM_GEN << v3
      << SNAC_FAM_LOC << v1
      << SNAC_FAM_BUD << v1
      << SNAC_FAM_SRV << v1
      << SNAC_FAM_MSG << v1
      << SNAC_FAM_INV << v1
      << SNAC_FAM_BOS << v1
      << SNAC_FAM_LUP << v1;
  }

  void CapAckSNAC::ParseBody(Buffer& b) {
    /* server sends back the list from ServerReady again
     * but with versions of families included
     * - again ignore for the moment
     */
    unsigned short cap, ver;
    while(b.beforeEnd()) {
      b >> cap
	>> ver;
    }

  }

  SetStatusSNAC::SetStatusSNAC(unsigned short status, bool web_aware)
    : m_status(status), m_sendextra(false), m_web_aware(web_aware) { }

  void SetStatusSNAC::OutputBody(Buffer& b) const {
    StatusTLV stlv(ALLOWDIRECT_EVERYONE, (m_web_aware ? WEBAWARE_WEBAWARE : WEBAWARE_NORMAL), m_status);
    b << stlv;
    if (m_sendextra) {
      UnknownTLV utlv;
      b << utlv;
      LANDetailsTLV ltlv(m_ip, m_port);
      b << ltlv;
    }
  }

  void SetStatusSNAC::setSendExtra(bool b) { m_sendextra = b; }

  void SetStatusSNAC::setIP(unsigned int ip) { m_ip = ip; }

  void SetStatusSNAC::setPort(unsigned short port) { m_port = port; }

  void SetIdleSNAC::OutputBody(Buffer& b) const {
    /* don't know what this value means exactly */
    b << (unsigned int)0x00000000;
  }

  void ClientReadySNAC::OutputBody(Buffer& b) const {
    /* related to capabilities again
     * - figure this out sometime
     */
    b << 0x00010003
      << 0x0110028a
      << 0x00020001
      << 0x0101028a
      << 0x00030001
      << 0x0110028a
      << 0x00150001
      << 0x0110028a
      << 0x00040001
      << 0x0110028a
      << 0x00060001
      << 0x0110028a
      << 0x00090001
      << 0x0110028a
      << 0x000a0001
      << 0x0110028a;
  }

  void PersonalInfoRequestSNAC::OutputBody(Buffer& b) const {
    // empty
  }

  void UserInfoSNAC::ParseBody(Buffer& b) {
    m_userinfo.Parse(b);
  }

  void MOTDSNAC::ParseBody(Buffer& b) {
    b >> m_status;

    /* as far as I know only one TLV follows,
     * but we'll treat it as a list
     */
    TLVList tlvlist;
    tlvlist.Parse(b, TLV_ParseMode_Channel02, (short unsigned int)-1);
    if (tlvlist.exists(TLV_WebAddress)) {
      WebAddressTLV *t = (WebAddressTLV*)tlvlist[TLV_WebAddress];
      m_url = t->Value();
    }
  }

}
