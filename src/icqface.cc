/*
*
* centericq user interface class
* $Id: icqface.cc,v 1.53 2001/12/07 11:06:14 konst Exp $
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

#include "icqhook.h"
#include "yahoohook.h"

const char *strregsound(regsound s) {
    return s == rscard ? _("sound card") :
	s == rsspeaker ? _("speaker") :
	s == rsdisable ? _("disable") :
	_("don't change");
}

const char *strregcolor(regcolor c) {
    return c == rcdark ? _("dark") :
	c == rcblue ? _("blue") :
	_("don't change");
}

const char *strint(unsigned int i) {
    static string s;
    return i ? (s = i2str(i)).c_str() : "";
}

icqface::icqface() {
    kinterface();
    raw();
    workareas.freeitem = &freeworkareabuf;
    kt_resize_event = &termresize;

#ifdef DEBUG
    time_t logtime = time(0);
    flog.clear();
    flog.open((conf.getdirname() + "log").c_str(), ios::app);
    if(flog.is_open()) flog << endl << "-- centericq log started on " << ctime(&logtime);
#endif

    mainscreenblock = inited = onlinefolder = dotermresize = false;
}

icqface::~icqface() {
    kendinterface();
    if(inited) {
	for(int i = 0; i < LINES; i++) printf("\n");
    }

#ifdef DEBUG
    if(flog.is_open()) flog.close();
#endif
}

void icqface::init() {
    mcontacts = new treeview(1, 2, 25, LINES-2,
	conf.getcolor(cp_main_menu), conf.getcolor(cp_main_selected),
	conf.getcolor(cp_main_menu), conf.getcolor(cp_main_menu));

    mcontacts->menu.idle = &menuidle;
    mcontacts->menu.otherkeys = &contactskeys;

    input.setcolor(conf.getcolor(cp_status));
    input.idle = &textinputidle;

    mainw = textwindow(0, 1, COLS-1, LINES-2, conf.getcolor(cp_main_frame));

    selector.setcolor(conf.getcolor(cp_dialog_menu),
	conf.getcolor(cp_dialog_highlight),
	conf.getcolor(cp_dialog_selected),
	conf.getcolor(cp_dialog_frame));

    mhist = verticalmenu(conf.getcolor(cp_main_text),
	conf.getcolor(cp_main_selected));

    attrset(conf.getcolor(cp_status));
    mvhline(0, 0, ' ', COLS);
    mvhline(LINES-1, 0, ' ', COLS);

    inited = true;
}

void icqface::done() {
    delete mcontacts;
}

void icqface::draw() {
    mainw.open();
    mainw.separatex(25);
    workarealine(WORKAREA_Y2);
    mvhline(WORKAREA_Y2, WORKAREA_X1, LTEE, 1);
    mvhline(WORKAREA_Y2, WORKAREA_X2, RTEE, 1);

    update();
}

void icqface::showtopbar() {
    string buf;
    protocolname pname;
    icqconf::imaccount ia;

    attrset(conf.getcolor(cp_status));
    mvhline(0, 0, ' ', COLS);

    kwriteatf(2, 0, conf.getcolor(cp_status),
	"CENTERICQ %s   UNSENT: %lu", VERSION, em.getunsentcount());

    for(pname = icq; pname != protocolname_size; (int) pname += 1) {
	if(pname != infocard) {
	    ia = conf.getourid(pname);

	    if(!ia.empty()) {
		buf += "  " + conf.getprotocolname(pname) + ":[";

		buf += imstatus2char[gethook(pname).getstatus()];
		buf += "]";
	    }
	}
    }

    kwriteatf(COLS-buf.size()-2, 0, conf.getcolor(cp_status), buf.c_str());
}

void icqface::update() {
    showtopbar();
    fillcontactlist();
    fneedupdate = false;
}

int icqface::contextmenu(icqcontact *c) {
    int ret = 0, i;
    static int elem = 0;

    verticalmenu m(conf.getcolor(cp_main_text),
	conf.getcolor(cp_main_selected));

    m.setwindow(textwindow(WORKAREA_X1, WORKAREA_Y1, WORKAREA_X1+27,
	WORKAREA_Y1+6, conf.getcolor(cp_main_text)));

    if(!c) return 0;

    if(c->getdesc() == contactroot) {
	m.additem(0, ACT_HISTORY, _(" Events history         h"));
    }

    if(m.empty())
    if(c->inlist())
    switch(c->getdesc().pname) {
	case icq:
	    m.additem(0, ACT_MSG,      _(" Send a message     enter"));
	    m.additem(0, ACT_URL,      _(" Send an URL            u"));
/*
	    m.additem(0, ACT_CONTACT,  _(" Send contacts          c"));
	    m.additem(0, ACT_FILE,     _(" Send a file            f"));
*/
	    m.addline();
	    m.additem(0, ACT_INFO,     _(" User's details         ?"));
	    m.additem(0, ACT_HISTORY,  _(" Events history         h"));
	    m.addline();
	    m.additem(0, ACT_IGNORE,   _(" Ignore user"));
	    break;

	case yahoo:
	case msn:
	    m.additem(0, ACT_MSG,      _(" Send a message     enter"));
	    m.addline();
	    m.additem(0, ACT_INFO,     _(" User's details         ?"));
	    m.additem(0, ACT_HISTORY,  _(" Events history         h"));
	    m.addline();
	    m.additem(0, ACT_IGNORE,   _(" Ignore user"));
	    break;

	case infocard:
	    m.additem(0, ACT_INFO,      _(" User's details         ?"));
	    m.additem(0, ACT_EDITUSER,  _(" Edit details"));
	    break;
    }

    if(m.empty()) {
	if(!c->inlist()) {
	    m.additem(0, ACT_MSG,       _(" Send a message     enter"));
	    m.addline();
	    m.additem(0, ACT_ADD,       _(" Add to list            a"));
	    m.addline();
	    m.additem(0, ACT_INFO,      _(" User's details         ?"));
	    m.additem(0, ACT_HISTORY,   _(" Events history         h"));
	    m.additem(0, ACT_IGNORE,    _(" Ignore user"));
	}
    }

    if(c->getdesc() != contactroot) {
	m.additem(0, ACT_REMOVE, _(" Remove user          del"));
	m.additem(0, ACT_RENAME, _(" Rename contact         r"));
	if(conf.getusegroups()) {
	    m.additem(0, ACT_GROUPMOVE, _(" Move to group.."));
	}
    }

    m.scale();
    m.idle = &menuidle;
    m.setpos(elem);
    i = m.open();
    m.close();
    elem = i-1;

    return (int) m.getref(elem);
}

