#ifndef __ICQCOMMON_H__
#define __ICQCOMMON_H__

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <list>

#include <sys/types.h>
#include <regex.h>

#include "kkstrtext.h"
#include "conf.h"

#define PERIOD_RECONNECT        30

#ifdef ENABLE_NLS

#include <libintl.h>
#define _(s)    gettext(s)

#else

#define _(s)    (s)

#endif


#ifdef __KTOOL_USE_NAMESPACES
#define __CENTERICQ_USE_NAMESPACES
#endif

#ifdef __CENTERICQ_USE_NAMESPACES

using namespace std;

#endif

#endif
