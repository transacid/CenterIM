#include "imexternal.h"
#include "icqconf.h"
#include "abstracthook.h"
#include "eventmanager.h"
#include "imlogger.h"
#include "icqcontacts.h"

#include <strstream>
#include <sys/wait.h>

imexternal external;

imexternal::imexternal() {
}

imexternal::~imexternal() {
}

void imexternal::load() {
    ifstream f(conf.getconfigfname("external").c_str());
    action a;

    actions.clear();

    if(f.is_open()) {
	while(!f.eof()) {
	    a = action();

	    if(a.load(f)) {
		actions.push_back(a);
	    }
	}

	f.close();
    }
}

int imexternal::exec(const imevent &ev) {
    bool r;
    auto_ptr<imevent> p(ev.getevent());
    int n = exec(p.get(), r);
    return n;
}

int imexternal::exec(imevent *ev, bool &eaccept, int option) {
    vector<action>::iterator i;
    int r = 0, rl;
    imcontact c;

    eaccept = true;

    for(i = actions.begin(); i != actions.end(); ++i) {
	if(i->exec(ev, rl, option)) {
	    eaccept = eaccept && !rl;
	    r++;
	}
    }

    return r;
}

vector<imexternal::actioninfo> imexternal::getlist() const {
    vector<actioninfo> r;
    vector<action>::const_iterator i;

    for(i = actions.begin(); i != actions.end(); ++i) {
	r.push_back(i->getinfo());
    }

    return r;
}

void imexternal::update(const vector<imexternal::actioninfo> &info) {
    vector<imexternal::actioninfo>::const_iterator i;

    for(i = info.begin(); i != info.end(); ++i) {
    }
}

// ----------------------------------------------------------------------------

imexternal::action::action(): options(0), enabled(true/*false*/) {
}

imexternal::action::~action() {
}

bool imexternal::action::exec(imevent *ev, int &result, int option) {
    bool r;
    char buf[512];
    abstracthook &hook = gethook(ev->getcontact().pname);

    r = enabled &&
	(find(event.begin(), event.end(), ev->gettype()) != event.end()) &&
	(find(proto.begin(), proto.end(), ev->getcontact().pname) != proto.end()) &&
	(find(status.begin(), status.end(), hook.getstatus()) != status.end())
    && ((option && (options & option)) || !option);

    if(r && !option) {
	r = !(options & aoprereceive) && !(options & aopresend);
    }

    if(r) {
	currentev = ev;

	writescript();
	result = execscript();

	sprintf(buf, _("executed external action %s, return code = %d"),
	    name.c_str(), result);

	logger.putmessage(buf);

	if(!option) {
	    respond();

	} else {
	    substitute();

	}
    }

    return r;
}

void imexternal::action::respond() {
    imevent *ev;

    if((options & aostdout) && !output.empty()) {
	ev = 0;

	switch(currentev->gettype()) {
	    case imevent::sms:
		ev = new imsms(currentev->getcontact(),
		    imevent::outgoing, output);
		break;
	    default:
		ev = new immessage(currentev->getcontact(),
		    imevent::outgoing, output);
		break;
	}

	if(ev) {
	    em.store(*ev);
	    delete ev;
	}
    }
}

void imexternal::action::substitute() {
    if((options & aostdout) && !output.empty()) {
	if(currentev->gettype() == imevent::message) {
	    immessage *m = static_cast<immessage *>(currentev);
	    *m = immessage(m->getcontact(), m->getdirection(), output);

	} else if(currentev->gettype() == imevent::url) {
	    imurl *m = static_cast<imurl *>(currentev);
	    *m = imurl(m->getcontact(), m->getdirection(), m->geturl(), output);

	} else if(currentev->gettype() == imevent::sms) {
	    imsms *m = static_cast<imsms *>(currentev);
	    *m = imsms(m->getcontact(), m->getdirection(), output, m->getphone());

	} else if(currentev->gettype() == imevent::authorization) {
	    imauthorization *m = static_cast<imauthorization *>(currentev);
	    *m = imauthorization(m->getcontact(), m->getdirection(), m->getauthtype(), output);

	}
    }
}

void imexternal::action::writescript() {
    ostrstream tfname;
    ofstream f;
    int i = 0;

    do {
	tfname.clear();
	tfname << conf.getdirname() << "centericq-external-tmp." << dec << time(0)+i++;
	sname = string(tfname.str(), tfname.pcount());
    } while(!access(sname.c_str(), F_OK));

    f.open(sname.c_str());

    if(f.is_open()) {
	f << code << endl;
	f.close();
    }
}

