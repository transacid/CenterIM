/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Jabber
 *  Copyright (C) 1998-1999 The Jabber Team http://jabber.org/
 */

#include "jabber.h"
#include "connwrap.h"

#ifdef HAVE_THREAD
#include <pthread.h>
#endif
/* local macros for launching event handlers */
#define STATE_EVT(arg) if(j->on_state) { (j->on_state)(j, (arg) ); }


/* prototypes of the local functions */
static void startElement(void *userdata, const char *name, const char **attribs);
static void endElement(void *userdata, const char *name);
static void charData(void *userdata, const char *s, int slen);


/*
 *  jab_new -- initialize a new jabber connection
 *
 *  parameters
 *      user -- jabber id of the user
 *      pass -- password of the user
 *
 *  results
 *      a pointer to the connection structure
 *      or NULL if allocations failed
 */
jconn jab_new(char *user, char *pass, char *server, int port, int ssl)
{
    pool p;
    jconn j;

    if(!user) return(NULL);

    p = pool_new();
    if(!p) return(NULL);
    j = pmalloc_x(p, sizeof(jconn_struct), 0);
    if(!j) return(NULL);
    j->p = p;

    j->user = jid_new(p, user);
    j->pass = pstrdup(p, pass);
    j->port = port;
    j->server = server;

    j->state = JCONN_STATE_OFF;
    j->cw_state = 0;
    j->id = 1;
    j->fd = -1;
    j->ssl = ssl;

    return j;
}

/*
 *  jab_delete -- free a jabber connection
 *
 *  parameters
 *      j -- connection
 *
 */
void jab_delete(jconn j)
{
    if(!j) return;

    jab_stop(j);
    pool_free(j->p);
}

/*
 *  jab_state_handler -- set callback handler for state change
 *
 *  parameters
 *      j -- connection
 *      h -- name of the handler function
 */
void jab_state_handler(jconn j, jconn_state_h h)
{
    if(!j) return;

    j->on_state = h;
}

/*
 *  jab_packet_handler -- set callback handler for incoming packets
 *
 *  parameters
 *      j -- connection
 *      h -- name of the handler function
 */
void jab_packet_handler(jconn j, jconn_packet_h h)
{
    if(!j) return;

    j->on_packet = h;
}

void jab_logger(jconn j, jconn_logger h)
{
    if(!j) return;

    j->logger = h;
}


/*
 *  jab_start -- start connection
 *
 *  parameters
 *      j -- connection
 *
 */
void jab_start(jconn j)
{
    xmlnode x;
    char *t,*t2;

    if(!j || (j->state != JCONN_STATE_OFF && j->state != JCONN_STATE_CONNECTING) ) return;
    
    if (!(j->cw_state & CW_CONNECT_WANT_SOMETHING)) { /* same as state != JCONN_STATE_CONNECTING */
	j->parser = XML_ParserCreate(NULL);
	XML_SetUserData(j->parser, (void *)j);
	XML_SetElementHandler(j->parser, startElement, endElement);
	XML_SetCharacterDataHandler(j->parser, charData);

	if (j->cw_state & CW_CONNECT_BLOCKING)
	    j->fd = make_netsocket(j->port, j->server, NETSOCKET_CLIENT, j->ssl);
	else
	    j->fd = make_nb_netsocket(j->port, j->server, NETSOCKET_CLIENT, j->ssl, &j->cw_state);
	
	if(j->fd < 0) {
	    STATE_EVT(JCONN_STATE_OFF);
	    return;
	}
    }
    else { /* subsequent calls to cw_nb_connect until it finishes negociation */
	if (cw_nb_connect(j->fd, 0, 0, j->ssl, &j->cw_state)) {
	    STATE_EVT(JCONN_STATE_OFF);
	    return;
	}
    }
    if (j->cw_state & CW_CONNECT_WANT_SOMETHING){ /* check if it finished negociation */
	j->state = JCONN_STATE_CONNECTING;
	STATE_EVT(JCONN_STATE_CONNECTING);
	return;
    }
    change_socket_to_blocking(j->fd);
    
    j->state = JCONN_STATE_CONNECTED;
    STATE_EVT(JCONN_STATE_CONNECTED)

    /* start stream */
    x = jutil_header(NS_CLIENT, j->user->server);
    t = xmlnode2str(x);
    /* this is ugly, we can create the string here instead of jutil_header */
    /* what do you think about it? -madcat */
    t2 = strstr(t,"/>");
    if (t2 != NULL)
      {
    *t2++ = '>';
    *t2 = '\0';
      }
    jab_send_raw(j,"<?xml version='1.0'?>");
    jab_send_raw(j,t);
    xmlnode_free(x);

    j->state = JCONN_STATE_ON;
    STATE_EVT(JCONN_STATE_ON)

}

