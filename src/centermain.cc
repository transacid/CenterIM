/*
*
* centericq main() function
* $Id: centermain.cc,v 1.4 2001/04/17 08:09:26 konst Exp $
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

#ifdef ENABLE_NLS
#include <locale.h>
#endif

int main(int argc, char **argv) {
    char savedir[1024];

    getcwd(savedir, 1024);

    try {

#if defined(HAVE_SETLOCALE) && defined(ENABLE_NLS)
	setlocale(LC_ALL, "");
	bindtextdomain("centericq", LOCALE_DIR);
	textdomain("centericq");
#endif

	cicq.commandline(argc, argv);
	cicq.exec();

    } catch(int errcode) {
    }

    chdir(savedir);
}
