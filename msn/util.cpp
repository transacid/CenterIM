/*
 * util.cpp
 * libmsn
 *
 * Created by Mark Rowe on Mon Mar 22 2004.
 * Copyright (c) 2004 Mark Rowe. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <msn/util.h>
#include <cctype>
#include <cerrno>

namespace MSN 
{    
    std::pair<std::string, int> splitServerAddress(std::string & address, int default_port)
    {
	unsigned int pos;
	int port = default_port;
	
	if ((pos = address.find(":")) != std::string::npos)
	{
	    std::string port_s = address.substr(pos + 1);
	    address = address.substr(0, pos);
	    port = decimalFromString(port_s);
	}
	return std::make_pair(address, port);
    }

    std::string decodeURL(std::string s)
    {
	std::string out;
	std::string::iterator i;
	
	for (i = s.begin(); i != s.end(); i++)
	{
	    if (*i == '%')
	    {
		char entity[3] = {*(++i), *(++i), 0};
		int c = strtol(entity, NULL, 16);
		out += c;
	    }
	    else
		out += *i;
	}
	return out;
    }
    
    std::string encodeURL(std::string s)
    {
	std::string out;
	std::string::iterator i;
	
	for (i = s.begin(); i != s.end(); i++)
	{
	    if(!(isalpha(*i) || isdigit(*i)))
	    {
		unsigned char high_nibble = ((unsigned char) *i) >> 4;
		unsigned char low_nibble = ((unsigned char) *i) & 0x0F;
		out += '%';
		out += (high_nibble < 0x0A ? '0' + high_nibble : 'a' + high_nibble - 0x0A);
		out += (low_nibble < 0x0A ? '0' + low_nibble : 'a' + low_nibble - 0x0A);
		
		continue;
	    }
	    out += *i;
	}

	return out;
    }
    
    std::vector<std::string> splitString(std::string s, std::string sep)
    {
	std::vector<std::string> array;     
	size_t position;
	
	position = s.find_first_of(sep);        
	while (position != s.npos)
	{
	    if (position)
		array.push_back(s.substr(0, position));

	    s.erase(0, position + sep.length());
	    position = s.find_first_of(sep);
	}
	
	if (! s.empty())
	    array.push_back(s);
	
	return array;
    }
    
    int nocase_cmp(const std::string & s1, const std::string& s2) 
    {
	std::string::const_iterator it1, it2;
	
	for (it1 = s1.begin(), it2 = s2.begin();
	     it1 != s1.end() && it2 != s2.end();
	     ++it1, ++it2)
	{ 
	    if (std::toupper(*it1) != std::toupper(*it2)) 
		return std::toupper(*it1) - std::toupper(*it2);
	}
	size_t size1 = s1.size(), size2 = s2.size(); 
	return size1 - size2;
    }
    
    unsigned int decimalFromString(std::string s) throw (std::logic_error)
    {
	unsigned int result = strtol(s.c_str(), NULL, 10);
	errno = 0;
	if (result == 0 && errno != 0)
	    throw std::logic_error(strerror(errno));
	return result;
    }
}
