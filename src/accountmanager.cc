/*
*
* centericq account manager dialog implementation
* $Id: accountmanager.cc,v 1.43 2005/01/26 23:52:46 konst Exp $
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

#include "accountmanager.h"
#include "icqface.h"
#include "icqhook.h"
#include "yahoohook.h"
#include "imcontroller.h"
#include "icqcontacts.h"
#include "impgp.h"

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
    int n, b, i, citem, action, pos;
    set<hookcapab::enumeration> capab;
    string spname, tmp;
    bool fin;
    icqconf::imaccount a;
    protocolname pname;

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

    map<protocolname, bool> mod;
    for(protocolname pname = icq; pname != protocolname_size; pname++)
	mod[pname] = false;

    for(fin = false; !fin; ) {
	t.clear();

	for(pname = icq; pname != protocolname_size; pname++) {
	    if(pname != rss && gethook(pname).enabled()) {
		a = conf.getourid(pname);

		n = t.addnode(0, 0, 0, " " + conf.getprotocolname(a.pname) + " ");
		citem = ((int) (a.pname)+1) * 100;
		capab = gethook(a.pname).getCapabs();

		if(!a.empty()) {
		    tmp = "";

		    if(!a.server.empty() && a.port)
			tmp = a.server + ":" + i2str(a.port);

		    t.addleaff(n, 0, citem+9, _(" Server : %s "), tmp.c_str());

		    if(capab.count(hookcapab::ssl))
			t.addleaff(n, 0, citem+13, _(" Secured : %s "),
			    stryesno(a.additional["ssl"] == "1"));
		}

		switch(a.pname) {
		    case icq:
		    case gadu:
			t.addleaff(n, 0, citem+2, _(" UIN : %s "),
			    a.uin ? i2str(a.uin).c_str() : "");
			break;

		    default:
			t.addleaff(n, 0, citem+1, _(" Login : %s "),
			    a.nickname.c_str());
			break;
		}

		t.addleaff(n, 0, citem+5, capab.count(hookcapab::optionalpassword) ?
		    _(" Password (optional) : %s ") : _(" Password : %s "),
		    string(a.password.size(), '*').c_str());

		if(!a.empty())
		switch(a.pname) {
		    case jabber:
			t.addleaff(n, 0, citem+14, _(" Priority : %s "),
			    a.additional["prio"].c_str());
			break;

		    case livejournal:
			t.addleaff(n, 0, citem+11, _(" Import friend list : %s "),
			    stryesno(a.additional["importfriends"] == "1"));
			break;

		    case irc:
			t.addleaff(n, 0, citem+12, _(" NickServ password (optional) : %s "),
			    string(a.additional["nickpass"].size(), '*').c_str());
			break;
		}

		if(a.empty()) {
		    t.addnode(n, 0, citem+6, _(" Register "));

		} else {
		#ifdef HAVE_GPGME
		    if(capab.count(hookcapab::pgp)) {
			t.addleaff(n, 0, citem+15, _(" OpenPGP key: %s "),
			    a.additional["pgpkey"].empty() ? "none"
				: a.additional["pgpkey"].c_str());

			if(!a.additional["pgpkey"].empty())
			    t.addleaff(n, 0, citem+16, _(" Key passphrase: %s "),
				string(a.additional["pgppass"].size(), '*').c_str());
		    }
		#endif

		    if(capab.count(hookcapab::changedetails))
			t.addnode(n, 0, citem+7,
			    a.pname == msn ?
			    _(" Change nickname ") :
			    _(" Update user details "));

		    if(capab.count(hookcapab::setaway))
			t.addnode(n, 0, citem+10, _(" Set away message "));

		    t.addnode(n, 0, citem+8, _(" Drop "));

		}
	    }
	}

	fin = !db.open(n, b, (void **) &citem) || (b == 1);

	if(!fin && citem) {
	    action = citem-(citem/100)*100;
	    pname = (protocolname) (citem/100-1);

	    a = conf.getourid(pname);
	    spname = conf.getprotocolname(pname);
	    abstracthook &hook = gethook(pname);

	    switch(action) {
		case 1:
		    tmp = face.inputstr(spname + _(" user name: "), a.nickname);
		    if(face.getlastinputkey() != KEY_ESC && !tmp.empty()) a.nickname = tmp;
		    break;

		case 2:
		    i = strtoul(face.inputstr(spname + _(" uin: "), i2str(a.uin)).c_str(), 0, 0);
		    if(face.getlastinputkey() != KEY_ESC && i) a.uin = i;
		    break;

		case 5:
		    tmp = face.inputstr(spname + _(" password: "), a.password, '*');
		    if(face.getlastinputkey() != KEY_ESC &&
		      (!tmp.empty() || capab.count(hookcapab::optionalpassword)))
			a.password = tmp;
		    break;

		case 6:
		    if(!a.empty()) {
			face.status(_("Drop the account information first!"));
		    } else {
			imcontrol.registration(a);
		    }
		    break;

		case 7:
		    imcontrol.updateinfo(a);
		    break;

		case 8:
		    if(!hook.online()) {
			a = icqconf::imaccount(a.pname);
		    } else {
			face.status(_("You have to disconnect the service first!"));
		    }
		    break;

		case 9:
		    tmp = "";
		    if(!a.server.empty())
			tmp = a.server + ":" + i2str(a.port);

		    tmp = face.inputstr(spname + _(" server address: "), tmp);

		    if(face.getlastinputkey() != KEY_ESC && !tmp.empty()) {
			if((pos = tmp.find(":")) != -1) {
			    a.server = tmp.substr(0, pos);
			    a.port = strtoul(tmp.substr(pos+1).c_str(), 0, 0);
			} else {
			    a.server = tmp;
			    a.port = 0;
			}
		    }
		    break;

		case 10:
		    if(face.edit(tmp = conf.getawaymsg(a.pname), spname + ": " + _("away message"))) {
			conf.setawaymsg(a.pname, tmp);
		    }
		    break;

		case 11:
		    a.additional["importfriends"] =
			(a.additional["importfriends"] == "") ? "1" : "0";
		    break;

		case 12:
		    tmp = face.inputstr(spname + _(" password: "), a.additional["nickpass"], '*');
		    if(face.getlastinputkey() != KEY_ESC)
			a.additional["nickpass"] = tmp;
		    break;

		case 13:
		    a.additional["ssl"] =
			(a.additional["ssl"] == "") ? "1" : "";

		    if(a.additional["ssl"] == "1") {
			if(a.port == icqconf::defservers[a.pname].port)
			    a.port = icqconf::defservers[a.pname].secureport;
		    } else {
			if(a.port == icqconf::defservers[a.pname].secureport)
			    a.port = icqconf::defservers[a.pname].port;
		    }
		    break;

		case 14:
		    tmp = leadcut(trailcut(face.inputstr(spname + _(" priority: "),
			a.additional["prio"])));

		    if(face.getlastinputkey() != KEY_ESC)
		    if(i2str(atoi(tmp.c_str())) == tmp)
			a.additional["prio"] = tmp;

		    break;

		case 15:
		    face.selectpgpkey(a.additional["pgpkey"], true);
		    break;

		case 16:
		    tmp = face.inputstr(_("PGP key passphrase: "), a.additional["pgppass"], '*');
		    if(face.getlastinputkey() != KEY_ESC)
			a.additional["pgppass"] = tmp;
		    break;
	    }

	    if(action != 7) {
		conf.setourid(a);
		mod[a.pname] = true;
	    }
	}
    }

    db.close();
    face.unblockmainscreen();

    for(pname = icq; pname != protocolname_size; pname++)
	if(mod[pname])
	    gethook(pname).ouridchanged(conf.getourid(pname));

    conf.save();
    face.relaxedupdate();
    fopen = false;
}

bool accountmanager::isopen() const {
    return fopen;
}
