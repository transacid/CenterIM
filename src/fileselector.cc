#include "fileselector.h"

#include <sys/types.h>
#include <dirent.h>

#include <algorithm>

static fileselector *instance;

fileselector::fileselector() {
}

fileselector::~fileselector() {
}

void fileselector::setcolor(int acnormal, int acselected, int accurrent,
int acfile, int acdir = 0, int acframe = 0) {
    cnormal = acnormal;
    cselected = acselected;
    cfile = acfile;
    ccurrent = accurrent;

    if(!(cdir = acdir)) cdir = acfile;
    if(!(cframe = acframe)) cframe = acnormal;
}

void fileselector::generatemenu() {
    DIR *d;
    struct dirent *de;
    struct stat st;
    list<item>::iterator i;
    vector<string> mitems;
    string prefix, fname;

    m.clear();
    items.clear();

    if(dcurrent.substr(dcurrent.size()-1) != "/")
	dcurrent += "/";

    if(d = opendir(dcurrent.c_str())) {
	while(de = readdir(d)) {
	    fname = dcurrent + de->d_name;

	    if(options & FSEL_DIRSELECT)
		if(!stat(fname.c_str(), &st))
		    if(!S_ISDIR(st.st_mode))
			continue;

	    if(options & FSEL_CHROOT)
		if(dcurrent == spoint)
		    if((string) de->d_name == "..")
			continue;

	    if((string) de->d_name != ".")
	    if(!lstat(fname.c_str(), &st)) {
		items.push_back(item(de->d_name, st.st_mode));
	    }
	}

	closedir(d);
    }

    items.sort();

    for(i = items.begin(); i != items.end(); i++) {
	m.additem(0, (void *) &(*i), i->getprefix() + i->fname);

	if(find(selected.begin(), selected.end(),
	dcurrent + i->fname) != selected.end()) {
	    m.setcolor(m.getcount()-1, cselected);
	}
    }
}

void fileselector::exec() {
    int n, mode;
    item *i;
    char buf[512];
    dhistitem di;
    vector<string>::iterator is;
    bool fin;

    m = verticalmenu(cnormal, ccurrent);
    m.setwindow(w);
    m.otherkeys = &menukeys;

    selected.clear();

    if(spoint.empty()) {
	spoint = getcwd(buf, 512);
    }

    if(dcurrent.empty()) {
	dcurrent = spoint;
    }

    if(chdir(dcurrent.c_str())) {
	chdir((dcurrent = "/").c_str());
    }

    if(spoint.substr(spoint.size()-1) != "/")
	spoint += "/";

    for(fin = finish = false; !fin; ) {
	generatemenu();
	instance = &(*this);
	fin = !(n = m.open()) || finish;

	if(!fin) {
	    if(i = (item *) m.getref(n-1)) {
		if(!chdir(i->fname.c_str())) {
		    di = dhistitem();
		    di.dirname = dcurrent;
		    m.getpos(di.pos, di.fpos);

		    dcurrent = getcwd(buf, 512);
		    m.setpos(0);

		    if(i->fname == "..") {
			if(!dhist.empty()) {
			    di = *(dhist.end()-1);
			    dhist.erase(dhist.end()-1);
			    chdir((dcurrent = di.dirname).c_str());
			    m.setpos(di.pos, di.fpos);
			}
		    } else {
			dhist.push_back(di);
		    }
		} else {
		    if(!(options & FSEL_MULTI)) {
			selected.push_back(dcurrent + i->fname);
		    }

		    fin = true;
		}
	    }
	}
    }

    if(options & FSEL_CHROOT) {
	for(is = selected.begin(); is != selected.end(); is++) {
	    is->erase(0, spoint.size());
	}
    }
}

void fileselector::close() {
    m.close();
}

int fileselector::menukeys(verticalmenu *m, int k) {
    string fname;
    struct stat st;
    item *i;
    vector<string>::iterator si;

    i = (item *) m->getref(m->getpos());

    switch(k) {
	case ' ':
	    if(instance->options & FSEL_DIRSELECT) {
		if(i) {
		    if(i->fname != "..") {
			instance->selected.push_back(instance->dcurrent + i->fname);
			instance->finish = true;
			return m->getpos()+1;
		    }
		}
	    }
	    break;

	case KEY_IC:
	    if(instance->options & FSEL_MULTI) {
		if(i) {
		    if(!stat(i->fname.c_str(), &st)) {
			fname = instance->dcurrent + i->fname;
			si = find(instance->selected.begin(),
			    instance->selected.end(), fname);

			if(si != instance->selected.end()) {
			    instance->selected.erase(si);
			    m->setcolor(m->getpos(), instance->cnormal);
			} else {
			    instance->selected.push_back(fname);
			    m->setcolor(m->getpos(), instance->cselected);
			}

			m->setpos(m->getpos()+1);
			m->redraw();
		    }
		}
	    }
	    break;
    }

    return -1;
}

// ----------------------------------------------------------------------------

const string fileselector::item::getprefix() const {
    string prefix = " ";
    struct stat st;

    if(S_ISLNK(mode)) {
	if(!stat(fname.c_str(), &st)) {
	    prefix = S_ISDIR(st.st_mode) ? "~" : "@";
	}
    } else if(S_ISDIR(mode)) {
	prefix = "/";
    } else if(S_ISREG(mode)) {
    } else {
	prefix = "?";
    }

    return prefix;
}

short fileselector::item::compare(const item &aitem) const {
    struct stat st[2];
    short r = 0;

    stat(fname.c_str(), &st[0]);
    stat(aitem.fname.c_str(), &st[1]);

    if(S_ISDIR(st[0].st_mode) && !S_ISDIR(st[1].st_mode)) r = 1; else
    if(!S_ISDIR(st[0].st_mode) && S_ISDIR(st[1].st_mode)) r = -1; else {
	r = -strcmp(fname.c_str(), aitem.fname.c_str());
    }

    return r;
}

bool fileselector::item::operator < (const item &aitem) const {
    return compare(aitem) > 0;
}

bool fileselector::item::operator > (const item &aitem) const {
    return compare(aitem) < 0;
}
