/*
*
* centericq core routines
* $Id: centericq.cc,v 1.3 2001/04/08 07:14:07 konst Exp $
*
*/

#include "centericq.h"
#include "icqconf.h"
#include "icqhist.h"
#include "icqhook.h"
#include "icqface.h"
#include "icqoffline.h"
#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqmlist.h"

centericq::centericq() {
}

centericq::~centericq() {
}

void centericq::commandline(int argc, char **argv) {
    int i;

    for(i = 1; i < argc; i++) {
	string args = argv[i];
	if((args == "-a") || (args == "--ascii")) {
	    kintf_graph = 0;
	} else {
	    kendinterface();
	    cout << "centericq command line parameters:" << endl << endl;
	    cout << "\t--ascii or -a : use characters for windows and UI controls" << endl;
	    cout << "\tanything else : display this stuff" << endl;
	    exit(0);
	}
    }
}

void centericq::exec() {
    sigset_t sigs;
    string socksuser, sockspass;

    sigemptyset(&sigs);
    sigaddset(&sigs, SIGINT);
    sigaddset(&sigs, SIGSTOP);
    sigprocmask(SIG_BLOCK, &sigs, 0);
    signal(SIGCHLD, &handlesignal);

    conf.initpairs();
    conf.load();
    face.init();

    ihook.setmanualstatus(conf.getstatus());

    if(!conf.getuin()) {
	reg();
	if(ihook.getreguin()) {
	    conf.checkdir();
	    conf.getsocksuser(socksuser, sockspass);
	    conf.savemainconfig(ihook.getreguin());
	}
    } else {
	if(!conf.getsavepwd()) {
	    string cpass = face.inputstr(_("Password: "), "", '*');
	    if(cpass.empty()) exit(0); else conf.setpassword(cpass);
	}
    }

    conf.checkdir();
    clist.load();
    conf.load();
    lst.load();

    face.done();
    face.init();

    if(conf.getuin()) {
	icq_Init(&icql, conf.getuin(), conf.getpassword().c_str(), "");
	ihook.init(&icql);

	if(!conf.getsockshost().empty()) {
	    conf.getsocksuser(socksuser, sockspass);
	    icq_SetProxy(&icql, conf.getsockshost().c_str(),
		conf.getsocksport(), socksuser.empty() ? 0 : 1,
		socksuser.c_str(), sockspass.c_str());
	}

	face.draw();
	mainloop();
    }

    lst.save();
    clist.save();
    conf.savestatus(ihook.getmanualstatus());
    face.done();
}

void centericq::reg() {
    unsigned int ruin;
    int sockfd;
    bool finished;
    string rpasswd, nick, fname, lname, email, socksuser, sockspass;
    fd_set fds;
    FILE *f;

    if(!fork()) {
	string d = (string) getenv("HOME") + "/.centericq/";
	execlp("/bin/rm", "/bin/rm", "-rf", d.c_str(), 0);
	exit(0);
    }

    if(face.regdialog(ruin, rpasswd)) {
	if(!ruin) {
	    face.progress.show(_(" Registration progress "));
	    face.getregdata(nick, fname, lname, email);
	    icq_Init(&icql, ruin, rpasswd.c_str(), nick.c_str());

	    if(!conf.getsockshost().empty()) {
		conf.getsocksuser(socksuser, sockspass);

		icq_SetProxy(&icql, conf.getsockshost().c_str(),
		    conf.getsocksport(), socksuser.empty() ? 0 : 1,
		    socksuser.c_str(), sockspass.c_str());
	    }

	   ihook.reginit(&icql);

	    for(finished = false; !finished; ) {
		face.progress.log(_("Connecting to the server %s ..."),
		    conf.getservername().c_str());

		if(icq_Connect(&icql, conf.getservername().c_str(), conf.getserverport()) != -1) {
		    while(!finished) {
			face.progress.log(_("Sending request"));
			icq_RegNewUser(&icql, rpasswd.c_str());

			time_t spent = time(0);

			while(!ihook.getreguin() && !finished) {
			    struct timeval tv;
			    tv.tv_sec = 5;
			    tv.tv_usec = 0;

			    if(time(0)-spent > PERIOD_WAITNEWUIN) {
				if(face.ask(_("Timed out waiting for a new uin. Retry?"), ASK_YES | ASK_NO, ASK_YES) == ASK_NO) {
				    exit(0);
				} else {
				    face.progress.log(_("Retrying.."));
				    icq_RegNewUser(&icql, rpasswd.c_str());
				    time(&spent);
				}
			    }

			    FD_ZERO(&fds);
			    FD_SET((sockfd = icq_GetSok(&icql)), &fds);
			    select(sockfd+1, &fds, 0, 0, &tv);
			    if(FD_ISSET(sockfd, &fds)) {
				icq_HandleServerResponse(&icql);
			    }
			}

			if(!finished) {
			    face.progress.log(_("New UIN received, %lu"), ihook.getreguin());
			    finished = true;
			}
		    }

		    face.progress.log(_("Disconnected"));
		    icq_Disconnect(&icql);
		    ihook.regdisconnected(&icql);
		    sleep(2);
		} else {
		    if(face.ask(_("Unable to connect to the icq server. Retry?"),
		    ASK_YES | ASK_NO) == ASK_NO) {
			finished = true;
		    }
		}
	    }

	    conf.registerinfo(ihook.getreguin(), rpasswd, nick, fname, lname, email);
	    face.progress.hide();
	} else {
	    ihook.setreguin(ruin);
	    conf.registerinfo(ruin, rpasswd, nick, fname, lname, email);
	}
    }
}