int icqface::generalmenu() {
    int i, r = 0;
    static int lastitem = 0;

    verticalmenu m(conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));
    m.setwindow(textwindow(WORKAREA_X1, WORKAREA_Y1, WORKAREA_X1+40, WORKAREA_Y1+11, conf.getcolor(cp_main_text)));

    m.idle = &menuidle;
    m.additem(0, ACT_STATUS,    _(" Change status                       s"));
    m.additem(0, ACT_QUICKFIND, _(" Go to contact..                 alt-s"));
    m.additem(0, ACT_DETAILS,   _(" Accounts.."));
    m.additem(0, ACT_FIND,      _(" Find/add users"));
    m.additem(0, ACT_CONF,      _(" CenterICQ config options"));
    m.addline();
    m.additem(0, ACT_IGNORELIST,    _(" View/edit ignore list"));
    m.additem(0, ACT_INVISLIST,     _(" View/edit invisible list"));
    m.additem(0, ACT_VISIBLELIST,   _(" View/edit visible list"));
    m.addline();
    m.additem(0, ACT_HIDEOFFLINE, conf.gethideoffline() ?
	_(" Show offline users                 F5") :
	_(" Hide offline users                 F5"));

    if(conf.getusegroups()) {
	m.additem(0,  ACT_ORG_GROUPS, (string) " " + _("Organize contact groups"));
    }

    m.setpos(lastitem);
    m.scale();

    i = m.open();
    m.close();

    if(i) {
	lastitem = i-1;
	r = (int) m.getref(lastitem);
    }

    return r;
}

icqcontact *icqface::mainloop(int &action) {
    int i, curid;
    icqcontact *c = 0;
    bool fin;

    for(fin = false; !fin; ) {
	extk = ACT_MSG;
	c = (icqcontact *) mcontacts->open(&i);
	if((int) c < 100) c = 0;

	if(i) {
	    switch(action = extk) {
		case ACT_GMENU:
		    if(!(action = generalmenu())) {
			continue;
		    } else {
			fin = true;
		    }
		    break;
	    }                

	    if(c) {
		if(action == ACT_MENU)
		if(!(action = contextmenu(c))) continue;
	    } else {
		switch(action) {
		    case ACT_MSG:
			curid = mcontacts->getid(mcontacts->menu.getpos());

			if(mcontacts->isnodeopen(curid)) {
			    mcontacts->closenode(curid);
			} else {
			    mcontacts->opennode(curid);
			}
			break;
		    case ACT_QUIT:
		    case ACT_STATUS:
		    case ACT_FIND:
		    case ACT_QUICKFIND:
		    case ACT_HIDEOFFLINE:
			break;
		    default:
			continue;
		}
	    }

	    break;
	}
    }

    return c;
}

void icqface::fillcontactlist() {
    int i, id, nnode, ngroup, prevgid;
    string dnick;
    icqcontact *c;
    void *savec;
    char prevsc = 'x', sc;
    bool online_added = false, groupchange;

    nnode = ngroup = prevgid = 0;

    savec = mcontacts->getref(mcontacts->getid(mcontacts->menu.getpos()));
    mcontacts->clear();
    clist.order();

    if(c = clist.get(contactroot))
    if(c->getmsgcount())
	mcontacts->addleaf(0, conf.getcolor(cp_main_highlight), c, "#ICQ ");

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(c->getdesc() == contactroot)
	    continue;

	if(c->getstatus() == offline)
	if(conf.gethideoffline())
	if(!c->getmsgcount()) {
	    continue;
	}

	sc = SORTCHAR(c);

	groupchange =
	    conf.getusegroups() &&
	    (c->getgroupid() != prevgid) &&
	    (sc != '#');

	if(groupchange && !strchr("!", sc)) {
	    ngroup = mcontacts->addnode(0, conf.getcolor(cp_main_highlight),
		0, " " + find(groups.begin(), groups.end(),
		c->getgroupid())->getname() + " ");
	}

	if(groupchange || (sc != '#')) {
	    if(strchr("candifo", sc)) sc = 'O';

	    if((sc != prevsc) || groupchange)
	    switch(sc) {
		case 'O':
		    nnode = conf.gethideoffline() && conf.getusegroups() ?
			ngroup : mcontacts->addnode(ngroup,
			    conf.getcolor(cp_main_highlight), 0, " Online ");

		    online_added = true;
		    break;
		case '_':
		    nnode = mcontacts->addnode(ngroup,
			conf.getcolor(cp_main_highlight), 0, " Offline ");
		    break;
		case '!':
		    ngroup = 0;
		    nnode = mcontacts->addnode(ngroup,
			conf.getcolor(cp_main_highlight), 0, " Not in list ");
		    break;
	    }

	    if(groupchange) prevgid = c->getgroupid();
	    prevsc = sc;
	}

	dnick = c->getdispnick();
	if(c->isbirthday()) dnick += " :)";

	if(c->getstatus() == offline) {
	    mcontacts->addleaff(nnode,
		c->getmsgcount() ? conf.getcolor(cp_main_highlight) : conf.getprotcolor(c->getdesc().pname),
		c, "%s%s ", c->getmsgcount() ? "#" : " ", dnick.c_str());
	} else {
	    mcontacts->addleaff(nnode,
		c->getmsgcount() ? conf.getcolor(cp_main_highlight) : conf.getprotcolor(c->getdesc().pname),
		c, "%s[%c] %s ", c->getmsgcount() ? "#" : " ", c->getshortstatus(), dnick.c_str());
	}
    }

    if(c = clist.get(contactroot))
    if(!c->getmsgcount())
	mcontacts->addleaf(0, conf.getcolor(cp_clist_root), c, " ICQ ");

    if(!mainscreenblock) mcontacts->redraw();

    if(!savec || (!onlinefolder && online_added && conf.gethideoffline())) {
	mcontacts->menu.setpos(0);
    } else
    for(i = 0; i < mcontacts->menu.getcount(); i++) {
	id = mcontacts->getid(i);
	if(mcontacts->getref(id) == savec) {
	    mcontacts->menu.setpos(i);
	    break;
	}
    }

    onlinefolder = online_added;

    if(!mainscreenblock) {
	mcontacts->menu.redraw();
    }
}

bool icqface::findresults() {
    bool finished = false, ret = false;
    unsigned int ffuin;
    icqcontact *c;
    dialogbox db;
    int r, b;

    saveworkarea();
    clearworkarea();

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+2, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));
    db.setmenu(new verticalmenu(conf.getcolor(cp_main_menu),
	conf.getcolor(cp_main_selected)));
    db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	conf.getcolor(cp_main_selected), _("Details"), _("Add"),
	_("New search"), 0));
