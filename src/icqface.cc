#include "icqface.h"
#include "icqconf.h"
#include "icqhook.h"
#include "icqhist.h"
#include "centericq.h"
#include "icqcontact.h"
#include "icqcontacts.h"
#include "icqmlist.h"

#include "konst.socket.h"

const char *stryesno(bool i) {
    return i ? _("yes") : _("no");
}

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

const char *strgender(unsigned char fgender) {
    switch(fgender) {
	case 1: return _("Female");
	case 2: return _("Male");
	default: return _("Not specified");
    }
}

string getbdate(unsigned char bday, unsigned char bmonth, unsigned char byear) {
    string ret;

    if((bday > 0) && (bday <= 31))
    if((bmonth > 0) && (bmonth <= 12)) {

	ret = i2str(bday) + "-";

	switch(bmonth) {
	    case  1: ret += _("Jan"); break;
	    case  2: ret += _("Feb"); break;
	    case  3: ret += _("Mar"); break;
	    case  4: ret += _("Apr"); break;
	    case  5: ret += _("May"); break;
	    case  6: ret += _("Jun"); break;
	    case  7: ret += _("Jul"); break;
	    case  8: ret += _("Aug"); break;
	    case  9: ret += _("Sep"); break;
	    case 10: ret += _("Oct"); break;
	    case 11: ret += _("Nov"); break;
	    case 12: ret += _("Dec"); break;
	    default: return "";
	}

	ret += '-';
	ret += i2str(1900 + byear);
    }
    return ret;
}

icqface::icqface(): mainscreenblock(false), inited(false), onlinefolder(false) {
    kinterface();
    workareas.freeitem = &freeworkareabuf;
}

icqface::~icqface() {
    kendinterface();
    if(inited) {
	for(int i = 0; i < LINES; i++) printf("\n");
    }
}

void icqface::init() {
    mcontacts = new treeview(1, 2, 25, LINES-2,
    conf.getcolor(cp_main_menu), conf.getcolor(cp_main_selected),
    conf.getcolor(cp_main_menu), conf.getcolor(cp_main_menu));

    mcontacts->menu.idle = &menuidle;
    mcontacts->menu.otherkeys = &contactskeys;

    il = new textinputline(conf.getcolor(cp_status));
    mainw = textwindow(0, 1, COLS-1, LINES-2, conf.getcolor(cp_main_frame));

    fm = new filemanager(1, 2, (int) (COLS*0.7), (int) (LINES*0.75),
    conf.getcolor(cp_dialog_menu), conf.getcolor(cp_dialog_highlight), 0,
    conf.getcolor(cp_dialog_selected), conf.getcolor(cp_dialog_frame));

    attrset(conf.getcolor(cp_status));
    mvhline(0, 0, ' ', COLS);
    mvhline(LINES-1, 0, ' ', COLS);

    inited = true;
}

void icqface::done() {
    delete mcontacts;
    delete il;
    delete fm;
}

void icqface::draw() {
    int i;
    string pass;

    mainw.open();
    mainw.separatex(25);
    workarealine(WORKAREA_Y2);
    mvhline(WORKAREA_Y2, WORKAREA_X1, LTEE, 1);
    mvhline(WORKAREA_Y2, WORKAREA_X2, RTEE, 1);

    update();
}

void icqface::update() {
    string st = _("STATUS: ") + textstatus(icql.icq_Status);
    string suin = i2str(icql.icq_Uin);

    attrset(conf.getcolor(cp_status));
    mvhline(0, 0, ' ', COLS);

    kwriteatf(2, 0, conf.getcolor(cp_status), "CENTERICQ %s", VERSION);
    kwriteatf(20, 0, conf.getcolor(cp_status), "%s", st.c_str());
    kwriteatf(COLS-suin.size()-1, 0, conf.getcolor(cp_status), "%s", suin.c_str());

    fillcontactlist();
}

int icqface::contextmenu(icqcontact *c) {
    int ret = 0, i;
    static int elem = 0;

    verticalmenu m(WORKAREA_X1+1, WORKAREA_Y1+1,
	WORKAREA_X1+27, WORKAREA_Y1+2, conf.getcolor(cp_main_text),
	conf.getcolor(cp_main_selected));

    m.setwindow(textwindow(WORKAREA_X1, WORKAREA_Y1, WORKAREA_X1+27,
	WORKAREA_Y1+7, conf.getcolor(cp_main_text)));

    if(!c) return 0;

    if(!c->getuin()) {
	m.additem(_(" Events history         h"));
    } else if(!c->isnonicq() && c->getuin() && c->inlist()) { 
	m.additem(_(" Send a message     enter"));
	m.additem(_(" Send an URL            u"));
	m.addline();
	m.additem(_(" Send a file            f"));
	m.addline();
	m.additem(_(" User's details         ?"));
	m.additem(_(" Events history         h"));
	m.additem(_(" Remove user          del"));
	m.additem(_(" Ignore user"));
	m.additem(_(" Rename user            r"));

	if(!lst.inlist(c->getuin(), csvisible))
	m.additem(_(" Add to visible list    v"));
	    else
	m.additem(_(" Remove from vis. list  v"));

    } else if(!c->isnonicq() && c->getuin() && !c->inlist()) {
	m.additem(_(" Send a message     enter"));
	m.additem(_(" Send URL               u"));
	m.addline();
	m.additem(_(" Add to list            a"));
	m.addline();
	m.additem(_(" User's details         ?"));
	m.additem(_(" Events history         h"));
	m.additem(_(" Remove user          del"));
	m.additem(_(" Ignore user"));

	if(!lst.inlist(c->getuin(), csvisible))
	m.additem(_(" Add to visible list    v"));
	    else
	m.additem(_(" Remove from vis. list  v"));

    } else if(c->isnonicq()) {
	m.additem(_(" User's details         ?"));
	m.additem(_(" Remove user          del"));
	m.additem(_(" Edit details"));
    }

    m.scale();
    m.idle = &menuidle;
    m.setpos(elem);
    i = m.open();
    m.close();
    elem = i-1;

    if(!c->getuin()) {
	switch(i) {
	    case 1: ret = ACT_HISTORY; break;
	}
    } else if(!c->isnonicq() && c->getuin() && c->inlist()) { 
	switch(i) {
	    case  1: ret = ACT_MSG; break;
	    case  2: ret = ACT_URL; break;
	    case  4: ret = ACT_FILE; break;
	    case  6: ret = ACT_INFO; break;
	    case  7: ret = ACT_HISTORY; break;
	    case  8: ret = ACT_REMOVE; break;
	    case  9: ret = ACT_IGNORE; break;
	    case 10: ret = ACT_RENAME; break;
	    case 11: ret = ACT_SWITCH_VIS; break;
	}
    } else if(!c->isnonicq() && c->getuin() && !c->inlist()) {
	switch(i) {
	    case  1: ret = ACT_MSG; break;
	    case  2: ret = ACT_URL; break;
	    case  4: ret = ACT_ADD; break;
	    case  6: ret = ACT_INFO; break;
	    case  7: ret = ACT_HISTORY; break;
	    case  8: ret = ACT_REMOVE; break;
	    case  9: ret = ACT_IGNORE; break;
	    case 10: ret = ACT_SWITCH_VIS; break;
	}
    } else if(c->isnonicq()) {
	switch(i) {
	    case 1: ret = ACT_INFO; break;
	    case 2: ret = ACT_REMOVE; break;
	    case 3: ret = ACT_EDITUSER; break;
	}
    }

    return ret;
}

