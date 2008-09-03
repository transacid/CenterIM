#include "imotr.h"
#include "icqconf.h"
#include "icqface.h"
#include "icqcontacts.h"
#include "abstracthook.h"
#include "centerim.h"
#include "hooks/jabberhook.h"
#include "imevents.h"

/* libotr headers */
#ifdef __cplusplus
extern "C" {
#endif
#include <libotr/privkey.h>
#include <libotr/proto.h>
#include <libotr/message.h>
#include <libotr/userstate.h>
#ifdef __cplusplus
}
#endif


imotr otr;

//#define OTRDEBUG


static OtrlPolicy policy_cb(void *opdata, ConnContext *context)
{
#ifdef OTRDEBUG
//    face.log(_("[OTR-DEBUG] policy_cb(...) - returning OTRL_POLICY_DEFAULT"));
#endif
    return OTRL_POLICY_DEFAULT;         // TODO: add user specific policies
    
    return (OTRL_POLICY_ALLOW_V2 | OTRL_POLICY_REQUIRE_ENCRYPTION | OTRL_POLICY_SEND_WHITESPACE_TAG |
            OTRL_POLICY_WHITESPACE_START_AKE | OTRL_POLICY_ERROR_START_AKE);
}


static void create_privkey_cb(void *opdata, const char *accountname,
    const char *protocol)
{
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] create_privkey_cb(..., " + string(accountname) + ")");
#endif
    // Generate the key
    otrl_privkey_generate(otr.get_userstate(), (string(conf->getdirname()) + PRIVKEYFNAME).c_str(), accountname, protocol);
    
    // gaim-otr: otrg_ui_update_fingerprint();
}


static int is_logged_in_cb(void *opdata, const char *accountname, const char *protocol, const char *recipient)
{
    icqcontact *c;
    string recipientname = recipient;
    recipientname = recipientname.substr(0, recipientname.find_last_of('/'));                      // removing "/<instandmessager>"

    if (c = clist.get(imcontact(recipientname, jabber)))
    {
        if (c->getstatus() == offline)
        {
#ifdef OTRDEBUG
            face.log("[OTR-DEBUG] is_logged_in_cb(..., " + string(accountname) + ", " + string(protocol)+ ", " + string(recipient) + ") - returning 0");
#endif
            return 0;               //  0: offline
        } else
        {
#ifdef OTRDEBUG
            face.log("[OTR-DEBUG] is_logged_in_cb(..., " + string(accountname) + ", " + string(protocol)+ ", " + string(recipient) + ") - returning 1");
#endif
            return 1;               //  1: online
        }
    } else 
    {                               // recipient not found in contact list
#ifdef OTRDEBUG
            face.log("[OTR-DEBUG] is_logged_in_cb(..., " + string(accountname) + ", " + string(protocol)+ ", " + string(recipient) + ") - returning -1");
#endif
        return -1;                  // -1: no idea if he's online...
    }
}


static void inject_message_cb(void *opdata, const char *accountname, const char *protocol, const char *recipient, const char *message)
{
    icqcontact *c;
    string recipientname = recipient;
    recipientname = recipientname.substr(0, recipientname.find_last_of('/'));       // removing "/<instandmessager>"

#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] inject_message_cp(" + string(accountname) + ", " + string(protocol) + ", " + recipientname + ", " + string(message) + ")");
#endif

    if (c = clist.get(imcontact(recipientname, jabber)))
    {
        jhook.send(immessage(c->getdesc(), imevent::outgoing, string(message)));
    } else
    {
        face.log(_("[OTR] Error: inject_message_cb, recipient \"") + recipientname + _("\" not found"));
    }
}


