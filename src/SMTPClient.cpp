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

#include "SMTPClient.h"
#include "exceptions.h"

#include "sstream_fix.h"

using std::string;
using std::ostringstream;
using std::endl;

namespace ICQ2000 {

  SMTPClient::SMTPClient(ContactRef self, const string& server_name,
			 unsigned short server_port)
    : m_state(NOT_CONNECTED), m_recv(), m_server_name(server_name),
      m_server_port(server_port), m_timeout(30),
      m_self_contact(self)
  {
    m_socket = new TCPSocket();
    Init();
  }

  SMTPClient::~SMTPClient() {
    if ( m_socket->getSocketHandle() > -1) SignalRemoveSocket( m_socket->getSocketHandle() );
    delete m_socket;
  }

  void SMTPClient::Init() {
  }

  void SMTPClient::Connect() {
    try {
      m_socket->setRemoteHost( m_server_name.c_str() );
      m_socket->setRemotePort( m_server_port );
      m_socket->setBindHost( m_bindhost.c_str() );
      m_socket->setBlocking(false);
      m_socket->Connect();
      SignalAddSocket( m_socket->getSocketHandle(), SocketEvent::WRITE );

      time(&m_last_operation);
      m_state = WAITING_FOR_CONNECT;

    } catch(DisconnectedException e) {

      SignalLog(LogEvent::WARN, e.what());
      Disconnect();
    } catch(SocketException e) {

      SignalLog(LogEvent::WARN, e.what());
      Disconnect();
    } catch(...) {

      SignalLog(LogEvent::WARN, "Uncaught SMTP connect exception");
      Disconnect();
    }
  }

  void SMTPClient::FinishNonBlockingConnect() {
    m_state = WAITING_FOR_INVITATION;
  }

  void SMTPClient::Recv() {
    try {
      while ( m_socket->connected() ) {
	if ( !m_socket->Recv(m_recv) ) break;
	Parse();
      }
    } catch(SocketException e) {
      ostringstream ostr;
      ostr << "Failed on recv: " << e.what();
      Disconnect();
      throw DisconnectedException( ostr.str() );
    } catch(ParseException e) {
      ostringstream ostr;
      ostr << "Failed parsing: " << e.what();
      m_state = NOT_CONNECTED;
      throw DisconnectedException( ostr.str() );
    } catch(SMTPException e) {
      SignalLog(LogEvent::ERROR, e.what());

      MessageEvent *ev = *m_msgqueue.begin();
      ev->setDelivered(false);
      ev->setFinished(true);
      ev->setDeliveryFailureReason(MessageEvent::Failed_SMTP);
      messageack.emit(ev);

      delete ev;
      m_msgqueue.pop_front();

      if(m_msgqueue.size()) {
	// process the next queued message
	SayFrom();
      } else {
	ostringstream ostr;
	ostr << "SMTP failed: " << e.what();
	m_state = NOT_CONNECTED;
	throw DisconnectedException( ostr.str() );
      }
    }
  }

  void SMTPClient::Parse() {
    if (m_recv.empty()) return;

    string response;
    m_recv.UnpackCRLFString(response);

    time(&m_last_operation);

    ostringstream ostr;
    ostr << "Received SMTP response from " << IPtoString( m_socket->getRemoteIP() ) << ":" << m_socket->getRemotePort() << endl << response;
    SignalLog(LogEvent::DIRECTPACKET, ostr.str());

    int code, npos;

    code = 0;
    if((npos = response.find(" ")) != -1) {
	code = strtoul(response.substr(0, npos).c_str(), 0, 0);
    }

    if(m_state == WAITING_FOR_INVITATION) {
      if(code == 220) {
	SayHello();
      } else throw ParseException("Didn't receive 220 response");
    } else if(m_state == WAITING_FOR_HELO_ACK) {
      if(code == 250) {
	SayFrom();
      } else throw ParseException("HELO command wasn't accepted");
    } else if(m_state == WAITING_FOR_MAIL_ACK) {
      if(code == 250) {
	SayTo();
      } else throw SMTPException("MAIL command wasn't accepted");
    } else if(m_state == WAITING_FOR_RCPT_ACK) {
      if(code == 250) {
	SayData();
      } else throw SMTPException("RCPT command wasn't accepted");
    } else if(m_state == WAITING_FOR_DATA_ACK) {
      if(code == 354) {
	SendText();
      } else throw SMTPException("DATA command wasn't accepted");
    } else if(m_state == WAITING_FOR_TEXT_ACK) {
      if(code == 250) {
	MessageEvent *ev = *m_msgqueue.begin();
	ev->setDelivered(true);
	ev->setFinished(true);
	messageack.emit(ev);

	m_msgqueue.pop_front();

	if(m_msgqueue.empty()) {
	  SayQuit();
	} else {
	  SayFrom();
	}
      } else throw SMTPException("The message text wasn't accepted");
    }
  }