/*
    ihook.clearfindresults();
    ihook.setfinddest(db.getmenu());
*/
    db.addautokeys();
    db.idle = &dialogidle;
    db.redraw();

    mainw.write(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight), "Find results");

    workarealine(WORKAREA_Y1+2);
    workarealine(WORKAREA_Y2-2);

    while(!finished) {
	finished = !db.open(r, b);
//        ihook.setfinddest(0);
	
	if(!finished)
	switch(b) {
	    case 0:
/*
		if(ffuin = ihook.getfinduin(r)) {
		    cicq.userinfo(imcontact(ffuin, icq));
		}
*/
		break;
	    case 1:
/*
		if(ffuin = ihook.getfinduin(r)) {
		    cicq.addcontact(imcontact(ffuin, icq));
		}
*/
		break;
	    case 2:
		ret = finished = true;
		break;
	}
	
    }

    db.close();
    restoreworkarea();

    return ret;
}

void icqface::infoclear(dialogbox &db, icqcontact *c, const imcontact realdesc) {
    for(int i = WORKAREA_Y1+1; i < WORKAREA_Y2; i++) {
	workarealine(i, ' ');
    }

    db.redraw();

    mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	_("Information about %s"), realdesc.totext().c_str());

    workarealine(WORKAREA_Y1+2);
    workarealine(WORKAREA_Y2-2);

    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();
    icqcontact::workinfo wi = c->getworkinfo();

    extracturls(bi.city + " " + bi.state + " " + bi.street + " " +
	wi.city + " " + wi.state + " " + wi.street + " " +
	wi.company + " " + wi.dept + " " + wi.position + " " +
	c->getabout());

    if(!mi.homepage.empty())
	extractedurls.push_back(mi.homepage);
    if(!wi.homepage.empty())
	extractedurls.push_back(wi.homepage);
}

void icqface::infogeneral(dialogbox &db, icqcontact *c) {
    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();

    workarealine(WORKAREA_Y1+8);
    workarealine(WORKAREA_Y1+12);

    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+2, conf.getcolor(cp_main_highlight), _("Nickname"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+3, conf.getcolor(cp_main_highlight), _("Name"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+4, conf.getcolor(cp_main_highlight), _("E-mail"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+5, conf.getcolor(cp_main_highlight), _("2nd e-mail"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+6, conf.getcolor(cp_main_highlight), _("Old e-mail"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+8, conf.getcolor(cp_main_highlight), _("Gender"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+9, conf.getcolor(cp_main_highlight), _("Birthdate"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+10, conf.getcolor(cp_main_highlight), _("Age"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+12, conf.getcolor(cp_main_highlight), _("Languages"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+13, conf.getcolor(cp_main_highlight), _("Last IP"));

    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+2, conf.getcolor(cp_main_text), c->getnick());
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+3, conf.getcolor(cp_main_text), bi.fname + " " + bi.lname);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+4, conf.getcolor(cp_main_text), bi.email);

    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+8, conf.getcolor(cp_main_text), strgender[mi.gender]);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+9, conf.getcolor(cp_main_text), mi.strbirthdate());
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+10, conf.getcolor(cp_main_text), mi.age ? i2str(mi.age) : "");

    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+13, conf.getcolor(cp_main_text), c->getlastip());

    if(c->getstatus() == offline) {
	time_t ls = c->getlastseen();

	mainw.write(WORKAREA_X1+2, WORKAREA_Y1+14, conf.getcolor(cp_main_highlight), _("Last seen"));
	mainw.write(WORKAREA_X1+14, WORKAREA_Y1+14, conf.getcolor(cp_main_text),
	    ls ? strdateandtime(ls) : _("Never"));
    }
}

void icqface::infohome(dialogbox &db, icqcontact *c) {
    int i, x;

    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();

    workarealine(WORKAREA_Y1+10);
    x = WORKAREA_X1+2;

    mainw.write(x, WORKAREA_Y1+2, conf.getcolor(cp_main_highlight), _("Address"));
    mainw.write(x, WORKAREA_Y1+3, conf.getcolor(cp_main_highlight), _("Location"));
    mainw.write(x, WORKAREA_Y1+4, conf.getcolor(cp_main_highlight), _("Zip code"));
    mainw.write(x, WORKAREA_Y1+5, conf.getcolor(cp_main_highlight), _("Phone"));
    mainw.write(x, WORKAREA_Y1+6, conf.getcolor(cp_main_highlight), _("Fax"));
    mainw.write(x, WORKAREA_Y1+7, conf.getcolor(cp_main_highlight), _("Cellular"));
    mainw.write(x, WORKAREA_Y1+8, conf.getcolor(cp_main_highlight), _("Timezone"));

    mainw.write(x, WORKAREA_Y1+10, conf.getcolor(cp_main_highlight), _("Homepage"));

    if(!bi.state.empty()) {
	if(bi.city.size()) bi.city += ", ";
	bi.city += bi.state;
    }

    if(!bi.country.empty()) {
	if(bi.city.size()) bi.city += ", ";
	bi.city += bi.country;
    }

    x += 10;
    mainw.write(x, WORKAREA_Y1+2, conf.getcolor(cp_main_text), bi.street);
    mainw.write(x, WORKAREA_Y1+3, conf.getcolor(cp_main_text), bi.city);
    mainw.write(x, WORKAREA_Y1+4, conf.getcolor(cp_main_text), bi.zip ? i2str(bi.zip) : "");
    mainw.write(x, WORKAREA_Y1+5, conf.getcolor(cp_main_text), bi.phone);
    mainw.write(x, WORKAREA_Y1+6, conf.getcolor(cp_main_text), bi.fax);
    mainw.write(x, WORKAREA_Y1+7, conf.getcolor(cp_main_text), bi.cellular);

    for(i = 0; !mi.homepage.empty(); i++) {
	mainw.write(x, WORKAREA_Y1+10+i, conf.getcolor(cp_main_text),
	    mi.homepage.substr(0, WORKAREA_X2-WORKAREA_X1-12));
	mi.homepage.erase(0, WORKAREA_X2-WORKAREA_X1-12);
    }
}

void icqface::infowork(dialogbox &db, icqcontact *c) {
    int i;
    icqcontact::workinfo wi = c->getworkinfo();

    workarealine(WORKAREA_Y1+6);
    workarealine(WORKAREA_Y1+11);

    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+2, conf.getcolor(cp_main_highlight), _("Address"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+3, conf.getcolor(cp_main_highlight), _("Location"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+4, conf.getcolor(cp_main_highlight), _("Zip code"));

    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+6, conf.getcolor(cp_main_highlight), _("Company"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+7, conf.getcolor(cp_main_highlight), _("Department"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+8, conf.getcolor(cp_main_highlight), _("Occupation"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+9, conf.getcolor(cp_main_highlight), _("Title"));

    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+11, conf.getcolor(cp_main_highlight), _("Phone"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+12, conf.getcolor(cp_main_highlight), _("Fax"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+13, conf.getcolor(cp_main_highlight), _("Homepage"));

    if(wi.city.size()) {
	if(wi.state.size()) wi.city += ", ";
	wi.city += wi.state;
    }

    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+2, conf.getcolor(cp_main_text), wi.street);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+3, conf.getcolor(cp_main_text), wi.city);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+4, conf.getcolor(cp_main_text), wi.zip ? i2str(wi.zip) : "");
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+6, conf.getcolor(cp_main_text), wi.company);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+7, conf.getcolor(cp_main_text), wi.dept);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+9, conf.getcolor(cp_main_text), wi.position);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+11, conf.getcolor(cp_main_text), wi.phone);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+12, conf.getcolor(cp_main_text), wi.fax);

    for(i = 0; !wi.homepage.empty(); i++) {
	mainw.write(WORKAREA_X1+14, WORKAREA_Y1+13+i, conf.getcolor(cp_main_text),
	    wi.homepage.substr(0, WORKAREA_X2-WORKAREA_X1-14));
	wi.homepage.erase(0, WORKAREA_X2-WORKAREA_X1-14);
    }
}

