/*
*
* centerim Jabber protocol handling class
* $Id: jabberhook.cc,v 1.85 2005/08/26 11:01:49 konst Exp $
*
* Copyright (C) 2002-2005 by Konstantin Klyagin <k@thekonst.net>
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

#include "icqcommon.h"

#ifdef BUILD_JABBER

#include "jabberhook.h"
#include "icqface.h"
#include "imlogger.h"
#include "eventmanager.h"
#include "icqgroups.h"
#include "impgp.h"
#include "icqcontacts.h"
#include "centerim.h"

#ifdef HAVE_SSTREAM
    #include <sstream>
#else
    #include <strstream>
#endif

#ifdef HAVE_LIBOTR
  #include "imotr.h"
#endif 


#define DEFAULT_CONFSERV "conference.jabber.org"
#define PERIOD_KEEPALIVE 30

#define NOTIFBUF 512

//base64 enc/dec functions to work with binary data in xmpp	 
static char b64table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *base64_encode( char *buffer, int bufferLen )
{
	if ( buffer==NULL || bufferLen<=0 )
		return NULL;

	char* res = (char*)malloc(((( bufferLen+2 )*4 )/3 ) + 1);
	if ( res == NULL )
		return NULL;

	unsigned char igroup[3];
	int nGroups = 0;
	char *r = res;
	char *peob = buffer + bufferLen;
	char *p;
	for ( p = buffer; p < peob; ) {
		igroup[ 0 ] = igroup[ 1 ] = igroup[ 2 ] = 0;
		int n;
		for ( n=0; n<3; n++ ) {
			if ( p >= peob ) break;
			igroup[n] = ( unsigned char ) *p;
			p++;
		}

		if ( n > 0 ) {
			r[0] = b64table[ igroup[0]>>2 ];
			r[1] = b64table[ (( igroup[0]&3 )<<4 ) | ( igroup[1]>>4 ) ];
			r[2] = b64table[ (( igroup[1]&0xf )<<2 ) | ( igroup[2]>>6 ) ];
			r[3] = b64table[ igroup[2]&0x3f ];

			if ( n < 3 ) {
				r[3] = '=';
				if ( n < 2 )
					r[2] = '=';
			}
			r += 4;
	}	}

	*r = '\0';

	return res;
}
	 
static char *base64_decode(const char *str, int *ret_len)
{
	if( !str )
		return NULL;
	char *out = NULL;
	char tmp = 0;
	const char *c;
	int tmp2 = 0;
	int len = 0, n = 0;


	c = str;

	while (*c) {
		if (*c >= 'A' && *c <= 'Z') {
			tmp = *c - 'A';
		} else if (*c >= 'a' && *c <= 'z') {
			tmp = 26 + (*c - 'a');
		} else if (*c >= '0' && *c <= 57) {
			tmp = 52 + (*c - '0');
		} else if (*c == '+') {
			tmp = 62;
		} else if (*c == '/') {
			tmp = 63;
		} else if (*c == '\r' || *c == '\n') {
			c++;
			continue;
		} else if (*c == '=') {
			if (n == 3) {
				out = (char*)realloc(out, len + 2);
				out[len] = (char)(tmp2 >> 10) & 0xff;
				len++;
				out[len] = (char)(tmp2 >> 2) & 0xff;
				len++;
			} else if (n == 2) {
				out = (char*)realloc(out, len + 1);
				out[len] = (char)(tmp2 >> 4) & 0xff;
				len++;
			}
			break;
		}
		tmp2 = ((tmp2 << 6) | (tmp & 0xff));
		n++;
		if (n == 4) {
			out = (char*)realloc(out, len + 3);
			out[len] = (char)((tmp2 >> 16) & 0xff);
			len++;
			out[len] = (char)((tmp2 >> 8) & 0xff);
			len++;
			out[len] = (char)(tmp2 & 0xff);
			len++;
			tmp2 = 0;
			n = 0;
		}
		c++;
	}

	out = (char*)realloc(out, len + 1);
	out[len] = 0;

	if (ret_len != NULL)
		*ret_len = len;

	return out;
}

static void jidsplit(const string &jid, string &user, string &host, string &rest) {
    int pos;
    user = jid;

    if((pos = user.find("/")) != -1) {
	rest = user.substr(pos+1);
	user.erase(pos);
    }

    if((pos = user.find("@")) != -1) {
	host = user.substr(pos+1);
	user.erase(pos);
    }
    else {
        host = user;
        user = (string)"";
    }
}

static string jidtodisp(const string &jid) {
    string user, host, rest;
    jidsplit(jid, user, host, rest);

    if(!user.empty())
	user += (string) "@" + host;
    else
	user = host;

    return user;
}

static imstatus get_presence(std::map<string, pair<int, imstatus> > res) // <resource, <prio, status> >
{
	if (res.empty())
		return offline;
	imstatus result = offline;
	int prio = -127;
	for (map<string, pair<int, imstatus> >::iterator it = res.begin(); it!= res.end(); it++)
	{
		if (it->second.first>prio)
		{
			result = it->second.second;
			prio = it->second.first;
		}
	}
	return result;
}


// ----------------------------------------------------------------------------

jabberhook jhook;

string jabberhook::jidnormalize(const string &jid) const {
    if(find(agents.begin(), agents.end(), jid) != agents.end())
	return jid;

    string user, host, rest;
    jidsplit(jid, user, host, rest);

    if(host.empty())
	host = conf->getourid(proto).server;

    if (user.empty())
	user = host;
    else
	user += (string) "@" + host;
    if(!rest.empty()) user += (string) "/" + rest;
    return user;
}

jabberhook::jabberhook(): abstracthook(jabber), jc(0), flogged(false), fonline(false) {
    fcapabs.insert(hookcapab::setaway);
    fcapabs.insert(hookcapab::setextstatus);
    fcapabs.insert(hookcapab::fetchaway);
    fcapabs.insert(hookcapab::authrequests);
    fcapabs.insert(hookcapab::directadd);
    fcapabs.insert(hookcapab::flexiblesearch);
    fcapabs.insert(hookcapab::flexiblereg);
    fcapabs.insert(hookcapab::visibility);
    fcapabs.insert(hookcapab::ssl);
    fcapabs.insert(hookcapab::changedetails);
    fcapabs.insert(hookcapab::conferencing);
    fcapabs.insert(hookcapab::groupchatservices);
    fcapabs.insert(hookcapab::changenick);
    fcapabs.insert(hookcapab::changeabout);
    fcapabs.insert(hookcapab::version);
#ifdef HAVE_THREAD
    fcapabs.insert(hookcapab::files);
#endif
    fcapabs.insert(hookcapab::pgp);
	fcapabs.insert(hookcapab::acknowledgements);
}

jabberhook::~jabberhook() {
}

void jabberhook::init() {
    char rnd[9];
    snprintf(rnd, 9, "%X%X", rand()%0xFFFF, rand()%0xFFFF);
    uuid = rnd;
    manualstatus = conf->getstatus(proto);
}

void jabberhook::connect() {
    icqconf::imaccount acc = conf->getourid(proto);
    string jid = getourjid();


    log(logConnecting);

    /* TODO: is there really a need for COPYING these char-arrays
     * shoudn't it be possible to feed the c_str() directly into jab_new
     * and get rid of the copying ?
     */
    char *cjid = strdup(jid.c_str());
    char *cpass = strdup(acc.password.c_str());
    char *cserver = strdup(acc.server.c_str());

    regmode = flogged = fonline = false;

    if(jc){
      jab_delete(jc);
    }
    
    statuses.clear();
    
    jc = jab_new(cjid, cpass, cserver, acc.port,
	acc.additional["ssl"] == "1" ? 1 : 0);

    jab_packet_handler(jc, &packethandler);
    jab_state_handler(jc, &statehandler);

    if(conf->getdebug())
	jab_logger(jc, &jlogger);

    if(jc->user) {
	fonline = true;
	jstate = STATE_CONNECTING;
	statehandler(0, -1);
	jab_start(jc);
    }

    free (cjid);
    free (cpass);
    free (cserver);
}

void jabberhook::disconnect() {
    // announce it to  everyone else
    setjabberstatus(offline, "");

    // announce it to the user
    statehandler(jc, JCONN_STATE_OFF);

    // close the connection
    jab_stop(jc);
    jab_delete(jc);
    jc = 0;
    statuses.clear();
}

void jabberhook::exectimers() {
    if(logged()) {
	if(timer_current-timer_keepalive > PERIOD_KEEPALIVE) {
	    jab_send_raw(jc, "  \t  ");
	    timer_keepalive = timer_current;
	}
    }
}

void jabberhook::main() {
    xmlnode x, z;
    char *cid;

    if(jc && jc->state == JCONN_STATE_CONNECTING) {
	jab_start(jc);
	return;
    }
    
    jab_poll(jc, 0);

    if(jstate == STATE_CONNECTING) {
	if(jc) {
	    x = jutil_iqnew(JPACKET__GET, NS_AUTH);
	    cid = jab_getid(jc);
	    xmlnode_put_attrib(x, "id", cid);
	    id = atoi(cid);

	    z = xmlnode_insert_tag(xmlnode_get_tag(x, "query"), "username");
	    xmlnode_insert_cdata(z, jc->user->user, (unsigned) -1);
	    jab_send(jc, x);
	    xmlnode_free(x);

	    jstate = STATE_GETAUTH;
	}

	if(!jc || jc->state == JCONN_STATE_OFF) {
	    face.log(_("+ [jab] unable to connect to the server"));
	    fonline = false;
	}
    }

    if(!jc) {
	statehandler(jc, JCONN_STATE_OFF);

    } else if(jc->state == JCONN_STATE_OFF || jc->fd == -1) {
	statehandler(jc, JCONN_STATE_OFF);

    }
}

void jabberhook::getsockets(fd_set &rfds, fd_set &wfds, fd_set &efds, int &hsocket) const {
    if(jc) {
	if (jc->cw_state & CW_CONNECT_WANT_READ)
	    FD_SET(jc->fd, &rfds);
	else if (jc->cw_state & CW_CONNECT_WANT_WRITE)
	    FD_SET(jc->fd, &wfds);
	else
	    FD_SET(jc->fd, &rfds);
	hsocket = max(jc->fd, hsocket);
    }
}

bool jabberhook::isoursocket(fd_set &rfds, fd_set &wfds, fd_set &efds) const {
    if(jc) return FD_ISSET(jc->fd, &rfds) || FD_ISSET(jc->fd, &wfds);
    return false;
}

bool jabberhook::online() const {
    return fonline;
}

bool jabberhook::logged() const {
    return fonline && flogged;
}

bool jabberhook::isconnecting() const {
    return fonline && !flogged;
}

bool jabberhook::enabled() const {
    return true;
}

