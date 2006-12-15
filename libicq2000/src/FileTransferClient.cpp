/*
 * FileTransferClient
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

#include "FileTransferClient.h"

#include "ICQ.h"
#include "constants.h"

#include "sstream_fix.h"

#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <iostream>
#include <algorithm>

using std::string;
using std::ostringstream;
using std::endl;
using std::ofstream;
using std::min;
using std::max;

namespace ICQ2000
{
  /*
   * Constructor when receiving an incoming connection
   */
  FileTransferClient::FileTransferClient(ContactRef self, MessageHandler *mh,
					 ContactTree *cl, unsigned int ext_ip, FileTransferEvent* ev)
    : m_state(WAITING_FOR_INIT), m_recv(),
      m_self_contact(self), m_contact(NULL), m_contact_list(cl), 
      m_message_handler(mh), m_incoming(true), m_local_ext_ip(ext_ip),
      m_ev(ev)
  {
    Init();
    m_listenserver.StartServer();
  }

  /*
   * Constructor for making an outgoing connection
   */
  FileTransferClient::FileTransferClient(ContactRef self, ContactRef c, MessageHandler *mh, unsigned int ext_ip, FileTransferEvent* ev)
    : m_state(NOT_CONNECTED), m_recv(), m_self_contact(self), 
      m_contact(c), m_message_handler(mh), m_incoming(false), m_local_ext_ip(ext_ip),
      m_ev(ev)
      
  {
    Init();
    m_socket = new TCPSocket();
    m_remote_uin = c->getUIN();
  }

  FileTransferClient::~FileTransferClient()
  {
    if (m_listenserver.isStarted())
    {
      SignalRemoveSocket( m_listenserver.getSocketHandle() );
      m_listenserver.Disconnect();
    }

    if ( m_socket->getSocketHandle() > -1) 
      SignalRemoveSocket( m_socket->getSocketHandle() );

    delete m_socket;
    
    if (m_fout.is_open()) m_fout.close();
    if (m_fin.is_open()) m_fin.close();
  }

  void FileTransferClient::Init()
  {
    m_more = false;
    m_continue = false;
    m_msgqueue = false;
    m_senddir = false;
    m_base_dir = NULL;
    m_timestamp = time(NULL);
    m_timeout = 60;  // will time out in 60 seconds
  }

  void FileTransferClient::Connect()
  {
    m_remote_tcp_version = m_contact->getTCPVersion();
    if (m_remote_tcp_version >= 7) m_eff_tcp_version = 7;
    else if (m_remote_tcp_version == 6) m_eff_tcp_version = 6;
    else throw DisconnectedException("Cannot direct connect to client with too old TCP version");

    m_socket->setRemoteIP( m_contact->getLanIP() );
    m_socket->setRemotePort( m_ev->getPort() );
    m_socket->setBlocking(false);
    m_socket->Connect();
    SignalAddSocket( m_socket->getSocketHandle(), SocketEvent::WRITE );

    if (m_contact->getDCCookie() != 0)
      m_session_id = m_contact->getDCCookie();
    else
      m_session_id = (unsigned int)(0xffffffff*(rand()/(RAND_MAX+1.0)));

    m_state = WAITING_FOR_INIT_ACK;
  }

  void FileTransferClient::FinishNonBlockingConnect()
  {
    SendInitPacket();
  }

  void FileTransferClient::clearoutMessagesPoll() 
  {
    if ((m_state != CONNECTED) &&
	(time(NULL) > (m_timestamp + m_timeout)))
      expired();
  }
 
  void FileTransferClient::SendFile() {
    if (m_state == CONNECTED && m_msgqueue)
    {
      SendPacket0x00();
      m_msgqueue = false;
    }
    else if (m_continue)
    {
      if (m_more)
      {
	SendPacket0x06();
      }
      else if (m_ev->getCurrFile() < m_ev->getTotalFiles())
      {
	SendPacket0x02();
	m_continue = false;
	m_more = false;
      }
      else
      {
	m_ev->setState(FileTransferEvent::COMPLETE);
	SignalLog(LogEvent::INFO, "FileTransfer is Complete");
	throw DisconnectedException("FileTransfer is Complete");

	m_continue = false;
      }
    }
    m_message_handler->handleUpdateFT(m_ev);
  }
  
  void FileTransferClient::expired()
  {
    m_ev->setFinished(false);
    m_ev->setDelivered(false);
    m_ev->setDirect(true);
    m_ev->setState(FileTransferEvent::TIMEOUT);
    messageack.emit(m_ev);
  }

  void FileTransferClient::Recv()
  {
    try
    {
      while ( m_socket->connected() )
      {
	if ( !m_socket->Recv(m_recv) ) break;
	Parse();
      }
    }
    catch(SocketException e)
    {
      ostringstream ostr;
      ostr << "Failed on recv: " << e.what();
      throw DisconnectedException( ostr.str() );
    }
    catch(ParseException e)
    {
      ostringstream ostr;
      ostr << "Failed parsing: " << e.what();
      throw DisconnectedException( ostr.str() );
    }
  }

  void FileTransferClient::Parse()
  {
    if (m_recv.empty()) return;

    unsigned short length;

    while (!m_recv.empty())
    {
      m_recv.setPos(0);

      m_recv.setLittleEndian();
      m_recv >> length;
      if (length > Incoming_Packet_Limit) throw ParseException("Received too long incoming packet");
      if (length == 0) return;
      if (m_recv.remains() < length) return; // waiting for more of the packet

      // Optimize??? not create new buffer and copy and erase in old.
      Buffer sb;
      m_recv.chopOffBuffer( sb, length+2 );
	 
      if (m_state != CONNECTED)
      {
	ostringstream ostr;
	ostr << "Received filepacket from " << IPtoString( m_socket->getRemoteIP() ) << ":" << m_socket->getRemotePort() << endl << sb;
	SignalLog(LogEvent::DIRECTPACKET, ostr.str());
      }
      
      if (m_state == WAITING_FOR_INIT)
      {
	ParseInitPacket(sb);

	if (m_incoming) {
	  SendInitAck();
	  SendInitPacket();
	  m_state = WAITING_FOR_INIT_ACK;
	} else {
	  SendInitAck();
	  m_state = CONNECTED;
	  flush_queue();
	  connected.emit(this);
	}

      }
      else if (m_state == WAITING_FOR_INIT_ACK)
      {
	ParseInitAck(sb);

	if (m_incoming) {
	  // Incoming
	  ConfirmUIN();
	  m_state = CONNECTED;          // v5 is done handshaking now
	  flush_queue();
	  connected.emit(this);

	}
	else
	{
	  // Outgoing - next packet should be their INIT
	  m_state = WAITING_FOR_INIT;
	}

      }
      else if (m_state == WAITING_FOR_INIT2)
      {
	ParseInit2(sb);
	// This is a V7 only packet

	if (m_incoming)
	{
	  SendInit2();
	  ConfirmUIN();
	}

	m_state = CONNECTED;
	flush_queue();
	connected.emit(this);

      }
      else if (m_state == CONNECTED)
      {
	ParsePacket(sb);
      }

      if (sb.beforeEnd())
      {
	ostringstream ostr;
	ostr  << "Buffer pointer not at end after parsing packet was: 0x" << std::hex << sb.pos()
	      << " should be: 0x" << sb.size();
	SignalLog(LogEvent::WARN, ostr.str());
      }
      
    }
    
  }

  void FileTransferClient::ConfirmUIN()
  {
    if ( m_contact_list->exists(m_remote_uin) )
    {
      ContactRef c = (*m_contact_list)[ m_remote_uin ];
      if ( (c->getExtIP() == m_local_ext_ip && c->getLanIP() == getIP() )
	   /* They are behind the same masquerading box,
	    * and the Lan IP matches
	    */
	   || c->getExtIP() == getIP())
      {
	m_contact = c;
      }
      else
      {
	// spoofing attempt most likely
	ostringstream ostr;
	ostr << "Refusing direct connection from someone that claims to be UIN "
	     << m_remote_uin << " since their IP " << IPtoString( getIP() ) << " != " << IPtoString( c->getExtIP() );
	throw DisconnectedException( ostr.str() );
      }
      
    }
    else
    {
      // don't accept direct connections from contacts not on contact list
      throw DisconnectedException("Refusing direct connection to contact not on contact list");
    }
  }

  void FileTransferClient::SendInitPacket()
  {
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

  void FileTransferClient::ParseInitPacket(Buffer &b)
  {
    b.setLittleEndian();
    unsigned short length;
    b >> length;

    unsigned char start_byte;
    b >> start_byte;
    if (start_byte != 0xff) throw ParseException("Init Packet didn't start with 0xff");
    
    unsigned short tcp_version;
    b >> tcp_version;
    b.advance(2); // revision or a length ??

    if (m_incoming)
    {
      m_remote_tcp_version = tcp_version;
      if (tcp_version <= 5) throw ParseException("Too old client < ICQ99");
      if (tcp_version == 6) m_eff_tcp_version = 6;
      else m_eff_tcp_version = 7;
    }
    else
    {
      if (tcp_version != m_remote_tcp_version)
	throw ParseException("Client claiming different TCP versions");
    }
    
    unsigned int our_uin;
    b >> our_uin;
    if (our_uin != m_self_contact->getUIN())
      throw ParseException("Local UIN in Init Packet not same as our Local UIN");

    // 00 00
    // xx xx       senders open port
    // 00 00
    b.advance(6);

    unsigned int remote_uin;
    b >> remote_uin;
    if (m_incoming)
    {
      m_remote_uin = remote_uin;
    }
    else
    {
      if (m_remote_uin != remote_uin)
	throw ParseException("Remote UIN in Init Packet for Remote Client not what was expected");
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
    if (m_incoming)
    {
      m_session_id = session_id;
    }
    else
    {
      if (m_session_id != session_id) throw ParseException("Session ID from Remote Client doesn't match the one we sent");
    }

    // 50 00 00 00  unknown 
    // 03 00 00 00  unknown
    b.advance(8);

    if (m_eff_tcp_version == 7)
    {
      b.advance(4); // 00 00 00 00  unknown
    }

  }

  void FileTransferClient::ParseInitAck(Buffer &b)
  {
    b.setLittleEndian();
    unsigned short length;
    b >> length;
    if (length != 4) throw ParseException("Init Ack not as expected");

    unsigned int a;
    b >> a;       // should be 0x00000001 really
  }

  void FileTransferClient::ParseInit2(Buffer &b)
  {
    b.setLittleEndian();
    unsigned short length;
    b >> length;
    if (length != 0x0021)
      throw ParseException("V7 final handshake packet incorrect length");

    unsigned char type;
    b >> type;
    if (type != 0x03)
      throw ParseException("Expecting V7 final handshake packet, received something else");

    unsigned int unknown;
    b >> unknown // 0x0000000a
      >> unknown;// 0x00000001 on genuine connections, otherwise some weird connections which we drop
    if (unknown != 0x00000001) throw DisconnectedException("Ignoring weird direct connection");
    b.advance(24); // unknowns
  }

  void FileTransferClient::SendInit2()
  {
    Buffer b;
    b.setLittleEndian();
    Buffer::marker m1 = b.getAutoSizeShortMarker();
    b << (unsigned char) 0x03       // start byte
      << (unsigned int)  0x0000000a // unknown
      << (unsigned int)  0x00000001 // unknown
      << (unsigned int)  (m_incoming ? 0x00000001 : 0x00000000) // unknown
      << (unsigned int)  0x00000000 // unknown
      << (unsigned int)  0x00000000; // unknown
    if (m_incoming)
    {
      b << (unsigned int) 0x00040001 // unknown
	<< (unsigned int) 0x00000000 // unknown
	<< (unsigned int) 0x00000000; // unknown
    }
    else
    {
      b << (unsigned int) 0x00000000 // unknown
	<< (unsigned int) 0x00000000 // unknown
	<< (unsigned int) 0x00040001; // unknown
    }
    b.setAutoSizeMarker(m1);
    Send(b);
  }

  void FileTransferClient::ParsePacket(Buffer& b)
  {
    b.setLittleEndian();
    unsigned short length;

    b >> length;
    
    unsigned char type;
    b >> type;

    switch (type)
    {
    case 0: ParsePacket0x00(b); 
      SendPacket0x05();      
      SendPacket0x01();
      break;
    case 1: ParsePacket0x01(b);
      SendPacket0x02();
      break;
    case 2: ParsePacket0x02(b);
      SendPacket0x03(0,m_ev->getCurrFile());
      break;
    case 3: ParsePacket0x03(b);
      m_continue = true;
      SendFile();
      break;
    case 5: ParsePacket0x05(b);
      break;
    case 6: ParsePacket0x06(b);
      break;
    default: 
      SignalLog(LogEvent::WARN, "Received unknown FileTransfer Packet");
    }
  }

  void FileTransferClient::ParsePacket0x00(Buffer& b)
  {
    unsigned int x1, t_num_files, t_size, speed;
    string nick;

    b >> x1
      >> t_num_files
      >> t_size
      >> speed;
    b.UnpackUint16StringNull(nick);

    m_ev->setTotalSize(t_size);
    m_ev->setTotalFiles(t_num_files);
    m_ev->setCurrFile(0);
    m_ev->setSpeed(speed);

    m_path = m_ev->getSavePath();       
  }

  void FileTransferClient::ParsePacket0x01(Buffer& b)
  {
    unsigned int speed;
    std::string nick;

    b >> speed;
    b.UnpackUint16StringNull(nick);

    m_ev->setSpeed(speed);
  }
	
  void FileTransferClient::ParsePacket0x02(Buffer& b)
  {
    unsigned int size, speed;
    string filename, subdir;
    unsigned char file_or_dir, tmp1;
    
    b >> file_or_dir;
    b.UnpackUint16StringNull(filename);
    b.UnpackUint16StringNull(subdir);
    b >> size;
    b >> tmp1;
    b >> tmp1;
    b >> tmp1;
    b >> tmp1;
    
    b >> speed;
    
    m_ev->setSize(size);
    m_ev->setPos(0);
    m_ev->setCurrFile(m_ev->getCurrFile()+1);
    m_ev->setSpeed(speed);

    m_path = m_ev->getSavePath();       
    // Add backslash if missing in path
    if (m_path.find_last_of('/') != (m_path.length()-1))
    {
      m_path += "/";
    }

    int pos = 0;
    while ((pos = subdir.find('\\')) != std::string::npos)
    {
      subdir[pos] = '/';
    }
    
    m_path += subdir;
    // Add backslash if missing in path again
    if (m_path.find_last_of('/') != (m_path.length()-1))
    {
      m_path += "/";
    }

    if (file_or_dir == 1)
    {
      m_path += filename;
      m_path += "/";
      mkdir(m_path.c_str(), S_IXUSR | 
	    S_IWUSR | 
	    S_IRUSR |
	    S_IXGRP |
	    S_IRGRP |
	    S_IXOTH | 
	    S_IROTH);
    }
    else
    {
      if (m_ev->getCurrFile() > 1)
      {
	if (m_fout.is_open())
	{
	  m_fout.close();
	}
      }

      m_fout.open(std::string(m_path+filename).c_str(), std::ios::out | std::ios::binary);
      if (!m_fout.good())
      {
	ostringstream ostr;
	ostr << "Opening "
	     << m_path << filename
	     << " for writing.";
	m_ev->setState(FileTransferEvent::ERROR);
	m_ev->setError(ostr.str());
	throw DisconnectedException("I/O error");
      }
    }

    // Notify the Gui that an new file is on it's way.. 
    m_message_handler->handleUpdateFT(m_ev);
  }

  void FileTransferClient::ParsePacket0x03(Buffer& b)
  {
    unsigned int npos, nfiles, x1, x2;
    
    b >> npos
      >> x1
      >> x2
      >> nfiles;

    // Resume is handled
    m_ev->setPos(npos);
    m_ev->setTotalPos(m_ev->getTotalPos()+npos);
    m_ev->setCurrFile(nfiles);

    if (npos > m_ev->getSize())
    {
      ostringstream ostr;
      ostr << "FileTransfer resume request got wrong offset: "
	   << npos << " when file only is " << m_ev->getSize();
      m_ev->setState(FileTransferEvent::ERROR);
      m_ev->setError(ostr.str());
      throw DisconnectedException("I/O error");
    }
    else
    {
      // We support resume
      ostringstream ostr;
      ostr << "Resuming filetransfer " << npos << "/" << m_ev->getSize();
      SignalLog(LogEvent::INFO, ostr.str());
      m_fin.seekg(npos);
    }

    if (!m_senddir)
    {
      m_more = true;
    }
  }
	
  void FileTransferClient::ParsePacket0x05(Buffer& b)
  {
    unsigned int speed;
    
    b >> speed;
    m_ev->setSpeed(speed);
  }
	
  void FileTransferClient::ParsePacket0x06(Buffer& b)
  {
    unsigned short length;
    length = b.remains();

    m_ev->setPos(m_ev->getPos()+length);
    m_ev->setTotalPos(m_ev->getTotalPos()+length);

    while ( length>0 )
    {
	b.Unpack( m_const_buf, min(length, (unsigned short)MAX_FileChunk) );
	m_fout.write((const char*)m_const_buf, std::min(length, (unsigned short)MAX_FileChunk) );
	length = max(0, length-(unsigned short)MAX_FileChunk);
    }
    m_fout.flush();  //prob. isn't needed.. but I feel safer ;)

   if (m_ev->getTotalPos() >= m_ev->getTotalSize())
    {
      m_ev->setState(FileTransferEvent::COMPLETE);
      throw DisconnectedException("FileTransfer is Complete");
    }
  }
  
  void FileTransferClient::SendPacket0x00()
  {
    Buffer b;
    unsigned int tmp_speed = m_ev->getSpeed();
    unsigned int tmp_tot_nr = m_ev->getTotalFiles();
    unsigned int tmp_tot_size = m_ev->getTotalSize();
    b.setLittleEndian();
    
    Buffer::marker m1 = b.getAutoSizeShortMarker();
    b << (unsigned char)0x00;
    b << (unsigned int)0x00000000;   //0x00000000 X1
    b << tmp_tot_nr;  //total number of files to send   CHANGE!!!!
    b << tmp_tot_size;  //total number of bytes to send   CHANGE!!!!
    b << tmp_speed;
    b.PackUint16StringNull(m_self_contact->getAlias());
    b.setAutoSizeMarker(m1);

    Send(b);
  }

  void FileTransferClient::SendPacket0x01()
  {
    Buffer b;
    unsigned int tmp_speed = m_ev->getSpeed();
    b.setLittleEndian();
    
    Buffer::marker m1 = b.getAutoSizeShortMarker();
    b << (unsigned char)0x01;
    b << tmp_speed;   //Speed   CHANGE!!!!
    b.PackUint16StringNull(m_self_contact->getAlias());
    b.setAutoSizeMarker(m1);
    
    Send(b);
  }

  void FileTransferClient::SendPacket0x02()
  {
    Buffer b;
    string tmp_name = m_ev->getFile();
    unsigned int tmp_size;
    unsigned int tmp_speed = m_ev->getSpeed();
    struct stat tmp_stat;
    unsigned char file_or_dir;
    
    stat(tmp_name.c_str(), &tmp_stat);
    if (S_ISDIR(tmp_stat.st_mode))
    {
      file_or_dir = 1;
      tmp_size = 0;
      m_senddir = true;
	 
      //setting base_dir
      if (m_base_dir == NULL)
      {
	m_base_dir = new std::string(tmp_name.substr(0, tmp_name.find_last_of('/', tmp_name.find_last_not_of('/'))+1)); 
      }
	 
    }
    else if (S_ISREG(tmp_stat.st_mode))
    {
      file_or_dir = 0;
      tmp_size = tmp_stat.st_size;
	 
      //setting base_dir
      if (m_base_dir == NULL)
      {
	m_base_dir = new std::string("");
      }

      m_senddir = false;
		 
      if (m_fin.is_open())
      {
	m_fin.clear();
	m_fin.close();
      }
	 
      m_fin.open(tmp_name.c_str(), std::ios::in | std::ios::binary);
      if (!m_fin.good())
      {
	ostringstream ostr;
	ostr << "Opening "
	     << tmp_name
	     << " for Reading.";
	    
	m_ev->setState(FileTransferEvent::ERROR);
	m_ev->setError(ostr.str());
	throw DisconnectedException("I/O error");
      }
      
    }
    else
    {
      ostringstream ostr;
      ostr  << "Trying to send unknown file type: "<< tmp_name;
      SignalLog(LogEvent::WARN, ostr.str());
      return;
    }

    std::string subdir = "";
    
    if (m_base_dir->length() != 0)
      subdir =
      tmp_name.substr(m_base_dir->length(),
		      tmp_name.find_last_of('/', tmp_name.find_last_not_of('/'))); 

    int pos = 0;
    while ((pos = subdir.find('/')) != std::string::npos)
      subdir[pos] = '\\';

    // Chopping of so just name is sent.
    pos = tmp_name.find_last_of('/');
    if (pos == tmp_name.length()-1)
    {
      tmp_name = tmp_name.substr(0,pos);
      pos = tmp_name.find_last_of('/');
    }
    tmp_name = tmp_name.substr(pos+1, tmp_name.length()-pos);

    // Chopping of filename
    if (m_base_dir->length() != 0)
      subdir = subdir.substr(0, subdir.length() - tmp_name.length() -1);

    
    // Remove last backslash from subdir
    if ((file_or_dir == 1) &&
	(subdir.find_last_of('\\') == (subdir.length()-1)))
      subdir = subdir.substr(0, subdir.length() -1);

    m_ev->setCurrFile(m_ev->getCurrFile()+1);
    m_ev->setPos(0);
    m_ev->setSize(tmp_size);
    
    b.setLittleEndian();
    
    Buffer::marker m1 = b.getAutoSizeShortMarker();
    b << (unsigned char)0x02;
    b << file_or_dir;
    b.PackUint16StringNull(tmp_name);  //name of next file.
    b.PackUint16StringNull(subdir);  //subdir
    b << tmp_size;
    b << (unsigned char)0x00;
    b << (unsigned char)0x00;
    b << (unsigned char)0x00;
    b << (unsigned char)0x00;
    b << tmp_speed;
    b.setAutoSizeMarker(m1);

    Send(b);
  }
  
  void FileTransferClient::SendPacket0x03(unsigned int npos, unsigned int nfiles)
  {
    Buffer b;
    unsigned int speed = m_ev->getSpeed();
    b.setLittleEndian();
    Buffer::marker m1 = b.getAutoSizeShortMarker();
    b << (unsigned char)0x03;  
    b << npos;  //filepos  RESUME?
    b << (unsigned int)0x00000000;   // X1 Unknown
    b << speed; //(unsigned int)0x00000064;  // speed
    b << nfiles;

    b.setAutoSizeMarker(m1);

    Send(b);
  }

  void FileTransferClient::SendPacket0x05()
  {
    Buffer b;
    unsigned int speed;
    b.setLittleEndian();
    Buffer::marker m1 = b.getAutoSizeShortMarker();

    speed = m_ev->getSpeed();
    
    b << (unsigned char)0x05;
    b << speed;

    b.setAutoSizeMarker(m1);

    Send(b);
  }

  void FileTransferClient::SendPacket0x06()
  {
    Buffer b;
    unsigned short length;
    b.setLittleEndian();
    Buffer::marker m1 = b.getAutoSizeShortMarker();

    if (!m_fin.good())
    {
      m_ev->setState(FileTransferEvent::ERROR);
      m_ev->setError("I/O error while sending data");
      throw DisconnectedException("I/O error in SendPacket0x06");
    }

    m_fin.read((char*)m_const_buf, 2048);
    length = m_fin.gcount();

    b << (unsigned char)0x06;
    
    b.Pack(m_const_buf, length);
    b.setAutoSizeMarker(m1);
    
    m_ev->setPos(m_ev->getPos()+length);
    m_ev->setTotalPos(m_ev->getTotalPos()+length);
    
    Send(b);
    
    // Send more?
    if (m_ev->getPos() < m_ev->getSize())
    {
      m_more = true;
    }
    else
    {
      m_more = false;
      if (m_ev->getFilesInQueue() == 0)
      {
	m_ev->setState(FileTransferEvent::COMPLETE);
	SignalLog(LogEvent::INFO, "FileTransfer is Complete");
	throw DisconnectedException("FileTransfer is Complete");
      }
    }
  }
 
  void FileTransferClient::SendInitAck()
  {
    Buffer b;
    b.setLittleEndian();
    Buffer::marker m1 = b.getAutoSizeShortMarker();
    b << (unsigned int)0x00000001;
    b.setAutoSizeMarker(m1);
    Send(b);
  }

  void FileTransferClient::SendPacketAck(ICQSubType *icqsubtype)
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
    //Encrypt(b,c);
    Send(c);
  }

  void FileTransferClient::Send(Buffer &b)
  {
    try
    {
      m_socket->Send(b);
    }
    catch(SocketException e)
    {
      ostringstream ostr;
      ostr << "Failed to send: " << e.what();
      throw DisconnectedException( ostr.str() );
    }
  }

  void FileTransferClient::SendEvent(MessageEvent *ev)
  {
    if (m_state == CONNECTED)
    {
      // send straight away
      SendFile();
    }
    else
    {
      // queue message
      m_msgqueue = true;
    }
  }

  void FileTransferClient::flush_queue()
  {
    if (m_msgqueue)
    {
      SendFile();
    }
  }

  unsigned int FileTransferClient::getUIN() const { return m_remote_uin; }

  unsigned int FileTransferClient::getIP() const { return m_socket->getRemoteIP(); }

  unsigned short FileTransferClient::getPort() const { return m_socket->getRemotePort(); }

  unsigned short FileTransferClient::getlistenPort() const { return m_listenserver.getPort(); }
  
  int FileTransferClient::getfd() const { return m_socket->getSocketHandle(); }
  
  int FileTransferClient::getlistenfd() { return m_listenserver.getSocketHandle(); }
	
  TCPSocket* FileTransferClient::getSocket() const { return m_socket; }
  
  void FileTransferClient::setSocket()
  {
    m_socket = m_listenserver.Accept();
    SignalLog(LogEvent::INFO, "Accepting incoming filetransfer");
  }
  
  void FileTransferClient::setContact(ContactRef c) { m_contact = c; }

  ContactRef FileTransferClient::getContact() const { return m_contact; }

  bool FileTransferClient::SetupFileTransfer(FileTransferEvent *ev)
  {
    struct stat tmp_stat;
    std::string str = ev->getDescription();
    stat(str.c_str(), &tmp_stat);
/* MITZ - what's the idea?!
    if (S_ISDIR(tmp_stat.st_mode))
    {
      int size = 0;
      int files = 0;
      int dirs = 0;
      listDirectory(str, size, files, dirs, ev);
		
      ev->setTotalSize(size);
      ostringstream ostr;
      ostr << files << " files in " << dirs << " directories";
      ev->setDescription(ostr.str());

    }
    else if (S_ISREG(tmp_stat.st_mode))
    {
      ev->addFile(str);
      ev->setTotalSize(tmp_stat.st_size);
      // Chopping of so just name is sent.
      int pos = str.find_last_of('/');
      if (pos == str.length()-1)
      {
	str = str.substr(0,pos);
	pos = str.find_last_of('/');
      }
      str = str.substr(pos+1, str.length()-pos);
      ev->setDescription(str);
    }
    else
    {
      return false;
    }
*/
    
    ev->setTotalFiles(ev->getFilesInQueue());
    ev->setCurrFile(0);

    return true;
  }

  void FileTransferClient::listDirectory(std::string str, int &size, int &files, int &dirs, FileTransferEvent *ev)
  {
    struct stat tmp_stat;
    if (str.length() > 1
	&& (str.substr(str.length()-1, str.length()) == "."
	    || str.substr(str.length()-2, str.length()) == ".."))
      return;

    // Add backslash if missing in path
    if (str.find_last_of('/') != (str.length()-1))
      str += "/";
    ev->addFile(str);
    dirs++;
    DIR *dir = opendir(str.c_str());
    if (dir == NULL)
      return;
    struct dirent *dent = readdir(dir);
	  
    std::string tmp_str;

    while (dent != NULL)
    {
      tmp_str = str;
      tmp_str += dent->d_name;
      stat(tmp_str.c_str(), &tmp_stat);
      if (S_ISREG(tmp_stat.st_mode))
      {
	ev->addFile(tmp_str);
	size += tmp_stat.st_size;
	files++;
      }
      else if (S_ISDIR(tmp_stat.st_mode))
      {
	listDirectory(tmp_str, size, files, dirs, ev);
      }

      dent = readdir(dir); //delete on old dent?
    }
    closedir(dir); 
  }

  FileTransferEvent* FileTransferClient::getEvent()
  {
    return m_ev;
  }
}