void icqface::infoabout(dialogbox &db, icqcontact *c) {
    db.getbrowser()->setbuf(c->getabout());
}

void commacat(string &text, string sub, bool nocomma = false) {
    if(!sub.empty()) {
	if(!nocomma)
	if(text[text.size()-1] != ',') text += ",";

	text += sub;
    }
}

void icqface::infointerests(dialogbox &db, icqcontact *c) {
    string text;
    db.getbrowser()->setbuf(text);
}

void icqface::userinfo(const imcontact cinfo, const imcontact realinfo) {
    bool finished = false, showinfo;
    icqcontact *c = clist.get(realinfo);
    textbrowser tb(conf.getcolor(cp_main_text));
    dialogbox db;
    int k, lastb, b;

    b = lastb = 0;
    saveworkarea();
    clearworkarea();

    status(_("F2 to URLs, ESC close"));

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+2, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));

    db.setbar(new horizontalbar(WORKAREA_X1+2, WORKAREA_Y2-1,
	conf.getcolor(cp_main_highlight), conf.getcolor(cp_main_selected),
	_("Info"), _("Home"), _("Work"), _("More"), _("About"),
	cinfo.pname != infocard ? _("Retrieve") : _("Edit"), 0));

    showinfo = true;
    db.addautokeys();
    db.idle = &dialogidle;
    db.otherkeys = &userinfokeys;

    while(!finished) {
	if(!showinfo) {
	    if(cicq.idle(HIDL_SOCKEXIT)) {
		finished = !db.open(k, b);
		showinfo = !finished;
	    }
	}

	if(c->updated()) {
	    showinfo = true;
	    c->unsetupdated();
	}

	if(showinfo) {
	    db.setbrowser(0);
	    infoclear(db, c, cinfo);

	    switch(b) {
		case 0: infogeneral(db, c); break;
		case 1: infohome(db, c); break;
		case 2: infowork(db, c); break;
		case 3:
		    db.setbrowser(&tb, false);
		    infointerests(db, c);
		    infoclear(db, c, cinfo);
		    break;
		case 4:
		    db.setbrowser(&tb, false);
		    infoabout(db, c);
		    infoclear(db, c, cinfo);
		    break;
		case 5:
		    switch(cinfo.pname) {
			case infocard:
			    updatedetails(c);
			    break;

			default:
			    gethook(cinfo.pname).requestinfo(cinfo);
			    break;
		    }

		    b = lastb;
		    showinfo = true;
		    continue;
	    }

	    refresh();
	    showinfo = false;
	    lastb = b;
	}
    }

    db.close();
    restoreworkarea();
    status("");
}

void icqface::makeprotocolmenu(verticalmenu &m) {
    icqconf::imaccount ia;
    protocolname ipname;

    static const string pitems[protocolname_size] = {
	_(" [icq] ICQ network"),
	_(" [yahoo] YAIM"),
	_(" [msn] M$ Messenger"),
	""
    };

    for(ipname = icq; ipname != protocolname_size; (int) ipname += 1) {
	ia = conf.getourid(ipname);

	if(!ia.empty()) {
	    m.additem(0, ipname, pitems[ipname]);
	}
    }
}

bool icqface::changestatus(protocolname &pname, imstatus &st) {
    int i;
    bool r;
    verticalmenu m(conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));

    vector<imstatus> mst;
    vector<imstatus>::iterator im;

    m.setwindow(textwindow(WORKAREA_X1, WORKAREA_Y1, WORKAREA_X1+27,
	WORKAREA_Y1+9, conf.getcolor(cp_main_text)));

    m.idle = &menuidle;
    makeprotocolmenu(m);

    m.scale();

    if(m.getcount() < 2) {
	i = 1;
    } else {
	i = m.open();
	m.close();
    }

    if(r = i) {
	pname = (protocolname) ((int) m.getref(i-1));
	m.clear();

	mst.push_back(available);
	mst.push_back(offline);
	mst.push_back(away);
	mst.push_back(dontdisturb);
	mst.push_back(notavail);
	mst.push_back(occupied);
	mst.push_back(freeforchat);
	mst.push_back(invisible);

	static string statustext[8] = {
	    _(" [o] Online"),
	    _(" [_] Offline"),
	    _(" [a] Away"),
	    _(" [d] Do not disturb"),
	    _(" [n] Not available"),
	    _(" [c] Occupied"),
	    _(" [f] Free for chat"),
	    _(" [i] Invisible")
	};

	for(im = mst.begin(); im != mst.end(); im++) {
	    m.additem(0, *im, statustext[im-mst.begin()]);
	    if(gethook(pname).getstatus() == *im) {
		m.setpos(m.getcount()-1);
	    }
	}

	m.scale();

	i = m.open();
	m.close();

	if(r = i) {
	    st = (imstatus) ((int) m.getref(i-1));
	}
    }

    return r;
}

#define INPUT_POS       LINES-2

const string icqface::inputstr(const string q, string defl = "", char passwdchar = 0) {
    screenarea sa(0, INPUT_POS, COLS, INPUT_POS);

    attrset(conf.getcolor(cp_status));
    mvhline(INPUT_POS, 0, ' ', COLS);
    kwriteatf(0, INPUT_POS, conf.getcolor(cp_status), "%s", q.c_str());

    input.setpasswordchar(passwdchar);
    input.removeselector();
    input.setvalue(defl);
    input.setcoords(q.size(), INPUT_POS, COLS-q.size());
    input.exec();

    sa.restore();
    return input.getvalue();
}

