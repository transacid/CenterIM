#include "colorschemer.h"

void parsecolordef(const string sdef, int pair, colordef &d) {
    int pos, i, npos;
    string r, p;

    r = sdef;

    for(i = 0; i < 3; i++) {
	if((pos = r.find_first_of(" \t")) == -1)
	    pos = r.size();

	p = r.substr(0, pos);

	if(pos != r.size()) {
	    npos = r.substr(pos+1).find_first_not_of(" \t");
	    if(npos == -1) npos = 0;
	    r.erase(0, pos+1+npos);
	}

	if(i == 0) {
	    d.name = p;

	} else if(i == 1) {
	    if((pos = p.find("/")) != -1) {
		init_pair(pair,
		    findcolor(p.substr(0, pos)),
		    findcolor(p.substr(pos+1)));

		d.code = normalcolor(pair);
		d.def = p;
	    }

	} else if(i == 2) {
	    if(p == "bold") {
		d.code = boldcolor(pair);
		d.def += (string) "\t" + p;
	    }

	}
    }
}
