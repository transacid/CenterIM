#ifndef __IMEXTERNAL_H__
#define __IMEXTERNAL_H__

#include "imevents.h"

class imexternal {
    public:
	enum aoption {
	    aostdin = 2,
	    aostdout = 4,
	    aoprereceive = 8,
	    aopresend = 16
	};

	struct actioninfo {
	    string name;
	    bool enabled;
	};

    private:
	class action {
	    private:
		vector<imevent::imeventtype> event;
		vector<protocolname> proto;
		vector<imstatus> status;

		int options;
		bool enabled;
		string name, code, sname, output;

		imevent *currentev;

		void writescript();
		int execscript();

		void respond();
		void substitute();

		static string geteventname(imevent::imeventtype et);

	    public:
		action();
		~action();

		bool exec(imevent *ev, int &result, int option);

		void disable();
		void enable();

		bool load(ifstream &f);

		const actioninfo getinfo() const;
	};

	vector<action> actions;

    public:
	imexternal();
	~imexternal();

	void load();

	int exec(const imevent &ev);
	int exec(imevent *ev, bool &result, int option = 0);
	    // returns the amount of external actions ran
	    // option can be aopresend or aoprereceive if needed
	    // and "result" is filled in with a bool that indicates
	    // if the event is accepted or not

	vector<actioninfo> getlist() const;
	void update(const vector<actioninfo> &info);
};

extern imexternal external;

#endif
