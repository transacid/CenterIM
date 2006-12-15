/*
 * Buffer class
 *
 * Copyright (C) 2001 Barnaby Gray <barnaby@beedesign.co.uk>
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

#include "buffer.h"

#include <algorithm>
#include <ctype.h>

using std::string;
using std::endl;
using std::ostream;

namespace ICQ2000
{

  Buffer::Buffer()
    : m_data(), m_endn(BIG), m_out_pos(0)
  { }

  Buffer::Buffer(const unsigned char* d, unsigned int size)
    : m_data(d, d+size), m_endn(BIG), m_out_pos(0)
  { }

  Buffer::Buffer(Buffer& b, unsigned int start, unsigned int data_len) 
    : m_data(b.m_data.begin()+start, b.m_data.begin()+start+data_len), 
      m_endn(BIG), m_out_pos(0)
  { }

  unsigned char& Buffer::operator[](unsigned int p)
  {
    return m_data[p];
  }

  void Buffer::clear()
  {
    m_data.clear();
    m_out_pos = 0;
  }

  bool Buffer::empty()
  {
    return m_data.empty();
  }

  void Buffer::chopOffBuffer(Buffer& b, unsigned int sz)
  {
    copy( m_data.begin(), m_data.begin()+sz, back_inserter(b.m_data) );
    m_data.erase( m_data.begin(), m_data.begin()+sz );
    m_out_pos = 0;
  }

  void Buffer::Pack(const unsigned char *d, unsigned int size)
  {
    copy(d, d+size, back_inserter(m_data));
  }

  void Buffer::PackUint16StringNull(const string& s)
  {
    (*this) << (unsigned short)(s.size()+1);
    Pack(s);
    (*this) << (unsigned char)0x00;
  }

  void Buffer::PackByteString(const string& s)
  {
    (*this) << (unsigned char)(s.size());
    Pack(s);
  }

  void Buffer::UnpackCRLFString(string& s)
  {
    iterator i;

    i = find(m_data.begin()+m_out_pos, m_data.end(), '\n');

    if (i != m_data.end() )
    {
      Unpack(s, i-m_data.begin()-m_out_pos+1);
    }
  }

  void Buffer::Pack(const string& s)
  {
    copy(s.begin(), s.end(), back_inserter(m_data));
  }

  unsigned char Buffer::UnpackChar()
  {
    if (m_out_pos + 1 > m_data.size())
      return 0;
    else
      return m_data[m_out_pos++];
  }

  void Buffer::UnpackUint32String(string& s)
  {
    unsigned int l;
    (*this) >> l;
    Unpack(s, l);
  }

  void Buffer::UnpackUint16StringNull(string& s)
  {
    unsigned short sh;
    (*this) >> sh;
    if (sh > 0)
    {
      Unpack(s, sh-1);
      (*this).advance(1);
    }
  }

  void Buffer::UnpackByteString(string& s)
  {
    unsigned char c;
    (*this) >> c;
    Unpack(s, c);
  }

  void Buffer::Unpack(string& s, unsigned int size)
  {
    if (m_out_pos >= m_data.size())
      return;

    if (m_out_pos+size > m_data.size())
      size = m_data.size()-m_out_pos;

    iterator i = m_data.begin()+m_out_pos;
    iterator end = m_data.begin()+m_out_pos+size;

    while (i != end)
    {
      s += *i;
      ++i;
    }

    m_out_pos += size;
  }

  void Buffer::Unpack(unsigned char *const d, unsigned int size) {
    if (m_out_pos+size > m_data.size()) size = m_data.size()-m_out_pos;
    copy(m_data.begin()+m_out_pos, m_data.begin()+m_out_pos+size, d);
    m_out_pos += size;
  }

  Buffer::marker Buffer::getAutoSizeShortMarker() 
  {
    // reserve a short
    (*this) << (unsigned short)0;

    marker m;
    m.position = size();
    m.endianness = m_endn;
    m.size = 2;
    return m;
  }

  Buffer::marker Buffer::getAutoSizeIntMarker() 
  {
    // reserve an int
    (*this) << (unsigned int)0;

    marker m;
    m.position = size();
    m.endianness = m_endn;
    m.size = 4;
    return m;
  }

  void Buffer::setAutoSizeMarker(const marker& m)
  {
    unsigned int autosize = size() - m.position;

    if (m.size == 2)
    {
      if (m.endianness == BIG)
      {
	m_data[ m.position - 2 ] = ((autosize >> 8) & 0xff);
	m_data[ m.position - 1 ] = ((autosize >> 0) & 0xff);
      }
      else
      {
	m_data[ m.position - 2 ] = ((autosize >> 0) & 0xff);
	m_data[ m.position - 1 ] = ((autosize >> 8) & 0xff);
      }
    }
    else if (m.size == 4)
    {
      if (m.endianness == BIG)
      {
	m_data[ m.position - 4 ] = ((autosize >> 24) & 0xff);
	m_data[ m.position - 3 ] = ((autosize >> 16) & 0xff);
	m_data[ m.position - 2 ] = ((autosize >> 8) & 0xff);
	m_data[ m.position - 1 ] = ((autosize >> 0) & 0xff);
      }
      else
      {
	m_data[ m.position - 4 ] = ((autosize >> 0) & 0xff);
	m_data[ m.position - 3 ] = ((autosize >> 8) & 0xff);
	m_data[ m.position - 2 ] = ((autosize >> 16) & 0xff);
	m_data[ m.position - 1 ] = ((autosize >> 24) & 0xff);
      }
    }
  }

  // -- Input stream methods --

  Buffer& Buffer::operator<<(unsigned char l)
  {
    m_data.push_back(l);
    return (*this);
  }

  Buffer& Buffer::operator<<(unsigned short l)
  {
    if (m_endn == BIG)
    {
      m_data.push_back((l>>8) & 0xFF);
      m_data.push_back(l & 0xFF);
    }
    else
    {
      m_data.push_back(l & 0xFF);
      m_data.push_back((l>>8) & 0xFF);
    }    
    return (*this);
  }

  Buffer& Buffer::operator<<(unsigned int l)
  {
    if (m_endn == BIG)
    {
      m_data.push_back((l >> 24) & 0xFF);
      m_data.push_back((l >> 16) & 0xFF);
      m_data.push_back((l >> 8) & 0xFF);
      m_data.push_back(l & 0xFF);
    }
    else
    {
      m_data.push_back(l & 0xFF);
      m_data.push_back((l >> 8) & 0xFF);
      m_data.push_back((l >> 16) & 0xFF);
      m_data.push_back((l >> 24) & 0xFF);
    }
    return (*this);
  }

  // strings stored as length (2 bytes), string m_data, _not_ null-terminated
  Buffer& Buffer::operator<<(const string& s)
  {
    (*this) << (unsigned short)s.size();
    Pack(s);
    return (*this);
  }

  // -- Output stream methods --

  Buffer& Buffer::operator>>(unsigned char& l)
  {
    if (m_out_pos + 1 > m_data.size()) l = 0;
    else l = m_data[m_out_pos++];
    return (*this);
  }

  Buffer& Buffer::operator>>(unsigned short& l)
  {
    if (m_out_pos + 2 > m_data.size())
    {
      l = 0;
      m_out_pos += 2;
    } else {
      if (m_endn == BIG) {
	l = ((unsigned short)m_data[m_out_pos++] << 8)
	+ ((unsigned short)m_data[m_out_pos++]);
      } else {
	l = ((unsigned short)m_data[m_out_pos++])
	+ ((unsigned short)m_data[m_out_pos++] << 8);
      }
    }
    return (*this);
  }

  Buffer& Buffer::operator>>(unsigned int& l)
  {
    if (m_out_pos + 4 > m_data.size())
    {
      l = 0;
      m_out_pos += 4;
    }
    else
    {
      if (m_endn == BIG)
      {
	l = ((unsigned int)m_data[m_out_pos++] << 24)
	+ ((unsigned int)m_data[m_out_pos++] << 16)
	+ ((unsigned int)m_data[m_out_pos++] << 8)
	+ ((unsigned int)m_data[m_out_pos++]);
      }
      else
      {
	l = ((unsigned int)m_data[m_out_pos++])
	+ ((unsigned int)m_data[m_out_pos++] << 8)
	+ ((unsigned int)m_data[m_out_pos++] << 16)
	+ ((unsigned int)m_data[m_out_pos++] << 24);
      }
    }
    return (*this);
  }

  // strings stored as length (2 bytes), string data, _not_ null-terminated
  Buffer& Buffer::operator>>(string& s)
  {
    if (m_out_pos + 2 > m_data.size())
    {
      s = ""; // clear() method doesn't seem to exist!
      m_out_pos += 2;
    }
    else
    {
      unsigned short sz;
      (*this) >> sz;
      Unpack(s, sz);
    }
    return (*this);
  }

  void Buffer::setEndianness(endian e)
  {
    m_endn = e;
  }

  void Buffer::setBigEndian() 
  {
    m_endn = BIG;
  }

  void Buffer::setLittleEndian()
  {
    m_endn = LITTLE;
  }

  void Buffer::dump(ostream& out)
  {
    char d[] = "123456789abcdef0";
    out << std::hex << std::setfill('0');
    unsigned int m = ((m_data.size()+15)/16)*16;

    for (unsigned int a = 0; a < m; a++)
    {
      if (a % 16 == 0) out << std::setw(4) << a << "  ";

      if (a < m_data.size())
      {
	out << std::setw(2) << (int)m_data[a] << " ";
	d[a%16] = isprint(m_data[a]) ? (m_data[a] < 0x80 ? m_data[a] : '.') : '.';
	// restrict output to a definitely non UTF-8 character set
      }
      else
      {
	out << "   ";
	d[a%16] = ' ';
      }

      if (a % 16 == 15)
	out << " " << d << endl;
    }
  }

  ostream& operator<<(ostream& out, Buffer& b)
  {
    b.dump(out);
    return out;
  }

}
