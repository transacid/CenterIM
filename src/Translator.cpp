/*
 * Translator class
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

#include "Translator.h"

namespace ICQ2000
{
  // ======================================================================
  //  Translator
  // ======================================================================

  Translator::Translator()
  { }

  Translator::~Translator()
  { }
  
  void Translator::client_to_server_inplace(std::string& str,
					    Encoding en,
					    const ICQ2000::ContactRef& c)
  {
    std::string tstr = client_to_server(str, en, c);
    str = tstr;
  }
  
  void Translator::server_to_client_inplace(std::string& str,
					    Encoding en,
					    const ICQ2000::ContactRef& c)
  {
    std::string tstr = server_to_client(str, en, c);
    str = tstr;
  }

  // ======================================================================
  //  NULLTranslator
  // ======================================================================

  NULLTranslator::NULLTranslator()
  { }

  std::string NULLTranslator::client_to_server(const std::string& str,
					       Encoding en,
					       const ICQ2000::ContactRef& c)
  {
    return str;
  }
  
  std::string NULLTranslator::server_to_client(const std::string& str,
					       Encoding en,
					       const ICQ2000::ContactRef& c)
  {
    return str;
  }

  // ======================================================================
  //  CRLFTranslator
  // ======================================================================

  CRLFTranslator::CRLFTranslator()
  { }

  std::string CRLFTranslator::client_to_server(const std::string& str,
					       Encoding en,
					       const ICQ2000::ContactRef& c)
  {
    std::string::const_iterator curr = str.begin();
    std::string ret;

    while ( curr != str.end() )
    {
      if (*curr == '\n')
      {
	ret += "\r\n";
      }
      else
      {
	ret += *curr;
      }
      
      ++curr;
    }
    
    return ret;
  }
  
  std::string CRLFTranslator::server_to_client(const std::string& str,
					       Encoding en,
					       const ICQ2000::ContactRef& c)
  {
    std::string::const_iterator curr = str.begin();
    std::string::const_iterator ncurr = curr;
    std::string ret;

    while ( curr != str.end() )
    {
      ++ncurr;
      
      if (*curr == '\r' && ncurr != str.end()
	  && *ncurr == '\n')
      {
	ret += '\n';
	++curr;
	++ncurr;
      }
      else
      {
	ret += *curr;
      }
      
      ++curr;
    }

    return ret;
  }
}
