#ifndef __EVENTMANAGER_H__
#define __EVENTMANAGER_H__

#include "imevents.h"

class imeventmanager {
    private:
    public:
	void store(const imevent &ev);
	    // if incoming, store to the history immediately
	    // if outgoing, return id
// recode here -> rusconv("kw", text)
// produce sounds here too
// ignores messages from contacts on ignore list
// increases contact's message count and calls face.relaxedupdate()


//        void accept(const imcontact cont);

	vector<imevent *> getevents(const imcontact &cont, time_t lastread) const;
	    // for history, event viewing, etc

	void resend();
	    // call every certain period of time
	int getunsentcount() const;
};

extern imeventmanager em;

#endif
