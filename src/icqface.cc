/*
*
* centericq user interface class
* $Id: icqface.cc,v 1.114 2002/05/09 11:41:07 konst Exp $
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
#include "imlogger.h"

const char *strregsound(icqconf::regsound s) {
    return s == icqconf::rscard ? _("sound card") :
	s == icqconf::rsspeaker ? _("speaker") :
	s == icqconf::rsdisable ? _("disable") :
	_("don't change");
}

const char *strregcolor(icqconf::regcolor c) {
    return c == icqconf::rcdark ? _("dark") :
	c == icqconf::rcblue ? _("blue") :
	_("don't change");
}

const char *strint(unsigned int i) {
    static char buf[64];

    if(i) {
	sprintf(buf, "%lu", i);
    } else {
	buf[0] = 0;
    }

    return buf;
}

const char *strgroupmode(icqconf::groupmode gmode) {
    return gmode == icqconf::group1 ? _("mode 1") :
	gmode == icqconf::group2 ? _("mode 2") :
	    _("no");
}

icqface::icqface() {
    workareas.freeitem = &freeworkareabuf;
    mainscreenblock = inited = onlinefolder = dotermresize = inchat = false;
    mcontacts = 0;
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
#ifdef DEBUG
    if(!flog.is_open()) {
	time_t logtime = time(0);

	flog.open((conf.getdirname() + "debug").c_str(), ios::app);
	if(flog.is_open())
	    flog << endl << "-- centericq debug log started on " << ctime(&logtime);
    }
#endif

    /* Calculate various sizes and coordinates */

    sizeDlg.width = COLS-20;
    sizeDlg.height = LINES-10;

    sizeBigDlg.width = COLS-10;
    sizeBigDlg.height = LINES-7;

    sizeWArea.x1 = (int) (COLS*0.32);
    if(sizeWArea.x1 < 25)
	sizeWArea.x1 = 25;

    sizeWArea.x2 = COLS-1;
    sizeWArea.y1 = 1;

    sizeWArea.y2 = LINES - ((int) (LINES/4));
    if(sizeWArea.y2 > LINES-6)
	sizeWArea.y2 = LINES-6;

    if(!mcontacts) {
	mcontacts = new treeview(conf.getcolor(cp_main_menu),
	    conf.getcolor(cp_main_selected),
	    conf.getcolor(cp_main_menu),
	    conf.getcolor(cp_main_menu));
    }

    mcontacts->setcoords(1, 2, sizeWArea.x1, LINES-2);
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
    kt_resize_event = &termresize;
}

void icqface::done() {
    kt_resize_event = 0;
    workareas.empty();
}

void icqface::draw() {
    mainw.open();
    mainw.separatex(sizeWArea.x1);
    workarealine(sizeWArea.y2);
    mvhline(sizeWArea.y2, sizeWArea.x1, LTEE, 1);
    mvhline(sizeWArea.y2, sizeWArea.x2, RTEE, 1);

    update();
}

void icqface::showtopbar() {
    string buf;
    protocolname pname;
    icqconf::imaccount ia;

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

    attrset(conf.getcolor(cp_status));
    mvhline(0, 0, ' ', COLS);
    mvprintw(0, 2, "CENTERICQ %s   UNSENT: %lu", VERSION, em.getunsentcount());
    mvprintw(0, COLS-buf.size()-2, "%s", buf.c_str());
}

void icqface::update() {
    showtopbar();
    fillcontactlist();
    fneedupdate = false;
}

