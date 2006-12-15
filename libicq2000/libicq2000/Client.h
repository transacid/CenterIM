/**
 * libICQ2000 Client header
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

#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <string>

#include <map>

#include <libicq2000/sigslot.h>

#include <time.h>

#include <libicq2000/events.h>
#include <libicq2000/constants.h>
#include <libicq2000/Contact.h>
#include <libicq2000/ContactTree.h>
#include <libicq2000/ContactList.h>
#include <libicq2000/userinfoconstants.h>

namespace ICQ2000
{
  
  /* predeclare classes */
  class MessageSNAC;
  class MessageACKSNAC;
  class MessageOfflineUserSNAC;
  class SrvResponseSNAC;
  class UINResponseSNAC;
  class RateInfoChangeSNAC;
  class BuddyOnlineSNAC;
  class BuddyOfflineSNAC;
  class UserInfoSNAC;
  class OutSNAC;
  class DirectClient;
  class DCCache;
  class MessageHandler;
  class RequestIDCache;
  class RequestIDCacheValue;
  class ICBMCookieCache;
  class SMTPClient;
  class SocketClient;
  class Buffer;
  class TCPSocket;
  class TCPServer;
  class Translator;
  class FileTransferClient;
  class FTCache;

  /**
   *  The main library object.  This is the object the user interface
   *  instantiates for a connection, hooks up to signal on and has the
   *  methods to connect, disconnect and send events from.
   */
  class Client : public sigslot::has_slots<>
  {
   private:
    enum State { NOT_CONNECTED,
		 AUTH_AWAITING_CONN_ACK,
		 AUTH_AWAITING_AUTH_REPLY,
		 BOS_AWAITING_CONN_ACK,
		 BOS_AWAITING_LOGIN_REPLY,
		 BOS_LOGGED_IN,
		 UIN_AWAITING_CONN_ACK,
		 UIN_AWAITING_UIN_REPLY
    } m_state;

    ContactRef m_self;
    std::string m_password;
    Status m_status_wanted;
    bool m_invisible_wanted;
    bool m_web_aware;
    unsigned short m_random_group;

    std::string m_authorizerHostname;
    unsigned short m_authorizerPort;

    std::string m_bosHostname;
    unsigned short m_bosPort;
    bool m_bosOverridePort;

    std::string m_client_bind_host;

    bool m_in_dc, m_out_dc;

    unsigned short m_client_seq_num;
    unsigned int m_requestid;
    
    Translator * m_translator;

    ContactTree m_contact_tree;
    
    ContactList m_visible_list;
    ContactList m_invisible_list;

    bool m_fetch_sbl;

    MessageHandler * m_message_handler;

    unsigned char *m_cookie_data;
    unsigned short m_cookie_length;

    unsigned int m_ext_ip;
    bool m_use_portrange;
    bool m_use_typing_notif;
    unsigned short m_upper_port, m_lower_port;
    TCPSocket * m_serverSocket;
    TCPServer * m_listenServer;

    SMTPClient * m_smtp;

    DCCache * m_dccache;
    FTCache * m_ftcache;

    time_t m_last_server_ping;

    RequestIDCache * m_reqidcache;
    ICBMCookieCache * m_cookiecache;

    Buffer * m_recv;
   
    void Init();
    unsigned short NextSeqNum();
    unsigned int NextRequestID();

    void ConnectAuthorizer(State state);
    void DisconnectAuthorizer();
    void ConnectBOS();
    void DisconnectBOS();

    // -- Ping server --
    void PingServer();

    DirectClient* ConnectDirect(const ContactRef& c);
    void DisconnectDirectConns();
    void DisconnectDirectConn(int fd);

    // ------------------ Signal dispatchers -----------------
    void SignalConnect();
    void SignalDisconnect(DisconnectedEvent::Reason r);
    void SignalMessage(MessageSNAC *snac);
    void SignalMessageACK(MessageACKSNAC *snac);
    void SignalMessageOfflineUser(MessageOfflineUserSNAC *snac);
    void SignalSrvResponse(SrvResponseSNAC *snac);
    void SignalUINResponse(UINResponseSNAC *snac);
    void SignalUINRequestError();
    void SignalRateInfoChange(RateInfoChangeSNAC *snac);
    void SignalLog(LogEvent::LogType type, const std::string& msg);
    void SignalUserOnline(BuddyOnlineSNAC *snac);
    void SignalUserOffline(BuddyOfflineSNAC *snac);
    void SignalAddSocket(int fd, SocketEvent::Mode m);
    void SignalRemoveSocket(int fd);
    // ------------------ Outgoing packets -------------------

    // -------------- Callbacks from ContactList -------------
    void contactlist_cb(ContactListEvent *ev);

    // ------- Callbacks from visible, invisible lists -------
    void visiblelist_cb(ContactListEvent *ev);
    void invisiblelist_cb(ContactListEvent *ev);

    // -------------- Callbacks from Contacts ----------------

    void SendAuthReq();
    void SendNewUINReq();
    void SendCookie();
    void SendCapabilities();
    void SendRateInfoRequest();
    void SendRateInfoAck();
    void SendPersonalInfoRequest();
    void SendAddICBMParameter();
    void SendSetUserInfo();
    void SendLogin();
    void SendRequestSBL();
    void SendSBLReceivedACK();
    void SendOfflineMessagesACK();

    void SendAdvancedACK(MessageSNAC *snac);

    void Send(Buffer& b);

    void HandleUserInfoSNAC(UserInfoSNAC *snac);

    void FLAPwrapSNAC(Buffer& b, const OutSNAC& snac);
    void FLAPwrapSNACandSend(const OutSNAC& snac);

    // ------------------ Incoming packets -------------------

    /**
     *  non-blocking receives all waiting packets from server
     *  and parses and handles them
     */
    void RecvFromServer();

    void Parse();
    void ParseCh1(Buffer& b, unsigned short seq_num);
    void ParseCh2(Buffer& b, unsigned short seq_num);
    void ParseCh3(Buffer& b, unsigned short seq_num);
    void ParseCh4(Buffer& b, unsigned short seq_num);

    // -------------------------------------------------------

    ContactRef getUserInfoCacheContact(unsigned int reqid);

    void mergeSBL(ContactTree& tree);

    void ICBMCookieCache_expired_cb(MessageEvent *ev);
    void dccache_expired_cb(DirectClient *dc);
    void ftcache_expired_cb(FileTransferClient *ftc);
    void reqidcache_expired_cb( RequestIDCacheValue *v );
    void dc_connected_cb(SocketClient *dc);
    void dc_log_cb(LogEvent *ev);
    void dc_socket_cb(SocketEvent *ev);
    void dc_messageack_cb(MessageEvent *ev);
    void ftc_messageack_cb(MessageEvent *ev);

    bool SendDirect(MessageEvent *ev);

    void SendViaServer(MessageEvent *ev);
    void SendViaServerAdvanced(MessageEvent *ev);
    void SendViaServerNormal(MessageEvent *ev);
    
    void Disconnect(DisconnectedEvent::Reason r = DisconnectedEvent::REQUESTED);

   public:
    Client();
    Client(const unsigned int uin, const std::string& password);
    ~Client();
   
    void setUIN(unsigned int uin);
    unsigned int getUIN() const;
    void setPassword(const std::string& password);
    std::string getPassword() const;

    ContactRef getSelfContact();

    bool setTranslationMap(const std::string& szMapFileName);
    const std::string& getTranslationMapFileName() const;
    const std::string& getTranslationMapName() const;
    bool usingDefaultMap() const;

    // -- Signals --
    /**
     *  The signal to connect to for listening to ConnectingEvent's.
     *  A ConnectingEvent is signalled when the client is trying to go online.
     * @see connected, ConnectingEvent
     */
    sigslot::signal1<ConnectingEvent*> connecting;

    /**
     *  The signal to connect to for listening to ConnectedEvent's.
     *  A ConnectedEvent is signalled when the client is online proper.
     * @see disconnected, ConnectedEvent
     */
    sigslot::signal1<ConnectedEvent*> connected;

    sigslot::signal1<SBLReceivedEvent*> sbl_received;

    /**
     *  The signal to connect to for listening to DisconnectedEvent's.
     *  A DisconnectedEvent is signalled when you were disconnected
     *  from the server.  This could have been because it was
     *  requested, or the server might have chucked you off. More
     *  information can be in the DisconnectedEvent.
     *
     *  DisconnectedEvent's don't necessarily match a ConnectedEvent,
     *  if you try connecting with an incorrect password, you will
     *  never get a ConnectedEvent before the DisconnectedEvent
     *  signalling incorrect password.
     * @see connected, DisconnectedEvent
     */
    sigslot::signal1<DisconnectedEvent*> disconnected;

    /**
     *  The signal to connect to for listening to incoming
     *  MessageEvent. This includes far more than just messages.
     * @see MessageEvent
     */
    sigslot::signal1<MessageEvent*> messaged;

    /**
     *  The signal to connect to for listening to the acknowledgements
     *  that the library will generate for when the remote client
     *  sends back a message ack. Additionally it will it is used for
     *  signalling to the Client message delivery failures and when
     *  messages are being reattempted to be send through the server.
     * @see messaged, MessageEvent
     */
    sigslot::signal1<MessageEvent*> messageack;

    /**
     *  The signal to connect to for listening to Contact list events.
     * @see ContactListEvent
     */
    sigslot::signal1<ContactListEvent*> contactlist;

    /**
     *  The signal to connect to for listening for Contact Userinfo events.
     * @see UserInfoChangeEvent
     */
    sigslot::signal1<UserInfoChangeEvent*> contact_userinfo_change_signal;

    /**
     *  The signal to connect to for listening for Contact Status change events.
     * @see StatusChangeEvent
     */
    sigslot::signal1<StatusChangeEvent*> contact_status_change_signal;

    /**
     *  The signal for when registering a new UIN has succeeded or
     *  failed after a call to RegisterUIN().
     * @see NewUINEvent, RegisterUIN
     */
    sigslot::signal1<NewUINEvent*> newuin;

    /**
     *  The signal for when the server signals the rate at which the client
     *  is sending has been changed.
     * @see RateInfoChangeEvent
     */
    sigslot::signal1<RateInfoChangeEvent*> rate;

    /**
     *  The signal for all logging messages that are passed back to
     *  the client.  This leads to a very flexible logging system, as
     *  the user interface may decide where to write the log message
     *  to (stdout, a dialog box, etc..) and also may pick which type
     *  of log messages to display and which to ignore.
     * @see LogEvent
     */
    sigslot::signal1<LogEvent*> logger;

    /**
     *  The signal for socket events. All clients must listen to this
     *  and implement their particular scheme of blocking on multiple
     *  sockets for read/write/exception, usually the select system
     *  call in someway. Often toolkits will hide all the details of
     *  select inside them, such as the way gtk or gtkmm do.
     *
     * @see SocketEvent
     */
    sigslot::signal1<SocketEvent*> socket;

    /**
     *  The signal to connect to for listening for Self Contact Userinfo events.
     * @see UserInfoChangeEvent
     */
    sigslot::signal1<UserInfoChangeEvent*> self_contact_userinfo_change_signal;

    /**
     *  The signal to connect to for listening for Self Contact Status change events.
     * @see StatusChangeEvent
     */
    sigslot::signal1<StatusChangeEvent*> self_contact_status_change_signal;

    /**
     *  Signal when someone requests your away message. The client
     *  should setMessage in the AutoMessageEvent to what your away
     *  message is. This allows dynamic away messages for different
     *  people.
     */
    sigslot::signal1<ICQMessageEvent*> want_auto_resp;

    /**
     *  Signal when a Search Result has been updated.  The last signal
     *  on a search result will be with
     *  SearchResultEvent::isFinished() set to true. After this the
     *  event is finished and deleted from memory by the library.
     */
    sigslot::signal1<SearchResultEvent*> search_result;

    /**
     *  Signal when an incoming file transfer request is received.
     */
    sigslot::signal1<FileTransferEvent*> filetransfer_incoming_signal;
    
    /**
     *  Signal when a FileTransferEvent object is updated.
     */
    sigslot::signal1<FileTransferEvent*> filetransfer_update_signal;
    
    // -------------

    // -- Send calls --
    void SendEvent(MessageEvent *ev);

    // -- File Transfer methods --
    void SendFileTransfer(FileTransferEvent *ev);
    void SendFileTransferACK(FileTransferEvent *ev);
    void CancelFileTransfer(FileTransferEvent *ev);
	
    // -- Set Status --
    void setStatus(const Status st);
    void setStatus(const Status st, bool inv);
    void setInvisible(bool inv);
    void setWebAware(bool wa);
    void setRandomChatGroup(unsigned short group);
    
    Status getStatus() const;
    bool getInvisible() const;

    Status getStatusWanted() const;
    bool getInvisibleWanted() const;

    bool getWebAware() const;

    void uploadSelfDetails();
    
    void uploadServerBasedContact(const ContactRef& c);
    void uploadServerBasedGroup(const ContactTree::Group& gp);
    void uploadServerBasedContactList();

    void updateServerBasedContact(const ContactRef& c);

    void removeServerBasedContact(const ContactRef& c);
    void removeServerBasedGroup(const ContactTree::Group& gp);
    void removeServerBasedContactList();

    // -- Contact List --
    void addVisible(ContactRef c);
    void removeVisible(const unsigned int uin);
    void addInvisible(ContactRef c);
    void removeInvisible(const unsigned int uin);
    ContactRef getContact(const unsigned int uin);

    ContactTree& getContactTree();

    void fetchSimpleContactInfo(ContactRef c);
    void fetchDetailContactInfo(ContactRef c);
    void fetchServerBasedContactList();
    void fetchServerBasedContactList(int); // the conditional form - TODO!
    void fetchSelfSimpleContactInfo();
    void fetchSelfDetailContactInfo();

    // -- Whitepage searches --
    SearchResultEvent* searchForContacts(const std::string& nickname, const std::string& firstname,
					 const std::string& lastname);

    SearchResultEvent* searchForContacts(const std::string& nickname, const std::string& firstname,
					 const std::string& lastname, const std::string& email,
					 AgeRange age, Sex sex, unsigned char language, const std::string& city,
					 const std::string& state, unsigned short country,
					 const std::string& company_name, const std::string& department,
					 const std::string& position, bool only_online);

    SearchResultEvent* searchForContacts(unsigned int uin);

    SearchResultEvent* searchForContacts(const std::string& keyword);

    SearchResultEvent* searchForContacts(RandomChatGroup group);

    /*
     *  Poll must be called regularly (at least every 60 seconds)
     *  but I recommended 5 seconds, so timeouts work with good
     *  granularity.
     *  It is not related to the socket callback - the client using
     *  this library must select() on the sockets it gets signalled
     *  and call socket_cb when select returns a status flag on one
     *  of the sockets. ickle simply uses the gtk-- built in signal handlers
     *  to do all this.
     */

    // -- Network settings --
    void setLoginServerHost(const std::string& host);
    std::string getLoginServerHost() const;

    void setLoginServerPort(const unsigned short& port);
    unsigned short getLoginServerPort() const;

    void setBOSServerOverridePort(const bool& b);
    bool getBOSServerOverridePort() const;

    void setBOSServerPort(const unsigned short& port);
    unsigned short getBOSServerPort() const;

    void setSMTPServerHost(const std::string& host);
    std::string getSMTPServerHost() const;

    void setSMTPServerPort(unsigned short port);
    unsigned short getSMTPServerPort() const;

    void setAcceptInDC(bool d);
    bool getAcceptInDC() const;

    void setUseOutDC(bool d);
    bool getUseOutDC() const;

    void setPortRangeLowerBound(unsigned short lower);
    void setPortRangeUpperBound(unsigned short upper);
    unsigned short getPortRangeLowerBound() const;
    unsigned short getPortRangeUpperBound() const;

    void setUsePortRange(bool b);
    bool getUsePortRange() const;

    void setClientBindHost(const std::string& host);
    std::string getClientBindHost() const;

    void setTypingNotifications(bool b);

    void Poll();
    void socket_cb(int fd, SocketEvent::Mode m);

    void RegisterUIN();

    /* isConnected() is a convenience for the
     * client, it should correspond exactly to ConnectedEvents
     * & DisconnectedEvents the client gets
     */
    bool isConnected() const;

    void set_translator(Translator * t);
  };
}

#endif
