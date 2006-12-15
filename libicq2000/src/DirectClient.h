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

#ifndef DIRECTCLIENT_H
#define DIRECTCLIENT_H

#include <list>

#include "libicq2000/sigslot.h"

#include <stdlib.h>

#include "socket.h"
#include "buffer.h"
#include "events.h"
#include "exceptions.h"
#include "Contact.h"
#include "ContactTree.h"
#include "SeqNumCache.h"
#include "SocketClient.h"
#include "MessageHandler.h"

namespace ICQ2000 {

  class UINICQSubType;
  
  class DirectClient : public SocketClient {
   private:
    enum State { NOT_CONNECTED,
		 WAITING_FOR_INIT,
		 WAITING_FOR_INIT_ACK,
		 WAITING_FOR_INIT2,
		 CONNECTED };

    State m_state;

    Buffer m_recv;

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

    void SendInitAck();
    void SendInitPacket();
    void SendInit2();
    void SendPacketEvent(MessageEvent *ev);
    void SendPacketAck(ICQSubType *i);
    void Send(Buffer &b);
    
    bool Decrypt(Buffer& in, Buffer& out);
    void Encrypt(Buffer& in, Buffer& out);
    static unsigned char client_check_data[];
    SeqNumCache m_msgcache;
    std::list<MessageEvent*> m_msgqueue;
    unsigned short m_seqnum;

    unsigned short NextSeqNum();
    void ConfirmUIN();

    void expired_cb(MessageEvent *ev);
    void flush_queue();

    void Init();

   public:
    DirectClient(ContactRef self, TCPSocket *sock, MessageHandler *mh, ContactTree *cl, unsigned int ext_ip,
		 unsigned short server_port);
    DirectClient(ContactRef self, ContactRef c, MessageHandler *mh, unsigned int ext_ip,
		 unsigned short server_port);
    ~DirectClient();

    void Connect();
    void FinishNonBlockingConnect();
    void Recv();

    unsigned int getUIN() const;
    unsigned int getIP() const;
    unsigned short getPort() const;
    int getfd() const;
    TCPSocket* getSocket() const;
    void clearoutMessagesPoll();

    void setContact(ContactRef c);
    ContactRef getContact() const;
    void SendEvent(MessageEvent* ev);
    void SendFTACK(FileTransferEvent *ev);
    void SendFTCancel(FileTransferEvent *ev);
  };

}

#endif
