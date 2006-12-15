/*
 * Translate class
 *
 * Copyright (C) 2003 Barnaby Gray <barnaby@beedesign.co.uk>
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

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <string>

#include <libicq2000/Contact.h>

namespace ICQ2000
{
  /**
   *  The encoding type the string is expected in. For example - most
   *  messages are in the encoding of the locale on the contact's
   *  computer. However, server-based lists are all stored in
   *  UTF-8. Lastly, email/pager messages from the ICQ website are
   *  sent to you are in ISO-8859-1. It is up to the translator how to
   *  do these conversions to provide a consistent character set to
   *  the client.
   */
  enum Encoding
  {
    ENCODING_CONTACT_LOCALE,
    ENCODING_UTF8,
    ENCODING_ISO_8859_1
  };
  
  /**
   *  Abstract base class for the character set translation which
   *  clients can implement and plug into the library.
   */
  class Translator
  {
   public:
    Translator();
    virtual ~Translator();

    /**
     *  Translate the string from the client-side encoding to a
     *  server-side encoding.
     *
     * @param str the string to translate
     * @param en  the expected encoding on the server-side
     * @param c   the contact relevant to the string
     * @return the translated string
     */
    virtual std::string client_to_server(const std::string& str,
					 Encoding en,
					 const ICQ2000::ContactRef& c) = 0;

    /**
     *  Translate the string from the server-side encoding to a client-side encoding.
     *
     * @param str the string to translate
     * @param en  the expected encoding from the server-side
     * @param c   the contact relevant to the string
     * @return the translated string
     */
    virtual std::string server_to_client(const std::string& str,
					 Encoding en,
					 const ICQ2000::ContactRef& c) = 0;

    virtual void client_to_server_inplace(std::string& str,
					  Encoding en,
					  const ICQ2000::ContactRef& c);
    
    virtual void server_to_client_inplace(std::string& str,
					  Encoding en,
					  const ICQ2000::ContactRef& c);
  };

  /**
   *  Null translator - a noop (the default).
   */
  class NULLTranslator : public Translator
  {
   public:
    NULLTranslator();

    virtual std::string client_to_server(const std::string& str,
					 Encoding en,
					 const ICQ2000::ContactRef& c);
    
    virtual std::string server_to_client(const std::string& str,
					 Encoding en,
					 const ICQ2000::ContactRef& c);
  };
  
  /**
   *  Simple implementation of a translator that does the standard
   *  unix LF -> windows CRLF format.
   */
  class CRLFTranslator : public Translator
  {
   public:
    CRLFTranslator();
    
    virtual std::string client_to_server(const std::string& str,
					 Encoding en,
					 const ICQ2000::ContactRef& c);
    
    virtual std::string server_to_client(const std::string& str,
					 Encoding en,
					 const ICQ2000::ContactRef& c);
  };
}

#endif /* TRANSLATOR_H */
