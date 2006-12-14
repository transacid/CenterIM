#ifndef __COLORSCHEMER_H__
#define __COLORSCHEMER_H__

#include <map>
#include <string>
#include <fstream>

#include "conscommon.h"

struct colordef {
    string name, def;
    int code;
};

void parsecolordef(const string sdef, int pair, colordef &d);

template <class T>
class colorschemer {
    private:
	map<T, colordef> colors;

    public:
	void clear() {
	    colors.clear();
	}

	void push(T elem, const string &def) {
	    parsecolordef(def, (int) elem, colors[elem]);
	}

	void load(const string &fname) {
	    ifstream f(fname.c_str());
	    if(f.is_open()) {
		load(f);
		f.close();
	    }
	}

	void save(const string &fname) const {
	    ofstream f(fname.c_str());
	    if(f.is_open()) {
		save(f);
		f.close();
	    }
	}

	void save(ofstream &f) const {
	    typename map<T, colordef>::const_iterator ic;

	    for(ic = colors.begin(); ic != colors.end(); ic++) {
		f << ic->second.name << "\t" << ic->second.def << endl;
	    }
	}

	void load(ifstream &f) {
	    int pos;
	    string buf, p;
	    typename map<T, colordef>::iterator ic;

	    while(!f.eof()) {
		getstring(f, buf);

		if((pos = buf.find_first_of(" \t")) != -1) {
		    p = buf.substr(0, pos);

		    for(ic = colors.begin(); ic != colors.end(); ic++) {
			if(ic->second.name == p) {
			    push(ic->first, buf);
			    break;
			}
		    }
		}
	    }
	}

	int operator[] (T elem) const {
	    typename map<T, colordef>::const_iterator ic = colors.find(elem);
	    return (ic != colors.end()) ? ic->second.code : normalcolor(0);
	}

	friend std::ostream& operator<< (std::ostream &o, const colorschemer &s) {
	    s.save(o);
	    return o;
	}
};

#endif