static void notify_cb(void *opdata, OtrlNotifyLevel level,
    const char *accountname, const char *protocol, const char *username,
    const char *title, const char *primary, const char *secondary)
{   // Display a notification message for a particular accountname / protocol / username conversation.
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] notify_cb");
#endif
    string notify_level = "";
    switch (level) 
    {
    case OTRL_NOTIFY_ERROR:     notify_level = _("Error");
                                break;
    case OTRL_NOTIFY_WARNING:   notify_level = _("Warning");
                                break;
    case OTRL_NOTIFY_INFO:      notify_level = _("Info");
                                break;
    }
    face.log("[OTR] " + notify_level);
    if (accountname)    face.log(_("      accountname: ") + string(accountname));
    if (protocol)       face.log(_("      protocol:    ") + string(protocol));
    if (username)       face.log(_("      username:    ") + string(username));
    if (title)          face.log(_("      title:       ") + string(title));
    if (primary)        face.log(_("      primary:     ") + string(primary));
    if (secondary)      face.log(_("      secondary:   ") + string(secondary));
}


static int display_otr_message_cb(void *opdata, const char *accountname,
    const char *protocol, const char *username, const char *msg)
{   /* Display an OTR control message for a particular accountname /
     * protocol / username conversation.  Return 0 if you are able to
     * successfully display it.  If you return non-0 (or if this
     * function is NULL), the control message will be displayed inline,
     * as a received message, or else by using the above notify()
     * callback. */
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] display_otr_message_cb(...) - returning -1");
#endif
    return -1;
}


static void update_context_list_cb(void *opdata)
{   /* When the list of ConnContexts changes (including a change in
     * state), this is called so the UI can be updated. */
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] update_context_list_cb(...)");
#endif
}


static const char *protocol_name_cb(void *opdata, const char *protocol)
{   /* Return a newly-allocated string containing a human-friendly name
     * for the given protocol id */
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] protocol_name_cb(...) - returning NULL");
#endif
    return NULL;
}


static void protocol_name_free_cb(void *opdata, const char *protocol_name)
{   // Deallocate a string allocated by protocol_name
    // Do nothing, since we didn't actually allocate any memory in protocol_name_cb.
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] protocol_name_free_cb");
#endif
}


static void confirm_fingerprint_cb(void *opdata, OtrlUserState us,
    const char *accountname, const char *protocol, const char *username,
    unsigned char fingerprint[20])
{   // A new fingerprint for the given user has been received.
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] confirm_fingerprint_cb");
#endif
    face.log(_("[OTR] Received unknown fingerprint from \"")  + string(username) + "\".");
    face.log(_("      You can verify it in the OTR options."));
}


static void write_fingerprints()
{
    otrl_privkey_write_fingerprints(otr.get_userstate(), (string(conf->getdirname()) + STOREFNAME).c_str());
}

static void write_fingerprints_cb(void *opdata)
{   // The list of known fingerprints has changed.  Write them to disk.
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] write_fingerprints_cb");
#endif
    write_fingerprints();
}


static void gone_secure_cb(void *opdata, ConnContext *context)
{   // A ConnContext has entered a secure state.
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] gone_secure_cb");
#endif
    face.log(_("[OTR] Connection is now secure..."));
}


static void gone_insecure_cb(void *opdata, ConnContext *context)
{   // A ConnContext has left a secure state.
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] gone_insecure_cb");
#endif
    face.log(_("[OTR] Connection is insecure..."));
}


static void still_secure_cb(void *opdata, ConnContext *context, int is_reply)
{   /* We have completed an authentication, using the D-H keys we
     * already knew.  is_reply indicates whether we initiated the AKE. */
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] still_secure_cb");
#endif
}


static void log_message_cb(void *opdata, const char *message)
{   // Log a message.  The passed message will end in "\n".
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] log_message_cb(..., " + string(message) + ")");
#endif
    face.log(_("[OTR] Log: ") + string(message));
}


static OtrlMessageAppOps ui_ops = 
{
    policy_cb,
    create_privkey_cb,
    is_logged_in_cb,
    inject_message_cb,
    notify_cb,
    display_otr_message_cb,
    update_context_list_cb,
    protocol_name_cb,
    protocol_name_free_cb,
    confirm_fingerprint_cb,
    write_fingerprints_cb,
    gone_secure_cb,
    gone_insecure_cb,
    still_secure_cb,
    log_message_cb
};






