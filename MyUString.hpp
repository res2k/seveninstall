/*
    SevenInstall
    Copyright (c) 2013-2017 Frank Richter

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org>
 */

/**\file
 * String type compatible to 7-zip's UString, but with some additional functionality
 */
#ifndef __7I_MYUSTRING_HPP__
#define __7I_MYUSTRING_HPP__

#include "Common/MyString.h"

#include <algorithm>
#include <functional>

/**\file
 * String type (mostly) compatible to 7-zip's UString, but with some additional functionality
 */
 class MyUString
{
  uint8_t storage[sizeof(UString)] alignas(UString);

  UString& us() { return *(reinterpret_cast<UString*> (&storage)); }
  const UString& us() const { return *(reinterpret_cast<const UString*> (&storage)); }

  void Reset()
  {
    us().~UString();
    memset (storage, 0, sizeof (storage));
  }
public:
  MyUString () { memset (storage, 0, sizeof (storage)); }
  MyUString (const MyUString& other) { new (storage) UString (other.us()); }
  MyUString (MyUString&& other) : MyUString () { std::swap_ranges (&storage[0], &storage[0] + sizeof (storage), &other.storage[0]); }
  MyUString (const UString& other) { new (storage) UString (other); }
  MyUString (UString&& other) : MyUString () { std::swap_ranges (&storage[0], &storage[0] + sizeof (storage), reinterpret_cast<uint8_t*> (&other)); }
  MyUString (const wchar_t* s) { new (storage) UString (s); }
  MyUString (const wchar_t* s, size_t n) : MyUString() { us().SetFrom (s, n); }
  ~MyUString () { us().~UString(); }

  unsigned Len() const { return us().Len(); }
  bool IsEmpty() const { return us().IsEmpty(); }

  operator const wchar_t* () const { return us().Ptr(); }
  operator const UString& () const;
  operator UString& ();
  wchar_t* Ptr() { return const_cast<wchar_t*> (us().Ptr()); }
  const wchar_t* Ptr() const { return us().Ptr(); }

  wchar_t* GetBuf (unsigned minLen) { return us().GetBuf (minLen); }
  wchar_t* GetBuf_SetEnd (unsigned minLen) { return us().GetBuf_SetEnd (minLen); }
  void ReleaseBuf_SetLen (unsigned newLen) { us().ReleaseBuf_SetLen (newLen); }
  void ReleaseBuf_SetEnd (unsigned newLen) { us().ReleaseBuf_SetEnd (newLen); }
  void ReleaseBuf_CalcLen (unsigned maxLen) { us().ReleaseBuf_CalcLen (maxLen); }

  MyUString& operator= (const wchar_t* other) { us() = other; return *this; }
  MyUString& operator= (const MyUString& other) { us() = other.us(); return *this; }
  MyUString& operator= (MyUString&& other)
  {
    Reset ();
    std::swap_ranges (&storage[0], &storage[0] + sizeof (storage), &other.storage[0]);
    return *this;
  }
  MyUString& operator= (const UString& other) { us() = other; return *this; }
  MyUString& operator= (UString&& other)
  {
    Reset ();
    std::swap_ranges (&storage[0], &storage[0] + sizeof (storage), reinterpret_cast<uint8_t*> (&other));
    return *this;
  }

  bool operator== (const MyUString& other) const { return us() == other.us(); }
  bool operator== (const UString& other) const { return us() == other; }
  bool operator!= (const MyUString& other) const { return us() != other.us(); }
  bool operator!= (const UString& other) const { return us() != other; }

  MyUString& operator+= (const wchar_t* s) { us() += s; return *this; }
  MyUString& operator+= (const MyUString& s) { us() += s.us(); return *this; }
  MyUString& operator+= (const UString& s) { us() += s; return *this; }
};

namespace std
{
  template<>
  struct hash<MyUString>
  {
    size_t operator()(const MyUString& s) const
    {
      hash<wstring_view> view_hash;
      return view_hash (wstring_view (s.Ptr(), s.Len()));
    }
  };
} // namespace std

#endif // __7I_MYUSTRING_HPP__
