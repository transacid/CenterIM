#include "textinputline.h"

textinputline::textinputline() {
    beginpos = length = position = 0;
    idle = 0;
    otherkeys = 0;
    passwordchar = 0;
    selector = 0;
}

textinputline::~textinputline() {
}

void textinputline::setcoords(int x, int y, int len) {
    x1 = x;
    y1 = y2 = y;
    length = len;
    x2 = x1+length;
}

bool textinputline::keymove(int key) {
    bool r = true;

    switch(key) {
	case KEY_LEFT:
	case CTRL('b'):
	    if(--position < 0) position = 0;
	    break;
	case KEY_RIGHT:
	case CTRL('f'):
	    if(++position > value.size()) position = value.size();
	    break;
	case KEY_HOME:
	case CTRL('a'):
	    position = 0;
	    break;
	case KEY_END:
	case CTRL('e'):
	    position = value.size();
	    break;
	default:
	    r = false;
    }

    return r;
}

void textinputline::redraw() {
    int displen;

    if(position > value.size()) {
	position = value.size();
    }

    if(position < beginpos) {
	beginpos = position;
    }

    while(position > beginpos+length-1) {
	beginpos++;
    }

    displen = value.length()-beginpos;
    if(displen > length) displen = length;

    attrset(color);
    kgotoxy(x1, y1);

    printstring((passwordchar ? string(displen, passwordchar) :
	value.substr(beginpos, displen)) +
	string(length-displen, ' '));

    kgotoxy(x1+position-beginpos, y1);
}

void textinputline::exec() {
    bool fin, go, fresh;
    vector<string> sel;
    vector<string>::iterator isel;

    screenbuffer.save(x1, y1, x2, y2);
    fresh = true;

    for(fin = false; !fin; ) {
	redraw();
	go = idle ? keypressed() : true;

	if(go) {
	    lastkey = getkey();

	    if(!keymove(lastkey))
	    switch(lastkey) {
		case '\r':
		    historyadd(value);
		    fin = true;
		    break;
		case KEY_DC:
		    value.replace(position, 1, "");
		    break;
		case CTRL('k'):
		    value = "";
		    break;
		case CTRL('u'):
		    value.replace(0, position, "");
		    position = 0;
		    break;
		case CTRL('t'):
		    if(selector) {
			selector->exec();
			selector->close();

			if(strchr("\r ", selector->getlastkey())) {
			    sel = selector->getselected();

			    if(!sel.empty()) {
				position = 0;

				if(sel.size() == 1) {
				    value = sel[0];
				} else {
				    value = "";
				    for(isel = sel.begin();
				    isel != sel.end(); isel++) {
					if(!value.empty()) value += " ";
					value += "\"" + *isel + "\"";
				    }
				}
			    }
			}
		    }
		    break;
		case KEY_BACKSPACE:
		case CTRL('h'):
		    if(position) {
			value.replace(--position, 1, "");
		    }
		    break;
		case KEY_ESC:
		    fin = true;
		    break;
		default:
		    if((lastkey > 31) && (lastkey < 256)) {
			if(fresh) {
			    value = string(1, (char) lastkey);
			    position = 1;
			} else {
			    value.insert(position++, string(1, (char) lastkey));
			}
		    }
	    }

	    fresh = false;
	} else {
	    if(idle) {
		(*idle)(*this);
	    }
	}
    }
}

void textinputline::close() {
}