const string icqface::inputfile(const string q, const string defl = "") {
    screenarea sa(0, INPUT_POS, COLS, INPUT_POS);
    string r;

    mvhline(INPUT_POS, 0, ' ', COLS);
    kwriteatf(0, INPUT_POS, conf.getcolor(cp_status), "%s", q.c_str());
    kwriteatf(COLS-8, INPUT_POS, conf.getcolor(cp_status), "[Ctrl-T]");

    selector.setwindow(textwindow(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT,
	conf.getcolor(cp_dialog_frame), TW_CENTERED,
	conf.getcolor(cp_dialog_highlight),
	_(" enter to select a file, esc to cancel ")));

    input.connectselector(selector);

    input.setcoords(q.size(), INPUT_POS, COLS-q.size()-8);
    input.setvalue(defl);
    input.exec();
    r = input.getvalue();

    if(r.rfind("/") != -1) {
	selector.setstartpoint(r.substr(0, r.find("/")));
    }

    sa.restore();
    return r;
}

void catqstr(string &q, int option, int defl) {
    char c = q[q.size()-1];
    if(!strchr("(/", c)) q += '/';

    switch(option) {
	case ASK_YES: q += 'y'; break;
	case ASK_NO: q += 'n'; break;
	case ASK_CANCEL: q += 'c'; break;
    }

    if(defl == option) q[q.size()-1] = toupper(q[q.size()-1]);
}

int icqface::ask(string q, int options, int deflt = -1) {
    int ret;
    screenarea sa(0, INPUT_POS, COLS, INPUT_POS);

    q += " (";
    if(options & ASK_YES) catqstr(q, ASK_YES, deflt);
    if(options & ASK_NO) catqstr(q, ASK_NO, deflt);
    if(options & ASK_CANCEL) catqstr(q, ASK_CANCEL, deflt);
    q += ") ";

    attrset(conf.getcolor(cp_status));
    mvhline(INPUT_POS, 0, ' ', COLS);
    kwriteatf(0, INPUT_POS, conf.getcolor(cp_status), "%s", q.c_str());

    for(ret = -1; ret == -1; ) {
	switch(getkey()) {
	    case 'y':
	    case 'Y':
		if(options & ASK_YES) ret = ASK_YES;
		break;
	    case 'n':
	    case 'N':
		if(options & ASK_NO) ret = ASK_NO;
		break;
	    case 'c':
	    case 'C':
		if(options & ASK_CANCEL) ret = ASK_CANCEL;
		break;
	    case '\r':
		if(deflt != -1) ret = deflt;
		break;
	}
    }

    sa.restore();
    return ret;
}

void icqface::saveworkarea() {
    int i;
    chtype **workareabuf = (chtype **) malloc(sizeof(chtype *) * (WORKAREA_Y2-WORKAREA_Y1+1));

    for(i = 0; i <= WORKAREA_Y2-WORKAREA_Y1; i++) {
	workareabuf[i] = (chtype *) malloc(sizeof(chtype) * (WORKAREA_X2-WORKAREA_X1+2));
	mvinchnstr(WORKAREA_Y1+i, WORKAREA_X1, workareabuf[i], WORKAREA_X2-WORKAREA_X1+1);
    }

    workareas.add(workareabuf);
}

void icqface::restoreworkarea() {
    int i;
    chtype **workareabuf = (chtype **) workareas.at(workareas.count-1);

    if(workareabuf && !dotermresize) {
	for(i = 0; i <= WORKAREA_Y2-WORKAREA_Y1; i++) {
	    mvaddchnstr(WORKAREA_Y1+i, WORKAREA_X1,
		workareabuf[i], WORKAREA_X2-WORKAREA_X1+1);
	}

	refresh();
    }

    workareas.remove(workareas.count-1);
}

void icqface::clearworkarea() {
    int i;

    attrset(conf.getcolor(cp_main_text));
    for(i = WORKAREA_Y1+1; i < WORKAREA_Y2; i++) {
	mvhline(i, WORKAREA_X1+1, ' ', WORKAREA_X2-WORKAREA_X1-1);
	refresh();
    }
}

void icqface::freeworkareabuf(void *p) {
    chtype **workareabuf = (chtype **) p;
    if(workareabuf) {
	for(int i = 0; i <= WORKAREA_Y2-WORKAREA_Y1; i++) free((chtype *) workareabuf[i]);
	free(workareabuf);
    }
}

void icqface::workarealine(int l, chtype c = HLINE) {
    attrset(conf.getcolor(cp_main_frame));
    mvhline(l, WORKAREA_X1+1, c, WORKAREA_X2-WORKAREA_X1-1);
}

void icqface::modelist(contactstatus cs) {
    int i, b;
    icqcontact *c;
    dialogbox db;
    modelistitem it;
    vector<imcontact>::iterator ic;

    saveworkarea();
    clearworkarea();

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+2, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));

    db.setmenu(new verticalmenu(conf.getcolor(cp_main_text),
	conf.getcolor(cp_main_selected)));

    db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	conf.getcolor(cp_main_selected),
	_("Details"), _("Add"), _("Remove"), _("Move to contacts"), 0));

    db.addautokeys();
    db.idle = &dialogidle;
    db.addkey(KEY_IC, 1);
    db.addkey(KEY_DC, 2);

    db.redraw();
    workarealine(WORKAREA_Y1+2);
    workarealine(WORKAREA_Y2-2);

    mainw.write(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	cs == csignore ?
	    _("Ignore list") :
	cs == csvisible ?
	    _("Visible list") :
	    _("Invisible list"));

    lst.fillmenu(db.getmenu(), cs);

    while(db.open(i, b)) {
	if(!lst.size() && b != 1) continue;
	if(b != 1) it = lst.menuat(i-1);

	switch(b) {
	    case 0:
		cicq.userinfo(it.getdesc());
		break;
	    case 1:
		muins.clear();

		if(multicontacts(_("Select contacts to add to the list"))) {
		    for(ic = muins.begin(); ic != muins.end(); ic++) {
			lst.push_back(modelistitem(clist.get(*ic)->getdispnick(), *ic, cs));
			if(cs == csignore) clist.remove(*ic);
		    }

		    lst.fillmenu(db.getmenu(), cs);
		    db.getmenu()->redraw();
		    face.update();
		}
		break;
	    case 2:
		lst.del(it.getdesc(), cs);
		lst.fillmenu(db.getmenu(), cs);
		db.getmenu()->redraw();
		break;
	    case 3:
		if(c = cicq.addcontact(it.getdesc())) {
		    c->setdispnick(it.getnick());
		    lst.del(it.getdesc(), cs);
		    lst.fillmenu(db.getmenu(), cs);
		    db.getmenu()->redraw();
		    face.update();
		}
		break;
	}
    }

    db.close();
    restoreworkarea();
}

