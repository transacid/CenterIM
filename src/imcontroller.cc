/*
*
* centericq protocol specific user interface related routines
* $Id: imcontroller.cc,v 1.56 2004/07/20 22:16:40 konst Exp $
*
* Copyright (C) 2001-2004 by Konstantin Klyagin <konst@konst.org.ua>
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

#include "imcontroller.h"
#include "icqface.h"
#include "icqhook.h"
#include "yahoohook.h"
#include "aimhook.h"
#include "irchook.h"
#include "jabberhook.h"
#include "msnhook.h"
#include "gaduhook.h"
#include "icqcontacts.h"
#include "eventmanager.h"

#define clr(c)     conf.getcolor(c)

imcontroller imcontrol;

imcontroller::imcontroller() {
}

imcontroller::~imcontroller() {
}

bool imcontroller::regdialog(protocolname pname) {
    bool finished, success;
    int n, i, b;
    string pcheck;
    string prserv = conf.getsockshost() + ":" + i2str(conf.getsocksport());
    dialogbox db;

    ruin = 0;
    finished = success = false;
    rnick = rfname = rlname = remail = rpasswd = "";

    rserver = icqconf::defservers[pname].server + ":" +
	i2str(icqconf::defservers[pname].port);

    db.setwindow(new textwindow(0, 0, face.sizeDlg.width, face.sizeDlg.height,
	clr(cp_dialog_frame), TW_CENTERED));

    db.getwindow()->set_titlef(clr(cp_dialog_highlight),
	_(" Register on the %s network "),
	conf.getprotocolname(pname).c_str());

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

	if(pname != gadu) {
	    i = db.gettree()->addnode(_(" Details "));
	    t.addleaff(i, 0, 8, _(" Server to use : %s "), rserver.c_str());
	}

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
		} else if(pname == icq && rpasswd.size() < 6) {
		    face.status(_("Password must be at least 6 chars long"));
		} else {
		    finished = success = true;
		}
		break;
	}
    }

    db.close();

    return success;
}

bool imcontroller::uinreg(icqconf::imaccount &account) {
    bool fin, success;
    abstracthook &hook = gethook(account.pname);

    if(success = regdialog(account.pname)) {
	unlink((conf.getdirname() + conf.getprotocolname(account.pname) + "-infoset").c_str());
	face.progress.show(_(" Registration progress "));

	for(fin = false; !fin; ) {
	    face.progress.log(_("Connecting to the server %s ..."), rserver.c_str());

	    if(hook.regconnect(rserver)) {
		while(!fin) {
		    face.progress.log(_("Sending request"));

		    while(!ruin && !fin) {
			fin = hook.regattempt(ruin, rpasswd, remail);

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
			ofstream f((conf.getdirname() + conf.getprotocolname(account.pname) + "-infoset").c_str());
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
		fin = (face.ask(_("Unable to connect to icq server. Retry?"),
		    ASK_YES | ASK_NO) == ASK_NO);
	    }
	}

	face.progress.hide();
    }

    return success;
}

bool imcontroller::jabberregistration(icqconf::imaccount &account) {
#ifdef BUILD_JABBER
    bool success;
    string err;
    int pos;

    if(success = regdialog(account.pname)) {
	unlink((conf.getdirname() + "jabber-infoset").c_str());
	face.progress.show(_(" Registration progress "));

	face.progress.log(_("Trying to register %s at %s ..."),
	    rnick.c_str(), rserver.c_str());

	if(success = jhook.regnick(rnick, rpasswd, rserver, err)) {
	    account.nickname = rnick;
	    account.password = rpasswd;
	    account.server = rserver;

	    if((pos = account.server.find(":")) != -1) {
		account.port = atoi(account.server.substr(pos+1).c_str());
		account.server.erase(pos);
	    } else {
		account.port = icqconf::defservers[jabber].port;
	    }

	    face.progress.log(_("The Jabber ID was successfully registered"));

	    conf.checkdir();
	    ofstream f((conf.getdirname() + "jabber-infoset").c_str());
	    if(f.is_open()) {
		f << rnick << endl <<
		    remail << endl <<
		    rfname << endl <<
		    rlname << endl;
		f.close();
	    }

	} else {
	    string msgerr = _("Failed");
	    if(!err.empty()) msgerr += (string) ": " + err;
	    face.progress.log("%s", msgerr.c_str());

	}

	face.progress.log(_("Disconnected"));
	sleep(2);
	face.progress.hide();
    }

    return success;
#else
    return false;
#endif
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

void imcontroller::aimupdateprofile() {
#ifdef BUILD_AIM
    icqcontact *c = clist.get(contactroot);

    c->clear();
    ahook.requestinfo(imcontact(conf.getourid(aim).nickname, aim));
    if(face.updatedetails(0, aim)) ahook.sendupdateuserinfo(*c);
#endif
}

void imcontroller::msnupdateprofile() {
#ifdef BUILD_MSN
    if(mhook.logged()) {
	mhook.requestinfo(imcontact(conf.getourid(msn).nickname, msn));
	string tmp = face.inputstr(_("new MSN friendly nick: "), clist.get(contactroot)->getnick());

	if(face.getlastinputkey() != KEY_ESC && !tmp.empty()) {
	    icqcontact *c = clist.get(contactroot);
	    c->setnick(tmp);
	    mhook.sendupdateuserinfo(*c);
	}
    } else {
	face.status(_("You must be logged to the MSN network to update the friendly nick"));
    }
#endif
}

void imcontroller::jabberupdateprofile() {
#ifdef BUILD_JABBER
    if(jhook.logged()) {
	icqcontact *c;

	(c = clist.get(contactroot))->clear();
	jhook.requestinfo(imcontact(conf.getourid(jabber).nickname, jabber));

	if(face.updatedetails(0, jabber))
	    jhook.sendupdateuserinfo(*c);

    } else {
	face.status(_("You must be logged to the Jabber network to update your details"));
    }
#endif
}

void imcontroller::gaduupdateprofile() {
#ifdef BUILD_GADU
    if(ghook.logged()) {
	icqcontact *c;

	(c = clist.get(contactroot))->clear();
	ghook.requestinfo(imcontact(conf.getourid(gadu).uin, jabber));

	if(face.updatedetails(0, gadu))
	    ghook.sendupdateuserinfo(*c);

    } else {
	face.status(_("You must be logged to the Gadu-Gadu network to update your details"));
    }
#endif
}

void imcontroller::registration(icqconf::imaccount &account) {
    switch(account.pname) {
	case icq:
#ifdef HAVE_LIBJPEG
	case gadu:
#endif
	    uinreg(account);
	    break;
	case jabber:
	    jabberregistration(account);
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
	case msn: msnupdateprofile(); break;
	case jabber: jabberupdateprofile(); break;
	case gadu: gaduupdateprofile(); break;
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
	    << (int) pname << "\t"
	    << (onlineonly ? "1" : "0") << "\t"
	    << uin << "\t"
	    << minage << "\t"
	    << maxage << "\t"
	    << country << "\t"
	    << language << "\t"
	    << (int) agerange << "\t"
	    << (int) gender << "\t"
	    << firstname << "\t"
	    << lastname << "\t"
	    << nick << "\t"
	    << city << "\t"
	    << state << "\t"
	    << company << "\t"
	    << department << "\t"
	    << position << "\t"
	    << email << "\t"
	    << room << "\t"
	    << (sincelast ? "1" : "0") << "\t"
	    << (reverse ? "1" : "0") << "\t"
	    << service << "\t"
	    << (photo ? "1" : "0") << "\t"
	    << kwords << "\t"
	    << randomgroup;

	vector<pair<string, string> >::const_iterator
	    ifp = flexparams.begin();

	while(ifp != flexparams.end()) {
	    of << ifp->first << "\t" << ifp->second << "\t";
	    ++ifp;
	}

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
    string buf, flexparam;
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
		sincelast = getfield(buf) == "1";
		reverse = getfield(buf) == "1";
		service = getfield(buf);
		photo = getfield(buf) == "1";
		kwords = getfield(buf);
		randomgroup = strtoul(getfield(buf).c_str(), 0, 0);

		while(!buf.empty())
		    if(!(flexparam = getfield(buf)).empty())
			flexparams.push_back(make_pair(flexparam, getfield(buf)));

		break;
	    }
	}

	f.close();
    }

    return r;
}
