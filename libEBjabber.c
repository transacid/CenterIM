/*
 * libEBjabber
 *
 * Copyright (C) 2000, Alex Wheeler <awheeler@speakeasy.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *      This code borrowed heavily from the jabber client jewel
 *      (http://jewel.sourceforge.net/)
 */



#include <stdio.h>
#include <jabber.h>
#include "libEBjabber.h"

#include "glib.h"

extern void JABBERStatusChange(void *data);
extern void JABBERAddBuddy(void *data);
extern void JABBERInstantMessage(void *data);
extern void JABBERDialog(void *data);
extern void JABBERListDialog(char **list, void *data);
extern void JABBERError(char* title, char *message);
extern void JABBERLogout(void *data);
void JABBERDelBuddy(void *data);

void j_on_state_handler(jconn conn, int state);
void j_on_packet_handler(jconn conn, jpacket packet);
void j_on_create_account(void *data);
void j_allow_subscribe(void *data);
void j_unsubscribe(void *data);
void j_on_pick_account(void *data);

/******************************************************
 * Support routines for mulitple simultaneous accounts
 *****************************************************/

JABBER_Conn *Connections=NULL;

JABBER_Conn *JCnewConn(void)
{
	JABBER_Conn *jnew=NULL;

	/* Create a new connection struct, and put it on the top of the list */
	jnew=calloc(1, sizeof(JABBER_Conn));
	jnew->next=Connections;
	Connections=jnew;
	return(Connections);
}

JABBER_Conn *JCfindConn(jconn conn)
{
	JABBER_Conn *current=Connections;

	while(current)
	{
		if(conn == current->conn)
			return(current);
		current=current->next;
	}
	return(NULL);
}

char *JCgetServerName(JABBER_Conn * JConn)
{
	if(!JConn || !JConn->conn || !JConn->conn->user)
		return(NULL);
	return(JConn->conn->user->server);
}

JABBER_Conn *JCfindServer(char *server)
{
	JABBER_Conn *current=Connections;

	while(current) {
		if(current->conn) {
			dprintf("Server: %s\n", current->conn->user->server);
			if(!strcmp(server, current->conn->user->server)) {
				return(current);
			}
		}
		current=current->next;
	}
	return(NULL);
}

JABBER_Conn *JCfindJID(char *JID)
{
	JABBER_Conn *current=Connections;

	while(current) {
		dprintf("JID: %s\n", current->jid);
		if(!strcmp(JID, current->jid)) {
			return(current);
		}
		current=current->next;
	}
	return(NULL);
}

int JCremoveConn(JABBER_Conn *JConn)
{
	JABBER_Conn *current=Connections, *last=current;

	while(current)
	{
		if(JConn==current)
		{
			last->next=current->next;
			free(current);
			return(0);
		}
		last=current;
		current=current->next;
	}
	return(-1);
}

char **JCgetJIDList(void)
{
	JABBER_Conn *current=Connections;
	char **list=NULL;
	int count=0;

	while(current)
	{
		/* Leave room for the last null entry */
		list=realloc(list, sizeof(char *)*(count+2));
		list[count]=strdup(current->jid);
		current=current->next;
		count++;
	}
	if(list)
		list[count]=NULL;
	return(list);
}

void JCfreeServerList(char **list)
{
	int count=0;
	while(list[count])
	{
		free(list[count++]);
	}
	free(list);
}

/****************************************
 * End jabber multiple connection routines
 ****************************************/

void jabber_callback_handler(gpointer data, gint source,
			  GdkInputCondition condition )
{
    JABBER_Conn *JConn=data;
    /* Let libjabber do the work, it calls what we setup in jab_packet_handler */
    jab_poll(JConn->conn, 0);
    /* Is this connection still good? */
    if(!JConn->conn) {
	JABBERLogout(NULL);
	gdk_input_remove(JConn->listenerID);
    }
    else if(JConn->conn->state==JCONN_STATE_OFF || JConn->conn->fd==-1) {
	/* No, clean up */
	JABBERLogout(NULL);
	gdk_input_remove(JConn->listenerID);
	jab_delete(JConn->conn);
	JConn->conn=NULL;
    }
}

/* Functions called from the everybuddy jabber.c file */

