#include "imcontroller.h"
#include "icqface.h"
#include "icqhook.h"
#include "aimhook.h"
#include "irchook.h"
#include "icqcontacts.h"

#define clr(c)     conf.getcolor(c)

imcontroller imcontrol;

imcontroller::imcontroller() {
}

imcontroller::~imcontroller() {
}

bool imcontroller::icqregdialog() {
    bool finished, success;
    int n, i, b;
    string pcheck;
    string prserv = conf.getsockshost() + ":" + i2str(conf.getsocksport());
    dialogbox db;

    ruin = 0;
    finished = success = false;
    rnick = rfname = rlname = remail = rpasswd = "";
    rserver = "login.icq.com:5190";

    db.setwindow(new textwindow(0, 0, face.sizeDlg.width, face.sizeDlg.height,
	clr(cp_dialog_frame), TW_CENTERED, clr(cp_dialog_highlight),
	_(" Register on ICQ network ")));

    db.settree(new treeview(clr(cp_dialog_text), clr(cp_dialog_selected),
	clr(cp_dialog_highlight), clr(cp_dialog_text)));

    db.setbar(new horizontalbar(clr(cp_dialog_text), clr(cp_dialog_selected),
	_("Change"), _("Go ahead"), 0));

    db.addautokeys();
    treeview &t = *db.gettree();

    while(!finished) {
	t.clear();
	
	i = db.gettree()->addnode(_(" Details "));
	t.addleaff(i, 0, 4, _(" Nickname : %s "), rnick.c_str());
	t.addleaff(i, 0, 5, _(" E-Mail : %s "), remail.c_str());
	t.addleaff(i, 0, 6, _(" First name : %s "), rfname.c_str());
	t.addleaff(i, 0, 7, _(" Last name : %s "), rlname.c_str());

	i = db.gettree()->addnode(_(" Password "));
	t.addleaff(i, 0, 2, _(" Password to set : %s "), string(rpasswd.size(), '*').c_str());
	t.addleaff(i, 0, 3, _(" Check the password : %s "), string(pcheck.size(), '*').c_str());

	i = db.gettree()->addnode(_(" Details "));
	t.addleaff(i, 0, 8, _(" Server to use : %s "), rserver.c_str());

	finished = !db.open(n, b, (void **) &i);

	if(!finished)
	switch(b) {
	    case 0:
		switch(i) {
		    case 2: rpasswd = face.inputstr(_("Password: "), rpasswd, '*'); break;
		    case 3: pcheck = face.inputstr(_("Check the password you entered: "), pcheck, '*'); break;
		    case 4: rnick = face.inputstr(_("Nickname to set: "), rnick); break;
		    case 5: remail = face.inputstr(_("E-Mail: "), remail); break;
		    case 6: rfname = face.inputstr(_("First name: "), rfname); break;
		    case 7: rlname = face.inputstr(_("Last name: "), rlname); break;
		    case 8: rserver = face.inputstr(_("Server: "), rserver); break;
		}
		break;

	    case 1:
		if(rpasswd != pcheck) {
		    face.status(_("Passwords do not match"));
		} else if(rpasswd.empty()) {
		    face.status(_("Password must be entered"));
		} else {
		    finished = success = true;
		}
		break;
	}
    }

    db.close();

    return success;
}

bool imcontroller::icqregistration(icqconf::imaccount &account) {
    bool fin, success;

    if(success = icqregdialog()) {
	unlink((conf.getdirname() + "icq-infoset").c_str());
	face.progress.show(_(" Registration progress "));

	for(fin = false; !fin; ) {
	    face.progress.log(_("Connecting to the server %s ..."), rserver.c_str());

	    if(ihook.regconnect(rserver)) {
		while(!fin) {
		    face.progress.log(_("Sending request"));

		    while(!ruin && !fin) {
			fin = ihook.regattempt(ruin, rpasswd);

			if(!fin) {
			    if(fin = (face.ask(_("Timed out waiting for a new uin. Retry?"),
			    ASK_YES | ASK_NO, ASK_YES) == ASK_NO)) {
				success = false;
			    } else {
				face.progress.log(_("Retrying.."));
			    }
			}
		    }

		    if(success) {
			face.progress.log(_("New UIN received, %lu"), ruin);
			account.uin = ruin;
			account.password = rpasswd;

			conf.checkdir();
			ofstream f((conf.getdirname() + "icq-infoset").c_str());
			if(f.is_open()) {
			    f << rnick << endl <<
				remail << endl <<
				rfname << endl <<
				rlname << endl;
			    f.close();
			}
		    }
		}

		face.progress.log(_("Disconnected"));
		sleep(2);
	    } else {
		fin = (face.ask(_("Unable to connect to the icq server. Retry?"),
		    ASK_YES | ASK_NO) == ASK_NO);
	    }
	}

	face.progress.hide();
    }

    return success;
}

