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
 * Classes to manage the installed files list file
 */

#ifndef __7I_INSTALLEDFILES_HPP__
#define __7I_INSTALLEDFILES_HPP__

#include "MyUString.hpp"

#include <stdio.h>

#include <set>
#include <vector>

#include <Windows.h>

/// Write an installed files list.
class InstalledFilesWriter
{
  MyUString logFileName;
  HANDLE file;
public:
  InstalledFilesWriter (const wchar_t* filename);
  ~InstalledFilesWriter ();

  const wchar_t* GetLogFileName() const { return logFileName; }

  void AddEntries (const std::set<MyUString>& fullPaths);
  void AddEntries (const std::vector<MyUString>& fullPaths);
  /// Remove the list that has been written
  void Discard ();
};

/// Read an installed files list.
class InstalledFilesReader
{
  HANDLE file;
  bool eof = false;
  char buf[256];
  char* buf_p = nullptr;
  char* buf_end = nullptr;

  MyUString GetLine();
public:
  InstalledFilesReader (const wchar_t* path);
  ~InstalledFilesReader ();

  MyUString GetFileName();
};

#endif // __7I_INSTALLEDFILES_HPP__
