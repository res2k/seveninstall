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

#include "support/printf.hpp"

static const wchar_t logHeader[] = L"; SevenInstall";

InstalledFilesWriter::InstalledFilesWriter (const wchar_t* guid) : file (INVALID_HANDLE_VALUE)
{
  logFileName = InstallLogFile (guid);
  file = CreateFileW (logFileName.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE)
  {
    THROW_HR(HRESULT_FROM_WIN32(GetLastError()));
  }
  else
  {
    Hwprintf (file, L"%ls\n", logHeader);
  }
}

InstalledFilesWriter::~InstalledFilesWriter ()
{
  if (file != INVALID_HANDLE_VALUE) CloseHandle (file);
}

void InstalledFilesWriter::AddEntries (const std::vector<std::wstring>& fullPaths)
{
  if (file != INVALID_HANDLE_VALUE)
  {
    for (const std::wstring& fullPath : fullPaths)
      Hwprintf (file, L"%ls\n", fullPath.c_str());
  }
}

void InstalledFilesWriter::Discard ()
{
  if (file != INVALID_HANDLE_VALUE)
  {
    CloseHandle (file);
    file = INVALID_HANDLE_VALUE;
  }
  DeleteFileW (logFileName.c_str());
}

//---------------------------------------------------------------------------

InstalledFilesReader::InstalledFilesReader (const wchar_t* filename) : file (INVALID_HANDLE_VALUE)
{
  file = CreateFileW (filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE)
  {
    THROW_HR(HRESULT_FROM_WIN32(GetLastError()));
  }
  // Skip BOM, if present
  char bom_buf[3];
  DWORD bytes_read = 0;
  if (!ReadFile (file, bom_buf, sizeof (bom_buf), &bytes_read, nullptr)
      || (bytes_read != 3)
      || (bom_buf[0] != 0xef)
      || (bom_buf[1] != 0xbb)
      || (bom_buf[2] != 0xbf))
  {
    SetFilePointer (file, 0, nullptr, FILE_BEGIN);
  }
}

InstalledFilesReader::~InstalledFilesReader ()
{
  if (file != INVALID_HANDLE_VALUE) CloseHandle (file);
}

std::wstring InstalledFilesReader::GetLine()
{
  if (file == INVALID_HANDLE_VALUE)
    THROW_HR(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

  std::string utf8_line;

  bool eol (false);
  while (!eol)
  {
    if ((buf_p >= buf_end) && !eof)
    {
      DWORD bytes_read = 0;
      if (ReadFile (file, buf, sizeof (buf), &bytes_read, nullptr))
      {
        buf_p = buf;
        buf_end = buf_p + bytes_read;
        if (bytes_read == 0)
        {
          // EOF.
          eof = true;
          buf_p = buf_end = nullptr;
        }
      }
    }
    if (eof) break;

    auto newline_pos = reinterpret_cast<char*> (memchr (buf_p, '\n', buf_end - buf_p));
    if (newline_pos)
    {
      *newline_pos = 0;
      utf8_line.append (buf_p);
      buf_p = newline_pos + 1;
      eol = true;
    }
    else
    {
      utf8_line.append (buf, buf_end - buf_p);
      buf_p = buf_end;
    }
  }

  // Remove trailing CR
  if ((utf8_line.size() > 0) && (utf8_line[utf8_line.size()-1] == '\r')) utf8_line.resize (utf8_line.size() - 1);

  std::wstring line;
  // Convert line from UTF-8 to wide char
  int buf_req = MultiByteToWideChar (CP_UTF8, 0, utf8_line.data(), utf8_line.size(), nullptr, 0);
  if (buf_req == 0) return std::wstring();
  line.resize (buf_req, 0);
  int ret = -1;
  MultiByteToWideChar (CP_UTF8, 0, utf8_line.data (), utf8_line.size (),
                       const_cast<wchar_t*> (line.data()), line.size());

  return line;
}

std::wstring InstalledFilesReader::GetFileName()
{
  if (file == INVALID_HANDLE_VALUE) return std::wstring();

  while (!eof)
  {
    std::wstring line (GetLine());
    if (line.empty() || (line[0] == ';')) continue;
    return line;
  }
  return std::wstring();
}