bool icqface::multicontacts(const string ahead = "") {
    int i, savefirst, saveelem;
    bool ret = true, finished = false;
    string head = ahead;

    imcontact *ic;
    vector<imcontact>::iterator c;
    vector<imcontact> mlst;

    verticalmenu m(WORKAREA_X1+1, WORKAREA_Y1+3, WORKAREA_X2, WORKAREA_Y2,
	conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));

    saveworkarea();
    clearworkarea();

    workarealine(WORKAREA_Y1+2);

    if(!head.size()) head = _("Event recipients");
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight), head);

    for(i = 0; i < clist.count; i++) {
	icqcontact *c = (icqcontact *) clist.at(i);
	if(!c->getdesc().empty()) {
	    mlst.push_back(c->getdesc());
	}
    }

    m.idle = &menuidle;
    m.otherkeys = &multiplekeys;

    while(!finished) {
	m.getpos(saveelem, savefirst);
	m.clear();

	for(c = mlst.begin(); c != mlst.end(); c++) {
	    icqcontact *cont = (icqcontact *) clist.get(*c);

	    m.additemf(
		conf.getprotcolor(c->pname),
		(imcontact *) &(*c), " [%c] %s",
		find(muins.begin(), muins.end(), *c) != muins.end() ? 'x' : ' ',
		cont->getdispnick().c_str()
	    );
	}

	m.setpos(saveelem, savefirst);

	switch(m.open()) {
	    case -2:
		if(m.getref(m.getpos())) {
		    ic = (imcontact *) m.getref(m.getpos());
		    c = find(muins.begin(), muins.end(), *ic);

		    if(c != muins.end()) {
			muins.erase(c);
		    } else {
			muins.push_back(*ic);
		    }
		}
		break;

	    case -3:
		quickfind(&m);
		break;

	    case 0:
		ret = false;

	    default:
		finished = true;
		break;
	}
    }

    restoreworkarea();
    return ret;
}

void icqface::log(const char *fmt, ...) {
    va_list ap;
    char buf[1024];

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    log((string) buf);
    va_end(ap);
}

void icqface::log(const string atext) {
    int i;
    string text = atext;

#ifdef DEBUG
    if(flog.is_open()) flog << text << endl;
#endif

    if(!mainscreenblock) {
	chtype *logline = new chtype[WORKAREA_X2-WORKAREA_X1+2];
	attrset(conf.getcolor(cp_main_text));

	for(i = WORKAREA_Y2+2; i < LINES-2; i++) {
	    mvinchnstr(i, WORKAREA_X1+1, logline, WORKAREA_X2-WORKAREA_X1);
	    mvaddchnstr(i-1, WORKAREA_X1+1, logline, WORKAREA_X2-WORKAREA_X1);
	}

	while((i = text.find("\n")) != -1) text[i] = ' ';
	while((i = text.find("\r")) != -1) text[i] = ' ';

	if(text.size() > WORKAREA_X2-WORKAREA_X1-2) text.resize(WORKAREA_X2-WORKAREA_X1-2);
	mvhline(LINES-3, WORKAREA_X1+2, ' ', WORKAREA_X2-WORKAREA_X1-2);
	kwriteatf(WORKAREA_X1+2, LINES-3, conf.getcolor(cp_main_text), "%s", text.c_str());
	delete logline;
    }
}

void icqface::status(const string text) {
    attrset(conf.getcolor(cp_status));
    mvhline(LINES-1, 0, ' ', COLS);
    kwriteatf(0, LINES-1, conf.getcolor(cp_status), "%s", text.c_str());
}

void icqface::blockmainscreen() {
    mainscreenblock = true;
}

void icqface::unblockmainscreen() {
    mainscreenblock = false;
    update();
}

void icqface::quickfind(verticalmenu *multi = 0) {
    bool fin;
    string nick, disp, upnick, upcurrent;
    string::iterator is;
    int k, i, len, lx, ly;
    bool found;
    icqcontact *c;
    verticalmenu *cm = (multi ? multi : &mcontacts->menu);

    status(_("QuickSearch: type to find, Alt-S find again, Enter finish"));

    if(multi) {
	lx = WORKAREA_X1+2;
	ly = WORKAREA_Y2;
    } else {
	lx = 2;
	ly = LINES-2;
    }

    for(fin = false; !fin; ) {
	attrset(conf.getcolor(cp_main_frame));
	mvhline(ly, lx, HLINE, 23);
	disp = nick;
	if(disp.size() > 18) disp.replace(0, disp.size()-18, "");
	kwriteatf(lx, ly, conf.getcolor(cp_main_highlight), "[ %s ]", disp.c_str());
	kgotoxy(lx+2+disp.size(), ly);
	refresh();

	if(cicq.idle())
	switch(k = getkey()) {
	    case KEY_ESC:
	    case '\r':
		fin = true;
		break;

	    case KEY_BACKSPACE:
		if(!nick.empty()) nick.resize(nick.size()-1);
		else fin = true;
		break;

	    default:
		if(isprint(k) || (k == ALT('s'))) {
		    i = cm->getpos() + (multi ? 1 : 2);

		    if(isprint(k)) {
			i--;
			nick += k;
		    }

		    for(is = nick.begin(), upnick = ""; is != nick.end(); is++)
			upnick += toupper(*is);

		    bool fin = false;
		    bool fpass = true;

		    for(; !fin; i++) {
			if(i >= cm->getcount()) {
			    if(fpass) {
				i = 0;
				fpass = false;
			    } else {
				fin = true;
				break;
			    }
			}

			if(multi) {
			    c = clist.get(*((imcontact *) cm->getref(i)));
			} else {
			    c = (icqcontact *) mcontacts->getref(i);
			}

			if((int) c > 100) {
			    string current = c->getdispnick();
			    len = current.size();
			    if(len > nick.size()) len = nick.size();
			    current.erase(len);

			    for(is = current.begin(), upcurrent = "";
			    is != current.end(); is++)
				    upcurrent += toupper(*is);

			    if(upnick == upcurrent) {
				cm->setpos(i - (multi ? 0 : 1));
				break;
			    }
			}
		    }

		    if(!multi) mcontacts->redraw();
		    else cm->redraw();
		}
		break;
	}
    }

    attrset(conf.getcolor(cp_main_frame));
    mvhline(ly, lx, HLINE, 23);
}

