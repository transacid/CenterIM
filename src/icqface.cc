/*
*
* centericq user interface class
* $Id: icqface.cc,v 1.249 2005/08/28 01:33:21 konst Exp $
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
#include "imlogger.h"
#include "irchook.h"
#include "imexternal.h"
#include "impgp.h"

const char *stryesno(bool b) {
    return b ? _("yes") : _("no");
}

const char *strgender(imgender g) {
    switch(g) {
	case genderMale: return _("Male");
	case genderFemale: return _("Female");
    }

    return _("Not specified");
}

const char *geteventviewresult(icqface::eventviewresult r) {
    switch(r) {
	case icqface::ok: return _("Ok");
	case icqface::next: return _("Next");
	case icqface::forward: return _("Fwd");
	case icqface::reply: return _("Reply");
	case icqface::open: return _("Open");
	case icqface::accept: return _("Accept");
	case icqface::reject: return _("Reject");
	case icqface::info: return _("User info");
	case icqface::add: return _("Add");
	case icqface::prev: return _("Prev");
    }

    return "";
}

const char *strregsound(icqconf::regsound s) {
    switch(s) {
	case icqconf::rscard: return _("sound card");
	case icqconf::rsspeaker: return _("speaker");
	case icqconf::rsdisable: return _("disable");
    }

    return _("don't change");
}

const char *strregcolor(icqconf::regcolor c) {
    switch(c) {
	case icqconf::rcdark: return _("dark");
	case icqconf::rcblue: return _("blue");
    }

    return _("don't change");
}

const char *strcolormode(icqconf::colormode cm) {
    switch(cm) {
	case icqconf::cmproto : return _("protocol");
	case icqconf::cmstatus : return _("status");
    }

    return _("don't change");
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
    switch(gmode) {
	case icqconf::group1: return _("mode 1");
	case icqconf::group2: return _("mode 2");
    }

    return _("no");
}

icqface::icqface() {
    workareas.freeitem = &freeworkareabuf;

    mainscreenblock = inited = onlinefolder = dotermresize =
	inchat = doredraw = false;

    mcontacts = 0;
}

icqface::~icqface() {
    kendinterface();

    if(inited) {
	for(int i = 0; i < LINES; i++) printf("\n");
    }

    if(conf.getdebug())
    if(flog.is_open())
	flog.close();
}

void icqface::init() {
    if(conf.getdebug())
    if(!flog.is_open()) {
	time_t logtime = time(0);

	flog.open((conf.getdirname() + "debug").c_str(), ios::app);
	if(flog.is_open())
	    flog << endl << "-- centericq debug log started on " << ctime(&logtime);
    }

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
    mvhline(LINES, 0, ' ', COLS);

    inited = true;
    kt_resize_event = &termresize;
}

void icqface::done() {
    kt_resize_event = 0;
    workareas.empty();
    xtermtitlereset();
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

    for(pname = icq; pname != protocolname_size; pname++) {
	ia = conf.getourid(pname);

	if(!ia.empty()) {
	    buf += " ";
	    buf += conf.getprotocolname(pname) + ":";
	    buf += imstatus2char[gethook(pname).getstatus()];
//            buf += ">";
	}
    }

    attrset(conf.getcolor(cp_status));
    mvhline(0, 0, ' ', COLS);
    mvprintw(0, 0, _(" CENTERICQ %s  UNSENT: %lu"), VERSION, em.getunsentcount());
    mvprintw(0, COLS-buf.size()-1, "%s", buf.c_str());
}

void icqface::update() {
    showtopbar();
    fillcontactlist();
    fneedupdate = false;
    if(doredraw) redraw();
}

int icqface::contextmenu(icqcontact *c) {
    int ret = 0, i;
    static int lastr = 0;
    imcontact cont;
    char buf[64];

    if(!c) return 0;

    verticalmenu m(conf.getcolor(cp_main_text),
	conf.getcolor(cp_main_selected));

    static map<int, string> actnames;

    if(actnames.empty()) {
	actnames[ACT_URL]       = getmenuitem(_("Send an URL"), 25, key_send_url, section_contact);
	actnames[ACT_SMS]       = _(" Send an SMS");
	actnames[ACT_CONTACT]   = getmenuitem(_("Send contacts"), 25, key_send_contact, section_contact);
	actnames[ACT_AUTH]      = _(" Request authorization");
	actnames[ACT_EDITUSER]  = getmenuitem(_("Edit details"), 25, key_edit, section_contact);
	actnames[ACT_FETCHAWAY] = getmenuitem(_("Fetch away message"), 25, key_fetch_away_message, section_contact);
	actnames[ACT_ADD]       = getmenuitem(_("Add to list"), 25, key_add, section_contact);
	actnames[ACT_RENAME]    = getmenuitem(_("Rename contact"), 25, key_rename, section_contact);
	actnames[ACT_GROUPMOVE] = _(" Move to group..");
	actnames[ACT_PING]      = _(" Ping");
	actnames[ACT_VERSION]   = getmenuitem(_("Fetch version info"), 25, key_rename, section_contact);
	actnames[ACT_FILE]      = _(" Send file(s)");
	actnames[ACT_CONFER]    = _(" Invite to conference..");
    }

    if(ischannel(c)) {
	m.setwindow(textwindow(sizeWArea.x1, sizeWArea.y1, sizeWArea.x1+33,
	    sizeWArea.y1+6, conf.getcolor(cp_main_text)));

	actnames[ACT_MSG]       = _(" Send a channel message   enter");
	actnames[ACT_HISTORY]   = getmenuitem(_("Channel chat history"), 25, key_history, section_contact);
	actnames[ACT_REMOVE]    = _(" Remove channel             del");
	actnames[ACT_JOIN]      = _(" Join channel");
	actnames[ACT_LEAVE]     = _(" Leave channel");
	actnames[ACT_INFO]      = getmenuitem(_("List nicknames"), 25, key_info, section_contact);

	if(lst.inlist(c, csignore))
	    actnames[ACT_IGNORE]    = getmenuitem(_("UnBlock channel messages"), 25, key_ignore, section_contact);
	else
	    actnames[ACT_IGNORE]    = getmenuitem(_("Block channel messages"), 25, key_ignore, section_contact);

    } else {
	m.setwindow(textwindow(sizeWArea.x1, sizeWArea.y1, sizeWArea.x1+27,
	    sizeWArea.y1+6, conf.getcolor(cp_main_text)));

	if(c->getdesc().pname != rss)
	    actnames[ACT_MSG] = getmenuitem(_("Send a message"), 25, key_compose, section_contact);

	actnames[ACT_HISTORY] = getmenuitem(_("Event history"), 25, key_history, section_contact);
	actnames[ACT_REMOVE] = getmenuitem(_("Remove user"), 25, key_remove, section_contact);
	actnames[ACT_INFO] = getmenuitem(_("User's details"), 25, key_info, section_contact);
	actnames[ACT_EXTERN] = getmenuitem(_("External actions.."), 25, key_contact_external_action, section_contact);

	actnames[ACT_IGNORE] = getmenuitem(lst.inlist(c, csignore) ?
	    _("Unset ignore user") : _("Ignore user"), 25, key_ignore,
		section_contact);

	actnames[ACT_PGPKEY] = c->getpgpkey().empty() ?
	    _(" Assign PGP key..") : _(" Unassign PGP key");

	sprintf(buf, _(" Turn PGP encryption %s"), c->getusepgpkey() ? "OFF" : "ON");
	actnames[ACT_PGPSWITCH] = buf;

	if(c->getdesc().pname == rss)
	    actnames[ACT_PING] = getmenuitem(_("Force check"), 25,
		key_rss_check, section_contact);

    }

    cont = c->getdesc();

    set<hookcapab::enumeration> capab = gethook(cont.pname).getCapabs();

    vector<int> actions;
    vector<int>::const_iterator ia;

    if(ischannel(cont)) {
	actions.push_back(ACT_MSG);
	actions.push_back(ACT_HISTORY);
	actions.push_back(ACT_REMOVE);
	actions.push_back(ACT_IGNORE);

	if(gethook(cont.pname).logged()) {
	    actions.push_back(ACT_INFO);
	    actions.push_back((c->getstatus() != offline) ? ACT_LEAVE : ACT_JOIN);
	}

	if(conf.getgroupmode() != icqconf::nogroups)
	    actions.push_back(ACT_GROUPMOVE);

    } else if(c->getdesc().pname != rss) {
	if(cont.pname != infocard) actions.push_back(ACT_MSG);
	if(capab.count(hookcapab::urls)) actions.push_back(ACT_URL);
	if(!conf.getourid(icq).empty()) actions.push_back(ACT_SMS);
	if(capab.count(hookcapab::contacts)) actions.push_back(ACT_CONTACT);
	if(capab.count(hookcapab::authrequests)) actions.push_back(ACT_AUTH);
	if(capab.count(hookcapab::files)) actions.push_back(ACT_FILE);

    #ifdef HAVE_GPGME
	if(capab.count(hookcapab::pgp)) {
	    actions.push_back(ACT_PGPKEY);
	    if(pgp.havekey(c->getpgpkey()))
		actions.push_back(ACT_PGPSWITCH);
	}
    #endif

	if(!actions.empty())
	    actions.push_back(0);

	actions.push_back(ACT_INFO);
	actions.push_back(ACT_EDITUSER);

	if(c->getstatus() != offline) {
	    if(capab.count(hookcapab::fetchaway)) actions.push_back(ACT_FETCHAWAY);
	    if(capab.count(hookcapab::version)) actions.push_back(ACT_VERSION);
	    if(capab.count(hookcapab::ping)) actions.push_back(ACT_PING);
	    if(capab.count(hookcapab::conferencesaretemporary)) actions.push_back(ACT_CONFER);
	}

	actions.push_back(ACT_HISTORY);
	actions.push_back(ACT_EXTERN);

	actions.push_back(0);
	actions.push_back(ACT_IGNORE);
	if(!c->inlist()) actions.push_back(ACT_ADD);

	actions.push_back(ACT_REMOVE);
	actions.push_back(ACT_RENAME);

	if(conf.getgroupmode() != icqconf::nogroups && c->inlist())
	    actions.push_back(ACT_GROUPMOVE);

    } else {
	actions.push_back(ACT_HISTORY);
	actions.push_back(ACT_REMOVE);
	actions.push_back(ACT_IGNORE);
	actions.push_back(ACT_INFO);
	actions.push_back(ACT_EDITUSER);
	actions.push_back(ACT_PING);

	if(conf.getgroupmode() != icqconf::nogroups)
	    actions.push_back(ACT_GROUPMOVE);

    }

    for(ia = actions.begin(); ia != actions.end(); ++ia) {
	if(*ia) {
	    m.additem(0, *ia, actnames[*ia].c_str());
	    if(*ia == lastr) m.setpos(m.getcount()-1);
	} else {
	    m.addline();
	}
    }

    m.scale();
    m.idle = &menuidle;
    i = (int) m.getref(m.open()-1);
    m.close();

    if(i) lastr = i;
    return i;
}

int icqface::generalmenu() {
    int i, r = 0;
    static int lastitem = 0;

    verticalmenu m(conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));
    m.setwindow(textwindow(sizeWArea.x1, sizeWArea.y1, sizeWArea.x1+40, sizeWArea.y1+11, conf.getcolor(cp_main_text)));

    m.idle = &menuidle;
    m.additem(0, ACT_STATUS,    getmenuitem(_("Change status"), 38, key_change_status, section_contact));
    m.additem(0, ACT_QUICKFIND, getmenuitem(_("Go to contact.."), 38, key_quickfind, section_contact));
    m.additem(0, ACT_DETAILS,   _(" Accounts.."));
    m.additem(0, ACT_CONF,      _(" CenterICQ config options"));
    m.additem(0, ACT_TRANSFERS, _(" File transfers monitor"));
    m.addline();
    m.additem(0, ACT_FIND,      _(" Find/add users"));
    m.additem(0, ACT_JOINDIALOG, _(" Join channel/conference"));
#ifdef BUILD_RSS
    m.additem(0, ACT_RSS,       _(" Link an RSS feed"));
#endif
    m.addline();
    m.additem(0, ACT_IGNORELIST,    _(" View/edit ignore list"));
    m.additem(0, ACT_INVISLIST,     _(" View/edit invisible list"));
    m.additem(0, ACT_VISIBLELIST,   _(" View/edit visible list"));
    m.addline();
    if(conf.gethideoffline())
	m.additem(0, ACT_HIDEOFFLINE, getmenuitem(_("Show offline users"), 38, key_hide_offline, section_contact));
    else
	m.additem(0, ACT_HIDEOFFLINE, getmenuitem(_("Hide offline users"), 38, key_hide_offline, section_contact));

    if(conf.getgroupmode() != icqconf::nogroups) {
	m.additem(0, ACT_ORG_GROUPS, _(" Organize contact groups"));
	m.additem(0, ACT_MASS_MOVE, _(" Mass group move.."));
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

/* called to prepare the next chat contact, returns true if it found one, false otherwise
   parameter is true if its next chat contact or false if is the previous
 */
