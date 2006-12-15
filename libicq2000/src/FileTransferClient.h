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

#ifndef FILETRANSFERCLIENT_H
#define FILETRANSFERCLIENT_H

#include <list>
#include <string>
#include <fstream>

#include "libicq2000/sigslot.h"

#include <stdlib.h>
#include <time.h>

#include "socket.h"
#include "buffer.h"
#include "events.h"
#include "exceptions.h"
#include "Contact.h"
#include "ContactTree.h"
#include "SocketClient.h"
#include "MessageHandler.h"

namespace ICQ2000 {

#define MAX_FileChunk 4096

  class UINICQSubType;
  
  class FileTransferClient : public SocketClient {
   private:
    enum State { NOT_CONNECTED,
		 WAITING_FOR_INIT,
		 WAITING_FOR_INIT_ACK,
		 WAITING_FOR_INIT2,
		 CONNECTED };

    TCPServer m_listenserver;
    FileTransferEvent* m_ev;

    std::string m_path, *m_base_dir;
    
    State m_state;
 
    bool m_more, m_continue;
    bool m_senddir;
    
    Buffer m_recv;

    unsigned char m_const_buf[MAX_FileChunk];
    std::ofstream m_fout;
    std::ifstream m_fin;

    ContactRef m_self_contact;
    ContactRef m_contact;
    ContactTree *m_contact_list;
    MessageHandler *m_message_handler;

    bool m_incoming;

    unsigned short m_remote_tcp_version;
    unsigned int m_remote_uin;
    unsigned char m_tcp_flags;
    unsigned short m_eff_tcp_version;

    unsigned int m_local_ext_ip, m_session_id;
    unsigned short m_local_server_port;

    void Parse();
    void ParseInitPacket(Buffer &b);
    void ParseInitAck(Buffer &b);
    void ParseInit2(Buffer &b);
    void ParsePacket(Buffer& b);
    void ParsePacketInt(Buffer& b);

    void ParsePacket0x00(Buffer& b);
    void ParsePacket0x01(Buffer& b);
    void ParsePacket0x02(Buffer& b);
    void ParsePacket0x03(Buffer& b);
    void ParsePacket0x05(Buffer& b);
    void ParsePacket0x06(Buffer& b);
    
    void SendPacket0x00();
    void SendPacket0x01();
    void SendPacket0x02();
    void SendPacket0x03(unsigned int npos, unsigned int nfiles);
    void SendPacket0x05();
    void SendPacket0x06();

    
    void SendInitAck();
    void SendInitPacket();
    void SendInit2();
    void SendPacketAck(ICQSubType *i);
    void Send(Buffer &b);

        
    unsigned int m_timeout;
    time_t m_timestamp;
    bool m_msgqueue;
   
    void ConfirmUIN();

    void expired();
    void flush_queue();

    void Init();

    static void listDirectory(std::string str, int &size, int &files, int &dirs, FileTransferEvent *ev);

    void SendFile();
    
   public:
    FileTransferClient(ContactRef self, MessageHandler *mh, ContactTree *cl, unsigned int ext_ip, FileTransferEvent* ev);
    FileTransferClient(ContactRef self, ContactRef c, MessageHandler *mh, unsigned int ext_ip, FileTransferEvent* ev);
    ~FileTransferClient();

    void Connect();
    void FinishNonBlockingConnect();
    void Recv();
    

    static bool SetupFileTransfer(FileTransferEvent *ev);

    FileTransferEvent* getEvent();
    
    unsigned int getUIN() const;
    unsigned int getIP() const;
    unsigned short getPort() const;
    unsigned short getlistenPort() const;
    int getfd() const;
    int getlistenfd();
    TCPSocket* getSocket() const;
    void setSocket();
    void clearoutMessagesPoll();

    void setContact(ContactRef c);
    ContactRef getContact() const;
    void SendEvent(MessageEvent* ev);
  };

}

#endif
