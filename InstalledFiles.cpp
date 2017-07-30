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

#define _CRT_SECURE_NO_WARNINGS 
#include "InstalledFiles.hpp"

#include "Error.hpp"
#include "Paths.hpp"

static const wchar_t logHeader[] = L"; SevenInstall";

InstalledFilesWriter::InstalledFilesWriter (const wchar_t* guid) : file (nullptr)
{
  logFileName = InstallLogFile (guid);
  file = _wfopen (logFileName.c_str(), L"w, ccs=UTF-8");
  if (!file)
  {
    THROW_HR(HRESULT_FROM_WIN32(GetLastError()));
  }
  else
  {
    fwprintf (file, L"%ls\n", logHeader);
  }
}

InstalledFilesWriter::~InstalledFilesWriter ()
{
  if (file) fclose (file);
}

void InstalledFilesWriter::AddEntries (const std::vector<std::wstring>& fullPaths)
{
  if (file)
  {
    for (const std::wstring& fullPath : fullPaths)
      fwprintf (file, L"%ls\n", fullPath.c_str());
  }
}

void InstalledFilesWriter::Discard ()
{
  if (file)
  {
    fclose (file);
    file = nullptr;
  }
  struct _stat64 stat_buf;
  if (_wstat64 (logFileName.c_str(), &stat_buf) == 0)
    _wunlink (logFileName.c_str());
}

//---------------------------------------------------------------------------

InstalledFilesReader::InstalledFilesReader (const wchar_t* filename) : file (nullptr)
{
  file = _wfopen (filename, L"r, ccs=UTF-8");
  if (!file)
  {
    THROW_HR(HRESULT_FROM_WIN32(GetLastError()));
  }
}

InstalledFilesReader::~InstalledFilesReader ()
{
  if (file) fclose (file);
}

std::wstring InstalledFilesReader::GetLine()
{
  if (!file)
    THROW_HR(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
  std::wstring line;
  wchar_t buf[256];

  bool eol (false);
  while (!eol)
  {
    if (!fgetws (buf, sizeof(buf)/sizeof(buf[0]), file))
    {
      if (feof (file)) return line;
      THROW_HR(HRESULT_FROM_WIN32(GetLastError()));
    }
    size_t n (wcslen (buf));
    if (n == 0) break;
    if (buf[n-1] == '\n')
    {
      buf[--n] = 0;
      eol = true;
    }
    line.append (buf);
  }
  return line;
}

std::wstring InstalledFilesReader::GetFileName()
{
  if (!file) return std::wstring();

  while (!feof (file))
  {
    std::wstring line (GetLine());
    if (line.empty() || (line[0] == ';')) continue;
    return line;
  }
  return std::wstring();
}