/*
 *  jab_stop -- stop connection
 *
 *  parameters
 *      j -- connection
 */
void jab_stop(jconn j)
{
    if(!j || j->state == JCONN_STATE_OFF) return;

    j->state = JCONN_STATE_OFF;
    cw_close(j->fd);
    j->fd = -1;
    XML_ParserFree(j->parser);
}

/*
 *  jab_getfd -- get file descriptor of connection socket
 *
 *  parameters
 *      j -- connection
 *
 *  returns
 *      fd of the socket or -1 if socket was not connected
 */
int jab_getfd(jconn j)
{
    if(j)
	return j->fd;
    else
	return -1;
}

/*
 *  jab_getjid -- get jid structure of user
 *
 *  parameters
 *      j -- connection
 */
jid jab_getjid(jconn j)
{
    if(j)
	return(j->user);
    else
	return NULL;
}

/*  jab_getsid -- get stream id
 *  This is the id of server's <stream:stream> tag and used for
 *  digest authorization.
 *
 *  parameters
 *      j -- connection
 */
char *jab_getsid(jconn j)
{
    if(j)
	return(j->sid);
    else
	return NULL;
}

/*
 *  jab_getid -- get a unique id
 *
 *  parameters
 *      j -- connection
 */
char *jab_getid(jconn j)
{
    snprintf(j->idbuf, 8, "%d", j->id++);
    return &j->idbuf[0];
}

/*
 *  jab_send -- send xml data
 *
 *  parameters
 *      j -- connection
 *      x -- xmlnode structure
 */
void jab_send(jconn j, xmlnode x)
{
    if (j && j->state != JCONN_STATE_OFF)
    {
	    char *buf = xmlnode2str(x);
	    if (buf) {
		cw_write(j->fd, buf, strlen(buf), j->ssl);
		if (j->logger)
		    (j->logger)(j, 0, buf);
	    }

#ifdef JDEBUG
	    printf ("out: %s\n", buf);
#endif
    }
}

/*
 *  jab_send_raw -- send a string
 *
 *  parameters
 *      j -- connection
 *      str -- xml string
 */
void jab_send_raw(jconn j, const char *str)
{
    if (j && j->state != JCONN_STATE_OFF) {
	cw_write(j->fd, str, strlen(str), j->ssl);

	if (j->logger)
	    (j->logger)(j, 0, str);
    }

#ifdef JDEBUG
    printf ("out: %s\n", str);
#endif
}

/*
 *  jab_recv -- read and parse incoming data
 *
 *  parameters
 *      j -- connection
 */
void jab_recv(jconn j)
{
    static char buf[32768];
    int len;

    if(!j || j->state == JCONN_STATE_OFF)
	return;

    len = cw_read(j->fd, buf, sizeof(buf)-1, j->ssl);
    if(len>0)
    {
	buf[len] = '\0';

	if (j->logger)
	    (j->logger)(j, 1, buf);

#ifdef JDEBUG
	printf (" in: %s\n", buf);
#endif
	XML_Parse(j->parser, buf, len, 0);
    }
    else if(len<=0)
    {
	STATE_EVT(JCONN_STATE_OFF);
	jab_stop(j);
    }
}

