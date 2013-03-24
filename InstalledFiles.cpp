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
