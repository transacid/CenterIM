/*
*
* kkiproc inter-process communications related routines
* $Id: kkiproc.cc,v 1.2 2001/06/03 21:12:05 konst Exp $
*
* Copyright (C) 1999-2001 by Konstantin Klyagin <konst@konst.org.ua>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at
* your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
* USA
*
*/

#include "kkiproc.h"

bool issuchpid(int pid) {
    string pdir = "/proc/" + i2str(pid);
    return !access(pdir.c_str(), F_OK);
}

int checkpid(char *pidfile) {
    FILE *f;
    char dir[20], curdir[500];
    int pid;

    if((f = fopen(pidfile, "r"))) {
	fscanf(f, "%s", dir);
	pid = atoi(dir);
	fclose(f);

	getcwd(curdir, 500);
	sprintf(dir, "/proc/%d", pid);
	if(!chdir(dir)) return 0; else chdir(curdir);
    }

    if((f = fopen(pidfile, "w"))) {
	fprintf(f, "%d", getpid());
	fclose(f);
	return 1;
    }

    return 0;
}

void detach(char *logfile) {
    if(logfile) freopen(logfile, "w", stdout);

    if(!fork()) {
	setsid();
	chdir("/");
    } else {
	_exit(0);
    }
}

time_t lastkeypress() {
    time_t t = 0;
    struct stat s;

#ifdef __linux__

    struct utmp *u;
    char tname[12];

    if(readlink("/proc/self/fd/0", tname, 12) != -1) {
	if(!strncmp(tname, "/dev/tty", 8) && isdigit(tname[8])) {
	    setutent();
	    while((u = getutent()))
	    if(u->ut_type == USER_PROCESS && strlen(u->ut_line) > 3 && !strncmp(u->ut_line, "tty", 3) && isdigit(u->ut_line[3]) && *u->ut_user && issuchpid(u->ut_pid)) {
		sprintf(tname, "/dev/%s", u->ut_line);
		if(!stat(tname, &s) && s.st_atime > t) t = s.st_atime;
	    }
	    endutent();
	} else {
	    if(!stat(tname, &s)) t = s.st_atime; else time(&t);
	}
    } else time(&t);

#else

    char *p;

    if((p = ttyname(0)) != NULL) {
	if(!stat(p, &s) && s.st_atime > t) t = s.st_atime; else time(&t);
    } else {
	time(&t);
    }
     
#endif

    return t;
}

char *getcurtty() {
    static char devname[32];

    if(readlink("/proc/self/fd/0", devname, 32) != -1)
    return devname; else return 0;
}

int dataready(int fd, int dowait) {
    struct timeval tv;
    fd_set fds;
    int rc;

    tv.tv_sec = tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    if(select(fd+1, &fds, 0, 0, dowait ? 0 : &tv) != -1) {
	rc = FD_ISSET(fd, &fds);
    } else {
	rc = 0;
    }

    return rc;
}

char *getprocentry(char *fname) {
    FILE *f = fopen(fname, "r");
    static char *p = 0;
    int fsize = kfilesize(fname);

    if(f) {
	p = (char *) realloc(p, fsize+1);
	fread(p, fsize, 1, f);
	fclose(f);
    } else {
	p = 0;
    }

    return p;
}

char *gethostname() {
    return getprocentry("/proc/kernel/hostname");
}

char *getdomainname() {
    return getprocentry("/proc/kernel/domainname");
}