bool jabberhook::send(const imevent &ev) {
    icqcontact *c = clist.get(ev.getcontact());
    string text, cname, enc;

    if(c) {
	if(ev.gettype() == imevent::message) {
	    const immessage *m = static_cast<const immessage *>(&ev);
	    text = m->gettext();

	} else if(ev.gettype() == imevent::url) {
	    const imurl *m = static_cast<const imurl *>(&ev);
	    text = m->geturl() + "\n\n" + m->getdescription();

	} else if(ev.gettype() == imevent::file) {
		const imfile *m = static_cast<const imfile *>(&ev);
		vector<imfile::record> files = m->getfiles();
		vector<imfile::record>::const_iterator ir = files.begin();

		string cjid_str = jidtodisp(ev.getcontact().nickname);
		transform(cjid_str.begin(), cjid_str.end(), cjid_str.begin(), ::tolower); 
		char *cjid = strdup(cjid_str.c_str());
	 
		struct send_file *trans_file = (struct send_file *)malloc( sizeof( struct send_file ) );
		srfiles[cjid].first = (*m);
		back_srfiles[*m] = cjid; //backward comp. , little hack :(
		 
		trans_file->full_jid_name = strdup(jhook.full_jids[cjid].c_str());
		trans_file->id = NULL;
		trans_file->transfer_type = 5;
		trans_file->host = NULL;
		trans_file->url = NULL;
		trans_file->sid_from_to = NULL;
		 
		srfiles[cjid].second.first = trans_file;
		srfiles[cjid].second.second = 0;
			

		face.transferupdate(files[0].fname, *m, icqface::tsInit, files[0].size, 0);

		send_file(cjid);

		} else if(ev.gettype() == imevent::authorization) {
	    const imauthorization *m = static_cast<const imauthorization *> (&ev);
	    char *cjid = strdup(jidnormalize(ev.getcontact().nickname).c_str());
	    xmlnode x = 0;

	    switch(m->getauthtype()) {
		case imauthorization::Granted:
		    x = jutil_presnew(JPACKET__SUBSCRIBED, cjid, 0);
		    break;
		case imauthorization::Rejected:
		    x = jutil_presnew(JPACKET__UNSUBSCRIBED, cjid, 0);
		    break;
		case imauthorization::Request:
		    x = jutil_presnew(JPACKET__SUBSCRIBE, cjid, 0);
		    break;
	    }

	    if(x) {
		jab_send(jc, x);
		xmlnode_free(x);
	    }

	    free(cjid);

	    return true;
	}

	text = rusconv("ku", text);

#ifdef HAVE_GPGME
	if(pgp.enabled(ev.getcontact())) {
	    enc = pgp.encrypt(text, c->getpgpkey(), proto);
	    if (enc.empty() && !text.empty()) // probably encryption error
		return false;
	    text = "This message is encrypted.";
	}
#endif

#ifdef HAVE_LIBOTR
    if (!otr.send_message(proto, jidnormalize(ev.getcontact().nickname), text))
    {
        return true;
    }
#endif

	/* TODO: do these really needs to be copied? */
	char *cjid = strdup(jidnormalize(c->getdesc().nickname).c_str());
	char *ctext = strdup(text.c_str());

	xmlnode x = jutil_msgnew(TMSG_CHAT, cjid, 0, ctext);

	if(ischannel(c)) {
	    xmlnode_put_attrib(x, "type", "groupchat");
	    if(!(cname = c->getdesc().nickname.substr(1)).empty())
		xmlnode_put_attrib(x, "to", cname.c_str());
	}

	if(!enc.empty()) {
	    xmlnode xenc = xmlnode_insert_tag(x, "x");
	    xmlnode_put_attrib(xenc, "xmlns", "jabber:x:encrypted");
	    xmlnode_insert_cdata(xenc, enc.c_str(), (unsigned) -1);
	}

	if(conf->getourid(jhook.proto).additional["acknowledgements"] == "1") {
	vector<agent>::iterator ia = find(jhook.agents.begin(),jhook.agents.end(), jhook.full_jids[cjid]);
	if(ia != jhook.agents.end() && ia->params[agent::ptReceipts].enabled)
	{
	    xmlnode request = xmlnode_insert_tag(x, "request");
	    xmlnode_put_attrib(request, "xmlns", NS_RECEIPTS);
	}
	}
#ifdef HAVE_SSTREAM
	stringstream idstream;
	idstream << ev.gettimestamp();
	xmlnode_put_attrib(x, "id", idstream.str().c_str());
#else
	strstream idstream;
	idstream << ev.gettimestamp();
	xmlnode_put_attrib(x, "id", idstream.str());
#endif

	jab_send(jc, x);
	xmlnode_free(x);

	free (cjid);
	free (ctext);

	return true;
    }

    return false;
}

void jabberhook::sendnewuser(const imcontact &ic) {
    sendnewuser(ic, true);
}

void jabberhook::removeuser(const imcontact &ic) {
    removeuser(ic, true);
}

void jabberhook::sendnewuser(const imcontact &ic, bool report) {
    xmlnode x, y, z;
    icqcontact *c;
    string cname;

    if(!logged())
	return;

    if(!ischannel(ic)) {
	char *cjid = strdup(jidnormalize(ic.nickname).c_str());
	if(roster.find(cjid) != roster.end()) {
	    free (cjid);
	    return;
	}

	if(report) log(logContactAdd, ic.nickname.c_str());

	x = jutil_presnew(JPACKET__SUBSCRIBE, cjid, 0);
	jab_send(jc, x);
	xmlnode_free(x);

	x = jutil_iqnew(JPACKET__SET, NS_ROSTER);
	xmlnode_put_attrib(x, "id", jab_getid(jc));
	y = xmlnode_get_tag(x, "query");
	z = xmlnode_insert_tag(y, "item");
	xmlnode_put_attrib(z, "jid", cjid);

	roster[cjid] = "";

	if(c = clist.get(ic)) {
	    vector<icqgroup>::const_iterator ig = find(groups.begin(), groups.end(), c->getgroupid());
	    if(ig != groups.end()) {
		z = xmlnode_insert_tag(z, "group");
		xmlnode_insert_cdata(z, rusconv("ku", ig->getname()).c_str(), (unsigned) -1);
		roster[cjid] = ig->getname();
	    }
	}

	jab_send(jc, x);
	xmlnode_free(x);

	if(c = clist.get(ic)) {
	    imcontact icc(jidtodisp(ic.nickname), proto);
	    if(ic.nickname != icc.nickname) {
		c->setdesc(icc);
	    }

	    c->setnick("");

	    string u, h, r;
	    jidsplit(icc.nickname, u, h, r);
	    if (!u.empty())
	    	c->setdispnick(u);
	    else
		c->setdispnick(icc.nickname);
	}

	requestinfo(c);
	
	free (cjid);
    } else {
	if(c = clist.get(ic)) {
	    cname = ic.nickname.substr(1);

	    if(!cname.empty()) {
		cname += "/" + conf->getourid(proto).nickname;

		char *ccname = strdup(cname.c_str());
		char *ourjid = strdup(getourjid().c_str());

		x = jutil_presnew(JPACKET__UNKNOWN, ccname, 0);
		xmlnode_insert_cdata(xmlnode_insert_tag(x, "status"), "Online", (unsigned) -1);

		free (ccname);
		free (ourjid);

		jab_send(jc, x);
		xmlnode_free(x);
	    }
	}
    }
}

void jabberhook::removeuser(const imcontact &ic, bool report) {
    xmlnode x, y, z;
    icqcontact *c;
    string cname;

    if(!logged())
	return;

    if(!ischannel(ic)) {
	char *cjid = strdup(jidnormalize(ic.nickname).c_str());

	map<string, string>::iterator ir = roster.find(cjid);

	if(ir == roster.end()) return;
	    else roster.erase(ir);

	if(find(agents.begin(), agents.end(), cjid) != agents.end()) {
	    if(report) face.log(_("+ [jab] unregistering from the %s agent"), cjid);

	    x = jutil_iqnew(JPACKET__SET, NS_REGISTER);
	    xmlnode_put_attrib(x, "id", jab_getid(jc));
	    xmlnode_put_attrib(x, "to", cjid);
	    y = xmlnode_get_tag(x, "query");
	    xmlnode_insert_tag(y, "remove");
	    jab_send(jc, x);
	    xmlnode_free(x);

	}

	if(report) log(logContactRemove, ic.nickname.c_str());

	x = jutil_presnew(JPACKET__UNSUBSCRIBE, cjid, 0);
	jab_send(jc, x);
	xmlnode_free(x);

	x = jutil_iqnew(JPACKET__SET, NS_ROSTER);
	xmlnode_put_attrib(x, "id", jab_getid(jc));
	y = xmlnode_get_tag(x, "query");
	z = xmlnode_insert_tag(y, "item");
	xmlnode_put_attrib(z, "jid", cjid);
	xmlnode_put_attrib(z, "subscription", "remove");
	jab_send(jc, x);
	xmlnode_free(x);

	free (cjid);

    } else {
	if(c = clist.get(ic)) {
	    cname = ic.nickname.substr(1);

	    if(!cname.empty()) {
		cname += "/" + conf->getourid(proto).nickname;
		char *ccname = strdup(cname.c_str());
		x = jutil_presnew(JPACKET__UNKNOWN, ccname, 0);
		xmlnode_put_attrib(x, "type", "unavailable");
		jab_send(jc, x);
		xmlnode_free(x);

		free(ccname);

		map<string, vector<string> >::iterator icm = chatmembers.find(ic.nickname);
		if(icm != chatmembers.end()) chatmembers.erase(icm);
	    }
	}
    }
}

void jabberhook::setautostatus(imstatus st) {
    if(st != offline) {
	if(getstatus() == offline) {
	    connect();
	} else {
	    string msg;

	    switch(st) {
		case dontdisturb:
		case occupied:
		case outforlunch:
		case notavail:
		    msg = conf->getextstatus(proto, st); //external online status
		    if( msg.empty() ) msg = conf->getawaymsg(proto);
		    break;
		case away:
		    msg = conf->getawaymsg(proto);
		    break;
		case available:
		case freeforchat:
		    msg = conf->getextstatus(proto, st); //external online status
	    }

	    setjabberstatus(ourstatus = st, msg);
	}
    } else {
	if(getstatus() != offline) {
	    disconnect();
	}
    }
}

void jabberhook::requestinfo(const imcontact &ic) {
    if(isconnecting() || logged()) {
	vector<agent>::const_iterator ia = find(agents.begin(), agents.end(), ic.nickname);

	if(ia != agents.end()) {
	    icqcontact *c = clist.get(imcontact(ic.nickname, proto));
	    if(c) {
		c->setdispnick(ia->name);
		c->setabout(ia->desc);
	    }

	} else {
	    char *cjid = strdup(jidnormalize(ic.nickname).c_str());
	    xmlnode x = jutil_iqnew2(JPACKET__GET), y;//gtalk vCard request fix
	    xmlnode_put_attrib(x, "to", cjid);
	    xmlnode_put_attrib(x, "id", "VCARDreq");
	    y = xmlnode_insert_tag(x, "vCard");
	    xmlnode_put_attrib(y, "xmlns", NS_VCARD);
	    jab_send(jc, x);
	    xmlnode_free(x);
	    free (cjid);

	}
    }
}

void jabberhook::requestawaymsg(const imcontact &ic) {
    icqcontact *c = clist.get(ic);

    if(c) {
	string am = awaymsgs[ic.nickname];

	if(!am.empty()) {
	    em.store(imnotification(ic, (string) _("Away message:") + "\n\n" + rusconv("uk",am)));
	} else {
	    face.log(_("+ [jab] no away message from %s, %s"),
		c->getdispnick().c_str(), ic.totext().c_str());
	}
    }
}

imstatus jabberhook::getstatus() const {
    return online() ? ourstatus : offline;
}

bool jabberhook::regnick(const string &nick, const string &pass,
const string &serv, string &err) {
    int pos, port;
    string jid = nick + "@" + serv;

    if((pos = jid.find(":")) != -1) {
	port = atoi(jid.substr(pos+1).c_str());
	jid.erase(pos);
    } else {
	port = icqconf::defservers[proto].port;
    }

    regmode = true;

    /* TODO: do these really need to be copied ??? */
    char *cjid = strdup(jid.c_str());
    char *cpass = strdup(pass.c_str());
    char *cserver = strdup(serv.c_str());

    jc = jab_new(cjid, cpass, cserver, port, 0);

    free (cjid);
    free (cpass);
    free (cserver);

    if(!jc->user) {
	err = _("Wrong nickname given, cannot register");
	fonline = false;
	return false;
    }

    jab_packet_handler(jc, &packethandler);
    jab_state_handler(jc, &statehandler);

    if(conf->getdebug())
	jab_logger(jc, &jlogger);

    jc->cw_state = CW_CONNECT_BLOCKING;
    jab_start(jc);

    if(jc) id = atoi(jab_reg(jc));

    if(!online()) {
	err = _("Unable to connect");

    } else {
	regdone = false;
	regerr = "";

	while(online() && !regdone && regerr.empty()) {
	    main();
	}

	disconnect();
	err = regdone ? "" : regerr;
    }

    return regdone;
}