int icqface::generalmenu() {
    static int lastitem = 0;

    verticalmenu m(conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));
    m.setwindow(textwindow(WORKAREA_X1, WORKAREA_Y1, WORKAREA_X1+40, WORKAREA_Y1+10, conf.getcolor(cp_main_text)));

    m.idle = &menuidle;
    m.additem(_(" Change ICQ status                   s"));
    m.additem(_(" Go to contact..                 alt-s"));
    m.additem(_(" Update your details"));
    m.additem(_(" Find/add users"));
    m.additem(_(" CenterICQ config options"));
    m.additem(_(" Add non-icq contact"));
    m.addline();
    m.additem(_(" View/edit ignore list"));
    m.additem(_(" View/edit visible list"));
    m.setpos(lastitem);

    int i = m.open();
    m.close();

    if(i) {
	lastitem = i-1;

	switch(i) {
	    case 1: return ACT_STATUS;
	    case 2: return ACT_QUICKFIND;
	    case 3: return ACT_DETAILS;
	    case 4: return ACT_FIND;
	    case 5: return ACT_CONF;
	    case 6: return ACT_NONICQ;
	    case 8: return ACT_IGNORELIST;
	    case 9: return ACT_VISIBLELIST;
	}
    }
}

icqcontact *icqface::mainloop(int &action) {
    int i, curid;
    icqcontact *c = 0;
    bool fin;

    for(fin = false; !fin; ) {
	extk = ACT_MSG;
	c = (icqcontact *) mcontacts->open(&i);
	if((int) c < 10) c = 0;

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
    int i, id, thisnode = 0;
    string dnick;
    icqcontact *c;
    void *savec;
    char prevsc = 'x', sc;
    bool online_added = false;

    savec = mcontacts->getref(mcontacts->getid(mcontacts->menu.getpos()));
    mcontacts->clear();
    clist.order();

    if(c = clist.get(0))
    if(c->getmsgcount())
	mcontacts->addleaf(0, conf.getcolor(cp_main_highlight), c, "#ICQ ");

    for(i = 0; i < clist.count; i++) {
	c = (icqcontact *) clist.at(i);

	if(!c->getuin()) continue;

	if((c->getstatus() == STATUS_OFFLINE) && 
	conf.gethideoffline() && !c->getmsgcount()) {
	    continue;
	}

	if(!c->getmsgcount())
	if((sc = c->getsortchar()) != prevsc) {
	    switch(sc) {
		case 'O':
		    thisnode = mcontacts->addnode(0, conf.getcolor(cp_main_highlight), (void *) 1, " Online ");
		    online_added = true;
		    break;
		case '_': thisnode = mcontacts->addnode(0, conf.getcolor(cp_main_highlight), (void *) 2, " Offline "); break;
		case '!': thisnode = mcontacts->addnode(0, conf.getcolor(cp_main_highlight), (void *) 3, " Not in list "); break;
		case 'N': thisnode = mcontacts->addnode(0, conf.getcolor(cp_main_highlight), (void *) 4, " Non-ICQ "); break;
	    }
	    prevsc = sc;
	}

	dnick = c->getdispnick();
	if(c->isbirthday()) dnick += " :)";

	if(c->getstatus() == STATUS_OFFLINE) {
	    mcontacts->addleaf(thisnode,
		c->getmsgcount() ? conf.getcolor(cp_main_highlight) : 0,
		c, (c->getmsgcount() ? "#" : " ") + dnick + " ");
	} else {
	    mcontacts->addleaff(thisnode,
		c->getmsgcount() ? conf.getcolor(cp_main_highlight) : 0,
		c, "%s[%c] %s ", c->getmsgcount() ? "#" : " ",
		c->getshortstatus(), dnick.c_str());
	}
    }

    if(c = clist.get(0))
    if(!c->getmsgcount())
	mcontacts->addleaf(0, conf.getcolor(cp_main_highlight), c, " ICQ ");

    if(!mainscreenblock) mcontacts->redraw();

    if(!savec || (!onlinefolder && online_added && conf.gethideoffline())) {
	mcontacts->menu.setpos(0);
    } else
    for(i = 0; i < mcontacts->menu.getcount(); i++) {
	int id = mcontacts->getid(i);
	if(mcontacts->getref(id) == savec) {
	    mcontacts->menu.setpos(i);
	    break;
	}
    }

    onlinefolder = online_added;

    if(!mainscreenblock)
	mcontacts->menu.redraw();
}

void icqface::getregdata(string &nick, string &fname, string &lname, string &email) {
    nick = rnick;
    fname = rfname;
    lname = rlname;
    email = remail;
}

bool icqface::findresults() {
    bool finished = false, ret = false;
    unsigned int ffuin;
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

    ihook.clearfindresults();
    ihook.setfinddest(db.getmenu());

    db.idle = &dialogidle;
    db.redraw();

    mainw.write(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight), "Find results");

    workarealine(WORKAREA_Y1+2);
    workarealine(WORKAREA_Y2-2);

    while(!finished) {
	finished = !db.open(r, b);
	ihook.setfinddest(0);
	
	if(!finished)
	switch(b) {
	    case 0:
		if(ffuin = ihook.getfinduin(r)) cicq.userinfo(ffuin);
		break;
	    case 1:
		if(ffuin = ihook.getfinduin(r))
		if(!clist.get(ffuin)) {
		    clist.addnew(ffuin, false);
		    face.update();
		}
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

void icqface::infoclear(dialogbox &db, icqcontact *c, unsigned int uin) {
    db.redraw();
    mainw.writef(WORKAREA_X1+2, WORKAREA_Y1,
	conf.getcolor(cp_main_highlight), _("Information about %lu, %s"), uin,
	c->isnonicq() ? _("Non-ICQ") : textstatus(c->getstatus()).c_str());

    workarealine(WORKAREA_Y1+2);
    workarealine(WORKAREA_Y2-2);
}

void icqface::infogeneral(dialogbox &db, icqcontact *c) {
    string fhomepage, fname, lname, fprimemail, fsecemail;
    string foldemail, fcity, fstate, fphone, ffax, fstreet, fcellular, tmp;
    bool reqauth, webaware, pubip;

    unsigned long fzip, fwzip;
    unsigned short fcountry, fwcountry;
    unsigned char fage, flang1, flang2, flang3, fbday, fbmonth, fbyear, fgender;

    c->getinfo(fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate, fphone, ffax, fstreet, fcellular, fzip, fcountry);
    c->getmoreinfo(fage, fgender, fhomepage, flang1, flang2, flang3, fbday, fbmonth, fbyear);
    c->getsecurity(reqauth, webaware, pubip);

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

    tmp = flang1 ? icq_GetMetaLanguageName(flang1) : "";

    if(flang2) {
	if(tmp.size()) tmp += ", ";
	tmp += icq_GetMetaLanguageName(flang2);
    }

    if(flang3) {
	if(tmp.size()) tmp += ", ";
	tmp += icq_GetMetaLanguageName(flang3);
    }

    fname = fname + " " + lname;

    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+2, conf.getcolor(cp_main_text), c->getnick());
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+3, conf.getcolor(cp_main_text), fname);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+4, conf.getcolor(cp_main_text), fprimemail);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+5, conf.getcolor(cp_main_text), fsecemail);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+6, conf.getcolor(cp_main_text), foldemail);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+8, conf.getcolor(cp_main_text), strgender(fgender));
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+9, conf.getcolor(cp_main_text), getbdate(fbday, fbmonth, fbyear));
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+10, conf.getcolor(cp_main_text), fage ? i2str(fage) : "");
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+12, conf.getcolor(cp_main_text), tmp);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+13, conf.getcolor(cp_main_text), c->getlastip());

    if(c->getstatus() == STATUS_OFFLINE) {
	char buf[64];
	time_t ls = c->getlastseen();

	mainw.write(WORKAREA_X1+2, WORKAREA_Y1+14, conf.getcolor(cp_main_highlight), _("Last seen"));
	mainw.write(WORKAREA_X1+14, WORKAREA_Y1+14, conf.getcolor(cp_main_text), ls ? time2str(&ls, "DD.MM.YY hh:mm", buf) : _("Never"));
    }
}

