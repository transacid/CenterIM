/*
*
* kkstrtext string related and text processing routines
* $Id: kkstrtext.cc,v 1.11 2002/02/20 17:18:43 konst Exp $
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

#include "kkstrtext.h"

int kwordcount(const char *strin, const char *delim) {
    int count = 0, i = 0, slen = strlen(strin);

    while(i<slen) {
	for( ; i < slen && strchr(delim, strin[i]); i++);
	if(i < slen) count++;
	for( ; i < slen && !strchr(delim, strin[i]); i++);
    }

    return count;
}

char *kgetword(int wordn, const char *strin, const char *delim, char *sout, int maxout) {
    int i = 0, count = 0, len = 0, slen = strlen(strin);

    sout[0] = 0;
    while(i < slen && count != wordn) {
	while(i < slen && strchr(delim, strin[i])) i++;
	if(i < slen) count++;
	while(i < slen && !strchr(delim, strin[i])) {
	    if(count == wordn) {
		sout[++len] = 0;
		sout[len-1] = strin[i];
	    }
	    i++;
	}
    }
    return sout;
}

int kwordcountq(char *strin, char *delim, char *q) {
    int slen = strlen(strin), count = 0, i = -1, quoted = 0, prevbs = 0;

    while(i<slen) {
	while(i<slen) {
	    i++;
	    if(strchr(q, strin[i]) && !prevbs) break;
	    if(!strchr(delim, strin[i])) break;
	    prevbs = (strin[i] == '\\');
	}
    
	if(i < slen) count++;
	while(i < slen && (quoted || !strchr(delim, strin[i]))) {
	    if(strchr(q, strin[i]) && !prevbs) quoted = !quoted;
	    prevbs = (strin[i] == '\\');
	    i++;
	}
    }

    return count;
}

int kwordposq(int n, char *s, char *delim, char *q) {
    int count = 0, slen = strlen(s), i = 0, quoted = 0, retval = -1;

    while(i < slen && count != n) {
	if(i < slen && !strchr(q, s[i]) && strchr(delim, s[i])) {
	    while(strchr(delim, s[i]) && i < slen) i++;
	}

	if(i < slen) count++;

	if(count != n)
	while(i < slen && (quoted || !strchr(delim, s[i]))) {
	    if(strchr(q, s[i]))
	    if(s[i-1] != '\\' || !i) quoted =! quoted;
	    i++;
	} else {
	    if(strchr(delim, s[i])) i++;
	    retval = i;
	}
    }
  
    return retval;
}

char *kgetwordq(int wordn, char *strin, char *delim, char *q, char *sout, int maxout) {
    int slen = strlen(strin), len = 0, quoted = 0, i = kwordposq(wordn, strin, delim, q);
    sout[0] = 0;

    if(i >= 0) {
	while(i < slen && len < maxout && (quoted || !strchr(delim, strin[i]))) {
	    if(strchr(q, strin[i])) {
		if(strin[i-1] != '\\' || !i) quoted =! quoted; else
		if(len > 0) len--;
	    }
      
	    sout[len++] = strin[i++];
	}
    }

    sout[len] = 0;
    return sout;
}

char *kstrcopy(char *strin, int frompos, int count, char *strout) {
    strcpy(strout, strin);
    strout += frompos-1;
    strout[count] = 0;
    return strout;
}

char *strcut(char *strin, int frompos, int count) {
    if(count > 0) {
	if(count > strlen(strin)-frompos) count = strlen(strin)-frompos;
	char *buf = (char *) malloc(strlen(strin) - frompos - count + 1);
	memcpy(buf, strin + frompos + count, strlen(strin) - frompos - count);
	memcpy(strin + frompos, buf, strlen(strin) - frompos - count);
	strin[strlen(strin) - count] = 0;
	free(buf);
    }
    return strin;
}

char *kcuturls(char *strin, char *strout) {
    int i, j;

    strout[0] = 0;
    for(i = 0; i < strlen(strin); i++) {
	if(!strncasecmp(strin + i, "http://", 7) || !strncasecmp(strin + i, "ftp://", 6))
	for( ; !strchr(",;*& \000", strin[i]); i++); else {
	    j = strlen(strout);
	    strout[j] = strin[i];
	    strout[j + 1] = 0;
	}
    }

    return strout;
}

char *strimlead(char *str)  { return trimlead(str, " \t");  }
char *strimtrail(char *str) { return trimtrail(str, " \t"); }
char *strim(char *str)      { return trim(str, " \t");      }

char *trimlead(char *str, char *chr) {
    while(strchr(chr, str[0]) && strlen(str)) strcpy(str, str + 1);
    return str;
}

char *trimtrail(char *str, char *chr) {
    while(strchr(chr, str[strlen(str)-1]) && strlen(str)) str[strlen(str)-1] = 0;
    return str;
}

char *trim(char *str, char *chr) {
    return trimlead(trimtrail(str, chr), chr);
}

char *time2str(const time_t *t, char *mask, char *sout) {
    struct tm *s;
    char ch, b[10], b1[20];
    int len, i, j;

    sout[0] = 0;
    s = localtime(t);

    for(i = 0; i < strlen(mask); i++) {
	len = 0;
    
	if(strchr("DMYhms", ch = mask[i])) {
	    j = i; len = 1;
	    while(mask[++j] == ch) len++;
	    sprintf(b, "%%0%dd", len);
	    i += len-1;
      
	    switch(ch) {
		case 'D': sprintf(b1, b, s->tm_mday); break;
		case 'M': sprintf(b1, b, s->tm_mon+1); break;
		case 'Y':
		    j = s->tm_year + 1900;
		    sprintf(b1, b, j);
		    if(len <= 3) strcut(b1, 0, 2);
		    break;
		case 'h': sprintf(b1, b, s->tm_hour); break;
		case 'm': sprintf(b1, b, s->tm_min); break;
		case 's': sprintf(b1, b, s->tm_sec); break;
	    }
	    strcat(sout, b1);
	} else {
	    len = strlen(sout);
	    sout[len+1] = 0;
	    sout[len] = mask[i];
	}
    }
    return sout;
}

time_t str2time(char *sdate, char *mask, time_t *t) {
    struct tm *s;
    int i, len, j, k;
    char ch, b[10];

    s = (struct tm*) malloc(sizeof(struct tm));
  
    for(i = 0; i < strlen(mask); i++) {
	len = 0;
    
	if(strchr("DMYhms", ch = mask[i])) {
	    j = i; len = 1;
	    while(mask[++j] == ch) len++;
	    i += len-1;

	    b[0] = 0;
	    for(j = i-len+1; j < i+1; j++) {
		k = strlen(b);
		b[k+1] = 0;
		b[k] = sdate[j];
	    }
      
	    switch(ch) {
		case 'D': s->tm_mday=atoi(b); break;
		case 'M': s->tm_mon=atoi(b); s->tm_mon--; break;
		case 'Y': s->tm_year=atoi(b); s->tm_year-=1900; break;
		case 'h': s->tm_hour=atoi(b); s->tm_hour--; break;
		case 'm': s->tm_min=atoi(b); break;
		case 's': s->tm_sec=atoi(b); break;
	    }
	}
    }

    s->tm_isdst = -1;
    *t = mktime(s);
    free(s);
    return *t;
}

const string unmime(const string text) {
    string r;
    char *buf = new char[text.size()+1];
    strcpy(buf, text.c_str());
    r = unmime(buf);
    delete buf;
    return r;
}

const string mime(const string text) {
    string r;
    char *buf = new char[text.size()*3+1];
    r = mime(buf, text.c_str());
    delete buf;
    return r;
}

char *unmime(char *text) {
    register int s, d;
    int htm;

    for(s = 0, d = 0; text[s] != 0; s++) {
	if(text[s] == '+') text[d++] = ' '; else
	if(text[s] == '%') {
	    sscanf(text + s + 1, "%2x", &htm);
	    text[d++] = htm;
	    s += 2;
	} else
	    text[d++] = text[s];
    }

    text[d] = 0;
    return(text);
}

char *mime(char *dst, const char *src) {
    register int s, d;
    char c;

    for(s = 0, d = 0; src[s]; s++) {
	if((src[s] >= 'a' && src[s] <= 'z') || 
	   (src[s] >= 'A' && src[s] <= 'Z') ||
	   (src[s] >= '0' && src[s] <= '9')) dst[d++] = src[s]; else {
	    if(src[s] != ' ') {
		dst[d++] = '%';
		c = (src[s] >> 4 & 0x0F);
		dst[d++] = (c > 9) ? 'A'+c-10 : '0'+c;
		c = (src[s] & 0x0F);
		dst[d++] = (c > 9) ? 'A'+c-10 : '0'+c;
	    } else
		dst[d++] = '+';
	}
    }
  
    dst[d] = '\0';
    return(dst);
}

char *strccat(char *dest, char c) {
    int k = strlen(dest);
    dest[k] = c;
    dest[k+1] = 0;
    return dest;
}

vector<int> getquotelayout(const string haystack, const string qs, const string aescs) {
    vector<int> r;
    string needle, escs;
    int pos, prevpos, curpos;
    char cchar, qchar, prevchar;

    qchar = 0;
    curpos = prevpos = 0;
    escs = (qs == aescs) ? "" : aescs;
    needle = qs + escs;

    while((pos = haystack.substr(curpos).find_first_of(needle)) != -1) {
	curpos += pos;
	cchar = *(haystack.begin()+curpos);

	if(escs.find(cchar) != -1) {
	    if(qchar)
	    if(prevpos == curpos-1)
	    if(escs.find(prevchar) != -1) {
		/* Neutralize previous esc char */
		cchar = 0;
	    }
	} else if(qs.find(cchar) != -1) {
	    if(!((escs.find(prevchar) != -1) && (prevpos == curpos-1))) {
		/* Wasn't an escape (right before this quote char) */

		if(!qchar || (qchar == cchar)) {
		    qchar = qchar ? 0 : cchar;
		    r.push_back(curpos);
		}
	    }
	}

	prevpos = curpos++;
	prevchar = cchar;
    }

    return r;
}