void imcontroller::icqupdatedetails() {
    icqcontact *c = clist.get(contactroot);

    if(ihook.logged()) {
	c->clear();
	ihook.requestinfo(imcontact(conf.getourid(icq).uin, icq));

	if(face.updatedetails()) {
	    ihook.sendupdateuserinfo(*c);
	}
    } else {
	face.status(_("You must be logged to the ICQ network to update the details"));
    }
}

void imcontroller::icqsynclist() {
    bool fin;
    int tobestored, attempt = 1;
    string msg;

    face.progress.show(_(" Contact list synchronization "));
    face.progress.log(_("Starting the synchronization process"), tobestored);

    for(fin = false; !fin; ) {
	ihook.getsyncstatus(tobestored);
	face.progress.log(_("Attempt #%d, %d contacts to be stored server-side"), attempt, tobestored);

	msg = "";
	if(!tobestored) msg = _("Finished");
	else if(!ihook.logged()) msg = _("Disconnected");
	else if(attempt > 10) msg = _("Too many attempts, gave up");

	if(!msg.empty()) {
	    face.progress.log("%s", msg.c_str());
	    fin = true;
	    sleep(2);
	}

	if(!fin) {
	    ihook.synclist();
	}

	attempt++;
    }

    face.progress.hide();
}

void imcontroller::aimupdateprofile() {
    icqcontact *c = clist.get(contactroot);

    c->clear();
    ahook.requestinfo(imcontact(conf.getourid(aim).uin, aim));

    if(face.updatedetails(0, aim)) {
	ahook.sendupdateuserinfo(*c, "");
    }
}

void imcontroller::ircchannels() {
    bool finished = false, success;
    int n, i, b;
    dialogbox db;
    string name, st;

    vector<irchook::channelInfo> channels;
    vector<irchook::channelInfo>::iterator ic;

    db.setwindow(new textwindow(0, 0, face.sizeDlg.width, face.sizeDlg.height,
	clr(cp_dialog_frame), TW_CENTERED, clr(cp_dialog_highlight),
	_(" IRC channels manager ")));

    db.settree(new treeview(clr(cp_dialog_text), clr(cp_dialog_selected),
	clr(cp_dialog_highlight), clr(cp_dialog_text)));

    db.setbar(new horizontalbar(clr(cp_dialog_text), clr(cp_dialog_selected),
	_("Add"), _("Remove"), _("Join/leave"), _("Show on the list"), _("Done"), 0));

    channels = irhook.getautochannels();

    db.addkey(KEY_IC, 0);
    db.addkey(KEY_DC, 1);
    db.addkey(' ', 2);
    db.addautokeys();

    treeview &t = *db.gettree();

    while(!finished) {
	t.clear();

	i = db.gettree()->addnode(_(" Channels "));

	for(ic = channels.begin(); ic != channels.end(); ic++) {
	    st = (string) (ic->joined ? _("joined") : _("not joined")) + ", " +
		(ic->contactlist ? _("shown") : _("hidden"));

	    t.addleaff(i, 0, 0, " %s [%s] ", ic->name.c_str(), st.c_str());
	}

	finished = !db.open(n, b, (void **) &i);

	if(!finished)
	switch(b) {
	    case 0:
		name = face.inputstr(_("channel name to add: "));
		if(!name.empty()) channels.push_back(name);
		break;

	    case 1:
		if(n > 1) {
		    ic = channels.begin()+n-2;
		    channels.erase(ic);
		}
		break;

	    case 2:
		if(n > 1) {
		    ic = channels.begin()+n-2;
		    ic->joined = !ic->joined;
		}
		break;

	    case 3:
		if(n > 1) {
		    ic = channels.begin()+n-2;
		    ic->contactlist = !ic->contactlist;
		}
		break;

	    case 4:
		finished = true;
		irhook.setautochannels(channels);
		break;
	}
    }

    db.close();
}

