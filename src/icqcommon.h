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
#include <unistd.h>
#include <regex.h>

#include <config.h>

#include "kkiproc.h"
#include "kkstrtext.h"
#include "conf.h"


#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(DARWIN)
#include <sys/param.h>
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

#define PERIOD_ATONCE           5
#define MAX_ATONCE              10

#endif