vector<int> getsymbolpositions(const string haystack, const string needles, const string qoutes, const string esc) {
    vector<int> r, qp, nr;
    vector<int>::iterator iq, ir;
    int pos, st, ed, cpos;

    for(cpos = 0; (pos = haystack.substr(cpos).find_first_of(needles)) != -1; ) {
	r.push_back(cpos+pos);
	cpos += pos+1;
    }

    qp = getquotelayout(haystack, qoutes, esc);
    for(iq = qp.begin(); iq != qp.end(); iq++) {
	if(!((iq-qp.begin()) % 2)) {
	    st = *iq;
	    ed = iq+1 != qp.end() ? *(iq+1) : haystack.size();
	    nr.clear();

	    for(ir = r.begin(); ir != r.end(); ir++) {
		if(!(*ir > st && *ir < ed)) {
		    nr.push_back(*ir);
		}
	    }

	    r = nr;
	}
    }

    return r;
}

#define CHECKESC(curpos, startpos, esc) \
    if(curpos > startpos+1) \
    if(strchr(esc, *(curpos-1))) \
    if(!strchr(esc, *(curpos-2))) { \
	curpos++; \
	continue; \
    }

const char *strqpbrk(const char *s, int offset,
const char *accept, const char *q, const char *esc = "") {
    if(!s) return 0;
    if(!s[0]) return 0;

    char qchar = 0;
    const char *ret = 0, *p = s;
    char *cset = (char *) malloc(strlen(accept)+strlen(q)+1);
    
    strcpy(cset, accept);
    strcat(cset, q);

    while(p = strpbrk(p, cset)) {
	if(strchr(q, *p)) {
	    if(strcmp(esc, q))
		CHECKESC(p, s, esc);

	    if(!qchar) {
		qchar = *p;
	    } else {
		if(*p == qchar) qchar = 0;
	    }
	} else if((p-s >= offset) && !qchar) {
	    ret = p;
	    break;
	}
	p++;
    }

    free(cset);
    return ret;
}

