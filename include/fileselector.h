#ifndef __FILESELECTOR_H__
#define __FILESELECTOR_H__

#include "cmenus.h"

#include <sys/stat.h>
#include <list>

#define FSEL_MULTI      2
#define FSEL_DIRSELECT  4
#define FSEL_CHROOT     8

__KTOOL_BEGIN_NAMESPACE

class fileselector: public abstractuicontrol {
    public:
	int (*otherkeys)(fileselector &caller, int k);
	void (*idle)(fileselector &caller);

    private:
	struct dhistitem {
	    string dirname;
	    int pos, fpos;
	};

	class item {
	    private:
		short compare(const item &aitem) const;

	    public:
		string fname;
		int mode;

		item(const string afname, int amode)
		    { fname = afname, mode = amode; }
		const string getprefix() const;

		bool operator < (const item &aitem) const;
		bool operator > (const item &aitem) const;
	};

	int options;
	int cnormal, cselected, cfile, cdir, cframe, ccurrent;

	vector<dhistitem> dhist;
	vector<string> selected;
	list<item> items;

	string spoint, dcurrent;
	bool finish;

	verticalmenu m;
	textwindow w;

	static int menukeys(verticalmenu *m, int k);

	void generatemenu();

    public:
	fileselector();
	~fileselector();

	void setoptions(int noptions)
	    { options = noptions; }
	void setstartpoint(const string aspoint)
	    { spoint = aspoint; }
	void setwindow(textwindow awindow)
	    { w = awindow; }
	void setcolor(int acnormal, int acselected, int accurrent,
	    int acfile, int acdir = 0, int acframe = 0);

	vector<string> getselected()
	    { return selected; }
	int getlastkey()
	    { return m.getlastkey(); }

	void exec();
	void close();
};

__KTOOL_END_NAMESPACE

#ifdef __KTOOL_USE_NAMESPACES

using ktool::fileselector;

#endif

#endif
