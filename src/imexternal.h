#ifndef __IMEXTERNAL_H__
#define __IMEXTERNAL_H__

#include "imevents.h"

class imexternal {
    public:
	enum aoption {
	    aostdin = 2,
	    aostdout = 4,
	    aoprereceive = 8,
	    aopresend = 16,
	    aomanual = 32,
	    aonowait = 64
	};

	struct actioninfo {
	    string name;
	    bool enabled;
	};

    private:
	class action {
	    private:
		set<imevent::imeventtype> event;
		set<protocolname> proto;
		set<imstatus> status;

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
		bool exec(const imcontact &ic, string &outbuf);

		bool load(ifstream &f);

		bool matches(int aoptions, protocolname apname) const;
		string getname() const { return name; }
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

	bool execmanual(const imcontact &ic, int id, string &outbuf);

	vector<pair<int, string> > getlist(int options, protocolname pname) const;
};

extern imexternal external;

#endif
