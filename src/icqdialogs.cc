/*
*
* centericq user interface class, dialogs related part
* $Id: icqdialogs.cc,v 1.60 2002/03/23 11:34:51 konst Exp $
*
* Copyright (C) 2001 by Konstantin Klyagin <konst@konst.org.ua>
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

bool icqface::finddialog(imsearchparams &s) {
    int n, b, i;
    int nuin, ninfo, nloc, nwork, nonl;
    bool finished, ret;
    dialogbox db;
    vector<protocolname> penabled;
    vector<protocolname>::iterator ipname;

    for(protocolname apname = icq; apname != protocolname_size; (int) apname += 1) {
	if(gethook(apname).logged() || apname == infocard) {
	    penabled.push_back(apname);
	}
    }

    if(penabled.empty()) {
	log(_("+ you must be logged in to find/add users"));
	return false;
    }

    if((ipname = find(penabled.begin(), penabled.end(),
    s.pname)) == penabled.end()) {
	ipname = penabled.begin();
    }

    blockmainscreen();
    finished = ret = false;

    db.setwindow(new textwindow(0, 0, sizeDlg.width, sizeDlg.height,
	conf.getcolor(cp_dialog_frame), TW_CENTERED,
	conf.getcolor(cp_dialog_highlight), _(" Find/add user(s) ")));

    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	conf.getcolor(cp_dialog_highlight),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	_("cLear"), _("Change"), _("Search/Add"), 0));

    db.addautokeys();
    db.idle = &dialogidle;
    treeview &tree = *db.gettree();
    db.getbar()->item = 1;

    while(!finished) {
	tree.clear();

	i = tree.addnode(_(" Network "));
	tree.addleaf(i, 0, 1, " " + conf.getprotocolname(*ipname) + " ");

	switch(*ipname) {
	    case icq:
		i = tree.addnode(_(" UIN "));
		tree.addleaff(i, 0, 10, " %s ", strint(s.uin));

		i = tree.addnode(_(" Details "));
		tree.addleaff(i, 0, 11, _(" Nickname : %s "), s.nick.c_str());
		tree.addleaff(i, 0, 12, _(" E-Mail : %s "), s.email.c_str());
		tree.addleaff(i, 0, 13, _(" First name : %s "), s.firstname.c_str());
		tree.addleaff(i, 0, 14, _(" Last name : %s "), s.lastname.c_str());

		tree.addleaff(i, 0, 15, _(" Age range : %s "), stragerange[s.agerange]);
		tree.addleaff(i, 0, 17, _(" Gender : %s "), strgender[s.gender]);
		tree.addleaff(i, 0, 18, _(" Language : %s "),
		    ICQ2000::UserInfoHelpers::getLanguageIDtoString(s.language).c_str());

		i = tree.addnode(_(" Location "));
		tree.addleaff(i, 0, 19, _(" City : %s "), s.city.c_str());
		tree.addleaff(i, 0, 20, _(" State : %s "), s.state.c_str());
		tree.addleaff(i, 0, 21, _(" Country : %s "),
		    ICQ2000::UserInfoHelpers::getCountryIDtoString(s.country).c_str());

		i = tree.addnode(_(" Work "));
		tree.addleaff(i, 0, 22, _(" Company : %s "), s.company.c_str());
		tree.addleaff(i, 0, 23, _(" Department : %s "), s.department.c_str());
		tree.addleaff(i, 0, 24, _(" Position : %s "), s.position.c_str());

		i = tree.addnode(_(" Online only "));
		tree.addleaff(i, 0, 25, " %s ", stryesno[s.onlineonly]);
		break;

	    default:
		i = tree.addnode(_(" Nickname "));
		tree.addleaf(i, 0, 11, " " + s.nick + " ");
		break;
	}

	finished = !db.open(n, b, (void **) &i);

	if(!finished)
	switch(b) {
	    case 0:
		s = imsearchparams();
		break;
	    case 1:
		switch(i) {
		    case 1:
			if(++ipname == penabled.end()) {
			    ipname = penabled.begin();
			}
			break;
		    case 10: s.uin = atol(inputstr(_("UIN: "), strint(s.uin)).c_str()); break;
		    case 11: s.nick = inputstr(_("Nickname: "), s.nick); break;
		    case 12: s.email = inputstr(_("E-Mail: "), s.email); break;
		    case 13: s.firstname = inputstr(_("First name: "), s.firstname); break;
		    case 14: s.lastname = inputstr(_("Last name: "), s.lastname); break;
		    case 15: selectagerange(s.agerange); break;
		    case 17: selectgender(s.gender); break;
		    case 18: selectlanguage(s.language); break;
		    case 19: s.city = inputstr(_("City: "), s.city); break;
		    case 20: s.state = inputstr(_("State: "), s.state); break;
		    case 21: selectcountry(s.country); break;
		    case 22: s.company = inputstr(_("Company: "), s.company); break;
		    case 23: s.department = inputstr(_("Department: "), s.department); break;
		    case 24: s.position = inputstr(_("Position: "), s.position); break;
		    case 25: s.onlineonly = !s.onlineonly; break;
		}
		break;

	    case 2:
		ret = finished = true;
		s.pname = *ipname;
		break;
	}
    }

    db.close();
    unblockmainscreen();

    return ret;
}

void icqface::gendetails(treeview *tree, icqcontact *c) {
    int saveitem, savefirst, i;

    if(!c) c = clist.get(contactroot);
    detailsfetched = false;

    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();
    icqcontact::workinfo wi = c->getworkinfo();
    string about = c->getabout();
    bool ourdetails = c->getdesc() == contactroot;

    tree->menu.getpos(saveitem, savefirst);
    tree->clear();

    i = tree->addnode(_(" General "));

    if((passinfo.pname == infocard) || (gethook(passinfo.pname).getcapabilities() & hoptCanChangeNick) || !ourdetails) {
	tree->addleaff(i, 0, 10, _(" Nickname : %s "), c->getnick().c_str());
    }

    if(!(passinfo.pname == aim) && (c->getdesc() == contactroot) || !ourdetails) {

	tree->addleaff(i, 0, 11, _(" First name : %s "), bi.fname.c_str());
	tree->addleaff(i, 0, 12, _(" Last name : %s "), bi.lname.c_str());
	tree->addleaff(i, 0, 13, _(" E-mail : %s "), bi.email.c_str());
	tree->addleaff(i, 0, 14, _(" Gender : %s "), strgender[mi.gender]);
	tree->addleaff(i, 0, 15, _(" Birthdate : %s "), mi.strbirthdate().c_str());
	tree->addleaff(i, 0, 16, _(" Age : %s "), strint(mi.age));

	i = tree->addnode(_(" Home "));
	tree->addleaff(i, 0, 17, _(" City : %s "), bi.city.c_str());
	tree->addleaff(i, 0, 18, _(" State : %s "), bi.state.c_str());
	tree->addleaff(i, 0, 19, _(" Country : %s "),
	    ICQ2000::UserInfoHelpers::getCountryIDtoString(bi.country).c_str());

	tree->addleaff(i, 0, 20, _(" Street address : %s "), bi.street.c_str());
	tree->addleaff(i, 0, 21, _(" Zip code : %s "), bi.zip.c_str());
	tree->addleaff(i, 0, 22, _(" Phone : %s "), bi.phone.c_str());
	tree->addleaff(i, 0, 23, _(" Fax : %s "), bi.fax.c_str());
	tree->addleaff(i, 0, 24, _(" Cellular phone : %s "), bi.cellular.c_str());

	i = tree->addnode(_(" Work "));
	tree->addleaff(i, 0, 25, _(" City : %s "), wi.city.c_str());
	tree->addleaff(i, 0, 26, _(" State : %s "), wi.state.c_str());
	tree->addleaff(i, 0, 27, _(" Country : %s "),
	    ICQ2000::UserInfoHelpers::getCountryIDtoString(wi.country).c_str());

	tree->addleaff(i, 0, 28, _(" Street address : %s "), wi.street.c_str());
	tree->addleaff(i, 0, 29, _(" Company : %s "), wi.company.c_str());
	tree->addleaff(i, 0, 30, _(" Department : %s "), wi.dept.c_str());
	tree->addleaff(i, 0, 31, _(" Position : %s "), wi.position.c_str());
	tree->addleaff(i, 0, 32, _(" Homepage : %s "), wi.homepage.c_str());
	tree->addleaff(i, 0, 33, _(" Phone : %s "), wi.phone.c_str());
	tree->addleaff(i, 0, 34, _(" Fax : %s "), wi.fax.c_str());

	i = tree->addnode(_(" More "));
	tree->addleaff(i, 0, 35, _(" Homepage : %s "), mi.homepage.c_str());

	tree->addleaff(i, 0, 36, _(" 1st language : %s "),
	    ICQ2000::UserInfoHelpers::getLanguageIDtoString(mi.lang1).c_str());

	tree->addleaff(i, 0, 37, _(" 2nd language : %s "),
	    ICQ2000::UserInfoHelpers::getLanguageIDtoString(mi.lang2).c_str());

	tree->addleaff(i, 0, 38, _(" 3rd language : %s "),
	    ICQ2000::UserInfoHelpers::getLanguageIDtoString(mi.lang3).c_str());

    }

    i = tree->addnode(_(" About "));
    tree->addleaff(i, 0, 39, " %s ", about.c_str());

    tree->menu.setpos(saveitem, savefirst);
}

bool icqface::updatedetails(icqcontact *c, protocolname upname) {
    string about, tmp;
    icqcontact::basicinfo bi;
    icqcontact::workinfo wi;
    icqcontact::moreinfo mi;
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
	about = c->getabout();

	if(!b) {
	    if((c->getdesc() == contactroot) && !c->updated()) {
		status(_("Wait a moment. Your details haven't been fetched yet"));
		continue;
	    } else
	    switch(citem) {
		case 10:
		    c->setnick(inputstr(_("Nickname: "), c->getnick()));
		    if(c->getdesc().pname == infocard) {
			c->setdispnick(c->getnick());
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
	    }

	    c->setbasicinfo(bi);
	    c->setmoreinfo(mi);
	    c->setworkinfo(wi);
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

    for(i = 0; i < ICQ2000::Country_table_size; i++) {
	m.additem(0, (int) ICQ2000::Country_table[i].code,
	    (string) " " + ICQ2000::Country_table[i].name);

	if(ICQ2000::Country_table[i].code == f) {
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

    for(i = 0; i < ICQ2000::Language_table_size; i++) {
	m.additemf(" %s", ICQ2000::Language_table[i]);
    }

    m.setpos(f);
    i = m.open();
    m.close();
    
    if(i) f = i-1;
}

void icqface::selectgender(imgender &f) {
    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-8, 20, LINES-4, conf.getcolor(cp_dialog_menu)));

    for(imgender i = genderUnspec; i != imgender_size; (int) i += 1) {
	m.additemf(0, (int) i, " %s", strgender[i]);
	if(i == f) m.setpos(m.getcount()-1);
    }

    int i = m.open();
    m.close();

    if(i) f = (imgender) ((int) m.getref(i-1));
}

void icqface::selectagerange(ICQ2000::AgeRange &r) {
    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-8, 18, LINES-3, conf.getcolor(cp_dialog_menu)));

    for(ICQ2000::AgeRange i = ICQ2000::range_NoRange; i <= ICQ2000::range_60_above; (int) i += 1) {
	const char *p = stragerange[i];

	if(i == ICQ2000::range_NoRange)
	    p = _("none");

	m.additemf(0, (int) i, " %s", p);
	if(i == r) m.setpos(m.getcount()-1);
    }

    int i = m.open();
    m.close();

    if(i) r = (ICQ2000::AgeRange) ((int) m.getref(i-1));
}

bool icqface::edit(string &txt, const string &header) {
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

    se.load(txt, "");
    se.open();

    if(editdone) {
	char *ctext = se.save("\r\n");
	txt = ctext;
	delete ctext;
    }

    w.close();
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

bool icqface::updateconf(icqconf::regsound &s, icqconf::regcolor &c) {
    bool finished, success;
    int nopt, n, i, b, nproxy, nconf, ncomm, aaway, ana, noth, nfeat, ncl;
    string tmp, phidden, socksuser, sockspass;
    string prserv = conf.getsockshost() + ":" + i2str(conf.getsocksport());
    string smtp = conf.getsmtphost() + ":" + i2str(conf.getsmtpport());

    bool quote = conf.getquote();
    bool rus = conf.getrussian();
    bool savepwd = conf.getsavepwd();
    bool socks = !conf.getsockshost().empty();
    bool hideoffl = conf.gethideoffline();
    bool antispam = conf.getantispam();
    bool mailcheck = conf.getmailcheck();
    bool makelog = conf.getmakelog();

    icqconf::groupmode gmode = conf.getgroupmode();

    dialogbox db;

    finished = success = false;
    if(conf.getsockshost().empty()) prserv = "";
    conf.getauto(aaway, ana);

    textwindow w(0, 0, sizeBigDlg.width, sizeBigDlg.height,
	conf.getcolor(cp_dialog_frame), TW_CENTERED);

    w.set_title(conf.getcolor(cp_dialog_highlight),
	_(" CenterICQ configuration ")
	);

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

	i = t.addnode(_(" Sound and colors "));
	t.addleaff(i, 0, 1, _(" Change sound device to : %s "), strregsound(s));
	t.addleaff(i, 0, 2, _(" Change color scheme to : %s "), strregcolor(c));

	i = t.addnode(_(" Contact list "));
	t.addleaff(i, 0, 17, _(" Arrange contacts into groups : %s "), strgroupmode(gmode));
	t.addleaff(i, 0,  6, _(" Hide offline users : %s "), stryesno[hideoffl]);
	t.addleaff(i, 0,  3, _(" Russian translation win1251-koi8 needed : %s "), stryesno[rus]);
	t.addleaff(i, 0, 14, _(" Anti-spam: kill msgs from users not on the list : %s "), stryesno[antispam]);
	t.addleaff(i, 0,  8, _(" Quote a message on reply : %s "), stryesno[quote]);
	t.addleaff(i, 0, 15, _(" Check the local mailbox : %s "), stryesno[mailcheck]);
	t.addleaff(i, 0, 13, _(" Remember passwords : %s "), stryesno[savepwd]);

	i = t.addnode(_(" Communications "));
	t.addleaff(i, 0, 19, _(" SMTP server : %s "), smtp.c_str());

/*
	t.addleaff(i, 0, 9, _(" Use SOCKS proxy : %s "), stryesno[socks]);
*/
	i = t.addnode(_(" Miscellaneous "));
	t.addleaff(i, 0, 4, _(" Automatically set Away period (min) : %d "), aaway);
	t.addleaff(i, 0, 5, _(" Automatically set N/A period (min) : %d "), ana);
	t.addleaff(i, 0, 18, _(" Detailed IM events log in ~/.centericq/log : %s "), stryesno[makelog]);
