#ifndef __CENTERICQ_H__
#define __CENTERICQ_H__

#include "icq.h"
#include <signal.h>
#include <sys/wait.h>
#include <fstream>

#include "konst.socket.h"
#include "konst.string.h"
#include "icqcontact.h"

class centericq {
    protected:
        static void handlesignal(int signum);

    public:
        centericq();
        ~centericq();

        void sendmsg(unsigned int uin, string text);
        void sendurl(unsigned int uin, string url, string text);
        void fwdmsg(unsigned int uin, string text);
        void commandline(int argc, char **argv);
        void exec();
        void reg();
        void mainloop();
        void userinfo(unsigned int uin, bool nonicq = false);
        void changestatus();
        void updatedetails();
        void updateconf();
        void sendfiles(unsigned int uin);
        void find();
        void nonicq(int id);
        void checkmail();
};

extern centericq cicq;

#endif
