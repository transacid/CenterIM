#ifndef __KONST_STRING_H_
#define __KONST_STRING_H_

#include <string>
#include <vector>

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "conf.h"

#define randlimit(l, h) ((int)((float)rand()/(float)RAND_MAX*(float)(h+1-l)+(float)l))

#define SWAPVAL(v1, v2) { \
    int SWAPVAL__v = v2; \
    v2 = v1, v1 = SWAPVAL__v; \
}

#define PRINTNULL(p) (p ? p : "")
#define TAB_SIZE      8

struct quotedblock {
    int begin, end;
};

void charpointerfree(void *p);
void nothingfree(void *p);
int stringcompare(void *s1, void *s2);
int intcompare(void *s1, void *s2);

const string leadcut(string base, string delim = "\t\n\r ");
const string trailcut(string base, string delim = "\t\n\r ");

const string getword(string &base, string delim = "\t\n\r ");
const string getrword(string &base, string delim = "\t\n\r ");

const string getwordquote(string &base, string quote = "\"", string delim = "\t\n\r ");
const string getrwordquote(string &base, string quote = "\"", string delim = "\t\n\r ");

int rtabmargin(bool fake, int curpos, const char *p = 0);
int ltabmargin(bool fake, int curpos, const char *p = 0);
void breakintolines(string text, vector<string> &lst, int linelen = 0);

void find_gather_quoted(vector<quotedblock> &lst, const string &str,
    const string &quote, const string &escape);

int find_quoted(const string &str, const string &needle, int offs = 0,
    const string &quote = "\"'", const string &escape = "\\");

int find_quoted_first_of(const string &str, const string &needle,
    int offs = 0, const string &quote = "\"'",
    const string &escape = "\\");

void splitlongtext(string text, vector<string> &lst,
    int size = 440, const string cont = "\n[continued]");

const string strdateandtime(time_t stamp, string fmt = "");
const string strdateandtime(struct tm *tms, string fmt = "");

bool iswholeword(const string s, int so, int eo);

int hex2int(const string ahex);

vector<int> getquotelayout(const string haystack, const string qs,
    const string aescs);

vector<int> getsymbolpositions(const string haystack,
    const string needles, const string qoutes, const string esc);

__KTOOL_BEGIN_C

int kwordcount(const char *strin, const char *delim);
char *kgetword(int wordn, const char *strin, const char *delim, char *sout, int maxout);

int kwordcountq(char *strin, char *delim, char *q);
int kwordposq(int n, char *s, char *delim, char *q);
char *kgetwordq(int wordn, char *strin, char *delim, char *q, char *sout, int maxout);

char *strcut(char *strin, int frompos, int count);
char *kstrcopy(char *strin, int frompos, int count, char *strout);
char *kcuturls(char *strin, char *strout);
char *trimlead(char *str, char *chr);
char *trimtrail(char *str, char *chr);
char *trim(char *str, char *chr);

char *strimlead(char *str);
char *strimtrail(char *str);
char *strim(char *str);

const char *strqpbrk(
    const char *s, int offset,
    const char *accept,
    const char *q,
    const char *esc = "");

const char *strqcasestr(
    const char *s,
    const char *str,
    const char *q,
    const char *esc = "");
    
const char *strqstr(const char *s,
    const char *str,
    const char *q,
    const char *esc = "");

char *strccat(char *dest, char c);
char *strinsert(char *buf, int pos, char *ins);
char *strcinsert(char *buf, int pos, char ins);

int strchcount(char *s, char *accept);
int stralone(char *buf, char *startword, int wordlen, char *delim);

char *time2str(const time_t *t, char *mask, char *sout);
time_t str2time(char *sdate, char *mask, time_t *t);

char *unmime(char *text);
char *mime(char *dst, const char *src);

int parseformat(char *command, char *quotes, char *fmt, ...);

const string justfname(const string fname);
const string justpathname(const string fname);

const string textscreen(const string text);
const string i2str(int i);
const string ui2str(int i);

__KTOOL_END_C

#endif
