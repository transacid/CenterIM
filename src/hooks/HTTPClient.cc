/*
*
* centericq HTTP protocol handling class
* $Id: HTTPClient.cc,v 1.19 2005/01/18 23:20:17 konst Exp $
*
* Copyright (C) 2003-2004 by Konstantin Klyagin <konst@konst.org.ua>
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

#include <md5.h>
#include <connwrap.h>

#ifdef BUILD_RSS

HTTPClient::HTTPClient() : m_recv(), m_state(NOT_CONNECTED),
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
    HTTPRequestEvent *ev = m_queue.front();
    m_hostname = !m_redirect.empty() ? m_redirect : ev->getURL();

    m_port = 80;
    m_resource = "/";
    m_content = m_redirect = "";
    m_recv.clear();

    if(up(m_hostname.substr(0, 7)) == "HTTP://") {
	m_hostname.erase(0, 7);
    }

    if((pos = m_hostname.find("@")) != -1) {
	ev->setAuth(HTTPRequestEvent::Basic, m_hostname.substr(0, pos), "");
	m_hostname.erase(0, pos+1);

	if((pos = ev->m_user.find(":")) != -1) {
	    ev->m_pass = ev->m_user.substr(pos+1);
	    ev->m_user.erase(pos);
	}
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
	if(ev->connectTries++ > 10) {
	    ev->setHTTPResp("too many connect attempts");
	    throw DisconnectedException("Too many connect attempts");
	}

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

    if(m_state == NOT_CONNECTED) {
	MessageEvent *ev = m_queue.front();

	ev->setDelivered(false);
	ev->setFinished(true);
	ev->setDeliveryFailureReason(MessageEvent::Failed);
	messageack.emit(ev);

	delete ev;
	m_queue.pop_front();
    }
}

void HTTPClient::Parse() {
    int npos;
    string response, head, val;
    HTTPRequestEvent *ev = m_queue.front();

    npos = m_recv.size();

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
		case 302:
		case 401:
		    break;
		default:
		    ev->setHTTPResp(response.substr(npos+1));
		    throw HTTPException("Didn't receive HTTP OK");
	    }

	    m_state = RECEIVING_HEADER;

	} else if(m_state == RECEIVING_HEADER) {
	    if((npos = response.find(" ")) != -1)
		head = up(response.substr(0, npos));

	    if(head == "CONTENT-LENGTH:") {
		m_length = strtoul(response.substr(npos+1).c_str(), 0, 0);
	    }

	    switch(m_code) {
		case 301:
		case 302:
		    if(head == "LOCATION:") {
			m_redirect = response.substr(npos+1);
			m_socket->Disconnect();
			SignalRemoveSocket(m_socket->getSocketHandle());
			m_state = NOT_CONNECTED;
			Connect();
		    }
		    break;

		case 401:
		    if(head == "WWW-AUTHENTICATE:") {
			if(ev->authTries++) {
			    ev->setHTTPResp("HTTP auth failed");
			    throw HTTPException("Authentification failed");
			}

			response.erase(0, npos+1);

			if((npos = response.find(" ")) != -1) {
			    head = up(response.substr(0, npos));
			    response.erase(0, npos+1);

			    if(head == "DIGEST")
				ev->authmethod = HTTPRequestEvent::Digest;

			    while(!response.empty()) {
				if((npos = response.find("=")) != -1) {
				    head = response.substr(0, npos);
				    response.erase(0, npos+1);

				    if(response.substr(0, 1) == "\"") {
					response.erase(0, 1);
					if((npos = response.find("\"")) != -1)
					    val = response.substr(0, npos);
				    } else {
					if((npos = response.find(",")) != -1)
					    val = response.substr(0, npos);
				    }

				    ev->authparams[head] = val;
				    response.erase(0, npos+1);

				    while(response.substr(0, 1).find_first_of(", \t") != -1)
					response.erase(0, 1);

				} else break;
			    }

			    m_socket->Disconnect();
			    SignalRemoveSocket(m_socket->getSocketHandle());
			    m_state = NOT_CONNECTED;
			    Connect();
			}
		    }
		    break;
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

string HTTPClient::strMethod(HTTPRequestEvent::RequestMethod m) {
    string r;

    switch(m) {
	case HTTPRequestEvent::GET: r = "GET"; break;
	case HTTPRequestEvent::POST: r = "POST"; break;
    }

    return r;
}

static string getMD5(const string &s) {
    md5_state_t state;
    md5_byte_t digest[16];
    char hexdigest[3];
    string r;
    int a;
	
    md5_init(&state);
    md5_append(&state, (md5_byte_t *) s.c_str(), s.size());
    md5_finish(&state, digest);

    for(a = 0; a < 16; a++) {
	sprintf(hexdigest, "%02x", digest[a]);
	r += hexdigest;
    }

    return r;
}

void HTTPClient::SendRequest() {
    Buffer b;
    HTTPRequestEvent *ev = m_queue.front();
    string rq, cnonce;
    int i;

    rq = strMethod(ev->getMethod());

    if(m_proxy_hostname.empty()) {
	b.Pack(rq + " " + m_resource + " HTTP/1.0\r\n");
	b.Pack((string) "Host: " + m_hostname);
	if(m_port != 80) b.Pack((string) ":" + i2str(m_port));
	b.Pack("\r\n");
    } else {
	b.Pack(rq + " http://" + m_hostname + m_resource + " HTTP/1.0\r\n");
    }

    if(!m_proxy_user.empty() && !m_proxy_hostname.empty()) {
	auto_ptr<char> ba(cw_base64_encode((m_proxy_user + ":" + m_proxy_passwd).c_str()));
	b.Pack((string) "Proxy-Authorization: Basic " + ba.get() + "\r\n");
    }

//    b.Pack((string) "Connection: keep-alive\r\n");
    b.Pack((string) "User-Agent: " + PACKAGE + "/" + VERSION + "\r\n");

    if(!ev->m_user.empty()) {
	rq = "";

	switch(ev->authmethod) {
	    case HTTPRequestEvent::Basic: {
		    auto_ptr<char> ba(cw_base64_encode((ev->m_user + ":" + ev->m_pass).c_str()));
		    rq = (string) "Basic " + ba.get();
		}
		break;

	    case HTTPRequestEvent::User:
		rq = (string) "User " + ev->m_user + ":" + ev->m_pass;
		break;

	    case HTTPRequestEvent::Digest:
		if(!ev->authparams.empty()) {
		    rq = "Digest ";

		    for(i = 0; i < 8; i++) {
			char c = randlimit(0, 15);
			if(c < 10) c += 48; else c += 87;
			cnonce += c;
		    }

		    ev->authparams["username"] = ev->m_user;
		    ev->authparams["uri"] = ev->m_url;
		    ev->authparams["nc"] = "00000001";
		    ev->authparams["cnonce"] = cnonce;

		    string a1 = ev->m_user + ":" + ev->authparams["realm"] + ":" + ev->m_pass;
		    string a2 = strMethod(ev->getMethod()) + ":" + ev->m_url;

		    string mid = (string) ":" + ev->authparams["nonce"] + ":" +
			ev->authparams["nc"] + ":" + cnonce + ":" +
			ev->authparams["qop"] + ":";

		    ev->authparams["response"] = getMD5(getMD5(a1) + mid + getMD5(a2));

		    map<string, string>::const_iterator ia = ev->authparams.begin();
		    while(ia != ev->authparams.end()) {
			if(ia != ev->authparams.begin()) rq += ", ";
			rq += ia->first + "=\"" + ia->second + "\"";
			++ia;
		    }
		}

		break;
	}

	if(!rq.empty()) {
	    b.Pack((string) "Authorization: " + rq + "\r\n");
	}
    }

    if(ev->getMethod() == HTTPRequestEvent::POST) {
	int len = 0;
	vector<pair<string, string> >::const_iterator ip = ev->pbegin();

	b.Pack("Content-type: application/x-www-form-urlencoded\r\n");

	while(ip != ev->pend()) {
	    len += ip->first.size() + ip->second.size() + 2;
	    ++ip;
	}

	len--;
	b.Pack((string) "Content-length: " + i2str(len) + "\r\n\r\n");

	for(ip = ev->pbegin(); ip != ev->pend(); ++ip) {
	    if(ip != ev->pbegin()) b.Pack("&");
	    b.Pack(ip->first);
	    b.Pack("=");
	    b.Pack(ip->second);
	}

    }

    b.Pack("\r\n");

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
    m_length = 0;

    try {
	while(m_socket->connected()) {
	    if(!m_socket->Recv(m_recv)) break;
	    Parse();
	    if(m_length && m_recv.size() >= m_length) break;
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

    }

    if(m_length && m_recv.size() >= m_length) {
	Disconnect();
	throw DisconnectedException("Content-Length reached");
    }
}

void HTTPClient::check_timeout() {
    time_t now = time(0);

    if(now-m_last_operation > m_timeout) {
	MessageEvent *ev = *m_queue.begin();
	ev->setDelivered(false);
	ev->setFinished(true);
	dynamic_cast<HTTPRequestEvent*>(ev)->setHTTPResp("Timed out");
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
	list<HTTPRequestEvent *>::const_iterator ir = m_queue.begin();
	while(ir != m_queue.end()) {
	    if(**ir == *rev) {
		delete ev;
		return;
	    }
	    ++ir;
	}

	m_queue.push_back(rev);

	if(m_state == NOT_CONNECTED) {
	    Connect();
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