JABBER_Conn *JABBER_Login(char *handle, char *passwd, char *host, int port) {
	/* At this point, we don't care about host and port */
	int  lID;
	char jid[256+1];
	char buff[4096];
	JABBER_Conn *JConn=NULL;

	dprintf( "%s %s %i\n", handle, host, port);
	/* Make sure the name is correct, support all formats
	 * handle       become handle@host/everybuddy
	 * handle@foo   becomes handle@foo/everybuddy
	 * handle@foo/bar is not changed
	 */

	if(!strchr(handle, '@')) {
		/* A host must be specified in the config file */
		if(!host) {
			JABBERError("No jabber server specified!", "Cannot login");
			return(NULL);
		}
		snprintf(jid, 256, "%s@%s/everybuddy", handle, host);
	}
	else if(!strchr(handle, '/'))
		snprintf(jid, 256, "%s/everybuddy", handle);

	else
		strncpy(jid, handle, 256);
	dprintf( "jid: %s\n", jid);
	JConn=JCnewConn();
	strncpy(JConn->jid,jid, LINE_LENGTH);
	JConn->conn=jab_new(jid, passwd);
	if(!JConn->conn) {
		snprintf(buff, 4096, "Connection to the jabber server: %s failed!", host);
		JABBERError(buff, "Jabber server not responding");
		free(JConn);
		return(NULL);
	}
	jab_packet_handler(JConn->conn, j_on_packet_handler);
	jab_state_handler(JConn->conn, j_on_state_handler);
	jab_start(JConn->conn);
	if(!JConn->conn || JConn->conn->state==JCONN_STATE_OFF) {
		snprintf(buff, 4096, "Connection to the jabber server: %s failed!", host);
		JABBERError(buff, "Jabber server not responding");
		jab_delete(JConn->conn);
		JConn->conn=NULL;
		return(NULL);
	}
	jab_auth(JConn->conn);
	lID = gdk_input_add(JConn->conn->fd, GDK_INPUT_READ,
			jabber_callback_handler, JConn);
	JConn->listenerID = lID;
	dprintf( "*** ListenerID: %i FD: %i\n", lID, JConn->conn->fd);
	/* FIXME: Is this supposed to be 1? */
	JConn->id = 1;
	JConn->reg_flag = 0;
	return(JConn);
}

int JABBER_AuthorizeContact(JABBER_Conn *conn, char *handle) {
	dprintf( "%s\n", handle);
	return(0);
}

int JABBER_SendMessage(JABBER_Conn *JConn, char *handle, char *message) {
	xmlnode x;

	if(!JConn) {
		dprintf("******Called with NULL JConn for user %s!!!\n", handle);
		return(0);
	}
	dprintf( "handle: %s message: %s\n", handle, message);
	dprintf( "********* %s -> %s\n", JConn->jid, handle);
	/* We always want to chat.  :) */
	x = jutil_msgnew(TMSG_CHAT, handle, NULL, message);
	jab_send(JConn->conn, x);
	return(0);
}

int JABBER_AddContact(JABBER_Conn *JConn, char *handle) {
	xmlnode x,y,z;
	char *jid=strdup(handle);
	JABBER_Dialog *JD;
	char *buddy_server=NULL;
	char **server_list;

	if(!JConn) {
		buddy_server=strchr(handle, '@');
		if(!buddy_server) {
			/* This happens for tranposrt buddies */
			buddy_server=strchr(handle, '.');
			if(!buddy_server) {
				dprintf("Something weird, buddy without an '@' or a '.'");
				return(1);
			}
		}
		buddy_server++;
		JConn=JCfindServer(buddy_server);
		if(JConn) {
			dprintf("Found matching server for %s\n", handle);
		}
		else {
			server_list=JCgetJIDList();
			if(!server_list)
				return(1);
			JD=calloc(sizeof(JABBER_Dialog), 1);
			JD->heading="Pick an account";
			JD->message=strdup("Unable to automatically determine which account to use for thise buddy:\nplease select the account that can talk to this buddy's server");
			JD->callback=j_on_pick_account;
			JD->requestor=strdup(handle);
			JABBERListDialog(server_list, JD);
			return(0);
		}
	}
	/* For now, everybuddy does not understand resources */
	jid=strtok(jid, "/");
	if(!jid)
		jid=strdup(handle);
	dprintf( "%s now %s\n", handle, jid);

	/* Ask to be subscribed */
	x=jutil_presnew(JPACKET__SUBSCRIBE, jid, NULL);
	jab_send(JConn->conn, x);

	/* Request roster update */
	x = jutil_iqnew (JPACKET__SET, NS_ROSTER);
	y = xmlnode_get_tag (x, "query");
	z = xmlnode_insert_tag (y, "item");
	xmlnode_put_attrib (z, "jid", jid);
	jab_send (JConn->conn, x);
	return(0);
}

