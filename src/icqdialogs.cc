/*
*
* centericq user interface class, dialogs related part
* $Id: icqdialogs.cc,v 1.151 2005/05/23 14:16:52 konst Exp $
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

#include "icqface.h"
#include "icqconf.h"
#include "centericq.h"
#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqmlist.h"
#include "icqgroups.h"
#include "abstracthook.h"
#include "ljhook.h"
#include "impgp.h"

#include <libicq2000/constants.h>

const char *stragerange(ICQ2000::AgeRange r) {
    switch(r) {
	case ICQ2000::RANGE_18_22: return "18-22";
	case ICQ2000::RANGE_23_29: return "23-29";
	case ICQ2000::RANGE_30_39: return "30-39";
	case ICQ2000::RANGE_40_49: return "40-49";
	case ICQ2000::RANGE_50_59: return "50-59";
	case ICQ2000::RANGE_60_ABOVE: return _("60-above");
    }

    return "";
}

const char *strrandomgroup(short unsigned int gr) {
    switch((ICQ2000::RandomChatGroup) gr) {
	case ICQ2000::GROUP_GENERALCHAT: return _("General Chat");
	case ICQ2000::GROUP_ROMANCE: return _("Romance");
	case ICQ2000::GROUP_GAMES: return _("Games");
	case ICQ2000::GROUP_STUDENTS: return _("Students");
	case ICQ2000::GROUP_20: return _("20 Something");
	case ICQ2000::GROUP_30: return _("30 Something");
	case ICQ2000::GROUP_40: return _("40 Something");
	case ICQ2000::GROUP_50PLUS: return _("50 Plus");
	case ICQ2000::GROUP_SEEKINGWOMEN: return _("Seeking Women");
	case ICQ2000::GROUP_SEEKINGMEN: return _("Seeking Men");
    }

    return "";
}

bool icqface::sprofmanager(string &name, string &act) {
    dialogbox db;
    string buf, tname;
    bool finished, r;
    int n, b, i, dx, dy;

    map<string, imsearchparams> profiles;
    map<string, imsearchparams>::iterator ip;

    string pfname = conf.getconfigfname("search-profiles");

    ifstream f(pfname.c_str());

    if(f.is_open()) {
	while(!f.eof()) {
	    getstring(f, buf);
	    tname = getword(buf, "\t");

	    if(!tname.empty())
		profiles[tname] = imsearchparams(tname);
	}

	f.close();
    }

    dx = (COLS-sizeDlg.width)/2+sizeDlg.width;
    dy = (LINES-sizeDlg.height)/2+sizeDlg.height;

    db.setwindow(new textwindow(dx-(int) (sizeDlg.width*0.6), dy-(int) (sizeDlg.height*0.6),
	dx, dy, conf.getcolor(cp_dialog_frame), 0, conf.getcolor(cp_dialog_highlight),
	_(" Search profiles ")));

    db.setmenu(new verticalmenu(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), _("Remove"), _("Load"), 0));

    db.addkey(KEY_DC, 0);

    db.addautokeys();
    db.idle = &dialogidle;
    verticalmenu &m = *db.getmenu();
    db.getbar()->item = 1;

    finished = r = false;

    while(!finished) {
	m.clear();

	for(ip = profiles.begin(); ip != profiles.end(); ++ip) {
	    m.additemf(0, (void *) ip->first.c_str(), " %s: %s",
		conf.getprotocolname(ip->second.pname).c_str(),
		ip->first.c_str());
	}

	finished = !db.open(n, b);

	if(!finished) {
	    if(!profiles.empty()) {
		tname = (char *) m.getref(n-1);
		ip = profiles.find(tname);
	    } else {
		ip = profiles.end();
	    }

	    switch(b) {
		case 0:
		    if(ip != profiles.end())
			profiles.erase(ip);
		    break;

		case 1:
		    if(ip != profiles.end()) {
			finished = r = true;
			name = tname;
			act = "load";
		    }
		    break;
	    }
	}
    }

    db.close();

    unlink(conf.getconfigfname("search-profiles").c_str());

    for(ip = profiles.begin(); ip != profiles.end(); ++ip) {
	ip->second.save(ip->first);
    }

    return r;
}

bool icqface::finddialog(imsearchparams &s, findsubject subj) {
    int n, b, i;
    int nuin, ninfo, nloc, nwork, nonl;
    bool finished, ret, proceed;
    dialogbox db;

    vector<protocolname> penabled;
    vector<protocolname>::iterator ipname, ipfname;

    vector<string> services;
    vector<string>::iterator iservice;

    string tname, act;
    imsearchparams ts;

    if(subj != fsrss) {
	for(protocolname apname = icq; apname != protocolname_size; apname++) {
	    if(subj == fschannel)
	    if(!gethook(apname).getCapabs().count(hookcapab::conferencing))
		continue;

	    if(gethook(apname).logged() || apname == infocard) {
		penabled.push_back(apname);
	    }
	}

	if(penabled.empty()) {
	    log(_("+ you must be logged in first"));
	    return false;
	}

	if((ipname = find(penabled.begin(), penabled.end(), s.pname)) == penabled.end()) {
	    ipname = penabled.begin();
	    s.pname = *ipname;
	}
    }

    blockmainscreen();
    finished = ret = false;

    db.setwindow(new textwindow(0, 0, sizeDlg.width, sizeDlg.height,
	conf.getcolor(cp_dialog_frame), TW_CENTERED));

    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	conf.getcolor(cp_dialog_highlight),
	conf.getcolor(cp_dialog_text)));

    switch(subj) {
	case fsuser:
	    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text), conf.getcolor(cp_dialog_selected),
		_("lOad"), _("sAve"), _("cLear"), _("Change"), _("Search/Add"), 0));
	    break;

	case fschannel:
	    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text), conf.getcolor(cp_dialog_selected),
		_("cLear"), _("Change"), _("Join/Create"), 0));
	    break;

	case fsrss:
	    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text), conf.getcolor(cp_dialog_selected),
		_("cLear"), _("Change"), _("lInk"), 0));
	    break;
    }

    struct {
	char *title;
	int item;

    } dlgparam[fs_size] = {
	{ _(" Find/add user(s) "), 3 },
	{ _(" Join/create a channel/conference "), 1 },
	{ _(" Link an RSS feed "), 1 }

    };

    db.getwindow()->set_title(conf.getcolor(cp_dialog_highlight), dlgparam[subj].title);
    db.getbar()->item = dlgparam[subj].item;

    db.addautokeys();
    db.idle = &dialogidle;
    treeview &tree = *db.gettree();

    bool protchanged = true;

    while(!finished) {
	tree.clear();

	if(protchanged) {
	    switch(subj) {
		case fsuser:
		    services = gethook(s.pname).getservices(servicetype::search);

		    if((iservice = find(services.begin(), services.end(), s.service)) == services.end())
			s.service = "";

		    if(s.service.empty())
		    if((iservice = services.begin()) != services.end()) {
			s.service = *iservice;
			s.flexparams = gethook(s.pname).getsearchparameters(s.service);
		    }
		    break;

		case fschannel:
		    services = gethook(s.pname).getservices(servicetype::groupchat);

		    if(s.service.empty())
		    if((iservice = services.begin()) != services.end())
			s.service = *iservice;
		    break;
	    }

	    protchanged = false;
	}

	if(subj != fsrss) {
	    i = tree.addnode(_(" Network "));
	    tree.addleaf(i, 0, 1, " " + conf.getprotocolname(s.pname) + " ");
	}

	if(subj != fschannel)
	switch(s.pname) {
	    case icq:
	    case gadu:
		i = tree.addnode(_(" UIN "));
		tree.addleaff(i, 0, 10, " %s ", strint(s.uin));

		if(!s.uin && !s.randomgroup && s.kwords.empty()) {
		    i = tree.addnode(_(" Details "));
		    tree.addleaff(i, 0, 11, _(" Nickname : %s "), s.nick.c_str());

		    if(s.pname == icq) {
			tree.addleaff(i, 0, 12, _(" E-Mail : %s "), s.email.c_str());
		    }

		    tree.addleaff(i, 0, 13, _(" First name : %s "), s.firstname.c_str());
		    tree.addleaff(i, 0, 14, _(" Last name : %s "), s.lastname.c_str());

		    if(s.pname == icq) {
			tree.addleaff(i, 0, 15, _(" Age range : %s "), stragerange(s.agerange));
		    }

		    tree.addleaff(i, 0, 17, _(" Gender : %s "), strgender(s.gender));

		    if(s.pname == icq) {
			tree.addleaff(i, 0, 18, _(" Language : %s "),
			    abstracthook::getLanguageIDtoString(s.language).c_str());
		    }

		    i = tree.addnode(_(" Location "));
		    tree.addleaff(i, 0, 19, _(" City : %s "), s.city.c_str());

		    if(s.pname == icq) {
			tree.addleaff(i, 0, 20, _(" State : %s "), s.state.c_str());
			tree.addleaff(i, 0, 21, _(" Country : %s "),
			    abstracthook::getCountryIDtoString(s.country).c_str());

			i = tree.addnode(_(" Work "));
			tree.addleaff(i, 0, 22, _(" Company : %s "), s.company.c_str());
			tree.addleaff(i, 0, 23, _(" Department : %s "), s.department.c_str());
			tree.addleaff(i, 0, 24, _(" Position : %s "), s.position.c_str());
		    }

		    i = tree.addnode(_(" Online only "));
		    tree.addleaff(i, 0, 25, " %s ", stryesno(s.onlineonly));
		}

		if(s.pname == icq) {
		    if(!s.uin && s.kwords.empty()) {
			i = tree.addnode(_(" Random chat group "));
			tree.addleaff(i, 0, 28, " %s ", strrandomgroup(s.randomgroup));
		    }

		    if(!s.uin && !s.randomgroup) {
			i = tree.addnode(_(" Keywords "));
			tree.addleaff(i, 0, 29, " %s ", s.kwords.c_str());
		    }
		}
		break;

	    default:
		if((s.pname == msn && !s.reverse)
		|| (s.pname != msn)) {
		    i = tree.addnode(_(" Nickname "));
		    tree.addleaf(i, 0, 11, " " + s.nick + " ");
		}

		if(s.pname == jabber)
		if(s.nick.empty() && !services.empty()) {
		    i = tree.addnode(_(" Search service "));
		    tree.addleaff(i, 0, 31, " %s ", s.service.c_str());

		    i = tree.addnode(_(" Search parameters "));

		    int id = 100;
		    vector<pair<string, string> >::const_iterator
			ifp = s.flexparams.begin();

		    while(ifp != s.flexparams.end()) {
			tree.addleaff(i, 0, id++, " %s : %s ",
			    ifp->first.c_str(), ifp->second.c_str());
			++ifp;
		    }
		}
		break;
	}

	if(subj == fsrss) {
	    i = tree.addnode(_(" Feed Parameters "));
	    tree.addleaff(i, 0, 33, _(" XML export URL : %s "), s.url.c_str());
	    tree.addleaff(i, 0, 34, _(" Check frequency (minutes) : %lu "), s.checkfrequency);
	}

	if((subj == fsuser) && s.pname == irc && s.nick.empty()) {
	    i = tree.addnode(_(" Details "));
	    tree.addleaff(i, 0, 26, _(" Channel : %s "), s.room.c_str());

	    if(!s.room.empty()) {
		tree.addleaff(i, 0, 27, _(" Name : %s "), s.firstname.c_str());
	    }

	    tree.addleaff(i, 0, 12, _(" E-Mail : %s "), s.email.c_str());

	    if(!s.room.empty()) {
		i = tree.addnode(_(" Joined since the last check only "));
		tree.addleaff(i, 0, 16, " %s ", stryesno(s.sincelast));
	    }
	}

	if((subj == fsuser) && s.pname == yahoo && s.nick.empty()) {
	    i = tree.addnode(_(" Details "));

	    if(s.kwords.empty()) tree.addleaff(i, 0, 27, _(" Name : %s "), s.firstname.c_str());
	    if(s.firstname.empty()) tree.addleaff(i, 0, 29, _(" Keywords : %s "), s.kwords.c_str());

	    i = tree.addnode(_(" Options "));

	    tree.addleaff(i, 0, 17, _(" Gender : %s "), strgender(s.gender));
	    tree.addleaff(i, 0, 15, _(" Age range : %s "), stragerange(s.agerange));
	    tree.addleaff(i, 0, 35, _(" Users with photos only : %s "), stryesno(s.photo));
	    tree.addleaff(i, 0, 25, _(" Look for online only : %s "), stryesno(s.onlineonly));
	}

	if((subj == fsuser) && s.nick.empty()
	&& (s.pname == msn || s.pname == livejournal)) {
	    i = tree.addnode(_(" Show users who have you on their list "));
	    tree.addleaff(i, 0, 30, " %s ", stryesno(s.reverse));
	}

	if(subj == fschannel) {
	    i = tree.addnode(_(" Name/Title "));
	    tree.addleaf(i, 0, 11, " " + s.nick + " ");

	    if(!s.nick.empty()) {
		if(gethook(s.pname).getCapabs().count(hookcapab::channelpasswords)) {
		    i = tree.addnode(_(" Password "));
		    tree.addleaf(i, 0, 32, " " + string(s.password.size(), '*') + " ");
		}

		if(gethook(s.pname).getCapabs().count(hookcapab::groupchatservices)) {
		    i = tree.addnode(_(" Service "));
		    tree.addleaf(i, 0, 31, " " + s.service + " ");
		}
	    }
	}

	finished = !db.open(n, b, (void **) &i);

	if(subj != fsuser)
	    b += 2;

	if(!finished)
	switch(b) {
	    case 0:
		if(sprofmanager(tname, act)) {
		    if(act == "load") {
			imsearchparams ts(tname);

			if((ipfname = find(penabled.begin(), penabled.end(),
			ts.pname)) != penabled.end()) {
			    s = ts;
			    ipname = ipfname;
			    protchanged = true;
			}
		    }
		}
		break;

	    case 1:
		tname = inputstr(_("New profile name: "));

		if(proceed = !tname.empty())
		if(ts.load(tname))
		    proceed = ask(_("The profile with this name already exists. Do you want to overwrite it?"),
			ASK_YES | ASK_NO, ASK_NO) == ASK_YES;

		if(proceed)
		    s.save(tname);
		break;

	    case 2:
		s = imsearchparams();
		s.pname = *ipname;
		protchanged = true;
		break;
		
	    case 3:
		switch(i) {
		    case 1:
			if(++ipname == penabled.end())
			    ipname = penabled.begin();

			if(protchanged = s.pname != *ipname) {
			    s.service = "";
			    s.flexparams.clear();
			}

			s.pname = *ipname;
			break;

		    case 10: s.uin = atol(inputstr(_("UIN: "), strint(s.uin)).c_str()); break;
		    case 11: s.nick = inputstr(subj != fschannel ? _("Nickname: ") : _("Name/Title: "), s.nick); break;
		    case 12: s.email = inputstr(_("E-Mail: "), s.email); break;
		    case 13: s.firstname = inputstr(_("First name: "), s.firstname); break;
		    case 14: s.lastname = inputstr(_("Last name: "), s.lastname); break;
		    case 15: selectagerange(s.agerange); break;
		    case 16: s.sincelast = !s.sincelast; break;
		    case 17: selectgender(s.gender); break;
		    case 18: selectlanguage(s.language); break;
		    case 19: s.city = inputstr(_("City: "), s.city); break;
		    case 20: s.state = inputstr(_("State: "), s.state); break;
		    case 21: selectcountry(s.country); break;
		    case 22: s.company = inputstr(_("Company: "), s.company); break;
		    case 23: s.department = inputstr(_("Department: "), s.department); break;
		    case 24: s.position = inputstr(_("Position: "), s.position); break;
		    case 25: s.onlineonly = !s.onlineonly; break;
		    case 26: s.room = inputstr(_("Channel: "), s.room); break;
		    case 27: s.firstname = inputstr(_("Name: "), s.firstname); break;
		    case 28: selectrandomgroup(s.randomgroup); break;
		    case 29: s.kwords = inputstr(_("Keywords: "), s.kwords); break;
		    case 30: s.reverse = !s.reverse; break;
		    case 31:
			if(services.size()) {
			    if(++iservice == services.end())
				iservice = services.begin();
			    s.service = *iservice;
			}
			break;
		    case 32: s.password = inputstr(_("Password: "), s.password, '*'); break;
		    case 33: s.url = inputstr(_("URL: "), s.url); break;
		    case 34: s.checkfrequency = atol(inputstr(_("Check frequency: "), strint(s.checkfrequency)).c_str()); break;
		    case 35: s.photo = !s.photo;
		}

		if(i >= 100) {
		    vector<pair<string, string> >::iterator ifp = s.flexparams.begin()+i-100;
		    ifp->second = inputstr(ifp->first + ": ", ifp->second);
		}
		break;

	    case 4:
		if(s.pname == jabber && s.nick.find("@") == -1) {
		    status(_("Wrong Jabber ID!"));
		} else {
		    ret = finished = true;
		}
		break;
	}
    }

    db.close();
    unblockmainscreen();

    return ret;
}

#define CHECKGENERAL if(!i) i = tree->addnode(_(" General "));

void icqface::gendetails(treeview *tree, icqcontact *c) {
    int saveitem, savefirst, i;

    if(!c) c = clist.get(contactroot);

    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();
    icqcontact::workinfo wi = c->getworkinfo();
    icqcontact::reginfo ri = c->getreginfo();

    string about = c->getabout();
    bool ourdetails = c->getdesc() == contactroot;

    abstracthook &h = gethook(passinfo.pname);
    set<hookcapab::enumeration> capab = h.getCapabs();

    tree->menu.getpos(saveitem, savefirst);
    tree->clear();

    i = 0;

    if(capab.count(hookcapab::flexiblereg) && ourdetails) {
	if(ri.service.empty()) {
	    vector<string> servs = h.getservices(servicetype::registration);
	    if(!servs.empty()) ri.service = servs[0]; else return;
	}

	if(ri.params.empty()) {
	    ri.params = h.getregparameters(ri.service);
	}

	c->setreginfo(ri);

	i = tree->addnode(_(" Registration service "));
	tree->addleaff(i, 0, 46, " %s ", ri.service.c_str());

	if(!ri.params.empty()) {
	    i = tree->addnode(_(" Registration parameters "));

	    vector<pair<string, string> >::const_iterator ir = ri.params.begin();

	    while(ir != ri.params.end()) {
		tree->addleaff(i, 0, 100+(ir-ri.params.begin()), " %s : %s ",
		    ir->first.c_str(), ir->second.c_str());
		++ir;
	    }
	}

	i = 0;

    }

    if(passinfo.pname == infocard || capab.count(hookcapab::changenick) || !ourdetails)
    if((capab.count(hookcapab::flexiblereg) && ri.params.empty()) || !capab.count(hookcapab::flexiblereg)) {
	CHECKGENERAL;
	tree->addleaff(i, 0, 10, _(" Nickname : %s "), c->getnick().c_str());
    }

    if(capab.count(hookcapab::changepassword) && ourdetails) {
	CHECKGENERAL;
	tree->addleaff(i, 0, 47, _(" Change password : %s "),
	    string(ri.password.size(), '*').c_str());
    }

    if(passinfo.pname != rss)
    if(passinfo.pname == icq && c->getdesc() == contactroot || !ourdetails ||
    (ourdetails && capab.count(hookcapab::flexiblereg) && ri.params.empty())) {
	CHECKGENERAL;

	tree->addleaff(i, 0, 11, _(" First name : %s "), bi.fname.c_str());
	tree->addleaff(i, 0, 12, _(" Last name : %s "), bi.lname.c_str());
	tree->addleaff(i, 0, 13, _(" E-mail : %s "), bi.email.c_str());

	tree->addleaff(i, 0, 14, _(" Gender : %s "), strgender(mi.gender));
	tree->addleaff(i, 0, 15, _(" Birthdate : %s "), mi.strbirthdate().c_str());
	tree->addleaff(i, 0, 16, _(" Age : %s "), strint(mi.age));

	i = tree->addnode(_(" Home "));
	tree->addleaff(i, 0, 17, _(" City : %s "), bi.city.c_str());
	tree->addleaff(i, 0, 18, _(" State : %s "), bi.state.c_str());
	tree->addleaff(i, 0, 19, _(" Country : %s "),
	    abstracthook::getCountryIDtoString(bi.country).c_str());

	tree->addleaff(i, 0, 20, _(" Street address : %s "), bi.street.c_str());
	tree->addleaff(i, 0, 21, _(" Zip code : %s "), bi.zip.c_str());
	tree->addleaff(i, 0, 22, _(" Phone : %s "), bi.phone.c_str());
	tree->addleaff(i, 0, 23, _(" Fax : %s "), bi.fax.c_str());
	tree->addleaff(i, 0, 24, _(" Cellular phone : %s "), bi.cellular.c_str());

	i = tree->addnode(_(" Work "));
	tree->addleaff(i, 0, 25, _(" City : %s "), wi.city.c_str());
	tree->addleaff(i, 0, 26, _(" State : %s "), wi.state.c_str());
	tree->addleaff(i, 0, 27, _(" Country : %s "),
	    abstracthook::getCountryIDtoString(wi.country).c_str());

	tree->addleaff(i, 0, 28, _(" Street address : %s "), wi.street.c_str());
	tree->addleaff(i, 0, 41, _(" Zip code : %s "), wi.zip.c_str());
	tree->addleaff(i, 0, 29, _(" Company : %s "), wi.company.c_str());
	tree->addleaff(i, 0, 30, _(" Department : %s "), wi.dept.c_str());
	tree->addleaff(i, 0, 31, _(" Position : %s "), wi.position.c_str());
	tree->addleaff(i, 0, 32, _(" Homepage : %s "), wi.homepage.c_str());
	tree->addleaff(i, 0, 33, _(" Phone : %s "), wi.phone.c_str());
	tree->addleaff(i, 0, 34, _(" Fax : %s "), wi.fax.c_str());

	i = tree->addnode(_(" More "));
	tree->addleaff(i, 0, 35, _(" Homepage : %s "), mi.homepage.c_str());

	if(passinfo.pname == icq) {
	    tree->addleaff(i, 0, 36, _(" 1st language : %s "),
		abstracthook::getLanguageIDtoString(mi.lang1).c_str());

	    tree->addleaff(i, 0, 37, _(" 2nd language : %s "),
		abstracthook::getLanguageIDtoString(mi.lang2).c_str());

	    tree->addleaff(i, 0, 38, _(" 3rd language : %s "),
		abstracthook::getLanguageIDtoString(mi.lang3).c_str());

	    i = tree->addnode(_(" Miscellaneous "));
	    if(ourdetails) {
		tree->addleaff(i, 0, 43, _(" Enable web status indicator : %s "), stryesno(bi.webaware));
		tree->addleaff(i, 0, 42, _(" Random chat group : %s "), strrandomgroup(bi.randomgroup));
	    } else {
		tree->addleaff(i, 0, 44, _(" Authorization required : %s "), stryesno(bi.requiresauth));
	    }
	}
    } else if(passinfo.pname == gadu) {
	CHECKGENERAL;

	tree->addleaff(i, 0, 11, _(" First name : %s "), bi.fname.c_str());
	tree->addleaff(i, 0, 12, _(" Last name : %s "), bi.lname.c_str());
	tree->addleaff(i, 0, 17, _(" City : %s "), bi.city.c_str());
	tree->addleaff(i, 0, 14, _(" Gender : %s "), strgender(mi.gender));
    }

    if(passinfo.pname == rss) {
	i = tree->addnode(_(" Feed Parameters "));
	tree->addleaff(i, 0, 48, _(" XML export URL : %s "), wi.homepage.c_str());
	tree->addleaff(i, 0, 49, _(" Check frequency (minutes) : %lu "), mi.checkfreq);
    }

    if(capab.count(hookcapab::changeabout))
    if((capab.count(hookcapab::flexiblereg) && ri.params.empty()) || !capab.count(hookcapab::flexiblereg)) {
	i = tree->addnode(_(" About "));
	tree->addleaff(i, 0, 39, " %s ", about.c_str());
    }

    tree->menu.setpos(saveitem, savefirst);
}

bool icqface::updatedetails(icqcontact *c, protocolname upname) {
    string about, tmp;
    icqcontact::basicinfo bi;
    icqcontact::workinfo wi;
    icqcontact::moreinfo mi;
    icqcontact::reginfo ri;
    bool finished = false, ret = false, msb;
    int n, b, citem;
    dialogbox db;

    if(!c) {
	status(_("Fetching your details"));
	c = clist.get(contactroot);
    }

    if(!(msb = mainscreenblock)) {
	blockmainscreen();
    }

    passinfo.pname = upname;

    textwindow w(0, 0, sizeDlg.width, sizeDlg.height,
	conf.getcolor(cp_dialog_frame), TW_CENTERED);

    if(c->getdesc() == contactroot) {
	w.set_titlef(conf.getcolor(cp_dialog_highlight), _(" Your %s details "),
	    conf.getprotocolname(upname).c_str());
    } else {
	w.set_titlef(conf.getcolor(cp_dialog_highlight), _(" %s: details "),
	    c->getdesc().totext().c_str());
    }

    detailsfetched = (c->getdesc() != contactroot);

    db.setwindow(&w, false);

    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), conf.getcolor(cp_dialog_highlight),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), _("Change"), _("Done"), 0));

    db.idle = &detailsidle;
    db.addautokeys();

    while(!finished) {
	gendetails(db.gettree(), c);
	finished = !db.open(n, b, (void **) &citem);
	if(finished) continue;

	bi = c->getbasicinfo();
	mi = c->getmoreinfo();
	wi = c->getworkinfo();
	ri = c->getreginfo();
	about = c->getabout();

	if(!b) {
	    if((c->getdesc() == contactroot) && !c->updated()) {
		status(_("Wait a moment. Your details haven't been fetched yet"));
		continue;
	    } else
	    switch(citem) {
		case 10:
		    c->setnick(inputstr(_("Nickname: "), c->getnick()));
		    switch(c->getdesc().pname) {
			case infocard:
			case livejournal:
			case rss:
			    c->setdispnick(c->getnick());
			    break;
		    }
		    break;
		case 11: bi.fname = inputstr(_("First name: "), bi.fname); break;
		case 12: bi.lname = inputstr(_("Last name: "), bi.lname); break;
		case 13: bi.email = inputstr(_("E-mail: "), bi.email); break;
		case 14: selectgender(mi.gender); break;
		case 15:
		    if(mi.birth_day || mi.birth_month || mi.birth_year) {
			tmp = i2str(mi.birth_day) + "-" +
			    i2str(mi.birth_month) + "-" +
			    i2str(mi.birth_year);
		    } else {
			tmp = "";
		    }

		    tmp = inputstr(_("Enter birthdate (DD-MM-YYYY): "), tmp);

		    if(tmp.size()) {
			int d, m, y;
			sscanf(tmp.c_str(), "%d-%d-%d", &d, &m, &y);

			if(y >= 1900)
			if((d > 0) && (d <= 31))
			if((m > 0) && (m <= 12)) {
			    mi.birth_day = d;
			    mi.birth_month = m;
			    mi.birth_year = y;
			}
		    } else {
			mi.birth_day = mi.birth_month = mi.birth_year = 0;
		    }
		    break;

		case 16: mi.age = atoi(inputstr(_("Age: "), strint(mi.age)).c_str()); break;

		case 17: bi.city = inputstr(_("City: "), bi.city); break;
		case 18: bi.state = inputstr(_("State: "), bi.state); break;
		case 19: selectcountry(bi.country); break;
		case 20: bi.street = inputstr(_("Street address: "), bi.street); break;
		case 21: bi.zip = inputstr(_("Zip code: "), bi.zip); break;
		case 22: bi.phone = inputstr(_("Phone: "), bi.phone); break;
		case 23: bi.fax = inputstr(_("Fax: "), bi.fax); break;
		case 24: bi.cellular = inputstr(_("Cellular phone: "), bi.cellular); break;

		case 25: wi.city = inputstr(_("City: "), wi.city); break;
		case 26: wi.state = inputstr(_("State: "), wi.state); break;
		case 27: selectcountry(wi.country); break;
		case 28: wi.street = inputstr(_("Street address: "), wi.street); break;
		case 29: wi.company = inputstr(_("Company: "), wi.company); break;
		case 30: wi.dept = inputstr(_("Department: "), wi.dept); break;
		case 31: wi.position = inputstr(_("Position: "), wi.position); break;
		case 32: wi.homepage = inputstr(_("Homepage: "), wi.homepage); break;
		case 33: wi.phone = inputstr(_("Phone: "), wi.phone); break;
		case 34: wi.fax = inputstr(_("Fax: "), wi.fax); break;

		case 35: mi.homepage = inputstr(_("Homepage: "), mi.homepage); break;

		case 36: selectlanguage(mi.lang1); break;
		case 37: selectlanguage(mi.lang2); break;
		case 38: selectlanguage(mi.lang3); break;

		case 39: edit(about, _("About")); break;
		case 40: bi.requiresauth = !bi.requiresauth; break;
		case 41: wi.zip = inputstr(_("Zip code: "), wi.zip); break;
		case 42: selectrandomgroup(bi.randomgroup); break;
		case 43: bi.webaware = !bi.webaware; break;
		case 44: bi.requiresauth = !bi.requiresauth; break;
		case 46: {
		    abstracthook &h = gethook(passinfo.pname);
		    vector<string> servs = h.getservices(servicetype::registration);
		    vector<string>::const_iterator is = find(servs.begin(), servs.end(), ri.service);

		    if(is != servs.end()) {
			if(++is == servs.end()) is = servs.begin();
			if(*is != ri.service) {
			    ri.service = *is;
			    ri.params.clear();
			}
		    }

		    } break;
		case 47:
		    ri.password = inputstr(_("New password: "), ri.password, '*');

		    if(!ri.password.empty())
		    if(inputstr(_("Check the new password: "), "", '*') != ri.password)
			ri.password = "";
		    break;
		case 48: wi.homepage = inputstr(_("URL: "), wi.homepage); break;
		case 49: mi.checkfreq = atol(inputstr(_("Check frequency: "), strint(mi.checkfreq)).c_str()); break;
		default:
		    if(citem >= 100) {
			vector<pair<string, string> >::iterator ifp = ri.params.begin()+citem-100;
			ifp->second = inputstr(ifp->first + ": ", ifp->second);
		    }
		    break;
	    }

	    c->setbasicinfo(bi);
	    c->setmoreinfo(mi);
	    c->setworkinfo(wi);
	    c->setreginfo(ri);
	    c->setabout(about);
	} else {
	    ret = (c->getdesc() != contactroot) || c->updated();
	    finished = true;
	}
    }

    db.close();

    if(!msb) {
	unblockmainscreen();
    }

    return ret;
}

void icqface::selectcountry(unsigned short &f) {
    int i;

    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-16, 30, LINES-3, conf.getcolor(cp_dialog_menu)));
    m.idle = &menuidle;

    for(i = 0; i < abstracthook::Country_table_size; i++) {
	m.additem(0, (int) abstracthook::Country_table[i].code,
	    (string) " " + abstracthook::Country_table[i].name);

	if(abstracthook::Country_table[i].code == f) {
	    m.setpos(i);
	}
    }

    i = m.open();
    m.close();

    if(i) f = (unsigned short) ((int) m.getref(i-1));
}

void icqface::selectlanguage(unsigned short &f) {
    int i;
    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-13, 20, LINES-4, conf.getcolor(cp_dialog_menu)));
    m.idle = &menuidle;

    for(i = 0; i < abstracthook::Language_table_size; i++) {
	m.additemf(" %s", abstracthook::Language_table[i]);
    }

    m.setpos(f);
    i = m.open();
    m.close();
    
    if(i) f = i-1;
}

void icqface::selectrandomgroup(unsigned short &f) {
    int i;
    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-15, 20, LINES-3, conf.getcolor(cp_dialog_menu)));
    m.idle = &menuidle;

    m.additemf(" %s", _("none"));

    for(i = 1; i <= ((int) ICQ2000::GROUP_SEEKINGMEN); i++) {
	m.additemf(" %s", strrandomgroup(i));
    }

    m.setpos(f);
    i = m.open();
    m.close();

    if(i) f = i-1;
}

void icqface::selectgender(imgender &f) {
    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-8, 20, LINES-4, conf.getcolor(cp_dialog_menu)));
    m.idle = &menuidle;

    for(imgender i = genderUnspec; i != imgender_size; i++) {
	m.additemf(0, (int) i, " %s", strgender(i));
	if(i == f) m.setpos(m.getcount()-1);
    }

    int i = m.open();
    m.close();

    if(i) f = (imgender) ((int) m.getref(i-1));
}

void icqface::selectagerange(ICQ2000::AgeRange &r) {
    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-8, 18, LINES-3, conf.getcolor(cp_dialog_menu)));
    m.idle = &menuidle;

    for(ICQ2000::AgeRange i = ICQ2000::RANGE_NORANGE; i <= ICQ2000::RANGE_60_ABOVE; i = ICQ2000::AgeRange(i+1)) {
	const char *p = stragerange(i);

	if(i == ICQ2000::RANGE_NORANGE)
	    p = _("none");

	m.additemf(0, (int) i, " %s", p);
	if(i == r) m.setpos(m.getcount()-1);
    }

    int i = m.open();
    m.close();

    if(i) r = (ICQ2000::AgeRange) ((int) m.getref(i-1));
}

bool icqface::edit(string &txt, const string &header) {
    bool msb = mainscreenblock;
    if(!msb) blockmainscreen();

    texteditor se;
    textwindow w(0, 0, sizeDlg.width, sizeDlg.height, conf.getcolor(cp_dialog_frame), TW_CENTERED);
    w.set_title(conf.getcolor(cp_dialog_highlight), (string) " " + header + _(" [Ctrl-X save, Esc cancel] "));
    w.open();

    editdone = false;
    se.setcoords(w.x1+1, w.y1+1, w.x2, w.y2);
    se.addscheme(cp_dialog_text, cp_dialog_text, 0, 0);
    se.idle = &editidle;
    se.otherkeys = &editaboutkeys;
    se.wrap = true;
    se.smarttab = false;

    se.load(txt, "");
    se.open();

    if(editdone) {
	char *ctext = se.save("\r\n");
	txt = ctext;
	delete ctext;
    }

    w.close();
    if(!msb) unblockmainscreen();

    return editdone;
}

bool icqface::sendfiles(const imcontact &cinfo, string &msg, linkedlist &flist) {
    int n, i, b;
    bool finished = false;
    string fname;
    dialogbox db;

    flist.freeitem = &charpointerfree;
    flist.empty();

    blockmainscreen();

    textwindow w(0, 0, sizeDlg.width, sizeDlg.height, conf.getcolor(cp_dialog_frame), TW_CENTERED);

    w.set_titlef(conf.getcolor(cp_dialog_highlight),
	_(" Send file(s) to %s, %lu "),
	    clist.get(cinfo)->getdispnick().c_str(), cinfo.uin);

    db.setwindow(&w, false);
    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), conf.getcolor(cp_dialog_highlight),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	_("Add"), _("Remove"), _("Comment"), _("Send"), 0));

    db.addkey(KEY_IC, 0);
    db.addkey(KEY_DC, 1);
    db.addautokeys();
    db.idle = &dialogidle;

    while(!finished) {
	db.gettree()->clear();
	
	int nfiles = db.gettree()->addnode(_(" Files "));
	int ncomment = db.gettree()->addnode(_(" Comment "));

	for(i = 0; i < flist.count; i++)
	    db.gettree()->addleaf(nfiles,  0, 0, " " +
	    justfname((char *) flist.at(i)) + " ");

	db.gettree()->addleaf(ncomment, 0, 0, " " + msg + " ");
	finished = !db.open(n, b);

	if(!finished)
	switch(b) {
	    case 0:
		fname = inputfile(_("filename: "));
		if(fname.size()) flist.add(strdup(fname.c_str()));
		break;
	    case 1:
		flist.remove(n-2);
		break;
	    case 2:
		msg = inputstr(_("comment: "), msg);
		break;
	    case 3:
		finished = true;
		break;
	} else {
	    flist.empty();
	}
    }

    db.close();
    unblockmainscreen();
    return flist.count;
}

void icqface::multichange(bool conv[], bool newstate) {
    protocolname pname;

    for(pname = icq; pname != protocolname_size; pname++)
	if((!conf.getourid(pname).empty() || pname == rss) && conv[pname]) break;

    if(pname == protocolname_size || !newstate) {
	for(pname = icq; pname != protocolname_size; pname++)
	    if(!conf.getourid(pname).empty() || pname == rss)
		conv[pname] = newstate;
    }
}

bool icqface::updateconf(icqconf::regsound &s, icqconf::regcolor &c) {
    bool finished, success, hasany;
    int nopt, n, i, b, nconf, ncomm, aaway, ana, noth, nfeat, ncl;
    protocolname pname;
    string tmp, phidden;

    string smtp = conf.getsmtphost() + ":" + i2str(conf.getsmtpport());

    bool quote = conf.getquote();
    bool savepwd = conf.getsavepwd();
    bool hideoffl = conf.gethideoffline();
    bool antispam = conf.getantispam();
    bool mailcheck = conf.getmailcheck();
    bool makelog = conf.getmakelog();
    bool askaway = conf.getaskaway();
    bool bidi = conf.getbidi();
    bool emacs = conf.getemacs();
    bool proxyconnect = conf.getproxyconnect();

    bool logtimestamps, logonline;
    conf.getlogoptions(logtimestamps, logonline);

    int ptpmin, ptpmax;
    conf.getpeertopeer(ptpmin, ptpmax);
    bool ptp = ptpmax;

    string httpproxy;
    if (!conf.gethttpproxyuser().empty()) {
	httpproxy = conf.gethttpproxyuser();
	if(!httpproxy.empty())
	    httpproxy += ":" + conf.gethttpproxypasswd() + "@" + conf.gethttpproxyhost() + ":" + i2str(conf.gethttpproxyport());
    } else {
	httpproxy = conf.gethttpproxyhost();
	if(!httpproxy.empty())
	    httpproxy += ":" + i2str(conf.gethttpproxyport());
    }

    vector<string> convlanguages;
    convlanguages.push_back(_("None"));
    convlanguages.push_back(_("Russian"));
    convlanguages.push_back(_("Polish"));
    vector<string>::const_iterator iconvlang = convlanguages.begin();

    string convertfrom = conf.getconvertfrom();
    string convertto = conf.getconvertto();

    icqconf::groupmode gmode = conf.getgroupmode();
    icqconf::colormode cm = conf.getcolormode();

    bool chatmode[protocolname_size], conv[protocolname_size],
	entersends[protocolname_size], nonimonline[protocolname_size];

    for(pname = icq; pname != protocolname_size; pname++) {
	chatmode[pname] = conf.getchatmode(pname);
	entersends[pname] = conf.getentersends(pname);
	conv[pname] = conf.getcpconvert(pname);
	nonimonline[pname] = conf.getnonimonline(pname);
    }

    for(hasany = false, pname = icq; pname != protocolname_size && !hasany; pname++)
	hasany = !conf.getourid(pname).empty();

    dialogbox db;

    finished = success = false;

    conf.getauto(aaway, ana);

    textwindow w(0, 0, sizeBigDlg.width, sizeBigDlg.height,
	conf.getcolor(cp_dialog_frame), TW_CENTERED);

    w.set_title(conf.getcolor(cp_dialog_highlight), _(" CenterICQ configuration "));

    db.setwindow(&w, false);

    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), conf.getcolor(cp_dialog_highlight),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), _("Change"), _("Done"), 0));

    db.idle = &dialogidle;
    db.addautokeys();

    blockmainscreen();
    treeview &t = *db.gettree();

    while(!finished) {
	t.clear();

	i = t.addnode(_(" User interface "));
	t.addleaff(i, 0, 1, _(" Change sound device to : %s "), strregsound(s));
	t.addleaff(i, 0, 2, _(" Change color scheme to : %s "), strregcolor(c));

    #ifdef USE_FRIBIDI
	t.addleaff(i, 0, 20, _( " Enable bidirectional languages support : %s "), stryesno(bidi));
    #endif
	t.addleaff(i, 0, 23, _(" Enable emacs bindings in text editor : %s "), stryesno(emacs));

	i = t.addnode(_(" Codepages conversion "));

	for(tmp = "", pname = icq; pname != protocolname_size; pname++)
	if(conv[pname])
	if(!conf.getourid(pname).empty() || pname == rss)
	    tmp += conf.getprotocolname(pname) + " ";

	t.addleaff(i, 0, 26, _(" Switch to language preset : %s "), iconvlang->c_str());
	t.addleaff(i, 0, 27, _(" Remote charset : %s "), convertfrom.c_str());
	t.addleaff(i, 0, 28, _(" Local charset : %s "), convertto.c_str());

	if(hasany) {
	    t.addleaff(i, 0,  3, _(" For protocols : %s"), tmp.c_str());
	}

	i = t.addnode(_(" Contact list "));
	t.addleaff(i, 0, 17, _(" Arrange contacts into groups : %s "), strgroupmode(gmode));
	t.addleaff(i, 0,  6, _(" Hide offline users : %s "), stryesno(hideoffl));
	t.addleaff(i, 0, 14, _(" Anti-spam: kill msgs from users not on the list : %s "), stryesno(antispam));
	t.addleaff(i, 0,  8, _(" Quote a message on reply : %s "), stryesno(quote));
	t.addleaff(i, 0, 15, _(" Check the local mailbox : %s "), stryesno(mailcheck));
	t.addleaff(i, 0, 13, _(" Remember passwords : %s "), stryesno(savepwd));
	t.addleaff(i, 0,  7, _(" Edit away message on status change : %s "), stryesno(askaway));

	if(hasany) {
	    for(tmp = "", pname = icq; pname != protocolname_size; pname++)
		if(chatmode[pname] && !conf.getourid(pname).empty())
		    tmp += conf.getprotocolname(pname) + " ";

	    t.addleaff(i, 0, 16, _(" Chat messaging mode for : %s"), tmp.c_str());

	    for(tmp = "", pname = icq; pname != protocolname_size; pname++)
		if(entersends[pname] && !conf.getourid(pname).empty())
		    tmp += conf.getprotocolname(pname) + " ";

	    t.addleaff(i, 0, 25, _(" Enter key sends message for : %s"), tmp.c_str());

	} else {
	    t.addleaff(i, 0, 16, _(" Chat messaging mode : %s "), stryesno(chatmode[icq]));
	    t.addleaff(i, 0, 25, _(" Enter key sends message : %s "), stryesno(entersends[icq]));

	}

	for(tmp = "", pname = icq; pname != protocolname_size; pname++)
	    if(nonimonline[pname]) tmp += conf.getprotocolname(pname) + " ";

	t.addleaff(i, 0, 29, _(" Always online non-IM contacts for : %s"), tmp.c_str());
	t.addleaff(i, 0, 30, _(" Color contacts according to: %s "), strcolormode(cm));

	i = t.addnode(_(" Communications "));
	t.addleaff(i, 0, 19, _(" SMTP server : %s "), smtp.c_str());
	t.addleaff(i, 0, 24, _(" HTTP proxy server : %s "), httpproxy.c_str());

	if(!httpproxy.empty())
	    t.addleaff(i, 0, 11, _(" Proxy only for HTTP (rss and lj) : %s "), stryesno(!proxyconnect));

	t.addleaff(i, 0, 21, _(" Enable peer-to-peer communications : %s "), stryesno(ptp));

	if(ptp)
	    t.addleaff(i, 0, 22, _(" Port range to use for peer-to-peer : %s "), (i2str(ptpmin) + "-" + i2str(ptpmax)).c_str());

	i = t.addnode(_(" Logging "));
	t.addleaff(i, 0, 9, _(" Timestamps in the log window : %s "), stryesno(logtimestamps));
	t.addleaff(i, 0, 10, _(" Online/offile events in the log window : %s "), stryesno(logonline));
	t.addleaff(i, 0, 18, _(" Detailed IM events log in ~/.centericq/log : %s "), stryesno(makelog));

	i = t.addnode(_(" Miscellaneous "));
	t.addleaff(i, 0, 4, _(" Automatically set Away period (min) : %d "), aaway);
	t.addleaff(i, 0, 5, _(" Automatically set N/A period (min) : %d "), ana);

	void *p;
	finished = !db.open(n, b, &p);
	i = (int) p;

	if(!finished)
	switch(b) {
	    case 0:
		switch(i) {
		    case 1:
			s = s == icqconf::rsdontchange ? icqconf::rscard :
			    s == icqconf::rscard ? icqconf::rsspeaker :
			    s == icqconf::rsspeaker ? icqconf::rsdisable :
				icqconf::rsdontchange;
			break;
		    case 2:
			c = c == icqconf::rcdontchange ? icqconf::rcdark :
			    c == icqconf::rcdark ? icqconf::rcblue :
				icqconf::rcdontchange;
			break;
		    case 3:
			if(hasany) selectproto(conv, spIMplusRSS); else
			    for(pname = icq; pname != protocolname_size; pname++)
				conv[pname] = !conv[pname];
			break;
		    case 4:
			tmp = inputstr(_("Auto Away period (0 - disable): "), i2str(aaway));
			if(!tmp.empty()) aaway = atol(tmp.c_str());
			break;
		    case 5:
			tmp = inputstr(_("Auto N/A period (0 - disable): "), i2str(ana));
			if(!tmp.empty()) ana = atol(tmp.c_str());
			break;
		    case 6: hideoffl = !hideoffl; break;
		    case 7: askaway = !askaway; break;
		    case 8: quote = !quote; break;
		    case 9: logtimestamps = !logtimestamps; break;
		    case 10: logonline = !logonline; break;
		    case 11: proxyconnect = !proxyconnect; break;
		    case 13: savepwd = !savepwd; break;
		    case 14: antispam = !antispam; break;
		    case 15: mailcheck = !mailcheck; break;
		    case 16:
			if(hasany) selectproto(chatmode); else
			    for(pname = icq; pname != protocolname_size; pname++)
				chatmode[pname] = !chatmode[pname];
			break;

		    case 17:
			gmode =
			    gmode == icqconf::group1 ? icqconf::group2 :
			    gmode == icqconf::group2 ? icqconf::nogroups :
				icqconf::group1;
			break;
		    case 18: makelog = !makelog; break;
		    case 19:
			tmp = inputstr(_("SMTP server hostname: "), smtp);
			if(!tmp.empty()) smtp = tmp;
			break;
		    case 20: bidi = !bidi; break;
		    case 21:
			ptp = !ptp;
			if(ptp && !ptpmax)
			    ptpmin = 0, ptpmax = 65535;
			break;
		    case 22:
			tmp = inputstr(_("Peer-to-peer port range (min-max): "),
			    (i2str(ptpmin) + "-" + i2str(ptpmax)).c_str());

			if(tmp.size()) {
			    int tptpmin = atoi(getword(tmp, "-").c_str());
			    int tptpmax = atoi(tmp.c_str());

			    if(tptpmax)
			    if(tptpmin < tptpmax)
				ptpmin = tptpmin, ptpmax = tptpmax;
			}
			break;
		    case 23: emacs = !emacs; break;
		    case 24:
			httpproxy = inputstr(_("HTTP proxy server hostname: "), httpproxy);
			break;
		    case 25:
			if(hasany) selectproto(entersends); else
			    for(pname = icq; pname != protocolname_size; pname++)
				entersends[pname] = !entersends[pname];
			break;
		    case 26:
			iconvlang++;
			if(iconvlang == convlanguages.end())
			    iconvlang = convlanguages.begin();

			switch(iconvlang-convlanguages.begin()) {
			    case 0:
				convertto = convertfrom = "";
				// multichange(conv, false);
				break;

			    case 1:
				convertfrom = "cp1251";
				convertto = "koi8-r";
				multichange(conv, true);
				break;

			    case 2:
				convertfrom = "cp1250";
				convertto = "iso-8859-2";
				multichange(conv, true);
				break;
			}
			break;
		    case 27:
			convertfrom = inputstr(_("Charset to convert messages from: "), convertfrom);
			if(input.getlastkey() == '\r') iconvlang = convlanguages.begin();
			break;
		    case 28:
			convertto = inputstr(_("Charset to convert messages to: "), convertto);
			if(input.getlastkey() == '\r') iconvlang = convlanguages.begin();
			break;
		    case 29:
			selectproto(nonimonline, spnonIM);
			break;
		    case 30:
			cm = (cm == icqconf::cmproto ? icqconf::cmstatus : icqconf::cmproto );
		}
		break;
	    case 1:
		finished = success = true;
		conf.setquote(quote);
		conf.setsavepwd(savepwd);
		conf.setauto(aaway, ana);
		conf.sethideoffline(hideoffl);
		conf.setemacs(emacs);
		conf.setantispam(antispam);
		conf.setmailcheck(mailcheck);
		conf.setmakelog(makelog);
		conf.setaskaway(askaway);
		conf.setproxyconnect(proxyconnect);
		conf.setcharsets(convertfrom, convertto);

		for(pname = icq; pname != protocolname_size; pname++) {
		    conf.setchatmode(pname, chatmode[pname]);
		    conf.setentersends(pname, entersends[pname]);
		    conf.setnonimonline(pname, nonimonline[pname]);

		    bool bconv = conv[pname] && (!convertfrom.empty() || !convertto.empty());
		    conf.setcpconvert(pname, bconv || !hasany);
		}

		conf.setbidi(bidi);
		conf.setlogoptions(logtimestamps, logonline);

		if(ptp) conf.setpeertopeer(ptpmin, ptpmax);
		    else conf.setpeertopeer(0, 0);

		if(conf.getgroupmode() != gmode) {
		    conf.setgroupmode(gmode);
		    clist.rearrange();
		}
		conf.setcolormode(cm);

		conf.setsmtphost(smtp);
		conf.sethttpproxyhost(httpproxy);
		conf.save();
		break;
	}
    }

    db.close();
    unblockmainscreen();

    return success;
}

void icqface::selectproto(bool prots[], spmode mode) {
    static int saveelem = 0;
    int i, protmax;
    bool r, finished = false;

    protocolname pname;
    protocolname tempprots[protocolname_size];
    bool aprots[protocolname_size];

    i = 0;
    memcpy(aprots, prots, sizeof(aprots));

    for(pname = icq; pname != protocolname_size; pname++) {
	if(mode == spnonIM) {
	    if(pname != infocard)
	    if(!gethook(pname).getCapabs().count(hookcapab::nochat))
		continue;

	    if(pname == livejournal)
		continue;

	} else {
	    if(mode != spIMplusRSS || pname != rss)
	    if(pname != livejournal || mode == spIMonly) {
		if(gethook(pname).getCapabs().count(hookcapab::nochat))
		    continue;

		if(conf.getourid(pname).empty())
		    continue;

	    }

	    if(!gethook(pname).enabled())
		continue;
	}

	tempprots[i++] = pname;
    }

    protmax = i;

    vector<imstatus> mst;
    vector<imstatus>::iterator im;

    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-5-protmax, 18, LINES-4, conf.getcolor(cp_dialog_menu)));

    m.idle = &menuidle;
    m.otherkeys = &multiplekeys;
    m.setpos(saveelem);

    while(!finished) {
	saveelem = m.getpos();
	m.clear();

	for(i = 0; i < protmax; i++)
	    m.additemf(0, i, "[%c] %s", aprots[tempprots[i]] ? 'x' : ' ',
		conf.getprotocolname(tempprots[i]).c_str());

	m.setpos(saveelem);

	switch(m.open()) {
	    case -2:
		aprots[tempprots[m.getpos()]] = !aprots[tempprots[m.getpos()]];
		break;

	    default:
		finished = true;
		break;
	}
    }

    m.close();

    if(m.getlastkey() != KEY_ESC)
	memcpy(prots, aprots, sizeof(aprots));
}

int icqface::editaboutkeys(texteditor &e, int k) {
    switch(k) {
	case CTRL('x'): face.editdone = true; return -1;
	case 27: return -1;
    }

    return 0;
}

void icqface::detailsidle(dialogbox &db) {
    icqcontact *c;

    if(!face.detailsfetched)
    if(c = clist.get(contactroot))
    if(c->updated()) {
	face.gendetails(db.gettree());
	db.gettree()->redraw();
	face.status(_("Your details have been fetched"));
	face.detailsfetched = true;
    }

    cicq.idle(HIDL_SOCKEXIT);
}

int icqface::selectgroup(const string &text) {
    return groupmanager(text, true);
}

void icqface::organizegroups() {
    groupmanager(_("Organize contact groups"), false);
}

int icqface::groupmanager(const string &text, bool sel) {
    dialogbox db;
    static int n = 0;
    int r, ngrp, b, id;
    vector<icqgroup>::iterator i;
    string gname;
    bool touched = false;

    textwindow w(0, 0, sizeDlg.width, sizeDlg.height,
	conf.getcolor(cp_dialog_frame), TW_CENTERED);

    w.set_title(conf.getcolor(cp_dialog_highlight), " " + text + " ");

    db.setwindow(&w, false);

    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), conf.getcolor(cp_dialog_highlight),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	_("Add"), _("Rename"), _("rEmove"), _("move Up"), _("move Down"),
	sel ? _("Select") : _("Done"), 0));

    db.addautokeys();
    db.getbar()->item = 5;

    r = 0;
    db.idle = &dialogidle;
    blockmainscreen();
    treeview &t = *db.gettree();

    db.addkey(KEY_IC, 0);
    db.addkey(KEY_DC, 2);

    for(bool fin = false; !fin && !r; ) {
	t.clear();
	ngrp = t.addnode(_(" Groups "));

	sort(groups.begin(), groups.end());

	for(i = groups.begin(); i != groups.end(); ++i) {
	    b = i-groups.begin()+1;
	    id = t.addleaff(ngrp, 0, b, " %s ", i->getname().c_str());
	    if(n == b) t.setcur(id);
	}

	fin = !db.open(n, b, (void **) &n);

	if(!fin)
	switch(b) {
	    case 0:
		gname = inputstr(_("Name for a group to be created: "), "");
		if(!gname.empty()) {
		    groups.add(gname);
		    touched = true;
		}
		break;

	    case 1:
		if(n) {
		    i = groups.begin()+n-1;
		    gname = inputstr(_("New name for the group: "), i->getname());

		    if(input.getlastkey() != KEY_ESC)
		    if(!gname.empty()) {
			i->rename(gname);
			touched = true;
		    }
		}
		break;

	    case 2:
		if(n) {
		    i = groups.begin()+n-1;

		    if(i->getid() != 1) {
			if(ask(_("Are you sure want to remove the group?"), ASK_YES | ASK_NO, ASK_NO) == ASK_YES) {
			    groups.remove(i->getid());
			    clist.rearrange();
			    touched = true;
			}
		    }
		}
		break;

	    case 3:
		if(n) {
		    i = groups.begin()+n-1;
		    i->moveup();
		    n--;
		    touched = true;
		}
		break;

	    case 4:
		if(n) {
		    i = groups.begin()+n-1;
		    i->movedown();
		    n++;
		    touched = true;
		}
		break;

	    case 5:
		if(n) {
		    i = groups.begin()+n-1;
		    r = i->getid();
		} else if(!sel) {
		    fin = true;
		}
		break;
	}
    }

    if(touched) groups.save();

    db.close();
    unblockmainscreen();
    return r;
}

void icqface::transfermonitor() {
    dialogbox db;
    int n, b, np, cid;
    vector<filetransferitem>::iterator it;
    string fitem;

    vector< pair< pair<imcontact, imevent::imdirection>, int> > nodes;
    vector< pair< pair<imcontact, imevent::imdirection>, int> >::iterator in;

    textwindow w(0, 0, sizeDlg.width, sizeDlg.height,
	conf.getcolor(cp_dialog_frame), TW_CENTERED);

    w.set_titlef(conf.getcolor(cp_dialog_highlight), " %s ", _("File transfer status"));

    db.setwindow(&w, false);

    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), conf.getcolor(cp_dialog_highlight),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), _("Cancel"), _("Remove"), 0));

    db.getbar()->item = 1;
    db.addautokeys();

    db.idle = &transferidle;
    blockmainscreen();
    treeview &t = *db.gettree();

    db.addkey(KEY_DC, 1);

    for(bool fin = false; !fin; ) {
	t.clear();
	nodes.clear();

	for(it = transfers.begin(); it != transfers.end(); ++it) {
	    pair<imcontact, imevent::imdirection>
		contdir(it->fr.getcontact(), it->fr.getdirection());

	    for(in = nodes.begin(); in != nodes.end(); ++in)
		if(in->first.first == contdir.first
		&& in->first.second == contdir.second)
		    break;

	    if(in != nodes.end()) {
		cid = in->second;

	    } else {
		if(contdir.second == imevent::incoming) {
		    cid = t.addnodef(_(" Incoming from %s "), contdir.first.totext().c_str());
		} else {
		    cid = t.addnodef(_(" Outgoing to %s "), contdir.first.totext().c_str());
		}

		nodes.push_back(make_pair(contdir, cid));

	    }

	    switch(it->status) {
		case icqface::tsInit: fitem = _("init"); break;
		case icqface::tsStart: fitem = _("start"); break;
		case icqface::tsProgress: fitem = _("work"); break;
		case icqface::tsFinish: fitem = _("done"); break;
		case icqface::tsError: fitem = _("err"); break;
		case icqface::tsCancel: fitem = _("abort"); break;
	    }

	    fitem = (string) "[" + fitem + "] " + it->fname + ", " +
		i2str(it->bdone);

	    if(it->btotal > 0)
		fitem += (string) _(" of ") + i2str(it->btotal);

	    int limit = sizeDlg.width-9;

	    if(fitem.size() > limit) {
		int pos = fitem.find("] ");
		fitem.erase(pos+2, fitem.size()-limit+2);
		fitem.insert(pos+2, "..");
	    }

	    t.addleaff(cid, 0, it-transfers.begin(), " %s ", fitem.c_str());
	}

	fin = !db.open(np, b, (void **) &n);

	if(!fin && !db.gettree()->menu.getaborted()) {
	    it = transfers.end();

	    if(!db.gettree()->isnode(db.gettree()->getid(np-1)))
		it = transfers.begin() + (int) db.getmenu()->getref(n-1);

	    switch(b) {
		case 0:
		    if(it != transfers.end())
			gethook(it->fr.getcontact().pname).aborttransfer(it->fr);
		    break;

		case 1:
		    if(it != transfers.end())
		    switch(it->status) {
			case icqface::tsInit:
			case icqface::tsStart:
			case icqface::tsProgress:
			    status(_("Cannot remove a transfer which is in progress now"));
			    break;

			case icqface::tsFinish:
			case icqface::tsError:
			case icqface::tsCancel:
			    transfers.erase(it);
			    break;
		    }
		    break;
	    }
	}
    }

    db.close();
    unblockmainscreen();
}

void icqface::transferupdate(const string &fname, const imfile &fr,
transferstatus st, int btotal, int bdone) {
    vector<filetransferitem>::iterator i = transfers.begin();

    while(i != transfers.end()) {
	if(i->fr.getcontact() == fr.getcontact()
	&& i->fr.getdirection() == fr.getdirection()
	&& i->fname == fname)
	    break;
	++i;
    }

    if(i == transfers.end()) {
	transfers.push_back(filetransferitem(fr));
	i = transfers.end()-1;
	i->fname = fname;
    }

    if(btotal) i->btotal = btotal;
    if(bdone) i->bdone = bdone;
    i->status = st;
}

void icqface::invokelist(string &s, vector<string> &v, const string &def, textwindow *w) {
    int delems = (int) ((w->y2-w->y1)/2);
    int vmax = v.size() > delems ? delems : v.size();
    if(vmax < 2) return;
    vmax++;

    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(w->x1, w->y2-vmax, w->x1+30, w->y2, conf.getcolor(cp_dialog_menu)));
    m.idle = &menuidle;

    vector<string>::const_iterator iv = v.begin();
    while(iv != v.end()) {
	string cs = *iv;
	if(cs.empty()) cs = def;
	m.additem((string) " " + cs);
	if(cs == s) m.setpos(iv-v.begin());
	++iv;
    }

    int i = m.open();
    m.close();

    if(i) s = v[i-1];
}

bool icqface::setljparams(imxmlevent *ev) {
    bool r = false;

#ifdef BUILD_LJ

    #define LJP_LIST(x, l, d) \
	tmp = ev->getfield(x); \
	invokelist(tmp, l, d, db.getwindow()); \
	ev->setfield(x, tmp);

    #define LJP_STR(x, m) \
	ev->setfield(x, inputstr(m, ev->getfield(x)));

    #define LJP_BOOL(x) \
	ev->setfield(x, ev->getfield(x) == "1" ? "" : "1"); 

    int i, n, b;
    dialogbox db;

    vector<string> journals, moods, pictures;
    vector<string>::iterator im;
    string tmp;
    bool custmood;

    static vector<string> snames, svalues;

    journals = lhook.getjournals();
    pictures = lhook.getpictures();
    moods = lhook.getmoods();

    ev->setfield("_eventkind", "posting");

    if(ev->field_empty("journal") && !journals.empty())
	ev->setfield("journal", journals.front());

    if(ev->field_empty("mood") && !moods.empty())
	ev->setfield("mood", moods.front());

    if(ev->field_empty("picture") && !pictures.empty())
	ev->setfield("picture", pictures.front());

    if(ev->field_empty("security"))
	ev->setfield("security", "public");

    if(snames.empty()) {
	snames.push_back(_("public (visible to all)"));
	svalues.push_back("public");
	snames.push_back(_("private"));
	svalues.push_back("private");
	snames.push_back(_("friends only"));
	svalues.push_back("usemask");
    }

    textwindow w(0, 0, sizeDlg.width, sizeDlg.height, conf.getcolor(cp_dialog_frame), TW_CENTERED);
    w.set_title(conf.getcolor(cp_dialog_highlight), _(" LiveJournal posting: attributes "));
    db.setwindow(&w, false);

    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), conf.getcolor(cp_dialog_highlight),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text), conf.getcolor(cp_dialog_selected),
	_("Change"), _("Detect music"), _("Post"), _("cAncel"), 0));

    db.addautokeys();
    db.idle = &dialogidle;

    blockmainscreen();
    treeview &t = *db.gettree();

    for(bool fin = false; !fin && !r; ) {
	t.clear();

	i = t.addnode(_(" General "));
	t.addleaff(i, 0, 10, _(" Post to journal : %s "), ev->getfield("journal").c_str());
	t.addleaff(i, 0, 11, _(" Subject : %s "), ev->getfield("subject").c_str());

	vector<string>::iterator is = find(svalues.begin(), svalues.end(), ev->getfield("security"));
	if(is == svalues.end()) is = svalues.begin();
	tmp = is != svalues.end() ? snames[is-svalues.begin()] : "";
	t.addleaff(i, 0, 12, _(" Security : %s "), tmp.c_str());

	i = t.addnode(_(" Fancy stuff "));

	custmood = find(moods.begin(), moods.end(), ev->getfield("mood")) == moods.end() || ev->getfield("mood").empty();
	t.addleaff(i, 0, 20, _(" Mood : %s "), custmood ? _("(none/custom)") : ev->getfield("mood").c_str());
	if(custmood) t.addleaff(i, 0, 23, _(" Custom mood : %s "), ev->getfield("mood").c_str());

	t.addleaff(i, 0, 21, _(" Music : %s "), ev->getfield("music").c_str());
	t.addleaff(i, 0, 22, _(" Picture : %s "), ev->getfield("picture").empty() ? _("(default)") : ev->getfield("picture").c_str());

	i = t.addnode(_(" Options "));
	t.addleaff(i, 0, 30, _(" Disable auto-formatting : %s "), stryesno(ev->getfield("preformatted") == "1"));
	t.addleaff(i, 0, 31, _(" Disable sending comments by e-mail : %s "), stryesno(ev->getfield("noemail") == "1"));
	t.addleaff(i, 0, 32, _(" Disallow comments : %s "), stryesno(ev->getfield("nocomments") == "1"));
	t.addleaff(i, 0, 33, _(" Backdated entry : %s "), stryesno(ev->getfield("backdated") == "1"));

	fin = !db.open(n, b, (void **) &n);

	if(!fin) {
	    if(b == 0) {
		switch(n) {
		    case 10: LJP_LIST("journal", journals, ""); break;
		    case 11: LJP_STR("subject", _("Posting subject: ")); break;
		    case 12:
			tmp = snames[find(svalues.begin(), svalues.end(), ev->getfield("security"))-svalues.begin()];
			invokelist(tmp, snames, "", db.getwindow());
			tmp = svalues[find(snames.begin(), snames.end(), tmp)-snames.begin()];
			ev->setfield("security", tmp);
			break;

		    case 20: LJP_LIST("mood", moods, _("(none/custom)")); break;
		    case 21: LJP_STR("music", _("Currently playing: ")); break;
		    case 22: LJP_LIST("picture", pictures, _("(default)")); break;
		    case 23: LJP_STR("mood", _("Current mood: ")); break;
		    case 30: LJP_BOOL("preformatted"); break;
		    case 31: LJP_BOOL("noemail"); break;
		    case 32: LJP_BOOL("nocomments"); break;
		    case 33: LJP_BOOL("backdated"); break;
		}

	    } else if(b == 1) {
		ev->setfield("music", conf.execaction("detectmusic"));

	    } else if(b == 2) {
		r = true;

	    } else if(b == 3) {
		r = false;
		fin = true;

	    }
	}
    }

    db.close();
    unblockmainscreen();

#endif

    return r;
}

void icqface::findpgpkey(dialogbox &db, const vector<string> &items) {
    bool fin;
    string sub, upsub;
    int k, i;

    xtermtitle(_("pgp key quick search"));

    status(_("PGP key search: type to find, %s find again, Enter finish"),
	getstatkey(key_quickfind, section_contact).c_str());

    textwindow *w = db.getwindow();
    verticalmenu *cm = db.getmenu();

    for(fin = false; !fin; ) {
	attrset(conf.getcolor(cp_dialog_frame));
	mvhline(w->y2-2, w->x1+1, HLINE, w->x2-w->x1-2);
	kwriteatf(w->x1+2, w->y2-2, conf.getcolor(cp_dialog_highlight), "[ %s ]", sub.c_str());
	kgotoxy(w->x1+4+sub.size(), w->y2-2);
	refresh();

	if(cicq.idle())
	switch(k = getkey()) {
	    case KEY_ESC:
	    case '\r':
		fin = true;
		break;

	    case KEY_BACKSPACE:
	    case CTRL('h'):
		if(!sub.empty()) sub.resize(sub.size()-1);
		    else fin = true;
		break;

	    default:
		if(isprint(k) || key2action(k, section_contact) == key_quickfind) {
		    i = cm->getpos()+1;

		    if(isprint(k)) {
			i--;
			sub += k;
		    }

		    if(i <= items.size()) {
			upsub = up(sub);
			vector<string>::const_iterator it = items.begin()+i;
			bool passed = false;

			while(1) {
			    if(it == items.end()) {
				if(!passed) {
				    passed = true;
				    it = items.begin();
				    continue;
				} else {
				    break;
				}
			    }

			    if(it->find(upsub) != -1) {
				cm->setpos(it-items.begin());
				break;
			    }
			    ++it;
			}

			cm->redraw();
		    }
		}
		break;
	}
    }

    attrset(conf.getcolor(cp_dialog_frame));
    mvhline(w->y2-2, w->x1+1, HLINE, w->x2-w->x1-2);
    refresh();
}

bool icqface::selectpgpkey(string &keyid, bool secretonly) {
    bool r = false;

#ifdef HAVE_GPGME
    int n, b;
    dialogbox db;
    bool msb;
    string buf;
    vector<string> items;

    textwindow w(0, 0, sizeDlg.width, sizeDlg.height, conf.getcolor(cp_dialog_frame), TW_CENTERED);
    w.set_title(conf.getcolor(cp_dialog_highlight), _(" Select PGP key to use "));
    db.setwindow(&w, false);

    db.setmenu(new verticalmenu(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), _("Select"), 0));

    db.addautokeys();

    db.idle = &dialogidle;
    db.otherkeys = &findpgpkeys;

    verticalmenu &m = *db.getmenu();

    if(!(msb = mainscreenblock)) blockmainscreen();

    vector<string> keys = pgp.getkeys(secretonly);
    vector<string>::const_iterator i = keys.begin();

    m.additemf(0, 0, _(" Use no key"));
    items.push_back("");

    while(i != keys.end()) {
	buf = pgp.getkeyinfo(*i, secretonly);
	m.additem(0, 0, (string) " " + buf);
	items.push_back(up(buf));
	if(*i == keyid) m.setpos(m.getcount()-1);
	++i;
    }

    keys.insert(keys.begin(), "");

    for(bool fin = false; !fin && !r; ) {
	xtermtitle(_("pgp key selection"));

	status(_("PGP key selection: %s for quick lookup"),
	    getstatkey(key_quickfind, section_contact).c_str());

	fin = !db.open(n, b);

	if(!fin) {
	    if(n < 1) {
		if(n == -3)
		    findpgpkey(db, items);

	    } else if(!b) {
		keyid = keys[n-1];
		r = fin = true;
	    }
	}
    }

    db.close();
    if(!msb) unblockmainscreen();

#endif

    return r;
}