void jabberhook::setjabberstatus(imstatus st, string msg) {
    xmlnode x = jutil_presnew(JPACKET__UNKNOWN, 0, 0);

    switch(st) {
	case away:
	    xmlnode_insert_cdata(xmlnode_insert_tag(x, "show"), "away", (unsigned) -1);
	    break;

	case occupied:
	case dontdisturb:
	    xmlnode_insert_cdata(xmlnode_insert_tag(x, "show"), "dnd", (unsigned) -1);
	    break;

	case freeforchat:
	    xmlnode_insert_cdata(xmlnode_insert_tag(x, "show"), "chat", (unsigned) -1);
	    break;

	case outforlunch:
	case notavail:
	    xmlnode_insert_cdata(xmlnode_insert_tag(x, "show"), "xa", (unsigned) -1);
	    break;

	case invisible:
	    xmlnode_put_attrib(x, "type", "invisible");
	    break;

	case offline:
	    xmlnode_put_attrib(x, "type", "unavailable");
	    break;

    }

    map<string, string> add = conf->getourid(proto).additional;

    if(!add["prio"].empty())
	xmlnode_insert_cdata(xmlnode_insert_tag(x, "priority"),
	    add["prio"].c_str(), (unsigned) -1);

    if(msg.empty())
	msg = imstatus2str(st);
	int pos = 0;
	while( (pos = msg.find( '\r' )) != string::npos ) {
		if( msg[pos+1] == '\n' ) {
			msg.erase(pos, 1);
		} else {
			msg.replace(pos, 1, "\n");
		}
	}

    xmlnode_insert_cdata(xmlnode_insert_tag(x, "status"),
	rusconv("ku", msg).c_str(), (unsigned) -1);

//check if our avatar changed and send update request
    icqcontact *ic = clist.get(contactroot);
    icqcontact::basicinfo bi = ic->getbasicinfo();

    if(!bi.avatar.empty())
    {
	    string my_avatar_hash;
	    if( get_my_avatar_hash(my_avatar_hash) )
	    {
		    xmlnode y = xmlnode_insert_tag(x, "x");
		    xmlnode_put_attrib(y, "xmlns", NS_VCARDUP);
		    xmlnode z = xmlnode_insert_tag(y, "photo");
		    xmlnode_insert_cdata(z, my_avatar_hash.c_str(), (unsigned) -1);
	    }
    }

#ifdef HAVE_GPGME

    if(!add["pgpkey"].empty()) {
	pgp.clearphrase(proto);
	xmlnode sign = xmlnode_insert_tag(x, "x");
	xmlnode_put_attrib(sign, "xmlns", "jabber:x:signed");
	xmlnode_insert_cdata(sign, pgp.sign(msg, add["pgpkey"], proto).c_str(), (unsigned) -1);
    }

#endif

    jab_send(jc, x);
    xmlnode_free(x);

    sendvisibility();

    logger.putourstatus(proto, getstatus(), ourstatus = st);
}

void jabberhook::sendvisibility() {
    xmlnode x;
    icqlist::iterator i;

    for(i = lst.begin(); i != lst.end(); ++i)
    if(i->getdesc().pname == proto) {
	x = jutil_presnew(JPACKET__UNKNOWN, 0, 0);
	xmlnode_put_attrib(x, "to", jidnormalize(i->getdesc().nickname).c_str());

	if(i->getstatus() == csvisible && ourstatus == invisible) {

	} else if(i->getstatus() == csvisible && ourstatus != invisible) {

	} else if(i->getstatus() == csinvisible && ourstatus == invisible) {

	} else if(i->getstatus() == csinvisible && ourstatus != invisible) {
	    xmlnode_put_attrib(x, "type", "unavailable");

	}

	jab_send(jc, x);
	xmlnode_free(x);
    }
}

vector<string> jabberhook::getservices(servicetype::enumeration st) const {
    vector<agent>::const_iterator ia = agents.begin();
    vector<string> r;

    agent::param_type pt;

    switch(st) {
	case servicetype::search:
	    pt = agent::ptSearch;
	    break;
	case servicetype::registration:
	    pt = agent::ptRegister;
	    break;
	case servicetype::groupchat:
	    while(ia != agents.end()) {
		if(ia->type == agent::atGroupchat) r.push_back(ia->jid);
		++ia;
	    }
	default:
	    return r;
    }

    while(ia != agents.end()) {
	if(ia->params[pt].enabled)
	    r.push_back(ia->name);
	++ia;
    }

    return r;
}

vector<pair<string, string> > jabberhook::getservparams(const string &agentname, agent::param_type pt) const {
    vector<agent>::const_iterator ia = agents.begin();

    while(ia != agents.end()) {
	if(ia->name == agentname)
	if(ia->params[pt].enabled)
	    return ia->params[pt].paramnames;

	++ia;
    }

    return vector<pair<string, string> >();
}

vector<pair<string, string> > jabberhook::getsearchparameters(const string &agentname) const {
    return getservparams(agentname, agent::ptSearch);
}

vector<pair<string, string> > jabberhook::getregparameters(const string &agentname) const {
    return getservparams(agentname, agent::ptRegister);
}

void jabberhook::gotagentinfo(xmlnode x) {
    xmlnode y;
    string name, data, ns;
    //agent::param_type pt;
    vector<agent>::iterator ia = jhook.agents.begin();
    const char *from = xmlnode_get_attrib(x, "from"), *p, *q;

    if(!from) return;

    while(ia != jhook.agents.end()) {
	if(ia->jid == from)
	if(y = xmlnode_get_tag(x, "query")) {
	    p = xmlnode_get_attrib(y, "xmlns"); if(p) ns = p;
	    if (ns == NS_DISCOINFO) {
	    for(y = xmlnode_get_firstchild(y); y; y = xmlnode_get_nextsibling(y)) {
		p = xmlnode_get_name(y); name = p ? p : "";
		if (name == "identity") {
		    if (q = xmlnode_get_attrib(y, "name"))
			ia->desc = q;
		    if (q = (xmlnode_get_attrib(y, "category"))) {
			data = q;
			if (data == "conference") {
			    ia->type = agent::atGroupchat;
			} else if (data == "gateway") {
			    ia->type = agent::atTransport;
			}
		    }
		} else if (name == "feature") {
		    if ((q = xmlnode_get_attrib(y, "var")) && (!strcmp(q, NS_SEARCH))) { // can do search, ask for parameters
			ia->params[agent::ptSearch].enabled = true;
	    		ia->params[agent::ptSearch].paramnames.clear();
			xmlnode z = jutil_iqnew(JPACKET__GET, NS_SEARCH);
			char *id = jab_getid(jc);
			ignore_ids.insert((string)id);
			xmlnode_put_attrib(z, "id", id);
			xmlnode_put_attrib(z, "to", from);
			jab_send(jc, z);
			xmlnode_free(z);
		    } else if ((q = xmlnode_get_attrib(y, "var")) && (!strcmp(q, NS_RECEIPTS))) {
			ia->params[agent::ptReceipts].enabled = true;
			}
		}
	    }
	    break;
	    } else if (ns == NS_SEARCH) { // probably agent info with jabber:iq:search
		for(y = xmlnode_get_firstchild(y); y; y = xmlnode_get_nextsibling(y)) {
		    p = xmlnode_get_name(y); name = p ? p : "";
		    p = xmlnode_get_data(y); data = p ? p : "";
		    if (name == "item") // it's probably not answer with parameters
			break;
		    if (name == "instructions") {
			ia->params[agent::ptSearch].instruction = data;
		    } else if ((name == "x") && (NSCHECK(y, "jabber:x:data"))) {
			continue;
		    } else if (name == "key") {
		    	ia->params[agent::ptSearch].key = data;
		    } else {
			ia->params[agent::ptSearch].paramnames.push_back(make_pair(name, data));
		    }
	    	}
		if ((name != "item") && ia->params[agent::ptSearch].paramnames.empty())
		    agents.erase(ia);
		break;
	    }
	}
	++ia;
    }

/*
    while(ia != jhook.agents.end()) {
	if(ia->jid == from)
	if(y = xmlnode_get_tag(x, "query")) {
	    p = xmlnode_get_attrib(y, "xmlns"); if(p) ns = p;

	    if(ns == NS_SEARCH) pt = agent::ptSearch; else
	    if(ns == NS_REGISTER) pt = agent::ptRegister; else
		break;

	    ia->params[pt].enabled = true;
	    ia->params[pt].paramnames.clear();

	    for(y = xmlnode_get_firstchild(y); y; y = xmlnode_get_nextsibling(y)) {
		p = xmlnode_get_name(y); name = p ? p : "";
		p = xmlnode_get_data(y); data = p ? p : "";

		if(name == "instructions") ia->params[pt].instruction = data; else
		if(name == "key") ia->params[pt].key = data; else
		if(!name.empty() && name != "registered") {
		    ia->params[pt].paramnames.push_back(make_pair(name, data));
		}
	    }

	    if(ia->params[pt].paramnames.empty()) agents.erase(ia);
	    break;
	}
	++ia;
    }
*/
}

void jabberhook::lookup(const imsearchparams &params, verticalmenu &dest) {
    xmlnode x, y;

    searchdest = &dest;

    while(!foundguys.empty()) {
	delete foundguys.back();
	foundguys.pop_back();
    }

    if(!params.service.empty()) {
	x = jutil_iqnew(JPACKET__SET, NS_SEARCH);
	xmlnode_put_attrib(x, "id", "Lookup");

	y = xmlnode_get_tag(x, "query");

	vector<agent>::const_iterator ia = agents.begin();
	while(ia != agents.end()) {
	    if(ia->name == params.service) {
		xmlnode_put_attrib(x, "to", ia->jid.c_str());
		xmlnode_insert_cdata(xmlnode_insert_tag(y, "key"),
		    ia->params[agent::atSearch].key.c_str(), (unsigned int) -1);
		break;
	    }
	    ++ia;
	}

	vector<pair<string, string> >::const_iterator ip = params.flexparams.begin();
	while(ip != params.flexparams.end()) {
	    xmlnode_insert_cdata(xmlnode_insert_tag(y,
		ip->first.c_str()), ip->second.c_str(), (unsigned int) -1);
	    ++ip;
	}

	jab_send(jc, x);
	xmlnode_free(x);

    } else if(!params.room.empty()) {
	icqcontact *c;
	string room = params.room.substr(1);

	if(c = clist.get(imcontact(params.room, proto))) {
	    vector<string>::const_iterator im = chatmembers[room].begin();
	    while(im != chatmembers[room].end()) {
		foundguys.push_back(c = new icqcontact(imcontact(*im, proto)));
		searchdest->additem(conf->getcolor(cp_clist_jabber), c, (string) " " + *im);
		++im;
	    }
	}

	face.findready();
	log(logConfMembers, foundguys.size());

	searchdest->redraw();
	searchdest = 0;
    }
}

void jabberhook::renamegroup(const string &oldname, const string &newname) {
    map<string, string>::iterator ir = roster.begin();

    while(ir != roster.end()) {
	if(ir->second == oldname) {
	    icqcontact *c = clist.get(imcontact(jidtodisp(ir->first), proto));
	    if(c) {
		updatecontact(c);
		ir->second = newname;
	    }
	}

	++ir;
    }
}

void jabberhook::ouridchanged(const icqconf::imaccount &ia) {
    if(logged()) {
	setautostatus(ourstatus);
	// send a new presence
    }
}

// ----------------------------------------------------------------------------

void jabberhook::gotsearchresults(xmlnode x) {
    xmlnode y, z;
    const char *jid, *nick, *first, *last, *email;
    icqcontact *c;

    if(!searchdest)
	return;

    if(y = xmlnode_get_tag(x, "query"))
    for(y = xmlnode_get_tag(y, "item"); y; y = xmlnode_get_nextsibling(y)) {
	jid = xmlnode_get_attrib(y, "jid");
	nick = first = last = email = 0;

	z = xmlnode_get_tag(y, "nick"); if(z) nick = xmlnode_get_data(z);
	z = xmlnode_get_tag(y, "first"); if(z) first = xmlnode_get_data(z);
	z = xmlnode_get_tag(y, "last"); if(z) last = xmlnode_get_data(z);
	z = xmlnode_get_tag(y, "email"); if(z) email = xmlnode_get_data(z);

	if(jid) {
	    c = new icqcontact(imcontact(jidnormalize(jid), proto));
	    icqcontact::basicinfo cb = c->getbasicinfo();

	    if(nick) {
		c->setnick(nick);
		c->setdispnick(c->getnick());
	    }

	    if(first) cb.fname = first;
	    if(last) cb.lname = last;
	    if(email) cb.email = email;
	    c->setbasicinfo(cb);

	    foundguys.push_back(c);

	    string line = (string) " " + c->getnick();
	    if(line.size() > 12) line.resize(12);
	    else line += string(12-line.size(), ' ');
	    line += " " + cb.fname + " " + cb.lname;
	    if(!cb.email.empty()) line += " <" + cb.email + ">";

	    searchdest->additem(conf->getcolor(cp_clist_jabber), c, line);
	}
    }

    face.findready();

    log(logSearchFinished, foundguys.size());

    searchdest->redraw();
    searchdest = 0;
}

