/*
*
* kkconsui abstract ui control class
* $Id: abstractui.cc,v 1.4 2001/08/06 21:32:45 konst Exp $
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
    fisopen = emacs = false;
    x1 = x2 = y1 = y2 = 0;
}

abstractuicontrol::abstractuicontrol(const abstractuicontrol &a) {
    int i;

    fisopen = a.fisopen;
    emacs = a.emacs;
    x1 = a.x1;
    x2 = a.x2;
    y1 = a.y1;
    y2 = a.y2;
}

abstractuicontrol::~abstractuicontrol() {
}

bool abstractuicontrol::empty() {
    return !x1 && !x2 && !y1 && !y2;
}

void abstractuicontrol::redraw() {
}

void abstractuicontrol::close() {
    screenbuffer.restore();
    fisopen = false;
}

bool abstractuicontrol::isopen() {
    return fisopen;
}