/*
 *  jab_poll -- check socket for incoming data
 *
 *  parameters
 *      j -- connection
 *      timeout -- poll timeout
 */
void jab_poll(jconn j, int timeout)
{
    fd_set fds;
    struct timeval tv;
    int r;

    if (!j || j->state == JCONN_STATE_OFF || j->fd == -1)
	return;

    FD_ZERO(&fds);
    FD_SET(j->fd, &fds);

    if(timeout <= 0) {
	r = select(j->fd + 1, &fds, NULL, NULL, NULL);

    } else {
	tv.tv_sec = 0;
	tv.tv_usec = timeout;
	r = select(j->fd + 1, &fds, NULL, NULL, &tv);

    }

    if(r > 0) {
	jab_recv(j);

    } else if(r) {
	STATE_EVT(JCONN_STATE_OFF);
	jab_stop(j);

    }
}

/*
 *  jab_auth -- authorize user
 *
 *  parameters
 *      j -- connection
 *
 *  returns
 *      id of the iq packet
 */
char *jab_auth(jconn j)
{
    xmlnode x,y,z;
    char *hash, *user, *id;

    if(!j) return(NULL);

    x = jutil_iqnew(JPACKET__SET, NS_AUTH);
    id = jab_getid(j);
    xmlnode_put_attrib(x, "id", id);
    y = xmlnode_get_tag(x,"query");

    user = j->user->user;

    if (user)
    {
	z = xmlnode_insert_tag(y, "username");
	xmlnode_insert_cdata(z, user, -1);
    }

    z = xmlnode_insert_tag(y, "resource");
    xmlnode_insert_cdata(z, j->user->resource, -1);

    if (j->sid)
    {
	z = xmlnode_insert_tag(y, "digest");
	hash = pmalloc(x->p, strlen(j->sid)+strlen(j->pass)+1);
	strcpy(hash, j->sid);
	strcat(hash, j->pass);
	hash = shahash(hash);
	xmlnode_insert_cdata(z, hash, 40);
    }
    else
    {
	z = xmlnode_insert_tag(y, "password");
	xmlnode_insert_cdata(z, j->pass, -1);
    }

    jab_send(j, x);
    xmlnode_free(x);
    return id;
}

/*
 *  jab_reg -- register user
 *
 *  parameters
 *      j -- connection
 *
 *  returns
 *      id of the iq packet
 */
char *jab_reg(jconn j)
{
    xmlnode x,y,z;
    char *hash, *user, *id;

    if (!j) return(NULL);

    x = jutil_iqnew(JPACKET__SET, NS_REGISTER);
    id = jab_getid(j);
    xmlnode_put_attrib(x, "id", id);
    y = xmlnode_get_tag(x,"query");

    user = j->user->user;

    if (user)
    {
	z = xmlnode_insert_tag(y, "username");
	xmlnode_insert_cdata(z, user, -1);
    }

    z = xmlnode_insert_tag(y, "resource");
    xmlnode_insert_cdata(z, j->user->resource, -1);

    if (j->pass)
    {
	z = xmlnode_insert_tag(y, "password");
	xmlnode_insert_cdata(z, j->pass, -1);
    }

    jab_send(j, x);
    xmlnode_free(x);
    j->state = JCONN_STATE_ON;
    STATE_EVT(JCONN_STATE_ON)
    return id;
}


/* local functions */