void jabberhook::gotloggedin() {
    xmlnode x, y;

    flogged = true;

//  x = jutil_iqnew(JPACKET__GET, NS_AGENTS);
//  xmlnode_put_attrib(x, "id", "Agent List");
//  jab_send(jc, x);
//  xmlnode_free(x);

    char *server = strdup(jc->user->server);

    jhook.agents.push_back(agent(server, server, "", agent::atUnknown));

	char *id;
    x = jutil_iqnew(JPACKET__GET, NS_DISCOINFO);
	id = jab_getid(jc);
	ignore_ids.insert((string)id);
    xmlnode_put_attrib(x, "id", id);
    xmlnode_put_attrib(x, "to", server);
    jab_send(jc, x);
    xmlnode_free(x);

    x = jutil_iqnew(JPACKET__GET, NS_DISCOITEMS);
	id = jab_getid(jc);
	ignore_ids.insert((string)id);
	xmlnode_put_attrib(x, "id", id);
	xmlnode_put_attrib(x, "to", server);
    jab_send(jc, x);
    xmlnode_free(x);
    free(server);


    x = jutil_iqnew(JPACKET__GET, NS_ROSTER);
    xmlnode_put_attrib(x, "id", "Roster");
    jab_send(jc, x);
    xmlnode_free(x);
}

void jabberhook::gotroster(xmlnode x, bool login) {
    xmlnode y, z;
    imcontact ic;
    icqcontact *c;
    string grp;

    for(y = xmlnode_get_tag(x, "item"); y; y = xmlnode_get_nextsibling(y)) {
	const char *alias = xmlnode_get_attrib(y, "jid");
	const char *sub = xmlnode_get_attrib(y, "subscription");
	const char *name = xmlnode_get_attrib(y, "name");
	const char *group = 0;

	z = xmlnode_get_tag(y, "group");
	if(z) group = xmlnode_get_data(z);
	grp = group ? rusconv("uk", group) : "";

	if(alias) {
	    updatinguser = jidnormalize(alias);
	    ic = imcontact(jidtodisp(alias), proto);
	    clist.updateEntry(ic, grp);
	    updatinguser = "";

	    if(c = clist.get(ic)) {
		if(name) c->setdispnick(rusconv("uk", name)); else {
		    string u, h, r;
		    jidsplit(alias, u, h, r);
		    if (!u.empty())
			c->setdispnick(u);
		    else
			c->setdispnick(jidtodisp(alias));
		}
	    }

	    roster[jidnormalize(alias)] = grp;
	}
    }

    if (login)
	postlogin();
}

void jabberhook::postlogin() {
    int i;
    icqcontact *c;

    ourstatus = available;
    time(&timer_keepalive);

    log(logLogged);
    setautostatus(jhook.manualstatus);
    face.update();

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc().pname == proto)
	if(ischannel(c))
	if(c->getbasicinfo().requiresauth)
	    c->setstatus(available);
    }

    agents.insert(agents.begin(), agent("vcard", "Jabber VCard", "", agent::atStandard));
    agents.begin()->params[agent::ptRegister].enabled = true;

    string buf;
    ifstream f(conf->getconfigfname("jabber-infoset").c_str());

    if(f.is_open()) {
	icqcontact *c = clist.get(contactroot);

	c->clear();
	icqcontact::basicinfo bi = c->getbasicinfo();
	icqcontact::reginfo ri = c->getreginfo();

	ri.service = agents.begin()->name;
	getstring(f, buf); c->setnick(buf);
	getstring(f, buf); bi.email = buf;
	getstring(f, buf); bi.fname = buf;
	getstring(f, buf); bi.lname = buf;
	f.close();

	c->setbasicinfo(bi);
	c->setreginfo(ri);

	sendupdateuserinfo(*c);
	unlink(conf->getconfigfname("jabber-infoset").c_str());
    }
}

void jabberhook::conferencecreate(const imcontact &confid, const vector<imcontact> &lst) {
    char *jcid = strdup(confid.nickname.substr(1).c_str());
    xmlnode x = jutil_presnew(JPACKET__UNKNOWN, jcid, 0);
    jab_send(jc, x);
    xmlnode_free(x);
    free (jcid);
}

void jabberhook::vcput(xmlnode x, const string &name, const string &val) {
    xmlnode_insert_cdata(xmlnode_insert_tag(x, name.c_str()),
	jhook.rusconv("ku", val).c_str(), (unsigned int) -1);
}

void jabberhook::vcputphone(xmlnode x, const string &type, const string &place, const string &number) {
    xmlnode z = xmlnode_insert_tag(x, "TEL");
    vcput(z, type, "");
    vcput(z, place, "");
    vcput(z, "NUMBER", number);
}

void jabberhook::vcputaddr(xmlnode x, const string &place, const string &street,
const string &locality, const string &region, const string &pcode,
unsigned short country) {
    xmlnode z = xmlnode_insert_tag(x, "ADR");
    vcput(z, place, "");
    vcput(z, "STREET", street);
    vcput(z, "LOCALITY", locality);
    vcput(z, "REGION", region);
    vcput(z, "PCODE", pcode);
    vcput(z, "CTRY", getCountryIDtoString(country));
}

void jabberhook::vcputavatar(xmlnode x, const string &type, const string &val) {
    xmlnode z = xmlnode_insert_tag(x, "PHOTO");
    vcput(z, "TYPE", type);
    xmlnode_insert_cdata(xmlnode_insert_tag(z, "BINVAL"), val.c_str(), (unsigned int) -1);
}

void jabberhook::sendupdateuserinfo(const icqcontact &c) {
    xmlnode x, y, z;
    icqcontact::reginfo ri = c.getreginfo();
    string buf;
    char cbuf[64];

    vector<agent>::const_iterator ia = agents.begin();

    while(ia != agents.end()) {
	if(ia->name == ri.service) {
	    if(ia->type == agent::atStandard) {
		x = jutil_iqnew2(JPACKET__SET);//vCard w/o trash query tag in vcard
		xmlnode_put_attrib(x, "id", jab_getid(jc));
		y = xmlnode_insert_tag(x, "vCard");
		xmlnode_put_attrib(y, "xmlns", NS_VCARD);
		xmlnode_put_attrib(y, "version", "3.0");

		icqcontact::basicinfo bi = c.getbasicinfo();
		icqcontact::moreinfo mi = c.getmoreinfo();
		icqcontact::workinfo wi = c.getworkinfo();
		conf->setavatar(proto, bi.avatar); //saving avatar path to file

		vcput(y, "DESC", c.getabout());
		vcput(y, "EMAIL", bi.email);
		vcput(y, "URL", mi.homepage);
		vcput(y, "TITLE", wi.position);
		vcput(y, "AGE", i2str(mi.age));
		vcput(y, "NICKNAME", c.getnick());

		vcput(y, "GENDER",
		    mi.gender == genderMale ? "Male" :
		    mi.gender == genderFemale ? "Female" : "");

		if(mi.birth_year && mi.birth_month && mi.birth_day) {
		    snprintf(cbuf, sizeof(cbuf), "%04d-%02d-%02d", mi.birth_year, mi.birth_month, mi.birth_day);
		    vcput(y, "BDAY", cbuf);
		}

		if(!(buf = bi.fname).empty()) buf += " " + bi.lname;
		vcput(y, "FN", buf);

		z = xmlnode_insert_tag(y, "N");
		vcput(z, "GIVEN", bi.fname);
		vcput(z, "FAMILY", bi.lname);

		z = xmlnode_insert_tag(y, "ORG");
		vcput(z, "ORGNAME", wi.company);
		vcput(z, "ORGUNIT", wi.dept);

		vcputphone(y, "VOICE", "HOME", bi.phone);
		vcputphone(y, "FAX", "HOME", bi.fax);
		vcputphone(y, "VOICE", "WORK", wi.phone);
		vcputphone(y, "FAX", "WORK", wi.fax);

		vcputaddr(y, "HOME", bi.street, bi.city, bi.state, bi.zip, bi.country);
		vcputaddr(y, "WORK", wi.street, wi.city, wi.state, wi.zip, wi.country);

		vcput(y, "HOMECELL", bi.cellular);
		vcput(y, "WORKURL", wi.homepage);

		string avatar, image_type;
		if( get_base64_avatar(image_type, avatar) )
			vcputavatar(y, image_type, avatar);

	    } else {
		x = jutil_iqnew(JPACKET__SET, NS_REGISTER);
		xmlnode_put_attrib(x, "id", "Register");
		y = xmlnode_get_tag(x, "query");

		xmlnode_put_attrib(x, "to", ia->jid.c_str());
		xmlnode_insert_cdata(xmlnode_insert_tag(y, "key"),
		    ia->params[agent::ptRegister].key.c_str(), (unsigned int) -1);

		vector<pair<string, string> >::const_iterator ip = ri.params.begin();
		while(ip != ri.params.end()) {
		    xmlnode_insert_cdata(xmlnode_insert_tag(y,
			ip->first.c_str()), ip->second.c_str(), (unsigned int) -1);
		    ++ip;
		}

	    }

	    jab_send(jc, x);
	    xmlnode_free(x);
	    break;
	}
	++ia;
    }
}

void jabberhook::gotmessage(const string &type, const string &from, const string &abody, const string &enc) {
    string u, h, r, body(abody);
    jidsplit(from, u, h, r);

    imcontact ic(jidtodisp(from), proto), chic;

    if(clist.get(chic = imcontact((string) "#" + ic.nickname, proto))) {
	ic = chic;
	if(!r.empty()) body.insert(0, r + ": ");
    }

#ifdef HAVE_GPGME
    icqcontact *c = clist.get(ic);

    if(c) {
	if(!enc.empty()) {
	    c->setusepgpkey(true);
	    if(pgp.enabled(proto)) {
		body = pgp.decrypt(enc, proto);
		if (body.empty()) // probably decryption error, store at least encrypted message
		    body = enc;
	    }
	    else c->setusepgpkey(false);
	} else {
	    c->setusepgpkey(false);
	}
    }
#endif
    
#ifdef HAVE_LIBOTR
    if (!otr.receive_message(proto, from, body)) return;
#endif

    em.store(immessage(ic, imevent::incoming, rusconv("uk", body)));
}

void jabberhook::updatecontact(icqcontact *c) {
    xmlnode x, y;

    if(logged()) {
	if (jidnormalize(c->getdesc().nickname) == updatinguser)
	    return;
	char *cjid = strdup(jidnormalize(c->getdesc().nickname).c_str());
        char *cname = strdup(rusconv("ku", c->getdispnick()).c_str());

	x = jutil_iqnew(JPACKET__SET, NS_ROSTER);
	xmlnode_put_attrib(x, "id", jab_getid(jc));
	y = xmlnode_insert_tag(xmlnode_get_tag(x, "query"), "item");
	xmlnode_put_attrib(y, "jid", cjid);
	xmlnode_put_attrib(y, "name", cname);

	vector<icqgroup>::const_iterator ig = find(groups.begin(), groups.end(), c->getgroupid());
	if(ig != groups.end()) {
	    y = xmlnode_insert_tag(y, "group");
	    xmlnode_insert_cdata(y, rusconv("ku", ig->getname()).c_str(), (unsigned) -1);
	}

	jab_send(jc, x);
	xmlnode_free(x);
	free (cjid);
	free (cname);
    }
}

