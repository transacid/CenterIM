/*
*
* kkconsui common routines
* $Id: conscommon.cc,v 1.1 2001/06/27 13:42:07 konst Exp $
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

#include "conscommon.h"

bool kintf_graph = true, kintf_refresh = true;

void (*kinputidle)(void);

void kreinit(int sn) {
    struct winsize size;

    if(ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0) {
	resizeterm(size.ws_row, size.ws_col);
	wrefresh(curscr);
    }

    signal(sn, &kreinit);
}

void kinterface() {
    initscr();
    nonl();
    cbreak();
    noecho();
    keypad(stdscr, 1);
    raw();
    
    start_color();
    atexit(kendinterface);
//      nodelay(stdscr, TRUE);
    ESCDELAY = 0;
    signal(SIGWINCH, &kreinit);
}

void kendinterface() {
    endwin();
}

int getkey() {
    int k, n, fin = 0, r = 0;
    fd_set rd;
    struct timeval tv;

    refresh();

    FD_ZERO(&rd);
    FD_SET(STDIN_FILENO, &rd);

    while(!fin) {
	while(!select(STDIN_FILENO+1, &rd, 0, 0, 0));
	ioctl(STDIN_FILENO, FIONREAD, &r);

	switch(k = getch()) {
	    case 27:
		tv.tv_sec = 10;
		tv.tv_usec = 0;

		if(r <= 1) r = select(STDIN_FILENO+1, &rd, 0, 0, &tv);

		if(r) {
		    n = getch();
		    if(isdigit(n)) k = KEY_F(n-'0' ? n-'0' : 10); else
		    if(n != 27) k = ALT(n);
		} else {
		    k = 0;
		}

		fin = 1;
		break;
	    case CTRL('l'):
		endwin();
		refresh();
		break;
	    default:
		fin = 1;
		break;
	}
    }

    return k;
}

int getctrlkeys() {
#ifdef __linux__
    unsigned char modifiers = 6;
    if(ioctl(0, TIOCLINUX, &modifiers) < 0) return 0;
    return (int) modifiers;
#else
    return 0;
#endif
}

int emacsbind(int k) {
    switch(k) {
	case CTRL('b'): return KEY_LEFT;
	case CTRL('f'): return KEY_RIGHT;
	case CTRL('n'): return KEY_DOWN;
	case CTRL('p'): return KEY_UP;
	case CTRL('d'): return KEY_DC;
	case CTRL('a'): return KEY_HOME;
	case CTRL('e'): return KEY_END;
	case CTRL('v'): return KEY_NPAGE;
	case CTRL('k'): return CTRL('y');
	default: return k;
    }
}

void ktool::printchar(char c) {
    printw("%c", !iscntrl(c) ? c : ' ');
}

void ktool::printstring(string s) {
    int i;

    for(i = 0; i < s.size(); i++)
    if(iscntrl(s[i])) s[i] = ' ';

    printw("%s", s.c_str());
}

char *kinput(unsigned int size, char *buf) {
    int c, x, y;
    unsigned int offset = 0, i;
    char t[size], go, finish = 0;

    refresh();
    strcpy(buf, "");

    while(!finish) {
	if(kinputidle) go = keypressed(); else go = 1;
      
	if(go) switch(c = getkey()) {
	    case '\r' :
		finish = 1;
		break;
	  
	    case KEY_LEFT :
		if(offset > 0) {
		    offset--;
		    kgotoxy(kwherex() - 1, kwherey());
		}
		break;

	    case KEY_RIGHT :
		if(offset < strlen(buf)) printchar(buf[offset++]);
		break;

	    case KEY_DC :
		if(strlen(buf) > 0 && offset < strlen(buf)) {
		    x = kwherex(); y = kwherey();
		    for(i = offset + 1; i < strlen(buf); i++) printchar(buf[i]);
		    printw(" ");
		    kgotoxy(x, y);
		    refresh();

		    t[0] = 0;
		    for(i = 0; i < strlen(buf); i++)
		    if(i != offset) sprintf(t, "%s%c", t, buf[i]);
		    strcpy(buf, t);
		}
		break;
	  
	    case KEY_BACKSPACE : case 8 :
		if(strlen(buf) && offset > 0) {
		    offset--;
		    kgotoxy(kwherex() - 1, kwherey());
		    x = kwherex(); y = kwherey();
		    for(i = offset + 1; i < strlen(buf); i++) printchar(buf[i]);
		    printw(" ");
		    kgotoxy(x, y);
		    refresh();

		    t[0] = 0;
		    for(i = 0; i < strlen(buf); i++)
		    if(i != offset) sprintf(t, "%s%c", t, buf[i]);
		    strcpy(buf, t);
		}
		break;
      
	    default :
		if(strlen(buf) < size && c > 31 && c < 255) {
		    printchar(c);
		    refresh();
		    offset++;
		    sprintf(buf, "%s%c", buf, c);
		}
	} else if(kinputidle) (*kinputidle)();
    }
    return buf;
}

int kwherex() {
    return getcurx(stdscr);
}

int kwherey() {
    return getcury(stdscr);
}

void kwriteatf(int x, int y, int c, const char *fmt, ...) {
    char buf[10240];
    va_list ap;
    
    va_start(ap, fmt);    
    attrset(c);
    vsprintf(buf, fmt, ap);
    mvprintw(y, x, "");
    printstring(buf);
    refresh();
}

void kwriteat(int x, int y, const char *msg, int c) {
    attrset(c);
    mvprintw(y, x, "");
    printstring(msg);
    refresh();
}

int keypressed() {
    struct timeval tv;
    fd_set readfds;
    tv.tv_sec = tv.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    select(1, &readfds, 0, 0, &tv);

    return FD_ISSET(0, &readfds);
}

void kgotoxy(int x, int y) {
    move(y, x);
}

void setbeep(int freq, int duration) {
    // freq: 21-32766
    // duration: 0-2000
    // freq or duration == -2  ==>  set the default value

    if(freq >= 21) printf("\e[10;%d]", freq); else
    if(freq == -2) printf("\e[10]");

    if(duration >= 0) printf("\e[11;%d]", duration); else
    if(duration == -2) printf("\e[11]");

    fflush(stdout);
}

void delay(int milisec) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = milisec*1000;
    select(0, 0, 0, 0, &tv);
}

/*
chtype **savelines(int x1, int y1, int x2, int y2) {
    int i;
    chtype **scrbuf = (chtype **) malloc(sizeof(chtype *) * (y2-y1+1));

    for(i = 0; i <= y2-y1; i++) scrbuf[i] = (chtype *) malloc(sizeof(chtype) * (x2-x1+2));
    for(i = 0; i <= y2-y1; i++) mvinchnstr(y1+i, x1, scrbuf[i], x2-x1+1);

    return scrbuf;
}

void restorelines(chtype **scrbuf, int x1, int y1, int x2, int y2) {
    int i;

    if(scrbuf) {
	for(i = 0; i <= y2-y1; i++) mvaddchnstr(y1+i, x1, scrbuf[i], x2-x1+1);
	for(i = 0; i <= y2-y1; i++) free((chtype *) scrbuf[i]);
	free(scrbuf);
	scrbuf = 0;
	refresh();
    }
}
*/
