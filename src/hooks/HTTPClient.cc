/*
*
* centericq HTTP protocol handling class
* $Id: HTTPClient.cc,v 1.4 2003/07/23 23:21:04 konst Exp $
*
* Copyright (C) 2003 by Konstantin Klyagin <konst@konst.org.ua>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at
* your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
* USA
*
*/

#include "HTTPClient.h"

#ifdef BUILD_RSS

HTTPClient::HTTPClient() : m_recv(&m_transl), m_state(NOT_CONNECTED),
m_timeout(50), m_proxy_port(8080) {
    m_socket = new TCPSocket();
    Init();
}

HTTPClient::~HTTPClient() {
    if(m_socket->getSocketHandle() > -1) SignalRemoveSocket(m_socket->getSocketHandle());
    delete m_socket;
}

void HTTPClient::Init() {
}

void HTTPClient::Connect() {
    if(m_queue.empty())
	return;

    int pos;
    HTTPRequestEvent *ev = *m_queue.begin();
    m_hostname = !m_redirect.empty() ? m_redirect : ev->getURL();

    m_port = 80;
    m_resource = "/";
    m_content = m_redirect = "";
    m_recv.clear();

    if(up(m_hostname.substr(0, 7)) == "HTTP://") {
	m_hostname.erase(0, 7);
    }

    if((pos = m_hostname.find("/")) != -1) {
	m_resource = m_hostname.substr(pos);
	m_hostname.erase(pos);
    }

    if((pos = m_hostname.find(":")) != -1) {
	m_port = strtoul(m_hostname.substr(pos+1).c_str(), 0, 0);
	m_hostname.erase(pos);
    }

    try {
	if(m_proxy_hostname.empty()) {
	    m_socket->setRemoteHost(m_hostname.c_str());
	    m_socket->setRemotePort(m_port);
	} else {
	    m_socket->setRemoteHost(m_proxy_hostname.c_str());
	    m_socket->setRemotePort(m_proxy_port);
	}

	m_socket->setBindHost(m_bindhost.c_str());
	m_socket->setBlocking(false);
	m_socket->Connect();
	SignalAddSocket(m_socket->getSocketHandle(), SocketEvent::WRITE);

	time(&m_last_operation);
	m_state = WAITING_FOR_CONNECT;

    } catch(DisconnectedException e) {
	SignalLog(LogEvent::WARN, e.what());
	Disconnect();

    } catch(SocketException e) {
	SignalLog(LogEvent::WARN, e.what());
	Disconnect();

    } catch(...) {
	SignalLog(LogEvent::WARN, "Uncaught HTTP connect exception");
	Disconnect();

    }
}

void HTTPClient::Parse() {
    int npos;
    string response;

    while(1) {
	response = "";
	m_recv.UnpackCRLFString(response);
	if(response.empty()) break;

	while(!response.empty()) {
	    if(response.substr(response.size()-1).find_first_of("\r\n") != -1)
		response.erase(response.size()-1); else break;
	}

	time(&m_last_operation);

	string log = "Received HTTP response from " +
	    IPtoString(m_socket->getRemoteIP()) + ":" +
	    i2str(m_socket->getRemotePort()) + "\n" + response;

	SignalLog(LogEvent::DIRECTPACKET, log);

	if(m_state == WAITING_FOR_HEADER) {
	    m_code = 0;

	    if((npos = response.find(" ")) != -1) {
		m_code = strtoul(response.substr(npos+1).c_str(), 0, 0);
	    }

	    switch(m_code) {
		case 200:
		case 301:
		    break;
		    break;
		default:
		    HTTPRequestEvent *ev = *m_queue.begin();
		    dynamic_cast<HTTPRequestEvent*>(ev)->setHTTPResp(response.substr(npos+1));
		    throw HTTPException("Didn't receive HTTP OK");
	    }

	    m_state = RECEIVING_HEADER;

	} else if(m_state == RECEIVING_HEADER) {
	    if(m_code == 301)
	    if((npos = response.find(" ")) != -1)
	    if(up(response.substr(0, npos)) == "LOCATION:") {
		m_redirect = response.substr(npos+1);
		m_socket->Disconnect();
		m_state = NOT_CONNECTED;
		Connect();
	    }

	    if(response.empty()) m_state = RECEIVING_CONTENT;

	} else if(m_state == RECEIVING_CONTENT) {
	    m_content += response + "\n";

	}
    }
}

void HTTPClient::Send(Buffer &b) {
    string log;

    try {
	log = "Sending HTTP command to " + IPtoString(m_socket->getRemoteIP()) + ":" +
	    i2str(m_socket->getRemotePort());

	SignalLog(LogEvent::DIRECTPACKET, log);
	m_socket->Send(b);

    } catch(SocketException e) {
	log = (string) "Failed to send: " + e.what();
	m_state = NOT_CONNECTED;
	throw DisconnectedException(log);
    }
}

