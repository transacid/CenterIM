// -*- mode: C++ -*-
/*
 * ref_ptr. A reference counted template class. Similar to the STL
 * auto_ptr, but generalised for reference counting.
 *
 * Copyright (C) 2002 Barnaby Gray <barnaby@beedesign.co.uk>
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

#ifndef REF_PTR_H
#define REF_PTR_H

namespace ICQ2000 {

  template <typename Object>
  class ref_ptr {
   protected:
    Object *m_instance;
    /* note: the actual reference count is controlled on the object -
       it turned out problematic having it stored in the ref_ptr,
       for reasons I won't go into here ;-) */

   public:
    ref_ptr()
      : m_instance(NULL)
    { }

    ref_ptr(const ref_ptr<Object>& that)
      : m_instance(that.m_instance)
    {
      if (m_instance != NULL)
	++(m_instance->count);
    }

    ref_ptr(Object *o)
      : m_instance(o)
    {
      if (m_instance != NULL)
	++(m_instance->count);
      /* zeroed inside Contact now */
    }
    
    ~ref_ptr()
    {
      if (m_instance != NULL && --(m_instance->count) == 0)
	delete m_instance;
    }
    
    Object* get()
    {
      return m_instance;
    }
    
    const Object* get() const
    {
      return m_instance;
    }
    
    Object* operator->()
    {
      return m_instance;
    }

    const Object* operator->() const
    {
      return m_instance;
    }

    Object& operator*()
    {
      return *m_instance;
    }

    const Object& operator*() const
    {
      return *m_instance;
    }

    // for debugging
    unsigned int count() const
    {
      return m_instance->count;
    }
      
    ref_ptr& operator=(const ref_ptr<Object>& that) {
      if (m_instance != NULL && --(m_instance->count) == 0) {
	delete m_instance;
      }
      m_instance = that.m_instance;
      if (m_instance != NULL)
	++(m_instance->count);
      return *this;		
    }

  };
  
}

#endif
