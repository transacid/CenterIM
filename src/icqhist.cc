#include "icqhist.h"
#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqconf.h"

icqhistory::icqhistory() {
}

icqhistory::~icqhistory() {
}

FILE *icqhistory::open(unsigned int uin, const char *mode) {
    icqcontact *c = clist.get(uin);

    if(!c) {
	if(conf.getantispam()) return 0;
	else clist.addnew(uin);
    }

    string fname = (string) getenv("HOME") + "/.centericq/" + i2str(uin) + "/history";
    return fopen(fname.c_str(), mode);
}

void icqhistory::putmessage(unsigned int uin, string text, int dir, struct tm *timestamp) {
    FILE *f = open(uin, "a");
    if(f) {
	fprintf(f, "\f\n");
	fprintf(f, "%s\nMSG\n", dir == HIST_MSG_OUT ? "OUT" : "IN");
	fprintf(f, "%lu\n%lu\n%s\n", mktime(timestamp), time(0), text.c_str());
	fclose(f);
    }
}

void icqhistory::puturl(unsigned int uin, string url, string desc, int dir, struct tm *timestamp) {
    FILE *f = open(uin, "a");
    if(f) {
	fprintf(f, "\f\n");
	fprintf(f, "%s\nURL\n", dir == HIST_MSG_OUT ? "OUT" : "IN");
	fprintf(f, "%lu\n%lu\n%s\n%s\n", mktime(timestamp), time(0), url.c_str(), desc.c_str());
	fclose(f);
    }
}

void icqhistory::putfile(unsigned int uin, unsigned long seq, string fname,
int fsize, int dir, struct tm *timestamp) {
    FILE *f = open(uin, "a");
    if(f) {
	fprintf(f, "\f\n");
	fprintf(f, "%s\nFILE\n", dir == HIST_MSG_OUT ? "OUT" : "IN");
	fprintf(f, "%lu\n%lu\n", mktime(timestamp), time(0));
	fprintf(f, "%lu\n%s\n%lu\n", seq, fname.c_str(), fsize);
	fclose(f);
    }
}

void icqhistory::putmail(string nick, string email, string msg, int mailt,
struct tm *timestamp) {
}

void icqhistory::putcontact(unsigned int uin, icqcontactmsg *conts,
int dir, struct tm *timestamp) {
    FILE *f = open(uin, "a");
    icqcontactmsg *cont;
    char buf[64];

    if(f) {
	fprintf(f, "\f\n");
	fprintf(f, "%s\nCONTACT\n", dir == HIST_MSG_OUT ? "OUT" : "IN");
	fprintf(f, "%lu\n%lu\n", mktime(timestamp), time(0));

	for(cont = conts; cont; ) {
	    fprintf(f, "%s\t%lu\n", mime(buf, cont->nick), cont->uin);
	    cont = (icqcontactmsg *) cont->next;
	}

	fclose(f);
    }
}

bool icqhistory::opencontact(unsigned int uin, time_t lastread = 0) {
    bool ret = true;
    FILE *f;

    if(f = open(uin, "r")) {
	opens.add(new storedopen(f));
    } else {
	ret = false;
    }

    return ret;
}

int icqhistory::setpos(int n) {
    char buf[512];
    storedopen *so = (storedopen *) opens.at(opens.count-1);

    rewind(so->f);
    so->rn = 0;

    if(n)
    while(!feof(so->f)) {
	freads(so->f, buf, 512);

	if(!strcmp(buf, "\f")) {
	    if(++so->rn == n) {
		break;
	    }
	}
    }

    return so->rn;
}

int icqhistory::setposlastread(time_t lr) {
    int ml = 0, fpos;
    char buf[512];
    bool finished;
    storedopen *so = (storedopen *) opens.at(opens.count-1);

    for(so->rn = 0, rewind(so->f), finished = false; !feof(so->f) && !finished; ) {
	freads(so->f, buf, 512);

	if(!strcmp(buf, "\f")) {
	    ml = 0;
	    fpos = ftell(so->f);
	    so->rn++;
	} else switch(ml) {
	    case 4:
		if(strtoul(buf, 0, 0) > lr) {
		    fseek(so->f, fpos, SEEK_SET);
		    finished = true;
		}
		break;
	}

	ml++;
    }

    return so->rn;
}

