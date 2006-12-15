/*
 * General sockets class wrapper
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

#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <exception>

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

namespace ICQ2000
{

  unsigned int StringtoIP(const std::string& ip);

  std::string IPtoString(unsigned int ip);

  class Buffer;

  class TCPSocket
  {
   public:
    enum State {
      NOT_CONNECTED,
      NONBLOCKING_CONNECT,
      CONNECTED
    };

    static unsigned long gethostname(const char *hostname);
  
   private:
    static const unsigned int max_receive_size = 4096;
  
    int m_socketDescriptor;
    bool m_socketDescriptor_valid;
    struct sockaddr_in remoteAddr, localAddr;
    bool blocking;
    State m_state;

    void fcntlSetup();

  public:
    TCPSocket();
    ~TCPSocket();

    // used after a successful accept on TCPServer
    TCPSocket( int fd, struct sockaddr_in addr );

    void Connect();
    void FinishNonBlockingConnect();
    void Disconnect();
  
    int getSocketHandle();

    void Send(Buffer& b);

    bool Recv(Buffer& b);

    bool connected();

    void setRemoteHost(const char *host);
    void setRemotePort(unsigned short port);
    void setRemoteIP(unsigned int ip);
    void setBindHost(const char *host);

    void setBlocking(bool b);
    bool isBlocking() const;

    State getState() const;

    unsigned int getRemoteIP() const;
    unsigned short getRemotePort() const;

    unsigned int getLocalIP() const;
    unsigned short getLocalPort() const;

  };

  class TCPServer {
  private:
    int m_socketDescriptor;
    bool m_socketDescriptor_valid;
    struct sockaddr_in localAddr;

  public:
    TCPServer();
    ~TCPServer();

    int getSocketHandle();
  
    void StartServer();
    void StartServer(unsigned short lower, unsigned short upper);
    void Disconnect();

    void setBindHost(const char *host);

    bool isStarted() const;

    // blocking accept
    TCPSocket* Accept();
  
    unsigned short getPort() const;
    unsigned int getIP() const;

  };

  class SocketException : std::exception {
  private:
    std::string m_errortext;

  public:
    SocketException(const std::string& text);
    ~SocketException() throw() { }

    const char* what() const throw();
  };
 
}

#endif
