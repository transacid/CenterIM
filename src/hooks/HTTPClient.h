#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include "icqcommon.h"

#ifdef BUILD_RSS

#include <libicq2000/SocketClient.h>
#include <libicq2000/buffer.h>
#include <libicq2000/exceptions.h>

using namespace ICQ2000;

class HTTPRequestEvent : public MessageEvent {
    private:
	std::string m_content, m_url, m_httpresp;

    public:
	HTTPRequestEvent(const string &url): MessageEvent(ContactRef()), m_url(url) { }

	string getContent() const { return m_content; }
	string getURL() const { return m_url; }
	string getHTTPResp() const { return m_httpresp; }
	MessageType getType() const { return Normal; }

	void setContent(const string &content) { m_content = content; }
	void setHTTPResp(const string &resp) { m_httpresp = resp; }
};

class HTTPClient : public SocketClient {
    private:
	enum State { NOT_CONNECTED,
	    WAITING_FOR_CONNECT,
	    WAITING_FOR_HEADER,
	    RECEIVING_HEADER,
	    RECEIVING_CONTENT,
	    DISCONNECTING };

	list<HTTPRequestEvent*> m_queue;
	State m_state;

	Translator m_transl;
	Buffer m_recv;

	std::string m_hostname, m_resource, m_content;
	unsigned short m_port;
	time_t m_last_operation, m_timeout;

	void Init();
	void Parse();
	void Send(Buffer &b);

	void SendRequest();
	void Disconnect();

	void check_timeout();

    public:
	HTTPClient();
	~HTTPClient();

	void Connect();
	void FinishNonBlockingConnect();
	void Recv();

	void setTimeout(time_t t);
	time_t getTimeout() const;

	void clearoutMessagesPoll();
	void SendEvent(MessageEvent* ev);

	void socket_cb(int fd, SocketEvent::Mode m);
};

class HTTPException : public std::exception {
    private:
	std::string m_errortext;

    public:
	HTTPException() {}
	HTTPException(const std::string& text): m_errortext(text) {};
	~HTTPException() throw() {}

	const char* what() const throw() { return m_errortext.c_str(); }
};

#endif

#endif
