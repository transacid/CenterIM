/*
 * Message Handler
 *
 * Copyright (C) 2002 Barnaby Gray <barnaby@beedesign.co.uk>
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

#include "MessageHandler.h"
#include "ContactTree.h"
#include "SNAC-MSG.h"
#include "SNAC-SRV.h"
#include "sstream_fix.h"
#include "Translator.h"

using std::string;
using std::ostringstream;
using std::endl;

namespace ICQ2000
{

  MessageHandler::MessageHandler(ContactRef self, ContactTree *cl, Translator * & tr)
    : m_self_contact(self), m_contact_list(cl), m_translator(tr)
  { }
  
  /*
   * This method handles:
   * - Receiving incoming messages
   * - Signalling the message
   * - Setting up the UINICQSubType for the ACK
   * (the actual ACK is sent in the caller - dependent on whether it
   *  was direct/thru server)
   */
  bool MessageHandler::handleIncoming(ICQSubType *ist, time_t t)
  {
    ContactRef contact;
    bool advanced, ack = false;

    if (ist->getType() == MSG_Type_FT)
    {
      ostringstream ostr;
      ostr << "FileTransfer request through server." <<endl;
      SignalLog( LogEvent::WARN, ostr.str() );
      //handleIncomingFT(static_cast<FTICQSubType*>(ist));
      return false;
    }
    
    UINICQSubType *uist = dynamic_cast<UINICQSubType*>(ist);
    MessageEvent *ev = ICQSubTypeToEvent(ist, contact, advanced);
    ICQMessageEvent *mev = dynamic_cast<ICQMessageEvent*>(ev);

    Status st = m_self_contact->getStatus();

    if (advanced) {
      // update the status of the contact with that sent in the packet
      unsigned short s = uist->getStatus();
      contact->setStatus( Contact::MapICQStatusToStatus(s),
			  Contact::MapICQStatusToInvisible(s) );
    } else {
      /* mark all non-advanced (ICQ) messages when in Occupied/DND as 'to
       * contact list', so they can be guaranteed to arrive, as they should
       * never be turned down (no ack is sent back) */
      if (mev != NULL && (st == STATUS_OCCUPIED || st == STATUS_DND))
	mev->setToContactList(true);
    }
    
    if (t == 0) t = ev->getTime();
    else ev->setTime(t);

    ev->setDelivered(true);
    // default to true - for clients that ignore this functionality

    if (ev->getType() != MessageEvent::AwayMessage) {
      messaged.emit(ev);
      contact->set_last_message_time( t );
    } else {
      contact->set_last_away_msg_check_time( t );

      ostringstream ostr;
      ostr << "Away_msg respons to: " << contact->getAlias() << endl;
      SignalLog( LogEvent::INFO, ostr.str() );
    }

    if (advanced) {
      /* All these operations only apply to advanced messages -
       * UINICQSubType messages, those that are associated to a UIN and
       * that were sent as an advanced message (through server or
       * direct, NOT offline), and consequently will the ack'ed.
       */

      /* request away message if we are not online */
      if (st != STATUS_ONLINE && advanced)
      {
	want_auto_resp.emit(mev);
	uist->setAwayMessage( m_translator->client_to_server(mev->getAwayMessage(), ENCODING_CONTACT_LOCALE, contact ) );
      }
      else
      {
	uist->setAwayMessage( "" );
      }

      /* update the UINICQSubType */
	
      uist->setACK(true);
      ack = true;
      
      if (ev->isDelivered())
      {
	/* set accept-status of ACK */
	switch(st) {
	case STATUS_ONLINE:
	  uist->setStatus(AcceptStatus_Online);
	  break;
	case STATUS_AWAY:
	  uist->setStatus(AcceptStatus_Away);
	  break;
	case STATUS_NA:
	  uist->setStatus(AcceptStatus_NA);
	  break;
	case STATUS_OCCUPIED:
	  uist->setStatus(AcceptStatus_Occ_Accept);
	  break;
	default:
	  uist->setStatus(AcceptStatus_Online);
	}
      } else {
	MessageEvent::DeliveryFailureReason r = ev->getDeliveryFailureReason();
	/* set accept-status of ACK */
	switch(r) {
	case MessageEvent::Failed_Denied:
	  uist->setStatus(AcceptStatus_Denied);
	  break;
	case MessageEvent::Failed_Occupied:
	  uist->setStatus(AcceptStatus_Occupied);
	  break;
	case MessageEvent::Failed_DND:
	  uist->setStatus(AcceptStatus_DND);
	  break;
	case MessageEvent::Failed_Ignored:
	  ack = false;
	  break;
	default:
	  uist->setStatus(AcceptStatus_Denied);
	}
      }

    }

    // delete the temporary event
    delete ev;

    // whether the message needs ack'ing
    return ack;
  }

  FileTransferEvent* MessageHandler::handleIncomingFT(FTICQSubType *ist,
						      bool direct)
  {
    ContactRef contact = lookupUIN( ist->getSource() );
    FileTransferEvent *ev = new FileTransferEvent(contact, ist->getMessage(),
						  ist->getDescription(),
						  ist->getSize(), ist->getSeqNum());

    ev->setDelivered(true);
    ev->setDirect(direct);
    ev->setFinished(true);

    ostringstream ostr;
    ostr << "Incoming file transfer received.";
    SignalLog(LogEvent::INFO, ostr.str() );

    filetransfer_incoming_signal.emit(ev);
    return ev;
  }
	
  /*
   * This method handles:
   * - Setting up the UINICQSubType for Sending messages
   * (the actual message is sent by the caller -
   *  dependent on whether it is going direct/thru server)
   */
  UINICQSubType* MessageHandler::handleOutgoing(MessageEvent *ev)
  {
    UINICQSubType *icq = EventToUINICQSubType(ev);

    /* our status is sent in packets */
    icq->setStatus( Contact::MapStatusToICQStatus(m_self_contact->getStatus(), m_self_contact->isInvisible() ) );
    icq->setDestination( ev->getContact()->getUIN() );
    icq->setSource( m_self_contact->getUIN() );

    return icq;
  }

  void MessageHandler::handleIncomingFTACK(FileTransferEvent *ev, FTICQSubType *ft)
  {
    if ( ft->getStatus()==AcceptStatus_Online && ( ft->getPort() != 0 || ft->getRevPort() != 0 ) )
      ev->setState(FileTransferEvent::ACCEPTED);
    else
      ev->setState(FileTransferEvent::REJECTED);

    ev->setRefusalMessage(ft->getMessage());
    ev->setPort(ft->getPort());
    if ( ev->getPort()==0 )
      ev->setPort( ft->getRevPort() );
    ostringstream ostr;
    ostr << "Incoming file transfer ACK: ";
        
    if (ev->getState() == FileTransferEvent::ACCEPTED)
      ostr << "Is Accepted to port: " << ev->getPort();
    else
      ostr << "Is Rejected: " << ev->getRefusalMessage();
    
    SignalLog(LogEvent::INFO, ostr.str() );

    filetransfer_update_signal.emit(ev);
  }
	
  void MessageHandler::handleIncomingACK(MessageEvent *ev, UINICQSubType *icq)
  {
    ICQMessageEvent *aev = dynamic_cast<ICQMessageEvent*>(ev);
    if (aev == NULL) return;

    aev->setAwayMessage( m_translator->server_to_client( icq->getAwayMessage(), ENCODING_CONTACT_LOCALE, ev->getContact() ) );
    aev->setFinished(true);

    switch(icq->getStatus()) {
    case AcceptStatus_Online:
      aev->setDelivered(true);
      break;
    case AcceptStatus_Denied:
      aev->setDelivered(false);
      aev->setDeliveryFailureReason(MessageEvent::Failed_Denied);
      break;
    case AcceptStatus_Away:
      aev->setDelivered(true);
      break;
    case AcceptStatus_Occupied:
      aev->setDelivered(false);
      aev->setDeliveryFailureReason(MessageEvent::Failed_Occupied);
      break;
    case AcceptStatus_DND:
      aev->setDelivered(false);
      aev->setDeliveryFailureReason(MessageEvent::Failed_DND);
      break;
    case AcceptStatus_Occ_Accept:
      aev->setDelivered(true);
      break;
    case AcceptStatus_NA:
      aev->setDelivered(true);
      break;
    default:
      {
	ostringstream ostr;
	ostr << "Unknown accept-status in ACK: " << icq->getStatus() << endl;
	SignalLog( LogEvent::WARN, ostr.str() );
      }
    }

    if (aev->getType() == MessageEvent::AwayMessage)
    {
      // count any ack to an away message request as always delivered
      aev->setDelivered(true);
    }
    else if (aev->getType() == MessageEvent::FileTransfer) {
	 handleIncomingFTACK(static_cast<FileTransferEvent*>(ev),
			     static_cast<FTICQSubType*>(icq));

	 aev->setDelivered(true);
    }
    
    messageack.emit(ev);
  }

  void MessageHandler::handleUpdateFT(FileTransferEvent *ev)
  {
    filetransfer_update_signal.emit(ev);
  }
  
  void MessageHandler::handleIncomingFTCancel(FileTransferEvent *ev)
  {
    ev->setState(FileTransferEvent::CANCELLED);
    ostringstream ostr;
    ostr << "FileTransfer cancel received" << endl;
    SignalLog( LogEvent::WARN, ostr.str() );
    filetransfer_update_signal.emit(ev);
  }
	
  /**
   *  Convert a UINICQSubType into an ICQMessageEvent
   */
  ICQMessageEvent* MessageHandler::UINICQSubTypeToEvent(UINICQSubType *st, const ContactRef& contact)
  {
    ICQMessageEvent *e = NULL;
    unsigned short type = st->getType();
    
    switch(type) {

    case MSG_Type_Normal:
    {
      NormalICQSubType *nst = static_cast<NormalICQSubType*>(st);
      e = new NormalMessageEvent(contact,
				 m_translator->server_to_client( nst->getMessage(), ENCODING_CONTACT_LOCALE, contact ),
				 nst->isMultiParty() );
      break;
    }

    case MSG_Type_URL:
    {
      URLICQSubType *ust = static_cast<URLICQSubType*>(st);
      e = new URLMessageEvent(contact,
			      m_translator->server_to_client( ust->getMessage(), ENCODING_CONTACT_LOCALE, contact ),
			      m_translator->server_to_client( ust->getURL(), ENCODING_CONTACT_LOCALE, contact ));
      break;
    }

    case MSG_Type_AuthReq:
    {
      AuthReqICQSubType *ust = static_cast<AuthReqICQSubType*>(st);
      e = new AuthReqEvent(contact, m_translator->server_to_client( ust->getMessage(), ENCODING_CONTACT_LOCALE, contact ) );
      break;
    }

    case MSG_Type_AuthRej:
    {
      AuthRejICQSubType *ust = static_cast<AuthRejICQSubType*>(st);
      e = new AuthAckEvent(contact, m_translator->server_to_client( ust->getMessage(), ENCODING_CONTACT_LOCALE, contact ), false);
      break;
    }

    case MSG_Type_AuthAcc:
    {
      e = new AuthAckEvent(contact, true);
      e->getContact()->setAuthAwait(false);
      break;
    }

    case MSG_Type_AutoReq_Away:
    case MSG_Type_AutoReq_Occ:
    case MSG_Type_AutoReq_NA:
    case MSG_Type_AutoReq_DND:
    case MSG_Type_AutoReq_FFC:
    {
      e = new AwayMessageEvent(contact);
      break;
    }

    case MSG_Type_UserAdd:
    {
      e = new UserAddEvent(contact);
      break;
    }

    case MSG_Type_Contact:
    {
      ContactICQSubType *cst = static_cast<ContactICQSubType*>(st);
      e = new ContactMessageEvent(contact, cst->getContacts());
      break;
    }

    default:
      break;

    } // end of switch
    
    if (e != NULL) {
      e->setUrgent( st->isUrgent() );
      e->setToContactList( st->isToContactList() );
    }
    
    return e;
  }
  
  /**
   *  Convert an ICQSubType into a MessageEvent
   */
  MessageEvent* MessageHandler::ICQSubTypeToEvent(ICQSubType *st, ContactRef& contact, bool& adv)
  {
    MessageEvent *e = NULL;

    adv = false;

    switch(st->getType()) {
    case MSG_Type_Normal:
    case MSG_Type_URL:
    case MSG_Type_AuthReq:
    case MSG_Type_AuthRej:
    case MSG_Type_AuthAcc:
    case MSG_Type_AutoReq_Away:
    case MSG_Type_AutoReq_Occ:
    case MSG_Type_AutoReq_NA:
    case MSG_Type_AutoReq_DND:
    case MSG_Type_AutoReq_FFC:
    case MSG_Type_UserAdd:
    case MSG_Type_FT:
    case MSG_Type_Contact:
    {
      UINICQSubType *ist = static_cast<UINICQSubType*>(st);
      adv = ist->isAdvanced();
      contact = lookupUIN( ist->getSource() );
      e = UINICQSubTypeToEvent(ist, contact);
      break;
    }

    case MSG_Type_EmailEx:
    {
      // these come from 'magic' UIN 10
      EmailExICQSubType *subtype = static_cast<EmailExICQSubType*>(st);
      std::string email = m_translator->server_to_client( subtype->getEmail(), ENCODING_ISO_8859_1, contact );
      contact = lookupEmail( email, subtype->getSender() );
      e = new EmailExEvent(contact,
			   email,
			   m_translator->server_to_client( subtype->getSender(), ENCODING_ISO_8859_1, contact ),
			   m_translator->server_to_client( subtype->getMessage(), ENCODING_ISO_8859_1, contact ));
      break;
    }

    case MSG_Type_WebPager:
    {
      WebPagerICQSubType *subtype = static_cast<WebPagerICQSubType*>(st);
      std::string email = m_translator->server_to_client( subtype->getEmail(), ENCODING_ISO_8859_1, contact );
      contact = lookupEmail( email, subtype->getSender() );
      e = new WebPagerEvent(contact,
			    email,
			    m_translator->server_to_client( subtype->getEmail(), ENCODING_ISO_8859_1, contact ),
			    m_translator->server_to_client( subtype->getMessage(), ENCODING_ISO_8859_1, contact ));
      break;
    }

    case MSG_Type_SMS:
    {
      /* TODO: Encoding!?! Someone told me once SMSs are UTF-8 encoded.. need to verify! */
      SMSICQSubType *sst = static_cast<SMSICQSubType*>(st);
      if (sst->getSMSType() == SMSICQSubType::SMS) {
	contact = lookupMobile(sst->getSender());
	e = new SMSMessageEvent(contact, sst->getMessage(),
				sst->getSource(),sst->getSenders_network(),
				sst->getTime());
      } else if (sst->getSMSType() == SMSICQSubType::SMS_Receipt) {
	contact = lookupMobile(sst->getDestination());
	e = new SMSReceiptEvent(contact, sst->getMessage(),
				sst->getMessageId(), sst->getSubmissionTime(),
				sst->getDeliveryTime(), sst->delivered());
      }
      break;
    }
    
    default:
      break;

    } // end of switch
    
    return e;
  }
  
  /**
   * Convert a MessageEvent into a UINICQSubType
   */
  UINICQSubType* MessageHandler::EventToUINICQSubType(MessageEvent *ev)
  {
    ContactRef c = ev->getContact();
    UINICQSubType *ist = NULL;

    if (ev->getType() == MessageEvent::Normal) {

      NormalMessageEvent *nv = static_cast<NormalMessageEvent*>(ev);
      ist = new NormalICQSubType( m_translator->client_to_server( nv->getMessage(), ENCODING_CONTACT_LOCALE, c ) );

    } else if (ev->getType() == MessageEvent::URL) {

      URLMessageEvent *uv = static_cast<URLMessageEvent*>(ev);
      ist = new URLICQSubType( m_translator->client_to_server( uv->getMessage(), ENCODING_CONTACT_LOCALE, c ),
			       m_translator->client_to_server( uv->getURL(),     ENCODING_CONTACT_LOCALE, c ));

    } else if (ev->getType() == MessageEvent::FileTransfer) {

      FileTransferEvent *uv = static_cast<FileTransferEvent*>(ev);
      ist = new FTICQSubType( m_translator->client_to_server( uv->getMessage(), ENCODING_CONTACT_LOCALE, c ),
						m_translator->client_to_server( uv->getDescription(),     ENCODING_CONTACT_LOCALE, c ), uv->getTotalSize());
					    
   
    } else if (ev->getType() == MessageEvent::AwayMessage) {

      ist = new AwayMsgSubType( c->getStatus() );

    } else if (ev->getType() == MessageEvent::AuthReq) {

      AuthReqEvent *uv = static_cast<AuthReqEvent*>(ev);
      ist = new AuthReqICQSubType( m_translator->client_to_server( m_self_contact->getAlias(),     ENCODING_CONTACT_LOCALE, c ),
				   m_translator->client_to_server( m_self_contact->getFirstName(), ENCODING_CONTACT_LOCALE, c ),
				   m_translator->client_to_server( m_self_contact->getLastName(),  ENCODING_CONTACT_LOCALE, c ),
				   m_translator->client_to_server( m_self_contact->getEmail(),     ENCODING_CONTACT_LOCALE, c ),
				   m_self_contact->getAuthReq(),
				   m_translator->client_to_server( uv->getMessage(),               ENCODING_CONTACT_LOCALE, c ));

    }
    else if (ev->getType() == MessageEvent::AuthAck)
    {

      AuthAckEvent *uv = static_cast<AuthAckEvent*>(ev);
      if(uv->isGranted())
        ist = new AuthAccICQSubType();
      else
        ist = new AuthRejICQSubType( m_translator->client_to_server( uv->getMessage(), ENCODING_CONTACT_LOCALE, c ) );
      
    }
    else if (ev->getType() == MessageEvent::UserAdd)
    {
      ist = new UserAddICQSubType( m_translator->client_to_server( m_self_contact->getAlias(),     ENCODING_CONTACT_LOCALE, c ),
				   m_translator->client_to_server( m_self_contact->getFirstName(), ENCODING_CONTACT_LOCALE, c ),
				   m_translator->client_to_server( m_self_contact->getLastName(),  ENCODING_CONTACT_LOCALE, c ),
				   m_translator->client_to_server( m_self_contact->getEmail(),     ENCODING_CONTACT_LOCALE, c ),
				   m_self_contact->getAuthReq() );
    }
    else if (ev->getType() == MessageEvent::Contacts)
    {
      ContactMessageEvent *cv = static_cast<ContactMessageEvent*>(ev);
      ist = new ContactICQSubType(cv->getContacts());
    }
    
    ICQMessageEvent *iev;
    if (ist != NULL && (iev = dynamic_cast<ICQMessageEvent*>(ev)) != NULL) {
      ist->setUrgent( iev->isUrgent() );
      ist->setToContactList( iev->isToContactList() );
    }
    
    return ist;
  }

  void MessageHandler::SignalLog(LogEvent::LogType type, const string& msg) {
    LogEvent ev(type,msg);
    logger.emit(&ev);
  }

  ContactRef MessageHandler::lookupUIN(unsigned int uin) {
    ContactRef ret;

    if (m_contact_list->exists(uin)) {
      ret = m_contact_list->lookup_uin(uin);
    } else {
      ret = ContactRef( new Contact(uin) );
    }

    return ret;
  }

  ContactRef MessageHandler::lookupMobile(const string& m) {
    ContactRef ret;
    
    if (m_contact_list->mobile_exists(m)) {
      ret = m_contact_list->lookup_mobile(m);
    } else {
      ret = ContactRef( new Contact(m) );
      ret->setMobileNo(m);
    }

    return ret;
  }

  ContactRef MessageHandler::lookupEmail(const string& email, const string& alias) {
    ContactRef ret;
    
    if (m_contact_list->email_exists(email)) {
      ret = m_contact_list->lookup_email(email);
    } else {
      ret = ContactRef( new Contact(alias) );
      ret->setEmail(email);
    }

    return ret;
  }

}
