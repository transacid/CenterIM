/*
*
* centericq main() function
* $Id: centermain.cc,v 1.2 2001/04/08 07:14:07 konst Exp $
*
*/

#include "centericq.h"
#include "icqhook.h"
#include "icqface.h"
#include "icqconf.h"
#include "icqhist.h"
#include "icqoffline.h"
#include "icqcontacts.h"
#include "icqmlist.h"

centericq cicq;
icqconf conf;
icqcontacts clist;
icqhook ihook;
icqface face;
icqhistory hist;
icqoffline offl;
icqlist lst;

struct icq_link icql;

int main(int argc, char **argv) {
    char savedir[1024];

    getcwd(savedir, 1024);

    try {

	string localedir = conf.getsharedir() + "/locale";
	setlocale(LC_ALL, "");
	bindtextdomain("centericq", localedir.c_str());
	textdomain("centericq");

	cicq.commandline(argc, argv);
	cicq.exec();

    } catch(int errcode) {
    }

    chdir(savedir);
}
