/*
*
* centericq user interface class, dialogs related part
* $Id: icqdialogs.cc,v 1.29 2001/11/26 13:02:52 konst Exp $
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
#include "icqhist.h"
#include "centericq.h"
#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqmlist.h"
#include "icqgroups.h"

#include "icqhook.h"
#include "yahoohook.h"

bool icqface::finddialog(icqhook::searchparameters &s) {
    int n, b, i;
    int nuin, ninfo, nloc, nwork, nonl;
    bool finished, ret;
    dialogbox db;
    vector<protocolname> penabled;
    vector<protocolname>::iterator ipname;

    for(protocolname apname = icq; apname != protocolname_size; (int) apname += 1) {
	if(gethook(apname).online()) {
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

    db.setwindow(new textwindow(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT,
	conf.getcolor(cp_dialog_frame), TW_CENTERED,
	conf.getcolor(cp_dialog_highlight), _(" Find user(s) ")));

    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	conf.getcolor(cp_dialog_highlight),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	_("cLear"), _("Change"), _("Search"), 0));

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
		tree.addleaff(i, 0, 10, _(" UIN : %s "), strint(s.uin));

		i = tree.addnode(_(" Details "));
		tree.addleaff(i, 0, 11, _(" Nickname : %s "), s.nick.c_str());
		tree.addleaff(i, 0, 12, _(" E-Mail : %s "), s.email.c_str());
		tree.addleaff(i, 0, 13, _(" First name : %s "), s.firstname.c_str());
		tree.addleaff(i, 0, 14, _(" Last name : %s "), s.lastname.c_str());
		tree.addleaff(i, 0, 15, _(" Min. age : %s "), strint(s.minage));
		tree.addleaff(i, 0, 16, _(" Max. age : %s "), strint(s.maxage));
		tree.addleaff(i, 0, 17, _(" Gender : %s "), strgender(s.gender));
		tree.addleaff(i, 0, 18, _(" Language : %s "), s.language ? icq_GetMetaLanguageName(s.language) : "");

		i = tree.addnode(_(" Location "));
		tree.addleaff(i, 0, 19, _(" City : %s "), s.city.c_str());
		tree.addleaff(i, 0, 20, _(" State : %s "), s.state.c_str());
		tree.addleaff(i, 0, 21, _(" Country : %s "), s.country ? icq_GetCountryName(s.country) : "");

		i = tree.addnode(_(" Work "));
		tree.addleaff(i, 0, 22, _(" Company : %s "), s.company.c_str());
		tree.addleaff(i, 0, 23, _(" Department : %s "), s.department.c_str());
		tree.addleaff(i, 0, 24, _(" Position : %s "), s.position.c_str());

		i = tree.addnode(_(" Online only "));
		tree.addleaff(i, 0, 25, " %s ", stryesno(s.onlineonly));
		break;

	    case yahoo:
	    case msn:
		i = tree.addnode(_(" Nickname "));
		tree.addleaf(i, 0, 11, " " + s.nick + " ");
		break;
	}

	finished = !db.open(n, b, (void **) &i);

	if(!finished)
	switch(b) {
	    case 0:
		s = icqhook::searchparameters();
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
		    case 15: s.minage = atol(inputstr(_("Minimal age: "), strint(s.minage)).c_str()); break;
		    case 16: s.maxage = atol(inputstr(_("Maximal age: "), strint(s.maxage)).c_str()); break;
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

void icqface::gendetails(treeview *tree, icqcontact *c = 0) {
    if(!c) c = clist.get(contactroot);

    string fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate;
    string fphone, ffax, fstreet, fcellular, fhomepage, fwcity, fwstate;
    string fwphone, fwfax, fwaddress, fcompany, fdepartment, fjob, fwhomepage;
    string fabout;

    unsigned char flang1, flang2, flang3, fbday, fbmonth, fbyear, fage, fgender;
    unsigned long fzip, fwzip;
    unsigned short fcountry, fwcountry, foccupation;

    c->getinfo(fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate, fphone, ffax, fstreet, fcellular, fzip, fcountry);
    c->getmoreinfo(fage, fgender, fhomepage, flang1, flang2, flang3, fbday, fbmonth, fbyear);
    c->getworkinfo(fwcity, fwstate, fwphone, fwfax, fwaddress, fwzip, fwcountry, fcompany, fdepartment, fjob, foccupation, fwhomepage);

    fabout = c->getabout();

    int saveitem, savefirst, i;
    
    tree->menu.getpos(saveitem, savefirst);
    tree->clear();

    i = tree->addnode(_(" General "));
    tree->addleaff(i, 0, 0, _(" Nickname : %s "), c->getnick().c_str());
    tree->addleaff(i, 0, 0, _(" First name : %s "), fname.c_str());
    tree->addleaff(i, 0, 0, _(" Last name : %s "), lname.c_str());
    tree->addleaff(i, 0, 0, _(" E-mail : %s "), fprimemail.c_str());
    tree->addleaff(i, 0, 0, _(" Second e-mail : %s "), fsecemail.c_str());
    tree->addleaff(i, 0, 0, _(" Old e-mail : %s "), foldemail.c_str());
    tree->addleaff(i, 0, 0, _(" Gender : %s "), strgender(fgender));
    tree->addleaff(i, 0, 0, _(" Birthdate : %s "), getbdate(fbday, fbmonth, fbyear).c_str());
    tree->addleaff(i, 0, 0, _(" Age : %s "), strint(fage));

    i = tree->addnode(_(" Home "));
    tree->addleaff(i, 0, 0, _(" City : %s "), fcity.c_str());
    tree->addleaff(i, 0, 0, _(" State : %s "), fstate.c_str());
    tree->addleaff(i, 0, 0, _(" Country : %s "), fcountry ? icq_GetCountryName(fcountry) : "");
    tree->addleaff(i, 0, 0, _(" Street address : %s "), fstreet.c_str());
    tree->addleaff(i, 0, 0, _(" Zip code : %s "), strint(fzip));
    tree->addleaff(i, 0, 0, _(" Phone : %s "), fphone.c_str());
    tree->addleaff(i, 0, 0, _(" Fax : %s "), ffax.c_str());
    tree->addleaff(i, 0, 0, _(" Cellular phone : %s "), fcellular.c_str());

    i = tree->addnode(_(" Work "));
    tree->addleaff(i, 0, 0, _(" City : %s "), fwcity.c_str());
    tree->addleaff(i, 0, 0, _(" State : %s "), fwstate.c_str());
    tree->addleaff(i, 0, 0, _(" Country : %s "), fwcountry ? icq_GetCountryName(fwcountry) : "");
    tree->addleaff(i, 0, 0, _(" Street address : %s "), fwaddress.c_str());
    tree->addleaff(i, 0, 0, _(" Company : %s "), fcompany.c_str());
    tree->addleaff(i, 0, 0, _(" Department : %s "), fdepartment.c_str());
    tree->addleaff(i, 0, 0, _(" Occupation : %s "), foccupation ? icq_GetMetaOccupationName(foccupation) : "");
    tree->addleaff(i, 0, 0, _(" Position : %s "), fjob.c_str());
    tree->addleaff(i, 0, 0, _(" Homepage : %s "), fwhomepage.c_str());
    tree->addleaff(i, 0, 0, _(" Phone : %s "), fwphone.c_str());
    tree->addleaff(i, 0, 0, _(" Fax : %s "), fwfax.c_str());

    i = tree->addnode(_(" More "));
    tree->addleaff(i, 0, 0, _(" Homepage : %s "), fhomepage.c_str());
    tree->addleaff(i, 0, 0, _(" 1st language : %s "), flang1 ? icq_GetMetaLanguageName(flang1) : "");
    tree->addleaff(i, 0, 0, _(" 2nd language : %s "), flang2 ? icq_GetMetaLanguageName(flang2) : "");
    tree->addleaff(i, 0, 0, _(" 3rd language : %s "), flang3 ? icq_GetMetaLanguageName(flang3) : "");

    i = tree->addnode(_(" About "));
    tree->addleaff(i, 0, 0, " %s ", fabout.c_str());

    tree->menu.setpos(saveitem, savefirst);
}

bool icqface::updatedetails(icqcontact *c = 0) {
    bool finished = false, ret = false;
    int n, b;
    dialogbox db;

    if(!c) {
	status(_("Fetching your ICQ details"));
	c = clist.get(contactroot);
	if(mainscreenblock) return false;
	    // Another dialog is already on top
    }

    blockmainscreen();

    textwindow w(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT, conf.getcolor(cp_dialog_frame), TW_CENTERED);

    if(!c->getdesc().uin) {
	w.set_title(conf.getcolor(cp_dialog_highlight), _(" Your ICQ details "));
    } else {
	w.set_titlef(conf.getcolor(cp_dialog_highlight), _(" %s's ICQ details "), c->getdispnick().c_str());
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
	finished = !db.open(n, b);

	string fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate;
	string fphone, ffax, fstreet, fcellular, fhomepage, fwcity, fwstate;
	string fwphone, fwfax, fwaddress, fcompany, fdepartment, fjob;
	string fwhomepage, fabout, tmp;

	unsigned char fage, flang1, flang2, flang3, fbday, fbmonth, fbyear, fgender;
	unsigned long fzip, fwzip;
	unsigned short fcountry, fwcountry, foccupation;

	c->getinfo(fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate, fphone, ffax, fstreet, fcellular, fzip, fcountry);
	c->getmoreinfo(fage, fgender, fhomepage, flang1, flang2, flang3, fbday, fbmonth, fbyear);
	c->getworkinfo(fwcity, fwstate, fwphone, fwfax, fwaddress, fwzip, fwcountry, fcompany, fdepartment, fjob, foccupation, fwhomepage);
	fabout = c->getabout();
	const char *pp = fabout.c_str();

	if(!b) {
	    if(!c->getdesc().uin && (c->updated() < 5)) {
		status(_("Wait a moment. Your ICQ details haven't been fetched yet"));
		continue;
	    } else
	    switch(n) {
		case  0: finished = true; break;
		case  2:
		    c->setnick(inputstr(_("Nickname: "), c->getnick()));
		    if(c->getdesc().pname == infocard) {
			c->setdispnick(c->getnick());
		    }
		    break;
		case  3: fname = inputstr(_("First name: "), fname); break;
		case  4: lname = inputstr(_("Last name: "), lname); break;
		case  5: fprimemail = inputstr(_("E-mail: "), fprimemail); break;
		case  6: fsecemail = inputstr(_("Second e-mail: "), fsecemail); break;
		case  7: foldemail = inputstr(_("Old e-mail: "), foldemail); break;
		case  8: selectgender(fgender); break;
		case  9:
		    tmp = i2str(fbday) + "-" + i2str(fbmonth) + "-" + i2str(1900+fbyear);
		    tmp = inputstr(_("Enter birthdate (DD-MM-YYYY): "), tmp);

		    if(tmp.size()) {
			int d, m, y;
			sscanf(tmp.c_str(), "%d-%d-%d", &d, &m, &y);

			if(y >= 1900)
			if((d > 0) && (d <= 31))
			if((m > 0) && (m <= 12)) {
			    fbday = d;
			    fbmonth = m;
			    fbyear = y-1900;
			}
		    } else {
			fbday = fbmonth = fbyear = 0;
		    }

		    break;
		case 10: fage = atoi(inputstr(_("Age: "), strint(fage)).c_str()); break;

		case 12: fcity = inputstr(_("City: "), fcity); break;
		case 13: fstate = inputstr(_("State: "), fstate); break;
		case 14: selectcountry(fcountry); break;
		case 15: fstreet = inputstr(_("Street address: "), fstreet); break;
		case 16: fzip = strtoul(inputstr(_("Zip code: "), strint(fzip)).c_str(), 0, 0); break;
		case 17: fphone = inputstr(_("Phone: "), fphone); break;
		case 18: ffax = inputstr(_("Fax: "), ffax); break;
		case 19: fcellular = inputstr(_("Cellular phone: "), fcellular); break;

		case 21: fwcity = inputstr(_("City: "), fwcity); break;
		case 22: fwstate = inputstr(_("State: "), fwstate); break;
		case 23: selectcountry(fwcountry); break;
		case 24: fwaddress = inputstr(_("Street address: "), fwaddress); break;
		case 25: fcompany = inputstr(_("Company: "), fcompany); break;
		case 26: fdepartment = inputstr(_("Department: "), fdepartment); break;
		case 27: selectoccupation(foccupation); break;
		case 28: fjob = inputstr(_("Position: "), fjob); break;
		case 29: fwhomepage = inputstr(_("Homepage: "), fwhomepage); break;
		case 30: fwphone = inputstr(_("Phone: "), fwphone); break;
		case 31: fwfax = inputstr(_("Fax: "), fwfax); break;

		case 33: fhomepage = inputstr(_("Homepage: "), fhomepage); break;
		case 34: selectlanguage(flang1); break;
		case 35: selectlanguage(flang2); break;
		case 36: selectlanguage(flang3); break;

		case 38: editabout(fabout); break;
	    }

	    c->setinfo(fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate, fphone, ffax, fstreet, fcellular, fzip, fcountry);
	    c->setmoreinfo(fage, fgender, fhomepage, flang1, flang2, flang3, fbday, fbmonth, fbyear);
	    c->setworkinfo(fwcity, fwstate, fwphone, fwfax, fwaddress, fwzip, fwcountry, fcompany, fdepartment, fjob, foccupation, fwhomepage);
	    c->setabout(fabout);
	} else {
	    ret = c->getdesc().uin || (c->updated() >= 5);
	    finished = true;
	}
    }

    db.close();
    unblockmainscreen();

    return ret;
}

void icqface::selectcountry(unsigned short &f) {
    int i;

    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-16, 30, LINES-3, conf.getcolor(cp_dialog_menu)));

    m.additem(0, 0, (string) " " + _("Not entered"));

    for(i = 0;; i++) {
	if(icq_Countries[i].code == 0xffff) break; else {
	    m.additem(0, (int) icq_Countries[i].code, (string) " " + icq_Countries[i].name);
	    if(icq_Countries[i].code == f) {
		m.setpos(i+1);
	    }
	}
    }

    i = m.open();
    m.close();

    if(i) f = (unsigned short) ((int) m.getref(i-1));
}

void icqface::selectlanguage(unsigned char &f) {
    int i;
    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-13, 20, LINES-4, conf.getcolor(cp_dialog_menu)));

    for(i = 0; i <= 67; i++) m.additemf(" %s", icq_GetMetaLanguageName(i));
    m.setpos(f);
    i = m.open();
    m.close();
    
    if(i) f = i-1;
}

void icqface::selectoccupation(unsigned short &f) {
    int i;
    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-16, 35, LINES-1, conf.getcolor(cp_dialog_menu)));

    extern icq_ArrayType icq_MetaOccupation[];

    m.additem(_(" Not entered"));

    for(i = 0;; i++)
    if(icq_MetaOccupation[i].code == 99) break; else {
	m.additemf(" %s", icq_MetaOccupation[i].name);
    }

    m.setpos(f);
    i = m.open();
    m.close();

    switch(i) {
	case 0: break;
	case 1: f = 0; break;
	default:
	    f = icq_MetaOccupation[i-2].code;
	    break;
    }
}

void icqface::selectgender(unsigned char &f) {
    int i;
    verticalmenu m(conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_selected));
    m.setwindow(textwindow(4, LINES-8, 20, LINES-4, conf.getcolor(cp_dialog_menu)));

    for(i = 0; i <= 2; i++) {
	m.additemf(0, i, " %s", strgender(i));
    }
    
    m.setpos(f);
    i = m.open();
    m.close();

    if(i) f = (int) m.getref(i-1);
}

void icqface::editabout(string &fabout) {
    texteditor se;
    textwindow w(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT, conf.getcolor(cp_dialog_frame), TW_CENTERED);
    w.set_title(conf.getcolor(cp_dialog_highlight), _(" About [Ctrl-X save, Esc cancel] "));
    w.open();

    editdone = false;
    se.setcoords(w.x1+1, w.y1+1, w.x2, w.y2);
    se.addscheme(cp_dialog_text, cp_dialog_text, 0, 0);
    se.idle = &editidle;
    se.otherkeys = &editaboutkeys;
    se.wrap = true;

    se.load(fabout, "");
    se.open();

    if(editdone) {
	char *ctext = se.save("\r\n");
	fabout = ctext;
	delete ctext;
    }

    w.close();
}

bool icqface::sendfiles(const imcontact cinfo, string &msg, linkedlist &flist) {
    int n, i, b;
    bool finished = false;
    string fname;
    dialogbox db;

    flist.freeitem = &charpointerfree;
    flist.empty();

    blockmainscreen();

    textwindow w(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT, conf.getcolor(cp_dialog_frame), TW_CENTERED);

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

bool icqface::updateconf(regsound &s, regcolor &c) {
    bool finished, success;
    int nopt, n, i, b, nproxy, nconf, ncomm, aaway, ana, noth, nfeat, ncl;
    string tmp, phidden, socksuser, sockspass;
    string prserv = conf.getsockshost() + ":" + i2str(conf.getsocksport());

    bool quote = conf.getquote();
    bool rus = icq_Russian == 1;
    bool savepwd = conf.getsavepwd();
    bool socks = !conf.getsockshost().empty();
    bool hideoffl = conf.gethideoffline();
    bool antispam = conf.getantispam();
    bool mailcheck = conf.getmailcheck();
    bool serveronly = conf.getserveronly();
    bool usegroups = conf.getusegroups();

    dialogbox db;

    finished = success = false;
    if(conf.getsockshost().empty()) prserv = "";
    conf.getauto(aaway, ana);

    textwindow w(0, 0, BIGDIALOG_WIDTH, BIGDIALOG_HEIGHT, conf.getcolor(cp_dialog_frame), TW_CENTERED);
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

	i = t.addnode(_(" Sound and colors "));
	t.addleaff(i, 0, 1, _(" Change sound device to : %s "), strregsound(s));
	t.addleaff(i, 0, 2, _(" Change color scheme to : %s "), strregcolor(c));

	i = t.addnode(_(" Contact list "));
	t.addleaff(i, 0, 17, _(" Arrange contacts into groups : %s "), stryesno(usegroups));
	t.addleaff(i, 0,  6, _(" Hide offline users : %s "), stryesno(hideoffl));
	t.addleaff(i, 0,  3, _(" Russian translation win1251-koi8 needed : %s "), stryesno(rus));
	t.addleaff(i, 0, 14, _(" Anti-spam: kill msgs from users not on the list : %s "), stryesno(antispam));
	t.addleaff(i, 0,  8, _(" Quote a message on reply : %s "), stryesno(quote));
	t.addleaff(i, 0, 15, _(" Check the local mailbox : %s "), stryesno(mailcheck));
	t.addleaff(i, 0, 16, _(" Send all events through server : %s "), stryesno(serveronly));
	t.addleaff(i, 0, 13, _(" Remember passwords : %s "), stryesno(savepwd));

	i = t.addnode(_(" Communications "));
	t.addleaff(i, 0, 9, _(" Use SOCKS proxy : %s "), stryesno(socks));

	i = t.addnode(_(" Miscellaneous "));
	t.addleaff(i, 0, 4, _(" Automatically set Away period (min) : %d "), aaway);
	t.addleaff(i, 0, 5, _(" Automatically set N/A period (min) : %d "), ana);

	if(socks) {
	    conf.getsocksuser(socksuser, sockspass);
	    i = t.addnode(_(" SOCKS proxy settings "));
	    t.addleaff(i, 0, 10, _(" Proxy server address : %s "), prserv.c_str());
	    t.addleaff(i, 0, 11, _(" Proxy user name : %s "), socksuser.c_str());
	    t.addleaff(i, 0, 12, _(" Proxy password : %s "), phidden.assign(sockspass.size(), '*').c_str());
	}

	void *p;
	finished = !db.open(n, b, &p);
	i = (int) p;

	if(!finished)
	switch(b) {
	    case 0:
		switch(i) {
		    case 1:
			s = s == rsdontchange ? rscard :
			    s == rscard ? rsspeaker :
			    s == rsspeaker ? rsdisable : rsdontchange;
			break;
		    case 2:
			c = c == rcdontchange ? rcdark :
			    c == rcdark ? rcblue : rcdontchange;
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
		    case 16: serveronly = !serveronly; break;
		    case 17: usegroups = !usegroups; break;
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
		conf.setserveronly(serveronly);
		conf.setrussian(rus);

		if(conf.getusegroups() != usegroups) {
		    conf.setusegroups(usegroups);
		    clist.rearrange();
		}

		conf.setsockshost(socks ? prserv : "");
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

    if(!cicq.idle(HIDL_SOCKEXIT))
    if(c = clist.get(contactroot))
    if(c->updated() >= 5) {
	face.gendetails(db.gettree());
	db.gettree()->redraw();
	face.status(_("Your ICQ details have been fetched"));
    }
}

int icqface::selectgroup(const string text) {
    return groupmanager(text, true);
}

void icqface::organizegroups() {
    groupmanager(_("Organize contact groups"), false);
}

int icqface::groupmanager(const string text, bool sel) {
    dialogbox db;
    static int n = 0;
    int r, ngrp, b, id;
    vector<icqgroup>::iterator i;
    string gname;

    textwindow w(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT,
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