void imotr::init()
{
    OTRL_INIT;                                  // Initialize the OTR library 

    userstate = otrl_userstate_create();        // Make our OtrlUserState; we'll only use the one.

    
    otrl_privkey_read(userstate, (string(conf->getdirname()) + PRIVKEYFNAME).c_str());
    otrl_privkey_read_fingerprints(userstate, (string(conf->getdirname()) + STOREFNAME).c_str(), NULL, NULL);
    
}

imotr::imotr() 
	: userstate(0)
{
}

imotr::~imotr()
{
	if (0 != userstate)
		otrl_userstate_free(userstate);
}


OtrlUserState imotr::get_userstate()
{
    return userstate;
}



bool imotr::send_message(const protocolname pname, const string &recipient, string &text)
{
    if (pname != jabber)                // at the moment, otr is only with jabber supported...
    {
        return true;
    }

    gcry_error_t err = 0;
    char *newmessage = NULL;
   
    string accountname   = conf->getourid(pname).nickname + "@" + conf->getourid(pname).server;
    string protocolname  = "jabber";

#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] otrl_message_sending(" + accountname + ", " + protocolname + ", " + recipient + ", " + text + ")");
#endif 
    if (otrl_proto_message_type(text.c_str()) == OTRL_MSGTYPE_NOTOTR)
    {
        err = otrl_message_sending(userstate, &ui_ops, NULL, accountname.c_str(), protocolname.c_str(), 
                                   recipient.c_str(), text.c_str(), NULL, &newmessage, NULL, NULL);
    }
    if (err)
    {
        face.log(_("[OTR] Error while encrypting message... no message sent!"));
        face.log(_("      accountname: ") + accountname);
        face.log(_("      recipient:   ") + recipient);
        if (newmessage) otrl_message_free(newmessage);
        return false;
    }
    
    if (newmessage)
    {
        text = newmessage;

#ifdef OTRDEBUG
        face.log("[OTR-DEBUG] otrl_message_sending - newmessage: \"" + text + "\")");
#endif
        otrl_message_free(newmessage);
    }

    return true;
}


bool imotr::receive_message(const protocolname pname, const string &from, string &text)
{
    if (pname != jabber)                // at the moment, otr is only with jabber supported...
    {
        return true;
    }

    int ignore_message;
    char *newmessage = NULL;
    
    string accountname   = conf->getourid(pname).nickname + "@" + conf->getourid(pname).server;
    string protocolname = "jabber";
    string sendername   = from.substr(0, from.find_last_of('/'));   // remove "/<im-name>"

#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] otrl_message_receive(" + accountname + ", " + protocolname + ", " + sendername + ", " + text + ")");
#endif

    ignore_message = otrl_message_receiving(userstate, &ui_ops, NULL, accountname.c_str(), protocolname.c_str(), 
                                            sendername.c_str(), text.c_str(), &newmessage, NULL, NULL, NULL);
    
    if (ignore_message == 1) return false;
    if (newmessage == NULL) return true;

    text = newmessage;
    
#ifdef OTRDEBUG
        face.log("[OTR-DEBUG] otrl_message_sending - newmessage: \"" + text + "\")");
#endif

    otrl_message_free(newmessage);
    return true;
}



void imotr::start_session(icqcontact *contact)
{
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] imotr::start_session(contact)");
#endif
    if (contact->getdesc().pname != jabber)             // at the moment, otr is only with jabber supported...
    {
        face.log(_("[OTR] Error: At the moment, only jabber is supported"));
        return;
    }
    ConnContext *context;
    string accountname   = conf->getourid(contact->getdesc().pname).nickname + "@" + conf->getourid(contact->getdesc().pname).server;
    string protocolname  = "jabber";
    string username      =  contact->getdesc().nickname;
    
    face.log(_("[OTR] Trying to start a secure session with \"") + username +"\"...");
    inject_message_cb(NULL, accountname.c_str(), protocolname.c_str(), username.c_str(), "?OTRv2?");
}

