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
#include <cerrno>
#include <cctype>

namespace MSN 
{    
    std::pair<std::string, int> splitServerAddress(const std::string & address, int default_port)
    {
        size_t pos;
        std::string host = address;
        int port = default_port;
        
        if ((pos = address.find(":")) != std::string::npos)
        {
            std::string port_s = address.substr(pos + 1);
            host = address.substr(0, pos);
            port = decimalFromString(port_s);
        }

        if (host == "" || port < 0)
            throw std::runtime_error("Invalid zero-length address or negative port number!");

        return std::make_pair(host, port);
    }

    std::string decodeURL(const std::string & s)
    {
        std::string out;
        std::string::const_iterator i;
        
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
    
    std::string encodeURL(const std::string & s)
    {
        std::string out;
        std::string::const_iterator i;
        
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
    
    std::vector<std::string> splitString(const std::string & s, const std::string & sep, bool suppressBlanks)
    {
        std::vector<std::string> array;     
        size_t position, last_position;
        
        last_position = position = 0;
        while (position + sep.size() <= s.size())
        {
            if (s[position] == sep[0] && s.substr(position, sep.size()) == sep)
            {
                if (!suppressBlanks || position - last_position > 0)
                    array.push_back(s.substr(last_position, position - last_position));
                last_position = position = position + sep.size();
            }
            else
                position++;
        }
        if (!suppressBlanks || last_position - s.size())
            array.push_back(s.substr(last_position));
        
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
        return (int) (size1 - size2);
    }
    
    unsigned int decimalFromString(const std::string & s) throw (std::logic_error)
    {
        unsigned int result = strtol(s.c_str(), NULL, 10);
        errno = 0;
        if (result == 0 && errno != 0)
            throw std::logic_error(strerror(errno));
        return result;
    }
}
