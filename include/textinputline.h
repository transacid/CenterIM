#ifndef __KONST_UI_INPUT_H_
#define __KONST_UI_INPUT_H_

#include "conf.h"
#include "conscommon.h"
#include "filemanager.h"

__KTOOL_BEGIN_NAMESPACE

class textinputline: public abstractuicontrol {
    protected:
	int ncolor, visiblestart, visiblelen, actuallen, pos, lastkey;
	linkedlist *history;
	char *actualstr, *visiblestr, passwordchar;

	void imove(int delta);
	void draw();
	void drawcheck(bool redraw);

    public:
	int (*otherkeys)(textinputline *caller, int k);
	void (*idle)(textinputline *caller);
	filemanager *fm;

	textinputline(int clr = 0);
	~textinputline();

	char *open(int x, int y, const char *buf, int visiblelen, int actuallen);
	void setbuf(const char *buf);
	void close();
	void setpasswordchar(const char npc);

	int getlastkey();
};

__KTOOL_END_NAMESPACE

#ifdef __KTOOL_USE_NAMESPACES

using ktool::textinputline;

#endif

#endif
