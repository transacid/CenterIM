/*
 * Buffer class header
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

#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <iostream>
#include <iomanip>
#include <string>
#include <iterator>

namespace ICQ2000
{
  
  class Buffer
  {
  public:
    typedef unsigned int size_type;
  
    enum endian { BIG, LITTLE };

    struct marker {
      size_type position;
      endian endianness;
      int size;
    };

  private:
    typedef std::vector<unsigned char>::iterator iterator;

    std::vector<unsigned char> m_data;
    endian m_endn;
    size_type m_out_pos;

  public:
    Buffer();
    Buffer(const unsigned char *d, unsigned int size); // construct from an array
    Buffer(Buffer& b, unsigned int start, unsigned int data_len); // construct by copying from another Buffer

    unsigned int size() const { return m_data.size(); }
    unsigned int pos() const { return m_out_pos; }
    unsigned int remains() const { return m_data.size() - m_out_pos; }

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }

    void clear();
    bool empty();
    void advance(unsigned int ad) { m_out_pos += ad; }
    bool beforeEnd() const { return (m_out_pos < m_data.size()); }
    void setPos(unsigned int o) { m_out_pos = o; }
    void chopOffBuffer(Buffer& b, unsigned int sz);

    void setEndianness(endian e);
    void setBigEndian();
    void setLittleEndian();

    marker getAutoSizeShortMarker();
    marker getAutoSizeIntMarker();
    void setAutoSizeMarker(const marker& m);

    Buffer& operator<<(unsigned char);
    Buffer& operator<<(unsigned short);
    Buffer& operator<<(unsigned int);
    Buffer& operator<<(signed char l) { return (*this) << (unsigned char)l; }
    Buffer& operator<<(signed short l) { return (*this) << (unsigned short)l; }
    Buffer& operator<<(signed int l) { return (*this) << (unsigned int)l; }
    Buffer& operator<<(const std::string&);

    Buffer& operator>>(unsigned char&);
    Buffer& operator>>(unsigned short&);
    Buffer& operator>>(unsigned int&);
    Buffer& operator>>(signed char& l) { return (*this) >> (unsigned char&)l; }
    Buffer& operator>>(signed short& l) { return (*this) >> (unsigned short&)l; }
    Buffer& operator>>(signed int& l) { return (*this) >> (unsigned int&)l; }
    Buffer& operator>>(std::string&);

    void Pack(const unsigned char *d, unsigned int size);
    void Pack(const std::string& s);
    void PackUint16StringNull(const std::string& s);
    void PackByteString(const std::string& s);
    void UnpackCRLFString(std::string& s);

    void Unpack(std::string& s, unsigned int size);
    void Unpack(unsigned char *const d, unsigned int size);
    unsigned char UnpackChar();
    void UnpackUint32String(std::string& s);
    void UnpackUint16StringNull(std::string& s);
    void UnpackByteString(std::string& s);

    unsigned char& operator[](unsigned int p);

    void dump(std::ostream& out);
  };

  std::ostream& operator<<(std::ostream&,Buffer&);
  
}

#endif
