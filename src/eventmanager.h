#ifndef __EVENTMANAGER_H__
#define __EVENTMANAGER_H__

#include "imevents.h"

class imeventmanager {
    private:
	int unsent;

	enum eventwritemode { history, offline };

	void eventwrite(const imevent &ev, eventwritemode mode);
	imevent *eventread(ifstream &f) const;

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