#include "imcontroller.h"
#include "icqface.h"

#define clr(c)     conf.getcolor(c)

imcontroller imcontrol;

imcontroller::imcontroller() {
}

imcontroller::~imcontroller() {
}

bool imcontroller::icqregdialog() {
/*
    bool finished, success;
    int n, i, b;
    string pcheck;
    string prserv = conf.getsockshost() + ":" + i2str(conf.getsocksport());
    dialogbox db;

    ruin = 0;
    finished = success = false;
    rnick = rfname = rlname = remail = rpasswd = "";

    db.setwindow(new textwindow(0, 0, BIGDIALOG_WIDTH, BIGDIALOG_HEIGHT,
	clr(cp_dialog_frame), TW_CENTERED, clr(cp_dialog_highlight),
	_(" Register on ICQ network ")));

    db.settree(new treeview(clr(cp_dialog_text), clr(cp_dialog_selected),
	clr(cp_dialog_highlight), clr(cp_dialog_text)));

*//*
    db.setbar(new horizontalbar(clr(cp_dialog_text), clr(cp_dialog_selected),
	_("Change"), _("Done"), 0));

    treeview &t = *db.gettree();

    while(!finished) {
	t.clear();
	
	i = db.gettree()->addnode(_(" Details "));
	t.addleaff(i, 0, 2, _(" Password to set : %s "), string(rpasswd.size(), '*').c_str());
	t.addleaff(i, 0, 3, _(" Check the password : %s "), string(pcheck.size(), '*').c_str());
	t.addleaff(i, 0, 4, _(" Nickname : %s "), rnick.c_str());
	t.addleaff(i, 0, 5, _(" E-Mail : %s "), remail.c_str());
	t.addleaff(i, 0, 6, _(" First name : %s "), rfname.c_str());
	t.addleaff(i, 0, 7, _(" Last name : %s "), rlname.c_str());

	finished = !db.open(n, b, (void **) &i);

	if(!finished)
	switch(b) {
	    case 0:
		switch(i) {
*//*
		    case 2: rpasswd = inputstr(_("Password: "), rpasswd, '*'); break;
		    case 3: pcheck = inputstr(_("Check the password you entered: "), pcheck, '*'); break;
		    case 4: rnick = inputstr(_("Nickname to set: "), rnick); break;
		    case 5: remail = inputstr(_("E-Mail: "), remail); break;
		    case 6: rfname = inputstr(_("First name: "), rfname); break;
		    case 7: rlname = inputstr(_("Last name: "), rlname); break;
		}
		break;

	    case 1:
		if(rpasswd != pcheck) {
		    status(_("Passwords do not match"));
		} else {
		    finished = success = true;
		}
		break;
	}
    }

    db.close();

    return success;
*/
    return false;
}

void imcontroller::icqregistration(imaccount &account) {
    unsigned int ruin;
    int sockfd;
    bool finished;
    string rpasswd, nick, fname, lname, email, socksuser, sockspass;
    fd_set fds;
    FILE *f;

    if(icqregdialog()) {
	face.progress.show(_(" Registration progress "));
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
		ihook.regdisconnected(&icql, 0);
		sleep(2);
	    } else {
		if(face.ask(_("Unable to connect to the icq server. Retry?"),
		ASK_YES | ASK_NO) == ASK_NO) {
		    finished = true;
		}
	    }
	}

	face.progress.hide();
    }
}

void imcontroller::icqupdatedetails(imaccount &account) {
}

void imcontroller::register(imaccount &account) {
    switch(account.pname) {
	case icq:
	    icqregistration(account);
	    break;
	case yahoo:
	    break;
    }
}

void imcontroller::updateinfo(imaccount &account) {
    switch(account.pname) {
	case icq:
	    icqupdatedetails(account);
	    break;
	case yahoo:
	    break;
    }
}
