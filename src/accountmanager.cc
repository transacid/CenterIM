#include "accountmanager.h"
#include "icqface.h"
#include "icqhook.h"
#include "yahoohook.h"
#include "imcontroller.h"
#include "icqcontacts.h"

#define getcolor(c)     conf.getcolor(c)

accountmanager manager;

accountmanager::accountmanager() {
    fopen = false;
}

accountmanager::~accountmanager() {
}

icqconf::imaccount accountmanager::addcontact() {
    icqconf::imaccount account;
    return account;
}

void accountmanager::exec() {
    dialogbox db;
    protocolname pname;
    icqconf::imaccount account;
    int n, b, i, citem, action, pos;
    string spname, tmp;
    bool fin, proceed;

    face.blockmainscreen();
    fopen = true;

    db.setwindow(new textwindow(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT,
	getcolor(cp_dialog_frame), TW_CENTERED,
	getcolor(cp_dialog_highlight), _(" IM account manager ")));

    db.settree(new treeview(getcolor(cp_dialog_text), getcolor(cp_dialog_selected),
	getcolor(cp_dialog_highlight), getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(getcolor(cp_dialog_text),
	getcolor(cp_dialog_selected), _("Change"), _("Done"), 0));

    db.addautokeys();
    db.idle = &face.dialogidle;

    treeview &t = *db.gettree();

    for(fin = false; !fin; ) {
	t.clear();

	for(pname = icq; pname != protocolname_size; (int) pname += 1) {
	    if(pname != infocard) {
		account = conf.getourid(pname);
		n = t.addnode(0, 0, 0, " " + conf.getprotocolname(pname) + " ");
		citem = ((int) (pname)+1) * 100;

		switch(pname) {
		    case yahoo:
		    case msn:
			t.addleaff(n, 0, citem+1, _(" Login : %s "),
			    account.nickname.c_str());
			break;
		    case icq:
			if(!account.server.empty() && account.port) {
			    tmp = account.server + ":" + i2str(account.port);
			} else {
			    tmp = "";
			}

			t.addleaff(n, 0, citem+9, _(" Server : %s "), tmp.c_str());
			t.addleaff(n, 0, citem+2, _(" UIN : %s "),
			    account.uin ? i2str(account.uin).c_str() : "");
			break;
		}

		t.addleaff(n, 0, citem+5, _(" Password : %s "),
		    string(account.password.size(), '*').c_str());

		if(account.empty()) {
		    t.addnode(n, 0, citem+6, _(" Register "));
		} else {
		    t.addnode(n, 0, citem+7, _(" Update user details "));
		    t.addnode(n, 0, citem+8, _(" Drop "));
		}
	    }
	}

	fin = !db.open(n, b, (void **) &citem) || (b == 1);

	if(!fin)
	if(citem) {
	    pname = (protocolname) (citem/100-1);
	    action = citem-(citem/100)*100;

	    spname = conf.getprotocolname(pname);
	    account = conf.getourid(pname);

	    switch(action) {
		case 1:
		    account.nickname = face.inputstr(spname + _(" user name: "),
			account.nickname);
		    break;
		case 2:
		    account.uin = strtoul(face.inputstr(spname + _(" uin: "),
			i2str(account.uin)).c_str(), 0, 0);
		    break;
		case 5:
		    account.password = face.inputstr(spname + _(" password: "),
			account.password, '*');
		    break;
		case 6:
		    if(!account.empty()) {
			face.status(_("Drop the account information first!"));
		    } else {
			imcontrol.registration(account);
		    }
		    break;
		case 7:
		    imcontrol.updateinfo(account);
		    break;
		case 8:
		    if(!gethook(pname).online()) {
			account = icqconf::imaccount(pname);
		    } else {
			face.status(_("You have to disconnect the service first!"));
		    }
		    break;
		case 9:
		    tmp = account.server + ":" + i2str(account.port);
		    tmp = face.inputstr(spname + _(" server address: "), tmp);

		    if(!tmp.empty()) {
			if((pos = tmp.find(":")) != -1) {
			    account.server = tmp.substr(0, pos);
			    account.port = strtoul(tmp.substr(pos+1).c_str(), 0, 0);
			} else {
			    account.server = tmp;
			    account.port = 0;
			}
		    }
		    break;
	    }

	    conf.setourid(account);
	}
    }

    db.close();
    face.unblockmainscreen();

    clist.checkdefault();
    conf.save();
    face.relaxedupdate();
    fopen = false;
}

bool accountmanager::isopen() const {
    return fopen;
}
