#ifndef __IMEXTERNAL_H__
#define __IMEXTERNAL_H__

#include "imevents.h"

class imexternal {
    public:
	struct actioninfo {
	    string name;
	    bool enabled;
	};

    private:
	class action {
	    private:
		enum aoption {
		    aostdin = 2,
		    aostdout = 4
		};

		vector<imevent::imeventtype> event;
		vector<protocolname> proto;
		vector<imstatus> status;

		int options;
		bool enabled;
		string name, code, sname, output;
		const imevent *currentev;

		void writescript();
		void execscript();
		void respond();

		static const string geteventname(imevent::imeventtype et);

	    public:
		action();
		~action();

		bool exec(const imevent &ev);

		void disable();
		void enable();

		bool load(ifstream &f);
		void ssave(ofstream &f) const;

		const actioninfo getinfo() const;
	};

	vector<action> actions;

    public:
	imexternal();
	~imexternal();

	void load();
	void ssave() const;

	int exec(const imevent &ev);
	    // returns the amount of external actions ran

	vector<actioninfo> getlist() const;
	void update(const vector<actioninfo> &info);
};

extern imexternal external;

#endif