const char *strqcasestr(const char *s, const char *str,
const char *q, const char *esc = "") {
    char quote = 0;
    int i;

    for(i = 0; i < strlen(s); i++) {
	if(strchr(q, s[i])) {
	    if(strcmp(esc, q))
		CHECKESC(s+i, s, esc);
	    quote = !quote;
	}

	if(!quote)
	if(!strncasecmp(s + i, str, strlen(str))) return s + i;
    }

    return 0;
}

const char *strqstr(const char *s, const char *str,
const char *q, const char *esc = "") {
    char quote;
    const char *ret = 0, *p, *ss, *r;
    p = ss = s;

    while(p = strstr(ss, str)) {
	quote = 0;
	r = s;

	while(r = strpbrk(r, q)) {
	    if(r > p) break;
	    if(strcmp(esc, q))
		CHECKESC(r, s, esc);
	    quote = !quote;
	    r++;
	}
	
	if(!quote) {
	    ret = p;
	    break;
	} else {
	    ss = p+strlen(str);
	}
    }

    return ret;
}

// select %list from %word where %any
// insert [(%list)] values (%list) into %word
// update [table] %word %any where %any

int parseformat(char *command, char *quotes, char *fmt, ...) {
    const char *cp = command, *fnp, *fp;
    char *p, *t;
    const char *cnp;
    va_list ap;

    va_start(ap, fmt);
    fnp = fp = fmt;

    while(fp && cp && *fp && *cp) {
	fnp = strqpbrk(fp+1, 0, "%[]", quotes);
	for(; *fp && strchr(" ]", *fp); fp++);
	for(; *cp && *cp == ' '; cp++);

	switch(*fp) {
	    case '[':
		if(strncasecmp(fp+1, cp, fnp-fp-1)) {
		    if(!(fp = strqpbrk(fp+1, 0, "]", quotes)));
		} else {
		    cp += fnp-fp-1;
		    fp += fnp-fp;
		}
		break;
	    case '%':
		if(!strncmp(fp+1, "list", 4)) {
		    fp += 5;
		    fnp = strqpbrk(fp+1, 0, "%[]", quotes);
		    p = (char *) malloc(fnp-fp+1);
		    p[fnp-fp] = 0;
		    strncpy(p, fp, fnp-fp);
		    cnp = strqcasestr(cp+1, p, quotes);
		    free(p);

		    t = va_arg(ap, char *);
		    t[cnp-cp] = 0;
		    strncpy(t, cp, cnp-cp);

		    cp += cnp-cp;
		} else if(!strncmp(fp+1, "word", 4)) {
		    fp += 5;
		    if(!(cnp = strqpbrk(cp, 0, ";() ,.", "'"))) cnp = cp+strlen(cp);
		    t = va_arg(ap, char *);
		    t[cnp-cp] = 0;
		    strncpy(t, cp, cnp-cp);

		    cp += cnp-cp;
		} else if(!strncmp(fp+1, "any", 3)) {
		}
		break;
	    default:
		if(!strncasecmp(fp, cp, fnp-fp)) {
		    cp += fnp-fp;
		    fp += fnp-fp;
		} else;
	}
    }

    va_end(ap);
    return 1;
}

