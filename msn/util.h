#ifndef __msn_util_h__
#define __msn_util_h__

/*
 * util.h
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

#include <string>
#include <map>
#include <vector>
#include <stdexcept>

namespace MSN 
{
    /** URL-encode a string
     *
     * @param  s  The string to encode.
     * @return    A string with all non-alphanumeric characters replaced by their
     *            URL-encoded equivalent.
     */
    std::string encodeURL(const std::string & s);
    
    /** URL-decode a string
     *
     * @param  s  The URL-encoded string to decode.
     * @return    A string with all URL-encoded sequences replaced by their
     *            @c ASCII equivalent.
     */
    std::string decodeURL(const std::string & s);
    
    /** Split a string containing a hostname and port number into its respective parts.
     *
     * @param  address       A string in the form "hostname:port".
     * @param  default_port  A port number to return in the event that ":port" is omitted from @a address.
     * @return               A pair containing the hostname and port number.
     */
    std::pair<std::string, int> splitServerAddress(const std::string & address, int default_port=1863);
    
    /** Compare two strings in a case insensitive fashion
     */
    int nocase_cmp(const std::string & s1, const std::string & s2);
    
    /** Split @a string at each occurence of @a separator.
     */
    std::vector<std::string> splitString(const std::string & string, const std::string & separator, bool suppressBlanks=true);
    
    /** Convert a string, @a s, that contains decimal digits into an unsigned int.
     */
    unsigned int decimalFromString(const std::string & s) throw (std::logic_error);
}
#endif