void jabberhook::gotvcard(const imcontact &ic, xmlnode v) {
    xmlnode ad, n;
    char *p;
    string name, data;
    bool wasrole = false;

    icqcontact *c = clist.get(ic);
    if(!c || isourid(ic.nickname)) c = clist.get(contactroot);

    if(c) {
	icqcontact::basicinfo bi = c->getbasicinfo();
	icqcontact::moreinfo mi = c->getmoreinfo();
	icqcontact::workinfo wi = c->getworkinfo();

	for(ad = xmlnode_get_firstchild(v); ad; ad = xmlnode_get_nextsibling(ad)) {
	    p = xmlnode_get_name(ad); name = p ? up(p) : "";
	    p = xmlnode_get_data(ad); data = p ? rusconv("uk", p) : "";

	    if(name == "NICKNAME") c->setnick(data); else
	    if(name == "DESC") c->setabout(data); else
	    if(name == "EMAIL") bi.email = data; else
	    if(name == "URL") mi.homepage = data; else
	    if(name == "AGE") mi.age = atoi(data.c_str()); else
	    if(name == "HOMECELL") bi.cellular = data; else
	    if(name == "WORKURL") wi.homepage = data; else
	    if(name == "GENDER") {
		if(data == "Male") mi.gender = genderMale; else
		if(data == "Female") mi.gender = genderFemale; else
		    mi.gender = genderUnspec;
	    } else
	    if(name == "TITLE" || name == "ROLE") {
		if(!wasrole) {
		    wasrole = true;
		    wi.position = "";
		}

		if(!wi.position.empty()) wi.position += " / ";
		wi.position += data;
	    } else
	    if(name == "FN") {
		bi.fname = getword(data);
		bi.lname = data;
	    } else
	    if(name == "BDAY") {
		mi.birth_year = atoi(getword(data, "-").c_str());
		mi.birth_month = atoi(getword(data, "-").c_str());
		mi.birth_day = atoi(getword(data, "-").c_str());
	    } else
	    if(name == "ORG") {
		if(p = xmlnode_get_tag_data(ad, "ORGNAME")) wi.company = rusconv("uk", p);
		if(p = xmlnode_get_tag_data(ad, "ORGUNIT")) wi.dept = rusconv("uk", p);
	    } else
	    if(name == "N") {
		if(p = xmlnode_get_tag_data(ad, "GIVEN")) bi.fname = rusconv("uk", p);
		if(p = xmlnode_get_tag_data(ad, "FAMILY")) bi.lname = rusconv("uk", p);
	    } else
	    if(name == "ADR") {
		if(xmlnode_get_tag(ad, "HOME")) {
		    if(p = xmlnode_get_tag_data(ad, "STREET")) bi.street = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "LOCALITY")) bi.city = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "REGION")) bi.state = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "PCODE")) bi.zip = rusconv("uk", p);

		    if((p = xmlnode_get_tag_data(ad, "CTRY"))
		    || (p = xmlnode_get_tag_data(ad, "COUNTRY")))
			bi.country = getCountryByName(p);

		} else if(xmlnode_get_tag(ad, "WORK")) {
		    if(p = xmlnode_get_tag_data(ad, "STREET")) wi.street = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "LOCALITY")) wi.city = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "REGION")) wi.state = rusconv("uk", p);
		    if(p = xmlnode_get_tag_data(ad, "PCODE")) wi.zip = rusconv("uk", p);

		    if((p = xmlnode_get_tag_data(ad, "CTRY"))
		    || (p = xmlnode_get_tag_data(ad, "COUNTRY")))
			wi.country = getCountryByName(p);
		}
	    } else
	    if(name == "TEL") {
		if(p = xmlnode_get_tag_data(ad, "NUMBER")) {
		    if(xmlnode_get_tag(ad, "VOICE")) {
			if(xmlnode_get_tag(ad, "HOME")) bi.phone = rusconv("uk", p); else
			if(xmlnode_get_tag(ad, "WORK")) wi.phone = rusconv("uk", p);

		    } else if(xmlnode_get_tag(ad, "FAX")) {
			if(xmlnode_get_tag(ad, "HOME")) bi.fax = rusconv("uk", p); else
			if(xmlnode_get_tag(ad, "WORK")) wi.fax = rusconv("uk", p);
		    }
		}
	    }
	 else if( name == "PHOTO" )//get and write user avatar
	    {
		    if(!isourid(ic.nickname)) {
			    string contact_dir = c->getdirname() + "avatar";
			    if(p = xmlnode_get_tag_data(ad, "TYPE"))
			    {

				    string ext;
				    if( get_img_ext(p, ext) )
				    {
					    contact_dir += (string)"." + ext;
					    if(p = xmlnode_get_tag_data(ad, "BINVAL")) {
						    int len;
						    char *ptr = base64_decode( p, &len );
						    if( ptr )
						    {
							    int ggg = open(contact_dir.c_str(), O_CREAT | O_WRONLY | O_TRUNC,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
							    if( ggg >= 0 )
							    {
								    write(ggg, ptr, len);
								    close(ggg);
								    if( justpathname( bi.avatar ).empty()  )
									    bi.avatar = c->getdirname() + bi.avatar;
								    if( (contact_dir != bi.avatar) && !bi.avatar.empty() ) //clear old avatar
									    unlink(bi.avatar.c_str());
								    bi.avatar = "avatar." + ext;
							    }
							    free(ptr);
						    }
					    }
				    }
			    }
		    }
	    }
	}

	if(isourid(ic.nickname)) {//fill configuration window 
		map<string, string> add = conf->getourid(proto).additional;
		if(!add["avatar"].empty()) {
			bi.avatar = add["avatar"];
		}
	}
	c->setbasicinfo(bi);
	c->setmoreinfo(mi);
	c->setworkinfo(wi);

	if(isourid(ic.nickname)) {
	    icqcontact *cc = clist.get(ic);
	    if(cc) {
		cc->setnick(c->getnick());
		cc->setabout(c->getabout());
		cc->setbasicinfo(bi);
		cc->setmoreinfo(mi);
		cc->setworkinfo(wi);
	    }
	}
    }
    xmlnode_free(v); //without it very HUGE memory leaks on some accounts
}

void jabberhook::requestversion(const imcontact &ic)
{
	string cjid_str = ic.nickname;
	transform(cjid_str.begin(), cjid_str.end(), cjid_str.begin(), ::tolower); 
	 
	xmlnode x = jutil_iqnew(JPACKET__GET, NS_VERSION);
	 
	xmlnode_put_attrib(x, "to", jhook.full_jids[cjid_str].c_str());
	xmlnode_put_attrib(x, "id", "versionreq");
	xmlnode_put_attrib(x, "from", getourjid().c_str());
	jab_send(jc, x);
	xmlnode_free(x);
}
 
void jabberhook::sendversion(const imcontact &ic, xmlnode i) {
	string id;
	char *p = xmlnode_get_attrib(i, "id"); if(p) id = p; else id = "versionreq";
	string cjid_str = ic.nickname;
	transform(cjid_str.begin(), cjid_str.end(), cjid_str.begin(), ::tolower); 
	const char *cjid = jhook.full_jids[cjid_str].c_str();
	xmlnode x = jutil_iqnew(JPACKET__RESULT, NS_VERSION), y;
	xmlnode_put_attrib(x, "to", cjid);

	xmlnode_put_attrib(x, "from", getourjid().c_str());
	xmlnode_put_attrib(x, "id", id.c_str() );
	y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "name");
	xmlnode_insert_cdata(y, PACKAGE, (unsigned) -1 );
	y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "version"); 
	xmlnode_insert_cdata(y, centerim::version, (unsigned) -1 );
#ifdef HAVE_UNAME
	if(conf->getourid(jhook.proto).additional["osinfo"] == "1")
	{
		struct utsname buf;
		if( !uname( &buf ) )
		{
			string os = buf.sysname;
			os += " ";
			os += buf.release;
			y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "os");
			xmlnode_insert_cdata(y, os.c_str(), (unsigned) -1 );
		}
	}
#endif
	jab_send(jc, x);
	xmlnode_free(x);
}

void jabberhook::gotversion(const imcontact &ic, xmlnode x) { //fix version parsing
    xmlnode y = xmlnode_get_tag(x, "query"), z;
    char *p;
    string vinfo;
    if(y) {
	if(z = xmlnode_get_tag(y, "name"))
	if(p = xmlnode_get_data(z))
	{
	    if(p) vinfo = rusconv("uk", p);
	}

	if(z = xmlnode_get_tag(y, "version"))
	if(p = xmlnode_get_data(z)) {
	    if(!vinfo.empty()) vinfo += ", ";
	    vinfo += rusconv("uk", p);
	}

	if(z = xmlnode_get_tag(y, "os"))
	if(p = xmlnode_get_data(z)) {
	    if(!vinfo.empty()) vinfo += " / ";
	    vinfo += rusconv("uk", p);
	}

	char buf[NOTIFBUF];
	snprintf(buf, NOTIFBUF, _("The remote is using %s"), vinfo.c_str());
	em.store(imnotification(ic, buf));
    }
}

void jabberhook::senddiscoinfo(const imcontact &ic, xmlnode i) {
	string id;
	char *p = xmlnode_get_attrib(i, "id"); if(p) id = p; else id = "discoinfo";
	string cjid_str = ic.nickname;
	transform(cjid_str.begin(), cjid_str.end(), cjid_str.begin(), ::tolower); 
	const char *cjid = jhook.full_jids[cjid_str].c_str();
	xmlnode x = jutil_iqnew(JPACKET__RESULT, NS_DISCOINFO), y;
	xmlnode_put_attrib(x, "to", cjid);
	xmlnode_put_attrib(x, "from", getourjid().c_str());
	xmlnode_put_attrib(x, "id", id.c_str() );

	y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "identity");
	xmlnode_put_attrib(y, "category", "client");
	xmlnode_put_attrib(y, "type", "pc");
	xmlnode_put_attrib(y, "name", "centerim");

	y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "feature"); 
	xmlnode_put_attrib(y, "var", NS_DISCOINFO);

	y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "feature"); 
	xmlnode_put_attrib(y, "var", NS_VERSION);

	y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "feature"); 
	xmlnode_put_attrib(y, "var", NS_VCARD);
	
	y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "feature"); 
	xmlnode_put_attrib(y, "var", NS_VCARDUP);
	
	y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "feature"); 
	xmlnode_put_attrib(y, "var", NS_OOB );

	y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "feature"); 
	xmlnode_put_attrib(y, "var", NS_BYTESTREAMS );

	y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "feature"); 
	xmlnode_put_attrib(y, "var", NS_SI );

	y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "feature"); 
	xmlnode_put_attrib(y, "var", NS_SIFILE );

	if(conf->getourid(jhook.proto).additional["acknowledgements"] == "1") {
		y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "feature"); 
		xmlnode_put_attrib(y, "var", NS_RECEIPTS);
	}

	jab_send(jc, x);
	xmlnode_free(x);
}

bool jabberhook::isourid(const string &jid) {
    icqconf::imaccount acc = conf->getourid(jhook.proto);
    int pos;

    string ourjid = acc.nickname;
    if(ourjid.find("@") == -1) ourjid += (string) "@" + acc.server;
    if((pos = ourjid.find(":")) != -1) ourjid.erase(pos);

    return jidnormalize(jid) == ourjid;
}

string jabberhook::getourjid() {
    icqconf::imaccount acc = conf->getourid(jhook.proto);
    string jid = acc.nickname;
    int pos;

    if(jid.find("@") == -1)
	jid += (string) "@" + acc.server;

    if((pos = jid.find(":")) != -1)
	jid.erase(pos);

    if(jid.find("/") == -1)
	jid += "/centerim" + jhook.uuid;

    return jid;
}

imstatus jabberhook::process_presence(string id, string s, int prio, imstatus ust)
{
	if (statuses.find(id) == statuses.end()) { // new and only presence
		if (ust != offline)
			(statuses[id])[s] = pair<int, imstatus>(prio, ust);
	} else {
		if (statuses[id].find(s) == statuses[id].end()) { // unknown resource
			if (ust != offline)
				(statuses[id])[s] = pair<int, imstatus>(prio, ust);
		} else {
			if (ust == offline) // remove resource
				(statuses[id]).erase(s);
			else { // known contact
				(statuses[id])[s] = pair<int, imstatus>(prio, ust);
			}
		}
		ust = get_presence(statuses[id]);
	}
	return ust;
}

// ----------------------------------------------------------------------------

void jabberhook::statehandler(jconn conn, int state) {
    static int previous_state = -1;

    switch(state) {
	case JCONN_STATE_OFF:
	    jhook.flogged = jhook.fonline = false;

	    if(previous_state != JCONN_STATE_OFF) {
		logger.putourstatus(jhook.proto, jhook.getstatus(), jhook.ourstatus = offline);
		jhook.log(logDisconnected);
		jhook.roster.clear();
		jhook.agents.clear();
		jhook.ignore_ids.clear();
		clist.setoffline(jhook.proto);
		face.update();
	    }
	    break;

	case JCONN_STATE_CONNECTED:
	    break;

	case JCONN_STATE_AUTH:
	    break;

	case JCONN_STATE_ON:
	    if(jhook.regmode) jhook.fonline = true;
	    break;

	default:
	    break;
    }

    previous_state = state;
}