void icqface::infohome(dialogbox &db, icqcontact *c) {
    string tmp;
    int i;

    string fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate;
    string fphone, ffax, fstreet, fcellular, fhomepage;
    
    unsigned long fzip;
    unsigned short fcountry;
    unsigned char flang1, flang2, flang3, fbday, fbmonth, fbyear, fage, fgender;

    c->getinfo(fname, lname, fprimemail, fsecemail, foldemail, fcity, fstate, fphone, ffax, fstreet, fcellular, fzip, fcountry);
    c->getmoreinfo(fage, fgender, fhomepage, flang1, flang2, flang3, fbday, fbmonth, fbyear);

    workarealine(WORKAREA_Y1+9);

    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+2, conf.getcolor(cp_main_highlight), _("Address"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+3, conf.getcolor(cp_main_highlight), _("Location"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+4, conf.getcolor(cp_main_highlight), _("Zip code"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+5, conf.getcolor(cp_main_highlight), _("Phone"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+6, conf.getcolor(cp_main_highlight), _("Fax"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+7, conf.getcolor(cp_main_highlight), _("Cellular"));
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+9, conf.getcolor(cp_main_highlight), _("Homepage"));

    tmp = fcity;
    
    if(fstate.size()) {
	if(tmp.size()) tmp += ", ";
	tmp += fstate;
    }
    
    if(fcountry) {
	if(tmp.size()) tmp += ", ";
	tmp += icq_GetCountryName(fcountry);
    }

    mainw.write(WORKAREA_X1+12, WORKAREA_Y1+2, conf.getcolor(cp_main_text), fstreet);
    mainw.write(WORKAREA_X1+12, WORKAREA_Y1+3, conf.getcolor(cp_main_text), tmp);
    mainw.write(WORKAREA_X1+12, WORKAREA_Y1+4, conf.getcolor(cp_main_text), fzip ? i2str(fzip) : "");
    mainw.write(WORKAREA_X1+12, WORKAREA_Y1+5, conf.getcolor(cp_main_text), fphone);
    mainw.write(WORKAREA_X1+12, WORKAREA_Y1+6, conf.getcolor(cp_main_text), ffax);
    mainw.write(WORKAREA_X1+12, WORKAREA_Y1+7, conf.getcolor(cp_main_text), fcellular);

    const char *p = fhomepage.c_str();

    for(i = 0; ; i++) {
	tmp.assign(p, 0, WORKAREA_X2-WORKAREA_X1-12);
	mainw.write(WORKAREA_X1+12, WORKAREA_Y1+9+i, conf.getcolor(cp_main_text), tmp);
	p += tmp.size();
	if(tmp.size() < WORKAREA_X2-WORKAREA_X1-12) break;
    }
}

void icqface::infowork(dialogbox &db, icqcontact *c) {
    string fwcity, fwstate, fwphone, fwfax, fwaddress, fcompany, fdepartment;
    string fjob, fwhomepage, tmp;

    int i;
    unsigned long fwzip;
    unsigned short fwcountry, foccupation;

    workarealine(WORKAREA_Y1+6);
    workarealine(WORKAREA_Y1+11);

    c->getworkinfo(fwcity, fwstate, fwphone, fwfax, fwaddress, 
	fwzip, fwcountry, fcompany, fdepartment, fjob,
	foccupation, fwhomepage);

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

    if(fwstate.size()) {
	if(fwcity.size()) fwcity += ", ";
	fwcity += fwstate;
    }
    
    if(fwcountry) {
	if(fwcity.size()) fwcity += ", ";
	fwcity += icq_GetCountryName(fwcountry);
    }
    
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+2, conf.getcolor(cp_main_text), fwaddress);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+3, conf.getcolor(cp_main_text), fwcity);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+4, conf.getcolor(cp_main_text), fwzip ? i2str(fwzip) : "");
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+6, conf.getcolor(cp_main_text), fcompany);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+7, conf.getcolor(cp_main_text), fdepartment);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+8, conf.getcolor(cp_main_text), foccupation ? icq_GetMetaOccupationName(foccupation) : "");
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+9, conf.getcolor(cp_main_text), fjob);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+11, conf.getcolor(cp_main_text), fwphone);
    mainw.write(WORKAREA_X1+14, WORKAREA_Y1+12, conf.getcolor(cp_main_text), fwfax);

    const char *p = fwhomepage.c_str();

    for(i = 0; ; i++) {
	tmp.assign(p, 0, WORKAREA_X2-WORKAREA_X1-14);
	mainw.write(WORKAREA_X1+14, WORKAREA_Y1+13+i, conf.getcolor(cp_main_text), tmp);
	p += tmp.size();
	if(tmp.size() < WORKAREA_X2-WORKAREA_X1-14) break;
    }
}

void icqface::infoabout(dialogbox &db, icqcontact *c) {
    db.getbrowser()->setbuf(c->getabout().c_str());
}

void commacat(string &text, string sub, bool nocomma = false) {
    if(!sub.empty()) {
	if(!nocomma)
	if(text[text.size()-1] != ',') text += ",";

	text += sub;
    }
}

