#ifndef __ICQCOMMON_H__
#define __ICQCOMMON_H__

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <regex.h>

#include <config.h>

#ifdef HAVE_SSTREAM
    #include <sstream>
#else
    #include <strstream>
#endif

#include "kkiproc.h"
#include "kkstrtext.h"
#include "conf.h"

#include <sys/param.h>

#ifdef BSD
#include <sys/mount.h>
#else
#include <sys/vfs.h>
#endif

#define PERIOD_RECONNECT        40

#ifdef ENABLE_NLS

#include <libintl.h>
#define _(s)    ::gettext(s)

#else

#define _(s)    (s)

#endif

#ifdef __KTOOL_USE_NAMESPACES
#define __CENTERICQ_USE_NAMESPACES
#endif

#ifdef __CENTERICQ_USE_NAMESPACES

using namespace std;

#endif

#define PERIOD_RESEND           20
#define PERIOD_CHECKMAIL        30
#define PERIOD_DISPUPDATE       2
#define PERIOD_AUTOSAVE         120
#define PERIOD_TYPING           6

#define PERIOD_ATONCE           5
#define MAX_ATONCE              10

/*
*
* Several helper routines
*
*/

string up(string s);
string lo(string s);

/*! .. and defines.
 */

#define ENUM_PLUSPLUS(tp) \
    inline tp& operator ++(tp &p, int) { \
	int t = p; \
	return p = static_cast<tp>(++t); \
    }

#endif