void jabberhook::packethandler(jconn conn, jpacket packet) {
    char *p;
    xmlnode x, y;
    string from, type, body, enc, ns, id, u, h, s, packet_id;
    imstatus ust;
    int npos;
    bool isagent;

    jpacket_reset(packet);

    p = xmlnode_get_attrib(packet->x, "from"); if(p) from = p;
    p = xmlnode_get_attrib(packet->x, "type"); if(p) type = p;
	p = xmlnode_get_attrib(packet->x, "id"); if (p) packet_id = p;
	
    imcontact ic(jidtodisp(from), jhook.proto);

    switch(packet->type) {
	case JPACKET_MESSAGE:
	    x = xmlnode_get_tag(packet->x, "body");
	    p = xmlnode_get_data(x); if(p) body = p;

	    if(x = xmlnode_get_tag(packet->x, "subject"))
	    if(p = xmlnode_get_data(x))
		body = (string) p + ": " + body;

	    /* there can be multiple <x> tags. we're looking for one with
	       xmlns = jabber:x:encrypted */

	    for(x = xmlnode_get_firstchild(packet->x); x; x = xmlnode_get_nextsibling(x)) {
		if((p = xmlnode_get_name(x)) && !strcmp(p, "x"))
		if((p = xmlnode_get_attrib(x, "xmlns")) && !strcasecmp(p, "jabber:x:encrypted"))
		if(p = xmlnode_get_data(x)) {
		    enc = p;
		    break;
		}
	    }

	    if(!body.empty())
		jhook.gotmessage(type, from, body, enc);

		if(jhook.getstatus() != invisible) {
	    	if(x = xmlnode_get_tag(packet->x, "request"))
			if(NSCHECK(x, NS_RECEIPTS)) {
				const char *id = xmlnode_get_attrib(packet->x, "id");
				xmlnode receipt = jutil_receiptnew(from.c_str(), id);
				jab_send(conn, receipt);
				xmlnode_free(receipt);
				icqcontact *c = clist.get(ic);
				c->sentAcks.insert(em.getevents(ic, 1).back()->getsenttimestamp());
				face.relaxedupdate();
				if(c->isopenedforchat())
					c->sethasevents(true);
			}
		}

		if(x = xmlnode_get_tag(packet->x, "received"))
		{
			if(p = xmlnode_get_attrib(packet->x, "id"))
			{
				icqcontact *c = clist.get(ic);
				c->receivedAcks.insert(strtoul(p, 0, 0));
				face.relaxedupdate();
				if(c->isopenedforchat())
					c->sethasevents(true);
			}
		}

	    break;

	case JPACKET_IQ:
	    if(type == "result") {
		if(p = xmlnode_get_attrib(packet->x, "id")) {
		    int iid = atoi(p);

		    if(iid == jhook.id) {
			if(!jhook.regmode) {
			    if(jhook.jstate == STATE_GETAUTH) {
				if(x = xmlnode_get_tag(packet->x, "query"))
				if(!xmlnode_get_tag(x, "digest")) {
				    jhook.jc->sid = 0;
				}

				jhook.id = atoi(jab_auth(jhook.jc));
				jhook.jstate = STATE_SENDAUTH;

			    } else {
				jhook.gotloggedin();
				jhook.jstate = STATE_LOGGED;
			    }

			} else {
			    jhook.regdone = true;

			}
			return;
		    }

		    if(!strcmp(p, "VCARDreq")) {
			x = xmlnode_get_firstchild(packet->x);
			if(!x) x = packet->x;

			jhook.gotvcard(ic, x);
			return;

		    } else if(!strcmp(p, "versionreq")) {
			jhook.gotversion(ic, packet->x);
			return;

		    }
		}

		if(x = xmlnode_get_tag(packet->x, "query")) {
		    p = xmlnode_get_attrib(x, "xmlns"); if(p) ns = p;

		    if(ns == NS_ROSTER) {
			jhook.gotroster(x, true);

		    } else if(ns == NS_AGENTS) {
			for(y = xmlnode_get_tag(x, "agent"); y; y = xmlnode_get_nextsibling(y)) {
			    const char *alias = xmlnode_get_attrib(y, "jid");

			    if(alias) {
				const char *name = xmlnode_get_tag_data(y, "name");
				const char *desc = xmlnode_get_tag_data(y, "description");
				const char *service = xmlnode_get_tag_data(y, "service");
				agent::agent_type atype = agent::atUnknown;

				if(xmlnode_get_tag(y, "groupchat")) atype = agent::atGroupchat; else
				if(xmlnode_get_tag(y, "transport")) atype = agent::atTransport; else
				if(xmlnode_get_tag(y, "search")) atype = agent::atSearch;

				if(alias && name && desc) {
				    jhook.agents.push_back(agent(alias, name, desc, atype));

				    if(atype == agent::atSearch) {
					x = jutil_iqnew (JPACKET__GET, NS_SEARCH);
					xmlnode_put_attrib(x, "to", alias);
					xmlnode_put_attrib(x, "id", "Agent info");
					jab_send(conn, x);
					xmlnode_free(x);
				    }

				    if(xmlnode_get_tag(y, "register")) {
					x = jutil_iqnew (JPACKET__GET, NS_REGISTER);
					xmlnode_put_attrib(x, "to", alias);
					xmlnode_put_attrib(x, "id", "Agent info");
					jab_send(conn, x);
					xmlnode_free(x);
				    }
				}
			    }
			}

			if(find(jhook.agents.begin(), jhook.agents.end(), DEFAULT_CONFSERV) == jhook.agents.end())
			    jhook.agents.insert(jhook.agents.begin(), agent(DEFAULT_CONFSERV, DEFAULT_CONFSERV,
				_("Default Jabber conference server"), agent::atGroupchat));

		    } else if(ns == NS_SEARCH || ns == NS_REGISTER) {
			p = xmlnode_get_attrib(packet->x, "id"); id = p ? p : "";

			if(id == "Agent info") {
			    jhook.gotagentinfo(packet->x);
			} else if(id == "Lookup") {
			    jhook.gotsearchresults(packet->x);
			} else if(id == "Register") {
			    x = jutil_iqnew(JPACKET__GET, NS_REGISTER);
			    xmlnode_put_attrib(x, "to", from.c_str());
			    xmlnode_put_attrib(x, "id", "Agent info");
			    jab_send(conn, x);
			    xmlnode_free(x);
			} else if (ns == NS_SEARCH) {
			    jhook.gotagentinfo(packet->x);
			}
		    } else if(ns == NS_DISCOITEMS) {
			for(y = xmlnode_get_tag(x, "item"); y; y = xmlnode_get_nextsibling(y)) {
			    if (p = xmlnode_get_attrib(y, "jid")) {
				jhook.agents.push_back(agent(p, p, _(""), agent::atUnknown));
				x = jutil_iqnew(JPACKET__GET, NS_DISCOINFO);
				char *pid = jab_getid(conn);
				jhook.ignore_ids.insert((string)pid);
				xmlnode_put_attrib(x, "id", pid);
				xmlnode_put_attrib(x, "to", p);
				jab_send(conn, x);
				xmlnode_free(x);
			    }
			}
		    }
		    else if(ns == NS_DISCOINFO) {
			jhook.gotagentinfo(packet->x);
		    }
		}

	    } else if(type == "get") {
		    if( x = xmlnode_get_tag(packet->x, "query") )
			    if( p = xmlnode_get_attrib( x, "xmlns" ) )
			    {		    
				    if(!strcmp(p, NS_VERSION)) { //user request our version
					    jhook.full_jids[ic.nickname] = from;
					    jhook.sendversion(ic, packet->x);
					    return;
				    }

					// send the supported features (and don't leak our presence)
					if(!strcmp(p, NS_DISCOINFO) && jhook.getstatus() != invisible) {
						jhook.full_jids[ic.nickname] = from;
						jhook.senddiscoinfo(ic, packet->x);
						return;
					}
			    }
			// unknown IQ get without specific response
			x = jutil_iqnew2(JPACKET__ERROR);
			xmlnode_put_attrib(x, "to", from.c_str());
			xmlnode_put_attrib(x, "id", packet_id.c_str());
			y = xmlnode_insert_tag(x, "error");
			xmlnode_put_attrib(y, "type", "cancel");
			xmlnode z = xmlnode_insert_tag(y, "feature-not-implemented");
			xmlnode_put_attrib(z, "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas");
			jab_send(conn, x);
			xmlnode_free(x);
	    } else if(type == "set") {
		    if( x = xmlnode_get_tag(packet->x, "query") )
		    {
			    if( p = xmlnode_get_attrib( x, "xmlns" ) )
			    {		    
				    if(!strcmp(p, NS_BYTESTREAMS)) {
					    jhook.full_jids[ic.nickname] = from;
#ifdef HAVE_THREAD
					    jhook.getfile_byte(ic, packet->x);
#endif
					    return;
				    }
				    else if(!strcmp(p, NS_OOB)) {
					    jhook.full_jids[ic.nickname] = from;
#ifdef HAVE_THREAD
					    jhook.getfile_http(ic, packet->x);
#endif
					    return;
				    }
				    else if (!strcmp(p, NS_ROSTER)) {
					jhook.gotroster(x, false);
					x = jutil_iqnew2(JPACKET__ERROR);
					xmlnode_put_attrib(x, "to", from.c_str());
					xmlnode_put_attrib(x, "id", packet_id.c_str());
					xmlnode_put_attrib(x, "type", "result");
					jab_send(conn, x);
					xmlnode_free(x);
					break;
				    }
			    }
		    }
		    else if( x = xmlnode_get_tag(packet->x, "si") )
			    if( p = xmlnode_get_attrib( x, "xmlns" ) )
			    {		    
				    if(!strcmp(p, NS_SI)) {
					    if( p = xmlnode_get_attrib( x, "profile" ) )
					    {
						    if(!strcmp(p, NS_SIFILE)) {
							    {
								    jhook.full_jids[ic.nickname] = from;
#ifdef HAVE_THREAD
								    jhook.file_transfer_request(ic, packet->x);
#endif
								    return;
							    }
						    }
					    }
				    }
			    }
		// unknown IQ set without specific response
		x = jutil_iqnew2(JPACKET__ERROR);
		xmlnode_put_attrib(x, "to", from.c_str());
		xmlnode_put_attrib(x, "id", packet_id.c_str());
		y = xmlnode_insert_tag(x, "error");
		xmlnode_put_attrib(y, "type", "cancel");
		xmlnode z = xmlnode_insert_tag(y, "feature-not-implemented");
		xmlnode_put_attrib(z, "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas");
		jab_send(conn, x);
		xmlnode_free(x);

	    } else if(type == "error") {
		string name, desc;
		int code;

		x = xmlnode_get_tag(packet->x, "error");
		p = xmlnode_get_attrib(x, "code"); if(p) code = atoi(p);
		p = xmlnode_get_attrib(x, "id"); if(p) name = p;
		p = xmlnode_get_tag_data(packet->x, "error"); if(p) desc = p;

		set<string>::iterator sit;
		if ((sit = jhook.ignore_ids.find(packet_id)) != jhook.ignore_ids.end()) { // expected error, ignore
			jhook.ignore_ids.erase(sit);
			break;
		}
		
		switch(code) {
		    case 501: /* Not Implemented */
		        if(jhook.regmode) {
		            jhook.regerr = desc;
		        }
		        break;
		    case 401: /* Unauthorized */
		    case 302: /* Redirect */
		    case 400: /* Bad request */
		    case 402: /* Payment Required */
		    case 403: /* Forbidden */
		    case 404: /* Not Found */
		    case 405: /* Not Allowed */
		    case 406: /* Not Acceptable */
		    case 407: /* Registration Required */
		    case 408: /* Request Timeout */
		    case 409: /* Conflict */
		    case 500: /* Internal Server Error */
		    case 502: /* Remote Server Error */
		    case 503: /* Service Unavailable */
		    case 504: /* Remote Server Timeout */
		    default:
			if(!jhook.regmode) {
			    face.log(desc.empty() ?
				_("+ [jab] error %d") :
				_("+ [jab] error %d: %s"),
				code, desc.c_str());

			    if(!jhook.flogged) {
				close(jhook.jc->fd);
				jhook.jc->fd = -1;
			    }

			} else {
			    jhook.regerr = desc;

			}
		}

	    }
	    break;

	case JPACKET_PRESENCE:
	    if (type == "error")
	    {
		if(!jhook.regmode) {
		    string desc;
		    int code;
		    x = xmlnode_get_tag(packet->x, "error");
		    p = xmlnode_get_attrib(x, "code"); if(p) code = atoi(p);
		    p = xmlnode_get_tag_data(packet->x, "error"); if(p) desc = p;
		    face.log(desc.empty() ?
			_("+ [jab] error %d") :
			_("+ [jab] error %d: %s"),
			code, desc.c_str());
		}
		break;
	    }
	    x = xmlnode_get_tag(packet->x, "show");
	    ust = available;

	    if(x) {
		p = xmlnode_get_data(x); if(p) ns = p;

		if(!ns.empty()) {
		    if(ns == "away") ust = away; else
		    if(ns == "dnd") ust = dontdisturb; else
		    if(ns == "xa") ust = notavail; else
		    if(ns == "chat") ust = freeforchat;
		}
	    }

	    if(type == "unavailable")
		ust = offline;

	    jidsplit(from, u, h, s);
	    id = jidtodisp(from);
	    jhook.full_jids[ic.nickname] = from; //writing full JID

	    if(clist.get(imcontact((string) "#" + id, jhook.proto))) {
		if(ust == offline) {
		    vector<string>::iterator im = find(jhook.chatmembers[id].begin(), jhook.chatmembers[id].end(), s);
		    if(im != jhook.chatmembers[id].end())
			jhook.chatmembers[id].erase(im);

		    jhook.full_jids.erase(ic.nickname); //erase full JID if user change status to offline

			vector<agent>::iterator ia = find(jhook.agents.begin(), jhook.agents.end(), from);
			if(ia != jhook.agents.end())
			jhook.agents.erase(ia);
		} else {
		    vector<string>::iterator im = find(jhook.chatmembers[id].begin(), jhook.chatmembers[id].end(), s);
		    if(im == jhook.chatmembers[id].end()) {
			jhook.chatmembers[id].push_back(s);
			sort(jhook.chatmembers[id].begin(),jhook.chatmembers[id].end());
		    }
		}

	    } else {
		icqcontact *c = clist.get(ic);
		
		if(c) {
			int prio = jutil_priority(packet->x); // priority
			ust = jhook.process_presence(id, s, prio, ust);
		    if(c->getstatus() != ust) {
			jhook.awaymsgs[ic.nickname] = "";
			logger.putonline(c, c->getstatus(), ust);
			if(c->getstatus() == offline)
			{
				x = jutil_iqnew(JPACKET__GET, NS_DISCOINFO);
				xmlnode_put_attrib(x, "id", jab_getid(conn));
				xmlnode_put_attrib(x, "to", from.c_str());
				jab_send(conn, x);
				xmlnode_free(x);
				jhook.agents.push_back(agent(from, "", "", agent::atUnknown));
			}
			c->setstatus(ust);
		    }

		    if(x = xmlnode_get_tag(packet->x, "status"))
		    if(p = xmlnode_get_data(x))
			jhook.awaymsgs[ic.nickname] = p;

		     
/*This code checking if user in your contacl list send update avatar request(presence message with another hash)*/
		    xmlnode y; 
		    if(x = xmlnode_get_tag(packet->x, "x"))
			    if(p = xmlnode_get_attrib(x, "xmlns"))
				    if(!strcmp(p,  NS_VCARDUP))
					    if( y = xmlnode_get_tag(x, "photo")) //quering avatar hash
							    if(p = xmlnode_get_data(y))
							    {
								    icqcontact::basicinfo bi = c->getbasicinfo();
								    string temp_av = bi.avatar;
								    if( justpathname( temp_av ).empty()  )
									    temp_av = c->getdirname() + bi.avatar;
								    string contact_dir = temp_av;
								    if( !contact_dir.empty() )
								    {
									    int ava_file = open( contact_dir.c_str(), O_RDONLY );
									    if( ava_file >= 0 )
									    {
										    struct stat buf;
										    fstat( ava_file, &buf );
										    long int file_size = buf.st_size;
										    char *hash = (char*)calloc( 1, file_size+1 );
										    read( ava_file, hash, file_size );
										    close( ava_file );
										    unsigned char hashval[20];
										    char *pos;
										    char final[41];
										    shaBlock((unsigned char *)hash, file_size, hashval);
										    pos = final;
										    int k;
										    for(k=0;k<20;k++)
										    {
											    snprintf(pos, 3, "%02x", hashval[k]);
											    pos += 2;
										    }
										    if( strcmp( p, final ) != 0 )
											    jhook.requestinfo(c);
										    free(hash);
									    }
									    else
										    jhook.requestinfo(c);
								    }
							    }
		     

#ifdef HAVE_GPGME
		    if(x = xmlnode_get_tag(packet->x, "x"))
		    if(p = xmlnode_get_attrib(x, "xmlns"))
		    if((string) p == "jabber:x:signed")
		    if(p = xmlnode_get_data(x))
			c->setpgpkey(pgp.verify(p, jhook.awaymsgs[ic.nickname]));
#endif
		}
	    }
	    break;

	case JPACKET_S10N:
	    isagent = find(jhook.agents.begin(), jhook.agents.end(), from) != jhook.agents.end();

	    if(type == "subscribe") {
		if(!isagent) {
		    em.store(imauthorization(ic, imevent::incoming,
			imauthorization::Request, _("The user wants to subscribe to your network presence updates")));

		} else {
		    char *cfrom = strdup(from.c_str());
		    x = jutil_presnew(JPACKET__SUBSCRIBED, cfrom, 0);
		    jab_send(jhook.jc, x);
		    xmlnode_free(x);
		    free (cfrom);
		}

	    } else if(type == "unsubscribe") {
		char *cfrom = strdup(from.c_str());
		x = jutil_presnew(JPACKET__UNSUBSCRIBED, cfrom, 0);
		jab_send(jhook.jc, x);
		xmlnode_free(x);
		em.store(imnotification(ic, _("The user has removed you from his contact list (unsubscribed you, using the Jabber language)")));
		free (cfrom);

	    }

	    break;

	default:
	    break;
    }
	
	set<string>::iterator idit;
	if ((idit = jhook.ignore_ids.find(packet_id)) != jhook.ignore_ids.end()) { // erase ignores for packets we've received
		jhook.ignore_ids.erase(idit);
	}
}

