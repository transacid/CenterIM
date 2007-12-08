#ifndef __IMOTR_H__
#define __IMOTR_H__

#include "icqcommon.h"


#include "imcontact.h"

/* libotr headers */
#ifdef __cplusplus
extern "C" {
#endif
#include <libotr/context.h>
#include <libotr/userstate.h>
#ifdef __cplusplus
}
#endif

#define PRIVKEYFNAME "otr.private_key"
#define STOREFNAME   "otr.fingerprints"



extern OtrlUserState otrg_plugin_userstate;


class imotr {
   private:

    OtrlUserState userstate;
    
    void start_session(OtrlAuthInfo *auth);
   

   public:
    OtrlUserState get_userstate(); 
    bool send_message(const protocolname pname, const string &touser, string &text);      // return false: do not send message
    bool receive_message(const protocolname pname, const string &from, string &text);     // return false: do not show message

    string get_msg_state(const protocolname pname, const string user);
    string is_verified(const protocolname pname, const string user);
    int  yesno(const char *question);
    void dialog();

    void start_session(icqcontact *contact);
    void end_session(icqcontact *contact);


	imotr();
	~imotr();

	void init();
};

extern imotr otr;


#endif