int JABBER_RemoveContact(JABBER_Conn *JConn, char *handle) {
	xmlnode x,y,z;
	char *jid=strdup(handle);

	if(!JConn) {
		fprintf(stderr, "**********NULL JConn sent to JABBER_RemoveContact!\n");
		return(-1);
	}
	x = jutil_presnew (JPACKET__UNSUBSCRIBE, handle, NULL);

	jab_send (JConn->conn, x);

	x = jutil_iqnew (JPACKET__SET, NS_ROSTER);
	y = xmlnode_get_tag (x, "query");
	z = xmlnode_insert_tag (y, "item");
	xmlnode_put_attrib (z, "jid", jid);
	xmlnode_put_attrib (z, "subscription", "remove");
	jab_send (JConn->conn, x);

	return(0);
}

int JABBER_EndChat(JABBER_Conn *JConn, char *handle) {
	dprintf( "Empty function\n");
	return(0);
}


int JABBER_Logout(JABBER_Conn *JConn) {
	dprintf( "Entering\n");
	if(!JConn)
		return(-1);
	if(JConn->conn) {
		dprintf( "JConn->conn exists, closing everything up\n");
		gdk_input_remove(JConn->listenerID);
		jab_stop(JConn->conn);
		jab_delete(JConn->conn);
	}
	JConn->conn=NULL;
	JCremoveConn(JConn);
	dprintf( "Leaving\n");
	return(0);
}

int JABBER_ChangeState(JABBER_Conn *JConn, int state) {
	xmlnode x,y;
	/* Unique away states are possible, but not supported by
	everybuddy yet.  status would hold that value */
	char show[7]="", *status="";

	dprintf("(%i)\n", state);
	if(!JConn->conn)
		return(-1);
	x = jutil_presnew (JPACKET__UNKNOWN, NULL, NULL);

	if (state != JABBER_ONLINE) {
		y = xmlnode_insert_tag (x, "show");
		switch (state) {
		case JABBER_AWAY:
			strcpy (show, "away");
			break;
		case JABBER_DND:
			strcpy (show, "dnd");
			break;
		case JABBER_XA:
			strcpy (show, "xa");
			break;
		case JABBER_CHAT:
			strcpy (show, "chat");
			break;
		default:
			strcpy (show, "unknown");
			dprintf( "Unknown state: %i suggested\n", state);
		}
		xmlnode_insert_cdata (y, show, -1);
	}
	if (strlen (status) > 0) {
		y = xmlnode_insert_tag (x, "status");
		xmlnode_insert_cdata (y, status, -1);
	}
	dprintf( "Setting status to: %s - %s\n", show, status);
	jab_send (JConn->conn, x);
	return(0);
}

/* ----------------------------------------------*/


/* Functions to handle jabber responses */