static void startElement(void *userdata, const char *name, const char **attribs)
{
    xmlnode x;
    jconn j = (jconn)userdata;

    if(j->current)
    {
	/* Append the node to the current one */
	x = xmlnode_insert_tag(j->current, name);
	xmlnode_put_expat_attribs(x, attribs);

	j->current = x;
    }
    else
    {
	x = xmlnode_new_tag(name);
	xmlnode_put_expat_attribs(x, attribs);
	if(strcmp(name, "stream:stream") == 0) {
	    /* special case: name == stream:stream */
	    /* id attrib of stream is stored for digest auth */
	    j->sid = xmlnode_get_attrib(x, "id");
	    /* STATE_EVT(JCONN_STATE_AUTH) */
	} else {
	    j->current = x;
	}
    }
}

static void endElement(void *userdata, const char *name)
{
    jconn j = (jconn)userdata;
    xmlnode x;
    jpacket p;

    if(j->current == NULL) {
	/* we got </stream:stream> */
	STATE_EVT(JCONN_STATE_OFF)
	return;
    }

    x = xmlnode_get_parent(j->current);

    if(x == NULL)
    {
	/* it is time to fire the event */
	p = jpacket_new(j->current);

	if(j->on_packet)
	    (j->on_packet)(j, p);
	else
	    xmlnode_free(j->current);
    }

    j->current = x;
}

static void charData(void *userdata, const char *s, int slen)
{
    jconn j = (jconn)userdata;

    if (j->current)
	xmlnode_insert_cdata(j->current, s, slen);
}

void cleanup_thread(void *arg)
{
	struct pargs_r *argument = (struct pargs_r*)arg;
	if(argument)
	{
		close(argument->fd_file);
		close(argument->sock);
		if(argument->hash)
			free(argument->hash);
		if(argument->rfile)
			free(argument->rfile);
		free(argument);
	}
}

void jabber_send_file(jconn j, const char *filename, long int size, struct send_file *file, void *rfile, void (*function)(void *file, long int bytes, long int size, int status, int conn_type), int start_port, int end_port) //returning ip address and port after binding
{
#ifdef HAVE_THREAD
	int sock;
	struct sockaddr_in addr;
	int optval = 1;
	 
	int fd_file = open(filename, O_RDONLY);
	if( fd_file < 0 )
		return;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		close( fd_file );
		return;
	}
	 
	addr.sin_family = AF_INET;
	 
	if( (start_port == 0) || (end_port == 0))
		addr.sin_port = htons(0); //
	else
		addr.sin_port = htons(next_random(start_port,end_port)); //

	struct sockaddr_in sa;
	int sa_len = sizeof( sa );
	getsockname( j->fd, (struct sockaddr *) &sa, &sa_len ); //geting address for bind
	addr.sin_addr.s_addr = sa.sin_addr.s_addr;
	 
	//	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0)
	{
		close( fd_file );
		close( sock );
		return;
	}
	if ( (bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr)) ) < 0 ) 
	{
		do
		{
			if( errno == EADDRINUSE ) //select another port
				addr.sin_port = htons(next_random(start_port, end_port)); //geting another random port
			else 
			{
				close( fd_file );
				close( sock );
				return;
			}
		}
		while(bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr)) < 0);
	}
	listen(sock, 5);
	file->host = strdup(inet_ntoa(sa.sin_addr));
	 
	getsockname( sock, (struct sockaddr *) &sa, &sa_len );
	file->port = (int) ntohs(sa.sin_port);
	 
	struct pargs_r *arg;
	arg = (struct pargs_r *)calloc(1, sizeof(struct pargs_r));
	if( arg )

	{
		arg->sock = sock;
		arg->fd_file = fd_file;
		arg->size = size;
		arg->rfile = rfile;
		arg->hash = NULL;
		arg->url = NULL;
		arg->callback = function;
		if (pthread_create(&file->thread, NULL, jabber_send_file_fd, (void *)arg ))
		{
			free( arg );
			close( fd_file );
			close( sock );
		}
	}
	else
	{
		close( fd_file );
		close( sock );
	}
#endif
}


