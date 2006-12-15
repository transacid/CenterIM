/*
 * XML Parser/Generator
 * Simple XML parser/generator sufficient to
 * send+receive the XML in ICQ SMS messages
 *
 * Copyright (C) 2001 Barnaby Gray <barnaby@beedesign.co.uk>.
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

#ifndef XML_H
#define XML_H

#include <string>
#include <list>
#include <ctype.h>

class XmlLeaf;

class XmlNode {
 private:
  static std::string parseTag(std::string::iterator& curr, std::string::iterator end);
  static void skipWS(std::string::iterator& curr, std::string::iterator end);

  void parseAttr(const std::string &t);

 protected:
  std::string tag;
  std::list<std::pair<std::string, std::string> > attributes;
  
  XmlNode(const std::string& t);

 public:
  virtual ~XmlNode();

  virtual bool isBranch() = 0;
  bool isLeaf();

  std::string getTag();

  std::string getAttrib(const std::string &aname) const;
  bool existsAttrib(const std::string& attrib) const;

  static XmlNode *parse(std::string::iterator& start, std::string::iterator end);

  static std::string quote(const std::string& s);
  static std::string unquote(const std::string& s);
  static std::string replace_all(const std::string& s, const std::string& r1, const std::string& r2);

  virtual std::string toString(int n) = 0;
};

class XmlLeaf : public XmlNode {
 private:
  std::string name, value;

 public:
  XmlLeaf(const std::string& t, const std::string& v);
  virtual ~XmlLeaf();

  bool isBranch();
  std::string getValue();

  virtual std::string toString(int n);

};

class XmlBranch : public XmlNode {
 private:
  std::list<XmlNode*> children;

 public:
  XmlBranch(const std::string& t);
  virtual ~XmlBranch();

  bool isBranch();
  bool exists(const std::string& tag);
  XmlNode *getNode(const std::string& tag, int n = 0);
  XmlBranch *getBranch(const std::string& tag, int n = 0);
  XmlLeaf *getLeaf(const std::string& tag);

  std::list<std::string> getChildren() const;

  void pushnode(XmlNode *c);

  std::string toString(int n);

};

#endif