void j_on_packet_handler(jconn conn, jpacket packet) {
	xmlnode x, y, z;
	char *id, *ns, *jid, *sub, *name, *from, *group;
	char *desc, *service, *to, *url;
	char *show, *type, *body, *subj, *ask, *code;
	char buff[8192+1];      /* FIXME: FInd right size */
	int iid, status;
	JABBER_Conn *JConn=NULL;
	JABBER_InstantMessage JIM;
	JABBER_Dialog *JD;
	struct jabber_buddy JB;

	JConn=JCfindConn(conn);
	jpacket_reset(packet);
	dprintf( "Packet: %s\n", packet->x->name);
	switch(packet->type) {
	case JPACKET_MESSAGE:
		dprintf( "MESSAGE recieved\n");
		from = xmlnode_get_attrib (packet->x, "from");
		type = xmlnode_get_attrib (packet->x, "type");
		/* type can be empty, chat, groupchat, or error */
		x = xmlnode_get_tag (packet->x, "body");
		body = xmlnode_get_data (x);
		x = xmlnode_get_tag (packet->x, "subject");
		if (x) {
			subj = xmlnode_get_data (x);
			snprintf(buff, 8192, "%s: %s", subj, body);
		}
		else {
			subj = NULL;
			strncpy(buff, body, 8192);
		}
		fprintf (stderr, "from %s, type %s, body: %s, subj %s\n", from, type, body, subj);
		JIM.msg=buff;
		dprintf( "JIM.msg: %s\n", JIM.msg);
		/* For now, everybuddy does not understand resources */
		JIM.sender=strtok(from, "/");
		if(!JIM.sender)
			JIM.sender=from;
		dprintf( "JIM.sender: %s\n", JIM.sender);
		dprintf( "Rendering message\n");
		JABBERInstantMessage(&JIM);
		break;
	case JPACKET_IQ:
		type = xmlnode_get_attrib(packet->x, "type");
		if(!type) {
			dprintf( "JPACKET_IQ: NULL type\n");
			return;
		}
		dprintf( "IQ recieved: %s\n", type);
		if(!strcmp(type, "result")) {
			id = xmlnode_get_attrib (packet->x, "id");
			if (id) {
				iid = atoi (id);
				dprintf( "iid: %i\n", iid);
				if (iid == JConn->id) {
					if (!JConn->reg_flag) {
						fprintf (stderr, "requesting roster\n");
						x = jutil_iqnew (JPACKET__GET, NS_ROSTER);
						jab_send (conn, x);
						fprintf (stderr, "requesting agent list\n");
						x = jutil_iqnew (JPACKET__GET, NS_AGENTS);
						jab_send (conn, x);
						return;
					}
					else {
						JConn->reg_flag = 0;
						JConn->id = atoi (jab_auth (conn));
						return;
					}
				}
			}
			else {
				dprintf( "No ID!\n");
			}
			x = xmlnode_get_tag (packet->x, "query");
			if (!x) {
				dprintf( "No X\n");
				break;
			}
			ns = xmlnode_get_attrib (x, "xmlns");
			dprintf( "ns: %s\n", ns);
			if (strcmp (ns, NS_ROSTER) == 0) {
				y = xmlnode_get_tag (x, "item");
				for (;;) {
					jid = xmlnode_get_attrib (y, "jid");
					sub = xmlnode_get_attrib (y, "subscription");
					name = xmlnode_get_attrib (y, "name");
					group = xmlnode_get_attrib (y, "group");
					dprintf( "Buddy: %s(%s) %s %s\n", jid, group, sub, name);
					if(jid && sub && name) {
						JB.jid=strtok(jid, "/");
						if(!JB.jid) {
							JB.jid=strdup(jid);
						}
						else {
							JB.jid=strdup(JB.jid);
						}
						JB.name=strdup(name);
						JB.sub=strdup(sub);
						/* State does not matter */
						JB.status=JABBER_OFFLINE;
						JB.JConn=JConn;
						JABBERAddBuddy(&JB);
					}
					y = xmlnode_get_nextsibling (y);
					if (!y) break;
				}
			}
			else if(strcmp (ns, NS_AGENTS)==0) {
				y = xmlnode_get_tag (x, "agent");
				for (;;) {
					jid = xmlnode_get_attrib (y, "jid");
					if(jid) {
						name = xmlnode_get_tag_data (y, "name");
						desc = xmlnode_get_tag_data (y, "description");
						service = xmlnode_get_tag_data (y, "service");
						dprintf( "Agent[%s]: %s [%s] [%s]\n",
							name, jid, desc, service);
						z = jutil_iqnew (JPACKET__GET, NS_AGENT);
						xmlnode_put_attrib(z, "to", jid);
//                                              xmlnode_put_attrib(z, "id", "1");
						to=xmlnode_get_attrib(z, "to");
						dprintf("to: %s - %s - %s\n", to,
							xmlnode_get_attrib(z, "type"),
							xmlnode_get_attrib(z, "id"));
						jab_send (conn, z);
					}
					y = xmlnode_get_nextsibling (y);
					if (!y) break;
				}
			}
			else if(strcmp (ns, NS_AGENT)==0) {
				name=xmlnode_get_tag_data(x, "name");
				url =xmlnode_get_tag_data(x, "url");
				dprintf("agent: %s - %s\n", name, url);
			}
			jab_send_raw (conn, "<presence/>");
			return;
			
		}
		else
		if(!strcmp(type, "set")) {
			x = xmlnode_get_tag (packet->x, "query");
			ns = xmlnode_get_attrib (x, "xmlns");
			if (strcmp (ns, NS_ROSTER) == 0) {
				y = xmlnode_get_tag (x, "item");
				jid = xmlnode_get_attrib (y, "jid");
				sub = xmlnode_get_attrib (y, "subscription");
				name = xmlnode_get_attrib (y, "name");
				ask = xmlnode_get_attrib (y, "ask");
				dprintf( "Need to find a buddy: %s %s %s %s\n", jid, sub, name, ask);
/*                              bud = buddy_find (jid);
*/
				if (sub) {
					if (strcmp (sub, "remove") == 0) {
						dprintf( "Need to remove a buddy: %s\n", jid);
/*                                              buddy_remove_from_clist (bud);
						buddy_remove (bud);
*/
						break;
					}
				}
/*
				if (!bud) {
					buddy_add_to_clist (buddy_add (jid, sub, name));
					if (!name && !ask) roster_new ("Edit User", jid, NULL);
				}
				else if (name) {
					free (bud->name);
					bud->name = strdup (name);
					set_name (bud);
				}
*/
			}

		}
		else
		if(!strcmp(type, "error")) {
			x = xmlnode_get_tag (packet->x, "error");
			code = xmlnode_get_attrib (x, "code");
			desc = xmlnode_get_tag_data(packet->x, "error");
			dprintf("Recieved error: %s\n", desc);
			switch(atoi(code)) {
			case 302: /* Redirect */
				break;
			case 400: /* Bad request */
				break;
			case 401: /* Unauthorized */
				JD=calloc(sizeof(JABBER_Dialog), 1);
				JD->heading="Register? You're not authorized";
				JD->message=strdup("Do you want to try to create an account on the jabber server?");
				JD->callback=j_on_create_account;
				JD->JConn=JConn;
				JABBERDialog(JD);
				break;
			case 402: /* Payment Required */
				break;
			case 403: /* Forbidden */
				break;
			case 404: /* Not Found */
				break;
			case 405: /* Not Allowed */
				break;
			case 406: /* Not Acceptable */
				break;
			case 407: /* Registration Required */
				break;
			case 408: /* Request Timeout */
				break;
			case 409: /* Conflict */
				break;
			case 500: /* Internal Server Error */
				break;
			case 501: /* Not Implemented */
				break;
			case 502: /* Remote Server Error */
				break;
			case 503: /* Service Unavailable */
				break;
			case 504: /* Remote Server Timeout */
				break;
			default:
				fprintf(stderr, "Error: %s\n", code);
			}
		}
		break;
	case JPACKET_PRESENCE:
		status = JABBER_ONLINE;
		type = xmlnode_get_attrib (packet->x, "type");
		from = xmlnode_get_attrib (packet->x, "from");
		/* For now, everybuddy does not understand resources */
		JB.jid=strtok(from, "/");
		if(!JB.jid)
			JB.jid=from;
		dprintf( "PRESENCE recieved type: %s from: %s\n", type, from);
		x = xmlnode_get_tag (packet->x, "show");
		if (x) {
			show = xmlnode_get_data (x);
			printf ("show: %s\n", show);
			if (strcmp (show, "away") == 0) status = JABBER_AWAY;
			else if (strcmp (show, "dnd") == 0) status = JABBER_DND;
			else if (strcmp (show, "xa") == 0) status = JABBER_XA;
			else if (strcmp (show, "chat") == 0) status = JABBER_CHAT;
		}
		if (type) {
			if (strcmp (type, "unavailable") == 0) status = JABBER_OFFLINE;
		}
		JB.status=status;
		JB.JConn=JConn;
		JABBERStatusChange(&JB);
		break;
	case JPACKET_S10N:
		dprintf( "S10N recieved\n");
		from = xmlnode_get_attrib (packet->x, "from");
		type = xmlnode_get_attrib (packet->x, "type");
		if (type) {
			JD=calloc(sizeof(JABBER_Dialog), 1);
			if (strcmp (type, "subscribe") == 0) {
				sprintf(buff, "%s wants to subscribe you", from);
				JD->message=strdup(buff);
				JD->heading="Subscribe Request";
				JD->callback=j_allow_subscribe;
				JD->requestor=strdup(from);
				JD->JConn=JConn;
				JABBERDialog(JD);
				return;
			}
			else
			if (strcmp (type, "unsubscribe") == 0) {
				x = jutil_presnew (JPACKET__UNSUBSCRIBED, from, NULL);
				jab_send (conn, x);
				sprintf(buff, "%s unsubscribed you, do you want to unsubscribe them?", from);
				JD->message=strdup(buff);
				JD->heading="Remove User";
				JD->callback=j_unsubscribe;
				JD->requestor=strdup(from);
				JD->JConn=JConn;
				JABBERDialog(JD);
				return;
			}
		}
		break;
	default:
		fprintf(stderr, "unrecognized packet: %i recieved\n", packet->type);
		break;
	}
}

