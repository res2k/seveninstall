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

#include "InstallRemove.hpp"

#include "ArgsHelper.hpp"
#include "CommonArgs.hpp"
#include "Error.hpp"
#include "ExitCode.hpp"
#include "Extract.hpp"
#include "InstalledFiles.hpp"
#include "Paths.hpp"
#include "Registry.hpp"
#include "RegistryLocations.hpp"

#include <memory>
#include <unordered_set>

// Helper: Sort a std::vector<MyUString>
template<typename SortFunc>
static inline void SortStringVec (std::vector<MyUString>& vec, SortFunc sort_func)
{
  qsort_s (vec.data(), vec.size(), sizeof (MyUString),
           [](void* f, const void* a, const void* b)
           {
             const auto& a_str = *(reinterpret_cast<const MyUString*> (a));
             const auto& b_str = *(reinterpret_cast<const MyUString*> (b));
             return (*reinterpret_cast<SortFunc*> (f))(a_str, b_str);
           },
           &sort_func);
}

class RemoveHelper
{
  std::vector<MyUString> directories;
public:
  ~RemoveHelper();

  void ScheduleRemove (const wchar_t* path);
  void FlushDelayed ();
};

#include "Shlwapi.h"

static void TryDelete (DWORD fileAttr, const wchar_t* file)
{
  if ((fileAttr & FILE_ATTRIBUTE_READONLY) != 0)
  {
    if (!SetFileAttributesW (file, fileAttr & ~FILE_ATTRIBUTE_READONLY))
    {
      DWORD result (GetLastError());
      fprintf (stderr, "Error clearing 'read-only' attribute of %ls: %ls\n", file,
               GetErrorString (result).Ptr());
      return;
    }
  }
  if (!DeleteFileW (file))
  {
    DWORD result (GetLastError());
    fprintf (stderr, "Error deleting %ls: %ls\n", file,
             GetErrorString (result).Ptr());
  }
}

static void TryDelete (const wchar_t* file)
{
  DWORD fileAttr (GetFileAttributesW (file));
  if (fileAttr == INVALID_FILE_ATTRIBUTES)
  {
    // Weird.
    DWORD result (GetLastError());
    fprintf (stderr, "Huh. Error obtaining attributes for %ls: %ls\n", file,
             GetErrorString (result).Ptr());
  }
  else
  {
    // Remove it
    TryDelete (fileAttr, file);
  }
}