  void SMTPClient::clearoutMessagesPoll() {
    if(!m_msgqueue.empty()) {
      if(m_state == NOT_CONNECTED) {
	Connect();
      } else {
	check_timeout();
      }
    }
  }

  void SMTPClient::SendEvent(MessageEvent* ev) {
      m_msgqueue.push_back(ev);

      if(m_state == NOT_CONNECTED) {
	Connect();
      }
  }

  void SMTPClient::Send(Buffer &b) {
    try {
      ostringstream ostr;
      ostr << "Sending SMTP command to "  << IPtoString( m_socket->getRemoteIP() ) << ":" << m_socket->getRemotePort() << endl << b;
      SignalLog(LogEvent::DIRECTPACKET, ostr.str());
      m_socket->Send(b);
    } catch(SocketException e) {
      ostringstream ostr;
      ostr << "Failed to send: " << e.what();
      m_state = NOT_CONNECTED;
      throw DisconnectedException( ostr.str() );
    }
  }

  void SMTPClient::SayHello() {
    Buffer b;
    b.Pack("HELO localhost\n");
    Send(b);
    m_state = WAITING_FOR_HELO_ACK;
  }

  void SMTPClient::SayFrom() {
    Buffer b;

    MessageEvent *ev = *m_msgqueue.begin();
    b.Pack("MAIL FROM:");

    if(ev->getType() == MessageEvent::SMS) {
      const SMSMessageEvent *aev = static_cast<const SMSMessageEvent*>(ev);
      b.Pack(aev->getSMTPFrom());

    } else if(ev->getType() == MessageEvent::Email) {
      b.Pack(getContactEmail(m_self_contact));

    }

    b.Pack("\n");
    Send(b);
    m_state = WAITING_FOR_MAIL_ACK;
  }

  void SMTPClient::SayTo() {
    Buffer b;

    MessageEvent *ev = *m_msgqueue.begin();
    b.Pack("RCPT TO:");

    if(ev->getType() == MessageEvent::SMS) {
      const SMSMessageEvent *aev = static_cast<const SMSMessageEvent*>(ev);
      b.Pack(aev->getSMTPTo());

    } else if(ev->getType() == MessageEvent::Email) {
      b.Pack(getContactEmail( ev->getContact() ));

    }

    b.Pack("\n");
    Send(b);
    m_state = WAITING_FOR_RCPT_ACK;
  }

  void SMTPClient::SayData() {
    Buffer b;

    b.Pack("DATA\n");
    Send(b);

    m_state = WAITING_FOR_DATA_ACK;
  }

  void SMTPClient::SendText() {
    Buffer b;

    MessageEvent *ev = *m_msgqueue.begin();

    if(ev->getType() == MessageEvent::SMS) {
      const SMSMessageEvent *aev = static_cast<const SMSMessageEvent*>(ev);

      if(!aev->getSMTPSubject().empty()) {
	b.Pack("Subject: ");
	b.Pack(aev->getSMTPSubject());
	b.Pack("\n\n");
      }

      b.Pack(aev->getMessage());
    } else {
      const EmailMessageEvent *aev = static_cast<const EmailMessageEvent*>(ev);
      b.Pack(aev->getMessage());
    }

    b.Pack("\n.\n");
    Send(b);

    time(&m_last_operation);
    m_state = WAITING_FOR_TEXT_ACK;
  }

  void SMTPClient::SayQuit() {
    Buffer b;
    b.Pack("QUIT\n");
    Send(b);

    m_state = DISCONNECTING;
  }

  void SMTPClient::check_timeout() {
    time_t now = time(0);

    if(now-m_last_operation > m_timeout) {
      Disconnect();
    }
  }

  void SMTPClient::Disconnect() {
    m_socket->Disconnect();
    m_state = NOT_CONNECTED;
    if ( m_socket->getSocketHandle() > -1) SignalRemoveSocket( m_socket->getSocketHandle() );
  }

  string SMTPClient::getContactEmail(ContactRef cont) const {
    if(cont->getEmail().empty()) {
      ostringstream ostr;
      ostr << std::dec << cont->getUIN() << "@pager.icq.com";
      return ostr.str();
    } else {
      return cont->getEmail();
    }
  }

  void SMTPClient::setServerHost(const string& host) { m_server_name = host; }
  string SMTPClient::getServerHost() const { return m_server_name; }

  void SMTPClient::setServerPort(unsigned short port) { m_server_port = port; }
  unsigned short SMTPClient::getServerPort() const { return m_server_port; }

  void SMTPClient::setTimeout(time_t t) { m_timeout = t; }
  time_t SMTPClient::getTimeout() const { return m_timeout; }

  SMTPException::SMTPException() { }
  SMTPException::SMTPException(const string& text) : m_errortext(text) { }

  const char* SMTPException::what() const throw() { return m_errortext.c_str(); }

}
