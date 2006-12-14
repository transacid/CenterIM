/*
aim.c - FireTalk generic AIM definitions
Copyright (C) 2000 Ian Gulliver

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "firetalk-int.h"
#include "firetalk.h"
#include "aim.h"
#include "safestring.h"

char *aim_interpolate_variables(const char * const input, const char * const nickname) {
	static char output[16384]; /* 2048 / 2 * 16 + 1 (max size with a string full of %n's, a 16-char nick and a null at the end) */
	int o = 0,gotpercent = 0;
	size_t nl,dl,tl,l,i;
	char date[15],tim[15];
	{ /* build the date and time */
		int hour;
		int am = 1;
		struct tm *t;
		time_t b;
		b = time(NULL);
		t = localtime(&b);
		if (t == NULL)
			return NULL;
		hour = t->tm_hour;
		if (hour >= 12)
			am = 0;
		if (hour > 12)
			hour -= 12;
		if (hour == 0)
			hour = 12;
		sprintf(tim,"%d:%02d:%02d %s",hour,t->tm_min,t->tm_sec,am == 1 ? "AM" : "PM");
		safe_snprintf(date,15,"%d/%d/%d",t->tm_mon + 1,t->tm_mday,t->tm_year + 1900);
	}
	nl = strlen(nickname);
	dl = strlen(date);
	tl = strlen(tim);
	l = strlen(input);
	for (i = 0; i < l; i++) {
		switch (input[i]) {
			case '%':
				if (gotpercent == 1) {
					gotpercent = 0;
					output[o++] = '%';
					output[o++] = '%';
				} else
					gotpercent = 1;
				break;
			case 'n':
				if (gotpercent == 1) {
					gotpercent = 0;
					memcpy(&output[o],nickname,nl);
					o += nl;
				} else
					output[o++] = 'n';
				break;
			case 'd':
				if (gotpercent == 1) {
					gotpercent = 0;
					memcpy(&output[o],date,dl);
					o += dl;
				} else
					output[o++] = 'd';
				break;
			case 't':
				if (gotpercent == 1) {
					gotpercent = 0;
					memcpy(&output[o],tim,tl);
					o += tl;
				} else
					output[o++] = 't';
				break;
			default:
				if (gotpercent == 1) {
					gotpercent = 0;
					output[o++] = '%';
				}
				output[o++] = input[i];

		}
	}
	output[o] = '\0';
	return output;
}

const char * const aim_normalize_room_name(const char * const name) {
	static char newname[2048];
	if (name == NULL)
		return NULL;
	if (strchr(name,':'))
		return name;
	if (strlen(name) > 2045)
		return NULL;
	safe_strncpy(newname,"4:",2048);
	safe_strncat(newname,name,2048);
	return newname;
}

char *aim_handle_ect(void *conn, const char * const from, char * message, const int reply) {
	char *tempchr1, *tempchr2;

	while ((tempchr1 = strstr(message,"<!--ECT "))) {
		if ((tempchr2 = strstr(&tempchr1[8],"-->"))) {
			/* valid ECT */
			char *endcommand;
			*tempchr2 = '\0';
			endcommand = strchr(&tempchr1[8],' ');
			if (endcommand) {
				*endcommand = '\0';
				endcommand++;
				if (reply == 1)
					firetalk_callback_subcode_reply(conn,from,&tempchr1[8],endcommand);
				else
					firetalk_callback_subcode_request(conn,from,&tempchr1[8],endcommand);
			} else {
				if (reply == 1)
					firetalk_callback_subcode_reply(conn,from,&tempchr1[8],NULL);
				else
					firetalk_callback_subcode_request(conn,from,&tempchr1[8],NULL);
			}
			memmove(tempchr1,&tempchr2[3],strlen(&tempchr2[3]) + 1);
		}
	}
	return message;
}