void centericq::mainloop() {
    bool finished = false;
    string text, url;
    int action, old;
    icqcontact *c;
    char buf[512];
    list<unsigned int>::iterator i;

    face.draw();

    while(!finished) {
	face.status(_("F2/M current contact menu, F3/S change icq status, F4/G general actions, Q quit"));
	c = face.mainloop(action);

	switch(action) {
	    case ACT_IGNORELIST: face.modelist(csignore); break;
	    case ACT_VISIBLELIST: face.modelist(csvisible); break;
	    case ACT_INVISIBLELIST: face.modelist(csinvisible); break;
	    case ACT_STATUS: changestatus(); break;
	    case ACT_DETAILS:
		if(ihook.logged()) updatedetails();
		break;
	    case ACT_FIND:
		if(ihook.logged()) find();
		break;
	    case ACT_CONF: updateconf(); break;
	    case ACT_NONICQ: nonicq(0); break;
	    case ACT_QUICKFIND: face.quickfind(); break;
	    case ACT_QUIT:
		if(icql.icq_Status != STATUS_OFFLINE) {
		    icq_Logout(&icql);
		    icq_Disconnect(&icql);
		}
		icq_Done(&icql);
		finished = true;
		break;
	}

	if(!c) continue;

	if(c->getuin() && !c->isnonicq())
	switch(action) {
	    case ACT_URL:
		url = "";
		text = "";
		if(face.editurl(c->getuin(), url, text))
		for(i = face.muins.begin(); i != face.muins.end(); i++)
		    sendurl(*i, url, text);
		break;

	    case ACT_IGNORE:
		sprintf(buf, _("Ignore all events from %s (%lu)?"), c->getdispnick().c_str(), c->getuin());
		if(face.ask(buf, ASK_YES | ASK_NO, ASK_NO) == ASK_YES) {
		    lst.add(new icqlistitem(c->getdispnick(), c->getuin(), csignore));
		    clist.remove(c->getuin());
		    face.update();
		}
		break;

	    case ACT_SWITCH_VIS:
		if(lst.inlist(c->getuin(), csvisible)) {
		    lst.del(c->getuin(), csvisible);
		    face.log(_("+ %s has been removed from the visible list"), c->getdispnick().c_str());
		} else {
		    lst.add(new icqlistitem(c->getdispnick(), c->getuin(), csvisible));
		    face.log(_("+ %s has been added to the visible list"), c->getdispnick().c_str());
		}
		break;

	    case ACT_FILE: if(c->getstatus() != STATUS_OFFLINE) sendfiles(c->getuin()); break;
	    case ACT_CHAT: break;
	}
	
	if(c->isnonicq())
	switch(action) {
	    case ACT_EDITUSER: nonicq(c->getuin()); break;
	    case ACT_HISTORY: continue;
	}

	if(!c->inlist())
	switch(action) {
	    case ACT_ADD:
		if(!c->inlist()) {
		    c->includeintolist();
		    face.update();
		}
		break;
	}

	if(c->getuin())
	switch(action) {
	    case ACT_INFO: userinfo(c->getuin(), c->isnonicq()); break;
	    case ACT_REMOVE:
		sprintf(buf, _("Are you sure want to remove %s (%lu)?"), c->getdispnick().c_str(), c->getuin());
		if(face.ask(buf, ASK_YES | ASK_NO, ASK_NO) == ASK_YES) {
		    clist.remove(c->getuin(), c->isnonicq());
		    face.update();
		}
		break;
	    case ACT_RENAME:
		text = face.inputstr(_("New nickname to show: "), c->getdispnick());
		if(text.size()) {
		    c->setdispnick(text);
		    face.update();
		}
		break;
	}
	
	switch(action) {
	    case ACT_HISTORY: face.history(c->getuin()); break;
	    case ACT_MSG:
		if(c->getmsgcount()) {
		    face.read(c->getuin());
		} else if(c->getuin() && !c->isnonicq()) {
		    text = "";
		    if(face.editmsg(c->getuin(), text))
		    for(i = face.muins.begin(); i != face.muins.end(); i++)
			sendmsg(*i, text);
		}
		break;
	}
    }
}