char *strinsert(char *buf, int pos, char *ins) {
    char *p = strdup(buf+pos);
    memcpy(buf+pos+strlen(ins), p, strlen(p)+1);
    memcpy(buf+pos, ins, strlen(ins));
    free(p);
    return buf;
}

char *strcinsert(char *buf, int pos, char ins) {
    char *p = strdup(buf+pos);
    memcpy(buf+pos+1, p, strlen(p)+1);
    buf[pos] = ins;
    free(p);
    return buf;
}

int strchcount(char *s, char *accept) {
    char *p = s-1;
    int ret = 0;
    while(p = strpbrk(p+1, accept)) ret++;
    return ret;
}

int stralone(char *buf, char *startword, int wordlen, char *delim) {
    int leftdelim = 0, rightdelim = 0;
    leftdelim = (buf != startword && strchr(delim, *(startword-1))) || buf == startword;
    rightdelim = !*(startword+wordlen) || strchr(delim, *(startword+wordlen));
    return leftdelim && rightdelim;
}

const string justfname(const string fname) {
    return fname.substr(fname.rfind("/")+1);
}

const string justpathname(const string fname) {
    int pos;
    
    if((pos = fname.rfind("/")) != -1) {
	return fname.substr(0, pos);
    } else {
	return "";
    }
}