void icqface::infointerests(dialogbox &db, icqcontact *c) {
    string intr[4], af[4], bg[4], text;
    int i, naf[4], nbg[4], nint[4];
    bool bint, baf, bbg;

    bint = baf = bbg = false;

    c->getinterests(intr[0], intr[1], intr[2], intr[3]);
    c->getaffiliations(af[0], af[1], af[2], af[3]);
    c->getbackground(bg[0], bg[1], bg[2], bg[3]);

    for(i = 0; i < 4; i++) {
	nint[i] = atol(getword(intr[i]).c_str());
	naf[i] = atol(getword(af[i]).c_str());
	nbg[i] = atol(getword(bg[i]).c_str());

	bint = bint || (nint[i] && !intr[i].empty());
	baf = baf || naf[i];
	bbg = bbg || nbg[i];
    }

    if(bint) {
	text = (string) "* " + _("Interests") + "\n";

	commacat(text, intr[0], true);
	for(i = 1; i < 4; i++) commacat(text, intr[i]);
	text += "\n\n";
    }

    if(baf) {
	text += (string) "* " + _("Affiliations") + "\n";

	for(i = 0; i < 4; i++)
	if(naf[i] && !af[i].empty()) {
	    text += (string) icq_GetMetaAffiliationName(naf[i]) + ": " + af[i] + "\n";
	}
	text += "\n";
    }

    if(bbg) {
	text += (string) "* " + _("Background/Past") + "\n";

	for(i = 0; i < 4; i++)
	if(nbg[i] && !bg[i].empty()) {
	    text += (string) icq_GetMetaBackgroundName(nbg[i]) + ": " + bg[i] + "\n";
	}
    }

    db.getbrowser()->setbuf(text.c_str());
}

