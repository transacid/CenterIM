/*
*
* kkconsui textinputline class
* $Id: textinputline.cc,v 1.2 2001/06/03 21:12:05 konst Exp $
*
* Copyright (C) 1999-2001 by Konstantin Klyagin <konst@konst.org.ua>
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

#include "textinputline.h"

textinputline::textinputline(int clr):
ncolor(clr), fm(0), passwordchar(0), idle(0), otherkeys(0) {
    history = new linkedlist;
    history->freeitem = &charpointerfree;
}

textinputline::~textinputline() {
    delete history;
}

void textinputline::drawcheck(bool redraw) {
    bool toleft = pos < visiblestart;
    bool toright = pos >= visiblestart+visiblelen;

    if(toleft || toright) {
	if(toleft)  while((visiblestart -= visiblelen) > pos);
	if(toright) while((visiblestart += visiblelen) < pos);

	if(visiblestart < 0) {
	    visiblestart = 0;
	} else if(visiblestart > strlen(actualstr)-visiblelen) {
	    visiblestart = strlen(actualstr)-visiblelen;
	}

	draw();
    } else {
	mvprintw(y1, x1+pos-visiblestart, "");
	if(redraw) draw(); else refresh();
    }
}

void textinputline::draw() {
    int alen = strlen(actualstr)-visiblestart > visiblelen ? visiblelen : strlen(actualstr)-visiblestart;

    if(passwordchar) {
	memset(visiblestr, passwordchar, visiblelen);
    } else {
	strncpy(visiblestr, actualstr+visiblestart, alen);
    }

    visiblestr[alen] = 0;
    attrset(ncolor);
    mvprintw(y1, x1, "%s", visiblestr);
    for(int i = 0; i < visiblelen-alen; i++) printw(" ");
    mvprintw(y1, x1+pos-visiblestart, "");
    refresh();
}

char *textinputline::open(int x, int y, const char *buf, int pvisiblelen, int pactuallen) {
    bool finished = false, firstpass = true;
    int k, go;
    
    actualstr = new char[ (actuallen = pactuallen) + 1];
    visiblestr = new char[ (visiblelen = pvisiblelen) + 1];
    x1 = x, y1 = y, x2 = x+visiblelen, y2 = y1+visiblelen;
    visiblestart = pos = 0, strcpy(actualstr, buf);
    savescr();
    draw();

    while(!finished) {
	if(idle) go = keypressed(); else go = 1;

	if(go) switch(k = getkey()) {
	    case KEY_LEFT:
		if(pos) {
		    pos--;
		    drawcheck(false);
		}
		break;
		
	    case KEY_RIGHT:
		if(pos != strlen(actualstr)) {
		    pos++;
		    drawcheck(false);
		}
		break;
		
	    case KEY_HOME:
	    case CTRL('a'):
		pos = 0;
		drawcheck(false);
		break;
		
	    case KEY_END:
	    case CTRL('e'):
		pos = strlen(actualstr);
		drawcheck(false);
		break;
		
	    case '\r':
		history->add(strdup(actualstr));
		finished = true;
		break;
		
	    case KEY_DC:
		if(firstpass) {
		    actualstr[0] = 0;
		    pos = 0;
		} else if(pos < strlen(actualstr)) {
		    strcut(actualstr, pos, 1);
		}
		
		drawcheck(true);
		break;
		
	    case CTRL('t'):
		if(fm) {
		    int x = kwherex(), y = kwherey();
		    fm->open();
		    char *p = (char *) fm->selected->at(0);
		    if(p) setbuf(p);
		    kgotoxy(x, y);
		}
		break;
		
	    case KEY_BACKSPACE:
	    case CTRL('h'):
		if(pos > 0) {
		    strcut(actualstr, --pos, 1);
		    drawcheck(true);
		}
		break;
		
	    case 27:
		actualstr[0] = 0;
		finished = true;
		break;
		
	    default:
		if((k > 31) && (k < 256)) {
		    if(firstpass) {
			sprintf(actualstr, "%c", k);
			pos = 1;
			drawcheck(true);
		    } else if(strlen(actualstr) < actuallen) {
			strcinsert(actualstr, pos++, k);
			drawcheck(true);
		    }
		} else if(otherkeys) {
		    if((*otherkeys)(this, k));
		}
	} else {
	    if(idle) (*idle)(this);
	}

	firstpass = false;
    }
    
    restscr();
    delete visiblestr;
    return actualstr;
}

void textinputline::setbuf(const char *buf) {
    visiblestart = pos = 0;
    strncpy(actualstr, buf, actuallen);
    actualstr[actuallen] = 0;
    draw();
}

void textinputline::setpasswordchar(const char npc) {
    passwordchar = npc;
}

void textinputline::close() {
}