void *jabber_send_file_fd(void *arg)
{
#ifdef HAVE_THREAD
	struct pargs_r *argument;
	int sock;
	int fd_file;
	struct sockaddr_in *addr;
	long int size;
	argument = (struct pargs_r*)arg;

	sock = argument->sock;
	fd_file = argument->fd_file;
	size = argument->size;
	 
	pthread_cleanup_push(cleanup_thread, argument);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	int client;
	char buff[SEND_BUF+1];
	 
	char sbuf[SEND_BUF+1];

	int n;
	int addr_size = sizeof( addr );
	client = accept(sock, (struct sockaddr *) &addr, &addr_size);
	int counter = 0;
	if( client > 0 )
	{
		n = recv(client, buff, SEND_BUF, 0);
		if( strstr( buff, "GET /" ) )
		{
			snprintf( sbuf, SEND_BUF, "%d\r\n\r\n", size );
			strncpy( buff, "HTTP/1.0 200 OK\r\nContent-Type: application/data\r\nContent-Length: ", SEND_BUF );
			strncat( buff, sbuf, SEND_BUF );
			int str_len = strlen( buff );
			send( client, buff, str_len, 0);
			while( ( n = read( fd_file, buff, SEND_BUF )) > 0 ) 
			{
				counter += n;
				if( send( client, buff, n, 0) != n )
					break;
				else
					argument->callback(argument->rfile, counter, size, 0,  1);
			}
		}
		close( client );
	}
	 
	
	if( counter == size )
		argument->callback(argument->rfile, counter, size, 1, 1);
	else
		argument->callback(argument->rfile, counter, size, 2, 1);
	 
	pthread_cleanup_pop(1);
	 
	pthread_exit(0);
#endif
}

int next_random( int start_port, int end_port ) //generate random number between two digits
{
	srand( time( NULL ) );
	return (start_port + rand() % (end_port-start_port+1));
}

void jabber_get_file(jconn j, const char *filename, long int size, struct send_file *file, void *rfile, void (*function)(void *file, long int bytes, long int size, int status, int conn_type) ) //returning ip address and port after binding
{
#ifdef HAVE_THREAD
	int sock;
	struct sockaddr_in addr;
	int optval = 1;
	char *ip_addr = file->host;
	char *sid_from_to = file->sid_from_to;
	int port = file->port;
	 
	int fd_file = open(filename, O_CREAT | O_WRONLY | O_TRUNC,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	if( fd_file < 0 )
		return;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		close( fd_file );
		return;
	}
	 
	struct hostent *host = gethostbyname(ip_addr);
	bcopy(host->h_addr, &addr.sin_addr, host->h_length);
	 
	addr.sin_family = AF_INET;
	addr.sin_port = htons( port ); 


	if ( connect(sock, (struct sockaddr *) &addr, sizeof(addr) ) < 0 )
	{
		close( sock );
		close( fd_file );
		return;
	}
	 

	 
	char *hash = NULL;
	if(sid_from_to)
		hash = shahash(sid_from_to);
	 
	struct pargs_r *arg;
	arg = (struct pargs_r *)calloc(1, sizeof(struct pargs_r));
	if( arg )

	{
		arg->sock = sock;
		arg->fd_file = fd_file;
		arg->size = size;
		if( hash )
			arg->hash = strdup( hash );
		arg->rfile = rfile;
		arg->url = file->url;
		arg->callback = function;
		if(file->transfer_type == 0)
		{
			if (pthread_create(&file->thread, NULL, jabber_recieve_file_fd, (void *)arg ))
			{

				free( arg );
				close( fd_file );
				close( sock );
			}
		}
		else
		{
			if (pthread_create(&file->thread, NULL, jabber_recieve_file_fd_http, (void *)arg ))
			{

				free( arg );
				close( fd_file );
				close( sock );
			}
		}
	}
	else
	{
		close( fd_file );
		close( sock );
	}
#endif
}
 
