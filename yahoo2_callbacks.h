#ifndef YAHOO2_CALLBACKS_H
#define YAHOO2_CALLBACKS_H
#include "libyahoo2.h"

/*
 * yahoo2_callbacks.h
 *
 * Callback interface for libyahoo2
 */

/*
 * The following functions need to be implemented in the client
 * interface.  They will be called by the library when each
 * event occurs.
 */

struct yahoo_callbacks {

	/*
	 * Name: yahoo_login_done
	 * 	Called when the login process is complete
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	succ - enum yahoo_login_status
	 * 	url  - url to reactivate account if locked
	 */
	void (*yahoo_login_done)(guint32 id, int succ, char *url);


	

	/*
	 * Name: yahoo_got_buddies
	 * 	Called when the contact list is got from the server
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	buds - the buddy list
	 */
	void (*yahoo_got_buddies)(guint32 id, struct yahoo_buddy **buds);


	
	
	/*
	 * Name: yahoo_status_changed
	 * 	Called when remote user's status changes.
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	who  - the handle of the remote user
	 * 	stat - status code (enum yahoo_status)
	 * 	msg  - the message if stat == YAHOO_STATUS_CUSTOM
	 * 	away - whether the contact is away or not (YAHOO_STATUS_CUSTOM)
	 */
	void (*yahoo_status_changed)(guint32 id, char *who, int stat, char *msg, int away);



	
	/*
	 * Name: yahoo_got_im
	 * 	Called when remote user sends you a message.
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	who  - the handle of the remote user
	 * 	msg  - the message - NULL if stat == 2
	 * 	tm   - timestamp of message if offline
	 * 	stat - message status - 0
	 * 				1
	 * 				2 == error sending message
	 * 				5
	 */
	void (*yahoo_got_im)(guint32 id, char *who, char *msg, long tm, int stat);



	
	/*
	 * Name: yahoo_got_conf_invite
	 * 	Called when remote user sends you a conference invitation.
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	who  - the user inviting you
	 * 	room - the room to join
	 * 	msg  - the message
	 *      members - the initial members of the conference (null terminated list)
	 */
	void (*yahoo_got_conf_invite)(guint32 id, char *who, char *room, char *msg, char **members);



	
	/*
	 * Name: yahoo_conf_userdecline
	 * 	Called when someone declines to join the conference.
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	who  - the user who has declined
	 * 	room - the room
	 * 	msg  - the declining message
	 */
	void (*yahoo_conf_userdecline)(guint32 id, char *who, char *room, char *msg);



	
	/*
	 * Name: yahoo_conf_userjoin
	 * 	Called when someone joins the conference.
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	who  - the user who has joined
	 * 	room - the room joined
	 */
	void (*yahoo_conf_userjoin)(guint32 id, char *who, char *room);



	
	/*
	 * Name: yahoo_conf_userleave
	 * 	Called when someone leaves the conference.
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	who  - the user who has left
	 * 	room - the room left
	 */
	void (*yahoo_conf_userleave)(guint32 id, char *who, char *room);



	
	/*
	 * Name: yahoo_conf_message
	 * 	Called when someone messages in the conference.
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	who  - the user who messaged
	 * 	room - the room
	 * 	msg  - the message
	 */
	void (*yahoo_conf_message)(guint32 id, char *who, char *room, char *msg);



	
	/*
	 * Name: yahoo_got_file
	 * 	Called when someone sends you a file
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	who  - the user who sent the file
	 * 	url  - the file url
	 * 	expires  - the expiry date of the file on the server (timestamp)
	 * 	msg  - the message
	 * 	fname- the file name if direct transfer
	 * 	fsize- the file size if direct transfer
	 */
	void (*yahoo_got_file)(guint32 id, char *who, char *url, long expires, char *msg, char *fname, long fesize);



	
	/*
	 * Name: yahoo_contact_added
	 * 	Called when a contact is added to your list
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	myid - the identity he was added to
	 * 	who  - who was added
	 * 	msg  - any message sent
	 */
	void (*yahoo_contact_added)(guint32 id, char *myid, char *who, char *msg);



	
	/*
	 * Name: yahoo_typing_notify
	 * 	Called when remote user starts or stops typing.
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	who  - the handle of the remote user
	 * 	stat - 1 if typing, 0 if stopped typing
	 */
	void (*yahoo_typing_notify)(guint32 id, char *who, int stat);



	
	/*
	 * Name: yahoo_game_notify
	 * 	Called when remote user starts or stops a game.
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	who  - the handle of the remote user
	 * 	stat - 1 if game, 0 if stopped gaming
	 */
	void (*yahoo_game_notify)(guint32 id, char *who, int stat);



	
	/*
	 * Name: yahoo_mail_notify
	 * 	Called when you receive mail, or with number of messages
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	from - who the mail is from - NULL if only mail count
	 * 	subj - the subject of the mail - NULL if only mail count
	 * 	cnt  - mail count - 0 if new mail notification
	 */
	void (*yahoo_mail_notify)(guint32 id, char *from, char *subj, int cnt);



	
	/*
	 * Name: yahoo_system_message
	 * 	System message
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	msg  - the message
	 */
	void (*yahoo_system_message)(guint32 id, char *msg);






	/*
	 * Name: yahoo_error
	 * 	Called on error.
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	err  - the error message
	 * 	fatal- whether this error is fatal to the connection or not
	 */
	void (*yahoo_error)(guint32 id, char *err, int fatal);





	/*
	 * Name: yahoo_add_input
	 * 	Add a listener for the fd
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	fd   - the fd on which to listen
	 */
	void (*yahoo_add_input)(guint32 id, int fd);




	/*
	 * Name: yahoo_remove_input
	 * 	Remove the listener for the fd
	 * Params:
	 * 	id   - the id that identifies the server connection
	 * 	fd   - the fd on which to listen
	 */
	void (*yahoo_remove_input)(guint32 id, int fd);

};

void yahoo_register_callbacks(struct yahoo_callbacks *yc);

#endif