bool icqface::next_chat(bool next) {
  find_next_action = (next ? 1:-1);
  if (last_selected) last_selected->setopenedforchat(false);
  return true;
}

icqcontact *icqface::find_next_chat() {
  if (find_next_action == 0) return 0;
  int i;
  if (last_selected){
    for(i = 0; i < clist.count && clist.at(i) != last_selected; i++)
      ;
    i += find_next_action;
  }
  else i = find_next_action == 1 ? 0 : clist.count -1;

  icqcontact *c;
  for (; i < clist.count && i >= 0; i += find_next_action){
    c = (icqcontact *) clist.at(i);
    if (c->isopenedforchat() || c->hasevents()) {
      find_next_action = 0;
      extk = ACT_MSG;
      return c;
    }
  }
  
  find_next_action = 0;
  return 0;
}

icqcontact *icqface::mainloop(int &action) {
    int i, curid;
    icqcontact *c = 0, *c1 = 0; icqgroup *g;
    bool fin;

    for(fin = false; !fin; ) {
	extk = ACT_MSG;

	/* Obtain the (icqcontact *) from the treeview. If a node is
	   selected, throw out the contact and obtain the correct (icqgroup *). */
	
	if (!(c = c1 = find_next_chat())){ //check if next_chat was called from inside a chat window
	    last_selected = 0; // next_chat (if called) was called from contacts menu so there was no last selected contact
	    c = (icqcontact *) mcontacts->open(&i);
	    if ((c1 = find_next_chat())) c = c1; //check if next_chat was called from contacts menu
	}

	if(!c1 && c && mcontacts->isnode(i)) {
	    c = 0;
	    g = (icqgroup *) mcontacts->getref(mcontacts->getid(mcontacts->menu.getpos()));
	} else {
	    g = 0;
	}
	
	if((unsigned int) c < 100) c = 0;

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
		if(action == ACT_MSG && !c->hasevents()) {
		    switch(c->getdesc().pname) {
			case infocard: action = ACT_SMS; break;
			case livejournal: action = ACT_LJ; break;
		    }
		}

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
    if (c) last_selected = c;
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

    bool online_added, prevoffline, grouponline, groupchange,
	ontop, iscurrentnode, birthday;

    time_t timer = time(0);

    grouponline = true;
    online_added = prevoffline = false;
    nnode = ngroup = prevgid = 0;

    iscurrentnode = mcontacts->isnode(mcontacts->getid(mcontacts->menu.getpos()));
    ontop = !mcontacts->menu.getpos() && iscurrentnode;

    savec = mcontacts->getref(mcontacts->getid(mcontacts->menu.getpos()));

    int ccolor = conf.getcolor(cp_main_text);

    if(!iscurrentnode && savec) {
	ig = find(groups.begin(), groups.end(), ((icqcontact *) savec)->getgroupid());

	if(ig != groups.end()) {
	    g = &(*ig);
	    if(g->iscollapsed() && !((icqcontact *) savec)->hasevents())
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

	c->remindbirthday(birthday = c->isbirthday());

	abstracthook &h = gethook(c->getdesc().pname);

	if(c->getstatus() == offline)
	if(conf.gethideoffline())
	if(!c->hasevents())
	if(c->inlist() || !h.logged() || h.getCapabs().count(hookcapab::cltemporary)) {
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

	if(birthday) dnick += " :)";

	if(conf.getgroupmode() != icqconf::nogroups && g->iscollapsed() &&
	    !c->hasevents() && sc != '!')
		continue;

	if(conf.getcolormode() == icqconf::cmstatus) {
	    ccolor = conf.getstatuscolor(c->getstatus());
	} else {
	    ccolor = conf.getprotcolor(c->getdesc().pname);
	}

	if(c->getstatus() == offline) {
	    mcontacts->addleaff(nnode,
		c->hasevents() ? conf.getcolor(cp_main_highlight) : ccolor,
		c, "%s%s ", c->hasevents() ? "#" : c->getpostponed().empty() ? (c->isopenedforchat() ? "*" : " ") : ">", dnick.c_str());

	} else {
	    char *fmt = "%s[%c]%s%s ";

	    if(lst.inlist(c->getdesc(), csvisible)) fmt = "%s<%c>%s%s "; else
	    if(lst.inlist(c->getdesc(), csinvisible)) fmt = "%s{%c}%s%s ";

	    char shortstatus = c->getshortstatus();

	    if(c->getlasttyping()) {
		if(timer-c->getlasttyping() > PERIOD_TYPING) c->setlasttyping(0);
		    else shortstatus = 'T';
	    }

	    mcontacts->addleaff(nnode,
		c->hasevents() ? conf.getcolor(cp_main_highlight) : ccolor,
		c, fmt, c->hasevents() ? "#" : (c->isopenedforchat() ? "*" : " "), shortstatus,
		c->getpostponed().empty() ? " " : ">", dnick.c_str());

	}
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

bool icqface::findresults(const imsearchparams &sp, bool fauto) {
    bool finished = false, ret = false;
    unsigned int ffuin;
    icqcontact *c;
    dialogbox db;
    int r, b;
    char *nick;

    saveworkarea();
    clearworkarea();

    abstracthook &h = gethook(sp.pname);

    db.setwindow(new textwindow(sizeWArea.x1+1, sizeWArea.y1+2, sizeWArea.x2,
	sizeWArea.y2, conf.getcolor(cp_main_text), TW_NOBORDER));
    db.setmenu(new verticalmenu(conf.getcolor(cp_main_menu),
	conf.getcolor(cp_main_selected)));

    if(!h.getCapabs().count(hookcapab::nochat)) {
	db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	    conf.getcolor(cp_main_selected), _("Details"), _("Message"),
	    _("Add"), fauto ? 0 : _("New search"), 0));
    } else {
	db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	    conf.getcolor(cp_main_selected), _("Details"), _("Add"),
	    fauto ? 0 : _("New search"), 0));
    }

    db.addautokeys();
    db.redraw();

    if(!sp.nick.empty() && h.getCapabs().count(hookcapab::directadd)) {
	imcontact ic(sp.nick, sp.pname);
	static icqcontact *singlec = 0, *c;

	delete singlec;
	singlec = 0;

	mainw.write(sizeWArea.x1+2, sizeWArea.y1,
	    conf.getcolor(cp_main_highlight), _("Contact to add"));

	c = clist.get(ic);
	if(!c) c = singlec = new icqcontact(ic);

	db.getmenu()->additem(conf.getprotcolor(sp.pname),
	    c, (string) " " + sp.nick);

    } else {
	mainw.write(sizeWArea.x1+2, sizeWArea.y1,
	    conf.getcolor(cp_main_highlight), _("Searching contacts.."));

	h.lookup(sp, *db.getmenu());

    }

    db.idle = &dialogidle;
    db.otherkeys = &findreskeys;

    workarealine(sizeWArea.y1+2);
    workarealine(sizeWArea.y2-2);

    while(!finished) {
	finished = !db.open(r, b);
	h.stoplookup();

	if(h.getCapabs().count(hookcapab::nochat) && b > 0)
	    b++;

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
			if(c = (icqcontact *) db.getmenu()->getref(r-1)) {
			    imcontact cont = c->getdesc();
			    existed = (c = clist.get(cont));
			    if(!existed) c = clist.addnew(cont);

			    if(c)
			    if(!cicq.sendevent(immessage(cont, imevent::outgoing, ""), icqface::ok))
			    if(!existed)
				clist.remove(cont);
			}
		    }
		    break;

		case 2:
		    if(r) {
			c = (icqcontact *) db.getmenu()->getref(r-1);
			if(c) cicq.addcontact(c->getdesc(), c->getbasicinfo().requiresauth);
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

void icqface::findready() {
    mvhline(sizeWArea.x1+2, sizeWArea.y1, ' ', sizeWArea.x2-sizeWArea.x1-2);
    mainw.write(sizeWArea.x1+2, sizeWArea.y1,
	conf.getcolor(cp_main_highlight), _("Search results [done]"));
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

void icqface::infolivejournal(dialogbox &db, icqcontact *c) {
    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();

    workarealine(sizeWArea.y1+5);
    workarealine(sizeWArea.y1+7);

    mainw.write(sizeWArea.x1+2, sizeWArea.y1+2, conf.getcolor(cp_main_highlight), _("Nickname"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+3, conf.getcolor(cp_main_highlight), _("Name"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+5, conf.getcolor(cp_main_highlight), _("URL"));

    mainw.write(sizeWArea.x1+14, sizeWArea.y1+2, conf.getcolor(cp_main_text), c->getnick());
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+3, conf.getcolor(cp_main_text), bi.fname + " " + bi.lname);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+5, conf.getcolor(cp_main_text), mi.homepage);

    mainw.write(sizeWArea.x1+2, sizeWArea.y1+7, conf.getcolor(cp_main_highlight), _("This is your LiveJournal account. Use it to"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+8, conf.getcolor(cp_main_highlight), _("post new entries to your journal."));
}

void icqface::infofriends(dialogbox &db, icqcontact *c) {
    string text, buf;
    icqcontact::basicinfo bi = c->getbasicinfo();

    text += (string) "* " + _("The following users have you listed as their friend:") + " ";
    while(!(buf = getword(bi.zip)).empty()) {
	text += buf;
	if(!bi.zip.empty()) text += ", ";
    }

    db.getbrowser()->setbuf(text);
}

void icqface::infogeneral(dialogbox &db, icqcontact *c) {
    string langs, options, buf;
    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();

    workarealine(sizeWArea.y1+8);
    workarealine(sizeWArea.y1+12);

#ifdef HAVE_GPGME
    if(gethook(c->getdesc().pname).getCapabs().count(hookcapab::pgp))
    if(!c->getpgpkey().empty()) {
	buf = (string) "[" + string(5, ' ') + c->getpgpkey() + ": ";
	
	if(pgp.havekey(c->getpgpkey())) {
	    buf += _("found");
	    buf += ", ";
	    buf += (pgp.enabled(c) ? _("used") : _("not used"));
	} else {
	    buf += _("not found");
	}

	buf += " ]";

	mainw.write(sizeWArea.x2-buf.size()-1, sizeWArea.y1+11,
	    conf.getcolor(cp_main_text), buf);

	mainw.write(sizeWArea.x2-buf.size()+1, sizeWArea.y1+11,
	    conf.getcolor(cp_main_highlight), "PGP");
    }
#endif

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

    mainw.write(sizeWArea.x1+14, sizeWArea.y1+8, conf.getcolor(cp_main_text), strgender(mi.gender));
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+9, conf.getcolor(cp_main_text), mi.strbirthdate());
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+10, conf.getcolor(cp_main_text), mi.age ? i2str(mi.age) : "");

    if(mi.lang1) commacat(langs, abstracthook::getLanguageIDtoString(mi.lang1), langs.empty());
    if(mi.lang2) commacat(langs, abstracthook::getLanguageIDtoString(mi.lang2), langs.empty());
    if(mi.lang3) commacat(langs, abstracthook::getLanguageIDtoString(mi.lang3), langs.empty());
    commaform(langs);

    mainw.write(sizeWArea.x1+14, sizeWArea.y1+12, conf.getcolor(cp_main_text), langs);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+13, conf.getcolor(cp_main_text), c->getlastip());

    time_t ls = c->getlastseen();
    buf = "";

    if(c->getstatus() == offline) {
	mainw.write(sizeWArea.x1+2, sizeWArea.y1+14, conf.getcolor(cp_main_highlight), _("Last seen"));
	mainw.write(sizeWArea.x1+14, sizeWArea.y1+14, conf.getcolor(cp_main_text),
	    ls ? strdateandtime(ls) : _("Never"));

    } else if(c->getdesc() != contactroot) {
	int days, hours, minutes, tdiff = timer_current-ls;

	days = (int) (tdiff/86400);
	hours = (int) ((tdiff-days*86400)/3600);
	minutes = (int) ((tdiff-days*8600-hours*3600)/60);

	if(days) buf = i2str(days) + " " + _("days");

	if(hours) {
	    if(!buf.empty()) buf += ", ";
	    buf += i2str(hours) + " " + _("hours");
	}

	if(minutes) {
	    if(!buf.empty()) buf += ", ";
	    buf += i2str(minutes) + " " + _("min");
	}

	if(buf.empty())
	    buf = i2str(tdiff) + " " + _("seconds");

	mainw.write(sizeWArea.x1+2, sizeWArea.y1+14, conf.getcolor(cp_main_highlight), _("Online"));
	mainw.write(sizeWArea.x1+14, sizeWArea.y1+14, conf.getcolor(cp_main_text), buf);
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
	bi.city += abstracthook::getCountryIDtoString(bi.country);
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
	wi.city += abstracthook::getCountryIDtoString(wi.country);
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

	for(i = data.begin(); i != data.end(); ++i) {
	    text += " + " + *i + "\n";
	}

	text += "\n";
    }

    data = c->getbackground();

    if(!data.empty()) {
	text += (string) "* " + _("Background") + "\n";

	for(i = data.begin(); i != data.end(); ++i)
	    text += " + " + *i + "\n";
    }

    commaform(text);
    db.getbrowser()->setbuf(text);
}

void icqface::inforss(dialogbox &db, icqcontact *c) {
    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();
    icqcontact::workinfo wi = c->getworkinfo();

    workarealine(sizeWArea.y1+7);
    workarealine(sizeWArea.y1+10);

    mainw.write(sizeWArea.x1+2, sizeWArea.y1+2, conf.getcolor(cp_main_highlight), _("Handle"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+3, conf.getcolor(cp_main_highlight), _("RSS doc"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+4, conf.getcolor(cp_main_highlight), _("Version"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+5, conf.getcolor(cp_main_highlight), _("Encoding"));

    mainw.write(sizeWArea.x1+2, sizeWArea.y1+7, conf.getcolor(cp_main_highlight), _("Title"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+8, conf.getcolor(cp_main_highlight), _("Link"));

    mainw.write(sizeWArea.x1+2, sizeWArea.y1+10, conf.getcolor(cp_main_highlight), _("Frequency"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+11, conf.getcolor(cp_main_highlight), _("Last check"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+12, conf.getcolor(cp_main_highlight), _("Next check"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+13, conf.getcolor(cp_main_highlight), _("Last result"));

    mainw.write(sizeWArea.x1+14, sizeWArea.y1+2, conf.getcolor(cp_main_text), c->getnick());
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+3, conf.getcolor(cp_main_text), wi.homepage);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+4, conf.getcolor(cp_main_text), bi.city);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+5, conf.getcolor(cp_main_text), bi.state);

    mainw.write(sizeWArea.x1+14, sizeWArea.y1+7, conf.getcolor(cp_main_text), bi.fname);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+8, conf.getcolor(cp_main_text), mi.homepage);

    string freq, last, next;
    char buf[512];

    if(mi.checkfreq) {
	sprintf(buf, _("%lu minutes"), mi.checkfreq);
	freq = buf;
	if(mi.checklast) next = strdateandtime(mi.checklast+mi.checkfreq*60);
	    else next = _("Now");
    } else {
	freq = next = _("Never");
    }

    if(mi.checklast) last = strdateandtime(mi.checklast);
	else last = _("Never");

    mainw.write(sizeWArea.x1+14, sizeWArea.y1+10, conf.getcolor(cp_main_text), freq);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+11, conf.getcolor(cp_main_text), last);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+12, conf.getcolor(cp_main_text), next);
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+13, conf.getcolor(cp_main_text), bi.lname);
}

void icqface::infoljrss(dialogbox &db, icqcontact *c) {
    icqcontact::basicinfo bi = c->getbasicinfo();
    icqcontact::moreinfo mi = c->getmoreinfo();

    mainw.write(sizeWArea.x1+2, sizeWArea.y1+2, conf.getcolor(cp_main_highlight), _("Birthdate"));
    mainw.write(sizeWArea.x1+2, sizeWArea.y1+3, conf.getcolor(cp_main_highlight), _("Full name"));

    mainw.write(sizeWArea.x1+14, sizeWArea.y1+2, conf.getcolor(cp_main_text), mi.strbirthdate());
    mainw.write(sizeWArea.x1+14, sizeWArea.y1+3, conf.getcolor(cp_main_text), bi.email);
}

void icqface::userinfo(const imcontact &cinfo, const imcontact &realinfo) {
    bool finished = false, showinfo;
    textbrowser tb(conf.getcolor(cp_main_text));
    dialogbox db;
    int k, lastb, b;

    imcontact savepassinfo = passinfo;
    icqcontact *c = clist.get(passinfo = realinfo);

    b = lastb = 0;
    saveworkarea();
    clearworkarea();

    status(_("%s to URLs, %s external actions, %s close"),
	getstatkey(key_show_urls, section_info).c_str(),
	getstatkey(key_user_external_action, section_info).c_str(),
	getstatkey(key_quit, section_editor).c_str());

    xtermtitle(_("user info for %s"), realinfo.totext().c_str());

    db.setwindow(new textwindow(sizeWArea.x1+1, sizeWArea.y1+2, sizeWArea.x2,
	sizeWArea.y2, conf.getcolor(cp_main_text), TW_NOBORDER));

    if(cinfo.pname == rss) {
	if(c->inlist() && realinfo != contactroot) {
	    db.setbar(new horizontalbar(sizeWArea.x1+2, sizeWArea.y2-1,
		conf.getcolor(cp_main_highlight), conf.getcolor(cp_main_selected),
		_("Info"), _("About"), _("Check"), _("Edit"), islivejournal(c) ? _("LJ") : 0, 0));
	} else {
	    db.setbar(new horizontalbar(sizeWArea.x1+2, sizeWArea.y2-1,
		conf.getcolor(cp_main_highlight), conf.getcolor(cp_main_selected),
		_("Info"), _("About"), _("Retreive"), 0));
	}

    } else if(cinfo.pname == livejournal) {
	db.setbar(new horizontalbar(sizeWArea.x1+2, sizeWArea.y2-1,
	    conf.getcolor(cp_main_highlight), conf.getcolor(cp_main_selected),
	    _("Info"), _("Friend of"), 0));


    } else {
	db.setbar(new horizontalbar(sizeWArea.x1+2, sizeWArea.y2-1,
	    conf.getcolor(cp_main_highlight), conf.getcolor(cp_main_selected),
	    _("Info"), _("Home"), _("Work"), _("More"), _("About"),
	    cinfo.pname != infocard ? _("Retrieve") : _("Edit"), 0));

    }

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

	    if(cinfo.pname == rss) switch(b) {
		case 0: inforss(db, c); break;
		case 1: 
		    db.setbrowser(&tb, false);
		    infoabout(db, c);
		    infoclear(db, c, cinfo);
		    break;
		case 2:
		    gethook(cinfo.pname).ping(c);
		    b = lastb;
		    continue;
		case 3:
		    updatedetails(c, c->getdesc().pname);
		    showinfo = true;
		    b = lastb;
		    continue;
		case 4: infoljrss(db, c); break;

	    } else if(cinfo.pname == livejournal) switch(b) {
		case 0: infolivejournal(db, c); break;
		case 1:
		    db.setbrowser(&tb, false);
		    infofriends(db, c);
		    infoclear(db, c, cinfo);
		    break;

	    } else switch(b) {
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
    status("@");
    passinfo = savepassinfo;
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
	_(" [jab] Jabber"),
	"",
	_(" [lj] LiveJournal"),
	_(" [gg] Gadu-Gadu"),
	""
    };

    for(ipname = icq; ipname != protocolname_size; ipname++) {
	ia = conf.getourid(ipname);

	if(!ia.empty()) {
	    m.additem(0, ipname, pitems[ipname]);
	}
    }
}

bool icqface::changestatus(vector<protocolname> &pnames, imstatus &st) {
    int i, choice, protcount;
    bool r, alrlogged = false;
    verticalmenu m(conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));

    vector<imstatus> mst;
    vector<imstatus>::iterator im;
    protocolname pname, onechoice;

    m.setwindow(textwindow(sizeWArea.x1, sizeWArea.y1, sizeWArea.x1+27,
	sizeWArea.y1+9, conf.getcolor(cp_main_text)));

    m.idle = &menuidle;

    for(protcount = 0, pname = icq; pname != protocolname_size; pname++) {
	if(!conf.getourid(pname).empty()) {
	    protcount++;
	    onechoice = pname;
	}

	alrlogged = alrlogged || gethook(pname).getstatus() != offline;
    }

    if(protcount < 2) {
	i = 1;
	choice = onechoice;

    } else {
	m.additem(0, -1, _(" All protocols"));
	if(alrlogged) m.additem(0, -2, _(" Already logged in only"));
	m.addline();
	makeprotocolmenu(m);
	m.scale();

	i = m.open();
	choice = (int) m.getref(i-1);

	m.close();
    }

    if(r = i) {
	switch(choice) {
	    case -1:
		for(pname = icq; pname != protocolname_size; pname++)
		    if(!conf.getourid(pname).empty())
			pnames.push_back(pname);
		break;
	    case -2:
		for(pname = icq; pname != protocolname_size; pname++)
		    if(!conf.getourid(pname).empty())
		    if(gethook(pname).getstatus() != offline)
			pnames.push_back(pname);
		break;
	    default:
		pnames.push_back((protocolname) choice);
		break;
	}

	m.clear();

	mst.push_back(available);
	mst.push_back(offline);
	mst.push_back(away);
	mst.push_back(dontdisturb);
	mst.push_back(notavail);
	mst.push_back(occupied);
	mst.push_back(freeforchat);
	mst.push_back(invisible);

	for(im = mst.begin(); im != mst.end(); ++im) {
	    string sst = (string) " [" + imstatus2char[*im] + "] " + imstatus2str(*im);
	    m.additem(0, *im, sst);

	    if(pnames.size() == 1)
	    if(gethook(pnames.front()).getstatus() == *im)
		m.setpos(m.getcount()-1);
	}

	if(pnames.size() > 1) {
	    m.setpos(0);
	}

	m.otherkeys = &statuskeys;

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

string icqface::inputstr(const string &q, const string &defl, char passwdchar) {
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

string icqface::inputfile(const string &q, const string &defl) {
    screenarea sa(0, INPUT_POS, COLS, INPUT_POS);
    string r;

    mvhline(INPUT_POS, 0, ' ', COLS);
    kwriteatf(0, INPUT_POS, conf.getcolor(cp_status), "%s", q.c_str());
    kwriteatf(COLS-8, INPUT_POS, conf.getcolor(cp_status), "[Ctrl-T]");

    selector.setoptions(0);
    selector.setwindow(textwindow(0, 0, sizeDlg.width, sizeDlg.height,
	conf.getcolor(cp_dialog_frame), TW_CENTERED,
	conf.getcolor(cp_dialog_highlight),
	_(" <Space> or <Enter> confirm, <Esc> cancel ")));

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

string icqface::inputdir(const string &q, const string &defl) {
    screenarea sa(0, INPUT_POS, COLS, INPUT_POS);
    string r;

    mvhline(INPUT_POS, 0, ' ', COLS);
    kwriteatf(0, INPUT_POS, conf.getcolor(cp_status), "%s", q.c_str());
    kwriteatf(COLS-8, INPUT_POS, conf.getcolor(cp_status), "[Ctrl-T]");

    selector.setoptions(FSEL_DIRSELECT);
    selector.setwindow(textwindow(0, 0, sizeDlg.width, sizeDlg.height,
	conf.getcolor(cp_dialog_frame), TW_CENTERED,
	conf.getcolor(cp_dialog_highlight),
	_(" <Space> confirm, <Esc> cancel ")));

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

int icqface::ask(string q, int options, int deflt ) {
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

void icqface::workarealine(int l, chtype c ) {
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

    db.setwindow(new textwindow(sizeWArea.x1+1, sizeWArea.y1+2, sizeWArea.x2,
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

    set<protocolname> ps;
    if(cs == csvisible || cs == csinvisible) {
	for(protocolname pname = icq; pname != protocolname_size; pname++)
	    if(gethook(pname).getCapabs().count(hookcapab::visibility))
		ps.insert(pname);
    }

    lst.fillmenu(db.getmenu(), cs);

    while(db.open(i, b)) {
	if(!db.getmenu()->getcount() && b != 1) continue;
	if(b != 1) it = lst.menuat(i-1);

	switch(b) {
	    case 0:
		cicq.userinfo(it.getdesc());
		break;
	    case 1:
		muins.clear();

		if(multicontacts(_("Select contacts to add to the list"), ps, cs)) {
		    bool removecl;

		    if(cs == csignore) {
			char buf[512];
			sprintf(buf, _("Remove the %d contacts from the contact list as well?"), muins.size());
			removecl = face.ask(buf, ASK_YES | ASK_NO, ASK_NO) == ASK_YES;
		    }

		    for(ic = muins.begin(); ic != muins.end(); ++ic) {
			lst.push_back(modelistitem(clist.get(*ic)->getdispnick(), *ic, cs));

			if(removecl && cs == csignore)
			    clist.remove(*ic);
		    }

		    lst.fillmenu(db.getmenu(), cs);
		    db.getmenu()->redraw();
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
		}
		break;
	}

	face.relaxedupdate();
    }

    db.close();
    restoreworkarea();
}

bool icqface::multicontacts(const string &ahead,
const set<protocolname> &protos, contactstatus cs) {

    int i, savefirst, saveelem, prevgid;
    bool ret = true, finished = false;
    string head = ahead;

    imcontact ic;
    vector<imcontact>::iterator c, cc;
    vector<imcontact> mlst;

    map<int, bool> groupcheck;
    icqcontact *cont;

    verticalmenu m(sizeWArea.x1+1, sizeWArea.y1+3, sizeWArea.x2, sizeWArea.y2,
	conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));

    saveworkarea();
    clearworkarea();

    workarealine(sizeWArea.y1+2);

    if(!head.size()) head = _("Event recipients");
    mainw.write(sizeWArea.x1+2, sizeWArea.y1, conf.getcolor(cp_main_highlight), head);

    vector<icqgroup>::iterator ig = groups.begin();

    while(ig != groups.end()) {
	for(i = 0; i < clist.count; i++) {
	    icqcontact *c = (icqcontact *) clist.at(i);
	    imcontact desc = c->getdesc();

	    if(!desc.empty())
	    if(c->getgroupid() == ig->getid() || conf.getgroupmode() == icqconf::nogroups)
	    if(protos.empty() || protos.count(desc.pname))
	    if(!lst.inlist(desc, cs))
		mlst.push_back(desc);
	}

	ig++;
    }

    m.idle = &menuidle;
    m.otherkeys = &multiplekeys;

    while(!finished) {
	m.getpos(saveelem, savefirst);
	m.clear();

	int prevgid = -1;

	for(c = mlst.begin(); c != mlst.end(); ++c) {
	    cont = (icqcontact *) clist.get(*c);

	    if(conf.getgroupmode() != icqconf::nogroups)
	    if(cont->getgroupid() != prevgid) {
		prevgid = cont->getgroupid();

		if(groupcheck.find(prevgid) == groupcheck.end())
		    groupcheck[prevgid] = false;

		m.additemf(conf.getcolor(cp_main_highlight), 0, "\002[%c]\002\002 %s",
		    groupcheck[prevgid] ? 'x' : ' ',
		    groups.getname(prevgid).c_str());
	    }

	    m.additemf(conf.getprotcolor(c->pname), cont, " [%c] %s",
		find(muins.begin(), muins.end(), *c) != muins.end() ? 'x' : ' ',
		cont->getdispnick().c_str());
	}

	m.setpos(saveelem, savefirst);

	switch(m.open()) {
	    case -2:
		if(m.getref(m.getpos())) {
		    ic = ((icqcontact *) m.getref(m.getpos()))->getdesc();
		    c = find(muins.begin(), muins.end(), ic);

		    if(c != muins.end()) muins.erase(c);
			else muins.push_back(ic);

		    cont = clist.get(ic);
		    groupcheck[cont->getgroupid()] = false;

		} else {
		    bool been = false;
		    int gid = ((icqcontact *) m.getref(m.getpos()+1))->getgroupid();
		    groupcheck[gid] = !groupcheck[gid];

		    for(c = mlst.begin(); c != mlst.end(); ++c) {
			cont = (icqcontact *) clist.get(*c);

			if(cont->getgroupid() == gid) {
			    been = true;
			    cc = find(muins.begin(), muins.end(), *c);

			    if(groupcheck[gid] && cc == muins.end()) {
				muins.push_back(*c);
			    } else if(!groupcheck[gid] && cc != muins.end()) {
				muins.erase(cc);
			    }
			} else if(been) break;
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

    if(conf.getdebug())
    if(flog.is_open())
	flog << text << endl;

    /*! Add a timestamp if needed
     */

    bool lts, lo;
    conf.getlogoptions(lts, lo);

    if(lts)
    if(text.size() > 3)
    if(ispunct(text[0]) && isspace(text[1]) && !isspace(text[2])) {
	time_t t;
	char stime[64];
	time(&t);
	strftime(stime, 64, "%R ", localtime(&t));
	text.insert(2, stime);
    }

    while((i = text.find("\n")) != -1) text[i] = ' ';
    while((i = text.find("\r")) != -1) text[i] = ' ';

    while((lastlog.size() > LINES-sizeWArea.y2-2) && !lastlog.empty())
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
    string t(text);

    if(t == "@") {
	if(!fstatus.empty()) fstatus.pop_back();
	if(!fstatus.empty()) t = fstatus.back();

    } else if(t == "#") {
	if(!fstatus.empty()) t = fstatus.back();

    } else {
	fstatus.push_back(t);
	while(fstatus.size() > 5)
	    fstatus.erase(fstatus.begin());

    }

    attrset(conf.getcolor(cp_status));
    mvhline(LINES-1, 0, ' ', COLS);
    kwriteatf(0, LINES-1, conf.getcolor(cp_status), "%s", t.c_str());
}

void icqface::status(const char *fmt, ...) {
    va_list ap;
    char buf[1024];

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    status((string) buf);
    va_end(ap);
}

void icqface::blockmainscreen() {
    mainscreenblock = true;
}

void icqface::unblockmainscreen() {
    mainscreenblock = false;
    update();
}

void icqface::quickfind(verticalmenu *multi) {
    bool fin;
    string nick, disp, upnick, upcurrent;
    string::iterator is;
    int k, i, len, lx, ly;
    bool found;
    icqcontact *c;
    verticalmenu *cm = (multi ? multi : &mcontacts->menu);

    status(_("QuickSearch: type to find, %s find again, Enter finish"),
	getstatkey(key_quickfind, section_contact).c_str());

    xtermtitle(_("contact list quick search"));

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
		if(isprint(k) || (key2action(k, section_contact) == key_quickfind)) {
		    i = cm->getpos() + (multi ? 1 : 2);

		    if(isprint(k)) {
			i--;
			nick += k;
		    }

		    for(is = nick.begin(), upnick = ""; is != nick.end(); ++is)
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

			if((unsigned int) c > 100) {
			    string current = c->getdispnick();
			    len = current.size();
			    if(len > nick.size()) len = nick.size();
			    current.erase(len);

			    for(is = current.begin(), upcurrent = "";
			    is != current.end(); ++is)
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
    if(!regcomp(&r, "(http://[^ \t\n]+|https://[^ \t\n]+|ftp://[^ \t\n]+|www\\.[^ \t\n]+)", REG_EXTENDED)) {
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

	for(i = extractedurls.begin(); i != extractedurls.end(); ++i)
	    m.additem(" " + *i);

	if(n = m.open())
	    conf.execaction("openurl", extractedurls[n-1]);

	restoreworkarea();
    }
}

void icqface::userinfoexternal(const imcontact &ic) {
    vector<pair<int, string> > r = external.getlist(imexternal::aomanual, ic.pname);

    if(r.empty()) {
	log(_("+ no external actions defined for %s"), conf.getprotocolname(ic.pname).c_str());

    } else {
	verticalmenu m(conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));

	m.setwindow(textwindow(sizeWArea.x1, sizeWArea.y1, sizeWArea.x1+27, sizeWArea.y1+9, conf.getcolor(cp_main_text)));
	m.idle = &menuidle;

	vector<pair<int, string> >::const_iterator ir = r.begin();
	while(ir != r.end()) {
	    m.additem((string(" ") + ir->second).c_str());
	    ++ir;
	}

	m.scale();
	int i = m.open();
	m.close();

	if(i) {
	    char cbuf[512];
	    sprintf(cbuf, _("Result of the external action %s:"),
		(r.begin()+i-1)->second.c_str());

	    string buf;
	    external.execmanual(ic, i-1, buf);

	    if(!buf.empty()) {
		imnotification ev(ic, (string) cbuf + "\n\n" + buf);
		saveworkarea();
		eventview(&ev, vector<eventviewresult>(), true);
		restoreworkarea();
	    }
	}
    }
}

void icqface::showeventbottom(const imcontact &ic) {
    const char *text = ischannel(ic) ?
	_("%s send, %s multi, %s/%s pr/nxt chat, %s hist, %s URLs, %s expand, %s memb") :
	_("%s send, %s multi, %s/%s pr/nxt chat, %s hist, %s URLs, %s expand, %s info");

    status(text,
	getstatkey(key_send_message, section_editor).c_str(),
	getstatkey(key_multiple_recipients, section_editor).c_str(),
	getstatkey(key_prev_chat, section_editor).c_str(),
	getstatkey(key_next_chat, section_editor).c_str(),
	getstatkey(key_history, section_editor).c_str(),
	getstatkey(key_show_urls, section_editor).c_str(),
	getstatkey(key_fullscreen, section_editor).c_str(),
	getstatkey(key_info, section_editor).c_str());
}

bool icqface::eventedit(imevent &ev) {
    bool r;
    texteditor editor;
    icqcontact *c;
    string msg;

    editdone = r = false;
    passinfo = ev.getcontact();

    editor.addscheme(cp_main_text, cp_main_text, 0, 0);
    editor.otherkeys = &editmsgkeys;
    editor.idle = &editidle;
    editor.wrap = true;
    editor.smarttab = false;
    editor.emacs = conf.getemacs();

    saveworkarea();
    clearworkarea();

    peerinfo(2, ev.getcontact());

    mainw.writef(sizeWArea.x1+2, sizeWArea.y1, conf.getcolor(cp_main_highlight),
	_("Outgoing %s"), streventname(ev.gettype()));

    showeventbottom(ev.getcontact());

    if(ev.gettype() == imevent::message) {
	editor.setcoords(sizeWArea.x1+2, sizeWArea.y1+3, sizeWArea.x2, sizeWArea.y2);

	immessage *m = static_cast<immessage *>(&ev);
	editor.load(msg = m->gettext(), "");

	editdone = false;
	editor.open();
	r = editdone;

	auto_ptr<char> p(editor.save("\r\n"));
	*m = immessage(ev.getcontact(), imevent::outgoing, p.get());

	if(c = clist.get(ev.getcontact()))
	    c->setpostponed(r ? "" : p.get());

    } else if(ev.gettype() == imevent::xml) {
	editor.setcoords(sizeWArea.x1+2, sizeWArea.y1+3, sizeWArea.x2, sizeWArea.y2);

	while(1) {
	    imxmlevent *m = static_cast<imxmlevent *>(&ev);
	    editor.load(msg = m->gettext(), "");

	    editdone = false;
	    editor.open();
	    r = editdone;

	    auto_ptr<char> p(editor.save("\r\n"));
	    m->setfield("text", p.get());

	    if(r)
	    if(ev.getcontact().pname == livejournal)
	    if(!setljparams(m))
		continue;

	    if(c = clist.get(ev.getcontact()))
		c->setpostponed(r ? "" : p.get());

	    break;
	}

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
	editor.load(msg = m->getmessage(), "");
	editor.open();

	r = editdone;

	auto_ptr<char> p(editor.save("\r\n"));
	*m = imsms(ev.getcontact(), imevent::outgoing, p.get(), m->getphone());

	if(c = clist.get(ev.getcontact()))
	    c->setpostponed(r ? "" : p.get());

    } else if(ev.gettype() == imevent::authorization) {
	imauthorization *m = static_cast<imauthorization *>(&ev);

	if(gethook(ev.getcontact().pname).getCapabs().count(hookcapab::authreqwithmessages)) {
	    editor.setcoords(sizeWArea.x1+2, sizeWArea.y1+3, sizeWArea.x2, sizeWArea.y2);
	    editor.load(msg = m->getmessage(), "");
	    editor.open();
	    auto_ptr<char> p(editor.save("\r\n"));
	    *m = imauthorization(ev.getcontact(), imevent::outgoing, imauthorization::Request, p.get());
	    r = editdone;
	} else {
	    *m = imauthorization(ev.getcontact(), imevent::outgoing, imauthorization::Request, "");
	    r = true;
	}


    } else if(ev.gettype() == imevent::contacts) {
	imcontacts *m = static_cast<imcontacts *>(&ev);
	imcontact cont;

	vector<imcontact> smuins = muins;
	vector<imcontact>::iterator imc;

	vector< pair<unsigned int, string> > clst;
	vector< pair<unsigned int, string> >::const_iterator icc;

	muins.clear();

	for(icc = m->getcontacts().begin(); icc != m->getcontacts().end(); ++icc) {
	    cont = imcontact();
	    cont.pname = ev.getcontact().pname;

	    if(icc->first) cont.uin = icc->first; else
		cont.nickname = icc->second;

	    muins.push_back(cont);
	}

	set<protocolname> ps;
	ps.insert(ev.getcontact().pname);
	r = multicontacts(_("Send contacts.."), ps);

	for(imc = muins.begin(); imc != muins.end(); ++imc) {
	    if(imc->uin) {
		icqcontact *cc = clist.get(*imc);
		if(cc) imc->nickname = cc->getnick();
	    }

	    clst.push_back(make_pair(imc->uin, imc->nickname));
	}

	muins = smuins;

	*m = imcontacts(ev.getcontact(), imevent::outgoing, clst);

    } else if(ev.gettype() == imevent::file) {
	dialogbox db;
	int baritem, mitem;
	bool finished = false, floop = true;
	vector<imfile::record>::const_iterator ir;

	imfile *m = static_cast<imfile *>(&ev);

	db.setwindow(new textwindow(sizeWArea.x1+1, sizeWArea.y1+2, sizeWArea.x2,
	    sizeWArea.y2, conf.getcolor(cp_main_text), TW_NOBORDER));
	db.setmenu(new verticalmenu(conf.getcolor(cp_main_menu),
	    conf.getcolor(cp_main_selected)));
	db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	    conf.getcolor(cp_main_selected),
	    _("Add"), _("Remove"), _("Send"), 0));

	db.addkey(KEY_IC, 0);
	db.addkey(KEY_DC, 1);

	db.addautokeys();
	db.idle = &dialogidle;

	db.redraw();

	workarealine(sizeWArea.y1+2);
	workarealine(sizeWArea.y2-2);

	while(!finished && !r) {
	    db.getmenu()->clear();

	    for(ir = m->getfiles().begin(); ir != m->getfiles().end(); ++ir)
		db.getmenu()->additemf(" %s", ir->fname.c_str());

	    if(floop) {
		baritem = 0;
		floop = false;
	    } else {
		finished = !db.open(mitem, baritem);
	    }

	    if(!finished) {
		vector<imfile::record> files = m->getfiles();

		switch(baritem) {
		    case 0:
			if((msg = inputfile(_("file name: "))).size()) {
			    struct stat st;

			    if(!stat(msg.c_str(), &st)) {
				imfile::record fr;
				fr.fname = msg;
				fr.size = st.st_size;
				files.push_back(fr);
			    }
			}
			break;
		    case 1:
			if(mitem > 0 && mitem <= m->getfiles().size())
			    files.erase(files.begin()+mitem-1);
			break;
		    case 2:
			r = true;
			break;
		}

		m->setfiles(files);
	    }
	}
    }

    editdone = false;
    restoreworkarea();
    status("@");
    return r;
}

string icqface::extracttime(const imevent &ev) {
    string ds1, ds2, r;
    int tdiff;
    time_t t, ts;
    char buf[64];

    ds1 = time2str(&(t = ev.gettimestamp()), "DD.MM.YY hh:mm", buf);
    ds2 = time2str(&(ts = ev.getsenttimestamp()), "DD.MM.YY hh:mm", buf);

    r = ds1 + " ";
    tdiff = ts-t;
    if(tdiff < 0) tdiff *= -1;

    if(tdiff > 5)
    if(ds1 != ds2) r += (string) "[" + ds2 + "] ";

    return r;
}

void icqface::renderchathistory() {
    int count, chatmargin;
    string text;
    time_t lastread;
    struct stat st;

    vector<imevent *> events;
    typedef pair<imevent::imdirection, vector<string> > histentry;
    vector<histentry> toshow;
    vector<histentry>::iterator ir;
    vector<string>::reverse_iterator il;

    static imevent *lastev = 0;

    delete lastev;
    passevent = lastev = 0;

    histentry h;

    icqcontact *c = clist.get(passinfo);
    if(!c) return;

    count = 0;

    if(!stat((c->getdirname() + "history").c_str(), &st)) {
	count = st.st_size-chatlines*(sizeWArea.x2-sizeWArea.x1)*2;
	if(count < 0) count = 0;
    }

    c->sethistoffset(count);

    count = 0;
    events = em.getevents(passinfo, chatlastread);
    chatlastread = 0;
    lastread = c->getlastread();

    if(events.size()) {
	c->setlastread(events.back()->gettimestamp());
    }

    while(events.size() > chatlines) {
	delete *events.begin();
	events.erase(events.begin());
    }

    while(events.size()) {
	if(events.back()->getdirection() == imevent::incoming) {
	    if(events.back()->gettimestamp() > lastread) {
		if(events.back()->gettype() == imevent::authorization
		|| events.back()->gettype() == imevent::contacts
		|| events.back()->gettype() == imevent::file) {
		    bool fin, enough;
		    fin = enough = false;
		    saveworkarea();

		    while(!fin && !enough)
			cicq.readevent(*events.back(), enough, fin);

		    restoreworkarea();
		}
	    }
	}

	if(count < chatlines) {
	    text = extracttime(*events.back());
	    text += events.back()->gettext();
	    chatlastread = events.back()->gettimestamp()-1;

	    h = histentry();
	    h.first = events.back()->getdirection();
	    breakintolines(text, h.second, sizeWArea.x2-sizeWArea.x1-2);
	    toshow.push_back(h);

	    if(!lastev)
	    if(h.first == imevent::incoming) {
		passevent = lastev = events.back()->getevent();
		extracturls(text);
	    }

	    count += h.second.size();
	}

	delete events.back();
	events.pop_back();
    }

    chatmargin = sizeWArea.y1+chatlines;
    text = string(sizeWArea.x2-sizeWArea.x1-2, ' ');

    attrset(conf.getcolor(cp_main_text));
    for(count = 0; count < chatlines; count++)
	mvprintw(chatmargin-count, sizeWArea.x1+2, "%s", text.c_str());

    for(count = 0, ir = toshow.begin(); ir != toshow.end() && count < chatlines; ++ir) {
	switch(ir->first) {
	    case imevent::incoming:
		attrset(conf.getcolor(cp_main_history_incoming));
		break;
	    case imevent::outgoing:
		attrset(conf.getcolor(cp_main_history_outgoing));
		break;
	}

	for(il = ir->second.rbegin(); il != ir->second.rend() && count < chatlines; ++il) {
	    kgotoxy(sizeWArea.x1+2, chatmargin-count);
	    printstring(*il);
	    count++;
	}
    }

    peerinfo(chatlines+1, passinfo);
}

void icqface::peerinfo(int line, const imcontact &ic) {
    workarealine(sizeWArea.y1+line);

    icqcontact *c = clist.get(ic);
    if(!c) return;

    string text = conf.getprotocolname(passinfo.pname) + " " + c->getdispnick();
    int maxsize = sizeWArea.x2-sizeWArea.x1-10;
    bool pgpon = false;

    if(text.size() > maxsize) {
	text.erase(maxsize);
	text += "..";
    }

#ifdef HAVE_GPGME
    if(pgpon = pgp.enabled(passinfo))
	text += string(4, ' ');
#endif

    text = (string) "[ " + text + " ]";

    kwriteatf(sizeWArea.x2-text.size()-1, sizeWArea.y1+line,
	conf.getcolor(cp_main_text), "%s", text.c_str());

    if(pgpon) {
	kwriteatf(sizeWArea.x2-6, sizeWArea.y1+line,
	    conf.getcolor(cp_main_highlight), "PGP");
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
    c->setopenedforchat(true);

    muins.clear();
    muins.push_back(passinfo);

    chatlines = (int) ((sizeWArea.y2-sizeWArea.y1)*0.75);
//    workarealine(sizeWArea.y1+chatlines+1);

    editor.addscheme(cp_main_text, cp_main_text, 0, 0);
    editor.otherkeys = &editmsgkeys;
    editor.idle = &editchatidle;
    editor.wrap = true;
    editor.smarttab = false;
    editor.emacs = conf.getemacs();
    editor.setcoords(sizeWArea.x1+2, sizeWArea.y1+chatlines+2, sizeWArea.x2, sizeWArea.y2);
    editor.load(c->getpostponed(), "");

    showeventbottom(ic);

    for(bool finished = false; !finished && clist.get(ic); ) {
	renderchathistory();

	editdone = false;
	editor.open();
	auto_ptr<char> p(editor.save("\r\n"));

	editor.close();
	editor.load("", "");

	if(editdone) {
	    auto_ptr<imevent> ev(new immessage(ic, imevent::outgoing, p.get()));

	    for(i = muins.begin(); i != muins.end(); ++i) {
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

    c->toggleopenedforchat();
    c->save();
    restoreworkarea();
    status("@");
    inchat = false;
    update();
}

icqface::eventviewresult icqface::eventview(const imevent *ev,
vector<eventviewresult> abuttons, bool nobuttons) {

    string title_event, title_timestamp, text;
    horizontalbar *bar = 0;
    dialogbox db;
    int mitem, baritem;
    eventviewresult r;
    static int elem = 0;

    vector<eventviewresult> actions;
    vector<eventviewresult>::iterator ia;

    if(!nobuttons) {
	if(ev->gettype() == imevent::message
	|| ev->gettype() == imevent::notification
	|| ev->gettype() == imevent::xml) {
	    actions.push_back(forward);

	    if(ev->getdirection() == imevent::incoming)
	    if(ev->getcontact().pname != rss /*|| islivejournal(ev->getcontact())*/)
		actions.push_back(reply);

	} else if(ev->gettype() == imevent::url) {
	    actions.push_back(forward);
	    actions.push_back(open);

	    if(ev->getdirection() == imevent::incoming) {
		actions.push_back(reply);
	    }

	} else if(ev->gettype() == imevent::sms) {
	    if(ev->getdirection() == imevent::incoming) {
		actions.push_back(reply);
	    }

	} else if(ev->gettype() == imevent::authorization) {
	    if(ev->getdirection() == imevent::incoming) {
		actions.push_back(info);
		actions.push_back(accept);
		actions.push_back(reject);
	    }

	} else if(ev->gettype() == imevent::contacts) {
	    actions.push_back(info);
	    actions.push_back(add);

	} else if(ev->gettype() == imevent::email) {
	    actions.push_back(forward);

	} else if(ev->gettype() == imevent::file) {
	    actions.push_back(info);

	    if(gethook(ev->getcontact().pname).knowntransfer(*static_cast<const imfile *>(ev))) {
		actions.push_back(accept);
		actions.push_back(reject);
	    }

	}
    }

    actions.push_back(ok);
    copy(abuttons.begin(), abuttons.end(), back_inserter(actions));

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

    clearworkarea();

    db.setwindow(new textwindow(sizeWArea.x1+1, sizeWArea.y1+3, sizeWArea.x2,
	sizeWArea.y2, conf.getcolor(cp_main_text), TW_NOBORDER));

    db.setbar(bar = new horizontalbar(conf.getcolor(cp_main_highlight),
	conf.getcolor(cp_main_selected), 0));

    bar->item = actions.size()-1;

    for(ia = actions.begin(); ia != actions.end(); ++ia)
	bar->items.push_back(geteventviewresult(*ia));

    mainw.writef(sizeWArea.x1+2, sizeWArea.y1, conf.getcolor(cp_main_highlight),
	title_event.c_str(), streventname(ev->gettype()), ev->getcontact().totext().c_str());

    mainw.writef(sizeWArea.x1+2, sizeWArea.y1+1, conf.getcolor(cp_main_highlight),
	title_timestamp.c_str(), extracttime(*ev).c_str());

    db.addautokeys();

    if(ev->gettype() == imevent::contacts) {
	const imcontacts *m = static_cast<const imcontacts *>(ev);

	db.setmenu(new verticalmenu(conf.getcolor(cp_main_menu),
	    conf.getcolor(cp_main_selected)));

	vector< pair<unsigned int, string> > lst = m->getcontacts();
	vector< pair<unsigned int, string> >::const_iterator il;

	for(il = lst.begin(); il != lst.end(); ++il) {
	    db.getmenu()->additemf(" %s", il->second.c_str());
	}

	db.getmenu()->setpos(elem-1);

    } else {
	text = ev->gettext();
	db.setbrowser(new textbrowser(conf.getcolor(cp_main_text)));
	db.getbrowser()->setbuf(text);
	extracturls(text);

	status(_("%s to URLs, %s to full-screenize, %s close"),
	    getstatkey(key_show_urls, section_history).c_str(),
	    getstatkey(key_fullscreen, section_history).c_str(),
	    getstatkey(key_quit, section_editor).c_str());

    }

    db.redraw();

    workarealine(sizeWArea.y1+3);
    workarealine(sizeWArea.y2-2);

    db.otherkeys = &eventviewkeys;
    db.idle = &dialogidle;

    passevent = ev;

    if(db.open(mitem, baritem)) {
	r = actions[baritem];
    } else {
	r = cancel;
    }

    elem = mitem;

    switch(r) {
	case add:
	case info:
	    extk = elem;
	    break;
    }

    passevent = 0;
    return r;
}

void icqface::fullscreenize(const imevent *ev) {
    textwindow w(0, 1, COLS, LINES-1, conf.getcolor(cp_main_text), TW_NOBORDER);
    textbrowser tb(0, 4, COLS, LINES-1, conf.getcolor(cp_main_text));

    int k;
    char buf[512], *fmt = 0;
    vector<string> lines;
    vector<string>::const_iterator il;

    w.open();

    switch(ev->getdirection()) {
	case imevent::incoming: fmt = _("%s from %s, received on %s"); break;
	case imevent::outgoing: fmt = _("%s to %s, sent on %s"); break;
    }

    sprintf(buf, fmt, streventname(ev->gettype()),
	ev->getcontact().totext().c_str(),
	strdateandtime(ev->gettimestamp()).c_str());

    w.write(0, 1, conf.getcolor(cp_main_highlight), buf);

    xtermtitle((string) _("full-screen view") + " " + buf);

    tb.setbuf(ev->gettext());

    blockmainscreen();

    status(_("%s or %s to close, Up/Down and PgUp/PgDn to scroll"),
	getstatkey(key_quit, section_editor).c_str(),
	getstatkey(key_fullscreen, section_editor).c_str());

    tb.idle = &textbrowseridle;
    tb.otherkeys = &fullscreenkeys;
    tb.open();

    unblockmainscreen();
    status("@");

    w.close();
}

void icqface::histmake(const vector<imevent *> &hist) {
    vector<imevent *>::const_reverse_iterator i;
    string text;
    time_t t, ts;
    char buf[64];
    int color;

    mhist.clear();
    mhist.setpos(0);

    for(i = hist.rbegin(); i != hist.rend(); ++i) {
	imevent &ev = **i;

	color = 0;

	t = ev.gettimestamp();
	ts = ev.getsenttimestamp();
	text = (string) + " " + time2str(&t, "DD.MM hh:mm", buf) + " ";
	if ((t - ts) > 0) 
	    text += (string) + "[" + time2str(&ts, "DD.MM hh:mm", buf) + "] ";
	text += ev.gettext();

	if(ev.getdirection() == imevent::incoming) {
	    color = conf.getcolor(cp_main_history_incoming);
	} else if(ev.getdirection() == imevent::outgoing) {
	    color = conf.getcolor(cp_main_history_outgoing);
	}

	mhist.additem(color, (void *) *i, text);
    }
}

bool icqface::histexec(imevent *&im) {
    dialogbox db;
    bool r, fin;
    int k;
    static string sub;
    char buf[512];

    r = fin = false;

    if(!mhist.empty()) {
	db.setwindow(new textwindow(sizeWArea.x1+1, sizeWArea.y1+2, sizeWArea.x2,
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

	sprintf(buf, _("History for %s, %d events total"),
	    im->getcontact().totext().c_str(),
	    mhist.getcount());

	mainw.write(sizeWArea.x1+2, sizeWArea.y1, conf.getcolor(cp_main_highlight), buf);

	status(_("%s search, %s again, %s cancel"),
	    getstatkey(key_search, section_history).c_str(),
	    getstatkey(key_search_again, section_history).c_str(),
	    getstatkey(key_quit, section_editor).c_str());

	while(!fin) {
	    if(db.open(k)) {
		switch(k) {
		    case -2:
			sub = face.inputstr(_("search for: "), sub);
			if(input.getlastkey() == KEY_ESC) break;
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
		    case -4:
			r = false;
			fin = true;
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

	    face.status("#");

	    flog = face.lastlog;
	    face.lastlog.clear();

	    bool lts, lo;
	    conf.getlogoptions(lts, lo);
	    conf.setlogoptions(false, lo);

	    for(il = flog.begin(); il != flog.end(); ++il)
		face.log(*il);

	    conf.setlogoptions(lts, lo);
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

void icqface::transferidle(dialogbox &b) {
    time_t tstart = time(0);

    while(!cicq.idle(HIDL_SOCKEXIT)) {
	if(time(0)-tstart > 3) {
	    b.gettree()->menu.abort();
	    break;
	}
    }
}

string icqface::getstatkey(int key, int section) const {
    int i = 2;
    string key2;
    string keys = action2key(key, section);

    while((key2 = action2key(key, section, i)) != "") {
	keys += "/" + key2;
	i++;
    }

    return keys;
}

string icqface::getmenuitem(string mtext, int width, int key, int section) {
    int i = 2;
    string text = " " + mtext + " ";
    string keyname = action2key(key, section);
    string key2;
    int s = keyname.size()+text.size();

    if (s > width)
	return text;

    while ((key2 = action2key(key, section, i)) != "" && (s += key2.size()+1) < width) {
	keyname += "/" + key2;
	i++;
    }

    for (s = keyname.size()+text.size(); s < width; s++)
	text += " ";

    text += keyname;

    return text;
}

int icqface::key2action(int k, int s) {
    for(vector<icqconf::keybinding>::size_type i = 0; i < icqconf::keys.size(); i++)
	if(k == icqconf::keys[i].key && s == icqconf::keys[i].section)
	    return icqconf::keys[i].action;

    return -1;
}

string icqface::action2key(int a, int s, int n) const {
    string key;
#ifdef HAVE_SSTREAM
    std::ostringstream o;
#else
    std::ostrstream o;
#endif
    vector<icqconf::keybinding>::size_type i = 0;

    for( ; i < icqconf::keys.size(); i++)
	if(a == icqconf::keys[i].action && s == icqconf::keys[i].section) {
	    if (n <= 1)
		break;
	    n--;
	}

    if (i == icqconf::keys.size())
	return "";
    else if(icqconf::keys[i].key == '\r')
	return "enter";
    else if(icqconf::keys[i].key == ' ')
	return "space";
    else if(icqconf::keys[i].key == '/')
	return "\'/\'";
    else if(icqconf::keys[i].key == ALT('\r'))
	return "alt-enter";
    else if(icqconf::keys[i].key == CTRL(' '))
	return "^space";
    else if(icqconf::keys[i].key == ALT(' '))
	return "alt-space";
    else if(icqconf::keys[i].key == 339)
	return "pageup";
    else if(icqconf::keys[i].key == 338)
	return "pagedown";
    else if(icqconf::keys[i].key == 9)
	return "tab";
    else if(icqconf::keys[i].key == 27)
	return "esc";
    else if(icqconf::keys[i].key == 331)
	return "insert";
    else if(icqconf::keys[i].key == KEY_DC)
	return "del";
    else if(icqconf::keys[i].alt && icqconf::keys[i].ctrl)
	o << "F" << (icqconf::keys[i].key-KEY_F0);
    else if(icqconf::keys[i].alt)
	o << "alt-" << (char)icqconf::keys[i].key;
    else if(icqconf::keys[i].ctrl)
	o << "^" << (char)(icqconf::keys[i].key+64);
    else
	o << (char)icqconf::keys[i].key;

    string ret;
#ifdef HAVE_SSTREAM
    ret = o.str();
#else
    ret = string(o.str(), o.pcount());
    o.freeze(false);
#endif

    return ret;
}

int icqface::contactskeys(verticalmenu &m, int k) {
    icqcontact *c = NULL;

    int id = face.mcontacts->getid(face.mcontacts->menu.getpos());
    if(id != -1 && ! face.mcontacts->isnode(id))
	c = (icqcontact *) face.mcontacts->getref(m.getpos()+1);

    set<hookcapab::enumeration> capab;

    face.extk = 0;

    if(c)
    if(c->getdesc() != contactroot) {
	capab = gethook(c->getdesc().pname).getCapabs();
    }

    switch(face.key2action(k, section_contact)) {
	case key_info:
	    face.extk = ACT_INFO;
	    break;

	case key_remove:
	    face.extk = ACT_REMOVE;
	    break;

	case key_quit:
	    face.extk = ACT_QUIT;
	    break;

	case key_send_url:
	    if(capab.count(hookcapab::urls))
		face.extk = ACT_URL;
	    break;

	case key_change_status:
	    face.extk = ACT_STATUS;
	    break;

	case key_history:
	    face.extk = ACT_HISTORY;
	    break;

	case key_next_chat:
	    face.extk = ACT_DUMMY;
	    face.next_chat(true);
	    break;
	    
	case key_prev_chat:
	    face.extk = ACT_DUMMY;
	    face.next_chat(false);
	    break;

	case key_add: face.extk = ACT_ADD; break;

	case key_send_contact:
	    if(capab.count(hookcapab::contacts))
		face.extk = ACT_CONTACT;
	    break;
	case key_rss_check:
	    if(c && c->getdesc().pname == rss)
		face.extk = ACT_PING;
	    break;

	case key_fetch_away_message:
	    if(capab.count(hookcapab::fetchaway))
		face.extk = ACT_FETCHAWAY;
	    break;

	case key_user_menu:
	    face.extk = ACT_MENU;
	    break;

	case key_general_menu:
	    face.extk = ACT_GMENU;
	    break;

	case key_hide_offline:
	    face.extk = ACT_HIDEOFFLINE;
	    break;

	case key_contact_external_action:
	    if(!ischannel(c) && c)
		face.extk = ACT_EXTERN;
	    break;

	case key_rename:
	    if(!ischannel(c) && c)
		face.extk = ACT_RENAME;
	    break;

	case key_version:
	    if(capab.count(hookcapab::version))
		face.extk = ACT_VERSION;
	    break;

	case key_edit:
	    if(!ischannel(c) && c)
		face.extk = ACT_EDITUSER;
	    break;

	case key_ignore:
	    if(c) face.extk = ACT_IGNORE;
	    break;

	case key_quickfind: face.extk = ACT_QUICKFIND; break;
    }

    if(face.extk && face.key2action(k, section_contact) != -1) {
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
	case '/':
	    return -3;
    }

    if(face.key2action(k, section_contact) == key_quickfind)
	return -3;

    return -1;
}

int icqface::findpgpkeys(dialogbox &db, int k) {
    int r;

    r = face.key2action(k, section_contact);
    switch(r) {
	case key_quickfind:
	    return -3;
    }

    return -1;
}

int icqface::historykeys(dialogbox &db, int k) {
    static string sub;

    switch(face.key2action(k, section_history)) {
	case key_search:
	    return -2;
	case key_search_again:
	    return -3;
	case key_quit:
	    return -4;
    }

    return -1;
}

int icqface::editmsgkeys(texteditor &e, int k) {
    char *p;

    if(k == '\r' && conf.getentersends(face.passinfo.pname)) {
	p = e.save("");
	face.editdone = strlen(p);
	delete p;
	if(face.editdone) return -1; else return 0;
    }

    switch(face.key2action(k, section_editor)) {
	case key_send_message:
	    p = e.save("");
	    face.editdone = strlen(p);
	    delete p;
	    if(face.editdone) return -1; else break;
	case key_multiple_recipients:
	    face.multicontacts("");
	    break;
	case key_history:
	    cicq.history(face.passinfo);
	    break;
	case key_prev_chat:
	    face.editdone = false;
	    face.next_chat(false);
	    return -1;
	case key_next_chat:
	    face.editdone = false;
	    face.next_chat(true);
	    return -1;
	case key_info:
	    cicq.userinfo(face.passinfo);
	    break;
	case key_show_urls:
	    face.showextractedurls(); break;
	case key_fullscreen:
	    if(face.passevent)
		face.fullscreenize(face.passevent);
	    break;
	case key_quit:
	    return -1;
    }

    return 0;
}

int icqface::userinfokeys(dialogbox &db, int k) {
    switch(face.key2action(k, section_info)) {
	case key_show_urls: face.showextractedurls(); break;
	case key_user_external_action: face.userinfoexternal(face.passinfo); break;
    }

    return -1;
}

int icqface::eventviewkeys(dialogbox &db, int k) {
    switch(face.key2action(k, section_history)) {
	case key_show_urls:
	    face.showextractedurls();
	    break;
	case key_fullscreen:
	    face.fullscreenize(face.passevent);
	    break;
	case key_quit:
	    return 0;
    }

    return -1;
}

int icqface::findreskeys(dialogbox &db, int k) {
    if(face.key2action(k, section_contact) == key_quickfind)
	return -3;

    return -1;
}

int icqface::statuskeys(verticalmenu &m, int k) {
    char *status_order = "o_adncfi";    // Shortcuts for status
    char *p = strchr(status_order, k);
    if (p)
	return (1 + p - status_order);
    return -1;
}

int icqface::fullscreenkeys(textbrowser &m, int k) {
    switch(face.key2action(k, section_editor)) {
	case key_fullscreen:
	    return 0;
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
    if(c->hasevents()) {
	face.renderchathistory();
    }
}

void icqface::textinputidle(textinputline &il) {
    cicq.idle();
}

void icqface::termresize(void) {
    face.dotermresize = true;
}

void icqface::relaxedupdate() {
    fneedupdate = true;
}

bool icqface::updaterequested() {
    return fneedupdate;
}

void icqface::redraw() {
    if(!mainscreenblock) {
	done();
	init();
	draw();
	doredraw = fneedupdate = false;
    } else {
	doredraw = fneedupdate = true;
    }
}

void icqface::xtermtitle(const string &text) {
    if(conf.getxtitles()) {
	string term = getenv("TERM") ? getenv("TERM") : "";

	if(term == "xterm" || term == "Eterm" || term == "aterm"
	|| term == "rxvt" || term.substr(0, 6) == "screen")
	    cout << "\x1b]1;\x07\x1b]2;" << "centericq" << (text.empty() ? "" : (string) ": " + text) << "\x07" << flush;
    }
}

void icqface::xtermtitle(const char *fmt, ...) {
    if(conf.getxtitles()) {
	va_list ap;
	char buf[1024];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	xtermtitle((string) buf);
	va_end(ap);
    }
}

void icqface::xtermtitlereset() {
    if(conf.getxtitles()) {
	const char *p = getenv("TERM");
	if(p) xtermtitle((string) p);
    }
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

void icqface::icqprogress::show(const string &title ) {
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
