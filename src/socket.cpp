/*
 * TCPSocket class
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

#include "sstream_fix.h"
#include <algorithm>

#include "socket.h"

#include "buffer.h"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef MSG_NOSIGNAL
    #define SEND_FLAGS MSG_NOSIGNAL
#else
    #define SEND_FLAGS 0
#endif

using std::string;
using std::ostringstream;
using std::istringstream;
//using std::copy;

namespace ICQ2000
{
  

  // StringtoIP and IPtoString both work on host order ip address expressed as unsigned int's
  unsigned int StringtoIP(const string& s) {
    istringstream istr(s);
    unsigned char d1,d2,d3,d4;
    unsigned int b1,b2,b3,b4;
    istr >> b1 >> d1 >> b2 >> d2 >> b3 >> d3 >> b4;
    if (!istr) return 0;
    istr >> d4;
    if (istr) return 0;

    if (d1 == '.' && d2 == '.' && d3 == '.'
	&& b1 < 256 && b2 < 256 && b3 < 256 && b4 < 256) {
      return (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4 << 0);
    } else {
      return 0;
    }
  };

  string IPtoString(unsigned int ip) {
    ostringstream ostr;
    ostr << (ip >> 24) << "."
	 << ((ip >> 16) & 0xff) << "."
	 << ((ip >> 8) & 0xff) << "."
	 << (ip & 0xff);
    return ostr.str();
  }

  TCPSocket::TCPSocket()
    : m_socketDescriptor(-1), m_socketDescriptor_valid(false),
      blocking(false), m_state(NOT_CONNECTED)
  {
    memset(&remoteAddr, 0, sizeof(remoteAddr));
  }

  TCPSocket::TCPSocket( int fd, struct sockaddr_in addr )
    : m_socketDescriptor(fd), m_socketDescriptor_valid(true), remoteAddr(addr),
      blocking(false), m_state(CONNECTED)
  {
    socklen_t localLen = sizeof(struct sockaddr_in);
    getsockname( m_socketDescriptor, (struct sockaddr *)&localAddr, &localLen );

    fcntlSetup();
  }

  TCPSocket::~TCPSocket() {
    Disconnect();
  }

  extern "C" int cw_connect(int sockfd, const struct sockaddr *serv_addr, int addrlen, int ssl);

  void TCPSocket::Connect() {
    if (m_state != NOT_CONNECTED) throw SocketException("Already connected");

    m_socketDescriptor = socket(AF_INET,SOCK_STREAM,0);
    if (m_socketDescriptor == -1) throw SocketException("Couldn't create socket");
    m_socketDescriptor_valid = true;
    remoteAddr.sin_family = AF_INET;

    fcntlSetup();

    if (cw_connect(m_socketDescriptor,(struct sockaddr *)&remoteAddr,sizeof(struct sockaddr), 0) == -1) {
      if (errno == EINPROGRESS) {
	m_state = NONBLOCKING_CONNECT;
	return; // non-blocking connect
      }

      // m_state will still be NOT_CONNECTED
      close(m_socketDescriptor);
      m_socketDescriptor_valid = false;
      throw SocketException("Couldn't connect socket");
    }

    socklen_t localLen = sizeof(struct sockaddr_in);
    getsockname( m_socketDescriptor, (struct sockaddr *)&localAddr, &localLen );

    m_state = CONNECTED;
  }

  void TCPSocket::FinishNonBlockingConnect() {
    // this should be called for non blocking connects
    // after the socket is writeable
    int so_error;
    socklen_t optlen = sizeof(so_error);
    if (getsockopt(m_socketDescriptor, SOL_SOCKET, SO_ERROR, (char *) &so_error, &optlen) == -1 || so_error != 0) {
      m_state = NOT_CONNECTED;
      close(m_socketDescriptor);
      m_socketDescriptor_valid = false;
      throw SocketException("Couldn't connect socket");
    }

    // success
    socklen_t localLen = sizeof(struct sockaddr_in);
    getsockname( m_socketDescriptor, (struct sockaddr *)&localAddr, &localLen );

    m_state = CONNECTED;
  }

  void TCPSocket::Disconnect() {

    if (m_socketDescriptor_valid) {
      close(m_socketDescriptor);
      m_socketDescriptor_valid = false;
    }

    m_state = NOT_CONNECTED;
  }

  int TCPSocket::getSocketHandle() { return m_socketDescriptor; }

  TCPSocket::State TCPSocket::getState() const { return m_state; }

  bool TCPSocket::connected() {
    return (m_state == CONNECTED);
  }

  void TCPSocket::setBlocking(bool b) {
    blocking = b;
    fcntlSetup();
  }

  bool TCPSocket::isBlocking() const {
    return blocking;
  }

  void TCPSocket::fcntlSetup() {
    if (m_socketDescriptor_valid) {
      int f = fcntl(m_socketDescriptor, F_GETFL);
      if (blocking) fcntl(m_socketDescriptor, F_SETFL, f & ~O_NONBLOCK);
      else fcntl(m_socketDescriptor, F_SETFL, f | O_NONBLOCK);
    }
  }

  void TCPSocket::Send(Buffer& b) {
    if (!connected()) throw SocketException("Not connected");

    int ret;
    unsigned int sent = 0;

    unsigned char data[b.size()];
    copy( b.begin(), b.end(), data );

    while (sent < b.size())
    {
      ret = send(m_socketDescriptor, (char *) (data + sent), b.size() - sent, SEND_FLAGS);
      if (ret == -1) {
	m_state = NOT_CONNECTED;
	close(m_socketDescriptor);
	m_socketDescriptor_valid = false;
	throw SocketException("Sending on socket");
      }
    
      sent += ret;
    }
  }

  bool TCPSocket::Recv(Buffer& b) {
    if (!connected()) throw SocketException("Not connected");

    unsigned char buffer[max_receive_size];

    int ret = recv(m_socketDescriptor, (char *) buffer, max_receive_size, 0);

    if (ret <= 0) {
      if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) return false;

      m_state = NOT_CONNECTED;
      close(m_socketDescriptor);
      m_socketDescriptor_valid = false;

      if (ret == 0) throw SocketException( "Other end closed connection" );
      else throw SocketException( strerror(errno) );
    }
  
    b.Pack(buffer,ret);
    return true;
  }

  void TCPSocket::setBindHost(const char *host) {
    memset(&localAddr, 0, sizeof(localAddr));
    if(strlen(host)) {
      localAddr.sin_addr.s_addr = gethostname(host);
      localAddr.sin_port = 0;
    }
  }

  void TCPSocket::setRemoteHost(const char *host) {
    remoteAddr.sin_addr.s_addr = gethostname(host);
  }

  void TCPSocket::setRemotePort(unsigned short port) {
    remoteAddr.sin_port = htons(port);
  }

  void TCPSocket::setRemoteIP(unsigned int ip) {
    remoteAddr.sin_addr.s_addr = htonl(ip);
  }

  unsigned short TCPSocket::getRemotePort() const {
    return ntohs( remoteAddr.sin_port );
  }

  unsigned int TCPSocket::getRemoteIP() const {
    return ntohl( remoteAddr.sin_addr.s_addr );
  }

  unsigned short TCPSocket::getLocalPort() const {
    return ntohs( localAddr.sin_port );
  }

  unsigned int TCPSocket::getLocalIP() const {
    return ntohl( localAddr.sin_addr.s_addr );
  }


  // returns ip address of host in network byte order

  unsigned long TCPSocket::gethostname(const char *hostname) {

    unsigned int ip = htonl( StringtoIP(hostname) );
    if (ip != 0) return ip;


    // try and resolve hostname
    struct hostent *hostEnt;
    if ((hostEnt = gethostbyname(hostname)) == NULL || hostEnt->h_addrtype != AF_INET) {
      throw SocketException("DNS lookup failed");
    } else {
      return *((unsigned long *)(hostEnt->h_addr));
    }
  }

  /**
   * TCPServer class
   */
  TCPServer::TCPServer()
    : m_socketDescriptor_valid(false)
  { }

  TCPServer::~TCPServer() {
    Disconnect();
  }

  void TCPServer::StartServer() {
    StartServer(0, 0);
  }

  void TCPServer::StartServer(unsigned short lower, unsigned short upper) {
    if (m_socketDescriptor_valid) throw SocketException("Already listening");
  
    m_socketDescriptor = socket( AF_INET, SOCK_STREAM, 0 );
    if (m_socketDescriptor < 0) throw SocketException("Couldn't create socket");
    m_socketDescriptor_valid = true;
  
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;

    bool bound = false;
    if (lower && upper) {
      unsigned short port;
      // attempt to bind to a port within the range
      for (port=lower;port<=upper;port++) {
	localAddr.sin_port = htons(port);
	if ( bind( m_socketDescriptor,(struct sockaddr *)&localAddr,sizeof(struct sockaddr) ) >= 0 ) {
	  bound = true;
	  break;
	}
      
      }
    }
  
    if (!bound) {
      // bind to a random port
      localAddr.sin_port = 0;
      if ( bind( m_socketDescriptor,(struct sockaddr *)&localAddr,sizeof(struct sockaddr) ) < 0 )
	throw SocketException("Couldn't bind socket");
    }

    listen( m_socketDescriptor, 5 );
    // queue size of 5 should be sufficient
  
    socklen_t localLen = sizeof(struct sockaddr_in);
    getsockname( m_socketDescriptor, (struct sockaddr *)&localAddr, &localLen );
  }

  unsigned short TCPServer::getPort() const {
    return ntohs( localAddr.sin_port );
  }

  unsigned int TCPServer::getIP() const {
    return ntohl( localAddr.sin_addr.s_addr );
  }

  TCPSocket* TCPServer::Accept() {
    int newsockfd;
    socklen_t remoteLen;
    struct sockaddr_in remoteAddr;

    if (!m_socketDescriptor_valid) throw SocketException("Not connected");

    remoteLen = sizeof(remoteAddr);
    newsockfd = accept( m_socketDescriptor,
			(struct sockaddr *) &remoteAddr, 
			&remoteLen );
    if (newsockfd < 0) {
      close(m_socketDescriptor);
      m_socketDescriptor_valid = false;
      throw SocketException("Error on accept");
    }

    return new TCPSocket( newsockfd, remoteAddr );
  }

  int TCPServer::getSocketHandle() { return m_socketDescriptor; }

  void TCPServer::Disconnect() {
    if (m_socketDescriptor_valid) {
      close(m_socketDescriptor);
      m_socketDescriptor = 0;
      m_socketDescriptor_valid = false;
    }
  }

  bool TCPServer::isStarted() const { return m_socketDescriptor_valid; }

  void TCPServer::setBindHost(const char *host) {
    memset(&localAddr, 0, sizeof(localAddr));
    if(strlen(host)) {
      localAddr.sin_addr.s_addr = TCPSocket::gethostname(host);
      localAddr.sin_port = 0;
    }
  }

  /**
   * SocketException class
   */
  SocketException::SocketException(const string& text) : m_errortext(text) { }

  const char* SocketException::what() const throw() {
    return m_errortext.c_str();
  }

}
