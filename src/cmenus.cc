#include "cmenus.h"

verticalmenu::verticalmenu(int px1, int py1, int px2, int py2, int pncolor, int pscolor) {
    initmembers();
    setcolors(pncolor, pscolor);
    setcoord(px1, py1, px2, py2);
}

verticalmenu::verticalmenu(int pncolor = 0, int pscolor = 0) {
    initmembers();
    setcolors(pncolor, pscolor);
}

void verticalmenu::initmembers() {
    firstdisp = curelem = 0;
    idle = 0;
    otherkeys = 0;
    clearonfocuslost = cycled = false;
}

void verticalmenu::setcolors(int pncolor, int pscolor) {
    ncolor = pncolor, scolor = pscolor;
}

void verticalmenu::clear() {
    items.clear();
}

bool verticalmenu::empty() {
    return items.empty();
}

void verticalmenu::setcoord(int nx1, int ny1, int nx2, int ny2) {
    x1 = nx1;
    x2 = nx2;
    y1 = ny1;
    y2 = ny2;
}

void verticalmenu::additemf(const char *fmt, ...) {
    char buf[10240];
    va_list ap;
    
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    additem(buf);
}

void verticalmenu::additemf(int color, void *ref, const char *fmt, ...) {
    char buf[10240];
    va_list ap;
    
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    additem(color, ref, buf);
}

void verticalmenu::additem(string text) {
    additem(ncolor, 0, text);
}

void verticalmenu::additem(int color, void *ref, string text) {
    verticalmenuitem i;
    i.text = text;
    i.color = color ? color : ncolor;
    i.kind = ITEM_NORM;
    i.ref = ref;
    items.push_back(i);
}

void verticalmenu::addline() {
    addline(ncolor, 0);
}

void verticalmenu::addline(int color, const char *fmt = 0, ...) {
    verticalmenuitem i;
    char buf[10240];
    va_list ap;

    if(fmt) {
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
        va_end(ap);
        i.text = buf;
        i.kind = ITEM_SEP;
    } else {
        i.kind = ITEM_LINE;
    }

    i.color = color;
    items.push_back(i);
}

bool verticalmenu::shownelem(int n, int selected) {
    bool hlight = true;
    int extra = 0;
    unsigned char c;

    if((n < 0) || (n >= items.size())) return false;
    verticalmenuitem item = items[n];

    if(!(selected && item.kind != ITEM_NORM)) {
        attrset(selected && (item.kind == ITEM_NORM) ? scolor : item.color);

        if(item.kind == ITEM_LINE) {
            if(!selected) mvhline(y1+n-firstdisp, x1, HLINE, x2-x1);
        } else if(!item.text.empty()) {
            mvprintw(y1+n-firstdisp, x1, "");
            for(int i = x1; i < x2+extra; i++) {
                if(i-x1 < item.text.size()) {
                    switch(c = item.text[i-x1]) {
                        case 1:
                            if(hlight = !hlight)
                                attrset(selected && (item.kind == ITEM_NORM) ? 
                                    scolor : item.color);
                            else attrset(ncolor);

                            extra++;
                            break;
                        case 2: addch(HLINE); break;
                        case 3: addch(VLINE); break;
                        case 4: addch(RTEE); break;
                        case 5: addch(LTEE); break;
                        case 6: addch(ULCORNER); break;
                        case 7: addch(LLCORNER); break;
                        case 8: addch(LRCORNER); break;
                        case 9: addch(URCORNER); break;
                        default:
                            printchar(c);
                            break;
                    }
                } else {
                    printw(" ");
                }
            }

        }
    }

    return item.kind == ITEM_NORM;
}

void verticalmenu::showall() {
    unsigned int p, k;

    if(curelem < 0) {
         curelem = 0;
    } else if(curelem > items.size()-1) {
         curelem = items.size()-1;
    }

   if((firstdisp+y2-y1 <= curelem) || (curelem < firstdisp)) {
        firstdisp = curelem-y2+y1+1;
    }

    if((firstdisp+y2-y1 > items.size()) && (y2-y1 < items.size())) {
        firstdisp = items.size()-y2+y1;
    } else if(firstdisp < 0) {
        firstdisp = 0;
    }
    
    attrset(ncolor);

    for(p = firstdisp; (p < firstdisp+y2-y1) && (p < items.size()); p++) {
        kgotoxy(x1, y1+p-firstdisp);
        shownelem(p, 0);
    }
    
    for(; p < firstdisp+y2-y1; p++) {
        mvhline(y1+p-firstdisp, x1, ' ', x2-x1);
    }
}

void verticalmenu::redraw() {
    showall();
    kgotoxy(x1, y1+curelem-firstdisp);
    if(curelem >= 0) shownelem(curelem, 1);
    refresh();
}

void verticalmenu::scale() {
    if((y2 = y1+items.size()) > LINES-2) y2 = LINES-2;
    if(!window.empty()) window.y2 = y2;
}

