#ifndef __msn_passport_h__
#define __msn_passport_h__

/*
 * passport.h
 * libmsn
 *
 * Created by Mark Rowe on Thu May 20 2004.
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
#include <stdexcept>
#include <ostream>

namespace MSN
{
    class InvalidPassport : public std::runtime_error
    {
public:
        InvalidPassport(std::string err) : std::runtime_error(err) {};
    };
    
    class Passport
    {
public:
        Passport(std::string email_) : email(email_) { validate(); };
        Passport(const char *email_) : email(std::string(email_)) { validate(); };
        Passport() : email("") {};
        
        operator std::string() const;
        const char *c_str() const;
        bool operator ==(const Passport & other) const { return this->email == other.email; };
        
        friend bool operator ==(const Passport & p, const std::string & other) { return p.email == other; };
        friend bool operator ==(const std::string & other, const Passport & p) { return p.email == other; };
        friend std::istream& operator >>(std::istream & is, Passport & p) { is >> p.email; p.validate(); return is; }
private:
            ;
        void validate();
        std::string email;
    };
}

std::ostream & operator << (std::ostream & os, const MSN::Passport& passport);
#endif
