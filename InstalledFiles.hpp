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

#include <Windows.h>

/// Write an installed files list.
class InstalledFilesWriter
{
  MyUString logFileName;
  HANDLE file;

  void PrintFile (const MyUString& filename);
public:
  InstalledFilesWriter (const wchar_t* filename);
  ~InstalledFilesWriter ();

  const wchar_t* GetLogFileName() const { return logFileName; }

  template<typename Container>
  void AddEntries (const Container& fullPaths)
  {
    if (file != INVALID_HANDLE_VALUE)
    {
      for (const MyUString& fullPath : fullPaths)
        PrintFile (fullPath);
    }
  }
  /// Remove the list that has been written
  void Discard ();
};

/// Read an installed files list.
class InstalledFilesReader
{
  HANDLE file;
  uint64_t fileSize = 0;
  bool eof = false;
  char buf[4 * 1024];
  char* buf_p = nullptr;
  char* buf_end = nullptr;

  MyUString GetLine();
public:
  InstalledFilesReader (const wchar_t* path);
  ~InstalledFilesReader ();

  uint64_t GetFileSize () const { return fileSize; }
  uint64_t GetProcessed () const;

  MyUString GetFileName();
};

class InstalledFilesCounter
{
public:
  InstalledFilesCounter (const wchar_t* listsDir);

  unsigned int IncFileRef (const MyUString& path);
  unsigned int DecFileRef (const MyUString& path);
private:
  std::unordered_map<MyUString, unsigned int> files_refs;

  void ReadLogFile (const wchar_t* path);
};

#endif // __7I_INSTALLEDFILES_HPP__