void HTTPClient::SendRequest() {
    Buffer b(&m_transl);

    if(m_proxy_hostname.empty()) {
	b.Pack((string) "GET " + m_resource + " HTTP/1.0\r\n");
	b.Pack((string) "Host: " + m_hostname + ":" + i2str(m_port) + "\r\n");
    } else {
	b.Pack((string) "GET " + m_hostname + m_resource + " HTTP/1.0\r\n");
    }

    b.Pack((string) "User-Agent: " + PACKAGE + "/" + VERSION + "\r\n\r\n");

    Send(b);
    m_state = WAITING_FOR_HEADER;
}

void HTTPClient::Disconnect() {
    SignalLog(LogEvent::WARN, "HTTP disconnected");
    m_socket->Disconnect();
    m_state = NOT_CONNECTED;
    if(m_socket->getSocketHandle() > -1) SignalRemoveSocket(m_socket->getSocketHandle());
}

void HTTPClient::FinishNonBlockingConnect() {
    SendRequest();
}

void HTTPClient::Recv() {
    try {
	while(m_socket->connected()) {
	    if(!m_socket->Recv(m_recv)) break;
	    Parse();
	}

    } catch(SocketException e) {
	Disconnect();
	throw DisconnectedException((string) "Failed on recv: " + e.what());

    } catch(ParseException e) {
	Disconnect();
	throw DisconnectedException((string) "Failed parsing: " + e.what());

    } catch(HTTPException e) {
	MessageEvent *ev = *m_queue.begin();
	ev->setDelivered(false);
	ev->setFinished(true);
	ev->setDeliveryFailureReason(MessageEvent::Failed);
	messageack.emit(ev);

	delete ev;
	m_queue.pop_front();

	Disconnect();
	SignalLog(LogEvent::ERROR, e.what());
	throw DisconnectedException((string) "HTTP failed: " + e.what());

    }
}

void HTTPClient::check_timeout() {
    time_t now = time(0);

    if(now-m_last_operation > m_timeout) {
	MessageEvent *ev = *m_queue.begin();
	ev->setDelivered(false);
	ev->setFinished(true);
	dynamic_cast<HTTPRequestEvent*>(ev)->setHTTPResp(_("Timed out"));
	ev->setDeliveryFailureReason(MessageEvent::Failed);
	messageack.emit(ev);

	delete ev;
	m_queue.pop_front();
	Disconnect();
    }
}

void HTTPClient::setTimeout(time_t t) { m_timeout = t; }
time_t HTTPClient::getTimeout() const { return m_timeout; }

void HTTPClient::clearoutMessagesPoll() {
    if(!m_queue.empty()) {
	if(m_state == NOT_CONNECTED) {
	    Connect();
	} else {
	    check_timeout();
	}
    }
}

void HTTPClient::SendEvent(MessageEvent* ev) {
    HTTPRequestEvent *rev = dynamic_cast<HTTPRequestEvent*>(ev);
    if(rev) {
	bool found = false;
	list<HTTPRequestEvent*>::const_iterator ir = m_queue.begin();

	while((ir != m_queue.end()) && !found) {
	    found = ((*ir)->getURL() == rev->getURL());
	    ++ir;
	}

	if(!found) {
	    m_queue.push_back(rev);

	    if(m_state == NOT_CONNECTED) {
		Connect();
	    }
	}
    }
}

// ----------------------------------------------------------------------------

void HTTPClient::socket_cb(int fd, SocketEvent::Mode m) {
    TCPSocket *sock = getSocket();

    if(sock->getState() == TCPSocket::NONBLOCKING_CONNECT && (m & SocketEvent::WRITE)) {
	try {
	    sock->FinishNonBlockingConnect();

	} catch(SocketException e) {
	    SignalLog(LogEvent::ERROR, (string) "Failed on non-blocking connect for direct connection: " + e.what());
	    SignalRemoveSocket(fd);
	    return;

	}

	SignalRemoveSocket(fd);
	SignalAddSocket(fd, SocketEvent::READ);
	
	try {
	    FinishNonBlockingConnect();

	} catch(DisconnectedException e) {
	    SignalLog(LogEvent::WARN, e.what());
	    SignalRemoveSocket(fd);
	}

    } else if(sock->getState() == TCPSocket::CONNECTED && (m & SocketEvent::READ)) {
	try {
	    Recv();

	} catch(DisconnectedException e) {
	    SignalLog(LogEvent::WARN, e.what());
	    SignalRemoveSocket(fd);

	    if(!m_recv.empty()) {
		string remains;
		m_recv.Unpack(remains, m_recv.size());
		m_content += remains;
	    }

	    HTTPRequestEvent *ev = *m_queue.begin();
	    ev->setDelivered(true);
	    ev->setFinished(true);
	    ev->setContent(m_content);
	    messageack.emit(ev);

	    delete ev;
	    m_queue.pop_front();

	}

    } else {
	SignalLog(LogEvent::ERROR, "Direct Connection socket in inconsistent state!");
	SignalRemoveSocket(fd);

    }
}

#endif
