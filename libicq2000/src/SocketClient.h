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

#ifndef SOCKETCLIENT_H
#define SOCKETCLIENT_H

#include <string>

#include "libicq2000/sigslot.h"

#include "socket.h"
#include "events.h"

namespace ICQ2000 {

  class SocketClient : public sigslot::has_slots<> {
   protected:
    void SignalAddSocket(int fd, SocketEvent::Mode m);
    void SignalRemoveSocket(int fd);

    TCPSocket *m_socket;
    std::string m_bindhost;

   public:
    virtual void Connect() = 0;
    virtual void FinishNonBlockingConnect() = 0;
    virtual void Recv() = 0;

    // ------------------ Signal dispatchers -----------------
    void SignalLog(LogEvent::LogType type, const std::string& msg);

    // ------------------  Signals ---------------------------
    sigslot::signal1<LogEvent*> logger;
    sigslot::signal1<MessageEvent*> messageack;
    sigslot::signal1<SocketEvent*> socket;
    sigslot::signal1<SocketClient*> connected;

    int getfd() const;
    TCPSocket* getSocket() const;
    void setClientBindHost(const std::string &host);
    virtual void clearoutMessagesPoll() = 0;

    virtual void SendEvent(MessageEvent* ev) = 0;
  };

  class SocketClientException : public std::exception {
   private:
    std::string m_errortext;
    
   public:
    SocketClientException();
    SocketClientException(const std::string& text);
    ~SocketClientException() throw() { }

    const char* what() const throw();
  };
  
  class DisconnectedException : public SocketClientException {
   public:
    DisconnectedException(const std::string& text);
  };

}

#endif
