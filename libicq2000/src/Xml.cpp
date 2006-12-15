/*
 * XML Parser/Generator
 *
 * Copyright (C) 2001 Barnaby Gray <barnaby@beedesign.co.uk>
 * improvements (C) 2003 Konstantin Klyagin <konst@konst.org.ua>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#include "Xml.h"

using std::string;
using std::list;
using std::pair;

// ---------- XmlNode ---------------------------

XmlNode::XmlNode(const string& at) { parseAttr(at); }

XmlNode::~XmlNode() { }

bool XmlNode::isLeaf() { return !isBranch(); }

string XmlNode::getTag() { return tag; }

string XmlNode::replace_all(const string& s, const string& r1, const string& r2) {
  string t(s);
  int curr = 0, next;
  while ( (next = t.find( r1, curr )) != -1) {
    t.replace( next, r1.size(), r2 );
    curr = next + r2.size();
  }
  return t;
}

string XmlNode::quote(const string& a) {
  return 
    replace_all(
    replace_all(
    replace_all(
    replace_all(
    replace_all(a,
    "&", "&amp;"),
    "<", "&lt;"),
    "\"", "&quot;"),
    "`", "&apos;"),
    ">", "&gt;");
}

string XmlNode::unquote(const string& a) {
  return 
    replace_all(
    replace_all(
    replace_all(
    replace_all(
    replace_all(
    replace_all(
    replace_all(a,
    "&lt;", "<"),
    "&gt;", ">"),
    "&quot;", "\""),
    "&apos;", "`"),
    "&nbsp;", " "),
    "&trade;", "(tm)"),
    "&amp;", "&");
}

XmlNode *XmlNode::parse(string::iterator& curr, string::iterator end) {
  int npos;
  string tag, tagname, cdata;

  do {
    skipWS(curr,end);
    if (curr == end || *curr != '<') return NULL;

    tag = parseTag(curr, end);
    tagname = tag;

    if (tag.empty() || tag[0] == '/') return NULL;

  } while(tag[0] == '?' || tag[0] == '!');

  if (*(tag.end()-1) == '/')
    return new XmlLeaf(unquote(tag.substr(0, tag.size()-1)),"");

  if ((npos = tagname.find_first_of(" \r\n\t")) != -1)
    tagname.erase(npos);

  skipWS(curr,end);
  if (curr == end) return NULL;

  if (string(curr, end).substr(0, 9) == "<![CDATA[") {
    string endtag;

    curr += 9;
    while (curr != end && endtag.size() < 3) {
      if (*curr == ']' || *curr == '>') {
	endtag += *curr;
	if(endtag != string("]]>").substr(0, endtag.size())) {
	  cdata += endtag;
	  endtag = "";
	}

      } else {
	cdata += *curr;
      }

      ++curr;
    }
  }

  if (*curr == '<' && cdata.empty()) {

    XmlNode *p = NULL;
    while (curr != end) {
      string::iterator mark = curr;
      string nexttag = parseTag(curr,end);
      if (nexttag.empty()) { if (p != NULL) delete p; return NULL; }
      if (nexttag[0] == '/') {
	// should be the closing </tag>
	if (nexttag.size() == tagname.size()+1 && nexttag.find(tagname,1) == 1) {
	  // is closing tag
	  if (p == NULL) p = new XmlLeaf(unquote(tag),"");
	  return p;
	} else {
	  if (p != NULL) delete p;
	  return NULL;
	}
      } else {
	if (p == NULL) p = new XmlBranch(unquote(tag));
	// an opening tag
	curr = mark;
	XmlNode *c = parse(curr,end);
	if (c != NULL) ((XmlBranch*)p)->pushnode(c);
      }
      skipWS(curr,end);
      if(curr == end || *curr != '<') {
	if (p != NULL) delete p;
	return NULL;
      }
    }

  } else {
    // XmlLeaf
    string value;
    while (curr != end && *curr != '<') {
      value += *curr;
      curr++;
    }

    if(curr == end) return NULL;
    string nexttag = parseTag(curr,end);
    if (nexttag.empty() || nexttag[0] != '/') return NULL;

    if(value.empty()) value = cdata;

    if (nexttag.size() == tagname.size()+1 && nexttag.find(tagname,1) == 1) {
      return new XmlLeaf(unquote(tag),unquote(value));
    } else {
      // error
      return NULL;
    }
  }
  
  // should never get here
  return NULL;
}

string XmlNode::parseTag(string::iterator& curr, string::iterator end) {
  string tag;
  if(curr == end || *curr != '<') return string();
  curr++;
  while(curr != end && *curr != '>') {
    tag += *curr;
    curr++;
  }
  if (curr == end) return string();
  curr++;
  return tag;
}

void XmlNode::skipWS(string::iterator& curr, string::iterator end) {
  while(curr != end && isspace(*curr)) curr++;
}

void XmlNode::parseAttr(const string &at) {
  string t(at);
  string::iterator is = t.begin();
  skipWS(is, t.end());
  tag = "";

  while(is != t.end() && !isspace(*is)) {
    tag += *is;
    is++;
  }

  char qchar;
  string aname, aval;

  while(is != t.end()) {
    aname = aval = "";
    skipWS(is, t.end());

    while(is != t.end() && !isspace(*is) && *is != '=') {
      aname += *is;
      is++;
    }

    if(is == t.end()) break;

    if(*is == '=') ++is;
    skipWS(is, t.end());

    if(is == t.end()) break;
    if(*is != '"' && *is != '\'') break;

    qchar = *is;
    is++;

    while(is != t.end() && *is != qchar) {
      aval += *is;
      is++;
    }

    if(is == t.end()) break;
    is++;

    attributes.push_back(make_pair(aname, aval));
  }
}

bool XmlNode::existsAttrib(const std::string& attrib) const {
  for(list<pair<string, string> >::const_iterator ia = attributes.begin(); ia != attributes.end(); ++ia) {
    if(ia->first == attrib) return true;
  }

  return false;
}

std::string XmlNode::getAttrib(const string &aname) const {
  for(list<pair<string, string> >::const_iterator ia = attributes.begin(); ia != attributes.end(); ++ia) {
    if(ia->first == aname)
      return ia->second;
  }

  return "";
}

// ----------- XmlBranch ------------------------

XmlBranch::XmlBranch(const string& t) : XmlNode(t) { }

XmlBranch::~XmlBranch() {
  list<XmlNode*>::iterator curr = children.begin();
  while (curr != children.end()) {
    delete (*curr);
    curr++;
  }
  children.clear();
}

bool XmlBranch::isBranch() { return true; }

bool XmlBranch::exists(const string& tag) {
  list<XmlNode*>::iterator curr = children.begin();
  while (curr != children.end()) {
    if ((*curr)->getTag() == tag) return true;
    curr++;
  }
  return false;
}

XmlNode *XmlBranch::getNode(const string& tag, int n) {
  int ncount = 0;
  list<XmlNode*>::iterator curr = children.begin();

  while (curr != children.end()) {
    if ((*curr)->getTag() == tag)
      if (ncount++ == n)
	return (*curr);

    curr++;
  }
  return NULL;
}

XmlBranch *XmlBranch::getBranch(const string& tag, int n) {
  XmlNode *t = getNode(tag, n);
  if (t == NULL || dynamic_cast<XmlBranch*>(t) == NULL) return NULL;
  return dynamic_cast<XmlBranch*>(t);
}

XmlLeaf *XmlBranch::getLeaf(const string& tag) {
  XmlNode *t = getNode(tag);
  if (t == NULL || dynamic_cast<XmlLeaf*>(t) == NULL) return NULL;
  return dynamic_cast<XmlLeaf*>(t);
}

void XmlBranch::pushnode(XmlNode *c) {
  children.push_back(c);
}

string XmlBranch::toString(int n) {
  string ret(n,'\t');
  ret += "<" + quote(tag) + ">\n";
  list<XmlNode*>::iterator curr = children.begin();
  while (curr != children.end()) {
    ret += (*curr)->toString(n+1);
    curr++;
  }
  ret += string(n,'\t') + "</" + quote(tag) + ">\n";

  return ret;
}

list<string> XmlBranch::getChildren() const {
  list<string> r;

  list<XmlNode*>::const_iterator curr = children.begin();
  while (curr != children.end()) {
    r.push_back((*curr)->getTag());
    curr++;
  }

  return r;
}

// ----------- XmlLeaf --------------------------

XmlLeaf::XmlLeaf(const string& t, const string& v)
  : XmlNode(t), value(unquote(v)) { }

XmlLeaf::~XmlLeaf() { }

bool XmlLeaf::isBranch() { return false; }

string XmlLeaf::getValue() { return value; }

string XmlLeaf::toString(int n) {
  string r = string(n,'\t') + "<" + quote(tag);
  list<pair<string, string> >::const_iterator ia = attributes.begin();
  while(ia != attributes.end()) {
    r += (string) " " + ia->first + "=\"" + ia->second + "\"";
  }

  return r + ">" + quote(value) + "</" + quote(tag) + ">\n";
}