void icqface::userinfo(unsigned int uin, bool nonicq, unsigned int realuin) {
    bool finished = false, showinfo = true;
    icqcontact *c = clist.get(realuin, nonicq);
    textbrowser tb(conf.getcolor(cp_main_text));
    dialogbox db;
    int k, lastb, b;

    b = lastb = 0;
    saveworkarea();
    clearworkarea();

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+2, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));
    db.setbar(new horizontalbar(WORKAREA_X1+2, WORKAREA_Y2-1,
	conf.getcolor(cp_main_highlight), conf.getcolor(cp_main_selected),
	_("Info"), _("Home"), _("Work"), _("More"), _("About"),
	!nonicq ? _("Retrieve") : _("Edit"), 0));
    db.idle = &dialogidle;

    while(!finished) {
	if(!showinfo) {
	    if(ihook.idle(HIDL_SOCKEXIT)) {
		finished = !db.open(k, b);
	    }
	    showinfo = !finished;
	}

	if(c->updated()) c->unsetupdated();

	if(showinfo) {
	    db.setbrowser(0);
	    infoclear(db, c, uin);

	    switch(b) {
		case 0: infogeneral(db, c); break;
		case 1: infohome(db, c); break;
		case 2: infowork(db, c); break;
		case 3:
		    db.setbrowser(&tb, false);
		    infointerests(db, c);
		    infoclear(db, c, uin);
		    break;
		case 4:
		    db.setbrowser(&tb, false);
		    infoabout(db, c);
		    infoclear(db, c, uin);
		    break;
		case 5:
		    if(nonicq) {
			updatedetails(c);
		    } else {
			c->setseq2(icq_SendMetaInfoReq(&icql, uin));
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
}

int icqface::changestatus(int old) {
    verticalmenu m(conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));
    m.setwindow(textwindow(WORKAREA_X1, WORKAREA_Y1, WORKAREA_X1+27, WORKAREA_Y1+9, conf.getcolor(cp_main_text)));

    m.idle = &menuidle;
    m.additem(_(" [o] Online"));
    m.additem(_(" [_] Offline"));
    m.additem(_(" [a] Away"));
    m.additem(_(" [d] Do not disturb"));
    m.additem(_(" [n] Not available"));
    m.additem(_(" [c] Occupied"));
    m.additem(_(" [f] Free for chat"));
    m.additem(_(" [i] Invisible"));

    switch(old) {
	case STATUS_ONLINE      : m.setpos(0); break;
	case STATUS_OFFLINE     : m.setpos(1); break;
	case STATUS_AWAY        : m.setpos(2); break;
	case STATUS_DND         : m.setpos(3); break;
	case STATUS_NA          : m.setpos(4); break;
	case STATUS_OCCUPIED    : m.setpos(5); break;
	case STATUS_FREE_CHAT   : m.setpos(6); break;
	case STATUS_INVISIBLE   : m.setpos(7); break;
    }

    int i = m.open();
    m.close();

    switch(i) {
	case 0: i = old; break;
	case 1: i = STATUS_ONLINE; break;
	case 2: i = STATUS_OFFLINE; break;
	case 3: i = STATUS_AWAY; break;
	case 4: i = STATUS_DND; break;
	case 5: i = STATUS_NA; break;
	case 6: i = STATUS_OCCUPIED; break;
	case 7: i = STATUS_FREE_CHAT; break;
	case 8: i = STATUS_INVISIBLE; break;
    }

    return i;
}

string icqface::textstatus(unsigned long st) {
    if(ihook.isconnecting()) return _("Connecting.."); else
    if((unsigned long) STATUS_OFFLINE == st) return _("Offline"); else
    if((st & STATUS_INVISIBLE) == STATUS_INVISIBLE) return _("Invisible"); else
    if((st & STATUS_FREE_CHAT) == STATUS_FREE_CHAT) return _("Free for chat"); else
    if((st & STATUS_DND) == STATUS_DND) return _("Do not disturb"); else
    if((st & STATUS_OCCUPIED) == STATUS_OCCUPIED) return _("Occupied"); else
    if((st & STATUS_NA) == STATUS_NA) return _("Not avail"); else
    if((st & STATUS_AWAY) == STATUS_AWAY) return _("Away"); else
    if(!(st & 0x01FF)) return _("Online"); else return _("Unknown");
}

string icqface::inputstr(string q, string defl = "", char passwdchar = 0) {
    int ln = LINES-2;
    chtype **sbuf = savelines(0, ln, COLS, ln);
    string ret;
    char *p;

    attrset(conf.getcolor(cp_status));
    mvhline(ln, 0, ' ', COLS);
    kwriteatf(0, ln, conf.getcolor(cp_status), "%s", q.c_str());
    il->setpasswordchar(passwdchar);
    il->fm = 0;

    ret = (p = il->open(q.size(), ln, defl.c_str(), COLS-q.size()-1, 2048));
    delete p;

    restorelines(sbuf, 0, ln, COLS, ln);
    return ret;
}

string icqface::inputfile(string q, string defl = "") {
    int ln = LINES-2;
    chtype **sbuf = savelines(0, ln, COLS, ln);
    string ret;
    char *p, *r;

    mvhline(ln, 0, ' ', COLS);
    kwriteatf(0, ln, conf.getcolor(cp_status), "%s", q.c_str());
    kwriteatf(COLS-8, ln, conf.getcolor(cp_status), "[Ctrl-T]");

    il->fm = fm;
    il->fm->w->set_title(conf.getcolor(cp_dialog_highlight), _(" enter to select a file, esc to cancel "));
    fm->onlydirs = fm->chroot = false;
    ret = (p = il->open(q.size(), ln, defl.c_str(), COLS-q.size()-10, 2048));

    if(r = strrchr(p, '/')) {
	*r = 0;
	fm->setstartpoint(p);
    }

    delete p;

    restorelines(sbuf, 0, ln, COLS, ln);
    return ret;
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
    int ln = LINES-1, ret;
    chtype **sbuf = savelines(0, ln, COLS, ln);

    q += " (";
    if(options & ASK_YES) catqstr(q, ASK_YES, deflt);
    if(options & ASK_NO) catqstr(q, ASK_NO, deflt);
    if(options & ASK_CANCEL) catqstr(q, ASK_CANCEL, deflt);
    q += ") ";

    attrset(conf.getcolor(cp_status));
    mvhline(ln, 0, ' ', COLS);
    kwriteatf(0, ln, conf.getcolor(cp_status), "%s", q.c_str());

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

    restorelines(sbuf, 0, ln, COLS, ln);
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
    chtype **workareabuf = (chtype **) workareas.at(workareas.count-1);

    if(workareabuf) {
	for(int i = 0; i <= WORKAREA_Y2-WORKAREA_Y1; i++)
	mvaddchnstr(WORKAREA_Y1+i, WORKAREA_X1, workareabuf[i], WORKAREA_X2-WORKAREA_X1+1);
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

void icqface::workarealine(int l) {
    attrset(conf.getcolor(cp_main_frame));
    mvhline(l, WORKAREA_X1+1, HLINE, WORKAREA_X2-WORKAREA_X1-1);
}

bool icqface::editurl(unsigned int uin, string &url, string &text) {
    bool finished = false;
    char *ctext, *curl;

    saveworkarea();
    clearworkarea();

    editdone = false;
    texteditor *se = new texteditor;
    textinputline *il = new textinputline(conf.getcolor(cp_main_highlight));

    il->idle = &textinputidle;

    se->setcoords(WORKAREA_X1+2, WORKAREA_Y1+5, WORKAREA_X2, WORKAREA_Y2);
    se->addscheme(cp_main_text, cp_main_text, 0, 0);
    se->otherkeys = &editmsgkeys;
    se->idle = &editidle;
    se->wrap = true;

    mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	_("URL to %s, %lu"), clist.get(uin)->getdispnick().c_str(), uin);

    workarealine(WORKAREA_Y1+2);
    workarealine(WORKAREA_Y1+4);

    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+2, conf.getcolor(cp_main_text),
	curl = il->open(WORKAREA_X1+2, WORKAREA_Y1+3, url.c_str(),
	WORKAREA_X2-WORKAREA_X1-2, 512)
    );

    if(curl[0]) {
	muins.clear();
	muins.push_back(uin);
	passuin = uin;
	status(_("Ctrl-X send, Ctrl-P multiple, Ctrl-O history, Alt-? details, ESC cancel"));

	se->load(text.c_str(), strdup(""));
	se->open();

	if(editdone) {
	    ctext = se->save("\r\n");
	    text = ctext;
	    url = curl;
	    delete ctext;
	}

	if(muins.empty()) muins.push_back(uin);
    } else {
	editdone = false;
    }

    delete se;
    delete il;
    delete curl;

    restoreworkarea();
    status("");
    return editdone;
}

bool icqface::editmsg(unsigned int uin, string &text) {
    char *ctext;
    texteditor *se = new texteditor;

    editdone = false;
    saveworkarea();
    clearworkarea();
    workarealine(WORKAREA_Y1+2);

    mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	_("Writing a message to %s, %lu"), clist.get(uin)->getdispnick().c_str(), uin);

    status(_("Ctrl-X send, Ctrl-P multiple, Ctrl-O history, Alt-? details, ESC cancel"));
    passuin = uin;

    muins.clear();
    muins.push_back(uin);

    se->setcoords(WORKAREA_X1+2, WORKAREA_Y1+3, WORKAREA_X2, WORKAREA_Y2);
    se->addscheme(cp_main_text, cp_main_text, 0, 0);

    se->otherkeys = &editmsgkeys;
    se->idle = &editidle;
    se->wrap = true;

    se->load(text.c_str(), strdup(""));
    se->open();
    ctext = se->save("\r\n");
    text = ctext;

    if(muins.empty()) muins.push_back(uin);

    delete se;
    delete ctext;

    restoreworkarea();
    status("");
    return editdone;
}

bool icqface::checkicqmessage(unsigned int uin, string text, bool &ret, int options) {
    bool proceed = true, fin;
    icqcontact *cn;
    char c;
    int i;

    if(!uin && !text.empty()) {
	c = text[0];
	text.replace(0, 1, "");
	uin = atol(text.c_str());
	if((i = text.find(" ")) != -1) text.replace(0, i+1, "");
	proceed = false;

	for(fin = false; !fin; )
	switch(i = showicq(uin, text, c, options)) {
	    case -1:
		ret = false;
		fin = true;
		break;
	    case 0:
		cicq.userinfo(uin);
		break;
	    case 1:
		clist.add(cn = new icqcontact(uin));
		cn->setnick(i2str(uin));
		cn->setseq2(icq_SendMetaInfoReq(&icql, uin));
		log(_("+ %ul has been added to the list"), uin);
		break;
	    case 2:
		switch(c) {
		    case ICQM_REQUEST:
			icq_SendAuthMsg(&icql, uin);
			log(_("+ authorization sent to %ul"), uin);
			break;
		    case ICQM_ADDED: break;
		}
		fin = true;
		break;
	    case 3:
		fin = true;
		break;
	}
    }

    return proceed;
}

int icqface::showicq(unsigned int uin, string text, char imt, int options = 0) {
    int i, b;
    bool hist = (options & HIST_HISTORYMODE);
    dialogbox db;

    saveworkarea();
    clearworkarea();

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+2, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));
    db.setbrowser(new textbrowser(conf.getcolor(cp_main_text)));

    if(imt == ICQM_REQUEST) {
	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	    _("Authorization request from %lu"), uin);

	db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	    conf.getcolor(cp_main_selected),
	    _("Details"), _("Add to the list"), _("Accept"),
	    hist ? _("Ok") : _("Ignore"), 0));
    } else if(imt == ICQM_ADDED) {
	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	    _("You were added to %lu's list"), uin);

	db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	    conf.getcolor(cp_main_selected), 
	    _("Details"), _("Add to the list"),
	    hist ? _("Ok") : _("Next"), 0));
    }

    if(db.getbar()) {
	db.getbar()->item = hist ? 3 : 2;
	db.redraw();

	workarealine(WORKAREA_Y1+2);
	workarealine(WORKAREA_Y2-2);

	db.getbrowser()->setbuf(text.c_str());
	if(!db.open(i, b)) b = -1;
	db.close();
    }

    restoreworkarea();
    return b;
}

