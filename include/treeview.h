#ifndef __KONST_UI_TREE_H_
#define __KONST_UI_TREE_H_

#include <vector>
#include <algorithm>

#include "conf.h"
#include "conscommon.h"
#include "abstractui.h"
#include "cmenus.h"

__KTOOL_BEGIN_NAMESPACE

class treeviewnode {
    public:
	int id, parentid, color;
	bool isnode, isopen;
	string text;
	void *ref;

	bool operator == (const int aid);
	bool operator == (const void *aref);
	bool operator != (const int aid);
	bool operator != (const void *aref);
};

class treeview : public abstractuicontrol {
    private:
	int bgcolor, selectcolor, nodecolor, leafcolor, idseq;

	vector<treeviewnode> items;
	vector<treeviewnode> nestlevel;
	vector<treeviewnode> refdeps;

	bool islast(int id);
	void drawnest(int y);
	void genmenu(int parent);
	void init();

    public:
	int curelem;
	bool collapsable;
	verticalmenu menu;

	treeview(int nx1, int ny1, int nx2, int ny2, int nbgcolor, int nselectcolor, int nnodecolor, int nleafcolor);
	treeview(int nbgcolor, int nselectcolor, int nnodecolor, int nleafcolor);
	treeview();
	~treeview();

        bool empty();

	int addnodef(int parent, int color, void *ref, const char *fmt, ...);
	int addleaff(int parent, int color, void *ref, const char *fmt, ...);

	int addnode(int parent, int color, void *ref, string text);
	int addleaf(int parent, int color, void *ref, string text);

	void opennode(int mpos);
	void closenode(int mpos);

	int getcount();
	int getid(int mpos);
	int getid(void *ref);
	bool isnode(int id);
	bool isnodeopen(int id);

	int getparent(int id);
	void *getref(int id);

	void clear();

	virtual void redraw();
	void *open(int *n);

	void setcur(int id);
	void setcoord(int nx1, int ny1, int nx2, int ny2);
};

__KTOOL_END_NAMESPACE

#ifdef __KTOOL_USE_NAMESPACES

using ktool::treeview;

#endif

#endif