/*
	if(socks) {
	    conf.getsocksuser(socksuser, sockspass);
	    i = t.addnode(_(" SOCKS proxy settings "));
	    t.addleaff(i, 0, 10, _(" Proxy server address : %s "), prserv.c_str());
	    t.addleaff(i, 0, 11, _(" Proxy user name : %s "), socksuser.c_str());
	    t.addleaff(i, 0, 12, _(" Proxy password : %s "), phidden.assign(sockspass.size(), '*').c_str());
	}
*/
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
		    case 3: rus = !rus; break;
		    case 4:
			tmp = inputstr(_("Auto Away period (0 - disable): "), i2str(aaway));
			if(tmp.size()) aaway = atol(tmp.c_str());
			break;
		    case 5:
			tmp = inputstr(_("Auto N/A period (0 - disable): "), i2str(ana));
			if(tmp.size()) ana = atol(tmp.c_str());
			break;
		    case 6: hideoffl = !hideoffl; break;
		    case 8: quote = !quote; break;
		    case 9: socks = !socks; break;
		    case 10:
			tmp = inputstr(_("SOCKS proxy server to use: "), prserv);
			if(!tmp.empty()) prserv = tmp;
			break;
		    case 11:
			socksuser = inputstr(_("SOCKS proxy user name: "), socksuser);
			conf.setsocksuser(socksuser, sockspass);
			break;
		    case 12:
			sockspass = inputstr(_("SOCKS proxy password: "), sockspass, '*');
			conf.setsocksuser(socksuser, sockspass);
			break;
		    case 13: savepwd = !savepwd; break;
		    case 14: antispam = !antispam; break;
		    case 15: mailcheck = !mailcheck; break;
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
		}
		break;
	    case 1:
		finished = success = true;
		conf.setquote(quote);
		conf.setsavepwd(savepwd);
		conf.setauto(aaway, ana);
		conf.sethideoffline(hideoffl);
		conf.setantispam(antispam);
		conf.setmailcheck(mailcheck);
		conf.setrussian(rus);
		conf.setmakelog(makelog);

		if(conf.getgroupmode() != gmode) {
		    conf.setgroupmode(gmode);
		    clist.rearrange();
		}

		conf.setsockshost(socks ? prserv : "");
		conf.setsmtphost(smtp);
		break;
	}
    }

    db.close();
    unblockmainscreen();

    return success;
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

    textwindow w(0, 0, sizeDlg.width, sizeDlg.height,
	conf.getcolor(cp_dialog_frame), TW_CENTERED);

    w.set_title(conf.getcolor(cp_dialog_highlight), " " + text + " ");

    db.setwindow(&w, false);

    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), conf.getcolor(cp_dialog_highlight),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	_("Add"), _("Rename"), _("Remove"), sel ? _("Select") : _("Done"), 0));

    db.addautokeys();
    db.getbar()->item = 3;

    r = 0;
    db.idle = &dialogidle;
    blockmainscreen();
    treeview &t = *db.gettree();

    db.addkey(KEY_IC, 0);
    db.addkey(KEY_DC, 2);

    for(bool fin = false; !fin && !r; ) {
	t.clear();
	ngrp = t.addnode(_(" Groups "));

	for(i = groups.begin(); i != groups.end(); i++) {
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
		}
		break;

	    case 1:
		if(n) {
		    i = groups.begin()+n-1;
		    gname = inputstr(_("New name for the group: "), i->getname());
		    if(!gname.empty()) i->rename(gname);
		}
		break;

	    case 2:
		if(n) {
		    i = groups.begin()+n-1;

		    if(i->getid() != 1) {
			if(ask(_("Are you sure want to remove the group?"), ASK_YES | ASK_NO, ASK_NO) == ASK_YES) {
			    groups.remove(i->getid());
			    clist.rearrange();
			}
		    }
		}
		break;

	    case 3:
		if(n) {
		    i = groups.begin()+n-1;
		    r = i->getid();
		} else if(!sel) {
		    fin = true;
		}
		break;
	}
    }

    db.close();
    unblockmainscreen();
    return r;
}