void centericq::sendmsg(unsigned int uin, string text) {
    time_t tcurrent;
    string piece, cont = _(" [continued]");
    int i, k, maxsize = MAX_UDPMSG_SIZE-cont.size();
    icqcontact *c = clist.get(uin);

    if(c->getdirect()) {
	time(&tcurrent);
	if(text.size() > MAX_TCPMSG_SIZE) text.resize(MAX_TCPMSG_SIZE);
	offl.sendmsg(uin, text);
	hist.putmessage(uin, text, HIST_MSG_OUT, localtime(&tcurrent));
    } else
    for(i = 0; i < text.size(); ) {
	piece = text.substr(i, k = maxsize);

	if(i+maxsize < text.size()) {
	    for(k = maxsize; (k > 0) && (piece[k] != ' '); k--);
	    if(k) piece.resize(k); else k = maxsize;
	    i += k;
	    piece += cont;
	} else {
	    i = text.size();
	}

	time(&tcurrent);
	offl.sendmsg(uin, piece);
	hist.putmessage(uin, piece, HIST_MSG_OUT, localtime(&tcurrent));
    }
}

void centericq::sendurl(unsigned int uin, string url, string text) {
    time_t tcurrent;
    time(&tcurrent);

    offl.sendurl(uin, url, text);
    hist.puturl(uin, url, text, HIST_MSG_OUT, localtime(&tcurrent));
}

void centericq::fwdmsg(unsigned int uin, string text) {
    icqcontact *c = (icqcontact *) clist.get(uin);
    char buf[512];
    list<unsigned int> lst;
    list<unsigned int>::iterator i;

    sprintf(buf, _("%s (%lu) wrote:"), c ? c->getdispnick().c_str() : "I", uin);
    text = (string) buf + "\n\n" + text;

    if(face.multicontacts("", lst))
    if(!lst.empty())
    if(face.editmsg(*lst.begin(), text))
    for(i = lst.begin(); i != lst.end(); i++)
	cicq.sendmsg(*i, text);
}

void centericq::changestatus() {
    int old;

    ihook.setmanualstatus(face.changestatus(old = icql.icq_Status));

    if(ihook.getmanualstatus() != old) {
	if(ihook.getmanualstatus() == STATUS_OFFLINE) {
	    icq_Logout(&icql);
	    icq_Disconnect(&icql);
	    ihook.disconnected(&icql);
	} else {
	    if(icql.icq_Status == STATUS_OFFLINE) {
		ihook.connect();
	    } else {
		icq_ChangeStatus(&icql, ihook.getmanualstatus());
	    }
	}

	face.update();
    }
}

void centericq::find() {
    static string email, nick, first, last;
    static unsigned int uin = 0;
    bool ret = true;

    while(ret) {
	if(ret = face.finddialog(uin, nick, email, first, last)) {
	    if(uin) {
		icq_SendSearchUINReq(&icql, uin);
	    } else {
		icq_SendSearchReq(&icql, email.c_str(),
		    nick.c_str(), first.c_str(),
		    last.c_str());
	    }
	    ret = face.findresults();
	}
    }
}

void centericq::userinfo(unsigned int uin, bool nonicq = false) {
    unsigned int realuin = uin;
    icqcontact *c = clist.get(uin, nonicq);

    if(!c) {
	c = clist.get(0);
	realuin = 0;
	c->clear();
	c->setseq2(icq_SendMetaInfoReq(&icql, uin));
    }

    if(c) face.userinfo(uin, nonicq, realuin);
}

