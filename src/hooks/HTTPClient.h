#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include "icqcommon.h"

#ifdef BUILD_RSS

#include <libicq2000/SocketClient.h>
#include <libicq2000/buffer.h>
#include <libicq2000/exceptions.h>

using namespace ICQ2000;

class HTTPRequestEvent : public MessageEvent {
    public:
	enum RequestMethod {
	    GET,
	    POST
	};

    private:
	string m_content, m_url, m_httpresp;
	vector<pair<string, string> > params;

	RequestMethod method;

    public:
	HTTPRequestEvent(const string &url, RequestMethod rt = GET)
	    : MessageEvent(ContactRef()), m_url(url), method(rt) { }

	string getContent() const { return m_content; }
	string getURL() const { return m_url; }
	string getHTTPResp() const { return m_httpresp; }

	MessageType getType() const { return Normal; }
	RequestMethod getMethod() const { return method; }

	void setContent(const string &content) { m_content = content; }
	void setHTTPResp(const string &resp) { m_httpresp = resp; }

	void addParam(const string pname, const string pval)
	    { params.push_back(make_pair(mime(pname), mime(pval))); }

	vector<pair<string, string> >::const_iterator pbegin() const { return params.begin(); }
	vector<pair<string, string> >::const_iterator pend() const { return params.end(); }

	bool operator == (HTTPRequestEvent &ev) const
	    { return m_url == ev.m_url && params == ev.params && method == ev.method; }

	bool operator != (HTTPRequestEvent &ev) const
	    { return !(*this == ev); }
};

class HTTPClient : public SocketClient {
    private:
	enum State {
	    NOT_CONNECTED,
	    WAITING_FOR_CONNECT,
	    WAITING_FOR_HEADER,
	    RECEIVING_HEADER,
	    RECEIVING_CONTENT,
	    DISCONNECTING
	};

	list<HTTPRequestEvent*> m_queue;
	State m_state;

	Translator m_transl;
	Buffer m_recv;

	string m_hostname, m_proxy_hostname;
	string m_resource, m_content, m_redirect;
	unsigned short m_port, m_proxy_port;
	time_t m_last_operation, m_timeout;
	int m_code;

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

	void setProxyServerHost(const string& host) { m_proxy_hostname = host; }
	string getProxyServerHost() const { return m_proxy_hostname; }

	void setProxyServerPort(const unsigned short& port) { m_proxy_port = port; }
	unsigned short getProxyServerPort() const { return m_proxy_port; }
};

class HTTPException : public exception {
    private:
	string m_errortext;

    public:
	HTTPException() {}
	HTTPException(const string& text): m_errortext(text) {};
	~HTTPException() throw() {}

	const char* what() const throw() { return m_errortext.c_str(); }
};

#endif

#endif
