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

void imexternal::ssave() const {
}

int imexternal::exec(const imevent &ev) {
    vector<action>::iterator i;
    int r = 0;
    imcontact c;

    for(i = actions.begin(); i != actions.end(); i++) {
	if(i->exec(ev)) r++;
    }

    return r;
}

vector<imexternal::actioninfo> imexternal::getlist() const {
    vector<actioninfo> r;
    vector<action>::const_iterator i;

    for(i = actions.begin(); i != actions.end(); i++) {
	r.push_back(i->getinfo());
    }

    return r;
}

void imexternal::update(const vector<imexternal::actioninfo> &info) {
    vector<imexternal::actioninfo>::const_iterator i;

    for(i = info.begin(); i != info.end(); i++) {
    }
}

// ----------------------------------------------------------------------------

imexternal::action::action(): options(0), enabled(true/*false*/) {
}

imexternal::action::~action() {
}

bool imexternal::action::exec(const imevent &ev) {
    bool r;
    char buf[512];
    abstracthook &hook = gethook(ev.getcontact().pname);

    r = enabled &&
	(find(event.begin(), event.end(), ev.gettype()) != event.end()) &&
	(find(proto.begin(), proto.end(), ev.getcontact().pname) != proto.end()) &&
	(find(status.begin(), status.end(), hook.getstatus()) != status.end());

    if(r) {
	currentev = &ev;

	writescript();
	execscript();
	respond();

	sprintf(buf, _("executed external action %s"), name.c_str());
	logger.putmessage(buf);
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

void imexternal::action::writescript() {
    ostrstream tfname;
    ofstream f;

    do {
	tfname.clear();
	tfname << conf.getdirname() << "centericq-external-tmp." << dec << time(0);
	sname = string(tfname.str(), tfname.pcount());
    } while(!access(sname.c_str(), F_OK));

    f.open(sname.c_str());

    if(f.is_open()) {
	f << code << endl;
	f.close();
    }
}

void imexternal::action::execscript() {
    int inpipe[2], outpipe[2], pid;
    string text;
    char ch;
    ifstream pout;
    icqcontact *c;
    fd_set rfds;

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
		if(currentev->gettype() == imevent::message) {
		    const immessage *m = static_cast<const immessage *>(currentev);
		    text = m->gettext();
		}

		write(outpipe[1], text.c_str(), text.size());
		close(outpipe[1]);
	    }

	    text = "";

	    if(options & aostdout) {
		pout.attach(inpipe[0]);

		if(pout.is_open()) {
		    while(!pout.eof()) {
			FD_ZERO(&rfds);
			FD_SET(inpipe[0], &rfds);

			if(select(inpipe[0]+1, &rfds, 0, 0, 0) < 0) break; else {
			    if(FD_ISSET(inpipe[0], &rfds))
			    if((ch = pout.get()) != EOF)
				text += ch;
			}
		    }

		    pout.close();
		}
	    }

	    waitpid(pid, 0, 0);

	    close(inpipe[0]);
	    close(outpipe[1]);
	    output = text;
	}
    }

    unlink(sname.c_str());
}

void imexternal::action::disable() {
    enabled = false;
}

void imexternal::action::enable() {
    enabled = true;
}

bool imexternal::action::load(ifstream &f) {
    int pos;
    string buf, sect, param;

    while(getconf(sect, buf, f, sect == "exec")) {
	if(sect.substr(0, 6) == "action") {
	    param = getword(buf);

	    if(name.empty()) {
		name = leadcut(sect.substr(7));
	    } else {
		if(name != leadcut(sect.substr(7))) {
		    // another "action" section
		    f.seekg(pos, ios::beg);
		    break;
		}
	    }

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
	    if(!code.empty()) code += "\n";
	    code += buf;
	}

	pos = f.tellg();
    }

    return !name.empty() && !code.empty();
}

void imexternal::action::ssave(ofstream &f) const {
}

const imexternal::actioninfo imexternal::action::getinfo() const {
    actioninfo r;

    r.name = name;
    r.enabled = enabled;

    return r;
}

const string imexternal::action::geteventname(imevent::imeventtype et) {
    switch(et) {
	case imevent::message:
	    return "msg";
	case imevent::url:
	    return "url";
	case imevent::sms:
	    return "sms";
	case imevent::online:
	    return "online";
    }

    return "";
}