void icqface::extracturls(const string buf) {
    int pos = 0;
    regex_t r;
    regmatch_t rm[1];
    const char *pp = buf.c_str();

    extractedurls.clear();
    if(!regcomp(&r, "(http://[^ ,\t\n]+|ftp://[^, \t\n]+|www\\.[^, \t\n]+)", REG_EXTENDED)) {
	while(!regexec(&r, buf.substr(pos).c_str(), 1, rm, 0)) {
	    extractedurls.push_back(buf.substr(pos+rm[0].rm_so, rm[0].rm_eo-rm[0].rm_so));
	    pos += rm[0].rm_eo;
	}
	regfree(&r);
    }
}

void icqface::showextractedurls() {
    if(extractedurls.empty()) {
	log(_("+ no URLs within the current context"));
    } else {
	int n;
	vector<string>::iterator i;
	verticalmenu m(WORKAREA_X1+1, WORKAREA_Y1+3, WORKAREA_X2, WORKAREA_Y2,
	    conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));

	saveworkarea();
	clearworkarea();
	workarealine(WORKAREA_Y1+2);

	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1,
	    conf.getcolor(cp_main_highlight),
	    _("URLs within the current context"));

	for(i = extractedurls.begin(); i != extractedurls.end(); i++)
	    m.additem(" " + *i);

	if(n = m.open())
	    conf.openurl(extractedurls[n-1]);

	restoreworkarea();
    }
}

bool icqface::eventedit(imevent &ev) {
    bool r;
    texteditor editor;
    icqcontact *c;

    editdone = r = false;
    passinfo = ev.getcontact();

    editor.addscheme(cp_main_text, cp_main_text, 0, 0);
    editor.otherkeys = &editmsgkeys;
    editor.idle = &editidle;
    editor.wrap = true;

    saveworkarea();
    clearworkarea();

    workarealine(WORKAREA_Y1+2);

    mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	_("Outgoing %s to %s"), eventnames[ev.gettype()],
	ev.getcontact().totext().c_str());

    status(_("Ctrl-X send, Ctrl-P multiple, Ctrl-O history, Alt-? details, ESC cancel"));

    if(ev.gettype() == imevent::message) {
	immessage *m = static_cast<immessage *>(&ev);

	editor.setcoords(WORKAREA_X1+2, WORKAREA_Y1+3, WORKAREA_X2, WORKAREA_Y2);
	editor.load(m->gettext(), "");
	editor.open();

	r = editdone;

	char *p = editor.save("\r\n");
	*m = immessage(ev.getcontact(), imevent::outgoing, p);

	if(c = clist.get(ev.getcontact()))
	    c->setpostponed(r ? "" : p);

	delete p;

    } else if(ev.gettype() == imevent::url) {
	static textinputline urlinp;
	imurl *m = static_cast<imurl *>(&ev);

	urlinp.setvalue(m->geturl());
	urlinp.setcoords(WORKAREA_X1+2, WORKAREA_Y1+3, WORKAREA_X2-WORKAREA_X1-2);
	urlinp.setcolor(conf.getcolor(cp_main_highlight));
	urlinp.idle = &textinputidle;

	workarealine(WORKAREA_Y1+4);

	urlinp.exec();

	string url = urlinp.getlastkey() != KEY_ESC ? urlinp.getvalue() : "";

	if(!url.empty()) {
	    editor.setcoords(WORKAREA_X1+2, WORKAREA_Y1+5, WORKAREA_X2, WORKAREA_Y2);
	    editor.load(m->getdescription(), "");
	    editor.open();

	    r = editdone;

	    char *p = editor.save("\r\n");
	    *m = imurl(ev.getcontact(), imevent::outgoing, url, p);
	    delete p;
	}
    }

    restoreworkarea();
    status("");
    return r;
}

icqface::eventviewresult icqface::eventview(const imevent *ev) {
    string title_event, title_timestamp, text;
    horizontalbar *bar;
    dialogbox db;
    int mitem, baritem;
    eventviewresult r;

    vector<eventviewresult> actions;
    vector<eventviewresult>::iterator ia;

    if(ev->gettype() == imevent::message) {
	const immessage *m = static_cast<const immessage *>(ev);
	text = m->gettext();
	actions.push_back(forward);
	if(ev->getdirection() == imevent::incoming) {
	    actions.push_back(reply);
	}
	actions.push_back(ok);
    } else if(ev->gettype() == imevent::url) {
	const imurl *m = static_cast<const imurl *>(ev);
	text = m->geturl() + "\n\n" + m->getdescription();
	actions.push_back(forward);
	actions.push_back(open);
	if(ev->getdirection() == imevent::incoming) {
	    actions.push_back(reply);
	}
	actions.push_back(ok);
    } else if(ev->gettype() == imevent::sms) {
	if(ev->getdirection() == imevent::incoming) {
	    actions.push_back(reply);
	}
	actions.push_back(ok);
    } else if(ev->gettype() == imevent::authorization) {
	if(ev->getdirection() == imevent::incoming) {
	    actions.push_back(accept);
	    actions.push_back(reject);
	}
	actions.push_back(ok);
    }

    saveworkarea();
    clearworkarea();

    status(_("F2 to URLs, ESC close"));

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+3, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));

    db.setbrowser(new textbrowser(conf.getcolor(cp_main_text)));

    db.setbar(bar = new horizontalbar(conf.getcolor(cp_main_highlight),
	conf.getcolor(cp_main_selected), 0));

    bar->item = actions.size()-1;

    for(ia = actions.begin(); ia != actions.end(); ia++) {
	bar->items.push_back(eventviewresultnames[*ia]);
    }

    switch(ev->getdirection()) {
	case imevent::outgoing:
	    title_event = _("Outgoing %s to %s");
	    title_timestamp = _("Sent on %s");
	    break;
	case imevent::incoming:
	    title_event = _("Incoming %s from %s");
	    title_timestamp = _("Received on %s");
	    break;
    }

    mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	title_event.c_str(), eventnames[ev->gettype()], ev->getcontact().totext().c_str());

    mainw.writef(WORKAREA_X1+2, WORKAREA_Y1+1, conf.getcolor(cp_main_highlight),
	title_timestamp.c_str(), strdateandtime(ev->gettimestamp()).c_str());

    db.addautokeys();
    db.otherkeys = &userinfokeys;
    db.idle = &dialogidle;
    db.redraw();
    db.getbrowser()->setbuf(text);

    extracturls(text);

    workarealine(WORKAREA_Y1+3);
    workarealine(WORKAREA_Y2-2);

    if(db.open(mitem, baritem)) {
	r = actions[baritem];
    } else {
	r = cancel;
    }

    db.close();
    restoreworkarea();

    return r;
}

