#ifndef __TEXTINPUTLINE_H__
#define __TEXTINPUTLINE_H__

#include "fileselector.h"

__KTOOL_BEGIN_NAMESPACE

class textinputline: public abstractuicontrol {
    public:
	int (*otherkeys)(textinputline &caller, int k);
	void (*idle)(textinputline &caller);

    private:
	vector<string> history;
	int length, position, beginpos, color, lastkey;
	string value;
	char passwordchar;
	fileselector *selector;

	bool keymove(int key);
	void redraw();

    public:
	textinputline();
	~textinputline();

	void historyadd(const string buf)
	    { history.push_back(buf); }
	void setvalue(const string buf)
	    { value = buf; }
	const string getvalue()
	    { return value; }
	void setpasswordchar(char npc)
	    { passwordchar = npc; }
	void setcolor(int nclr)
	    { color = nclr; }
	void connectselector(fileselector &fsel)
	    { selector = &fsel; }
	void removeselector()
	    { selector = 0; }

	void setcoords(int x, int y, int len);

	int getlastkey()
	    { return lastkey; }

	void exec();
	void close();
};

__KTOOL_END_NAMESPACE

#ifdef __KTOOL_USE_NAMESPACES

using ktool::textinputline;

#endif

#endif
