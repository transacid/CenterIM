#ifndef __COLORSCHEMER_H__
#define __COLORSCHEMER_H__

#include <map>
#include <string>
#include <fstream>

#include "conscommon.h"

template <class T>
class colorschemer {
    private:
	struct colordef {
	    string name, def;
	    int code;
	};

	string delim;
	map<T, colordef> colors;

    public:
	colorschemer()
	    : delim(" \t") { }

	void clear() {
	    colors.clear();
	}

	void push(T elem, const string &def) {
	    int pos, i, npos;
	    string r, p;

	    r = def;

	    for(i = 0; i < 3; i++) {
		if((pos = r.find_first_of(delim)) == -1)
		    pos = r.size();

		p = r.substr(0, pos);

		if(pos != r.size()) {
		    npos = r.substr(pos+1).find_first_not_of(delim);
		    if(npos == -1) npos = 0;
		    r.erase(0, pos+1+npos);
		}

		if(i == 0) {
		    colors[elem].name = p;

		} else if(i == 1) {
		    if((pos = p.find("/")) != -1) {
			init_pair((int) elem,
			    findcolor(p.substr(0, pos)),
			    findcolor(p.substr(pos+1)));

			colors[elem].code = normalcolor((int) elem);
			colors[elem].def = p;
		    }

		} else if(i == 2) {
		    if(p == "bold") {
			colors[elem].code = boldcolor((int) elem);
			colors[elem].def += (string) "\t" + p;
		    }

		}
	    }
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
	    map<T, colordef>::const_iterator ic;

	    for(ic = colors.begin(); ic != colors.end(); ic++) {
		f << ic->second.name << "\t" << ic->second.def << endl;
	    }
	}

	void load(ifstream &f) {
	    int pos;
	    string buf, p;
	    map<T, colordef>::iterator ic;

	    while(!f.eof()) {
		getstring(f, buf);

		if((pos = buf.find_first_of(delim)) != -1) {
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
	    map<T, colordef>::const_iterator ic = colors.find(elem);
	    return (ic != colors.end()) ? ic->second.code : normalcolor(0);
	}

	friend std::ostream& operator<< (std::ostream &o, const colorschemer &s) {
	    s.save(o);
	    return o;
	}
};

#endif
