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

FILE *icqoffline::open(unsigned int uin, const char *mode) {
    icqcontact *c = clist.get(uin);
    string fname = (string) getenv("HOME") + "/.centericq/" + i2str(uin) + "/offline";
    return fopen(fname.c_str(), mode);
}

void icqoffline::sendmsg(unsigned int uin, string text) {
    FILE *f = open(uin, "a");
    icqcontact *c = clist.get(uin);
    unsigned long seq;

    if(f) {
	if(text.size() > MAX_UDPMSG_SIZE) {
	    if(c->getdirect()) {
		seq = icq_SendMessage(&icql, uin, text.c_str(), ICQ_SEND_DIRECT);
	    } else {
		seq = 0;
	    }
	} else {
	    seq = icq_SendMessage(&icql, uin, text.c_str(), ICQ_SEND_BESTWAY);
	}

	fprintf(f, "\f\nMSG\n");
	fprintf(f, "%lu\n%lu\n", seq, time(0));
	fprintf(f, "%s\n", text.c_str());
	fclose(f);
	totalunsent++;
	face.showtopbar();
    }
}

void icqoffline::sendurl(unsigned int uin, string url, string text) {
    FILE *f = open(uin, "a");
    icqcontact *c = clist.get(uin);

    if(f) {
	unsigned long seq = icq_SendURL(&icql, uin, url.c_str(), text.c_str(),
	    c->getstatus() != STATUS_OFFLINE ? ICQ_SEND_BESTWAY : ICQ_SEND_THRUSERVER);

	fprintf(f, "\f\nURL\n");
	fprintf(f, "%lu\n%lu\n", seq, time(0));
	fprintf(f, "%s\n", url.c_str());
	fprintf(f, "%s\n", text.c_str());
	fclose(f);
	totalunsent++;
	face.showtopbar();
    }
}

void icqoffline::sendevent(unsigned int uin, bool msg, string url,
string text, FILE *of, unsigned long seq, time_t tm, scanaction act,
unsigned long sseq) {
    icqcontact *c = clist.get(uin);
    int way;
    bool send, save;

    if(c) {
	send = save = true;
	way = c->getdirect() ? ICQ_SEND_BESTWAY : ICQ_SEND_THRUSERVER;

	switch(act) {
	    case osresend:
		if(sseq == seq) {
		    way = ICQ_SEND_THRUSERVER;
		} else {
		    send = false;
		}
		break;
	    case osexpired:
		if(time(0)-tm > PERIOD_RESEND) {
		} else {
		    send = false;
		}
		break;
	    case ossendall:
		break;
	    case osremove:
		if(sseq == seq) {
		    send = save = false;
		} else {
		    send = false;
		}
		break;
	}

	if(msg && text.size()) {
	    if(send) {
		seq = icq_SendMessage(&icql, uin, text.c_str(), way);
	    }

	    if(save) {
		fprintf(of, "\f\nMSG\n");
		fprintf(of, "%lu\n%lu\n", seq, tm);
		fprintf(of, "%s\n", text.c_str());
		processed++;
	    }
	} else if(!msg && url.size()) {
	    if(send) {
		seq = icq_SendURL(&icql, uin, url.c_str(), text.c_str(), way);
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
}

void icqoffline::scan(unsigned long sseq, scanaction act) {
    unsigned int uin, t;
    unsigned long seq;
    int i, l = 0;
    FILE *f, *of;
    char buf[512];
    string url, text, ofname;
    bool msg;

    totalunsent = 0;

    for(i = processed = 0; i < clist.count; i++) {
	uin = ((icqcontact *) clist.at(i))->getuin();
	text = url = "";

	if(f = open(uin, "r")) {
	    ofname = (string) getenv("HOME") + "/.centericq/tmp." + i2str(getpid());
	    of = fopen(ofname.c_str(), "w");

	    while(!feof(f)) {
		freads(f, buf, 512);

		if((string) buf == "\f") {
		    l = 0;
		    sendevent(uin, msg, url, text, of, seq, t, act, sseq);
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

	    sendevent(uin, msg, url, text, of, seq, t, act, sseq);
	    fclose(f);
	    fclose(of);

	    text = (string) getenv("HOME") + "/.centericq/" + i2str(uin) + "/offline";
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