void charpointerfree(void *p) {
    char *cp = (char *) p;
    if(cp) delete cp;
}

void nothingfree(void *p) {
}

int stringcompare(void *s1, void *s2) {
    if(!s1 || !s2) {
	return s1 != s2;
    } else {
	return strcmp((char *) s1, (char *) s2);
    }
}

int intcompare(void *s1, void *s2) {
    return (int) s1 != (int) s2;
}

const string i2str(int i) {
    char buf[64];
    sprintf(buf, "%d", i);
    return (string) buf;
}

const string ui2str(int i) {
    char buf[64];
    sprintf(buf, "%du", i);
    return (string) buf;
}

const string textscreen(string text) {
    for(int i = 0; i < text.size(); i++)
    if(!isalnum(text[i])) text.insert(i++, "\\");
    
    return text;
}

const string leadcut(string base, string delim = "\t\n\r ") {
    while(!base.empty()) {
	if(strchr(delim.c_str(), base[0])) {
	    base.replace(0, 1, "");
	} else break;
    }
    return base;
}

const string trailcut(string base, string delim = "\t\n\r ") {
    while(!base.empty()) {
	if(strchr(delim.c_str(), base[base.size()-1])) {
	    base.resize(base.size()-1);
	} else break;
    }
    
    return base;
}

const string getword(string &base, string delim = "\t\n\r ") {
    string sub;
    int i;

    base = leadcut(base, delim);
    
    for(i = 0, sub = base; i < sub.size(); i++)
    if(strchr(delim.c_str(), sub[i])) {
	sub.resize(i);
	base.replace(0, i, "");
	base = leadcut(base, delim);
	break;
    }

    if(sub == base) base = "";
    return sub;
}

const string getwordquote(string &base, string quote = "\"", string delim = "\t\n\r ") {
    string sub;
    bool inquote = false;
    int i;

    base = leadcut(base, delim);

    for(i = 0, sub = base; i < sub.size(); i++) {
	if(strchr(quote.c_str(), sub[i])) {
	    inquote = !inquote;
	} else if(!inquote && strchr(delim.c_str(), sub[i])) {
	    sub.resize(i);
	    base.replace(0, i, "");
	    base = leadcut(base, delim);
	    break;
	}
    }

    if(sub == base) base = "";
    return sub;
}

const string getrword(string &base, string delim = "\t\n\r ") {
    string sub;
    int i;

    base = trailcut(base, delim);
    
    for(i = base.size()-1, sub = base; i >= 0; i--)
    if(strchr(delim.c_str(), base[i])) {
	sub = base.substr(i+1);
	base.resize(i);
	base = trailcut(base, delim);
	break;
    }

    if(sub == base) base = "";
    return sub;
}

