/*
*
* kkfsys file system related routines
* $Id: kkfsys.cc,v 1.2 2001/06/03 21:12:05 konst Exp $
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

#include "kkfsys.h"

unsigned long kfilesize(char *fname) {
    struct stat buf;
    if(!stat(fname, &buf)) return buf.st_size; else return 0;
}

void freads(FILE *f, char *s, int maxlen) {
    s[0] = 0;
    fgets(s, maxlen, f);

    for(int i = strlen(s)-1; ; i--)
    if(i >= 0 && strchr("\r\n", s[i])) s[i] = 0; else break;
}

int fcopy(const char *source, const char *dest) {
    FILE *inpf, *outf;
    int ret = -1, bc;
    char buf[8192];

    if(inpf = fopen(source, "r")) {
	if(outf = fopen(dest, "w")) {
	    ret = bc = 0;
	    
	    while((bc = fread(buf, 1, 8192, inpf))) {
		fwrite(buf, bc, 1, outf);
		ret += bc;
		if(bc < 8192) break;
	    }
	    
	    fclose(inpf);
	}
	
	fclose(outf);
    }

    return ret;
}

int fmove(const char *source, const char *dest) {
    int ret = fcopy(source, dest);
    if(ret != -1) unlink(source);
    return ret;
}

void stepftw(const char *fname, int *stopwalk, int (*fn)(const char *file, const struct stat *sb, int flag)) {
    struct stat st;
    int flag;

    flag = FTW_F;

    if(lstat(fname, &st)) {
#ifdef FTW_NS
	flag = FTW_NS;
#endif
    } else {
	if(S_ISLNK(st.st_mode)) {
#ifdef FTW_SL
	    flag = FTW_SL;
#endif
	} else
	if(S_ISREG(st.st_mode)) flag = FTW_F; else
	if(S_ISDIR(st.st_mode)) {
	    flag = FTW_D;
#ifdef FTW_DNR
	    if(access(fname, R_OK)) flag = FTW_DNR;
#endif
	}
    }

    if(fn) {
	if((*fn)(fname, &st, flag)) {
	    *stopwalk = 1;
	} else if(flag == FTW_D) {
	    if(stubnftw(fname, fn, 0, 1)) *stopwalk = 1;
	}
    }
}

int stubnftw(const char *dir, int (*fn)(const char *file, const struct stat *sb, int flag), int depth, int flags) {
    DIR *dr;
    struct dirent *ent;
    char *fname;
    int stopwalk = 0;

    if(!flags) {
	stepftw(dir, &stopwalk, fn);
    } else if(dr = opendir(dir)) {
	while((ent = readdir(dr)) && !stopwalk) {
	    if( !strcmp(ent->d_name, ".") ||
		!strcmp(ent->d_name, "..")) continue;

	    fname = (char *) malloc(strlen(dir) + strlen(ent->d_name) + 2);
	    strcpy(fname, dir);
	    if(fname[strlen(fname)] != '/') strcat(fname, "/");
	    strcat(fname, ent->d_name);

	    stepftw(fname, &stopwalk, fn);
	    free(fname);
	}

	closedir(dr);
    }

    return stopwalk;
}

const string ktool::pathfind(const string name, string path, int amode = F_OK) {
    string token, current;

    while(!(token = getword(path, ":")).empty()) {
	current = token + "/" + name;
	if(!access(current.c_str(), amode)) return current;
    }

    return "";
}

bool ktool::mksubdirs(string dir) {
    string subname, created;
    bool errhappen = false;

    if(!dir.empty())
    if(dir[0] == '/') created = "/";

    while(!dir.empty() && !errhappen) {
	subname = getword(dir, "/");

	if(!created.empty())
	if(*(created.end()-1) != '/') created += "/";

	created += subname;

	if(access(created.c_str(), F_OK))
	    errhappen = mkdir(created.c_str(), S_IRWXU);
    }

    return !errhappen;
}

const string ktool::readlink(const string fname) {
    char rfname[1024];
    int n;

    if((n = ::readlink(fname.c_str(), rfname, 1024)) != -1) {
	rfname[n] = 0;
	return rfname;
    } else {
	return "";
    }
}