static void TryDeleteDir (const wchar_t* path)
{
  if (!RemoveDirectoryW (path))
  {
    DWORD result (GetLastError());
    fprintf (stderr, "Error deleting %ls: %ls\n", path,
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
    fprintf (stderr, "Error obtaining attributes for %ls: %ls\n", path,
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
  SortStringVec (directories,
                 [](const MyUString& a, const MyUString& b)
                 {
                   if (a.Len() > b.Len())
                     return -1;
                   else if (a.Len() < b.Len())
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
    fprintf (stderr, "Error deleting %ls from registry: %ls\n", regPath, GetErrorString (err).Ptr());
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

static MyUString GetOutputDirectory (const ArgsHelper& args,
                                     const CommonArgs& commonArgs,
                                     Action action,
                                     const wchar_t*& outDirArg)
{
  /* Output directory:
   * - Install: Prefer command line
   * - Repair: Prefer previously used directory 
   * - Remove: Always use previously used directory */

  const wchar_t* realOutDirArg = nullptr;
  args.GetOption (L"-o", realOutDirArg);
  outDirArg = action != Action::Remove ? realOutDirArg : nullptr;
  switch (action)
  {
  case Action::Install:
    if (realOutDirArg) return realOutDirArg;
    break;
  case Action::Repair:
  case Action::Remove:
    {
      std::exception_ptr readRegException;
      try
      {
        auto regPath = ReadRegistryOutputDir (commonArgs.GetInstallScope (), commonArgs.GetGUID ());
        if (!regPath.IsEmpty()) return regPath;
      }
      catch (...)
      {
        readRegException = std::current_exception();
      }
      if ((action == Action::Repair) && realOutDirArg)
        return realOutDirArg;
      if (readRegException)
        std::rethrow_exception (readRegException);
    }
    break;
  }

  return MyUString ();
}

static std::unordered_set<MyUString> ReadPreviousFilesList (const CommonArgs& commonArgs,
                                                            bool requireList,
                                                            MyUString& listFilePath)
{
  std::unordered_set<MyUString> list;

  std::exception_ptr listException;
  try
  {
    listFilePath = ReadRegistryListFilePath (commonArgs.GetInstallScope (), commonArgs.GetGUID ());
    InstalledFilesReader listReader (listFilePath.Ptr ());

    MyUString installedFile;
    while (!(installedFile = listReader.GetFileName()).IsEmpty())
    {
      NormalizePath (installedFile);
      list.insert (installedFile);
    }
  }
  catch (...)
  {
    if (requireList) throw;
    // Else: Continue
    listException = std::current_exception();
  }

  // Print list exception
  if (listException)
  {
    try
    {
      std::rethrow_exception (listException);
    }
    catch (HRESULTException& hre)
    {
      fprintf (stderr, "Error reading list of installed files: %ls\n",
               GetHRESULTString (hre.GetHR()).Ptr());
    }
    catch (std::exception& e)
    {
      fprintf (stderr, "Error reading list of installed files: %s\n", e.what ());
    }
  }

  return list;
}

int DoInstallRemove (const ArgsHelper& args, Action action)
{
  bool doExtract = (action == Action::Install) || (action == Action::Repair);
  bool doRemove = (action == Action::Remove) || (action == Action::Repair);

  CommonArgs commonArgs (args);
  if (!commonArgs.checkValid (doExtract ? CommonArgs::Archives::Required : CommonArgs::Archives::None))
  {
    return ecArgsError;
  }

  std::vector<const wchar_t*> archives;

  InstallLogLocation logLocation;
  if (!logLocation.Init (commonArgs))
  {
    return ecArgsError;
  }

  archives = commonArgs.GetArchives ();

  try
  {
    const wchar_t* outDirArg = nullptr;
    MyUString outputDir = GetOutputDirectory (args, commonArgs, action, outDirArg);
    if (outputDir.IsEmpty())
    {
      if (action == Action::Install)
        fprintf (stderr, "'-o<DIR>' argument is required\n");
      else
        fprintf (stderr, "No installation directory found\n");
      return ecArgsError;
    }

    if (action == Action::Remove)
    {
      bool ignoreDependencies = args.GetOption (L"--ignore-dependents");
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
          fprintf (stderr, "Error checking for dependents: %ls\n", GetHRESULTString (e.GetHR()).Ptr());
        }
        if (has_dep > 0)
        {
          fprintf (stderr, "Aborting due to %ld dependents being present.\n", (long)has_dep);
          return ecHasDependencies;
        }
      }
      RegistryDelete (commonArgs.GetInstallScope(), keyPathDependencies.Ptr());
    }

    std::optional<InstalledFilesCounter> filesCounter;
    if (doRemove && !args.GetOption (L"--no-global-refs"))
    {
      // FIXME?: This may have weird results if the actual list file is at another location!
      filesCounter = InstalledFilesCounter (logLocation.GetLogsPath());
    }

    // Grab previous files list
    MyUString listFilePath;
    auto previousFiles = ReadPreviousFilesList (commonArgs, action == Action::Remove, listFilePath);

    // Extract new files (Install/Repair)
    std::vector<MyUString> extractedFiles;
    if (doExtract)
    {
      Extract (archives, outDirArg ? outDirArg : outputDir.Ptr(), extractedFiles);
    }

    // Remove previous files (Remove/Repair)
    if (doRemove)
    {
      // ...but exclude those just extracted
      for (const auto& extracted : extractedFiles)
      {
        previousFiles.erase (extracted);
      }

      RemoveHelper removeHelper;
      for (const auto& removeFile : previousFiles)
      {
        if (!filesCounter || (filesCounter->DecFileRef (removeFile) == 0))
          removeHelper.ScheduleRemove (removeFile.Ptr());
      }

      // Remove old directory, either for Remove action, or a 'move' repair
      if (doRemove && (!outDirArg || (outputDir != outDirArg)))
      {
        // Handle request for output dir removal
        if (args.GetOption (L"-r"))
        {
          // Schedule for removal
          removeHelper.ScheduleRemove (outputDir.Ptr());
        }
      }
      removeHelper.FlushDelayed();

      previousFiles.clear();
      // Remove previous list file
      if (!listFilePath.IsEmpty()) TryDelete (listFilePath.Ptr());

      // Remove registry entry
      {
        MyUString keyPathUninstall (regPathUninstallInfo);
        keyPathUninstall += commonArgs.GetGUID();
        RegistryDelete (commonArgs.GetInstallScope(), keyPathUninstall.Ptr());
      }
    }

    // Write new files list (Install/Repair)
    if (doExtract)
    {
      std::set<MyUString> allFiles (extractedFiles.begin(), extractedFiles.end());
      allFiles.insert (previousFiles.begin(), previousFiles.end());;

      // Generate file list
      InstalledFilesWriter listWriter (logLocation.GetFilename());
      try
      {
        // Add output dir to list so it'll get deleted on uninstall
        listWriter.AddEntries (allFiles);
        // Write registry entries
        WriteToRegistry (commonArgs.GetInstallScope (), commonArgs.GetGUID (), listWriter.GetLogFileName(), outputDir.Ptr());
      }
      catch(...)
      {
        listWriter.Discard ();
        throw;
      }
    }
  }
  catch (const HRESULTException& e)
  {
    fprintf (stderr, "Error during action: %ls\n", GetHRESULTString (e.GetHR()).Ptr());
    return e.GetHR();
  }
  catch (const std::exception& e)
  {
    fprintf (stderr, "Error during action: %s\n", e.what());
    return ecException;
  }

  return ecSuccess;
}