int icqface::contextmenu(icqcontact *c) {
    int ret = 0, i, capab;
    /*static */int elem = 0;
    imcontact cont;

    verticalmenu m(conf.getcolor(cp_main_text),
	conf.getcolor(cp_main_selected));

    m.setwindow(textwindow(sizeWArea.x1, sizeWArea.y1, sizeWArea.x1+27,
	sizeWArea.y1+6, conf.getcolor(cp_main_text)));

    if(!c) return 0;

    cont = c->getdesc();
    capab = gethook(cont.pname).getcapabilities();

    if(cont != contactroot) {
	if(cont.pname != infocard)
	    m.additem(0, ACT_MSG, _(" Send a message     enter"));

	if(capab & hoptCanSendURL)
	    m.additem(0, ACT_URL, _(" Send an URL            u"));

	if(capab & hoptCanSendSMS)
	    m.additem(0, ACT_SMS, _(" Send an SMS"));

	if(!m.empty()) m.addline();

	m.additem(0, ACT_INFO, _(" User's details         ?"));
	m.additem(0, ACT_EDITUSER, _(" Edit details"));

	if(c->getstatus() != offline)
	if(capab & hoptCanFetchAwayMsg)
	    m.additem(0, ACT_FETCHAWAY, _(" Fetch away message"));
    }

    m.additem(0, ACT_HISTORY, _(" Events history         h"));

    if(cont != contactroot) {
	m.addline();

	m.additem(0, ACT_IGNORE, _(" Ignore user"));

	if(!c->inlist()) {
	    m.additem(0, ACT_ADD, _(" Add to list            a"));
	}
    }

    if(c->getdesc() != contactroot) {
	m.additem(0, ACT_REMOVE, _(" Remove user          del"));
	m.additem(0, ACT_RENAME, _(" Rename contact         r"));
	if(conf.getgroupmode() != icqconf::nogroups)
	    m.additem(0, ACT_GROUPMOVE, _(" Move to group.."));
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
    m.setwindow(textwindow(sizeWArea.x1, sizeWArea.y1, sizeWArea.x1+40, sizeWArea.y1+11, conf.getcolor(cp_main_text)));

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

    if(conf.getgroupmode() != icqconf::nogroups) {
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
    icqcontact *c = 0; icqgroup *g;
    bool fin;

    for(fin = false; !fin; ) {
	extk = ACT_MSG;

	/* Obtain the (icqcontact *) from the treeview. If a node is
	   selected, throw out the contact and obtain the correct (icqgroup *). */

	c = (icqcontact *) mcontacts->open(&i);

	if(mcontacts->isnode(i) && c) {
	    c = 0;
	    g = (icqgroup *) mcontacts->getref(mcontacts->getid(mcontacts->menu.getpos()));
	} else {
	    g = 0;
	}
	
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

			/* kkconsui::treeview has collapsing built in, but when doing it
			   this way contacts with pending messages get moved to the top
			   even if their group is colapsed. */
			
			if(mcontacts->isnodeopen(curid)) {
			    mcontacts->closenode(curid);
			} else {
			    mcontacts->opennode(curid);
			}

			/* Handling of collapse events should happen in centericq::
			   mainloop, but as it stands this method doesn't handle
			   icqgroups, only icqcontacts, so we'll deal with collapsing
			   here. */
			   
			if(g) {
			    g->changecollapsed();
			    update();
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


#define ADDGROUP(nn) { \
    if(groupchange && !strchr("!", sc)) \
	ngroup = mcontacts->addnode(nn, conf.getcolor(cp_main_highlight), \
	g, " " + g->getname()  + " " \
	+ (g->iscollapsed() ? "(" + string(i2str(g->getcount(c->getstatus() != offline, !conf.gethideoffline() && !(c->getstatus() != offline && conf.getgroupmode() == icqconf::group2)))) +  ") " : "")); \
}

void icqface::fillcontactlist() {
    int i, id, nnode, ngroup, prevgid, n, nonline;
    string dnick;
    icqcontact *c;
    void *savec;
    char prevsc = 'x', sc;
    icqgroup *g = NULL;
    vector<icqgroup>::iterator ig;

    bool online_added = false, groupchange, prevoffline = false, grouponline = true, ontop, iscurrentnode;

    nnode = ngroup = prevgid = 0;

    iscurrentnode = mcontacts->isnode(mcontacts->getid(mcontacts->menu.getpos()));
    ontop = !mcontacts->menu.getpos() && iscurrentnode;

    savec = mcontacts->getref(mcontacts->getid(mcontacts->menu.getpos()));

    if(!iscurrentnode && savec) {
	ig = find(groups.begin(), groups.end(), ((icqcontact *) savec)->getgroupid());

	if(ig != groups.end()) {
	    g = &(*ig);
	    if(g->iscollapsed() && !((icqcontact *) savec)->getmsgcount())
		savec = g;
	}
    }

    /* if savec is a group, we need to figure out of it's the top or bottom
       one when we're in groupmode mode 2. */

    if(savec && iscurrentnode && conf.getgroupmode() == icqconf::group2) {
	for(i = 0; i < mcontacts->getcount(); i++) {
	    if(mcontacts->getref(mcontacts->getid(i)) == savec) {
		if(mcontacts->getid(i) != mcontacts->getid(mcontacts->menu.getpos()))
		    grouponline = false;
		break;
	    }
	}
    }

    mcontacts->clear();
    clist.order();

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	ig = find(groups.begin(), groups.end(), c->getgroupid());
	if(ig != groups.end()) {
	    g = &(*ig);
	}

	if(c->getdesc() == contactroot)
	    continue;
	    
	if(c->getstatus() == offline)
	if(conf.gethideoffline())
	if(!c->getmsgcount()) {
	    continue;
	}

	sc = SORTCHAR(c);

	groupchange =
	    (conf.getgroupmode() != icqconf::nogroups) &&
	    ((c->getgroupid() != prevgid) ||
	    ((conf.getgroupmode() == icqconf::group2) && 
		(prevoffline == false) && (c->getstatus() == offline))) &&
	    (sc != '#');

	if(conf.getgroupmode() == icqconf::group1)
	    ADDGROUP(0);

	if(groupchange || (sc != '#')) {
	    if(strchr("candifo", sc)) sc = 'O';

	    if((sc != prevsc) || groupchange) {
		switch(conf.getgroupmode()) {
		    case icqconf::group1:
			n = ngroup;
			break;
		    case icqconf::group2:
			if(prevsc == sc) {
			    n = -1;
			} else {
			    n = 0;
			}
			break;
		    default:
			n = 0;
			break;
		}

		/* Hide 'offline' and 'online' items when in group mode 1 and the current
		   group is collapsed */

		if(n != -1) {
		    switch(sc) {
			case 'O':
			    nnode = (conf.getgroupmode() == icqconf::group1 && g->iscollapsed()) ||
			     (conf.gethideoffline() && (conf.getgroupmode() != icqconf::nogroups)) ?
				n : mcontacts->addnode(n,
				    conf.getcolor(cp_main_highlight), 0, " Online ");

			    online_added = true;
			    break;
			case '_':
			    nnode = (conf.getgroupmode() == icqconf::group1 && g->iscollapsed())  ? 
				n : mcontacts->addnode(n, conf.getcolor(cp_main_highlight), 0, " Offline ");
			    break;
			case '!':
			    ngroup = 0;
			    nnode = mcontacts->addnode(ngroup,
				conf.getcolor(cp_main_highlight), 0, " Not in list ");
			    break;
		    }

		    nonline = nnode;
		}

		if(conf.getgroupmode() == icqconf::group2)
		if(groupchange && (sc != '!')) {
		    ADDGROUP(nonline);
		    nnode = ngroup;
		}
	    }

	    if(groupchange) prevgid = c->getgroupid();
	    prevoffline = (c->getstatus() == offline) ? true : false;
	    prevsc = sc;
	}

	dnick = c->getdispnick();
	if(c->isbirthday()) dnick += " :)";

	if(conf.getgroupmode() != icqconf::nogroups && g->iscollapsed() &&
	    !c->getmsgcount() && sc != '!')
		continue;

	if(c->getstatus() == offline) {
	    mcontacts->addleaff(nnode,
		c->getmsgcount() ? conf.getcolor(cp_main_highlight) : conf.getprotcolor(c->getdesc().pname),
		c, "%s%s ", c->getmsgcount() ? "#" : " ", dnick.c_str());
	} else {
	    mcontacts->addleaff(nnode,
		c->getmsgcount() ? conf.getcolor(cp_main_highlight) : conf.getprotcolor(c->getdesc().pname),
		c, "%s[%c] %s ", c->getmsgcount() ? "#" : " ", c->getshortstatus(), dnick.c_str());
	}

//        if(savec && c->getmsgcount() && ontop) savec = 0;
    }

    if(!mainscreenblock) mcontacts->redraw();

    if(!savec || ontop || (!onlinefolder && online_added && conf.gethideoffline())) {
	mcontacts->menu.setpos(0);

    } else if(savec) {
	for(i = 0; i < mcontacts->menu.getcount(); i++)
	if(mcontacts->getref(mcontacts->getid(i)) == savec) {
	    if(!grouponline) grouponline = true; else {
		mcontacts->menu.setpos(i);
		break;
	    }
	}

    }

    onlinefolder = online_added;

    if(!mainscreenblock) {
	mcontacts->menu.redraw();
    }
}

bool icqface::findresults(const imsearchparams &sp) {
    bool finished = false, ret = false;
    unsigned int ffuin;
    icqcontact *c;
    dialogbox db;
    int r, b;
    char *nick;

    saveworkarea();
    clearworkarea();

    db.setwindow(new textwindow(sizeWArea.x1, sizeWArea.y1+2, sizeWArea.x2,
	sizeWArea.y2, conf.getcolor(cp_main_text), TW_NOBORDER));
    db.setmenu(new verticalmenu(conf.getcolor(cp_main_menu),
	conf.getcolor(cp_main_selected)));
    db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	conf.getcolor(cp_main_selected), _("Details"), _("Message"), _("Add"),
	_("New search"), 0));

    gethook(sp.pname).lookup(sp, *db.getmenu());

    db.addautokeys();
    db.redraw();

    db.idle = &dialogidle;
    db.otherkeys = &findreskeys;

    mainw.write(sizeWArea.x1+2, sizeWArea.y1, conf.getcolor(cp_main_highlight), "Find results");

    workarealine(sizeWArea.y1+2);
    workarealine(sizeWArea.y2-2);

    while(!finished) {
	finished = !db.open(r, b);
	gethook(sp.pname).stoplookup();

	if(!finished) {
	    if(r == -3) {
		quickfind(db.getmenu());

	    } else switch(b) {
		case 0:
		    if(r) {
			c = (icqcontact *) db.getmenu()->getref(r-1);
			if(c) cicq.userinfo(c->getdesc());
		    }
		    break;

		case 1:
		    if(r) {
			bool existed;
			imcontact cont = ((icqcontact *) db.getmenu()->getref(r-1))->getdesc();

			existed = (c = clist.get(cont));
			if(!existed) c = clist.addnew(cont);

			if(c)
			if(!cicq.sendevent(immessage(cont, imevent::outgoing, ""), icqface::ok))
			if(!existed)
			    clist.remove(cont);
		    }
		    break;

		case 2:
		    if(r) {
			c = (icqcontact *) db.getmenu()->getref(r-1);
			if(c) cicq.addcontact(c->getdesc());
		    }
		    break;

		case 3:
		    ret = finished = true;
		    break;
	    }
	}
	
    }

    db.close();
    restoreworkarea();

    return ret;
}

void icqface::infoclear(dialogbox &db, icqcontact *c, const imcontact realdesc) {
    for(int i = sizeWArea.y1+1; i < sizeWArea.y2; i++) {
	workarealine(i, ' ');
    }

    db.redraw();

    mainw.writef(sizeWArea.x1+2, sizeWArea.y1, conf.getcolor(cp_main_highlight),
	_("Information about %s"), realdesc.totext().c_str());

    workarealine(sizeWArea.y1+2);
    workarealine(sizeWArea.y2-2);

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

void commacat(string &text, string sub, bool nocomma = false) {
    if(!sub.empty()) {
	if(!nocomma)
	if(text[text.size()-1] != ',') text += ",";

	text += sub;
    }
}

void commaform(string &text) {
    int pos = 0, ipos;

    while((pos = text.find(",", pos)) != -1) {
	if(text.substr(pos+1, 1) != " ") {
	    text.insert(pos+1, " ");
	}

	ipos = pos-1;
	while(ipos > 0) {
	    if(text.substr(ipos, 1) == " ") {
		text.erase(ipos--, 1);
	    } else {
		break;
	    }
	}

	pos++;
    }
}

void icqface::infogeneral(dialogbox &db, icqcontact *c) {
    string langs;
    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();

    workarealine(sizeWArea.y1+8);
    workarealine(sizeWArea.y1+12);

    mainw.write(sizeWArea.x1+2, sizeWArea.y1+2, conf.getcolor(cp_main_highlight), _("Nickname"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+3, conf.getcolor(cp_main_highlight), _("Name"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+4, conf.getcolor(cp_main_highlight), _("E-mail"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+5, conf.getcolor(cp_main_highlight), _("2nd e-mail"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+6, conf.getcolor(cp_main_highlight), _("Old e-mail"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+8, conf.getcolor(cp_main_highlight), _("Gender"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+9, conf.getcolor(cp_main_highlight), _("Birthdate"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+10, conf.getcolor(cp_main_highlight), _("Age"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+12, conf.getcolor(cp_main_highlight), _("Languages"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+13, conf.getcolor(cp_main_highlight), _("Last IP"));

    mainw.write(sizeWArea.x1+14, sizeWArea.y1+2, conf.getcolor(cp_main_text), c->getnick());
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+3, conf.getcolor(cp_main_text), bi.fname + " " + bi.lname);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+4, conf.getcolor(cp_main_text), bi.email);

    mainw.write(sizeWArea.x1+14, sizeWArea.y1+8, conf.getcolor(cp_main_text), strgender[mi.gender]);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+9, conf.getcolor(cp_main_text), mi.strbirthdate());
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+10, conf.getcolor(cp_main_text), mi.age ? i2str(mi.age) : "");

    if(mi.lang1) commacat(langs, ICQ2000::UserInfoHelpers::getLanguageIDtoString(mi.lang1), langs.empty());
    if(mi.lang2) commacat(langs, ICQ2000::UserInfoHelpers::getLanguageIDtoString(mi.lang2), langs.empty());
    if(mi.lang3) commacat(langs, ICQ2000::UserInfoHelpers::getLanguageIDtoString(mi.lang3), langs.empty());
    commaform(langs);

    mainw.write(sizeWArea.x1+14, sizeWArea.y1+12, conf.getcolor(cp_main_text), langs);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+13, conf.getcolor(cp_main_text), c->getlastip());

    if(c->getstatus() == offline) {
	time_t ls = c->getlastseen();

	mainw.write(sizeWArea.x1+2, sizeWArea.y1+14, conf.getcolor(cp_main_highlight), _("Last seen"));
	mainw.write(sizeWArea.x1+14, sizeWArea.y1+14, conf.getcolor(cp_main_text),
	    ls ? strdateandtime(ls) : _("Never"));
    }
}

void icqface::infohome(dialogbox &db, icqcontact *c) {
    int i, x;

    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();

    workarealine(sizeWArea.y1+10);
    x = sizeWArea.x1+2;

    mainw.write(x, sizeWArea.y1+2, conf.getcolor(cp_main_highlight), _("Address"));
    mainw.write(x, sizeWArea.y1+3, conf.getcolor(cp_main_highlight), _("Location"));
    mainw.write(x, sizeWArea.y1+4, conf.getcolor(cp_main_highlight), _("Zip code"));
    mainw.write(x, sizeWArea.y1+5, conf.getcolor(cp_main_highlight), _("Phone"));
    mainw.write(x, sizeWArea.y1+6, conf.getcolor(cp_main_highlight), _("Fax"));
    mainw.write(x, sizeWArea.y1+7, conf.getcolor(cp_main_highlight), _("Cellular"));
    mainw.write(x, sizeWArea.y1+8, conf.getcolor(cp_main_highlight), _("Timezone"));

    mainw.write(x, sizeWArea.y1+10, conf.getcolor(cp_main_highlight), _("Homepage"));

    if(!bi.state.empty()) {
	if(bi.city.size()) bi.city += ", ";
	bi.city += bi.state;
    }

    if(bi.country) {
	if(bi.city.size()) bi.city += ", ";
	bi.city += ICQ2000::UserInfoHelpers::getCountryIDtoString(bi.country);
    }

    x += 10;
    mainw.write(x, sizeWArea.y1+2, conf.getcolor(cp_main_text), bi.street);
    mainw.write(x, sizeWArea.y1+3, conf.getcolor(cp_main_text), bi.city);
    mainw.write(x, sizeWArea.y1+4, conf.getcolor(cp_main_text), bi.zip);
    mainw.write(x, sizeWArea.y1+5, conf.getcolor(cp_main_text), bi.phone);
    mainw.write(x, sizeWArea.y1+6, conf.getcolor(cp_main_text), bi.fax);
    mainw.write(x, sizeWArea.y1+7, conf.getcolor(cp_main_text), bi.cellular);
    mainw.write(x, sizeWArea.y1+8, conf.getcolor(cp_main_text), mi.strtimezone());

    for(i = 0; !mi.homepage.empty(); i++) {
	mainw.write(x, sizeWArea.y1+10+i, conf.getcolor(cp_main_text),
	    mi.homepage.substr(0, sizeWArea.x2-sizeWArea.x1-12));
	mi.homepage.erase(0, sizeWArea.x2-sizeWArea.x1-12);
    }
}

void icqface::infowork(dialogbox &db, icqcontact *c) {
    int i;
    icqcontact::workinfo wi = c->getworkinfo();

    workarealine(sizeWArea.y1+6);
    workarealine(sizeWArea.y1+11);

    mainw.write(sizeWArea.x1+2, sizeWArea.y1+2, conf.getcolor(cp_main_highlight), _("Address"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+3, conf.getcolor(cp_main_highlight), _("Location"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+4, conf.getcolor(cp_main_highlight), _("Zip code"));

    mainw.write(sizeWArea.x1+2, sizeWArea.y1+6, conf.getcolor(cp_main_highlight), _("Company"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+7, conf.getcolor(cp_main_highlight), _("Department"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+8, conf.getcolor(cp_main_highlight), _("Title"));

    mainw.write(sizeWArea.x1+2, sizeWArea.y1+11, conf.getcolor(cp_main_highlight), _("Phone"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+12, conf.getcolor(cp_main_highlight), _("Fax"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+13, conf.getcolor(cp_main_highlight), _("Homepage"));

    if(!wi.state.empty()) {
	if(wi.city.size()) wi.city += ", ";
	wi.city += wi.state;
    }

    if(wi.country) {
	if(wi.city.size()) wi.city += ", ";
	wi.city += ICQ2000::UserInfoHelpers::getCountryIDtoString(wi.country);
    }

    mainw.write(sizeWArea.x1+14, sizeWArea.y1+2, conf.getcolor(cp_main_text), wi.street);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+3, conf.getcolor(cp_main_text), wi.city);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+4, conf.getcolor(cp_main_text), wi.zip);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+6, conf.getcolor(cp_main_text), wi.company);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+7, conf.getcolor(cp_main_text), wi.dept);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+8, conf.getcolor(cp_main_text), wi.position);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+11, conf.getcolor(cp_main_text), wi.phone);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+12, conf.getcolor(cp_main_text), wi.fax);

    for(i = 0; !wi.homepage.empty(); i++) {
	mainw.write(sizeWArea.x1+14, sizeWArea.y1+13+i, conf.getcolor(cp_main_text),
	    wi.homepage.substr(0, sizeWArea.x2-sizeWArea.x1-14));
	wi.homepage.erase(0, sizeWArea.x2-sizeWArea.x1-14);
    }
}

void icqface::infoabout(dialogbox &db, icqcontact *c) {
    db.getbrowser()->setbuf(c->getabout());
}

void icqface::infointerests(dialogbox &db, icqcontact *c) {
    string text;
    vector<string> data;
    vector<string>::iterator i;

    data = c->getinterests();

    if(!data.empty()) {
	text += (string) "* " + _("Interests") + "\n";

	for(i = data.begin(); i != data.end(); i++) {
	    text += " + " + *i + "\n";
	}

	text += "\n";
    }

    data = c->getbackground();

    if(!data.empty()) {
	text += (string) "* " + _("Background") + "\n";

	for(i = data.begin(); i != data.end(); i++)
	    text += " + " + *i + "\n";
    }

    commaform(text);
    db.getbrowser()->setbuf(text);
}

void icqface::userinfo(const imcontact &cinfo, const imcontact &realinfo) {
    bool finished = false, showinfo;
    icqcontact *c = clist.get(realinfo);
    textbrowser tb(conf.getcolor(cp_main_text));
    dialogbox db;
    int k, lastb, b;

    b = lastb = 0;
    saveworkarea();
    clearworkarea();

    status(_("F2 to URLs, ESC close"));

    db.setwindow(new textwindow(sizeWArea.x1, sizeWArea.y1+2, sizeWArea.x2,
	sizeWArea.y2, conf.getcolor(cp_main_text), TW_NOBORDER));

    db.setbar(new horizontalbar(sizeWArea.x1+2, sizeWArea.y2-1,
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
			    showinfo = true;
			    break;

			default:
			    gethook(cinfo.pname).requestinfo(cinfo);
			    break;
		    }

		    b = lastb;
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
	_(" [aim] AOL TOC"),
	_(" [irc] IRC"),
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

    m.setwindow(textwindow(sizeWArea.x1, sizeWArea.y1, sizeWArea.x1+27,
	sizeWArea.y1+9, conf.getcolor(cp_main_text)));

    m.idle = &menuidle;
    makeprotocolmenu(m);

    if(m.getcount() < 2) {
	i = 1;
    } else {
	m.addline();
	m.additem(0, proto_all, _(" [all] All of them"));
	m.scale();

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

	    if(pname != proto_all)
	    if(gethook(pname).getstatus() == *im)
		m.setpos(m.getcount()-1);
	}

	if(pname == proto_all) {
	    m.setpos(0);
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

string icqface::inputstr(const string &q, const string &defl = "", char passwdchar = 0) {
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

string icqface::inputfile(const string &q, const string &defl = "") {
    screenarea sa(0, INPUT_POS, COLS, INPUT_POS);
    string r;

    mvhline(INPUT_POS, 0, ' ', COLS);
    kwriteatf(0, INPUT_POS, conf.getcolor(cp_status), "%s", q.c_str());
    kwriteatf(COLS-8, INPUT_POS, conf.getcolor(cp_status), "[Ctrl-T]");

    selector.setwindow(textwindow(0, 0, sizeDlg.width, sizeDlg.height,
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

int icqface::getlastinputkey() const {
    return input.getlastkey();
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
    chtype **workareabuf = (chtype **) malloc(sizeof(chtype *) * (sizeWArea.y2-sizeWArea.y1+1));

    for(i = 0; i <= sizeWArea.y2-sizeWArea.y1; i++) {
	workareabuf[i] = (chtype *) malloc(sizeof(chtype) * (sizeWArea.x2-sizeWArea.x1+2));
	mvinchnstr(sizeWArea.y1+i, sizeWArea.x1, workareabuf[i], sizeWArea.x2-sizeWArea.x1+1);
    }

    workareas.add(workareabuf);
}

void icqface::restoreworkarea() {
    int i;
    chtype **workareabuf = (chtype **) workareas.at(workareas.count-1);

    if(workareabuf && !dotermresize) {
	for(i = 0; i <= sizeWArea.y2-sizeWArea.y1; i++) {
	    mvaddchnstr(sizeWArea.y1+i, sizeWArea.x1,
		workareabuf[i], sizeWArea.x2-sizeWArea.x1+1);
	}

	refresh();
    }

    workareas.remove(workareas.count-1);
}

void icqface::clearworkarea() {
    int i;

    attrset(conf.getcolor(cp_main_text));
    for(i = sizeWArea.y1+1; i < sizeWArea.y2; i++) {
	mvhline(i, sizeWArea.x1+1, ' ', sizeWArea.x2-sizeWArea.x1-1);
	refresh();
    }
}

void icqface::freeworkareabuf(void *p) {
    chtype **workareabuf = (chtype **) p;
    if(workareabuf) {
	for(int i = 0; i <= face.sizeWArea.y2-face.sizeWArea.y1; i++) {
	    free((chtype *) workareabuf[i]);
	}

	free(workareabuf);
    }
}

void icqface::workarealine(int l, chtype c = HLINE) {
    attrset(conf.getcolor(cp_main_frame));
    mvhline(l, sizeWArea.x1+1, c, sizeWArea.x2-sizeWArea.x1-1);
}

void icqface::modelist(contactstatus cs) {
    int i, b;
    icqcontact *c;
    dialogbox db;
    modelistitem it;
    vector<imcontact>::iterator ic;

    saveworkarea();
    clearworkarea();

    db.setwindow(new textwindow(sizeWArea.x1, sizeWArea.y1+2, sizeWArea.x2,
	sizeWArea.y2, conf.getcolor(cp_main_text), TW_NOBORDER));

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
    workarealine(sizeWArea.y1+2);
    workarealine(sizeWArea.y2-2);

    mainw.write(sizeWArea.x1+2, sizeWArea.y1, conf.getcolor(cp_main_highlight),
	cs == csignore      ? _("Ignore list") :
	cs == csvisible     ? _("Visible list") :
	cs == csinvisible   ? _("Invisible list") : "");

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

bool icqface::multicontacts(const string &ahead = "") {
    int i, savefirst, saveelem;
    bool ret = true, finished = false;
    string head = ahead;

    imcontact ic;
    vector<imcontact>::iterator c;
    vector<imcontact> mlst;

    verticalmenu m(sizeWArea.x1+1, sizeWArea.y1+3, sizeWArea.x2, sizeWArea.y2,
	conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));

    saveworkarea();
    clearworkarea();

    workarealine(sizeWArea.y1+2);

    if(!head.size()) head = _("Event recipients");
    mainw.write(sizeWArea.x1+2, sizeWArea.y1, conf.getcolor(cp_main_highlight), head);

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

	    m.additemf(conf.getprotcolor(c->pname), cont, " [%c] %s",
		find(muins.begin(), muins.end(), *c) != muins.end() ? 'x' : ' ',
		cont->getdispnick().c_str()
	    );
	}

	m.setpos(saveelem, savefirst);

	switch(m.open()) {
	    case -2:
		if(m.getref(m.getpos())) {
		    ic = ((icqcontact *) m.getref(m.getpos()))->getdesc();
		    c = find(muins.begin(), muins.end(), ic);

		    if(c != muins.end()) {
			muins.erase(c);
		    } else {
			muins.push_back(ic);
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

void icqface::log(const string &atext) {
    int i;
    string text = atext;

#ifdef DEBUG
    if(flog.is_open())
	flog << text << endl;
#endif

    while((i = text.find("\n")) != -1) text[i] = ' ';
    while((i = text.find("\r")) != -1) text[i] = ' ';

    while(lastlog.size() > LINES-sizeWArea.y2-2)
	lastlog.erase(lastlog.begin());

    lastlog.push_back(text);

    if(!mainscreenblock && (sizeWArea.x2-sizeWArea.x1 > 0)) {
	chtype *logline = new chtype[sizeWArea.x2-sizeWArea.x1+2];
	attrset(conf.getcolor(cp_main_text));

	for(i = sizeWArea.y2+2; i < LINES-2; i++) {
	    mvinchnstr(i, sizeWArea.x1+1, logline, sizeWArea.x2-sizeWArea.x1);
	    mvaddchnstr(i-1, sizeWArea.x1+1, logline, sizeWArea.x2-sizeWArea.x1);
	}

	delete logline;

	if(text.size() > sizeWArea.x2-sizeWArea.x1-2) text.resize(sizeWArea.x2-sizeWArea.x1-2);
	mvhline(LINES-3, sizeWArea.x1+2, ' ', sizeWArea.x2-sizeWArea.x1-2);
	kwriteatf(sizeWArea.x1+2, LINES-3, conf.getcolor(cp_main_text), "%s", text.c_str());
    }
}

void icqface::status(const string &text) {
    attrset(conf.getcolor(cp_status));
    mvhline(LINES-1, 0, ' ', COLS);
    kwriteatf(0, LINES-1, conf.getcolor(cp_status), "%s", (fstatus = text).c_str());
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
	lx = sizeWArea.x1+2;
	ly = sizeWArea.y2;
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
	    case CTRL('h'):
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
			c = 0;

			if(i > cm->getcount()) {
			    if(fpass) {
				i = 0;
				fpass = false;
			    } else {
				fin = true;
				break;
			    }
			}
			
			if(!multi && mcontacts->isnode(i)) {
			    c = 0;
			} else if(!multi) {
			    c = (icqcontact *) mcontacts->getref(i);
			} else {
			    c = (icqcontact *) cm->getref(i);
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

void icqface::extracturls(const string &buf) {
    int pos = 0;
    regex_t r;
    regmatch_t rm[1];
    const char *pp = buf.c_str();

    extractedurls.clear();
    if(!regcomp(&r, "(http://[^ ,\t\n]+|https://[^ ,\t\n]+|ftp://[^, \t\n]+|www\\.[^, \t\n]+)", REG_EXTENDED)) {
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
	verticalmenu m(sizeWArea.x1+1, sizeWArea.y1+3, sizeWArea.x2, sizeWArea.y2,
	    conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));

	saveworkarea();
	clearworkarea();
	workarealine(sizeWArea.y1+2);

	mainw.writef(sizeWArea.x1+2, sizeWArea.y1,
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

    workarealine(sizeWArea.y1+2);

    mainw.writef(sizeWArea.x1+2, sizeWArea.y1, conf.getcolor(cp_main_highlight),
	_("Outgoing %s to %s"), eventnames[ev.gettype()],
	ev.getcontact().totext().c_str());

    status(_("Ctrl-X send, Ctrl-P multiple, Ctrl-O history, Alt-? details, ESC cancel"));

    if(ev.gettype() == imevent::message) {
	immessage *m = static_cast<immessage *>(&ev);

	editor.setcoords(sizeWArea.x1+2, sizeWArea.y1+3, sizeWArea.x2, sizeWArea.y2);
	editor.load(m->gettext(), "");
	editor.open();

	r = editdone;

	auto_ptr<char> p(editor.save("\r\n"));
	*m = immessage(ev.getcontact(), imevent::outgoing, p.get());

	if(c = clist.get(ev.getcontact()))
	    c->setpostponed(r ? "" : p.get());

    } else if(ev.gettype() == imevent::url) {
	static textinputline urlinp;
	imurl *m = static_cast<imurl *>(&ev);

	urlinp.setvalue(m->geturl());
	urlinp.setcoords(sizeWArea.x1+2, sizeWArea.y1+3, sizeWArea.x2-sizeWArea.x1-2);
	urlinp.setcolor(conf.getcolor(cp_main_highlight));
	urlinp.idle = &textinputidle;

	workarealine(sizeWArea.y1+4);

	urlinp.exec();

	string url = urlinp.getlastkey() != KEY_ESC ? urlinp.getvalue() : "";

	if(!url.empty()) {
	    editor.setcoords(sizeWArea.x1+2, sizeWArea.y1+5, sizeWArea.x2, sizeWArea.y2);
	    editor.load(m->getdescription(), "");
	    editor.open();

	    r = editdone;

	    auto_ptr<char> p(editor.save("\r\n"));
	    *m = imurl(ev.getcontact(), imevent::outgoing, url, p.get());
	}

    } else if(ev.gettype() == imevent::sms) {
	imsms *m = static_cast<imsms *>(&ev);

	editor.setcoords(sizeWArea.x1+2, sizeWArea.y1+3, sizeWArea.x2, sizeWArea.y2);
	editor.load(m->gettext(), "");
	editor.open();

	r = editdone;

	auto_ptr<char> p(editor.save("\r\n"));
	*m = imsms(ev.getcontact(), imevent::outgoing, p.get());

	if(c = clist.get(ev.getcontact()))
	    c->setpostponed(r ? "" : p.get());

    }

    restoreworkarea();
    status("");
    return r;
}

void icqface::renderchathistory() {
    int count, chatmargin;
    string text;
    char buf[64];
    time_t t;

    vector<imevent *> events;
    typedef pair<imevent::imdirection, vector<string> > histentry;
    vector<histentry> toshow;
    vector<histentry>::iterator ir;
    vector<string>::reverse_iterator il;
    histentry h;

    icqcontact *c = clist.get(passinfo);

    c->sethistoffset(0);
    count = 0;
    events = em.getevents(passinfo, chatlastread);
    chatlastread = 0;

    if(events.size()) {
	c->setlastread(events.back()->gettimestamp());
    }

    while(events.size() > chatlines) {
	delete events.front();
	events.erase(events.begin());
    }

    while(events.size()) {
	if(count < chatlines) {
	    text = (string) time2str(&(t = events.back()->gettimestamp()), "DD.MM hh:mm", buf) + " ";
	    text += events.back()->gettext();
	    chatlastread = t-1;

	    h = histentry();
	    h.first = events.back()->getdirection();
	    breakintolines(text, h.second, sizeWArea.x2-sizeWArea.x1-2);
	    toshow.push_back(h);

	    count += h.second.size();
	}

	delete events.back();
	events.erase(events.end()-1);
    }

    chatmargin = sizeWArea.y1+chatlines;
    text = string(sizeWArea.x2-sizeWArea.x1-2, ' ');

    attrset(conf.getcolor(cp_main_text));
    for(count = 0; count < chatlines; count++)
	mvprintw(chatmargin-count, sizeWArea.x1+2, "%s", text.c_str());

    for(count = 0, ir = toshow.begin(); ir != toshow.end() && count < chatlines; ir++) {
	switch(ir->first) {
	    case imevent::incoming: attrset(conf.getcolor(cp_main_text)); break;
	    case imevent::outgoing: attrset(conf.getcolor(cp_main_highlight)); break;
	}

	for(il = ir->second.rbegin(); il != ir->second.rend() && count < chatlines; il++) {
	    kgotoxy(sizeWArea.x1+2, chatmargin-count);
	    printstring(*il);
	    count++;
	}
    }
}

void icqface::chat(const imcontact &ic) {
    texteditor editor;
    imevent *sendev;
    vector<imcontact>::iterator i;

    icqcontact *c = clist.get(ic);
    if(!c) return;

    saveworkarea();
    clearworkarea();

    chatlastread = 0;
    inchat = true;
    passinfo = ic;

    muins.clear();
    muins.push_back(passinfo);

    chatlines = (int) ((sizeWArea.y2-sizeWArea.y1)*0.75);
    workarealine(sizeWArea.y1+chatlines+1);

    editor.addscheme(cp_main_text, cp_main_text, 0, 0);
    editor.otherkeys = &editmsgkeys;
    editor.idle = &editchatidle;
    editor.wrap = true;
    editor.setcoords(sizeWArea.x1+2, sizeWArea.y1+chatlines+2, sizeWArea.x2, sizeWArea.y2);
    editor.load(c->getpostponed(), "");

    status(_("Ctrl-X send, Ctrl-P multiple, Ctrl-O history, Alt-? details, ESC cancel"));

    for(bool finished = false; !finished; ) {
	renderchathistory();

	editdone = false;
	editor.open();
	auto_ptr<char> p(editor.save("\r\n"));

	editor.close();
	editor.load("", "");

	if(editdone) {
	    auto_ptr<imevent> ev(new immessage(ic, imevent::outgoing, p.get()));

	    for(i = muins.begin(); i != muins.end(); i++) {
		ev.get()->setcontact(*i);
		em.store(*ev.get());
	    }

	    muins.clear();
	    muins.push_back(passinfo);

	} else {
	    finished = true;
	}

	c->setpostponed(editdone ? "" : p.get());
    }

    c->save();
    restoreworkarea();
    status("");
    inchat = false;
    update();
}

icqface::eventviewresult icqface::eventview(const imevent *ev, vector<eventviewresult> abuttons) {
    string title_event, title_timestamp, text;
    horizontalbar *bar;
    dialogbox db;
    int mitem, baritem;
    eventviewresult r;

    vector<eventviewresult> actions;
    vector<eventviewresult>::iterator ia;

    if(ev->gettype() == imevent::message) {
	actions.push_back(forward);

	if(ev->getdirection() == imevent::incoming) {
	    actions.push_back(reply);
	}

	actions.push_back(ok);

    } else if(ev->gettype() == imevent::url) {
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

    } else if(ev->gettype() == imevent::email) {
	actions.push_back(forward);
	actions.push_back(ok);

    }

    copy(abuttons.begin(), abuttons.end(), back_inserter(actions));
    text = ev->gettext();

    saveworkarea();
    clearworkarea();

    status(_("F2 to URLs, ESC close"));

    db.setwindow(new textwindow(sizeWArea.x1, sizeWArea.y1+3, sizeWArea.x2,
	sizeWArea.y2, conf.getcolor(cp_main_text), TW_NOBORDER));

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

    mainw.writef(sizeWArea.x1+2, sizeWArea.y1, conf.getcolor(cp_main_highlight),
	title_event.c_str(), eventnames[ev->gettype()], ev->getcontact().totext().c_str());

    mainw.writef(sizeWArea.x1+2, sizeWArea.y1+1, conf.getcolor(cp_main_highlight),
	title_timestamp.c_str(), strdateandtime(ev->gettimestamp()).c_str());

    db.addautokeys();
    db.otherkeys = &userinfokeys;
    db.idle = &dialogidle;
    db.redraw();
    db.getbrowser()->setbuf(text);

    extracturls(text);

    workarealine(sizeWArea.y1+3);
    workarealine(sizeWArea.y2-2);

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

	text = (string) + " " + time2str(&(t = ev.gettimestamp()), "DD.MM hh:mm", buf) + " ";
	text += ev.gettext();

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
	db.setwindow(new textwindow(sizeWArea.x1, sizeWArea.y1+2, sizeWArea.x2,
	    sizeWArea.y2, conf.getcolor(cp_main_text), TW_NOBORDER));

	db.setmenu(&mhist, false);

	/*
	*
	* Now set the menu position
	*
	*/

	for(k = 0; k < mhist.getcount(); k++) {
	    imevent *rim = static_cast<imevent *> (mhist.getref(k));
	    if(rim == im) {
		mhist.setpos(k);
		break;
	    }
	}

	db.idle = &dialogidle;
	db.otherkeys = &historykeys;

	saveworkarea();
	clearworkarea();

	db.redraw();
	workarealine(sizeWArea.y1+2);

	im = static_cast<imevent *> (mhist.getref(0));

	mainw.writef(sizeWArea.x1+2, sizeWArea.y1, conf.getcolor(cp_main_highlight),
	    _("History for %s, %d events total"),
	    im->getcontact().totext().c_str(), mhist.getcount());

	status(_("S search, L again, ESC cancel"));

	while(!fin) {
	    if(db.open(k)) {
		switch(k) {
		    case -2:
			sub = face.inputstr(_("search for: "), sub);
		    case -3:
			if(!sub.empty())
			for(k = mhist.getpos()+1; k < mhist.getcount(); k++) {
			    im = static_cast<imevent *> (mhist.getref(k));
			    if(im)
			    if(im->contains(sub)) {
				mhist.setpos(k);
				break;
			    }
			}
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

    if(face.dotermresize) {
	if(&m == &face.mcontacts->menu) {
	    face.done();
	    face.init();
	    face.draw();

	    /*
	     * Terminal resize specific
	     */

	    vector<string> flog;
	    vector<string>::iterator il;

	    face.status(face.fstatus);
	    flog = face.lastlog;
	    face.lastlog.clear();

	    for(il = flog.begin(); il != flog.end(); il++)
		face.log(*il);

	    face.dotermresize = false;
	}
    }
}

void icqface::dialogidle(dialogbox &caller) {
    cicq.idle();
}

void icqface::textbrowseridle(textbrowser &b) {
    cicq.idle();
}

int icqface::contactskeys(verticalmenu &m, int k) {
    icqcontact *c = NULL;
    int id, capab;
    id = face.mcontacts->getid(face.mcontacts->menu.getpos());
    if(id != -1 && ! face.mcontacts->isnode(id))
	c = (icqcontact *) face.mcontacts->getref(m.getpos()+1);

    face.extk = capab = 0;

    if(c)
    if(c->getdesc() != contactroot) {
	capab = gethook(c->getdesc().pname).getcapabilities();
    }

    switch(k) {
	case '?':
	    face.extk = ACT_INFO;
	    break;

	case KEY_DC:
	    face.extk = ACT_REMOVE;
	    break;

	case 'q':
	case 'Q':
	    face.extk = ACT_QUIT;
	    break;

	case 'u':
	case 'U':
	    if(capab & hoptCanSendURL)
		face.extk = ACT_URL;
	    break;

	case 's':
	case 'S':
	case KEY_F(3):
	    face.extk = ACT_STATUS;
	    break;

	case 'h':
	case 'H':
	    face.extk = ACT_HISTORY;
	    break;

	case 'a':
	case 'A': face.extk = ACT_ADD; break;

	case 'f':
	case 'F':
	    if(capab & hoptCanSendFile)
		face.extk = ACT_FILE;
	    break;

	case 'm':
	case 'M':
	case KEY_F(2):
	    face.extk = ACT_MENU;
	    break;

	case 'g':
	case 'G':
	case KEY_F(4):
	    face.extk = ACT_GMENU;
	    break;

	case KEY_F(5):
	    face.extk = ACT_HIDEOFFLINE;
	    break;

	case 'r':
	case 'R':
	    face.extk = ACT_RENAME;
	    break;

	case 'c':
	case 'C':
//            face.extk = ACT_CONTACT;
	    break;
	
	case ALT('s'):
	case '/': face.extk = ACT_QUICKFIND; break;
    }

    if(k && face.extk && (strchr("?rRqQsShHmMuUgGaAfFcC/", k)
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
	case '/':
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

int icqface::findreskeys(dialogbox &db, int k) {
    switch(k) {
	case ALT('s'):
	case '/':
	    return -3;
    }

    return -1;
}

void icqface::editidle(texteditor &e) {
    cicq.idle();
}

void icqface::editchatidle(texteditor &e) {
    icqcontact *c;

    cicq.idle(HIDL_SOCKEXIT);

    if(c = clist.get(face.passinfo))
    if(c->getmsgcount()) {
	face.renderchathistory();
    }
}

void icqface::textinputidle(textinputline &il) {
    cicq.idle();
}

void icqface::termresize(void) {
    face.dotermresize = true;
}

// ----------------------------------------------------------------------------

icqface::icqprogress::icqprogress() {
    w = 0;
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

    if(curline >= face.sizeDlg.height-1) {
	w->redraw();
	curline = 0;
    }

    w->write(2, 1+curline++, buf);
}

void icqface::icqprogress::show(const string &title = "") {
    if(!w) {
	w = new textwindow(0, 0, face.sizeDlg.width, face.sizeDlg.height,
	conf.getcolor(cp_dialog_frame), TW_CENTERED);
    }

    w->set_title(conf.getcolor(cp_dialog_highlight), title);
    w->open();

    curline = 0;
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
