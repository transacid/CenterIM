#include "treeview.h"

treeview::treeview(int nx1, int ny1, int nx2, int ny2, int nbgcolor, int nselectcolor, int nnodecolor, int nleafcolor) {
    bgcolor = nbgcolor;
    selectcolor = nselectcolor;
    nodecolor = nnodecolor;
    leafcolor = nleafcolor;
    init();
    setcoord(nx1, ny1, nx2, ny2);
}

treeview::treeview(int nbgcolor, int nselectcolor, int nnodecolor, int nleafcolor) {
    bgcolor = nbgcolor;
    selectcolor = nselectcolor;
    nodecolor = nnodecolor;
    leafcolor = nleafcolor;
    init();
}

treeview::treeview() {
    init();
}

treeview::~treeview() {
}

void treeview::init() {
    curelem = 0;
    idseq = 0;
    collapsable = false;

    menu = verticalmenu(x1, y1, x2, y2, bgcolor, selectcolor);
    clear();
}

int treeview::addnodef(int parent, int color, void *ref, const char *fmt, ...) {
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    return addnode(parent, color, ref, buf);
}

int treeview::addleaff(int parent, int color, void *ref, const char *fmt, ...) {
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    return addleaf(parent, color, ref, buf);
}

int treeview::addnode(int parent, int color, void *ref, string text) {
    treeviewnode node;

    node.id = idseq++;
    node.text = text;
    node.ref = ref;
    node.parentid = parent;
    node.isnode = true;
    node.isopen = false;
    node.color = color ? color : nodecolor;

    items.push_back(node);
    return node.id;
}

int treeview::addleaf(int parent, int color, void *ref, string text) {
    treeviewnode node;

    node.id = idseq++;
    node.text = text;
    node.ref = ref;
    node.parentid = parent;
    node.isnode = false;
    node.color = color ? color : leafcolor;

    items.push_back(node);
    return node.id;
}

int treeview::getid(int mpos) {
    if((mpos >= 0) && (mpos < refdeps.size())) {
	return refdeps[mpos].id;
    } else {
	return -1;
    }
}

void treeview::opennode(int id) {
    vector<treeviewnode>::iterator i;
    i = find(items.begin(), items.end(), id);

    if(i != items.end()) {
	if(i->isnode && collapsable) i->isopen = true;
	genmenu(0);
    }
}

void treeview::closenode(int id) {
    vector<treeviewnode>::iterator i;
    i = find(items.begin(), items.end(), id);

    if(i != items.end()) {
	if(i->isnode && collapsable) i->isopen = false;
	genmenu(0);
    }
}

void treeview::clear() {
    treeviewnode node;

    items.clear();
    idseq = 1;

    node.id = 0;
    node.ref = 0;
    node.parentid = -1;
    node.isnode = true;

    items.push_back(node);
}

bool treeview::islast(int id) {
    int lastid, nid;
    vector<treeviewnode>::iterator i;

    if((i = find(items.begin(), items.end(), id)) != items.end())
    if((i = find(items.begin(), items.end(), i->parentid)) != items.end()) {
	nid = i->id;
	for(i = items.begin(); i != items.end(); i++)
	if(i->parentid == nid) lastid = i->id;
    }

    return lastid == id;
}

void treeview::genmenu(int parent) {
    char buf[512];
    int nproc = 0;
    vector<treeviewnode>::iterator i, k;

    if(!parent) {
	nestlevel.clear();
	refdeps.clear();
	menu.clear();
    } else {
	if((i = find(items.begin(), items.end(), parent)) != items.end())
	    nestlevel.push_back(*i);
    }

    for(i = items.begin(); i != items.end(); i++) {
	if(i->parentid == parent) {
	    buf[0] = 0;

	    for(k = nestlevel.begin(); k != nestlevel.end(); k++)
	    if(!islast(k->id)) {
		strcat(buf, "\003  ");
	    } else {
		strcat(buf, "   ");
	    }

	    if(islast(i->id)) {
		strcat(buf, "\007\002");
	    } else {
		strcat(buf, "\005\002");
	    }

	    nproc++;

	    if(i->isnode && collapsable) {
		menu.additemf(i->color, 0, "\001%s\001[%c]%s\001", buf,
		    i->isopen ? '-' : '+', i->text.c_str());
	    } else {
		menu.additemf(i->color, 0, "\001%s\001%s\001", buf,
		    i->text.c_str());
	    }

	    refdeps.push_back(*i);

	    if(i->isnode) if(!collapsable || (collapsable && i->isopen))
		genmenu(i->id);
	}
    }

    if(parent != -1)
    if(!nestlevel.empty()) nestlevel.pop_back();
}

void treeview::redraw() {
    savescr();
    genmenu(0);
    menu.redraw();
}

void *treeview::open(int *n) {
    void *p = 0;
    int k;

    savescr();
    redraw();

    if(k = menu.open()) p = getref(getid(k-1));

    if(n) *n = k;
    return p;
}

bool treeview::isnodeopen(int id) {
    vector<treeviewnode>::iterator i;

    if((i = find(items.begin(), items.end(), id)) != items.end()) {
	return i->isopen;
    } else {
	return true;
    }
}

bool treeview::isnode(int id) {
    vector<treeviewnode>::iterator i;

    if((i = find(items.begin(), items.end(), id)) != items.end()) {
	return i->isnode;
    } else {
	return false;
    }
}

int treeview::getparent(int id) {
    int ret = -1;
    vector<treeviewnode>::iterator i;

    if((i = find(items.begin(), items.end(), id)) != items.end())
    if((i = find(items.begin(), items.end(), i->parentid)) != items.end()) {
	ret = i->id;
    }

    return ret;
}

void *treeview::getref(int id) {
    vector<treeviewnode>::iterator i;

    if((i = find(items.begin(), items.end(), id)) != items.end()) {
	return i->ref;
    } else {
	return 0;
    }
}

void treeview::setcur(void *ref) {
    int menupos = 0;
    bool count = true;
    vector<treeviewnode>::iterator i;
    
    for(i = items.begin(); i != items.end(); i++) {
	if(isnode(i->id) && collapsable)
	if(isnodeopen(i->id)) {
	    if(i->ref == ref) {
		menu.setpos(menupos-1);
		break;
	    }
	    menupos++;
	}
    }
}

int treeview::getcount() {
    return items.size();
}

void treeview::setcoord(int nx1, int ny1, int nx2, int ny2) {
    menu.setcoord(x1 = nx1, y1 = ny1, x2 = nx2, y2 = ny2);
}

// ----------------------------------------------------------------------------

using ktool::treeviewnode;

bool treeviewnode::operator == (const int aid) {
    return id == aid;
}

bool treeviewnode::operator == (const void *aref) {
    return ref == aref;
}

bool treeviewnode::operator != (const int aid) {
    return id != aid;
}

bool treeviewnode::operator != (const void *aref) {
    return ref != aref;
}
