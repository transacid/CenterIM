#ifndef __ICQCOMMON_H__
#define __ICQCOMMON_H__

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <list>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex.h>

#include <config.h>

#include "kkiproc.h"
#include "kkstrtext.h"
#include "conf.h"

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

#define PERIOD_KEEPALIVE        100
#define PERIOD_SELECT           1
#define PERIOD_RESEND           20
#define PERIOD_CHECKMAIL        30
#define PERIOD_SOCKSALIVE       30
#define PERIOD_WAIT_KEEPALIVE   15
#define PERIOD_DISPUPDATE       2

#endif
