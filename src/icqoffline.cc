/*
*
* centericq messages sending/auto-postponing class
* $Id: icqoffline.cc,v 1.10 2001/11/11 14:30:16 konst Exp $
*
* Copyright (C) 2001 by Konstantin Klyagin <konst@konst.org.ua>
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

#include "icqoffline.h"
#include "icqhist.h"
#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqconf.h"
#include "icqhook.h"
#include "icqface.h"

icqoffline::icqoffline() {
}

icqoffline::~icqoffline() {
}

FILE *icqoffline::open(const imcontact cinfo, const char *mode) {
    icqcontact *c = clist.get(cinfo);
    string fname = c->getdirname() + "/offline";
    return fopen(fname.c_str(), mode);
}

void icqoffline::sendmsg(const imcontact cinfo, const string atext) {
    icqcontact *c = clist.get(cinfo);
    unsigned long seq;
    FILE *f = open(cinfo, "a");
    vector<string> lst;
    string text = atext;

    if(f) {
	if(!c->getmsgdirect()) {
	    splitlongtext(text, lst, MAX_UDPMSG_SIZE, "\r\n[continued]");
	}

	while(1) {
	    if(!lst.empty()) {
		text = *lst.begin();
		lst.erase(lst.begin());
	    }

	    seq = icq_SendMessage(&icql, cinfo.uin, text.c_str(),
		c->getmsgdirect() ? ICQ_SEND_BESTWAY : ICQ_SEND_THRUSERVER);

	    fprintf(f, "\f\nMSG\n");
	    fprintf(f, "%lu\n%lu\n", seq, time(0));
	    fprintf(f, "%s\n", text.c_str());
	    totalunsent++;

	    if(lst.empty()) break;
	}

	fclose(f);
	face.showtopbar();
    }
}

void icqoffline::sendurl(const imcontact cinfo, const string url, const string text) {
    icqcontact *c = clist.get(cinfo);
    FILE *f = open(cinfo, "a");

    if(f) {
	unsigned long seq = icq_SendURL(&icql, cinfo.uin, url.c_str(), text.c_str(),
	    c->getmsgdirect() ? ICQ_SEND_BESTWAY : ICQ_SEND_THRUSERVER);

	fprintf(f, "\f\nURL\n");
	fprintf(f, "%lu\n%lu\n", seq, time(0));
	fprintf(f, "%s\n", url.c_str());
	fprintf(f, "%s\n", text.c_str());
	fclose(f);

	totalunsent++;
	face.showtopbar();
    }
}

bool icqoffline::sendevent(const imcontact cinfo, bool msg, const string url,
const string atext, FILE *of, unsigned long seq, time_t tm, scanaction act,
unsigned long sseq) {
    icqcontact *c = clist.get(cinfo);
    vector<string> lst;
    bool send, save;
    int way;
    string text = atext;

    if(c) {
	lst.clear();
	send = save = true;
	way = c->getmsgdirect() ? ICQ_SEND_BESTWAY : ICQ_SEND_THRUSERVER;

	switch(act) {
	    case osresend:
		if(send = (sseq == seq)) {
		    c->setmsgdirect(false);
		    if(text.size() < MAX_UDPMSG_SIZE) {
			way = ICQ_SEND_THRUSERVER;
		    } else {
			splitlongtext(text, lst,
			    MAX_UDPMSG_SIZE, "\r\n[continued]");
		    }
		}
		break;
	    case osexpired:
		if(send = (time(0)-tm > PERIOD_RESEND)) {
		    c->setmsgdirect(false);
		    if(text.size() < MAX_UDPMSG_SIZE) {
			way = ICQ_SEND_THRUSERVER;
		    } else {
			splitlongtext(text, lst,
			    MAX_UDPMSG_SIZE, "\r\n[continued]");
		    }
		}
		break;
	    case osremove:
		if(sseq == seq) {
		    time_t t = time(0);
		    send = save = false;

		    if(msg) {
			hist.putmessage(cinfo, text, HIST_MSG_OUT, localtime(&t));
		    } else {
			hist.puturl(cinfo, url, text, HIST_MSG_OUT, localtime(&t));
		    }
		} else {
		    send = false;
		}
		break;
	    case ossendall:
		break;
	}

	if(msg && text.size()) {
	    if(save)
	    while(1) {
		if(!lst.empty()) {
		    text = *lst.begin();
		    lst.erase(lst.begin());
		}

		if(send)
		    seq = icq_SendMessage(&icql, cinfo.uin, text.c_str(), way);

		fprintf(of, "\f\nMSG\n");
		fprintf(of, "%lu\n%lu\n", seq, tm);
		fprintf(of, "%s\n", text.c_str());
		processed++;

		if(lst.empty()) break;
	    }
	} else if(!msg && url.size()) {
	    if(send) {
		seq = icq_SendURL(&icql, cinfo.uin, url.c_str(), text.c_str(), way);
	    }

	    if(save) {
		fprintf(of, "\f\nURL\n");
		fprintf(of, "%lu\n%lu\n", seq, tm);
		fprintf(of, "%s\n", url.c_str());
		fprintf(of, "%s\n", text.c_str());
		processed++;
	    }
	}
    }

    return c && save && ((!msg && url.size()) || (msg && text.size()));
}

void icqoffline::scan(unsigned long sseq, scanaction act) {
    unsigned int uin, t;
    unsigned long seq;
    int i, l = 0;
    FILE *f, *of;
    char buf[512];
    string url, text, ofname;
    bool msg;
    icqcontact *c;

    totalunsent = 0;

    for(i = processed = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);
	text = url = "";

	if(f = open(c->getdesc(), "r")) {
	    ofname = (string) getenv("HOME") + "/.centericq/tmp." + i2str(getpid());
	    of = fopen(ofname.c_str(), "w");

	    while(!feof(f)) {
		freads(f, buf, 512);

		if((string) buf == "\f") {
		    l = 0;
		    sendevent(c->getdesc(), msg, url, text, of, seq, t, act, sseq);
		    text = url = "";
		} else {
		    switch(l) {
			case 1:         // kind
			    msg = !strcmp(buf, "MSG"); break;
			case 2:         // seq
			    seq = strtoul(buf, 0, 0);
			    break;
			case 3:         // time
			    t = strtoul(buf, 0, 0);
			    break;
			case 4:         // url
			    if(!msg) url = buf; else text = buf;
			    break;

			default:
			    if(!feof(f)) {
				if(text.size()) text += "\r\n";
				text += buf;
			    }
			    break;
		    }
		}

		l++;
	    }

	    sendevent(c->getdesc(), msg, url, text, of, seq, t, act, sseq);
	    fclose(f);
	    fclose(of);

	    text = c->getdirname() + "/offline";
	    rename(ofname.c_str(), text.c_str());
	    if(!processed) unlink(text.c_str());
	}
    }

    totalunsent = processed;
    face.showtopbar();
}

int icqoffline::getunsentcount() {
    return totalunsent;
}