int verticalmenu::open() {
    bool finished = false;
    int go, k, ret;

    if(!window.empty()) window.open();

    redraw();

    while(!finished) {
        if(idle) go = keypressed(); else go = 1;
        if(go) {
            k = getkey();
            if(emacs) k = emacsbind(k);
            lastkey = k;

            switch(k) {
                case '\r':
                    finished = true;
                    if(clearonfocuslost) shownelem(curelem, 0);
                    if(items.empty()) ret = 0; else ret = curelem+1;
                    break;
        
                case 27:
                    if(clearonfocuslost) shownelem(curelem, 0);
                    finished = true;
                    ret = 0;
                    break;
        
                case KEY_UP:
                    if(curelem > 0) {
                        shownelem(curelem, 0);
                        if(--curelem < firstdisp) {
                            firstdisp = curelem - y2 + y1 + 1;
                            redraw();
                        } else {
                            while(curelem >= 0)
                            if(!shownelem(curelem, 1)) {
                                curelem--;
                            } else {
                                break;
                            }

                            refresh();
                        }
                    } else if(cycled) {
                        curelem = items.size()-1;
                        if((firstdisp = curelem-y2+y1+1) < 0) firstdisp = 0;
                        redraw();
                    }
                    break;

                case KEY_DOWN:
                    if(!items.empty()) {
                        if(curelem < items.size()-1) {
                            shownelem(curelem++, 0);
                        
                            if(curelem > firstdisp+y2-y1-1) {
                                firstdisp += y2 - y1;
                                redraw();
                            } else {
                                bool lastone;
                                int savecur = curelem-1;
                        
                                while(curelem < items.size()) {
                                    if(!(lastone = shownelem(curelem, 1)))
                                    curelem++; else break;
                                }

                                if(!lastone) shownelem(curelem = savecur, 1);
                                refresh();
                            }
                        } else if(cycled) {
                            curelem = firstdisp = 0;
                            redraw();
                        }
                    }
                    break;
        
                case KEY_PPAGE:
                    if((curelem -= y2-y1) < 0) curelem = 0;
                    firstdisp = curelem;
                    redraw();
                    break;
        
                case KEY_NPAGE:
                    if(!items.empty()) {
                        if((curelem += y2-y1) > items.size()-1) curelem = items.size()-1;
                        if((firstdisp = curelem-y2+y1+1) < 0) firstdisp = 0;
                    }
                    redraw();
                    break;
      
                case KEY_HOME:
                    curelem = firstdisp = 0;
                    redraw();
                    break;

                case KEY_END:
                    curelem = items.size()-1;
                    if((firstdisp = curelem-y2+y1+1) < 0) firstdisp = 0;
                    redraw();
                    break;
      
                default:
                    if(otherkeys) {
                        if((go = (*otherkeys)(this, k)) != -1) {
                            if(clearonfocuslost) shownelem(curelem, 0);
                            return go;
                        }
                    }
                    break;
            }
        } else {
            if(idle) (*idle)(this);
        }
    }

    return ret;
}

int verticalmenu::getcount() {
    return items.size();
}

int verticalmenu::getlastkey() {
    return lastkey;
}

void *verticalmenu::getref(int n) {
    return (n >= 0) && (n < items.size()) ? items[n].ref : 0;
}

int verticalmenu::getpos() {
    return curelem;
}

void verticalmenu::getpos(int &cur, int &first) {
    cur = curelem;
    first = firstdisp;
}

void verticalmenu::setpos(int cur, int first = -1) {
    curelem = cur;
    if(first != -1) firstdisp = first;
}

void verticalmenu::setcolor(int pos, int color) {
    if((pos >= 0) && (pos < items.size())) {
        items[pos].color = color;
    }
}

void verticalmenu::setwindow(textwindow nwindow) {
    window = nwindow;
    setcoord(window.x1+1, window.y1+1, window.x2, window.y2);
}

void verticalmenu::remove(int pos) {
    vector<verticalmenuitem>::iterator ii;
    int i;

    if((pos >= 0) && (pos < items.size())) {
        for(i = 0, ii = items.begin(); ii != items.end() && i < pos; i++, ii++);
        items.erase(ii);
    }
}

void verticalmenu::close() {
    window.close();
}

verticalmenu::~verticalmenu() {
}

// --------------------------------------------------------------------------

horizontalmenu::horizontalmenu(int x, int y, int normcolor, int selcolor, int framecolor) {
    ncolor = normcolor;
    scolor = selcolor;
    fcolor = framecolor;
    coordy = y;
    coordx = x;
}

horizontalmenu::horizontalmenu() {
    coordy = coordx = 0;
}

horizontalmenu::~horizontalmenu() {
}

#define HM_RIGHT        50001
#define HM_LEFT         50002
#define HM_CLOSE        50003

int horizontalmenu::menu_otherkeys(verticalmenu *ref, int k) {
    switch(k) {
        case KEY_RIGHT : return HM_RIGHT;
        case KEY_LEFT  : return HM_LEFT;
        case KEY_F(10) : return HM_CLOSE;
    }
    return -1;
}

void horizontalmenu::additem(int color, string text) {
    horizontalmenuitem i;
    i.text = text;
    i.color = color ? color : ncolor;
    menus.push_back(i);
}

