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
    set<hookcapab::enumeration> capab;
    string spname, tmp;
    bool fin, proceed;

    face.blockmainscreen();
    fopen = true;

    db.setwindow(new textwindow(0, 0, face.sizeDlg.width, face.sizeDlg.height,
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
		capab = gethook(pname).getCapabs();

		if(!account.empty()) {
		    tmp = "";

		    if(!account.server.empty() && account.port)
			tmp = account.server + ":" + i2str(account.port);

		    t.addleaff(n, 0, citem+9, _(" Server : %s "), tmp.c_str());
		}

		switch(pname) {
		    case icq:
			t.addleaff(n, 0, citem+2, _(" UIN : %s "),
			    account.uin ? i2str(account.uin).c_str() : "");
			break;

		    default:
			t.addleaff(n, 0, citem+1, _(" Login : %s "),
			    account.nickname.c_str());
			break;
		}

		t.addleaff(n, 0, citem+5, capab.count(hookcapab::optionalpassword) ?
		    _(" Password (optional) : %s ") : _(" Password : %s "),
		    string(account.password.size(), '*').c_str());

		if(account.empty()) {
		    t.addnode(n, 0, citem+6, _(" Register "));
		} else {

		    if(capab.count(hookcapab::changedetails))
			t.addnode(n, 0, citem+7,
			    pname == msn ?
			    _(" Change nickname ") :
			    _(" Update user details "));

		    if(capab.count(hookcapab::setaway))
			t.addnode(n, 0, citem+10, _(" Set away message "));

		    if(pname == irc)
			t.addnode(n, 0, citem+11, _(" Channel manager "));

		    if(capab.count(hookcapab::synclist))
			t.addnode(n, 0, citem+12, _(" Synchronize contact list "));

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
	    abstracthook &hook = gethook(pname);

	    switch(action) {
		case 1:
		    tmp = face.inputstr(spname + _(" user name: "), account.nickname);
		    if(face.getlastinputkey() != KEY_ESC && !tmp.empty()) account.nickname = tmp;
		    break;

		case 2:
		    i = strtoul(face.inputstr(spname + _(" uin: "), i2str(account.uin)).c_str(), 0, 0);
		    if(face.getlastinputkey() != KEY_ESC && i) account.uin = i;
		    break;

		case 5:
		    tmp = face.inputstr(spname + _(" password: "), account.password, '*');
		    if(face.getlastinputkey() != KEY_ESC &&
		      (!tmp.empty() || capab.count(hookcapab::optionalpassword)))
			account.password = tmp;
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
		    if(!hook.online()) {
			account = icqconf::imaccount(pname);
		    } else {
			face.status(_("You have to disconnect the service first!"));
		    }
		    break;

		case 9:
		    tmp = "";
		    if(!account.server.empty())
			tmp = account.server + ":" + i2str(account.port);

		    tmp = face.inputstr(spname + _(" server address: "), tmp);

		    if(face.getlastinputkey() != KEY_ESC && !tmp.empty()) {
			if((pos = tmp.find(":")) != -1) {
			    account.server = tmp.substr(0, pos);
			    account.port = strtoul(tmp.substr(pos+1).c_str(), 0, 0);
			} else {
			    account.server = tmp;
			    account.port = 0;
			}
		    }
		    break;

		case 10:
		    if(face.edit(tmp = conf.getawaymsg(pname),
		    spname + ": " + _("away message"))) {
			conf.setawaymsg(pname, tmp);
		    }
		    break;

		case 11:
		    imcontrol.channels(account);
		    break;

		case 12:
		    if(!hook.online()) {
			face.status(_("You must be logged in to preform the synchronization"));
		    } else {
			imcontrol.synclist(account);
			fin = true;
		    }
		    break;
	    }

	    if(action != 7) {
		conf.setourid(account);
	    }
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
