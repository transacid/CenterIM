/*
*
* kkconsui abstract ui control class
* $Id: abstractui.cc,v 1.2 2001/06/03 21:12:05 konst Exp $
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

#include "abstractui.h"

abstractuicontrol::abstractuicontrol() {
    isopen = emacs = false;
    x1 = x2 = y1 = y2 = 0;
    scrbuf = 0;
}

abstractuicontrol::abstractuicontrol(const abstractuicontrol &a) {
    int i;

    isopen = a.isopen;
    emacs = a.emacs;
    x1 = a.x1;
    x2 = a.x2;
    y1 = a.y1;
    y2 = a.y2;
    scrbuf = 0;
/*
    if(a.scrbuf) {
        scrbuf = (chtype **) malloc(sizeof(chtype *) * (y2-y1+1));
        for(i = 0; i <= y2-y1; i++) {
            scrbuf[i] = (chtype *) malloc(sizeof(chtype) * (x2-x1+2));
            memcpy(scrbuf[i], a.scrbuf[i], sizeof(chtype) * (x2-x1+1));
        }
    }
*/
}

abstractuicontrol::~abstractuicontrol() {
    if(scrbuf) {
        for(int i = 0; i <= y2-y1; i++) free((chtype *) scrbuf[i]);
        free(scrbuf);
        scrbuf = 0;
    }
}

void abstractuicontrol::savescr() {
    if(!scrbuf) {
        scrbuf = (chtype **) malloc(sizeof(chtype *) * (y2-y1+1));
        for(int i = 0; i <= y2-y1; i++) {
            scrbuf[i] = (chtype *) malloc(sizeof(chtype) * (x2-x1+2));
            mvinchnstr(y1+i, x1, scrbuf[i], x2-x1+1);
        }
    }
}

void abstractuicontrol::restscr() {
    if(scrbuf) {
        for(int i = 0; i <= y2-y1; i++) {
            mvaddchnstr(y1+i, x1, scrbuf[i], x2-x1+1);
            free((chtype *) scrbuf[i]);
        }

        refresh();
        free(scrbuf);
        scrbuf = 0;
    }
}

bool abstractuicontrol::empty() {
    return !x1 && !x2 && !y1 && !y2;
}

void abstractuicontrol::redraw() {
}

void abstractuicontrol::close() {
    restscr();
}