void j_on_state_handler(jconn conn, int state) {
	static int previous_state=JCONN_STATE_OFF;
	JABBER_Conn *JConn=NULL;
	char buff[4096];

	dprintf("Entering: new state: %i previous_state: %i\n", state, previous_state);
	JConn=JCfindConn(conn);
	switch(state) {
	case JCONN_STATE_OFF:
		if(previous_state!=JCONN_STATE_OFF) {
			dprintf("The Jabber server has disconnected you: %i\n", previous_state);
			snprintf(buff, 4096, "The Jabber server: %s has disconnected you!",
				JCgetServerName(JConn));
			JABBERError(buff, "Disconnect");
			gdk_input_remove(JConn->listenerID);
			/* FIXME: Do we need to free anything? */
			JConn->conn=NULL;
			JABBERLogout(NULL);
		}
		break;
	case JCONN_STATE_CONNECTED:
		dprintf( "JCONN_STATE_CONNECTED\n");
		break;
	case JCONN_STATE_AUTH:
		dprintf( "JCONN_STATE_AUTH\n");
		break;
	case JCONN_STATE_ON:
		dprintf( "JCONN_STATE_ON\n");
		break;
	default:
		dprintf( "UNKNOWN state: %i\n", state);
		break;
	}
	previous_state=state;
}