void centericq::updatedetails() {
    icqcontact *c = clist.get(0);

    c->clear();
    c->setseq2(icq_SendMetaInfoReq(&icql, icql.icq_Uin));
    face.status(_("Fetching your ICQ details"));

    if(face.updatedetails()) {
	string fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate;
	string fphone, ffax, fstreet, fcellular, fhomepage, fwcity, fwstate;
	string fwphone, fwfax, fwaddress, fcompany, fdepartment, fjob;
	string fwhomepage, icat1, int1, icat2, int2, icat3, int3, icat4;
	string int4, fabout;

	unsigned char flang1, flang2, flang3, fbyear, fbmonth, fbday, fage, fgender;
	unsigned long fzip, fwzip;
	unsigned short fcountry, fwcountry, foccupation;

	c->getinfo(fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate, fphone, ffax, fstreet, fcellular, fzip, fcountry);
	c->getmoreinfo(fage, fgender, fhomepage, flang1, flang2, flang3, fbday, fbmonth, fbyear);
	c->getworkinfo(fwcity, fwstate, fwphone, fwfax, fwaddress, fwzip, fwcountry, fcompany, fdepartment, fjob, foccupation, fwhomepage);
	fabout = c->getabout();

	icq_UpdateUserInfo(&icql, c->getnick().c_str(), fname.c_str(),
	    lname.c_str(), fprimemail.c_str());

	icq_UpdateMetaInfoHomepage(&icql, fage, fhomepage.c_str(),
	    fbyear, fbmonth, fbday, flang1, flang2, flang3, fgender);

	icq_UpdateMetaInfoSet(&icql, c->getnick().c_str(), fname.c_str(),
	    lname.c_str(), fprimemail.c_str(), fsecemail.c_str(),
	    foldemail.c_str(), fcity.c_str(), fstate.c_str(), fphone.c_str(),
	    ffax.c_str(), fstreet.c_str(), fcellular.c_str(), fzip, fcountry,
	    0, 0);

	icq_UpdateMetaInfoAbout(&icql, fabout.c_str());
    }
}

void centericq::sendfiles(unsigned int uin) {
    int i;
    string msg;
    unsigned long seq;
    icqcontact *c = (icqcontact *) clist.get(uin);
    linkedlist flist;

    if(c->getdirect()) {
	if(face.sendfiles(uin, msg, flist)) {
	    char **files = 0;

	    for(i = 0; i < flist.count; i++) {
		files = (char **) realloc(files, (i+1)*sizeof(char *));
		files[i] = strdup((char *) flist.at(i));
		files[i+1] = 0;
	    }

	    seq = icq_SendFileRequest(&icql, uin, msg.c_str(), files);
	    ihook.addfile(uin, seq, files[0], HIST_MSG_OUT);

	    face.log(_("+ sending file %s to %s, %lu"),
		justfname(files[0]).c_str(), c->getdispnick().c_str(), uin);
	}
    } else {
	face.log(_("+ remote client doesn't support file transfers"));
    }
}

void centericq::updateconf() {
    regsound snd = rsdontchange;
    regcolor clr = rcdontchange;
    string fname;

    if(face.updateconf(snd, clr)) {
	if(snd != rsdontchange) {
	    conf.setregsound(snd);
	    fname = (string) getenv("HOME") + "/.centericq/sounds";
	    unlink(fname.c_str());
	    conf.loadsounds();
	}

	if(clr != rcdontchange) {
	    conf.setregcolor(clr);
	    fname = (string) getenv("HOME") + "/.centericq/colorscheme";
	    unlink(fname.c_str());
	    conf.loadcolors();
	    face.done();
	    face.init();
	    face.draw();
	}

	conf.savemainconfig();
	face.update();
    }
}

void centericq::nonicq(int id) {
    icqcontact *c;

    if(!id) {
	c = clist.addnew(0, false, true);

	if(face.updatedetails(c)) {
	    c->save();
	} else {
	    clist.remove(c->getuin(), true);
	}
    } else {
	c = clist.get(id, true);
	if(face.updatedetails(c)) {
	    c->setdispnick(c->getnick());
	    c->save();
	} else {
	    c->load();
	}
    }
    face.update();
}

void centericq::checkmail() {
    static int fsize = -1;
    const char *fname = getenv("MAIL");
    struct stat st;
    int mcount = 0;
    char buf[512];
    string lastfrom;
    bool prevempty, header;
    FILE *f;

    if(fname)
    if(!stat(fname, &st))
    if(st.st_size != fsize) {

	if(fsize != -1) {
	    if(f = fopen(fname, "r")) {
		prevempty = header = true;

		while(!feof(f)) {
		    freads(f, buf, 512);

		    if(prevempty && !strncmp(buf, "From ", 5)) {
			lastfrom = strim(buf+5);
			mcount++;
			header = true;
		    } else if(header && !strncmp(buf, "From: ", 6)) {
			lastfrom = strim(buf+6);
		    }

		    if(prevempty = !buf[0]) header = false;
		}
		fclose(f);
	    }

	    if(mcount) {
		face.log(_("+ new mail arrived, %d messages"), mcount);
		face.log(_("+ last msg from %s"), lastfrom.c_str());
		clist.get(0)->playsound(EVT_EMAIL);
	    }
	}

	fsize = st.st_size;
    }
}

void centericq::handlesignal(int signum) {
    int status;

    switch(signum) {
	case SIGCHLD:
	    while(wait3(&status, WNOHANG, 0) > 0);
	    signal(SIGCHLD, &handlesignal);
	    break;
    }
}