void imcontroller::registration(icqconf::imaccount &account) {
    switch(account.pname) {
	case icq:
	    icqregistration(account);
	    break;
	default:
	    face.status("[" + conf.getprotocolname(account.pname) + "] " +
		_("registration is not supported"));
	    break;
    }
}

void imcontroller::updateinfo(icqconf::imaccount &account) {
    switch(account.pname) {
	case icq: icqupdatedetails(); break;
	case aim: aimupdateprofile(); break;
    }
}

void imcontroller::channels(icqconf::imaccount &account) {
    switch(account.pname) {
	case irc: ircchannels(); break;
    }
}

void imcontroller::synclist(icqconf::imaccount &account) {
    switch(account.pname) {
	case icq: icqsynclist(); break;
    }
}

// ----------------------------------------------------------------------------

void imsearchparams::save(const string &prname) const {
    string fname = conf.getconfigfname("search-profiles"), buf, orig;

    ifstream rf;
    ofstream of;

    of.open((fname + ".tmp").c_str());

    if(of.is_open()) {
	rf.open(fname.c_str());

	if(rf.is_open()) {
	    while(!rf.eof()) {
		getstring(rf, orig);
		if(getword(buf = orig, "\t") != prname)
		    of << orig << endl;
	    }

	    rf.close();
	    rf.clear();
	}

	of.close();
	of.clear();

	rename((fname + ".tmp").c_str(), fname.c_str());
    }

    of.open(fname.c_str(), ios::app);

    if(of.is_open()) {
	of << prname << "\t"
	    << dec << (int) pname << "\t"
	    << (onlineonly ? "1" : "0") << "\t"
	    << dec << uin << "\t"
	    << dec << minage << "\t"
	    << dec << maxage << "\t"
	    << dec << country << "\t"
	    << dec << language << "\t"
	    << dec << (int) agerange << "\t"
	    << dec << (int) gender << "\t"
	    << firstname << "\t"
	    << lastname << "\t"
	    << nick << "\t"
	    << city << "\t"
	    << state << "\t"
	    << company << "\t"
	    << department << "\t"
	    << position << "\t"
	    << email << "\t"
	    << room << endl;

	of.close();
    }
}

static string getfield(string &buf) {
    string r;
    int pos;

    if((pos = buf.find("\t")) == -1)
	pos = buf.size();

    r = buf.substr(0, pos);
    buf.erase(0, pos+1);

    return r;
}

bool imsearchparams::load(const string &prname) {
    ifstream f(conf.getconfigfname("search-profiles").c_str());
    string buf;
    bool r = false;

    if(f.is_open()) {
	while(!f.eof()) {
	    getstring(f, buf);

	    if(r = (getword(buf, "\t") == prname)) {
		pname = (protocolname) strtoul(getfield(buf).c_str(), 0, 0);
		onlineonly = getfield(buf) == "1";
		uin = strtoul(getfield(buf).c_str(), 0, 0);
		minage = strtoul(getfield(buf).c_str(), 0, 0);
		maxage = strtoul(getfield(buf).c_str(), 0, 0);
		country = strtoul(getfield(buf).c_str(), 0, 0);
		language = strtoul(getfield(buf).c_str(), 0, 0);
		agerange = (ICQ2000::AgeRange) strtoul(getfield(buf).c_str(), 0, 0);
		gender = (imgender) strtoul(getfield(buf).c_str(), 0, 0);
		firstname = getfield(buf);
		lastname = getfield(buf);
		nick = getfield(buf);
		city = getfield(buf);
		state = getfield(buf);
		company = getfield(buf);
		department = getfield(buf);
		position = getfield(buf);
		email = getfield(buf);
		room = getfield(buf);
		break;
	    }
	}

	f.close();
    }

    return r;
}
