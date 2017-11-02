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

static void TryDelete (DWORD fileAttr, const wchar_t* file)
{
  if ((fileAttr & FILE_ATTRIBUTE_READONLY) != 0)
  {
    if (!SetFileAttributesW (file, fileAttr & ~FILE_ATTRIBUTE_READONLY))
    {
      DWORD result (GetLastError());
      printf ("Error clearing 'read-only' attribute of %ls: %ls\n", file,
              GetErrorString (result).Ptr());
      return;
    }
  }
  if (!DeleteFileW (file))
  {
    DWORD result (GetLastError());
    printf ("Error deleting %ls: %ls\n", file,
            GetErrorString (result).Ptr());
  }
}

static void TryDeleteDir (const wchar_t* path)
{
  if (!RemoveDirectoryW (path))
  {
    DWORD result (GetLastError());
    printf ("Error deleting %ls: %ls\n", path,
            GetErrorString (result).Ptr());
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
            GetErrorString (result).Ptr());
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
  qsort (directories.data(), directories.size(), sizeof (MyUString),
         [](const void* a, const void* b)
         {
           const auto& a_str = *(reinterpret_cast<const MyUString*> (a));
           const auto& b_str = *(reinterpret_cast<const MyUString*> (b));
           if (a_str.Len() > b_str.Len())
             return -1;
           else if (a_str.Len() < b_str.Len())
             return 1;
           return 0;
         });
  // Remove directories
  for (const MyUString& dir : directories)
  {
    TryDeleteDir (dir.Ptr());
  }
  directories.clear();
}

static void RegistryDelete (InstallScope installScope, const wchar_t* regPath)
{
  LRESULT err (SHDeleteKeyW (RegistryParent (installScope), regPath));
  if (err != ERROR_SUCCESS)
  {
    printf ("Error deleting %ls from registry: %ls\n", regPath, GetErrorString (err).Ptr());
  }
}

static size_t HasDependents (InstallScope installScope, const wchar_t* regPath)
{
  const REGSAM key_access (KEY_READ | KEY_WOW64_64KEY);
  try
  {
    RegistryKey key (RegistryParent (installScope), regPath, key_access);
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

int DoRemove (const ArgsHelper& args)
{
  CommonArgs commonArgs (args);
  if (!commonArgs.checkValid (CommonArgs::Archives::None))
  {
    return ecArgsError;
  }

  try
  {
    bool ignoreDependencies (args.GetOption (L"--ignore-dependents"));
    {
      MyUString keyPathDependencies (regPathDependencyInfo);
      keyPathDependencies += commonArgs.GetGUID();
      MyUString keyPathDependents (keyPathDependencies);
      keyPathDependents += regPathDependentsSubkey;
      if (!ignoreDependencies)
      {
        size_t has_dep (0);
        try
        {
          has_dep = HasDependents (commonArgs.GetInstallScope(), keyPathDependents.Ptr());
        }
        catch (const HRESULTException& e)
        {
          printf ("Error checking for dependents: %ls\n", GetHRESULTString (e.GetHR()).Ptr());
        }
        if (has_dep > 0)
        {
          printf ("Aborting due to %ld dependents being present.\n", (long)has_dep);
          return ecHasDependencies;
        }
      }
      RegistryDelete (commonArgs.GetInstallScope(), keyPathDependencies.Ptr());
    }

    // Obtain list file path from registry
    MyUString listFilePath (ReadRegistryListFilePath (commonArgs.GetInstallScope(), commonArgs.GetGUID()));
    {
      RemoveHelper removeHelper;

      // Open list file
      InstalledFilesReader listReader (listFilePath.Ptr());
      // Remove all listed files
      {
        MyUString installedFile;
        while (!(installedFile = listReader.GetFileName()).IsEmpty())
        {
          removeHelper.ScheduleRemove (installedFile.Ptr());
        }
      }

      // Handle request for output dir removal
      if (args.GetOption (L"-r"))
      {
        // Grab previously used output directory
        MyUString outputDir (ReadRegistryOutputDir (commonArgs.GetInstallScope(), commonArgs.GetGUID()));
        // Schedule for removal
        removeHelper.ScheduleRemove (outputDir.Ptr());
      }
      removeHelper.FlushDelayed();
    }
    // Remove list file
    {
      DWORD fileAttr (GetFileAttributesW (listFilePath.Ptr()));
      if (fileAttr == INVALID_FILE_ATTRIBUTES)
      {
        // Weird.
        DWORD result (GetLastError());
        printf ("Huh. Error obtaining attributes for %ls: %ls\n", listFilePath.Ptr(),
                GetErrorString (result).Ptr());
      }
      else
      {
        // Remove it
        TryDelete (fileAttr, listFilePath.Ptr());
      }
    }
    // Clean up registry
    {
      MyUString keyPathUninstall (regPathUninstallInfo);
      keyPathUninstall += commonArgs.GetGUID();
      RegistryDelete (commonArgs.GetInstallScope(), keyPathUninstall.Ptr());
    }
  }
  catch (const HRESULTException& e)
  {
    return e.GetHR();
  }

  return ecSuccess;
}
