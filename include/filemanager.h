#ifndef __KONST_UI_FM_H_
#define __KONST_UI_FM_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "conf.h"
#include "cmenus.h"
#include "linkedlist.h"

__KTOOL_BEGIN_NAMESPACE

struct fmentry {
    char *fname;
    bool isdir, islink;
    int size, date;
};

class filemanager: public abstractuicontrol {
    private:
	verticalmenu *m;
	linkedlist *items;
	
	char *startpoint, *savepoint;
	int wcolor, nfcolor, nfscolor, ndcolor, scolor;
	bool spacepressed;

	bool fillmenu();

	static void freefe(void *p);
	
	static int sortfilelist(void *p1, void *p2);
	static int findfname(void *p1, void *p2);
	static int findstr(void *p1, void *p2);
	static int menukeys(verticalmenu *m, int k);

    public:
	bool multi, fullpath, onlydirs, chroot;
	linkedlist *selected;
	textwindow *w;

	filemanager();
	filemanager(int nx1, int ny1, int nx2, int ny2,
	    int nfclr, int ndclr, int nfsclr, int sclr, int wclr);

	~filemanager();

	void setcoords(int nx1, int ny1, int nx2, int ny2);
	void setcolor(int nfclr, int ndclr, int nfsclr, int sclr, int wclr);
	void setmask(const char *exp);
	void setstartpoint(const char *dir);
	void open();
};

__KTOOL_END_NAMESPACE

#ifdef __KTOOL_USE_NAMESPACES

using ktool::filemanager;

#endif

#endif