const string getrwordquote(string &base, string quote = "\"", string delim = "\t\n\r ") {
    string sub;
    bool inquote = false;
    int i;

    base = trailcut(base, delim);
    
    for(i = base.size()-1, sub = base; i >= 0; i--)
    if(strchr(quote.c_str(), base[i])) {
	inquote = !inquote;
    } else if(!inquote && strchr(delim.c_str(), base[i])) {
	sub = base.substr(i+1);
	base.resize(i);
	base = trailcut(base, delim);
	break;
    }

    if(sub == base) base = "";
    return sub;
}

int rtabmargin(bool fake, int curpos, const char *p = 0) {
    int ret = -1, n, near;

    if(p && (curpos != strlen(p))) {
	n = strspn(p+curpos, " ");

	if(fake) {
	    near = ((curpos/(TAB_SIZE/2))+1)*(TAB_SIZE/2);
	    if(n >= near-curpos) ret = near;
	}

	near = ((curpos/TAB_SIZE)+1)*TAB_SIZE;
	if(n >= near-curpos) ret = near;
    } else {
	if(p && fake) fake = (strspn(p, " ") == strlen(p));
	if(fake) ret = ((curpos/(TAB_SIZE/2))+1)*(TAB_SIZE/2);
	else ret = ((curpos/TAB_SIZE)+1)*TAB_SIZE;
    }

    return ret;
}

int ltabmargin(bool fake, int curpos, const char *p = 0) {
    int ret = -1, near, n = 0;
    const char *cp;
    
    if(p) {
	cp = p+curpos;

	if(curpos) {
	    if(*(--cp) == ' ') n++;
	    for(; (*cp == ' ') && (cp != p); cp--) n++;
	}

	if(fake) {
	    near = (curpos/(TAB_SIZE/2))*(TAB_SIZE/2);
	    if(near <= curpos-n)
	    if((ret = curpos-n) != 0) ret++;
	}

	near = (curpos/TAB_SIZE)*TAB_SIZE;
	if(near <= curpos-n) {
	    if((ret = curpos-n) != 0) ret++;
	} else ret = near;
	
    } else {
	if(fake) ret = (curpos/(TAB_SIZE/2))*(TAB_SIZE/2);
	else ret = (curpos/TAB_SIZE)*TAB_SIZE;
    }
    
    return ret;
}

void breakintolines(string text, vector<string> &lst, int linelen = 0) {
    int npos, dpos, i;
    string sub;

    lst.clear();
    while((npos = text.find("\r")) != -1) text.erase(npos, 1);

    while(!text.empty()) {
	if((npos = text.find("\n")) == -1)
	    npos = text.size();

	while((dpos = text.substr(0, npos).find("\t")) != -1) {
	    i = rtabmargin(false, dpos)-dpos;
	    npos += i-1;
	    text.erase(dpos, 1);
	    text.insert(dpos, sub.assign(i, ' '));
	}

	if(linelen)
	if(npos > linelen) {
	    npos = linelen;
	    if((dpos = text.substr(0, npos).find_last_of(" \t")) != -1)
		npos = dpos;
	}

	sub = text.substr(0, npos);
	text.erase(0, npos);

	if(text.substr(0, 1).find_first_of("\n ") != -1)
	    text.erase(0, 1);

	lst.push_back(sub);
    }
}

void find_gather_quoted(vector<quotedblock> &lst, const string &str,
const string &quote, const string &escape) {
    bool inquote = false;
    int npos = 0, qch;
    quotedblock qb;

    while((npos = str.find_first_of(quote, npos)) != -1) {
	if(npos)
	if(escape.find(str[npos-1]) == -1) {
	    inquote = !inquote;

	    if(inquote) {
		qb.begin = npos;
		qch = str[npos];
	    } else {
		if(str[npos] == qch) {
		    qb.end = npos;
		    lst.push_back(qb);
		} else {
		    inquote = true;
		}
	    }
	}
	npos++;
    }
}