void icqface::histmake(const vector<imevent *> &hist) {
    vector<imevent *>::const_reverse_iterator i;
    string text;
    time_t t;
    char buf[64];
    int color;

    mhist.clear();
    mhist.setpos(0);

    for(i = hist.rbegin(); i != hist.rend(); i++) {
	imevent &ev = **i;

	color = 0;

	text = (string) + " " +
	    time2str(&(t = ev.gettimestamp()), "DD.MM hh:mm", buf) + " ";

	if(ev.gettype() == imevent::message) {
	    const immessage *m = static_cast<const immessage *>(&ev);
	    text += m->gettext();
	} else if(ev.gettype() == imevent::url) {
	    const imurl *m = static_cast<const imurl *>(&ev);
	    text += m->geturl() + " " + m->getdescription();
	}

	if(ev.getdirection() == imevent::incoming) {
	    color = conf.getcolor(cp_main_text);
	} else if(ev.getdirection() == imevent::outgoing) {
	    color = conf.getcolor(cp_main_highlight);
	}

	mhist.additem(color, (void *) *i, text);
    }
}

bool icqface::histexec(imevent *&im) {
    dialogbox db;
    bool r, fin;
    int k;
    string sub;

    r = fin = false;

    if(!mhist.empty()) {
	db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+2, WORKAREA_X2,
	    WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));

	db.setmenu(&mhist, false);

	db.idle = &dialogidle;
	db.otherkeys = &historykeys;

	saveworkarea();
	clearworkarea();

	db.redraw();
	workarealine(WORKAREA_Y1+2);

	im = static_cast<imevent *> (mhist.getref(0));

	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	    _("History items for %s"), im->getcontact().totext().c_str());

	status(_("S search, L again, ESC cancel"));

	while(!fin) {
	    if(db.open(k)) {
		switch(k) {
		    case -2:
			sub = face.inputstr(_("search for: "), sub);
		    case -3:
			break;

		    default:
			if(mhist.getref(k-1)) {
			    im = static_cast<imevent *> (mhist.getref(k-1));
			    r = fin = true;
			}
			break;
		}
	    } else {
		r = false;
		fin = true;
	    }
	}

	db.close();
	restoreworkarea();
    }

    return r;
}

// ----------------------------------------------------------------------------

void icqface::menuidle(verticalmenu &m) {
    cicq.idle();

    if(face.dotermresize)
	if(&m == &face.mcontacts->menu) {
	    face.done();
	    face.init();
	    face.draw();
	    face.dotermresize = false;
	}
}

void icqface::dialogidle(dialogbox &caller) {
    cicq.idle();
}

void icqface::textbrowseridle(textbrowser &b) {
    cicq.idle();
}

int icqface::contactskeys(verticalmenu &m, int k) {
    switch(k) {
	case '?': face.extk = ACT_INFO; break;

	case KEY_DC: face.extk = ACT_REMOVE; break;

	case 'q':
	case 'Q': face.extk = ACT_QUIT; break;

	case 'u':
	case 'U': face.extk = ACT_URL; break;

	case 's':
	case 'S':
	case KEY_F(3): face.extk = ACT_STATUS; break;

	case 'h':
	case 'H': face.extk = ACT_HISTORY; break;

	case 'a':
	case 'A': face.extk = ACT_ADD; break;

	case 'f':
	case 'F': face.extk = ACT_FILE; break;

	case 'm':
	case 'M':
	case KEY_F(2): face.extk = ACT_MENU; break;

	case 'g':
	case 'G':
	case KEY_F(4): face.extk = ACT_GMENU; break;

	case KEY_F(5): face.extk = ACT_HIDEOFFLINE; break;

	case 'r':
	case 'R': face.extk = ACT_RENAME; break;

	case 'c':
	case 'C': face.extk = ACT_CONTACT; break;
	
	case ALT('s'):
	case '/': face.extk = ACT_QUICKFIND; break;
    }

    if(k && (strchr("?rRqQsShHmMuUgGaAfFcC/", k)
	|| (k == KEY_DC)
	|| (k == KEY_F(2))
	|| (k == KEY_F(3))
	|| (k == KEY_F(4))
	|| (k == KEY_F(5))
	|| (k == ALT('s')))) {
	return m.getpos()+1;
    } else {
	return -1;
    }
}

int icqface::multiplekeys(verticalmenu &m, int k) {
    switch(k) {
	case ' ':
	case 'x':
	case 'X':
	    return -2;
	case ALT('s'):
	    return -3;
    }
    return -1;
}

int icqface::historykeys(dialogbox &db, int k) {
    static string sub;

    switch(k) {
	case 's':
	case 'S':
	    return -2;
	case 'l':
	case 'L':
	    return -3;
    }

    return -1;
}

int icqface::editmsgkeys(texteditor &e, int k) {
    char *p;

    switch(k) {
	case CTRL('x'):
	    p = e.save("");
	    face.editdone = strlen(p);
	    delete p;
	    if(face.editdone) return -1; else break;
	case CTRL('p'):
	    face.multicontacts("");
	    break;
	case CTRL('o'):
	    cicq.history(face.passinfo);
	    break;
	case ALT('?'):
	    cicq.userinfo(face.passinfo);
	    break;
	case 27:
	    return -1;
    }

    return 0;
}

int icqface::userinfokeys(dialogbox &db, int k) {
    switch(k) {
	case KEY_F(2): face.showextractedurls(); break;
    }

    return -1;
}

void icqface::editidle(texteditor &e) {
    cicq.idle();
}

void icqface::textinputidle(textinputline &il) {
    cicq.idle();
}

void icqface::termresize(void) {
    face.dotermresize = true;
}

// ----------------------------------------------------------------------------

icqface::icqprogress::icqprogress(): curline(0), w(0) {
}

icqface::icqprogress::~icqprogress() {
    delete w;
}

void icqface::icqprogress::log(const char *fmt, ...) {
    va_list ap;
    char buf[1024];

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    if(curline >= DIALOG_HEIGHT-1) {
	w->redraw();
	curline = 0;
    }

    w->write(2, 1+curline++, buf);
}

void icqface::icqprogress::show(const string title = "") {
    if(!w) {
	w = new textwindow(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT,
	conf.getcolor(cp_dialog_frame), TW_CENTERED);
    }

    w->set_title(conf.getcolor(cp_dialog_highlight), title);
    w->open();
}

void icqface::icqprogress::hide() {
    w->close();
}

void icqface::relaxedupdate() {
    fneedupdate = true;
}

bool icqface::updaterequested() {
    return fneedupdate;
}
