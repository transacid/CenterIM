#include "imcontroller.h"
#include "icqface.h"

#define clr(c)     conf.getcolor(c)

imcontroller imcontrol;

imcontroller::imcontroller() {
}

imcontroller::~imcontroller() {
}

void imcontroller::icqregdialog(unsigned int &ruin, string &rpasswd) {
    bool finished, newuin, success, socks = false;
    int nmode, ndet, nopt, n, i, b, nproxy;
    string phidden, pcheck, socksuser, sockspass, psockscheck, tmp;
    string prserv = conf.getsockshost() + ":" + i2str(conf.getsocksport());
    dialogbox db;

    if(conf.getsockshost().empty()) prserv = "";

    finished = success = false;
    icq_Russian = 0;
    newuin = true;
    ruin = 0;
    rnick = rfname = rlname = remail = "";

    db.setwindow(new textwindow(0, 0, BIGDIALOG_WIDTH, BIGDIALOG_HEIGHT,
	clr(cp_dialog_frame), TW_CENTERED, clr(cp_dialog_highlight),
	_(" Register on ICQ network ")));

    db.settree(new treeview(clr(cp_dialog_text), clr(cp_dialog_selected),
	clr(cp_dialog_highlight), clr(cp_dialog_text)));

    db.setbar(new horizontalbar(clr(cp_dialog_text), clr(cp_dialog_selected),
	_("Change"), _("Done"), 0));

    while(!finished) {
	db.gettree()->clear();
	
	i = db.gettree()->addnode(_(" Details "));
	db.gettree()->addleaff(ndet, 0, (void *)  2, _(" Password to set : %s "), phidden.assign(rpasswd.size(), '*').c_str());
	db.gettree()->addleaff(ndet, 0, (void *)  3, _(" Check the password : %s "), phidden.assign(pcheck.size(), '*').c_str());
	db.gettree()->addleaff(ndet, 0, (void *)  4, _(" Nickname : %s "), rnick.c_str());
	db.gettree()->addleaff(ndet, 0, (void *)  5, _(" E-Mail : %s "), remail.c_str());
	db.gettree()->addleaff(ndet, 0, (void *)  6, _(" First name : %s "), rfname.c_str());
	db.gettree()->addleaff(ndet, 0, (void *)  7, _(" Last name : %s "), rlname.c_str());

	i = db.gettree()->addnode(_(" Options "));


	void *p;
	finished = !db.open(n, b, &p);
	i = (int) p;

	if(!finished)
	switch(b) {
	    case 0:
		switch(i) {
		    case 1: newuin = !newuin; break;
		    case 2: rpasswd = inputstr(_("Password: "), rpasswd, '*'); break;
		    case 3: pcheck = inputstr(_("Check the password you entered: "), pcheck, '*'); break;
		    case 4: rnick = inputstr(_("Nickname to set: "), rnick); break;
		    case 5: remail = inputstr(_("E-Mail: "), remail); break;
		    case 6: rfname = inputstr(_("First name: "), rfname); break;
		    case 7: rlname = inputstr(_("Last name: "), rlname); break;
		    case 8:
			conf.setregsound(
			    conf.getregsound() == rscard ? rsspeaker :
			    conf.getregsound() == rsspeaker ? rsdisable :
			    rscard);
			break;
		    case 9:
			conf.setregcolor(
			    conf.getregcolor() == rcdark ? rcblue :
			    conf.getregcolor() == rcblue ? rcdark : rcblue);
			break;
		    case 10:
			ruin = atol(inputstr(_("Existing UIN: "), i2str(ruin)).c_str());
			break;
		    case 11:
			icq_Russian = icq_Russian == 1 ? 0 : 1;
			break;
		    case 12:
			tmp = inputstr(_("ICQ server to use: "), serv);
			if(!tmp.empty()) serv = tmp;
			break;
		    case 13: socks = !socks; break;
		    case 14:
			tmp = inputstr(_("SOCKS proxy server to use: "), prserv);
			if(!tmp.empty()) prserv = tmp;
			break;
		    case 16:
			sockspass = inputstr(_("SOCKS proxy password: "), sockspass, '*');
			conf.setsocksuser(socksuser, sockspass);
			break;
		    case 17:
			socksuser = inputstr(_("SOCKS proxy user name: "), socksuser);
			conf.setsocksuser(socksuser, sockspass);
			break;
		    case 18:
			psockscheck = inputstr(_("Check the SOCKS proxy password: "), psockscheck, '*');
			break;
		    case 20: conf.setsavepwd(!conf.getsavepwd()); break;
		}
		break;
	    case 1:
		if(newuin && (rpasswd != pcheck)) {
		    status(_("Passwords do not match"));
		} else if(newuin && (sockspass != psockscheck)) {
		    status(_("SOCKS proxy passwords do not match"));
		} else {
		    if(socks) {
			conf.setsockshost(prserv);
		    } else {
			conf.setsockshost("");
		    }

		    conf.setserver(serv);
		    finished = success = true;

		    if(newuin) ruin = 0; else
		    if(!newuin && !ruin) success = false;
		}
		break;
	}
    }

    db.close();

    return success;
}

void imcontroller::icqregistration(imaccount &account) {
    unsigned int ruin;
    int sockfd;
    bool finished;
    string rpasswd, nick, fname, lname, email, socksuser, sockspass;
    fd_set fds;
    FILE *f;

    if(icqregdialog(ruin, rpasswd)) {
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
		    ihook.regdisconnected(&icql, 0);
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
