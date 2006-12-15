/*
 * DirectClient
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

#include "DirectClient.h"

#include "ICQ.h"
#include "constants.h"

#include "sstream_fix.h"

#include <stdlib.h>

using std::string;
using std::ostringstream;
using std::endl;

namespace ICQ2000 {

  unsigned char DirectClient::client_check_data[] = {
    "As part of this software beta version Mirabilis is "
    "granting a limited access to the ICQ network, "
    "servers, directories, listings, information and databases (\""
    "ICQ Services and Information\"). The "
    "ICQ Service and Information may databases (\""
    "ICQ Services and Information\"). The "
    "ICQ Service and Information may\0"
  };

  /*
   * Constructor when receiving an incoming connection
   */
  DirectClient::DirectClient(ContactRef self, TCPSocket *sock, MessageHandler *mh,
			     ContactTree *cl, unsigned int ext_ip, unsigned short server_port)
    : m_state(WAITING_FOR_INIT), m_recv(),
      m_self_contact(self), m_contact(NULL), m_contact_list(cl), 
      m_message_handler(mh), m_incoming(true), m_local_ext_ip(ext_ip),
      m_local_server_port(server_port)
  {
    m_socket = sock;
    Init();
  }

  /*
   * Constructor for making an outgoing connection
   */
  DirectClient::DirectClient(ContactRef self, ContactRef c, MessageHandler *mh, unsigned int ext_ip,
			     unsigned short server_port)
    : m_state(NOT_CONNECTED), m_recv(), m_self_contact(self), 
      m_contact(c), m_message_handler(mh), m_incoming(false), m_local_ext_ip(ext_ip),
      m_local_server_port(server_port)
      
  {
    Init();
    m_socket = new TCPSocket();
    m_remote_uin = c->getUIN();
  }

  DirectClient::~DirectClient()
  {
    m_msgcache.expireAll();
    
    while (!m_msgqueue.empty()) {
      expired_cb( m_msgqueue.front() );
      m_msgqueue.pop_front();
    }

    if ( m_socket->getSocketHandle() > -1) SignalRemoveSocket( m_socket->getSocketHandle() );
    delete m_socket;
  }

  void DirectClient::Init()
  {
    m_seqnum = 0xFFFF;
    m_msgcache.setDefaultTimeout(30);
    m_msgcache.expired.connect( this, &DirectClient::expired_cb) ;
  }

  void DirectClient::Connect() {
    m_remote_tcp_version = m_contact->getTCPVersion();
    if (m_remote_tcp_version >= 7) m_eff_tcp_version = 7;
    else if (m_remote_tcp_version == 6) m_eff_tcp_version = 6;
    else throw DisconnectedException("Cannot direct connect to client with too old TCP version");

    m_socket->setRemoteIP( m_contact->getLanIP() );
    m_socket->setRemotePort( m_contact->getLanPort() );
    m_socket->setBlocking(false);
    m_socket->Connect();
    SignalAddSocket( m_socket->getSocketHandle(), SocketEvent::WRITE );

    if (m_contact->getDCCookie() != 0)
    {
      m_session_id = m_contact->getDCCookie();
    }
    else
    {
      m_session_id = (unsigned int)(0xffffffff*(rand()/(RAND_MAX+1.0)));
    }
    

    m_state = WAITING_FOR_INIT_ACK;
  }

  void DirectClient::FinishNonBlockingConnect()
  {
    SendInitPacket();
  }

  void DirectClient::clearoutMessagesPoll() 
  {
    m_msgcache.clearoutPoll();
  }
  
  void DirectClient::expired_cb(MessageEvent *ev) {
    ev->setFinished(false);
    ev->setDelivered(false);
    ev->setDirect(true);
    messageack.emit(ev);
  }

  void DirectClient::Recv() {
    try {
      while ( m_socket->connected() ) {
	if ( !m_socket->Recv(m_recv) ) break;
	Parse();
      }
    } catch(SocketException e) {
      ostringstream ostr;
      ostr << "Failed on recv: " << e.what();
      throw DisconnectedException( ostr.str() );
    } catch(ParseException e) {
      ostringstream ostr;
      ostr << "Failed parsing: " << e.what();
      throw DisconnectedException( ostr.str() );
    }
  }

  void DirectClient::Parse() {
    if (m_recv.empty()) return;

    unsigned short length;

    while (!m_recv.empty()) {
      m_recv.setPos(0);

      m_recv.setLittleEndian();
      m_recv >> length;
      if (length > Incoming_Packet_Limit) throw ParseException("Received too long incoming packet");
      if (m_recv.remains() < length) return; // waiting for more of the packet


      Buffer sb;
      m_recv.chopOffBuffer( sb, length+2 );

      ostringstream ostr;
      ostr << "Received packet from " << IPtoString( m_socket->getRemoteIP() ) << ":" << m_socket->getRemotePort() << endl << sb;
      SignalLog(LogEvent::DIRECTPACKET, ostr.str());

      if (m_state == WAITING_FOR_INIT) {
	ParseInitPacket(sb);

	if (m_incoming) {
	  SendInitAck();
	  SendInitPacket();
	  m_state = WAITING_FOR_INIT_ACK;
	} else {
	  SendInitAck();
	  if (m_eff_tcp_version == 7) {
	    SendInit2();
	    m_state = WAITING_FOR_INIT2;
	  } else {
	    m_state = CONNECTED;
	    flush_queue();
	    connected.emit(this);
	  }
	}

      } else if (m_state == WAITING_FOR_INIT_ACK) {
	ParseInitAck(sb);

	if (m_incoming) {
	  // Incoming
	  if (m_eff_tcp_version == 7)
	    m_state = WAITING_FOR_INIT2; // v7 has an extra stage of handshaking
	  else {
	    ConfirmUIN();
	    m_state = CONNECTED;          // v5 is done handshaking now
	    flush_queue();
	    connected.emit(this);
	  }

	} else {
	  // Outgoing - next packet should be their INIT
	  m_state = WAITING_FOR_INIT;
	}

      } else if (m_state == WAITING_FOR_INIT2) {
	ParseInit2(sb);
	// This is a V7 only packet

	if (m_incoming) {
	  SendInit2();
	  ConfirmUIN();
	}

	m_state = CONNECTED;
	flush_queue();
	connected.emit(this);

      } else if (m_state == CONNECTED) {
	ParsePacket(sb);
      }


      if (sb.beforeEnd()) {
	/* we assert that parsing code eats uses all data
	 * in the FLAP - seems useful to know when they aren't
	 * as it probably means they are faulty
	 */
	ostringstream ostr;
	ostr  << "Buffer pointer not at end after parsing packet was: 0x" << std::hex << sb.pos()
	      << " should be: 0x" << sb.size();
	SignalLog(LogEvent::WARN, ostr.str());
      }
      
    }
    
  }

  void DirectClient::ConfirmUIN() {
    if ( m_contact_list->exists(m_remote_uin) ) {
      ContactRef c = (*m_contact_list)[ m_remote_uin ];
      if ( (c->getExtIP() == m_local_ext_ip && c->getLanIP() == getIP() )
	   /* They are behind the same masquerading box,
	    * and the Lan IP matches
	    */
	       || c->getExtIP() == getIP()) {
	m_contact = c;
      } else {
	// spoofing attempt most likely
	ostringstream ostr;
	ostr << "Refusing direct connection from someone that claims to be UIN "
	     << m_remote_uin << " since their IP " << IPtoString( getIP() ) << " != " << IPtoString( c->getExtIP() );
	throw DisconnectedException( ostr.str() );
      }
      
    } else {
      // don't accept direct connections from contacts not on contact list
      throw DisconnectedException("Refusing direct connection to contact not on contact list");
    }
  }

  void DirectClient::SendInitPacket() {
    Buffer b;
    b.setLittleEndian();
    Buffer::marker m1 = b.getAutoSizeShortMarker();

    b << (unsigned char)0xff;    // start byte
    b << (unsigned short)0x0007; // tcp version
    Buffer::marker m2 = b.getAutoSizeShortMarker();
    
    b << m_remote_uin;
    b << (unsigned short)0x0000;
    b << (unsigned int)m_local_server_port;

    b << m_self_contact->getUIN();
    b.setBigEndian();
    b << m_local_ext_ip;
    b << m_socket->getLocalIP();
    b << (unsigned char)0x04;    // mode
    b.setLittleEndian();
    b << (unsigned int)m_local_server_port;
    b << m_session_id;

    b << (unsigned int)0x00000050; // unknown
    b << (unsigned int)0x00000003; // unknown
    if (m_eff_tcp_version == 7) 
      b << (unsigned int)0x00000000; // unknown

    b.setAutoSizeMarker(m1);
    b.setAutoSizeMarker(m2);
    
    Send(b);
  }

  void DirectClient::ParseInitPacket(Buffer &b) {
    b.setLittleEndian();
    unsigned short length;
    b >> length;

    unsigned char start_byte;
    b >> start_byte;
    if (start_byte != 0xff) throw ParseException("Init Packet didn't start with 0xff");
    
    unsigned short tcp_version;
    b >> tcp_version;
    b.advance(2); // revision or a length ??

    if (m_incoming) {
      m_remote_tcp_version = tcp_version;
      if (tcp_version <= 5) throw ParseException("Too old client < ICQ99");
      if (tcp_version == 6) m_eff_tcp_version = 6;
      else m_eff_tcp_version = 7;
    } else {
      if (tcp_version != m_remote_tcp_version) throw ParseException("Client claiming different TCP versions");
    }
    
    unsigned int our_uin;
    b >> our_uin;
    if (our_uin != m_self_contact->getUIN()) throw ParseException("Local UIN in Init Packet not same as our Local UIN");

    // 00 00
    // xx xx       senders open port
    // 00 00
    b.advance(6);

    unsigned int remote_uin;
    b >> remote_uin;
    if (m_incoming) {
      m_remote_uin = remote_uin;
    } else {
      if (m_remote_uin != remote_uin) throw ParseException("Remote UIN in Init Packet for Remote Client not what was expected");
    }

    // xx xx xx xx  senders external IP
    // xx xx xx xx  senders lan IP
    b.advance(8);

    b >> m_tcp_flags;

    // xx xx        senders port again
    // 00 00
    b.advance(4);

    // xx xx xx xx  session id
    unsigned int session_id;
    b >> session_id;
    if (m_incoming) {
      m_session_id = session_id;
    } else {
      if (m_session_id != session_id) throw ParseException("Session ID from Remote Client doesn't match the one we sent");
    }

    // 50 00 00 00  unknown 
    // 03 00 00 00  unknown
    b.advance(8);

    if (m_eff_tcp_version == 7) {
      b.advance(4); // 00 00 00 00  unknown
    }

  }

  void DirectClient::ParseInitAck(Buffer &b) {
    b.setLittleEndian();
    unsigned short length;
    b >> length;
    if (length != 4) throw ParseException("Init Ack not as expected");

    unsigned int a;
    b >> a;       // should be 0x00000001 really
  }

  void DirectClient::ParseInit2(Buffer &b) {
    b.setLittleEndian();
    unsigned short length;
    b >> length;
    if (length != 0x0021) throw ParseException("V7 final handshake packet incorrect length");

    unsigned char type;
    b >> type;
    if (type != 0x03) throw ParseException("Expecting V7 final handshake packet, received something else");

    unsigned int unknown;
    b >> unknown // 0x0000000a
      >> unknown;
    b.advance(24); // unknowns
  }

  void DirectClient::SendInit2() {
    Buffer b;
    b.setLittleEndian();
    Buffer::marker m1 = b.getAutoSizeShortMarker();
    b << (unsigned char) 0x03       // start byte
      << (unsigned int)  0x0000000a // unknown
      << (unsigned int)  0x00000001 // unknown
      << (unsigned int)  (m_incoming ? 0x00000001 : 0x00000000) // unknown
      << (unsigned int)  0x00000000 // unknown
      << (unsigned int)  0x00000000; // unknown
    if (m_incoming) {
      b << (unsigned int) 0x00040001 // unknown
	<< (unsigned int) 0x00000000 // unknown
	<< (unsigned int) 0x00000000; // unknown
    } else {
      b << (unsigned int) 0x00000000 // unknown
	<< (unsigned int) 0x00000000 // unknown
	<< (unsigned int) 0x00040001; // unknown
    }
    b.setAutoSizeMarker(m1);
    Send(b);
  }

  void DirectClient::ParsePacket(Buffer& b) {
    Buffer c;
    if (!Decrypt(b, c)) throw ParseException("Decrypting failed");
    ParsePacketInt(c);
  }

  void DirectClient::ParsePacketInt(Buffer& b) {
    b.setLittleEndian();
    unsigned short length;
    b >> length;

    // we should get the decrypted packet in
    unsigned int checksum;
    unsigned short command, seqnum, unknown, version;
    unsigned char junk;
    string msg;
    ostringstream ostr;

    if (m_eff_tcp_version == 7) {
      unsigned char start_byte;
      b >> start_byte;
      if (start_byte != 0x02) throw ParseException("Message Packet didn't start with 0x02");
    }

    b >> checksum
      >> command
      >> unknown // 0x000e or 0x0012 in probe packets
      >> seqnum;

    if (unknown != 0x000e) throw ParseException("Ignoring weird packet");
    
    b.advance(12); // unknown 3 ints

    ICQSubType *i = ICQSubType::ParseICQSubType(b, true, (command == V6_TCP_ACK));
    if (dynamic_cast<UINICQSubType*>(i) == NULL) throw ParseException("Unknown ICQ subtype");
    UINICQSubType *icqsubtype = dynamic_cast<UINICQSubType*>(i);

    icqsubtype->setSeqNum(seqnum);
    icqsubtype->setSource( m_contact->getUIN() );

    if (command == 0) throw ParseException("Invalid TCP Packet");

    switch(command) {

    case V6_TCP_START:
    {
      if (icqsubtype->getType() == MSG_Type_FT)
      {
	FTICQSubType *fticqsubtype = static_cast<FTICQSubType*>( icqsubtype );
	FileTransferEvent *ev = m_message_handler->handleIncomingFT( fticqsubtype, true );
	m_msgcache.insert( seqnum, ev );
      }
      else
      {
	bool ack = m_message_handler->handleIncoming( icqsubtype );
	if (ack) SendPacketAck(icqsubtype);
      }
    }
    break;
    case V6_TCP_ACK:
      if ( m_msgcache.exists(seqnum) )
      {
	MessageEvent *ev = m_msgcache[seqnum];
	ev->setDirect(true);
	m_message_handler->handleIncomingACK( ev, icqsubtype );
	m_msgcache.remove(seqnum);
	if (icqsubtype->getType() != MSG_Type_FT)
	  delete ev;  //WARNING!!! FileTransferEvent shall not be deleted here?
	   
      }
      else
      {
	SignalLog(LogEvent::WARN, "Received Direct ACK for unknown message");
      }      
      break;

    case V6_TCP_CANCEL:
      // FileTransfer was cancelled before we responded
      if ( icqsubtype->getType() == MSG_Type_FT
	   && m_msgcache.exists(seqnum) )
      {
	MessageEvent *ev = m_msgcache[seqnum];
	if ( ev->getType() == MessageEvent::FileTransfer )
	{
	  FileTransferEvent *eev = static_cast<FileTransferEvent*>(ev);
	  m_message_handler->handleIncomingFTCancel( eev );
	  m_msgcache.remove(seqnum);
	}
	
      }
      else
      {
	SignalLog(LogEvent::WARN, "Received Direct Cancel for unknown message");
      }      
      break;
    default:
      ostr << "Unknown TCP Command received 0x" << command;
      throw ParseException( ostr.str() );
    }

    delete icqsubtype;

    b >> junk
      >> version;
  }

  bool DirectClient::Decrypt(Buffer& in, Buffer& out) {

    if (m_eff_tcp_version >= 6) {
      // Huge *thanks* to licq for this code
    
      unsigned long hex, key, B1, M1;
      unsigned int i;
      unsigned char X1, X2, X3;
      unsigned int correction;

      if (m_eff_tcp_version == 7) correction = 3;
      else correction = 2;

      unsigned int size = in.size()-correction;
      
      in.setLittleEndian();
      out.setLittleEndian();

      unsigned short length;
      in >> length;
      out << length;

      if (m_eff_tcp_version == 7) {
	unsigned char start_byte;
	in >> start_byte;
	out << start_byte;
      }

      unsigned int check;
      in >> check;
      out << check;

      // main XOR key
      key = 0x67657268 * size + check;
      
      for(i=4; i<(size+3)/4; i+=4) {
	hex = key + client_check_data[i&0xFF];
	
	out << (unsigned char)(in.UnpackChar() ^ (hex&0xFF));
	out << (unsigned char)(in.UnpackChar() ^ ((hex>>8)&0xFF));
	out << (unsigned char)(in.UnpackChar() ^ ((hex>>16)&0xFF));
	out << (unsigned char)(in.UnpackChar() ^ ((hex>>24)&0xFF));
      }
      
      unsigned char c;
      while (in.remains()) {
	in >> c;
	out << c;
      }

      B1 = (out[4+correction]<<24) | (out[6+correction]<<16) | (out[4+correction]<<8) | (out[6+correction]<<0);
      
      // special decryption
      B1 ^= check;
      
      // validate packet
      M1 = (B1 >> 24) & 0xFF;
      if(M1 < 10 || M1 >= size) return false;

      X1 = out[M1+correction] ^ 0xFF;
      if(((B1 >> 16) & 0xFF) != X1) return false;
      
      X2 = ((B1 >> 8) & 0xFF);
      if(X2 < 220) {
	X3 = client_check_data[X2] ^ 0xFF;
	if((B1 & 0xFF) != X3) return false;
      }
    }

    ostringstream ostr;
    ostr << "Decrypted Direct packet from "  << IPtoString( m_socket->getRemoteIP() ) << ":" << m_socket->getRemotePort() << endl << out;
    SignalLog(LogEvent::DIRECTPACKET, ostr.str());
      
    return true;
  }

  void DirectClient::Encrypt(Buffer& in, Buffer& out) {

    ostringstream ostr;
    ostr << "Unencrypted packet to "  << IPtoString( m_socket->getRemoteIP() ) << ":" << m_socket->getRemotePort() << endl << in;
    SignalLog(LogEvent::DIRECTPACKET, ostr.str());
      
    if (m_eff_tcp_version == 6 || m_eff_tcp_version == 7) {
      // Huge *thanks* to licq for this code
    
      unsigned long hex, key, B1, M1;
      unsigned int i, check;
      unsigned char X1, X2, X3;
      unsigned int size = in.size();

      in.setLittleEndian();
      out.setLittleEndian();

      if (m_eff_tcp_version == 7) {
	// correction for next byte
	out << (unsigned short)(size + 1);
	out << (unsigned char)0x02;
      } else {
	out << (unsigned short)size;
      }

      // calculate verification data
      M1 = (rand() % ((size < 255 ? size : 255)-10))+10;
      X1 = in[M1] ^ 0xFF;
      X2 = rand() % 220;
      X3 = client_check_data[X2] ^ 0xFF;

      B1 = (in[4]<<24)|(in[6]<<16)|(in[4]<<8)|(in[6]);

      // calculate checkcode
      check = (M1 << 24) | (X1 << 16) | (X2 << 8) | X3;
      check ^= B1;

      out << check;

      // main XOR key
      key = 0x67657268 * size + check;
      
      // XORing the actual data
      in.advance(4);
      for(i=4;i<(size+3)/4;i+=4){
	hex = key + client_check_data[i&0xFF];

	out << (unsigned char)(in.UnpackChar() ^ (hex&0xFF));
	out << (unsigned char)(in.UnpackChar() ^ ((hex>>8)&0xFF));
	out << (unsigned char)(in.UnpackChar() ^ ((hex>>16)&0xFF));
	out << (unsigned char)(in.UnpackChar() ^ ((hex>>24)&0xFF));
      }

      unsigned char c;
      while (in.remains()) {
	in >> c;
	out << c;
      }

    }

  }

  void DirectClient::SendInitAck()
  {
    Buffer b;
    b.setLittleEndian();
    Buffer::marker m1 = b.getAutoSizeShortMarker();
    b << (unsigned int)0x00000001;
    b.setAutoSizeMarker(m1);
    Send(b);
  }

  void DirectClient::SendPacketAck(ICQSubType *icqsubtype)
  {
    Buffer b;

    b.setLittleEndian();
    b << (unsigned int)0x00000000 // checksum (filled in by Encrypt)
      << V6_TCP_ACK
      << (unsigned short)0x000e
      << icqsubtype->getSeqNum()
      << (unsigned int)0x00000000
      << (unsigned int)0x00000000
      << (unsigned int)0x00000000;
    
    icqsubtype->Output(b);
    Buffer c;
    Encrypt(b,c);
    Send(c);
  }

  void DirectClient::SendPacketEvent(MessageEvent *ev)
  {
    Buffer b;

    unsigned short seqnum = NextSeqNum();

    UINICQSubType *ist = m_message_handler->handleOutgoing(ev);
    if (ist == NULL) return;

    ist->setAdvanced(true);
    
    b.setLittleEndian();
    b << (unsigned int)0x00000000 // checksum (filled in by Encrypt)
      << V6_TCP_START
      << (unsigned short)0x000e
      << seqnum
      << (unsigned int)0x00000000
      << (unsigned int)0x00000000
      << (unsigned int)0x00000000;
    ist->Output(b);

    Buffer c;
    Encrypt(b,c);
    Send(c);

    // Save seqnum so later ACK or CANCEL could be sent.
    if (ist->getType() == MSG_Type_FT)
    {
      FileTransferEvent *fev = static_cast<FileTransferEvent*>(ev);
      fev->setSeqNum(seqnum);
    }
    
    m_msgcache.insert(seqnum, ev);

    delete ist;
  }

  void DirectClient::Send(Buffer &b) {
    try {
      ostringstream ostr;
      ostr << "Sending packet to "  << IPtoString( m_socket->getRemoteIP() ) << ":" << m_socket->getRemotePort() << endl << b;
      SignalLog(LogEvent::DIRECTPACKET, ostr.str());
      m_socket->Send(b);
    } catch(SocketException e) {
      ostringstream ostr;
      ostr << "Failed to send: " << e.what();
      throw DisconnectedException( ostr.str() );
    }
  }


  void DirectClient::SendFTACK(FileTransferEvent *ev)
  {
    FTICQSubType icqsubtype;
    icqsubtype.setAdvanced(true);
    icqsubtype.setSeqNum(ev->getSeqNum());
    icqsubtype.setACK(true);
    if (ev->getState() == FileTransferEvent::ACCEPTED)
    {
      icqsubtype.setPort(ev->getPort());
      icqsubtype.setRevPort(ev->getPort());
      icqsubtype.setSize(0);
      icqsubtype.setMessage("");
	    
      // port numbers, etc..
    }
    else
    {
      icqsubtype.setStatus( AcceptStatus_Denied );
      icqsubtype.setPort(0);
      icqsubtype.setSize(0);
      icqsubtype.setMessage( ev->getRefusalMessage() );
    }
    SendPacketAck(&icqsubtype);
    while (m_msgcache.exists(ev->getSeqNum()))
    {
      m_msgcache.remove(ev->getSeqNum());
    }
  }

  void DirectClient::SendFTCancel(FileTransferEvent *ev)
  {
    FTICQSubType fist;
    fist.setAdvanced(true);
    fist.setSeqNum(ev->getSeqNum());

    fist.setPort(0);
    fist.setSize(0);
    fist.setMessage( "" );

    Buffer b;
    b.setLittleEndian();
    b << (unsigned int)0x00000000 // checksum (filled in by Encrypt)
      << V6_TCP_CANCEL
      << (unsigned short)0x000e
      << fist.getSeqNum()
      << (unsigned int)0x00000000
      << (unsigned int)0x00000000
      << (unsigned int)0x00000000;
    fist.Output(b);

    Buffer c;
    Encrypt(b,c);
    Send(c);

    m_msgcache.remove(ev->getSeqNum());
  }

  void DirectClient::SendEvent(MessageEvent *ev)
  {

    if (m_state == CONNECTED) {
      // send straight away
      SendPacketEvent(ev);
    } else {
      // queue message
      m_msgqueue.push_back(ev);
    }

  }

  void DirectClient::flush_queue() {
    while (!m_msgqueue.empty()) {
      SendPacketEvent( m_msgqueue.front() );
      m_msgqueue.pop_front();
    }
  }

  unsigned short DirectClient::NextSeqNum() {
    return m_seqnum--;
  }

  unsigned int DirectClient::getUIN() const { return m_remote_uin; }

  unsigned int DirectClient::getIP() const { return m_socket->getRemoteIP(); }

  unsigned short DirectClient::getPort() const { return m_socket->getRemotePort(); }

  int DirectClient::getfd() const { return m_socket->getSocketHandle(); }

  TCPSocket* DirectClient::getSocket() const { return m_socket; }

  void DirectClient::setContact(ContactRef c) { m_contact = c; }

  ContactRef DirectClient::getContact() const { return m_contact; }

}