bool icqface::showevent(unsigned int uin, int direction, time_t &lastread) {
    int event, dir, i, fsize, n;
    string text, url;
    struct tm tm;
    bool ret, proceed;
    unsigned long seq;
    icqcontactmsg *cont;

    if(ret = ((n = hist.readevent(event, lastread, tm, dir)) != -1)) {
	if(((dir == HIST_MSG_IN) && (direction & HIST_MSG_IN)) || ((dir == HIST_MSG_OUT) && (direction & HIST_MSG_OUT)))
	switch(event) {
	    case EVT_MSG:
		proceed = true;
		hist.getmessage(text);
		if(checkicqmessage(uin, text, ret, direction)) {
		    i = showmsg(uin, text, *localtime(&lastread),
			tm, dir, direction & HIST_HISTORYMODE);

		    switch(i) {
			case -1: ret = false; break;
			case 0:
			    cicq.fwdmsg(dir == HIST_MSG_IN ? uin : icql.icq_Uin, text);
			    break;
			case 1:
			    text = conf.getquote() ? quotemsg(text) : "";

			    if(face.editmsg(uin, text)) {
				cicq.sendmsg(uin, text);
			    }
			    break;
			case 2: break;
		    }
		}
		break;
	    case EVT_URL:
		hist.geturl(url, text);
		i = showurl(uin, url, text, *localtime(&lastread), tm, dir, direction & HIST_HISTORYMODE);
		switch(i) {
		    case -1: ret = false; break;
		    case 0:
			text = url + "\n\n" + text;
			cicq.fwdmsg(dir == HIST_MSG_IN ? uin : icql.icq_Uin, text);
			break;
		    case 1: break;
		}
		break;
	    case EVT_FILE:
		hist.getfile(seq, text, fsize);
		i = showfile(uin, seq, text, fsize, *localtime(&lastread), tm, dir, direction & HIST_HISTORYMODE);
		switch(i) {
		    case -1: ret = false; break;
		    case 0: acceptfile(uin, seq, text); break;
		    case 1: refusefile(uin, seq); break;
		}
		break;
	    case EVT_CONTACT:
		hist.getcontact(&cont);
		i = showcontact(uin, cont, *localtime(&lastread), tm, dir, direction & HIST_HISTORYMODE);
		switch(i) {
		    case -1: ret = false; break;
		}
		break;
	}
    } else {
	ret = false;
    }

    return ret;
}

int icqface::showmsg(unsigned int uin, string text, struct tm &recvtm,
struct tm &senttm, int inout = HIST_MSG_IN, bool inhistory = false) {
    int i, b;
    time_t t;
    char buf[512];
    icqcontact *c = clist.get(uin);
    dialogbox db;

    saveworkarea();
    clearworkarea();

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+3, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));

    if((!inhistory && !c->getmsgcount()) || inhistory) {
	db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	    conf.getcolor(cp_main_selected), _("Fwd"), _("Reply"), _("Done"), 0));
	db.getbar()->item = 2;
    } else {
	db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	    conf.getcolor(cp_main_selected), _("Fwd"), _("Reply"), _("Next"), 0));
	db.getbar()->item = 2;
    }

    if(inout == HIST_MSG_IN) {
	char srecv[64], ssent[64];

	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	_("Message from %s, %lu"), c->getdispnick().c_str(), uin);

	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1+1,
	    conf.getcolor(cp_main_highlight), _("%s, rcvd on %s"),
	    time2str(&(t = mktime(&senttm)), "DD.MM.YY hh:mm", ssent),
	    time2str(&(t = mktime(&recvtm)), "DD.MM.YY hh:mm", srecv));
    } else {
	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	_("Message to %s, %lu"), c->getdispnick().c_str(), uin);

	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1+1, conf.getcolor(cp_main_highlight),
	_("Sent on %s"), time2str(&(t = mktime(&recvtm)), "DD.MM.YY hh:mm", buf));
    }

    db.idle = &dialogidle;
    db.setbrowser(new textbrowser(conf.getcolor(cp_main_text)));
    db.redraw();

    workarealine(WORKAREA_Y1+3);
    workarealine(WORKAREA_Y2-2);

    db.getbrowser()->setbuf(text.c_str());
    if(!db.open(i, b)) b = -1;
    db.close();

    restoreworkarea();
    return b;
}

int icqface::showurl(unsigned int uin, string url, string text,
struct tm &recvtm, struct tm &senttm, int inout = HIST_MSG_IN,
bool inhistory = false) {
    int i, b;
    time_t t;
    char buf[512];
    icqcontact *c = clist.get(uin);
    dialogbox db;

    saveworkarea();
    clearworkarea();

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+5, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));

    if(inout == HIST_MSG_IN) {
	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	_("URL from %s, %lu"), c->getdispnick().c_str(), uin);

	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1+1, conf.getcolor(cp_main_highlight), _("%s, rcvd on %s"),
	time2str(&(t = mktime(&recvtm)), "DD.MM.YY hh:mm", buf),
	time2str(&(t = mktime(&senttm)), "DD.MM.YY hh:mm", buf));
    } else {
	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	_("URL to %s, %lu"), c->getdispnick().c_str(), uin);

	mainw.writef(WORKAREA_X1+2, WORKAREA_Y1+1, conf.getcolor(cp_main_highlight),
	_("Sent on %s"), time2str(&(t = mktime(&recvtm)), "DD.MM.YY hh:mm", buf));
    }

    mainw.write(WORKAREA_X1+2, WORKAREA_Y1+3, conf.getcolor(cp_main_text), url);

    if(inhistory || (!inhistory && !c->getmsgcount())) {
	db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	    conf.getcolor(cp_main_selected), _("Fwd"), _("Done"), 0));
	db.getbar()->item = 1;
    } else {
	db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	    conf.getcolor(cp_main_selected), _("Fwd"), _("Next"), 0));
	db.getbar()->item = 1;
    }

    db.idle = &dialogidle;
    db.setbrowser(new textbrowser(conf.getcolor(cp_main_text)));
    db.redraw();

    workarealine(WORKAREA_Y1+3);
    workarealine(WORKAREA_Y1+5);
    workarealine(WORKAREA_Y2-2);

    db.getbrowser()->setbuf(text.c_str());
    if(!db.open(i, b)) b = -1;
    db.close();
    restoreworkarea();

    return b;
}

