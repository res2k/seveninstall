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

#include "Error.hpp"

namespace
{
  template<typename T>
  struct LocalBuffer
  {
    T* p;

    LocalBuffer() : p (nullptr) {}
    ~LocalBuffer()
    {
      if (p) LocalFree (p);
    }
  };

  template<typename Ch>
  static void to_hex_impl (Ch* buf, long value)
  {
    *buf++ = '0';
    *buf++ = 'x';
    Ch* insert_pos = buf + 8;
    *insert_pos-- = 0;
    while (insert_pos >= buf)
    {
      auto val = value & 0xf;
      *insert_pos = static_cast<Ch> ((val >= 10) ? ('a' + val - 10) : ('0' + val));
      --insert_pos;
      value >>= 4;
    }
  }
}

template<typename Ch, size_t N>
static void to_hex (Ch (&buf)[N], long value)
{
  static_assert (N >= 11, "buffer too small"); // 0x + 8 hex digits + null terminator
  to_hex_impl (buf, value);
}

MyUString GetErrorString (DWORD error)
{
  wchar_t buf[256];
  DWORD n = FormatMessageW (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            nullptr, error, 0, buf, sizeof(buf)/sizeof(buf[0]), nullptr);
  if (n == 0)
  {
    DWORD err (GetLastError());
    if (err == ERROR_MORE_DATA)
    {
      LocalBuffer<wchar_t> newBuffer;
      n = FormatMessageW (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                          nullptr, error, 0, (LPWSTR)&(newBuffer.p), 0, nullptr);
      if (n != 0)
      {
        return newBuffer.p;
      }
    }
  }
  if (n == 0)
    to_hex (buf, error);
  return buf;
}

MyUString GetHRESULTString (HRESULT hr)
{
  if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
    return GetErrorString (HRESULT_CODE(hr));
  wchar_t buf[16];
  to_hex (buf, hr);
  return buf;
}
