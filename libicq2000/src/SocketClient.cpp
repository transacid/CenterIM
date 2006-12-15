/*
 * SocketClient
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

#include "SocketClient.h"

#include "sstream_fix.h"

using std::string;

namespace ICQ2000 {

  void SocketClient::SignalAddSocket(int fd, SocketEvent::Mode m) {
    AddSocketHandleEvent ev( fd, m );
    socket.emit(&ev);
  }

  void SocketClient::SignalRemoveSocket(int fd) {
    RemoveSocketHandleEvent ev(fd);
    socket.emit(&ev);
  }

  void SocketClient::SignalLog(LogEvent::LogType type, const string& msg) {
    LogEvent ev(type,msg);
    logger.emit(&ev);
  }

  int SocketClient::getfd() const { return m_socket->getSocketHandle(); }

  TCPSocket* SocketClient::getSocket() const { return m_socket; }

  void SocketClient::setClientBindHost(const std::string &host) { m_bindhost = host; }
  
  // -- exceptions ------------------------------------------------------------

  SocketClientException::SocketClientException() { }
  SocketClientException::SocketClientException(const string& text) : m_errortext(text) { }

  const char* SocketClientException::what() const throw() { return m_errortext.c_str(); }

  DisconnectedException::DisconnectedException(const string& text) : SocketClientException(text) { }
  
}