int icqface::showfile(unsigned int uin, unsigned long seq, string fname,
int fsize, struct tm &recvtm, struct tm &senttm, int inout = HIST_MSG_IN,
bool inhistory = false) {
    int i, b;
    icqcontact *c = clist.get(uin);
    dialogbox db;

    saveworkarea();
    clearworkarea();

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+2, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));
    db.setbrowser(new textbrowser(conf.getcolor(cp_main_text)));

    mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
    _("File from %s, %lu"), c->getdispnick().c_str(), uin);

    if(inhistory)
	db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	conf.getcolor(cp_main_selected), _("Ok"), 0));
    else
	db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	conf.getcolor(cp_main_selected), _("Accept"), _("Refuse"), 
	c->getmsgcount() ? _("Next") : _("Done"), 0));

    db.redraw();
    workarealine(WORKAREA_Y1+2);
    workarealine(WORKAREA_Y2-2);

    db.idle = &dialogidle;
    db.getbrowser()->setbuf(fname.c_str());
    if(!db.open(i, b)) b = -1;
    db.close();

    restoreworkarea();

    if(inhistory) b = 2;
    return b;
}

int icqface::showcontact(unsigned int uin, icqcontactmsg *cont, struct tm &recvtm,
struct tm &senttm, int inout = HIST_MSG_IN, bool inhistory = false) {
    int n, b, i;
    dialogbox db;
    icqcontactmsg *cur, *prev;
    icqcontact *c = clist.get(uin);
    linkedlist lst;

    lst.freeitem = &nothingfree;
    for(cur = cont; cur; cur = (icqcontactmsg *) cur->next) {
	lst.add(cur);
    }

    saveworkarea();
    clearworkarea();

    mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
    _("Contacts from %s, %lu"), c->getdispnick().c_str(), uin);

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+2, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));
    db.setmenu(new verticalmenu(conf.getcolor(cp_main_text),
	conf.getcolor(cp_main_selected)));
    db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	conf.getcolor(cp_main_selected), _("Details"), _("Add"),
	inhistory ? _("Ok") : _("Next"), 0));

    db.getbar()->item = 2;
    db.idle = &dialogidle;
    db.redraw();
    workarealine(WORKAREA_Y1+2);
    workarealine(WORKAREA_Y2-2);

    for(bool fin = false; !fin; ) {
	db.getmenu()->clear();
	for(i = 0; i < lst.count; i++) {
	    cur = (icqcontactmsg *) lst.at(i);
	    db.getmenu()->additemf(" %15lu   %s", cur->uin, cur->nick);
	}

	if(fin = !db.open(n, b)) b = -1; else
	if(cur = (icqcontactmsg *) lst.at(n-1))
	switch(b) {
	    case 0:
		cicq.userinfo(cur->uin);
		break;
	    case 1:
		if(!clist.get(cur->uin)) {
		    clist.addnew(cur->uin, false);
		    c = clist.get(cur->uin);
		    c->setdispnick(cur->nick);
		}

		lst.remove(n-1);
		break;
	    case 2:
		fin = true;
		break;
	}        
    }

    db.close();
    restoreworkarea();

    return b;
}

void icqface::acceptfile(unsigned int uin, unsigned long seq, string fname) {
    if(ihook.logged()) {
	icq_FileSession *sess;
	static string filesavedir;
	icqcontact *c = (icqcontact *) clist.get(uin);

	if(!filesavedir.size()) filesavedir = (string) getenv("HOME");
	filesavedir = inputstr(_("save file(s) in: "), filesavedir);

	chdir(getenv("HOME"));
	chdir(filesavedir.c_str());

	if(sess = icq_AcceptFileRequest(&icql, uin, seq)) {
	    log(_("+ file %s from %s, %lu started"), justfname(fname).c_str(), c->getdispnick().c_str(), uin);
	    ihook.addfile(uin, seq, fname, HIST_MSG_IN);
	}
    }
}

void icqface::refusefile(unsigned int uin, unsigned long seq) {
    if(ihook.logged()) {
	icqcontact *c = (icqcontact *) clist.get(uin);
	icq_RefuseFileRequest(&icql, uin, seq, "refused");
	log(_("+ file from %s, %lu refused"), c->getdispnick().c_str(), uin);
    }
}

void icqface::history(unsigned int uin) {
    int k, savepos, b;
    icqcontact *c = clist.get(uin);
    dialogbox db;

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+2, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));
    db.setmenu(new verticalmenu(conf.getcolor(cp_main_text),
	conf.getcolor(cp_main_selected)));
    db.idle = &dialogidle;

    hist.fillmenu(uin, db.getmenu());

    if(hist.opencontact(uin)) {
	saveworkarea();
	clearworkarea();

	db.redraw();
	workarealine(WORKAREA_Y1+2);

	if(uin) {
	    mainw.writef(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	    _("History items for %s, %lu"), c->getdispnick().c_str(), uin);
	} else {
	    mainw.write(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	    _("ICQ system messages history"));
	}

	while(db.open(k, b)) {
	    int n = (int) db.getmenu()->getref(k-1);
	    time_t lr;
	    hist.setpos(n);
	    showevent(uin, HIST_MSG_IN | HIST_MSG_OUT | HIST_HISTORYMODE, lr);
	}

	hist.closecontact();
	db.close();
	restoreworkarea();
    } else {
	log(_("+ no history items for %s, %lu"), c->getdispnick().c_str(), uin);
    }

    db.close();
}

void icqface::read(unsigned int uin) {
    icqcontact *c = clist.get(uin);

    saveworkarea();

    if(hist.opencontact(uin)) {
	if(hist.setposlastread(c->getlastread()) != -1) {
	    time_t lastread = 0;
	    while(showevent(uin, HIST_MSG_IN, lastread));

	    c->setlastread(lastread);
	    c->save();
	}

	hist.closecontact();
    }

    restoreworkarea();
    update();
}

void icqface::modelist(contactstatus cs) {
    int i, b;
    icqcontact *c;
    icqlistitem *it;
    dialogbox db;
    list<unsigned int>::iterator ic;

    saveworkarea();
    clearworkarea();

    db.setwindow(new textwindow(WORKAREA_X1, WORKAREA_Y1+2, WORKAREA_X2,
	WORKAREA_Y2, conf.getcolor(cp_main_text), TW_NOBORDER));
    db.setmenu(new verticalmenu(conf.getcolor(cp_main_text),
	conf.getcolor(cp_main_selected)));
    db.setbar(new horizontalbar(conf.getcolor(cp_main_highlight),
	conf.getcolor(cp_main_selected),
	_("Details"), _("Add"), _("Remove"), _("Move to contacts"), 0));

    db.idle = &dialogidle;
    db.addkey(KEY_IC, 1);
    db.addkey(KEY_DC, 2);

    db.redraw();
    workarealine(WORKAREA_Y1+2);
    workarealine(WORKAREA_Y2-2);

    mainw.write(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight),
	cs == csignore ? _("ICQ Ignore list") :
	cs == csvisible ? _("ICQ Visible list") : _("ICQ Invisible list"));

    lst.fillmenu(db.getmenu(), cs);

    while(db.open(i, b)) {
	it = i ? (icqlistitem *) lst.menuat(i-1) : 0;

	switch(b) {
	    case 0:
		if(it) cicq.userinfo(it->getuin());
		break;
	    case 1:
		muins.clear();

		if(multicontacts(_("Select contacts to add to the list"), muins)) {
		    for(ic = muins.begin(); ic != muins.end(); ic++) {
			lst.add(new icqlistitem(clist.get(*ic)->getdispnick(), *ic, cs));
			if(cs == csignore) clist.remove(*ic);
		    }

		    lst.fillmenu(db.getmenu(), cs);
		    db.getmenu()->redraw();
		}
		break;
	    case 2:
		if(it) {
		    lst.del(it->getuin(), cs);
		    lst.fillmenu(db.getmenu(), cs);
		    db.getmenu()->redraw();
		}
		break;
	    case 3:
		if(it) {
		    if(!clist.get(it->getuin())) {
			clist.addnew(it->getuin(), false);
			c = clist.get(it->getuin());
			c->setdispnick(it->getnick());
		    }

		    lst.del(it->getuin(), cs);
		    lst.fillmenu(db.getmenu(), cs);
		    db.getmenu()->redraw();
		    face.update();
		}
		break;
	}
    }

    db.close();
    restoreworkarea();
    clist.send();
}

