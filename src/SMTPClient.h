/*
 * SMTPClient
 *
 * Copyright (C) 2002 Konstantin Klyagin <konst@konst.org.ua>
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

#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include "SocketClient.h"
#include "buffer.h"

namespace ICQ2000 {

  class SMTPClient : public SocketClient {
   private:
    enum State { NOT_CONNECTED,
		 WAITING_FOR_CONNECT,
		 WAITING_FOR_INVITATION,
		 WAITING_FOR_HELO_ACK,
		 WAITING_FOR_MAIL_ACK,
		 WAITING_FOR_RCPT_ACK,
		 WAITING_FOR_DATA_ACK,
		 WAITING_FOR_TEXT_ACK,
		 DISCONNECTING };

    State m_state;

    std::list<MessageEvent*> m_msgqueue;
    Buffer m_recv;
    std::string m_server_name;
    unsigned short m_server_port;
    time_t m_last_operation, m_timeout;

    void expired_cb(MessageEvent *ev);
    void flush_queue();
    void check_timeout();

    std::string getContactEmail(ContactRef cont) const;

    void Init();
    void Parse();
    void Send(Buffer &b);

    ContactRef m_self_contact;

    void SayHello();
    void SayFrom();
    void SayTo();
    void SayData();
    void SayQuit();

    void SendText();

    void Disconnect();

   public:
    SMTPClient(ContactRef self, const std::string& server_name, unsigned short server_port);

    ~SMTPClient();

    void Connect();
    void FinishNonBlockingConnect();
    void Recv();

    void clearoutMessagesPoll();

    void setServerHost(const std::string& host);
    std::string getServerHost() const;

    void setServerPort(unsigned short port);
    unsigned short getServerPort() const;

    void setTimeout(time_t t);
    time_t getTimeout() const;

    void SendEvent(MessageEvent* ev);
  };

  class SMTPException : public std::exception {
   private:
    std::string m_errortext;
    
   public:
    SMTPException();
    SMTPException(const std::string& text);
    ~SMTPException() throw() { }

    const char* what() const throw();
  };

};

#endif