int icqhistory::readevent(int &event, time_t &lastread, struct tm &sent, int &dir) {
    int ml = 1, thisevent, thisdir;
    char buf[512];
    bool finished, mread = false;
    time_t t;
    struct tm tm;
    storedopen *so = (storedopen *) opens.at(opens.count-1);

    so->lastevent.clear();

    for(finished = false; !feof(so->f) && !finished; ) {
	freads(so->f, buf, 512);

	if(!strcmp(buf, "\f")) {
	    ml = 0;
	    if(mread) {
		finished = true;
	    } else {
		so->lastevent.clear();
	    }
	} else switch(ml) {
	    case 1:
		if(!strcmp(buf, "IN")) thisdir = HIST_MSG_IN; else
		if(!strcmp(buf, "OUT")) thisdir = HIST_MSG_OUT;
		break;
	    case 2:
		if(!strcmp(buf, "URL")) thisevent = EVT_URL; else
		if(!strcmp(buf, "MSG")) thisevent = EVT_MSG; else
		if(!strcmp(buf, "FILE")) thisevent = EVT_FILE; else
		if(!strcmp(buf, "CONTACT")) thisevent = EVT_CONTACT;
		break;
	    case 3:
		memcpy(&tm, localtime(&(t = atol(buf))), sizeof(tm));
		break;
	    case 4:
		dir = thisdir;
		event = thisevent;
		memcpy(&sent, &tm, sizeof(tm));
		lastread = atol(buf);
		so->rn++;
		mread = true;
		break;
	    default:
		if(ml > 4)
		    so->lastevent.push_back(buf);
		break;
	}

	ml++;
    }

    return mread ? so->rn : -1;
}

void icqhistory::getmessage(string &text) {
    storedopen *so = (storedopen *) opens.at(opens.count-1);
    vector<string>::iterator i;

    for(i = so->lastevent.begin(), text = ""; i != so->lastevent.end(); i++) {
	if(text.size()) text += "\n";
	text += *i;
    }
}

void icqhistory::geturl(string &url, string &text) {
    storedopen *so = (storedopen *) opens.at(opens.count-1);
    vector<string>::iterator i;

    url = text = "";

    for(i = so->lastevent.begin(); i != so->lastevent.end(); i++)
    if(i == so->lastevent.begin()) url = *i; else {
	if(text.size()) text += "\n";
	text += *i;
    }
}

void icqhistory::getfile(unsigned long &seq, string &fname, int &fsize) {
    storedopen *so = (storedopen *) opens.at(opens.count-1);
    int i;

    for(i = 0; i < so->lastevent.size(); i++)
    switch(i) {
	case 0: seq = strtoul(so->lastevent[i].c_str(), 0, 0); break;
	case 1: fname = so->lastevent[i]; break;
	case 2: fsize = atol(so->lastevent[i].c_str());
    }
}

void icqhistory::getcontact(icqcontactmsg **cont) {
    int len;
    const char *p;
    icqcontactmsg *cc;
    vector<string>::iterator i;
    storedopen *so = (storedopen *) opens.at(opens.count-1);

    *cont = 0;

    for(i = so->lastevent.begin(); i != so->lastevent.end(); i++)
    if(p = strrchr(i->c_str(), '\t')) {
	if(!*cont) {
	    cc = new icqcontactmsg;
	    *cont = cc;
	} else {
	    cc->next = new icqcontactmsg;
	    cc = (icqcontactmsg *) cc->next;
	    cc->next = 0;
	}

	cc->uin = strtoul(p+1, 0, 0);
	len = p-i->c_str();
	if(len > 31) len = 31;
	strncpy(cc->nick, i->c_str(), len);
	cc->nick[len] = 0;
	unmime(cc->nick);
	strim(cc->nick);
	cc->next = 0;
    }
}

void icqhistory::closecontact() {
    if(opens.count) {
	storedopen *so = (storedopen *) opens.at(opens.count-1);
	fclose(so->f);
	opens.remove(opens.count-1);
    }
}

void icqhistory::fillmenu(unsigned int uin, verticalmenu *m) {
    int event, dir, color, i, n = 0;
    struct tm tm;
    string text, url;
    icqcontactmsg *contact, *curcont;
    time_t lastread = 0;
    char buf[512];

    list<histentry> hitems;
    list<histentry>::iterator ii;
    
    if(opencontact(uin)) {
	while((n = readevent(event, lastread, tm, dir)) != -1) {
	    url = text = "";

	    switch(event) {
		case EVT_MSG:
		    getmessage(text);
		    break;
		case EVT_URL:
		    geturl(url, text);
		    text = url + " " + text;
		    break;
		case EVT_CONTACT:
		    getcontact(&contact);
		    for(curcont = contact; curcont; curcont = (icqcontactmsg *) curcont->next) {
			if(text.size()) text += ", ";
			text += (string) curcont->nick + " " + i2str(curcont->uin);
		    }
		    break;
	    }

	    if(!uin) text.replace(0, 1, "");

	    if(text.size()) {
		text = (string) " " + time2str(&lastread, "DD.MM hh:mm", buf) + " " + text;
		color = dir == HIST_MSG_IN ? conf.getcolor(cp_main_text) : conf.getcolor(cp_main_highlight);

		histentry h;
		h.text = text;
		h.lastread = n;
		h.color = color;
		hitems.push_back(h);
	    }
	}

	for(ii = --hitems.end(); ; ii--) {
	    m->additemf(ii->color, (void *) ii->lastread, "%s", ii->text.c_str());
	    if(ii == hitems.begin()) break;
	}

	closecontact();
    }
}

int icqhistory::getpos() {
    storedopen *so = (storedopen *) opens.at(opens.count-1);
    return so->rn;
}
