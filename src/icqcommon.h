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

#define PERIOD_KEEPALIVE        100
#define PERIOD_SELECT           1
#define PERIOD_TCP              1
#define PERIOD_RESOLVE          40
#define PERIOD_DISCONNECT       130
#define PERIOD_RESEND           15
#define PERIOD_CHECKMAIL        30
#define PERIOD_WAITNEWUIN       20
#define PERIOD_SOCKSALIVE       30
#define PERIOD_WAIT_KEEPALIVE   15
#define PERIOD_CLIST_RESEND     5
#define PERIOD_DISPUPDATE       2

#endif