int imexternal::action::execscript() {
    int inpipe[2], outpipe[2], pid;
    string text;
    char ch;
    icqcontact *c;
    fd_set rfds;
    int r = -1;
    struct sigaction sact, osact;

    memset(&sact, 0, sizeof(sact));
    sigaction(SIGCHLD, &sact, &osact);

    if(!pipe(inpipe) && !pipe(outpipe)) {
	pid = fork();

	if(!pid) {
	    if(c = clist.get(currentev->getcontact())) {
		setenv("SENDER_UIN", i2str(c->getdesc().uin).c_str(), 1);
		setenv("SENDER_NICK", c->getnick().c_str(), 1);
		setenv("SENDER_INFODIR", c->getdirname().c_str(), 1);

		setenv("EVENT_TYPE", geteventname(currentev->gettype()).c_str(), 1);
		setenv("EVENT_NETWORK", conf.getprotocolname(c->getdesc().pname).c_str(), 1);
	    }

	    dup2(inpipe[1], STDOUT_FILENO);
	    dup2(outpipe[0], STDIN_FILENO);

	    close(inpipe[1]);
	    close(inpipe[0]);
	    close(outpipe[0]);
	    close(outpipe[1]);

	    execl("/bin/sh", "sh", sname.c_str(), 0);
	    _exit(0);
	} else {
	    close(outpipe[0]);
	    close(inpipe[1]);

	    if(options & aostdin) {
		text = currentev->gettext();
		write(outpipe[1], text.c_str(), text.size());
		close(outpipe[1]);
	    }

	    text = "";

	    if(options & aostdout) {
		while(1) {
		    FD_ZERO(&rfds);
		    FD_SET(inpipe[0], &rfds);

		    if(select(inpipe[0]+1, &rfds, 0, 0, 0) < 0) break; else {
			if(FD_ISSET(inpipe[0], &rfds)) {
			    if(read(inpipe[0], &ch, 1) != 1) break; else {
				text += ch;
			    }
			}
		    }
		}
	    }

	    waitpid(pid, &r, 0);

	    close(inpipe[0]);
	    close(outpipe[1]);
	    output = text;
	}
    }

    sigaction(SIGCHLD, &osact, 0);
    unlink(sname.c_str());
    return r;
}

void imexternal::action::disable() {
    enabled = false;
}

void imexternal::action::enable() {
    enabled = true;
}

bool imexternal::action::load(ifstream &f) {
    int pos, npos, i;
    string buf, sect, param, aname;
    bool action, wasexec = false;

    static const struct {
	const char *name;
	int option;
    } actions[] = {
	{ "pre-receive", aoprereceive },
	{ "pre-send", aopresend },
	{ 0, 0 }
    };

    while(getconf(sect, buf, f, sect == "exec")) {
	if((npos = sect.find_first_of(" \t")) == -1)
	    npos = sect.size();

	aname = sect.substr(0, npos);

	if(!(action = aname == "action"))
	    for(i = 0; actions[i].name && !action; i++)
		action = aname == actions[i].name;

	if(action) {
	    if(wasexec) {
		f.seekg(pos, ios::beg);
		break;
	    }

	    if(npos != sect.size() && name.empty())
		name = leadcut(sect.substr(npos+1));

	    for(i = 0; actions[i].name; i++) {
		if(aname == actions[i].name) {
		    if(!(options & actions[i].option))
			options |= actions[i].option;
		    break;
		}
	    }

	    param = getword(buf);

	    if(param == "event") {
		while(!(param = getword(buf)).empty()) {
		    for(imevent::imeventtype et = imevent::message; et != imevent::imeventtype_size; (int) et += 1) {
			if((param == geteventname(et))
			|| (param == "all")) {
			    event.push_back(et);
			}
		    }
		}

	    } else if(param == "proto") {
		while(!(param = getword(buf)).empty()) {
		    for(protocolname pname = icq; pname != protocolname_size; (int) pname += 1) {
			if((param == conf.getprotocolname(pname))
			|| (param == "all")) {
			    proto.push_back(pname);
			}
		    }
		}

	    } else if(param == "status") {
		while(!(param = getword(buf)).empty()) {
		    if(param == "online") status.push_back(available); else
		    if(param == "away") status.push_back(away); else
		    if(param == "dnd") status.push_back(dontdisturb); else
		    if(param == "na") status.push_back(notavail); else
		    if(param == "occupied") status.push_back(occupied); else
		    if(param == "ffc") status.push_back(freeforchat); else
		    if(param == "invisible") status.push_back(invisible); else
		    if(param == "all") {
			status.push_back(available);
			status.push_back(away);
			status.push_back(dontdisturb);
			status.push_back(notavail);
			status.push_back(occupied);
			status.push_back(freeforchat);
			status.push_back(invisible);
		    }
		}

	    } else if(param == "options") {
		while(!(param = getword(buf)).empty()) {
		    if(param == "stdin") options |= aostdin; else
		    if(param == "stdout") options |= aostdout;
		}
	    }
	} else if(sect == "exec") {
	    wasexec = true;
	    if(!code.empty()) code += "\n";
	    code += buf;
	}

	pos = f.tellg();
    }

    return wasexec;
}

const imexternal::actioninfo imexternal::action::getinfo() const {
    actioninfo r;

    r.name = name;
    r.enabled = enabled;

    return r;
}

string imexternal::action::geteventname(imevent::imeventtype et) {
    switch(et) {
	case imevent::message:
	    return "msg";
	case imevent::url:
	    return "url";
	case imevent::sms:
	    return "sms";
	case imevent::online:
	    return "online";
	case imevent::notification:
	    return "notification";
	case imevent::authorization:
	    return "auth";
	case imevent::contacts:
	    return "contacts";
	case imevent::file:
	    return "file(s)";
    }

    return "";
}
