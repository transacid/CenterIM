#ifndef __EVENTMANAGER_H__
#define __EVENTMANAGER_H__

#include "imevents.h"

class imeventmanager {
    private:
	int unsent, recentlysent;
	time_t lastevent;

	enum eventwritemode { history, offline };

	void eventwrite(const imevent &ev, eventwritemode mode);
	imevent *eventread(ifstream &f) const;

	void setlock(const string &fname) const;
	void releaselock(const string &fname) const;

    public:
	imeventmanager();
	~imeventmanager();

	void store(const imevent &ev);

	vector<imevent *> getevents(const imcontact &cont, time_t lastread) const;

	void resend();

	int getunsentcount() const;
};

extern imeventmanager em;

#endif