void jabberhook::jlogger(jconn conn, int inout, const char *p) {
    string tolog = (string) (inout ? "[IN]" : "[OUT]") + "\n";
    tolog += p;
    face.log(tolog);
}

bool jabberhook::get_img_ext(const string &type, string &ext) {
	int pos;

	if(type.find("image/") != -1)  {
		if((pos = type.find("/")) != -1) {
			ext = type.substr(pos+1);
			return true;
		}
	}
	ext = (string)"";
	return false;

}

bool jabberhook::get_base64_avatar(string &type, string &ava) {
	 
	icqcontact *ic = clist.get(contactroot);
	icqcontact::basicinfo bi = ic->getbasicinfo();
	string contact_dir = conf->getdirname();
	 
	if(!bi.avatar.empty())
	{
		if( justpathname( bi.avatar ).empty()  )
			contact_dir += bi.avatar;
		else
			contact_dir = bi.avatar;
		string image_type = contact_dir;
		int pos;
		if( (pos = image_type.rfind( ".")) != -1 )
		{
			type += (string)"image/" + image_type.substr(pos+1);
			int avatar_file = open( contact_dir.c_str(), O_RDONLY );
			if( avatar_file >= 0 )
			{
				struct stat buf;
				fstat( avatar_file, &buf );
				long int file_size = buf.st_size;
				char *avatar_stream = (char*)calloc( 1, file_size+1 );
				read( avatar_file, avatar_stream, file_size );
				close( avatar_file );
				char *temp = base64_encode(avatar_stream, file_size);
				ava = temp;
				free(temp);
				free(avatar_stream);
				return true;

			}
		}
	}
	return false;
}

bool jabberhook::get_my_avatar_hash(string &my_hash)
{
	icqcontact *ic = clist.get(contactroot);
	icqcontact::basicinfo bi = ic->getbasicinfo();
	string contact_dir = conf->getdirname();

	if(!bi.avatar.empty())
	{
		if( justpathname( bi.avatar ).empty()  )
			contact_dir += bi.avatar;
		else
			contact_dir = bi.avatar;
		int ava_file = open( contact_dir.c_str(), O_RDONLY );

		if( ava_file >= 0 )
		{
			struct stat buf;
			fstat( ava_file, &buf );
			long int file_size = buf.st_size;
			char *hash = (char*)calloc( 1, file_size+1 );
			read( ava_file, hash, file_size );
			close( ava_file );
			unsigned char hashval[20];
			char *pos;
			char final[41];
			shaBlock((unsigned char *)hash, file_size, hashval);
			pos = final;
			int k;
			for(k=0;k<20;k++)
			{
				snprintf(pos, 3, "%02x", hashval[k]);
				pos += 2;
			}
			my_hash = final;
			free(hash);
			return true;
		}
	}
	return false;
}

bool jabberhook::url_port_get(const string &full_url, string &url, int &port, string &tail, string &filename) {

	int pos;

	url = full_url;
	string temp;
	if( (pos = url.find(":")) != -1)  {
		if((pos = url.find(":", pos+1)) != -1) {
			temp = url.substr(pos+1);
			if((pos = temp.find("/")) != -1) {
				tail = temp.substr(pos);
				temp.erase(pos);
				port = atoi(temp.c_str());
				if((pos = tail.rfind("/")) != -1) {
					filename = tail.substr(pos+1);
					url = url.substr(7); //remove http://
					if((pos = url.find(":")) != -1) {
						url.erase(pos); //remove after :port/
						return true;
					}
				}
			}

		}
	}
	return false;
}


void jabberhook::send_file(const string &cjid) //http sendfile
{
	int ptpmin, ptpmax;
	vector<imfile::record> files = srfiles[cjid].first.getfiles();
	int ir = srfiles[cjid].second.second;
	string full_jid = jhook.full_jids[cjid];

	if( ir < files.size() )
	{
		struct send_file *file = (struct send_file *)srfiles[cjid].second.first;

		xmlnode x, y;

		x = jutil_iqnew(JPACKET__SET, NS_OOB);
		xmlnode_put_attrib(x, "from", getourjid().c_str());
		xmlnode_put_attrib(x, "to", full_jid.c_str());

		string new_id = (string)"centerim" + i2str(rand());

		xmlnode_put_attrib(x, "id", new_id.c_str());
		y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "url"); 

		conf->getpeertopeer(ptpmin, ptpmax); //getting max and min port range 

		jabber_send_file(jhook.jc, files[ir].fname.c_str(), files[ir].size, file, strdup(cjid.c_str()), &progressbar, ptpmin, ptpmax ); //real send fuction, starting server
		string send_url = (string)"http://" + file->host + (string)":" + i2str(file->port) + (string)"/" + justfname(files[ir].fname);
		xmlnode_insert_cdata(y, send_url.c_str(), (unsigned) -1 );
		
		xmlnode k;
		k = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "desc");
		string desc = "File size: " + i2str(files[ir].size);
		xmlnode_insert_cdata(k, desc.c_str(), (unsigned) -1 );
		 
		if(x) {
			jab_send(jc, x);
			xmlnode_free(x);
		}
	}
}

bool jabberhook::knowntransfer(const imfile &fr) const {
	return transferinfo.find(fr) != transferinfo.end();
}

void jabberhook::replytransfer(const imfile &fr, bool accept, const string &localpath) {      
	struct send_file *file = (struct send_file *)transferinfo[fr].first;
	if(accept) {                                                                           
		transferinfo[fr].second = localpath;
		if(transferinfo[fr].second.substr(transferinfo[fr].second.size()-1) != "/")
			transferinfo[fr].second += "/";

		transferinfo[fr].second += justfname(fr.getfiles().begin()->fname);
		if(file)
		{
			if(file->transfer_type == 0) //socks5 file transfer
				jhook.bytenegotiat(fr);
			if(file->transfer_type == 1) //http file transfer
				getfile(fr);
		}


	} else {                                                                               clean_up_file(fr, 0);

	}                                                                                      
}

