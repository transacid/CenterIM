#ifndef __KONST_PROCESS_H_
#define __KONST_PROCESS_H_

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utmp.h>
#include <string.h>
#include <ctype.h>

#include "conf.h"
#include "kkfsys.h"
#include "kkstrtext.h"

__KTOOL_BEGIN_C

int checkpid(const char *pidfile);
bool issuchpid(int pid);
int dataready(int fd, int dowait);

time_t lastkeypress();
void detach(char *logfile);
char *getcurtty();

char *getprocentry(char *fname);

__KTOOL_END_C

#endif