void horizontalmenu::additem(string text) {
    additem(0, text);
}

void horizontalmenu::additemf(int color, const char *fmt, ...) {
    va_list ap;
    char buf[1024];
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    additem(color, buf);
}

void horizontalmenu::additemf(const char *fmt, ...) {
    va_list ap;
    char buf[1024];
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    additem(0, buf);
}

int horizontalmenu::getx(int n) {
    int i, l = 0;
    for(i = 0; i < n; i++) l += menus[i].text.size();
    if(n > 0) l += n*2;
    return l+coordx;
}

int horizontalmenu::menulen(int n) {
    verticalmenu m = menus[n].menu;
    vector<verticalmenuitem>::iterator i;
    int l = 0;

    for(i = m.items.begin(); i != m.items.end(); i++) {
        if(!i->text.empty() && (i->text.size() > l)) l = i->text.size();
    }

    return l;
}

void horizontalmenu::redraw() {
    int sv = selected;
    selected = -1;
    draw();
    selected = sv;
}

void horizontalmenu::draw() {
    vector<horizontalmenuitem>::iterator i;
    int n;
    
    attrset(ncolor);
    mvhline(coordy, 0, ' ', COLS);

    for(n = 0, i = menus.begin(); i != menus.end(); i++, n++) {
        kgotoxy(getx(n), coordy);
        if(n == selected) attrset(scolor); else attrset(ncolor);
        printstring(i->text);
    }

    refresh();
}

void horizontalmenu::moveelem(int old) {
    attrset(menus[old].color);
    kgotoxy(getx(old), coordy);
    printstring(menus[old].text);

    attrset(scolor);
    kgotoxy(getx(selected), coordy);
    printstring(menus[selected].text);
}

bool horizontalmenu::open(int *hor, int *pulld) {
    bool finished = false, done = false;
    int ch, osel, oact = 0;
    verticalmenu *m;

    if((selected < 0) || (selected >= menus.size())) selected = 0;
    saveline();
    draw();

    while(!finished) {
        if(!oact) {
            ch = getkey();
            if(emacs) ch = emacsbind(ch);
        } else {
            ch = oact;
        }

        osel = selected;
         
        switch(ch) {
            case KEY_RIGHT:
                if(++selected >= menus.size()) selected = 0;
                moveelem(osel);
                break;
            case KEY_LEFT:
                if(--selected < 0) selected = menus.size()-1;
                moveelem(osel);
                break;
            case '\r':
            case KEY_DOWN:
                m = pulldown(selected);
                if(!m->items.empty()) {
                    m->scale();
                    m->x2 = m->x1 + menulen(selected) + 1;
                    m->window.x2 = m->x2;
                    int r = m->open();

                    switch(r) {
                        case HM_RIGHT: oact = KEY_RIGHT; break;
                        case HM_LEFT: oact = KEY_LEFT; break;
                        case HM_CLOSE: oact = KEY_F(10); break;
                        default:
                            if(r) {
                                if(pulld) *pulld = r;
                                if(hor) *hor = selected+1;
                                done = finished = true;
                            }
                    }

                    m->close();
                    if(r == HM_RIGHT || r == HM_LEFT) continue;
                 } else if(!oact) {
                    if(pulld) *pulld = 0;
                    if(hor) *hor = selected+1;
                    done = finished = true;
                 }
                 break;
            case KEY_F(10):
            case 27:
                 finished = true;
                 break;
        }

        if(oact == KEY_RIGHT || oact == KEY_LEFT) oact = '\r'; else oact = 0;
    }

    restoreline();
    return done;
}

void horizontalmenu::close() {
}

void horizontalmenu::saveline() {
    if(!scrbuf) {
        scrbuf = (chtype **) malloc(sizeof(chtype*));
        scrbuf[0] = (chtype *) malloc(sizeof(chtype) * COLS);
        mvinchnstr(coordy, 0, scrbuf[0], COLS);
    }
}

void horizontalmenu::restoreline() {
    if(scrbuf) {
        mvaddchnstr(coordy, 0, scrbuf[0], COLS);
        refresh();
        free(scrbuf[0]);
        free(scrbuf);
        scrbuf = 0;
    }
}

verticalmenu *horizontalmenu::pulldown(int n) {
    if((n >= 0) && (n < menus.size())) {
        menus[n].menu.setcolors(ncolor, scolor);
        menus[n].menu.otherkeys = &menu_otherkeys;
        menus[n].menu.cycled = true;
        menus[n].menu.setwindow(textwindow(getx(n)-1, coordy+1, getx(n)+2, coordy+6, fcolor));
        return &menus[n].menu;
    } else {
        return 0;
    }
}

// --------------------------------------------------------------------------

using ktool::horizontalmenuitem;

horizontalmenuitem::horizontalmenuitem() {
}

horizontalmenuitem::horizontalmenuitem(const horizontalmenuitem &a) {
    text = a.text;
    color = a.color;
    menu = a.menu;
}

horizontalmenuitem::~horizontalmenuitem() {
}
