#include "imcontroller.h"
#include "icqface.h"
#include "icqhook.h"
#include "aimhook.h"
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

void imcontroller::aimupdateprofile() {
    icqcontact *c = clist.get(contactroot);

    if(ahook.logged()) {
	c->clear();
	ahook.requestinfo(imcontact(conf.getourid(aim).uin, aim));

	if(face.updatedetails(0, aim)) {
	    ahook.sendupdateuserinfo(*c, "");
	}
    } else {
	face.status(_("You must be logged to the AIM network to update your profile"));
    }
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