void imotr::end_session(icqcontact *contact)
{
#ifdef OTRDEBUG
    face.log("[OTR-DEBUG] imotr::end_session(contact)");
#endif
    if (contact->getdesc().pname != jabber)             // at the moment, otr is only with jabber supported...
    {
        face.log(_("[OTR] Error: At the moment, only jabber is supported"));
        return;
    }
    ConnContext *context;
    string accountname   = conf->getourid(contact->getdesc().pname).nickname + "@" + conf->getourid(contact->getdesc().pname).server;
    string protocolname  = "jabber";
    string username      =  contact->getdesc().nickname;
     
    face.log(_("[OTR] Ending secure session with \"") + username +"\"...");
    otrl_message_disconnect(userstate, &ui_ops, NULL, accountname.c_str(), protocolname.c_str(), username.c_str());
}


int imotr::yesno(const char *question)
{
    string tmp;
    tmp = face.inputstr(" " + string(question) + _("yes/no") + " ", _("no"));
    while ((face.getlastinputkey() != KEY_ESC) && (tmp != _("yes")) && (tmp != _("no")))
    {
        tmp = face.inputstr(_(" Please type 'yes' or 'no': "), _("no"));
    }
    if (face.getlastinputkey() == KEY_ESC) return -1;
    if (tmp == _("yes")) return 1;
    if (tmp == _("no"))  return 0;
}



void imotr::dialog() 
{
    dialogbox db;
    int n, b, citem, node;
    string tmp;
    bool fin;
    protocolname pname;

    vector< pair< int, void* > > action;

    face.blockmainscreen();

    db.setwindow(new textwindow(0, 0, face.sizeDlg.width, face.sizeDlg.height,
	conf->getcolor(cp_dialog_frame), TW_CENTERED,
	conf->getcolor(cp_dialog_highlight), _(" IM account manager ")));

    db.settree(new treeview(conf->getcolor(cp_dialog_text), conf->getcolor(cp_dialog_selected),
	conf->getcolor(cp_dialog_highlight), conf->getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf->getcolor(cp_dialog_text),
	conf->getcolor(cp_dialog_selected), _("Change"), _("Done"), 0));

    db.addautokeys();
    db.idle = &face.dialogidle;

    treeview &t = *db.gettree();

    map<protocolname, bool> mod;
    for(protocolname pname = icq; pname != protocolname_size; pname++)
	mod[pname] = false;

    for(fin = false; !fin; ) {
	t.clear();

    citem = 1;
    action.clear();

    node  = t.addnode(0, 0, 0, _(" Private keys "));
    for (OtrlPrivKey *privkey = userstate->privkey_root; privkey != NULL; privkey = privkey->next)
    {
        string accountname = privkey->accountname;
        string protocol    = privkey->protocol;
        char hash[45];
        if (!otrl_privkey_fingerprint(userstate, hash, accountname.c_str(), protocol.c_str()))
        {
            strncpy(hash, _("Error calculating Fingerprint"), 45);
        }
        n = t.addnode(node, 0, 0,   (_(" Account: ") + accountname + " ").c_str());
        t.addleaff(n, 0, 0, (_(" Protocol:    ") + protocol    + " ").c_str());
        t.addleaff(n, 0, 0, (_(" Fingerprint: ") + string(hash) + " ").c_str());
        t.addleaff(n, 0, citem++, _(" Forget key "));
        action.push_back(pair<int, void*>(0x100, privkey));
//        t.addleaff(n, 0, citem++, " Generate new key ");
//        action.push_back(pair<int, void*>(0x101, privkey));
    }   // for (all privkeys)   

    node  = t.addnode(0, 0, 0, _(" Public keys "));
    for (ConnContext *context = userstate->context_root; context != NULL; context = context->next)
    {
        char hash[45];
        string username    = context->username;
        string accountname = context->accountname;
        string protocol    = context->protocol;
        n = t.addnode(node, 0, 0, (_(" User: ") + username + " ").c_str());
        t.addleaff(n, 0, 0,       (_(" Protocol: ")   + protocol    + " ").c_str());
        t.addleaff(n, 0, 0,       (_(" Account:  ")   + accountname + " ").c_str());

        for (Fingerprint *fingerprint = context->fingerprint_root.next; fingerprint != NULL; fingerprint = fingerprint->next)
        {
            int newnode;
            string verified = (fingerprint->trust && (string(fingerprint->trust) == "yes")) ? _("Yes") : _("No");
           
            otrl_privkey_hash_to_human(hash, fingerprint->fingerprint);
            newnode = t.addnode(n, 0, 0, (_(" Fingerprint: ") + string(hash) + " ").c_str());
            t.addleaff(newnode, 0, citem++, (_(" Verified: ") + verified).c_str());
            action.push_back(pair<int, void*>(0x200, fingerprint));
            t.addleaff(newnode, 0, citem++, _(" Forget key "));
            action.push_back(pair<int, void*>(0x201, fingerprint));
        }
        if (context->active_fingerprint)
        {
            otrl_privkey_hash_to_human(hash, context->active_fingerprint->fingerprint);
            t.addleaff(n, 0, 0, (_(" Active fingerprint: ") + string(hash) + " ").c_str());
        }

    }



	fin = !db.open(n, b, (void **) &citem) || (b == 1);

	if(!fin && citem) 
    {
        int answer;
        citem--;
        switch (action[citem].first)
        {
            case (0x100):   // forget privkey
                            answer = yesno(_("Do you want to forget the selected key?"));
                		    if (answer == 1)
                            {
                                otrl_privkey_forget((OtrlPrivKey*)action[citem].second);
                                write_fingerprints();
                            }
                            break;

            case (0x101):   // generate new privkey
                            break;

            
            case (0x200):   // verify fingerprint
                            answer = yesno(_("Do you want verify the selected key?"));
                            if (answer == -1) break;
                            otrl_context_set_trust((Fingerprint*)action[citem].second, ((answer) ? "yes" : "no"));
                            write_fingerprints();
                            break;
            
            case (0x201):   // forget fingerprint
                            answer = yesno(_("Do you want to forget the selected key?"));
            	    	    if (answer == 1)
                            {
                                otrl_context_forget_fingerprint((Fingerprint*)action[citem].second, 1);
                                write_fingerprints();
                            }
                            break;
            
            
            default:        break;             
        }

    }
    }

    db.close();
    face.unblockmainscreen();

    face.relaxedupdate();
}