void j_on_create_account(void *data) {
	JABBER_Dialog_PTR jd;

	dprintf( "Entering, but doing little\n");
	jd=(JABBER_Dialog_PTR)data;
	jd->JConn->reg_flag = 1;
	jd->JConn->id = atoi (jab_reg (jd->JConn->conn));
}

void j_allow_subscribe(void *data) {
	JABBER_Dialog_PTR jd;
	xmlnode x, y, z;

	jd=(JABBER_Dialog_PTR)data;
	dprintf( "%s: Entering\n", jd->requestor);
	x=jutil_presnew(JPACKET__SUBSCRIBED, jd->requestor, NULL);
	jab_send(jd->JConn->conn, x);

	/* We want to request their subscription now */
	x=jutil_presnew(JPACKET__SUBSCRIBE, jd->requestor, NULL);
	jab_send(jd->JConn->conn, x);

	/* Request roster update */
	x = jutil_iqnew (JPACKET__SET, NS_ROSTER);
	y = xmlnode_get_tag (x, "query");
	z = xmlnode_insert_tag (y, "item");
	xmlnode_put_attrib (z, "jid", jd->requestor);
	xmlnode_put_attrib (z, "name", jd->requestor);
	jab_send (jd->JConn->conn, x);
}

void j_unsubscribe(void *data) {
	JABBER_Dialog_PTR jd;

	jd=(JABBER_Dialog_PTR)data;
	JABBERDelBuddy(jd->requestor);
}

void j_on_pick_account(void *data) {
	JABBER_Dialog_PTR jd;
	JABBER_Conn *JConn=NULL;

	jd=(JABBER_Dialog_PTR)data;
	JConn=JCfindJID(jd->response);
	dprintf("Found JConn for %s: %p\n", jd->requestor, JConn);
	/* Call it again, this time with a good JConn */
	if(!JConn) {
		fprintf(stderr, "NULL Jabber connector for buddy, should not happen!\n");
		return;
	}
	JABBER_AddContact(JConn, jd->requestor);
}
