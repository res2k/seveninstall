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

#include "Remove.hpp"

#include "ArgsHelper.hpp"
#include "CommonArgs.hpp"
#include "ExitCode.hpp"
#include "Error.hpp"
#include "InstalledFiles.hpp"
#include "Paths.hpp"
#include "Registry.hpp"
#include "RegistryLocations.hpp"

#include <algorithm>

#include "Shlwapi.h"

static bool StringSizeSort (const std::wstring& a, const std::wstring& b)
{
  return a.size() > b.size();
}

static void TryDelete (DWORD fileAttr, const wchar_t* file)
{
  if ((fileAttr & FILE_ATTRIBUTE_READONLY) != 0)
  {
    if (!SetFileAttributesW (file, fileAttr & ~FILE_ATTRIBUTE_READONLY))
    {
      DWORD result (GetLastError());
      printf ("Error clearing 'read-only' attribute of %ls: %ls\n", file,
              GetErrorString (result).c_str());
      return;
    }
  }
  if (!DeleteFileW (file))
  {
    DWORD result (GetLastError());
    printf ("Error deleting %ls: %ls\n", file,
            GetErrorString (result).c_str());
  }
}

static void TryDeleteDir (const wchar_t* path)
{
  if (!RemoveDirectoryW (path))
  {
    DWORD result (GetLastError());
    printf ("Error deleting %ls: %ls\n", path,
            GetErrorString (result).c_str());
  }
}

RemoveHelper::~RemoveHelper()
{
  FlushDelayed();
}

void RemoveHelper::ScheduleRemove (const wchar_t* path)
{
  DWORD fileAttr (GetFileAttributesW (path));
  if (fileAttr == INVALID_FILE_ATTRIBUTES)
  {
    // File does not exist (probably)...
    DWORD result (GetLastError());
    printf ("Error obtaining attributes for %ls: %ls\n", path,
            GetErrorString (result).c_str());
  }
  else if ((fileAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
  {
    // Handle directories later
    directories.push_back (path);
  }
  else
  {
    // Remove it
    TryDelete (fileAttr, path);
  }
}

void RemoveHelper::FlushDelayed ()
{
  // Sort directory by length descending (to get deeper nested dirs first)
  std::sort (directories.begin(), directories.end(), &StringSizeSort);
  // Remove directories
  for (const std::wstring& dir : directories)
  {
    TryDeleteDir (dir.c_str());
  }
  directories.clear();
}

static void RegistryDelete (const wchar_t* regPath)
{
  LRESULT err (SHDeleteKeyW (HKEY_LOCAL_MACHINE, regPath));
  if ((err == ERROR_ACCESS_DENIED) || (err == ERROR_FILE_NOT_FOUND))
  {
    err = SHDeleteKeyW (HKEY_CURRENT_USER, regPath);
  }
  if (err != ERROR_SUCCESS)
  {
    printf ("Error deleting %ls from registry: %ls\n", regPath, GetErrorString (err).c_str());
  }
}

static size_t HasDependents (const wchar_t* regPath)
{
  const REGSAM key_access (KEY_READ | KEY_WOW64_64KEY);
  try
  {
    AutoRootRegistryKey key (regPath, key_access);
    return key.NumSubkeys();
  }
  catch (const HRESULTException& e)
  {
    // Path not found means no dependencies
    if (e.GetHR() != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
      throw;
  }
  return 0;
}

int DoRemove (int argc, const wchar_t* const argv[])
{
  const wchar_t* guid (nullptr);

  ArgsHelper args (argc, argv);
  {
    CommonArgs commonArgs (args);
    if (!commonArgs.GetGUID (guid))
    {
      return ecArgsError;
    }
  }

  try
  {
    bool ignoreDependencies (args.GetOption (L"--ignore-dependents"));
    {
      std::wstring keyPathDependencies (regPathDependencyInfo);
      keyPathDependencies.append (guid);
      std::wstring keyPathDependents (keyPathDependencies);
      keyPathDependents.append (regPathDependentsSubkey);
      if (!ignoreDependencies)
      {
        size_t has_dep (0);
        try
        {
          has_dep = HasDependents (keyPathDependents.c_str());
        }
        catch (const HRESULTException& e)
        {
          printf ("Error checking for dependents: %ls\n", GetHRESULTString (e.GetHR()).c_str());
        }
        if (has_dep > 0)
        {
          printf ("Aborting due to %ld dependents being present.\n", (long)has_dep);
          return ecHasDependencies;
        }
      }
      RegistryDelete (keyPathDependencies.c_str());
    }

    // Obtain list file path from registry
    std::wstring listFilePath (ReadRegistryListFilePath (guid));
    {
      RemoveHelper removeHelper;

      // Open list file
      InstalledFilesReader listReader (listFilePath.c_str());
      // Remove all listed files
      {
        std::wstring installedFile;
        while (!(installedFile = listReader.GetFileName()).empty())
        {
          removeHelper.ScheduleRemove (installedFile.c_str());
        }
      }

      // Handle request for output dir removal
      if (args.GetOption (L"-r"))
      {
        // Grab previously used output directory
        std::wstring outputDir (ReadRegistryOutputDir (guid));
        // Schedule for removal
        removeHelper.ScheduleRemove (outputDir.c_str());
      }
      removeHelper.FlushDelayed();
    }
    // Remove list file
    {
      DWORD fileAttr (GetFileAttributesW (listFilePath.c_str()));
      if (fileAttr == INVALID_FILE_ATTRIBUTES)
      {
        // Weird.
        DWORD result (GetLastError());
        printf ("Huh. Error obtaining attributes for %ls: %ls\n", listFilePath.c_str(),
                GetErrorString (result).c_str());
      }
      else
      {
        // Remove it
        TryDelete (fileAttr, listFilePath.c_str());
      }
    }
    // Clean up registry
    {
      std::wstring keyPathUninstall (regPathUninstallInfo);
      keyPathUninstall.append (guid);
      RegistryDelete (keyPathUninstall.c_str());
    }
  }
  catch (const HRESULTException& e)
  {
    return e.GetHR();
  }

  return ecSuccess;
}
