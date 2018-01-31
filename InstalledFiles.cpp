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

InstalledFilesWriter::InstalledFilesWriter (const wchar_t* filename)
  : file (INVALID_HANDLE_VALUE), logFileName (filename)
{
  file = CreateFileW (logFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE)
  {
    THROW_HR(HRESULT_FROM_WIN32(GetLastError()));
  }
  else
  {
    Hprintf (file, "%ls\n", logHeader);
  }
}

InstalledFilesWriter::~InstalledFilesWriter ()
{
  if (file != INVALID_HANDLE_VALUE) CloseHandle (file);
}

void InstalledFilesWriter::Discard ()
{
  if (file != INVALID_HANDLE_VALUE)
  {
    CloseHandle (file);
    file = INVALID_HANDLE_VALUE;
  }
  DeleteFileW (logFileName.Ptr());
}

void InstalledFilesWriter::PrintFile (const MyUString& filename)
{
  Hprintf (file, "%ls\n", filename.Ptr());
}

//---------------------------------------------------------------------------

InstalledFilesReader::InstalledFilesReader (const wchar_t* filename) : file (INVALID_HANDLE_VALUE)
{
  file = CreateFileW (filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE)
  {
    THROW_HR(HRESULT_FROM_WIN32(GetLastError()));
  }
  GetFileSizeEx (file, reinterpret_cast<LARGE_INTEGER*> (&fileSize));
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

uint64_t InstalledFilesReader::GetProcessed () const
{
  uint64_t currentPos = 0;
  LARGE_INTEGER li_null = { 0, 0 };
  if (!SetFilePointerEx (file, li_null, reinterpret_cast<LARGE_INTEGER*> (&currentPos), FILE_CURRENT))
    return 0;
  return currentPos;
}

MyUString InstalledFilesReader::GetLine()
{
  if (file == INVALID_HANDLE_VALUE)
    THROW_HR(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

  AString utf8_line;

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
      utf8_line += buf_p;
      buf_p = newline_pos + 1;
      eol = true;
    }
    else
    {
      // @@@ Ugh, AString doesn't have append w/ pointer + size
      char last = 0;
      std::swap (last, *(buf_end - 1);
      utf8_line += buf_p;
      utf8_line += last;
      buf_p = buf_end;
    }
  }

  // Remove trailing CR
  utf8_line.TrimRight();

  MyUString line;
  // Convert line from UTF-8 to wide char
  int buf_req = MultiByteToWideChar (CP_UTF8, 0, utf8_line.Ptr(), utf8_line.Len(), nullptr, 0);
  if (buf_req == 0) return MyUString();
  auto line_ptr = line.GetBuf (buf_req);
  MultiByteToWideChar (CP_UTF8, 0, utf8_line.Ptr(), utf8_line.Len(),
                       line_ptr, buf_req);
  line.ReleaseBuf_SetEnd (buf_req);

  return line;
}

MyUString InstalledFilesReader::GetFileName()
{
  if (file == INVALID_HANDLE_VALUE) return MyUString();

  while (!eof)
  {
    MyUString line (GetLine());
    if (line.IsEmpty() || (line[0] == ';')) continue;
    return line;
  }
  return MyUString();
}

//---------------------------------------------------------------------------

InstalledFilesCounter::InstalledFilesCounter (const wchar_t* listsDir)
{
  MyUString wildcard = listsDir;
  wildcard += L"\\*.txt";
  WIN32_FIND_DATAW find_data;
  HANDLE find_handle = FindFirstFileW (wildcard.Ptr(), &find_data);
  if (find_handle != INVALID_HANDLE_VALUE)
  {
    do
    {
      MyUString file_path = listsDir;
      file_path += L"\\";
      file_path += find_data.cFileName;
      ReadLogFile (file_path.Ptr());
    }
    while (FindNextFile (find_handle, &find_data));

    FindClose (find_handle);
  }
}

unsigned int InstalledFilesCounter::IncFileRef (const MyUString& path)
{
  return ++(files_refs.try_emplace (path, 0).first->second);
}

unsigned int InstalledFilesCounter::DecFileRef (const MyUString& path)
{
  auto it = files_refs.find (path);
  if (it == files_refs.end ()) return 0;
  if (it->second == 0) return 0;
  return --(it->second);
}

void InstalledFilesCounter::ReadLogFile (const wchar_t* path)
{
  try
  {
    InstalledFilesReader reader (path);
    MyUString filename;
    while (!(filename = reader.GetFileName()).IsEmpty())
    {
      NormalizePath (filename);
      IncFileRef (filename);
    }
  }
  catch (HRESULTException& hre)
  {
    fprintf (stderr, "ERROR reading %ls: %ls\n", path, GetHRESULTString (hre.GetHR()).Ptr());
  }
  catch (std::exception& e)
  {
    fprintf (stderr, "ERROR reading %ls: %s\n", path, e.what());
  }
}