bool icqface::multicontacts(string head, list<unsigned int> &lst) {
    int i, savefirst, saveelem;
    unsigned int ffuin;
    bool ret = true, finished = false;
    list<unsigned int>::iterator c;

    verticalmenu m(WORKAREA_X1+1, WORKAREA_Y1+3, WORKAREA_X2, WORKAREA_Y2,
	conf.getcolor(cp_main_text), conf.getcolor(cp_main_selected));

    saveworkarea();
    clearworkarea();

    workarealine(WORKAREA_Y1+2);

    if(!head.size()) head = _("Event recipients");
    mainw.write(WORKAREA_X1+2, WORKAREA_Y1, conf.getcolor(cp_main_highlight), head);

    m.idle = &menuidle;
    m.otherkeys = &multiplekeys;

    while(!finished) {
	m.getpos(saveelem, savefirst);
	m.clear();

	for(i = 0; i < clist.count; i++) {
	    icqcontact *c = (icqcontact *) clist.at(i);

	    if(c->getuin())
		m.additemf(0, (void *) c->getuin(), " [%c] %s",
		    find(lst.begin(), lst.end(), c->getuin()) != lst.end()
		    ? 'x' : ' ', c->getdispnick().c_str());
	}

	m.setpos(saveelem, savefirst);

	switch(m.open()) {
	    case -2:
		if(ffuin = (unsigned int) m.getref(m.getpos())) {
		    c = find(lst.begin(), lst.end(), ffuin);
		    if(c != lst.end()) lst.erase(c); else lst.push_back(ffuin);
		}
		break;
	    case  0:
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

void icqface::log(string text) {
    int i;

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

void icqface::status(string text) {
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

string icqface::quotemsg(string text) {
    string ret;
    vector<string> lines;
    vector<string>::iterator i;

    formattext(text, lines, true, WORKAREA_X2-WORKAREA_X1-4);

    for(i = lines.begin(); i != lines.end(); i++) {
	if(!i->empty()) ret += (string) "> " + *i;
	ret = trailcut(ret);
	ret += "\n";
    }

    return ret;
}

void icqface::quickfind() {
    bool fin;
    string nick, disp;
    int k, i, len;
    bool found;
    icqcontact *c;

    status(_("QuickSearch: type to find, Alt-S find again, Enter finish"));

    for(fin = false; !fin; ) {
	attrset(conf.getcolor(cp_main_frame));
	mvhline(LINES-2, 2, HLINE, 23);
	disp = nick;
	if(disp.size() > 18) disp.replace(0, disp.size()-18, "");
	kwriteatf(2, LINES-2, conf.getcolor(cp_main_highlight), "[ %s ]", disp.c_str());
	kgotoxy(4+disp.size(), LINES-2);
	refresh();

	if(ihook.idle())
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
		    i = mcontacts->menu.getpos()+2;

		    if(isprint(k)) {
			nick += k;
			i--;
		    }

		    bool fin = false;
		    bool fpass = true;

		    for(; !fin; i++) {
			if(i >= mcontacts->menu.getcount()) {
			    if(fpass) {
				i = 0;
				fpass = false;
			    } else {
				fin = true;
			    }
			}

			c = (icqcontact *) mcontacts->getref(i);

			if((int) c > 10) {
			    len = c->getdispnick().size();
			    if(len > nick.size()) len = nick.size();

			    if(!c->getdispnick().compare(nick, 0, len)) {
				mcontacts->menu.setpos(i-1);
				break;
			    }
			}
		    }

		    mcontacts->redraw();
		}
		break;
	}
    }

    attrset(conf.getcolor(cp_main_frame));
    mvhline(LINES-2, 2, HLINE, 23);
}

// ----------------------------------------------------------------------------

void icqface::menuidle(verticalmenu *m) {
    ihook.idle();
}

void icqface::dialogidle(dialogbox *caller) {
    ihook.idle();
}

void icqface::textbrowseridle(textbrowser *b) {
    ihook.idle();
}

int icqface::contactskeys(verticalmenu *m, int k) {
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

	case 'r':
	case 'R': face.extk = ACT_RENAME; break;
	
	case 'v':
	case 'V': face.extk = ACT_SWITCH_VIS; break;

	case ALT('s'): face.extk = ACT_QUICKFIND; break;
    }

    if(strchr("?rRqQsShHmMuUgGaAfFvV", k)
	|| (k == KEY_DC)
	|| (k == KEY_F(2))
	|| (k == KEY_F(3))
	|| (k == KEY_F(4))
	|| (k == ALT('s'))) {
	return m->getpos()+1;
    } else {
	return -1;
    }
}

int icqface::multiplekeys(verticalmenu *m, int k) {
    switch(k) {
	case ' ':
	case 'x':
	case 'X':
	    return -2;
    }
    return -1;
}

int icqface::editmsgkeys(texteditor *e, int k) {
    char *p;

    switch(k) {
	case CTRL('x'):
	    p = e->save("");
	    face.editdone = strlen(p);
	    delete p;
	    if(face.editdone) return -1; else break;
	case CTRL('p'): face.multicontacts("", face.muins); break;
	case CTRL('o'): face.history(face.passuin); break;
	case ALT('?'): cicq.userinfo(face.passuin); break;
	case 27: return -1;
    }
    return 0;
}

void icqface::editidle(texteditor *e) {
    ihook.idle();
}

void icqface::textinputidle(textinputline *il) {
    ihook.idle();
}

// ----------------------------------------------------------------------------

icqprogress::icqprogress(): curline(0), w(0) {
}

icqprogress::~icqprogress() {
    delete w;
}

void icqprogress::log(const char *fmt, ...) {
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

void icqprogress::show(string title = "") {
    if(!w) {
	w = new textwindow(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT,
	conf.getcolor(cp_dialog_frame), TW_CENTERED);
    }

    w->set_title(conf.getcolor(cp_dialog_highlight), title.c_str());
    w->open();
}

void icqprogress::hide() {
    w->close();
}