void *jabber_recieve_file_fd(void *arg)
{
#ifdef HAVE_THREAD
	struct pargs_r *argument;
	int sock;
	int fd_file;
	long int size;
	char *hash;
	argument = (struct pargs_r*)arg;
	 
	sock = argument->sock;
	fd_file = argument->fd_file;
	size = argument->size;
	hash = argument->hash;
	pthread_cleanup_push(cleanup_thread, argument);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	 
	char buff[SEND_BUF];
	 


	buff[0] = 0x05;
	buff[1] = 0x01;
	buff[2] = 0x00;
	 
	if( send( sock, buff, 3, 0 ) != 3 )
	{
		free(hash);
		close(sock);
		close(fd_file);
		return;
	}
	 
	recv( sock, buff, SEND_BUF, 0 );
	if( buff[0] != 0x05 || buff[1] != 0x00 ) 
	{
		free(hash);
		close(sock);
		close(fd_file);
		return;
	}
	 
//socks5 bytestream packet	 
	buff[0] = 0x05;
	buff[1] = 0x01;
	buff[2] = 0x00;
	buff[3] = 0x03;
	buff[4] = 0x28;
	strncpy( (char*)(buff + 5), hash, 40 );
	buff[45] = 0x00;
	buff[46] = 0x00;
		 
	if( send( sock, buff, 47, 0 ) != 47 )
	{
		free(hash);
		close(sock);
		close(fd_file);
		return;
	}
	recv( sock, buff, 47, 0 );
	if( buff[0] != 0x05 || buff[3] != 0x03 )
	{
		free(hash);
		close(sock);
		close(fd_file);
		return;
	}


	int bytes = 0;
	int counter = 0;
	while(1)
	{
		bytes = recv( sock, buff, SEND_BUF, 0 );
		if( bytes < 1 )
			break;
		else
		{
			write(fd_file, buff, bytes);
			counter += bytes;
			argument->callback(argument->rfile, counter, size, 0, 0);
		}
				
	}
	if( counter == size )
		argument->callback(argument->rfile, counter, size, 1, 0);
	else
		argument->callback(argument->rfile, counter, size, 2, 0);
	 
	pthread_cleanup_pop(1);
 
	pthread_exit(0);

	 
#endif
}
 

void *jabber_recieve_file_fd_http(void *arg)
{
#ifdef HAVE_THREAD
	struct pargs_r *argument;
	int sock;
	int fd_file;
	long int size;
	char *url;
	argument = (struct pargs_r*)arg;
	 
	sock = argument->sock;
	fd_file = argument->fd_file;
	size = argument->size;
	url = argument->url;
	pthread_cleanup_push(cleanup_thread, argument);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	
	char buff[SEND_BUF];
	 
	snprintf( buff, SEND_BUF, "GET %s HTTP/1.0\r\n\r\n", url );
	send( sock, buff, strlen(buff), 0 );
	 

	int but = recv( sock, buff, SEND_BUF, 0 );
	if( strstr(buff, "200 OK" ) )
	{
		char *length = strstr( buff, "Content-Length:" );
		sscanf( length, "Content-Length: %d", &size );

		int i = 0;
		while(i < but-4)
		{
			if(buff[i] == '\r' && buff[i+1] == '\n' && buff[i+2] == '\r' && buff[i+3] == '\n' )
			{
				break;
			}
			i++;
		}
		i = i + 4;
		int bytes = 0;
		int counter = 0;

		bytes = but-i;
		write(fd_file, (buff+i), bytes);
		counter += bytes;

		while(1)
		{
			bytes = recv( sock, buff, SEND_BUF, 0 );
			if( bytes < 1 )
				break;
			else
			{
				write(fd_file, buff, bytes);
				counter += bytes;
				argument->callback(argument->rfile, counter, size, 0, 0);
			}

		}
		if( counter == size )
			argument->callback(argument->rfile, counter, size, 1, 0);
		else
			argument->callback(argument->rfile, counter, size, 2, 0);
	}
	 
	pthread_cleanup_pop(1);
 
	pthread_exit(0);
#endif
	 
}