void jabberhook::aborttransfer(const imfile &fr) {//overloaded function to aborting filetransfers
	 
	if( fr.getdirection() == imevent::incoming )
	{
		if( transferinfo.find(fr) != transferinfo.end() )
		{
			face.transferupdate(fr.getfiles().begin()->fname, fr,icqface::tsCancel, 0, 0);
			clean_up_file(fr, 0); //do some cleanups
		}
	}
	else
	{
		string sr = back_srfiles[fr];
		if( back_srfiles.find(fr) != back_srfiles.end() )
		{
			vector<imfile::record> files = fr.getfiles();
			face.transferupdate(files[srfiles[sr].second.second].fname, fr,icqface::tsCancel, 0, 0);
			clean_up_file(fr, 1, (char*)sr.c_str()); //do some cleanups
		}
	}
}
 
void jabberhook::progressbar(void *file, long int bytes, long int size, int status, int conn_type ) { //conn_type - litle trick to easy determine send or recieve conn. type
	char *jid = (char*)file;
	if( conn_type == 0 )
	{
		if( jhook.transferusers.find(jid) != jhook.transferusers.end() )
		{
			imfile fr = jhook.transferusers[jid]; 
			switch(status)
			{
				case 0:
					face.transferupdate(fr.getfiles().begin()->fname, fr, icqface::tsProgress, size, bytes);
					break;
				case 1:
					face.transferupdate(fr.getfiles().begin()->fname, fr, icqface::tsFinish, size, 0);
					jhook.clean_up_file(fr, 0);
					break;
				default:
					face.transferupdate(fr.getfiles().begin()->fname, fr, icqface::tsError, 0, 0);
					jhook.clean_up_file(fr, 0);
					break;
			}
		}
	}
	else if( conn_type == 1 )
	{
		if( jhook.srfiles.find(jid) != jhook.srfiles.end() )
		{
			imfile fr = jhook.srfiles[jid].first; 
			vector<imfile::record> files = fr.getfiles();
			int ir = jhook.srfiles[jid].second.second;

			if( files.size() > ir )
			{
				switch(status)
				{
					case 0:
						face.transferupdate(files[ir].fname, fr, icqface::tsProgress, size, bytes);
						break;
					case 1:
						face.transferupdate(files[ir].fname, fr, icqface::tsFinish, size, 0);
//						ir++;
						jhook.srfiles[jid].second.second++;
						if( files.size() > ir )
							jhook.send_file(jid);
						else
							jhook.clean_up_file(fr, 1, jid);
						break;
					default:
						face.transferupdate(files[ir].fname, fr, icqface::tsError, 0, 0);
						jhook.clean_up_file(fr, 1, jid);
						break;
				}
			}
		}
	}
}
 
void jabberhook::file_transfer_request(const imcontact &ic, xmlnode i) { //stream initialization request xep-0066
    char *p;
    char *filename, *filesize, *id;
    xmlnode x, z;
     
    p = xmlnode_get_attrib(i, "id"); 
    if(p) 
	    id = p;
    else
	    return;
     
    x = xmlnode_get_tag(i, "si");
    z = xmlnode_get_tag(x, "file");
     
    p = xmlnode_get_attrib(z, "name"); 
    if(p) 
	    filename = p;
    else
	    return;
     
    p = xmlnode_get_attrib(z, "size"); 
    if(p) 
	    filesize = p;
    else
	    return;
     
    
    struct send_file *trans_file = (struct send_file *)malloc( sizeof( struct send_file ) );
    trans_file->full_jid_name = strdup( jhook.full_jids[jidnormalize(ic.nickname).c_str()].c_str() );
    trans_file->id = strdup(id);
     
    trans_file->host = NULL;
    trans_file->url = NULL;
    trans_file->sid_from_to = NULL;
     
    trans_file->transfer_type = 0;
     
    imfile::record r;
    //get filename and filesize from request
    r.fname = filename;
    r.size = atoi(filesize);
     
    imfile fr(ic, imevent::incoming, "", vector<imfile::record>(1, r));
    jhook.transferinfo[fr].first = trans_file;
     
    transferusers[jidnormalize(ic.nickname)] = fr; //little hack
     
    em.store(fr);
    face.transferupdate(r.fname, fr, icqface::tsInit, r.size, 0);
}
     
 
void jabberhook::bytenegotiat(const imfile &fr) { //stream initialization result (xep-0066) for socks 5 bytestreams
	if( transferinfo.find(fr) != transferinfo.end() )
	{
		struct send_file *file = (struct send_file *)transferinfo[fr].first;
		char *cjid = file->full_jid_name;
		char *id = file->id;
		 
		xmlnode x0, x, q, w, e, y, z;

		x0 = jutil_iqnew2(JPACKET__RESULT);
		xmlnode_put_attrib(x0, "to", cjid);
		xmlnode_put_attrib(x0, "from", getourjid().c_str());
		xmlnode_put_attrib(x0, "id", id );

		y = xmlnode_insert_tag(x0, "si");
		xmlnode_put_attrib(y, "xmlns", NS_SI);
		 
		z = xmlnode_insert_tag(xmlnode_get_tag(x0,"si"), "feature");
		xmlnode_put_attrib(z, "xmlns", NS_NEG);
		 
		x = xmlnode_insert_tag(z, "x");
		xmlnode_put_attrib(x, "xmlns", NS_DATA);
		xmlnode_put_attrib(x, "type", "submit");
		 
		q = xmlnode_insert_tag(x, "field");
		xmlnode_put_attrib(q, "var", "stream-method");
		 
		w = xmlnode_insert_tag(q, "value");
		xmlnode_insert_cdata(w, NS_BYTESTREAMS, (unsigned) -1 );

		jab_send(jc, x0);
		xmlnode_free(x0);
	}
}
 
void jabberhook::reject_file(const imfile &fr) { //don't want to recieve file . xep-0065(socks5 bytestream)
	if( transferinfo.find(fr) != transferinfo.end() )
	{
		struct send_file *file = (struct send_file *)transferinfo[fr].first;
		 
		xmlnode x0 = jutil_iqnew2(JPACKET__RESULT), y,z;
		xmlnode_put_attrib(x0, "to", file->full_jid_name);
		xmlnode_put_attrib(x0, "from", getourjid().c_str());
		xmlnode_put_attrib(x0, "id", file->id);
		 
		jab_send(jc, x0);
		xmlnode_free(x0);
	}
}


void jabberhook::getfile_result(const imfile &fr) { //socks5 bytestream activation request xep-0065
	//prepare acknowledge packageto reciever
	if( transferinfo.find(fr) != transferinfo.end() )
	{
		struct send_file *file = (struct send_file *)transferinfo[fr].first;
		if(file)
		{
			if( file->full_jid_name && file->id )
			{
				xmlnode x = jutil_iqnew(JPACKET__RESULT, NS_BYTESTREAMS), y;
				xmlnode_put_attrib(x, "to", file->full_jid_name);
				xmlnode_put_attrib(x, "from", getourjid().c_str());
				xmlnode_put_attrib(x, "id", file->id );
				y = xmlnode_insert_tag(xmlnode_get_tag(x,"query"), "streamhost-used");
				xmlnode_put_attrib(y, "jid", file->full_jid_name );
				jab_send(jc, x);
				xmlnode_free(x);
			}
		}
	}
}
 
void jabberhook::getfile_http(const imcontact &ic, xmlnode i) { //xep-0066
	 
    string full_url, host, url, filename;
    int port;
    char *p;
    char *id;
     
    xmlnode x, y;
     
    p = xmlnode_get_attrib(i, "id"); 
    if(p)
	    id = p;
    else
	    return;
     
    x = xmlnode_get_tag(i, "query");
    y = xmlnode_get_tag(x, "url");
    p = xmlnode_get_data(y);
    if(p) 
	    full_url = p;
    else
	    return;
     
     
    struct send_file *trans_file = (struct send_file *)malloc( sizeof( struct send_file ) );
    trans_file->full_jid_name = strdup( jhook.full_jids[jidnormalize(ic.nickname).c_str()].c_str() );
    trans_file->id = strdup(id);
    if( url_port_get(full_url, host, port, url, filename) )
    {
	    trans_file->host = strdup(host.c_str());
	    trans_file->url = strdup(url.c_str());
	    trans_file->port = port;
	    trans_file->sid_from_to = NULL;
	    trans_file->transfer_type = 1;
    }
     
    imfile::record r;
    r.fname = filename;
    r.size = -1; //http initialization request over xmpp we don't know filesize :(
     
    imfile fr(ic, imevent::incoming, "", vector<imfile::record>(1, r));
    jhook.transferinfo[fr].first = trans_file;
     
    transferusers[jidnormalize(ic.nickname)] = fr; //little hack to create association imfile with JID
     
    em.store(fr);
    face.transferupdate(r.fname, fr, icqface::tsInit, r.size, 0);


}
 
void jabberhook::getfile(const imfile &fr) {
    if( transferinfo.find(fr) != transferinfo.end() )
    {
	    face.transferupdate(fr.getfiles().begin()->fname, fr, icqface::tsStart, 0, 0);
	     
	    struct send_file *file = (struct send_file *)transferinfo[fr].first;
	    if(file)
	    {
		    if( file->transfer_type == 0 )//bytestream method
		    {
#ifdef HAVE_THREAD
			    jabber_get_file(jhook.jc, transferinfo[fr].second.c_str(), fr.getfiles().begin()->size, file, strdup( jidtodisp(file->full_jid_name).c_str() ), &progressbar);
			    jhook.getfile_result(fr);
#endif
		    }
		    else if( file->transfer_type == 1 )//http GET method
		    {
#ifdef HAVE_THREAD
			    jabber_get_file(jhook.jc, transferinfo[fr].second.c_str(), fr.getfiles().begin()->size, file, strdup( jidtodisp(file->full_jid_name).c_str() ), &progressbar);
#endif
		    }
	    }
    }
}
 
//parse bytestream xmpp request
void jabberhook::getfile_byte(const imcontact &ic, xmlnode i) {
	 
    char *host, *jid, *sid;
    int port;
    char *p;
     
    xmlnode x, z;
     
    x = xmlnode_get_tag(i, "query"), z;
    p = xmlnode_get_attrib(x, "sid"); 
    if(p) 
	    sid = p;
    else
	    return;
     
    z = xmlnode_get_tag(x, "streamhost");
    p = xmlnode_get_attrib(z, "host");
    if(p)
	    host = p;
    else 
	    return;
     
    p = xmlnode_get_attrib(z, "jid");
    if(p)
	    jid = p;
    else 
	    return;
     
    p = xmlnode_get_attrib(z, "port");
    if(p)
	    port = atoi(p);
    else
	    return;
     
    string sid_from_to = (string)sid + (string)jid + getourjid() ;
    imfile fr = transferusers[jidnormalize(ic.nickname)];
    if( transferinfo.find(fr) != transferinfo.end() )
    {
	    struct send_file *file = (struct send_file *)transferinfo[fr].first;
	    file->sid_from_to = strdup( sid_from_to.c_str() );
	    file->port = port;
	    file->host = strdup( host );
	    file->url = NULL;
	    getfile(fr);
    }
}

void jabberhook::clean_up_file(const imfile &fr, int trans_type, char *cjid )
{
	
	struct send_file *file;
	if(trans_type == 0)
	{
		if(transferinfo.find(fr) != transferinfo.end())
			file = (struct send_file *)transferinfo[fr].first;
		else
			return;
	}
	else if(trans_type == 1)
	{
		if(!cjid)
			return;
		if(srfiles.find(cjid) != srfiles.end())
			file = (struct send_file *)srfiles[cjid].second.first;
		else
			return;
	}
	else
		return;

	if(file)
	{
#ifdef HAVE_THREAD
		pthread_cancel(file->thread);
#endif
		if(file->full_jid_name)
		{
			if(trans_type == 0)
				transferusers.erase(jidtodisp(file->full_jid_name));
			else if(trans_type == 1)
				back_srfiles.erase(srfiles[cjid].first);
			free(file->full_jid_name);
		}
		if(file->id)
			free(file->id);
		if(file->host)
			free(file->host);
		if(file->url)
			free(file->url);
		if(file->sid_from_to)
			free(file->sid_from_to);
		free(file);
		if(trans_type == 0)
			transferinfo[fr].first = NULL;
		else if (trans_type == 1)
			srfiles[cjid].second.first = NULL;
	}
	if(trans_type == 0)
		transferinfo.erase(fr);
	if(trans_type == 1)
		srfiles.erase(cjid);
}



#endif
