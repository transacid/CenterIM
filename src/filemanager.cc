/*
*
* kkconsui filemanager class
* $Id: filemanager.cc,v 1.2 2001/06/03 21:12:05 konst Exp $
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

#include "filemanager.h"

#define MAXLEN 8192

filemanager *fmexemplair;

filemanager::filemanager(int nx1, int ny1, int nx2, int ny2, int nfclr,
    int ndclr, int nfsclr, int sclr, int wclr):
multi(false), fullpath(true),
onlydirs(false), chroot(false) {
    x1 = nx1; y1 = ny1;
    x2 = nx2; y2 = ny2;
    nfcolor = nfclr;
    nfscolor = nfsclr;
    ndcolor = ndclr;
    scolor = sclr;
    wcolor = wclr;

    startpoint = new char[MAXLEN];
    savepoint = new char[MAXLEN];
    startpoint[0] = savepoint[0] = 0;

    items = new linkedlist;
    items->freeitem = &freefe;

    selected = new linkedlist;
    selected->freeitem = &charpointerfree;

    w = new textwindow(x1, y1, x2, y2, wcolor);
}

filemanager::~filemanager() {
    delete items;
    delete startpoint;
    delete savepoint;
    delete selected;
    delete w;
}

int filemanager::sortfilelist(void *p1, void *p2) {
    int res = 0;
    struct fmentry *fm1 = (fmentry *) p1;
    struct fmentry *fm2 = (fmentry *) p2;
    
    if(fm1->isdir && !fm2->isdir) res = 1; else
    if(!fm1->isdir && fm2->isdir) res = -1;

    if(!res) res = -strcmp(fm1->fname, fm2->fname);
    return res;
}

int filemanager::findfname(void *p1, void *p2) {
    const char *p = justfname((char *) p1).c_str();
    return strcmp(p, ((fmentry *) p2)->fname);
}

int filemanager::findstr(void *p1, void *p2) {
    return strcmp((char *) p1, (char *) p2);
}

void filemanager::freefe(void *p) {
    fmentry *fm = (fmentry *) p;
    delete fm->fname;
    delete fm;
}

void filemanager::setmask(const char *exp) {
}

void filemanager::setstartpoint(const char *dir) {
    strcpy(startpoint, dir);
    strcpy(savepoint, dir);
    while(savepoint[strlen(savepoint)-1] == '/') {
	savepoint[strlen(savepoint)-1] = 0;
    }
}

bool filemanager::fillmenu() {
    DIR *d;
    struct dirent *e;
    struct stat st;
    struct fmentry *fe;
    char *fname = new char[MAXLEN];
    int i;
    bool ret = false;

    if(d = opendir(startpoint)) {
	items->empty();
	m->clear();
	
	while(e = readdir(d)) {
	    if(strcmp(e->d_name, ".")) {
		if(chroot && !strcmp(startpoint, savepoint) && !strcmp(e->d_name, "..")) continue;
	    
		fe = new fmentry;
		sprintf(fname, "%s/%s", startpoint, fe->fname = strdup(e->d_name));

		lstat(fname, &st);
		fe->islink = (bool) S_ISLNK(st.st_mode);

		stat(fname, &st);
		fe->isdir = (bool) S_ISDIR(st.st_mode);
		fe->size = st.st_size;
		fe->date = st.st_mtime;
		
		if((onlydirs && fe->isdir) || !onlydirs) {
		    items->add(fe);
		} else {
		    delete fe;
		}
	    }
	}

	closedir(d);

	items->sort(&sortfilelist);

	for(i = 0; i < items->count; i++) {
	    fe = (fmentry *) items->at(i);
	    strcpy(fname, fe->fname);
	    fname[x2-x1-14] = 0;

	    if(fe->isdir && !strcmp(fe->fname, "..")) {
		m->additemf(ndcolor, 0, " %-*s %10s", x2-x1-14, fname, "up-dir");
	    } else if(fe->isdir && fe->islink) {
		m->additemf(ndcolor, 0, " %-*s %10s", x2-x1-14, fname, "dir,link");
	    } else if(!fe->isdir && fe->islink) {
		m->additemf(nfcolor, 0, " %-*s %10s", x2-x1-14, fname, "link");
	    } else if(fe->isdir && !fe->islink) {
		m->additemf(ndcolor, 0, " %-*s %10s", x2-x1-14, fname, "dir");
	    } else {
		m->additemf(nfcolor, 0, " %-*s %10d", x2-x1-14, fname, fe->size);
	    }
	}

	ret = true;
    }

    delete fname;
    return ret;
}

int filemanager::menukeys(verticalmenu *m, int k) {
    switch(k) {
	case ' ': // space
	    if(fmexemplair->onlydirs) {
		fmexemplair->spacepressed = true;
		return m->getpos()+1;
	    }
	    break;
	case 331: // insert
	    if(fmexemplair->multi) {
		int i = m->getpos();
		struct fmentry *fe = (fmentry *) fmexemplair->items->at(i);

		if(!fe->isdir) {
		    char *p, *fn = ((fmentry *) fmexemplair->items->at(i))->fname;

		    if(fmexemplair->fullpath) {
			p = new char[strlen(fmexemplair->startpoint)+strlen(fn)+2];
			sprintf(p, "%s/%s", fmexemplair->startpoint, fn);
		    } else {
			p = strdup(fn);
		    }

		    if((i = fmexemplair->selected->findnum(p, &findstr)) != -1) {
			fmexemplair->selected->remove(i);
			m->setcolor(i, fmexemplair->nfcolor);
		    } else {
			fmexemplair->selected->add(strdup(p));
			m->setcolor(i, fmexemplair->nfscolor);
		    }

		    delete p;
		    m->setpos(m->getpos()+1);
		    m->redraw();
		}
	    }
	    break;
    }
    return -1;
}

void filemanager::open() {
    char *curdir = new char[MAXLEN];
    int ch, r;
    
    savescr();
    fmexemplair = this;
    spacepressed = false;

    m = new verticalmenu(x1+1, y1+1, x2, y2, nfcolor, scolor);
    
    m->otherkeys = &menukeys;
    w->open();

    selected->empty();
    getcwd(curdir, MAXLEN);
    chdir(startpoint);
    if(!startpoint[0]) strcpy(startpoint, curdir);
    fillmenu();

    while(ch = m->open()) {
	chdir(startpoint);
	struct fmentry *fe = (fmentry *) items->at(ch-1);

	if(fe->isdir && !spacepressed) {
	    char *prevsp = strdup(startpoint);

	    selected->empty();
	    r = chdir(fe->fname);
	    getcwd(startpoint, MAXLEN);

	    if(fillmenu() && !r) {
		m->setpos(ch == 1 ? items->findnum(prevsp, &findfname) : 0);
	    } else {
		r = chdir(prevsp);
		getcwd(startpoint, MAXLEN);
	    }
	    delete prevsp;
	} else {
	    char *fn = ((fmentry *) items->at(ch-1))->fname, *p;
	    if(!multi) {
		if(fullpath) {
		    p = new char[strlen(startpoint)+strlen(fn)+2];
		    sprintf(p, "%s/%s", startpoint, fn);
		} else {
		    p = strdup(fn);
		}
		
		selected->add(p);
	    }
	    break;
	}
    }

    chdir(curdir);
    m->close();
    w->close();
    delete m;
    delete curdir;
}

