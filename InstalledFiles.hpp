/**\file
 * Classes to manage the installed files list file
 */

#ifndef __7I_INSTALLEDFILES_HPP__
#define __7I_INSTALLEDFILES_HPP__

#include <stdio.h>
#include <string>
#include <vector>

#include <Windows.h>

/// Write an installed files list.
class InstalledFilesWriter
{
  std::wstring logFileName;
  FILE* file;
public:
  InstalledFilesWriter (const wchar_t* guid);
  ~InstalledFilesWriter ();

  const wchar_t* GetLogFileName() const { return logFileName.c_str(); }

  void AddEntries (const std::vector<std::wstring>& fullPaths);
  /// Remove the list that has been written
  void Discard ();
};

/// Read an installed files list.
class InstalledFilesReader
{
  FILE* file;

  std::wstring GetLine();
public:
  InstalledFilesReader (const wchar_t* path);
  ~InstalledFilesReader ();

  std::wstring GetFileName();
};

#endif // __7I_INSTALLEDFILES_HPP__
