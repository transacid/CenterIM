/*
*
* centericq user interface class, dialogs related part
* $Id: icqdialogs.cc,v 1.8 2001/09/24 11:56:38 konst Exp $
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
#include "icqhook.h"
#include "icqhist.h"
#include "centericq.h"
#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqmlist.h"

bool icqface::regdialog(unsigned int &ruin, string &rpasswd) {
    bool finished, newuin, success, socks = false;
    int nmode, ndet, nopt, n, i, b, nproxy;
    string phidden, pcheck, socksuser, sockspass, psockscheck, tmp;
    string serv = conf.getservername() + ":" + i2str(conf.getserverport());
    string prserv = conf.getsockshost() + ":" + i2str(conf.getsocksport());
    dialogbox db;

    if(conf.getsockshost().empty()) prserv = "";

    finished = success = false;
    icq_Russian = 0;
    newuin = true;
    ruin = 0;
    rnick = rfname = rlname = remail = "";

    db.setwindow(new textwindow(0, 0, BIGDIALOG_WIDTH, BIGDIALOG_HEIGHT,
	conf.getcolor(cp_dialog_frame), TW_CENTERED,
	conf.getcolor(cp_dialog_highlight), _(" Register on ICQ network ")));
    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	_("Change"), _("Done"), 0));

    while(!finished) {
	db.gettree()->clear();
	nmode = db.gettree()->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Mode "));
	ndet  = db.gettree()->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Details "));
	nopt  = db.gettree()->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Options "));

	db.gettree()->addleaff(nmode, 0, (void *) 1, _(" Registration mode : %s "),
	newuin ? _("new uin") : _("existing user"));

	db.gettree()->addleaff(nopt, 0, (void *) 12, _(" Server address : %s "), serv.c_str());
	db.gettree()->addleaff(nopt, 0, (void *) 13, _(" Use SOCKS proxy : %s "), stryesno(socks));
	db.gettree()->addleaff(nopt, 0, (void *)  8, _(" Sound device : %s "), strregsound(conf.getregsound()));
	db.gettree()->addleaff(nopt, 0, (void *)  9, _(" Initial color scheme : %s "), strregcolor(conf.getregcolor()));
	db.gettree()->addleaff(nopt, 0, (void *) 11, _(" Russian translation win1251-koi8 needed : %s "), stryesno(icq_Russian == 1));

	if(newuin) {
	    db.gettree()->addleaff(ndet, 0, (void *)  2, _(" Password to set : %s "), phidden.assign(rpasswd.size(), '*').c_str());
	    db.gettree()->addleaff(ndet, 0, (void *)  3, _(" Check the password : %s "), phidden.assign(pcheck.size(), '*').c_str());
	    db.gettree()->addleaff(ndet, 0, (void *)  4, _(" Nickname : %s "), rnick.c_str());
	    db.gettree()->addleaff(ndet, 0, (void *)  5, _(" E-Mail : %s "), remail.c_str());
	    db.gettree()->addleaff(ndet, 0, (void *)  6, _(" First name : %s "), rfname.c_str());
	    db.gettree()->addleaff(ndet, 0, (void *)  7, _(" Last name : %s "), rlname.c_str());
	} else {
	    db.gettree()->addleaff(ndet, 0, (void *) 10, _(" UIN : %s "), strint(ruin));
	    db.gettree()->addleaff(ndet, 0, (void *)  2, _(" Password : %s "), phidden.assign(rpasswd.size(), '*').c_str());
	}

	db.gettree()->addleaff(ndet, 0, (void *) 20,
	    _(" Remember ICQ password : %s "),
	    stryesno(conf.getsavepwd()));

	if(socks) {
	    conf.getsocksuser(socksuser, sockspass);
	    nproxy = db.gettree()->addnode(0,conf.getcolor(cp_dialog_highlight),0, _(" SOCKS proxy settings "));
	    db.gettree()->addleaff(nproxy, 0, (void *) 14, _(" Proxy server address : %s "), prserv.c_str());
	    db.gettree()->addleaff(nproxy, 0, (void *) 17, _(" Proxy user name : %s "), socksuser.c_str());
	    db.gettree()->addleaff(nproxy, 0, (void *) 16, _(" Proxy password : %s "), phidden.assign(sockspass.size(), '*').c_str());
	    db.gettree()->addleaff(nproxy, 0, (void *) 18, _(" Check the password : %s "), phidden.assign(psockscheck.size(), '*').c_str());
	}

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

bool icqface::finddialog(searchparameters &s) {
    int n, b, i;
    bool finished = false, ret = false;
    dialogbox db;

    blockmainscreen();

    db.setwindow(new textwindow(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT,
	conf.getcolor(cp_dialog_frame), TW_CENTERED,
	conf.getcolor(cp_dialog_highlight), _(" Find user(s) ")));

    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	_("Clear"), _("Change"), _("Search"), 0));

    db.idle = &dialogidle;
    treeview &tree = *db.gettree();
    db.getbar()->item = 1;

    while(!finished) {
	tree.clear();

	int nuin = tree.addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" UIN "));
	int ninfo = tree.addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Details "));
	int nloc = tree.addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Location "));
	int nwork = tree.addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Work "));
	int nonl = tree.addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Online only "));

	tree.addleaff(nuin, 0, (void *) 10, _(" UIN : %s "), strint(s.uin));

	tree.addleaff(ninfo, 0, (void *) 11, _(" Nickname : %s "), s.nick.c_str());
	tree.addleaff(ninfo, 0, (void *) 12, _(" E-Mail : %s "), s.email.c_str());
	tree.addleaff(ninfo, 0, (void *) 13, _(" First name : %s "), s.firstname.c_str());
	tree.addleaff(ninfo, 0, (void *) 14, _(" Last name : %s "), s.lastname.c_str());
	tree.addleaff(ninfo, 0, (void *) 15, _(" Min. age : %s "), strint(s.minage));
	tree.addleaff(ninfo, 0, (void *) 16, _(" Max. age : %s "), strint(s.maxage));
	tree.addleaff(ninfo, 0, (void *) 17, _(" Gender : %s "), strgender(s.gender));
	tree.addleaff(ninfo, 0, (void *) 18, _(" Language : %s "), s.language ? icq_GetMetaLanguageName(s.language) : "");

	tree.addleaff(nloc, 0, (void *) 19, _(" City : %s "), s.city.c_str());
	tree.addleaff(nloc, 0, (void *) 20, _(" State : %s "), s.state.c_str());
	tree.addleaff(nloc, 0, (void *) 21, _(" Country : %s "), s.country ? icq_GetCountryName(s.country) : "");

	tree.addleaff(nwork, 0, (void *) 22, _(" Company : %s "), s.company.c_str());
	tree.addleaff(nwork, 0, (void *) 23, _(" Department : %s "), s.department.c_str());
	tree.addleaff(nwork, 0, (void *) 24, _(" Position : %s "), s.position.c_str());

	tree.addleaff(nonl, 0, (void *) 25, " %s ", stryesno(s.onlineonly));

	finished = !db.open(n, b, (void **) &i);

	if(!finished)
	switch(b) {
	    case 0:
		s = searchparameters();
		break;
	    case 1:
		switch(i) {
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
		break;
	}
    }

    db.close();
    unblockmainscreen();

    return ret;
}

void icqface::gendetails(treeview *tree, icqcontact *c = 0) {
    if(!c) c = clist.get(0);

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

    int saveitem, savefirst;
    
    tree->menu.getpos(saveitem, savefirst);
    tree->clear();

    int ngen = tree->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" General "));
    int nhome = tree->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Home "));
    int nwork = tree->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Work "));
    int nmore = tree->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" More "));
    int nabout = tree->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" About "));

    tree->addleaff(ngen, 0, 0, _(" Nickname : %s "), c->getnick().c_str());
    tree->addleaff(ngen, 0, 0, _(" First name : %s "), fname.c_str());
    tree->addleaff(ngen, 0, 0, _(" Last name : %s "), lname.c_str());
    tree->addleaff(ngen, 0, 0, _(" E-mail : %s "), fprimemail.c_str());
    tree->addleaff(ngen, 0, 0, _(" Second e-mail : %s "), fsecemail.c_str());
    tree->addleaff(ngen, 0, 0, _(" Old e-mail : %s "), foldemail.c_str());
    tree->addleaff(ngen, 0, 0, _(" Gender : %s "), strgender(fgender));
    tree->addleaff(ngen, 0, 0, _(" Birthdate : %s "), getbdate(fbday, fbmonth, fbyear).c_str());
    tree->addleaff(ngen, 0, 0, _(" Age : %s "), strint(fage));

    tree->addleaff(nhome, 0, 0, _(" City : %s "), fcity.c_str());
    tree->addleaff(nhome, 0, 0, _(" State : %s "), fstate.c_str());
    tree->addleaff(nhome, 0, 0, _(" Country : %s "), fcountry ? icq_GetCountryName(fcountry) : "");
    tree->addleaff(nhome, 0, 0, _(" Street address : %s "), fstreet.c_str());
    tree->addleaff(nhome, 0, 0, _(" Zip code : %s "), strint(fzip));
    tree->addleaff(nhome, 0, 0, _(" Phone : %s "), fphone.c_str());
    tree->addleaff(nhome, 0, 0, _(" Fax : %s "), ffax.c_str());
    tree->addleaff(nhome, 0, 0, _(" Cellular phone : %s "), fcellular.c_str());

    tree->addleaff(nwork, 0, 0, _(" City : %s "), fwcity.c_str());
    tree->addleaff(nwork, 0, 0, _(" State : %s "), fwstate.c_str());
    tree->addleaff(nwork, 0, 0, _(" Country : %s "), fwcountry ? icq_GetCountryName(fwcountry) : "");
    tree->addleaff(nwork, 0, 0, _(" Street address : %s "), fwaddress.c_str());
    tree->addleaff(nwork, 0, 0, _(" Company : %s "), fcompany.c_str());
    tree->addleaff(nwork, 0, 0, _(" Department : %s "), fdepartment.c_str());
    tree->addleaff(nwork, 0, 0, _(" Occupation : %s "), foccupation ? icq_GetMetaOccupationName(foccupation) : "");
    tree->addleaff(nwork, 0, 0, _(" Position : %s "), fjob.c_str());
    tree->addleaff(nwork, 0, 0, _(" Homepage : %s "), fwhomepage.c_str());
    tree->addleaff(nwork, 0, 0, _(" Phone : %s "), fwphone.c_str());
    tree->addleaff(nwork, 0, 0, _(" Fax : %s "), fwfax.c_str());

    tree->addleaff(nmore, 0, 0, _(" Homepage : %s "), fhomepage.c_str());
    tree->addleaff(nmore, 0, 0, _(" 1st language : %s "), flang1 ? icq_GetMetaLanguageName(flang1) : "");
    tree->addleaff(nmore, 0, 0, _(" 2nd language : %s "), flang2 ? icq_GetMetaLanguageName(flang2) : "");
    tree->addleaff(nmore, 0, 0, _(" 3rd language : %s "), flang3 ? icq_GetMetaLanguageName(flang3) : "");

    tree->addleaff(nabout, 0, 0, " %s ", fabout.c_str());

    tree->menu.setpos(saveitem, savefirst);
}

bool icqface::updatedetails(icqcontact *c = 0) {
    bool finished = false, ret = false;
    int n, b;
    dialogbox db;

    if(!c) c = clist.get(0);

    blockmainscreen();

    textwindow w(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT, conf.getcolor(cp_dialog_frame), TW_CENTERED);

    if(!c->getuin()) {
	w.set_title(conf.getcolor(cp_dialog_highlight), _(" Your ICQ details "));
    } else {
	w.set_titlef(conf.getcolor(cp_dialog_highlight), _(" %s's ICQ details "), c->getdispnick().c_str());
    }

    db.setwindow(&w, false);

    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_text)));
    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), _("Change"), _("Done"), 0));
    db.idle = &detailsidle;

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
	    if(!c->getuin() && (c->updated() < 5)) {
		status(_("Wait a moment. Your ICQ details haven't been fetched yet"));
	    } else
	    switch(n) {
		case  0: finished = true; break;
		case  2:
		    c->setnick(inputstr(_("Nickname: "), c->getnick()));
		    if(c->isnonicq()) c->setdispnick(c->getnick());
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
	    ret = c->getuin() || (c->updated() >= 5);
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

    m.additem(0, (void *) 0, (string) " " + _("Not entered"));

    for(i = 0;; i++) {
	if(icq_Countries[i].code == 0xffff) break; else {
	    m.additem(0, (void *) ((int) icq_Countries[i].code), (string) " " + icq_Countries[i].name);
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

    for(i = 0; i <= 2; i++) m.additemf(0, (void *) i, " %s", strgender(i));
    
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

bool icqface::sendfiles(unsigned int uin, string &msg, linkedlist &flist) {
    int n, i, b;
    bool finished = false;
    string fname;
    dialogbox db;

    flist.freeitem = &charpointerfree;
    flist.empty();

    blockmainscreen();

    textwindow w(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT, conf.getcolor(cp_dialog_frame), TW_CENTERED);
    w.set_titlef(conf.getcolor(cp_dialog_highlight), _(" Send file(s) to %s, %lu "), clist.get(uin)->getdispnick().c_str(), uin);

    db.setwindow(&w, false);
    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_text)));

    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected),
	_("Add"), _("Remove"), _("Comment"), _("Send"), 0));

    db.addkey(KEY_IC, 0);
    db.addkey(KEY_DC, 1);
    db.idle = &dialogidle;

    while(!finished) {
	db.gettree()->clear();
	
	int nfiles = db.gettree()->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Files "));
	int ncomment = db.gettree()->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Comment "));

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
	}
    }

    db.close();
    unblockmainscreen();
    return flist.count;
}

bool icqface::updateconf(regsound &s, regcolor &c) {
    bool finished, success;
    int nopt, n, i, b, nproxy, nconf, ncomm, aaway, ana;
    string tmp, phidden, socksuser, sockspass;
    string serv = conf.getservername() + ":" + i2str(conf.getserverport());
    string prserv = conf.getsockshost() + ":" + i2str(conf.getsocksport());

    bool quote = conf.getquote();
    bool rus = icq_Russian == 1;
    bool savepwd = conf.getsavepwd();
    bool socks = !conf.getsockshost().empty();
    bool hideoffl = conf.gethideoffline();
    bool antispam = conf.getantispam();
    bool mailcheck = conf.getmailcheck();
    bool serveronly = conf.getserveronly();

    dialogbox db;

    finished = success = false;
    if(conf.getsockshost().empty()) prserv = "";
    conf.getauto(aaway, ana);

    textwindow w(0, 0, BIGDIALOG_WIDTH, BIGDIALOG_HEIGHT, conf.getcolor(cp_dialog_frame), TW_CENTERED);
    w.set_title(conf.getcolor(cp_dialog_highlight), _(" CenterICQ quick config "));

    db.setwindow(&w, false);
    db.settree(new treeview(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_text)));
    db.setbar(new horizontalbar(conf.getcolor(cp_dialog_text),
	conf.getcolor(cp_dialog_selected), _("Change"), _("Done"), 0));
    db.idle = &dialogidle;

    blockmainscreen();

    while(!finished) {
	db.gettree()->clear();

	nconf = db.gettree()->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Configuration "));
	nopt = db.gettree()->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Options "));
	ncomm = db.gettree()->addnode(0, conf.getcolor(cp_dialog_highlight), 0, _(" Communications "));

	db.gettree()->addleaff(nconf, 0, (void *) 1, _(" Change sound device to : %s "), strregsound(s));
	db.gettree()->addleaff(nconf, 0, (void *) 2, _(" Change color scheme to : %s "), strregcolor(c));

	db.gettree()->addleaff(nopt, 0, (void *)  4, _(" Automatically set Away period (min) : %d "), aaway);
	db.gettree()->addleaff(nopt, 0, (void *)  5, _(" Automatically set N/A period (min) : %d "), ana);
	db.gettree()->addleaff(nopt, 0, (void *)  3, _(" Russian translation win1251-koi8 needed : %s "), stryesno(rus));
	db.gettree()->addleaff(nopt, 0, (void *)  6, _(" Hide offline users : %s "), stryesno(hideoffl));
	db.gettree()->addleaff(nopt, 0, (void *)  8, _(" Quote a message on reply : %s "), stryesno(quote));
	db.gettree()->addleaff(nopt, 0, (void *) 13, _(" Remember ICQ password : %s "), stryesno(savepwd));
	db.gettree()->addleaff(nopt, 0, (void *) 14, _(" Anti-spam: kill msgs from users not on the list : %s "), stryesno(antispam));
	db.gettree()->addleaff(nopt, 0, (void *) 15, _(" Check the local mailbox : %s "), stryesno(mailcheck));
	db.gettree()->addleaff(nopt, 0, (void *) 16, _(" Send all events through server : %s "), stryesno(serveronly));

	db.gettree()->addleaff(ncomm, 0, (void *) 7, _(" ICQ server address : %s "), serv.c_str());
	db.gettree()->addleaff(ncomm, 0, (void *) 9, _(" Use SOCKS proxy : %s "), stryesno(socks));

	if(socks) {
	    conf.getsocksuser(socksuser, sockspass);
	    nproxy = db.gettree()->addnode(0,conf.getcolor(cp_dialog_highlight),0, _(" SOCKS proxy settings "));
	    db.gettree()->addleaff(nproxy, 0, (void *) 10, _(" Proxy server address : %s "), prserv.c_str());
	    db.gettree()->addleaff(nproxy, 0, (void *) 11, _(" Proxy user name : %s "), socksuser.c_str());
	    db.gettree()->addleaff(nproxy, 0, (void *) 12, _(" Proxy password : %s "), phidden.assign(sockspass.size(), '*').c_str());
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
		    case 7:
			tmp = inputstr(_("ICQ server to use: "), serv);
			if(!tmp.empty()) serv = tmp;
			break;
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
		}
		break;
	    case 1:
		finished = success = true;
		conf.setserver(serv);
		conf.setquote(quote);
		conf.setsavepwd(savepwd);
		conf.setauto(aaway, ana);
		conf.sethideoffline(hideoffl);
		conf.setantispam(antispam);
		conf.setmailcheck(mailcheck);
		conf.setserveronly(serveronly);
		icq_Russian = rus ? 1 : 0;

		if(socks) conf.setsockshost(prserv);
		else conf.setsockshost("");
		
		break;
	}
    }

    db.close();
    unblockmainscreen();

    return success;
}

int icqface::editaboutkeys(texteditor *e, int k) {
    switch(k) {
	case CTRL('x'): face.editdone = true; return -1;
	case 27: return -1;
    }
    return 0;
}

void icqface::detailsidle(dialogbox *db) {
    icqcontact *c;

    if(!ihook.idle(HIDL_SOCKEXIT))
    if(c = clist.get(0))
    if(c->updated() >= 5) {
	face.gendetails(db->gettree());
	db->gettree()->redraw();
	face.status(_("Your ICQ details have been fetched"));
    }
}