int find_quoted(const string &str, const string &needle, int offs = 0,
const string &quote = "\"'", const string &escape = "\\") {
    vector<quotedblock> positions;
    vector<quotedblock>::iterator qi;
    int npos = offs;
    bool found;

    find_gather_quoted(positions, str, quote, escape);

    while((npos = str.find(needle, npos)) != -1) {
	for(found = false, qi = positions.begin(); qi != positions.end() && !found; qi++)
	    if((npos > qi->begin) && (npos < qi->end)) found = true;

	if(!found) break;
	npos++;
    }

    return !found ? npos : -1;
}

int find_quoted_first_of(const string &str, const string &needle, int offs = 0,
const string &quote = "\"'", const string &escape = "\\") {
    vector<quotedblock> positions;
    vector<quotedblock>::iterator qi;
    int npos = offs;
    bool found;

    find_gather_quoted(positions, str, quote, escape);

    while((npos = str.find_first_of(needle, npos)) != -1) {
	for(found = false, qi = positions.begin(); qi != positions.end() && !found; qi++)
	    if((npos > qi->begin) && (npos < qi->end)) found = true;

	if(!found) break;
	npos++;
    }

    return !found ? npos : -1;
}

void splitlongtext(string text, vector<string> &lst, int size = 440, const string cont = "\n[continued]") {
    string sub;
    int npos;

    lst.clear();

    while(!text.empty()) {
	if(text.size() <= size-cont.size()) {
	    npos = text.size();
	} else if((npos = text.substr(0, size-cont.size()).find_last_of(" \t")) == -1) {
	    npos = size-cont.size();
	}

	sub = text.substr(0, npos);
	text.erase(0, npos);

	if(text.size() > cont.size()) sub += cont; else {
	    sub += text;
	    text = "";
	}

	if((npos = text.find_first_not_of(" \t")) != -1)
	    text.erase(0, npos);

	lst.push_back(sub);
    }
}

const string strdateandtime(time_t stamp, string fmt = "") {
    return strdateandtime(localtime(&stamp), fmt);
}

const string strdateandtime(struct tm *tms, string fmt = "") {
    char buf[512];
    time_t current_time = time(0);
    time_t when = mktime(tms);

    if(fmt.empty()) {
	if(current_time > when + 6L * 30L * 24L * 60L * 60L /* Old. */
	|| current_time < when - 60L * 60L)                 /* Future. */ {
	    fmt = "%b %e  %Y";
	} else {
	    fmt = "%b %e %H:%M";
	}
    }

    strftime(buf, 512, fmt.c_str(), tms);
    return buf;
}

bool iswholeword(const string s, int so, int eo) {
    bool rm, lm;
    const string wdelims = "[](),.; <>-+{}=|&%~*/:?@";

    lm = !so || (wdelims.find(s.substr(so-1, 1)) != -1);
    rm = (eo == s.size()-1) || (wdelims.find(s.substr(eo, 1)) != -1);

    return rm && lm;
}

int hex2int(const string ahex) {
    int r, i;

    r = 0;

    if(ahex.size() <= 2) {
	for(i = 0; i < ahex.size(); i++) {
	    r += isdigit(ahex[i]) ? ahex[i]-48 : toupper(ahex[i])-55;
	    if(!i) r *= 16;
	}
    }

    return r;
}

bool getconf(string &st, string &buf, ifstream &f) {
    bool ret = false;
    static string sect;

    while(!f.eof() && !ret) {
	getstring(f, buf);

	if(buf.size())
	switch(buf[0]) {
	    case '%':
		sect = buf.substr(1);
		break;
	    case '#':
		if(buf[1] != '!') break;
	    default:
		ret = buf.size();
		break;
	}
    }

    st = sect;
    return ret;
}

bool getstring(istream &f, string &sbuf) {
    static char buf[2048];
    bool r;

    if(r = !f.eof()) {
	sbuf = "";

	do {
	    f.clear();
	    f.getline(buf, 2048);
	    sbuf += buf;
	} while(!f.good() && !f.eof());
    }

    return r;
}