string imotr::get_msg_state(const protocolname pname, const string user)
{
    if (pname != jabber)                // at the moment, otr is only with jabber supported...
    {
        return _("No Jabber");
    }
    ConnContext *context;
    string accountname   = conf->getourid(pname).nickname + "@" + conf->getourid(pname).server;
    string protocolname  = "jabber";
     
    context = otrl_context_find(userstate, user.c_str(), accountname.c_str(), protocolname.c_str(), 
                                0, NULL, NULL, NULL);
    if (!context)
    {
        return _("No OTR");
    }
    
    switch (context->msgstate)
    {
        case (OTRL_MSGSTATE_PLAINTEXT): return _("Plaintext");
        case (OTRL_MSGSTATE_ENCRYPTED): return _("Encrypted");
        case (OTRL_MSGSTATE_FINISHED):  return _("Finished");
        default:    return _("Unknown");
    }
}


string imotr::is_verified(const protocolname pname, const string user)
{
    if (pname != jabber)                // at the moment, otr is only with jabber supported...
    {
        return _("No Jabber");
    }
    ConnContext *context;
    string accountname   = conf->getourid(pname).nickname + "@" + conf->getourid(pname).server;
    string protocolname  = "jabber";
    
    context = otrl_context_find(userstate, user.c_str(), accountname.c_str(), protocolname.c_str(), 
                                0, NULL, NULL, NULL);
    if (context == NULL)
    {
        return _("No OTR");
    }
    if (context->active_fingerprint == NULL)
    {
        return _("No Encryption");
    }
    if (string(context->active_fingerprint->trust) == "yes")
    {
        return _("Verified");
    }
    return _("Unverified");
}
