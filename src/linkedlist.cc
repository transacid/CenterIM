/*
*
* konst.list.cc
*
* Linked lists operating module
* (c) Konstantin Klyagin, 1999
*
* History
*
* 13.12.1999  writing started, initial release has been done
*
*/

#include "linkedlist.h"

linkedlist::linkedlist(): count(0), flist(0), freeitem(0) {
}

linkedlist::~linkedlist() {
    empty();
//    if(flist) delete flist;
}
	
void linkedlist::add(void *p) {
    flinkedlist *l = flist, *k = new flinkedlist;

    k->data = p;
    k->next = 0;

    if(!count) flist = k; else {
	for(i = 0; i < count-1; i++, l = l->next);
	l->next = k;
    }

    count++;
}

void linkedlist::insert(int n, void *p) {
    flinkedlist *l = flist, *k = new flinkedlist;
    
    if(n <= count) {
	for(i = 0; i < n-1; i++, l = l->next);
	k->data = l->data;
	l->data = p;
	k->next = l->next;
	l->next = k;
	count++;
    } else add(p);
}

void linkedlist::remove(int n) {
    flinkedlist *l = flist, *pr;

    if((n < count) && (n >= 0)) {
	if(!n) {
	    flist = l->next;
	    if(freeitem) freeitem(l->data); else free(l->data);
	    delete l;
	} else {
	    for(pr = 0, i = 0; i < n; i++, l = l->next) {
		if(i == n-1) pr = l;
	    }

	    if(pr) {
		pr->next = l->next;
		if(freeitem) freeitem(l->data); else free(l->data);
		delete l;
	    }
	}

	count--;
    }
}

void linkedlist::empty() {
    flinkedlist *l, *p;
    
    for(i = 0, l = flist; i < count && l; i++) {
	p = l;
	l = l->next;
	if(freeitem) freeitem(p->data); else free(p->data);
	delete p;
    }

    count = 0;
}

void* linkedlist::at(int n) {
    flinkedlist *l = flist;

    if((n < count) && (n >= 0) && l) {
	for(i = 0; i < n; i++, l = l->next);
	return l->data;
    } else {
	return 0;
    }
}

void* linkedlist::find(void *p, listcompare *compare) {
    void *l;

    for(i = 0; i < count; i++) {
	l = at(i);
	if(!compare(p, l)) return l;
    }

    return 0;
}

int linkedlist::findnum(void *p, listcompare *compare) {
    void *l;

    for(i = 0; i < count; i++) {
	l = at(i);
	if(!compare(p, l)) return i;
    }

    return -1;
}

void linkedlist::sort(listcompare *compare) {
    int i;
    flinkedlist *f;
    void *tempdata;

    for(i = 0; i < count; i++)
    for(f = flist; f && f->next; f = f->next) {
	if(compare(f->next->data, f->data) > 0) {
	    tempdata = f->data;
	    f->data = f->next->data;
	    f->next->data = tempdata;
	}
    }
}

void linkedlist::replace(int n, void *p) {
    flinkedlist *l = flist;

    if(n < count) {
	for(i = 0; i < n; i++, l = l->next);
	if(l->data != p) {
	    if(freeitem) freeitem(l->data); else free(l->data);
	    l->data = p;
	}
    } else add(p);
}

void *linkedlist::foreach(listforeachfunc *exec, void *arg) {
    flinkedlist *l = flist;
    void *ret = 0;

    for(; !ret && l; l = l->next) {
	ret = exec(l->data, arg);
    }

    return ret;
}
