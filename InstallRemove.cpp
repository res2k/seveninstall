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
#include "ProgressReporter.hpp"
#include "Registry.hpp"
#include "RegistryLocations.hpp"

#include "Windows/FileName.h"

#include <queue>
#include <memory>
#include <unordered_set>

// Helper: Sort a std::vector<MyUString>
template<typename T, typename SortFunc>
static inline void SortVec (std::vector<T>& vec, SortFunc sort_func)
{
  qsort_s (vec.data(), vec.size(), sizeof (T),
           [](void* f, const void* a, const void* b)
           {
             const auto& a_str = *(reinterpret_cast<const T*> (a));
             const auto& b_str = *(reinterpret_cast<const T*> (b));
             return (*reinterpret_cast<SortFunc*> (f))(a_str, b_str);
           },
           &sort_func);
}

// Update a HRESULT with a new one if it indicates success.
static void UpdateHR (HRESULT& hr, HRESULT newHr)
{
  if (SUCCEEDED(hr)) hr = newHr;
}

class RemoveHelper
{
  HRESULT hr = S_OK;
  size_t notFoundCounter = 0;
  struct Dir
  {
    MyUString path;
    bool recursive = false;
  };
  std::vector<Dir> directories;
  std::unordered_set<MyUString> reallyDeleted;
public:
  ~RemoveHelper();

  HRESULT GetHR() const { return hr; }
  const std::unordered_set<MyUString>& GetReallyDeleted() const { return reallyDeleted; }

  void ScheduleRemove (const wchar_t* path);
  void FlushDelayed (ProgressReporter& progress);
};

#include "Shlwapi.h"

static DWORD TryDelete (DWORD fileAttr, const wchar_t* file)
{
  if ((fileAttr & FILE_ATTRIBUTE_READONLY) != 0)
  {
    SetFileAttributesW (file, fileAttr & ~FILE_ATTRIBUTE_READONLY);
    // Ignore errors, assume DeleteFile() will fail anyway
  }
  if (!DeleteFileW (file))
  {
    DWORD result (GetLastError());
    fprintf (stderr, "Error deleting %ls: %ls\n", file,
             GetErrorString (result).Ptr());
    return result;
  }
  return ERROR_SUCCESS;
}

static DWORD TryDelete (const wchar_t* file)
{
  DWORD fileAttr (GetFileAttributesW (file));
  if (fileAttr == INVALID_FILE_ATTRIBUTES)
  {
    // Weird.
    DWORD result (GetLastError());
    return result;
  }
  else
  {
    // Remove it
    return TryDelete (fileAttr, file);
  }
}

static DWORD TryDeleteDir (const wchar_t* path)
{
  if (!RemoveDirectoryW (path))
  {
    DWORD result (GetLastError());
    return result;
  }
  return ERROR_SUCCESS;
}

static DWORD TryDeleteDirRecursive (const wchar_t* path)
{
  typedef std::pair<MyUString, DWORD> deleteQueueEntry;
  std::deque<deleteQueueEntry> deleteQueue;

  DWORD fileAttr (GetFileAttributesW (path));
  if (fileAttr == INVALID_FILE_ATTRIBUTES)
  {
    return GetLastError ();
  }

  deleteQueue.push_back (std::make_pair(path, fileAttr));
  while (!deleteQueue.empty ())
  {
    auto currentEntry = std::move (deleteQueue.front ());
    deleteQueue.pop_front ();

    DWORD deleteResult = ERROR_SUCCESS;
    if ((currentEntry.second & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
      MyUString findPattern (currentEntry.first);
      findPattern += L"\\*";
      WIN32_FIND_DATA findData;
      auto findHandle = FindFirstFileEx (findPattern.Ptr (), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
      if (findHandle != INVALID_HANDLE_VALUE)
      {
        size_t numFiles = 0;
        do
        {
          if ((wcscmp (findData.cFileName, L".") == 0) || (wcscmp (findData.cFileName, L"..") == 0)) continue;
          MyUString fullPath (currentEntry.first);
          fullPath += L"\\";
          fullPath += findData.cFileName;
          deleteQueue.push_front (std::make_pair (std::move (fullPath), findData.dwFileAttributes));
          ++numFiles;
        } while (FindNextFile (findHandle, &findData));
        FindClose (findHandle);

        if (numFiles != 0)
        {
          // Scan again, later
          deleteQueue.push_back (currentEntry);
        }
        else
        {
          deleteResult = TryDeleteDir (currentEntry.first);
        }
      }
    }
    else
    {
      deleteResult = TryDelete (currentEntry.first);
    }

    if ((deleteResult != ERROR_SUCCESS) && (deleteResult != ERROR_FILE_NOT_FOUND))
    {
      fprintf (stderr, "Error deleting %ls: %ls\n", currentEntry.first.Ptr(),
        GetErrorString (deleteResult).Ptr ());
    }
  }

  return ERROR_SUCCESS;
}

RemoveHelper::~RemoveHelper()
{
  ProgressReporterDummy dummyProgress;
  FlushDelayed (dummyProgress);
}

void RemoveHelper::ScheduleRemove (const wchar_t* path)
{
  // Special case: A path ending with "\**" is assumed to be a directory and removed recursively
  auto path_len = wcslen (path);
  if ((path_len >= 3) && (wcscmp (path + path_len - 3, L"\\**") == 0))
  {
    MyUString new_path (path, path_len - 3);
    directories.push_back (Dir{ std::move (new_path), true });
    return;
  }

  DWORD fileAttr (GetFileAttributesW (path));
  if (fileAttr == INVALID_FILE_ATTRIBUTES)
  {
    // File does not exist (probably)...
    DWORD result (GetLastError());
    if (result == ERROR_FILE_NOT_FOUND)
    {
      ++notFoundCounter;
      reallyDeleted.insert (path);
    }
    else if (result != ERROR_SUCCESS)
    {
      UpdateHR (hr, HRESULT_FROM_WIN32(result));
      fprintf (stderr, "Error obtaining attributes for %ls: %ls\n", path,
               GetErrorString (result).Ptr ());
    }
    else
    {
      reallyDeleted.insert (path);
    }
  }
  else if ((fileAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
  {
    // Handle directories later
    directories.push_back (Dir{ path, false });
  }
  else
  {
    // Remove it
    auto result = TryDelete (fileAttr, path);
    if (result == ERROR_FILE_NOT_FOUND)
    {
      ++notFoundCounter;
      reallyDeleted.insert (path);
    }
    else if (result != ERROR_SUCCESS)
    {
      UpdateHR (hr, HRESULT_FROM_WIN32(result));
      fprintf (stderr, "Error deleting %ls: %ls\n", path,
               GetErrorString (result).Ptr ());
    }
    else
    {
      reallyDeleted.insert (path);
    }
  }
}

void RemoveHelper::FlushDelayed (ProgressReporter& progress)
{
  progress.SetTotal (directories.size ());
  // Sort directory by length descending (to get deeper nested dirs first)
  SortVec (directories,
    [](const Dir& a, const Dir& b)
    {
      if (a.path.Len () > b.path.Len ())
        return -1;
      else if (a.path.Len () < b.path.Len ())
        return 1;
      return 0;
    });
  // Remove directories
  size_t count = 0;
  for (const auto& dir : directories)
  {
    auto result = dir.recursive ? TryDeleteDirRecursive(dir.path.Ptr()) : TryDeleteDir (dir.path.Ptr());
    if (result == ERROR_FILE_NOT_FOUND)
    {
      ++notFoundCounter;
      reallyDeleted.insert (dir.path);
    }
    else if (result != ERROR_SUCCESS)
    {
      // Print the error, but don't let it cause an overall failure
      fprintf (stderr, "Error deleting %ls: %ls\n", dir.path.Ptr (),
               GetErrorString (result).Ptr ());
    }
    else
    {
      reallyDeleted.insert (dir.path);
    }
    progress.SetCompleted (++count);
  }
  directories.clear();

  if (notFoundCounter > 0)
  {
    fprintf (stderr, "%zu files/directories to delete were not found.\n", notFoundCounter);
  }

  notFoundCounter = 0;
}

static void RegistryDelete (InstallScope installScope, const wchar_t* regPath)
{
  const REGSAM key_access (KEY_READ | KEY_SET_VALUE | DELETE | KEY_WOW64_64KEY);
  try
  {
    MyUString parentPath;
    const wchar_t* keyName = nullptr;
    if (auto keyNameSep = wcsrchr (regPath, '\\'))
    {
      parentPath = MyUString (regPath, keyNameSep - regPath);
      keyName = keyNameSep + 1;
    }
    else
    {
      parentPath = regPath;
    }
    RegistryKey key (RegistryParent (installScope), parentPath.Ptr(), key_access);
    LRESULT err (SHDeleteKeyW (key, keyName));
    if (err != ERROR_SUCCESS)
    {
      fprintf (stderr, "Error deleting %ls from registry: %ls\n", regPath, GetErrorString (err).Ptr());
    }
  }
  catch (const HRESULTException& e)
  {
    fprintf (stderr, "Error deleting %ls from registry: %ls\n", regPath,
             GetHRESULTString (e.GetHR()).Ptr());
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

static MyUString GetExeDir ()
{
  auto path = GetExePath();
  int pathsep = path.ReverseFind_PathSepar();
  if (pathsep != -1) path.DeleteFrom (pathsep + 1);
  return path;
}

static std::unordered_set<MyUString> ReadPreviousFilesList (const CommonArgs& commonArgs,
                                                            MyUString& listFilePath,
                                                            bool silent)
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
    // Else: Continue
    listException = std::current_exception();
  }

  // Print list exception
  if (listException && !silent)
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

int DoInstallRemove (const ArgsHelper& args, BurnPipe& pipe, Action action)
{
  bool doExtract = (action == Action::Install) || (action == Action::Repair);
  bool doRemove = (action == Action::Remove) || (action == Action::Repair);

  /* HRESULT value to return. Updated when a sub-action failed, but other
   * sub-actions could still be performed. */
  HRESULT actionHR = S_OK;

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

  auto progressOutput = GetDefaultProgress (pipe);

  ProgressReporterMultiStep actionProgress (*progressOutput);
  auto progPhaseRegistryDelete = actionProgress.AddPhase (doRemove ? 1 : 0);
  auto progPhaseReadFilesLists = actionProgress.AddPhase (doRemove ? 2 : 0);
  auto progPhaseExtract = actionProgress.AddPhase (doExtract ? 100 : 0);
  auto progPhaseRemoveFiles = actionProgress.AddPhase (doRemove ? 100 : 0);
  auto progPhaseRemoveFlush = actionProgress.AddPhase (doRemove ? 5 : 0);
  auto progPhaseRemoveCleanup = actionProgress.AddPhase (doRemove ? 1 : 0);
  auto progPhaseWriteList = actionProgress.AddPhase (doExtract ? 1 : 0);
  auto progPhaseFinish = actionProgress.AddPhase (doExtract ? 1 : 0);

  archives = commonArgs.GetArchives ();

  try
  {
    const wchar_t* outDirArg = nullptr;
    MyUString outputDir;
    try
    {
      outputDir = GetOutputDirectory (args, commonArgs, action, outDirArg);
      if (outputDir.IsEmpty ())
      {
        if (action == Action::Install)
          fprintf (stderr, "'-o<DIR>' argument is required\n");
        else
          fprintf (stderr, "No installation directory found\n");
        if (!doRemove) return ecArgsError;
      }
    }
    catch (const HRESULTException& e)
    {
      fprintf (stderr, "Error determining installation directory: %ls\n", GetHRESULTString (e.GetHR()).Ptr());
      if (!doRemove) return e.GetHR();
    }
    catch (const std::exception& e)
    {
      fprintf (stderr, "Error determining installation directory: %s\n", e.what());
      if (!doRemove) return ecException;
    }

    if (action == Action::Remove)
    {
      auto& progRegistryDelete = actionProgress.GetPhase (progPhaseRegistryDelete);
      progRegistryDelete.SetTotal (1);

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

      progRegistryDelete.SetCompleted (1);
    }

    auto& progReadFilesLists = actionProgress.GetPhase (progPhaseReadFilesLists);
    progReadFilesLists.SetTotal (2);
    std::optional<InstalledFilesCounter> filesCounter;
    if (doRemove && !args.GetOption (L"--no-global-refs"))
    {
      // FIXME?: This may have weird results if the actual list file is at another location!
      filesCounter = InstalledFilesCounter (logLocation.GetLogsPath());
    }
    progReadFilesLists.SetCompleted (1);

    // Grab previous files list
    MyUString listFilePath;
    auto previousFiles = ReadPreviousFilesList (commonArgs, listFilePath, action == Action::Install);
    progReadFilesLists.SetCompleted (2);

    // Extract new files (Install/Repair)
    std::vector<MyUString> extractedFiles;
    if (doExtract)
    {
      try
      {
        Extract (actionProgress.GetPhase (progPhaseExtract), archives, outDirArg ? outDirArg : outputDir.Ptr(), extractedFiles);
      }
      catch(const HRESULTException& e)
      {
        actionHR = e.GetHR();
      }
    }

    // Remove previous files (Remove/Repair)
    if (doRemove)
    {
      auto& progRemoveFiles = actionProgress.GetPhase (progPhaseRemoveFiles);

      // ...but exclude those just extracted
      for (const auto& extracted : extractedFiles)
      {
        previousFiles.erase (extracted);
      }

      progRemoveFiles.SetTotal (previousFiles.size());
      RemoveHelper removeHelper;
      size_t n = 0;
      for (const auto& removeFile : previousFiles)
      {
        if (!filesCounter || (filesCounter->DecFileRef (removeFile) == 0))
          removeHelper.ScheduleRemove (removeFile.Ptr());
        progRemoveFiles.SetCompleted (++n);
      }

      auto& progRemoveFlush = actionProgress.GetPhase (progPhaseRemoveFlush);
      // Remove old directory, either for Remove action, or a 'move' repair
      if (doRemove && (!outDirArg || (outputDir != outDirArg)))
      {
        // Handle request for output dir removal
        if (args.GetOption (L"-r"))
        {
          // Schedule for removal
          if (!outputDir.IsEmpty()) removeHelper.ScheduleRemove (outputDir.Ptr());
        }
      }
      removeHelper.FlushDelayed (progRemoveFlush);
      UpdateHR (actionHR, removeHelper.GetHR());

      auto& progRemoveCleanup = actionProgress.GetPhase (progPhaseRemoveCleanup);
      progRemoveCleanup.SetTotal (2);
      for (const auto& deleted_file : removeHelper.GetReallyDeleted())
      {
        previousFiles.erase (deleted_file);
      }
      // Remove previous list file
      if (!listFilePath.IsEmpty()) TryDelete (listFilePath.Ptr());
      progRemoveCleanup.SetCompleted (1);

      // Remove registry entry
      {
        MyUString keyPathUninstall (regPathUninstallInfo);
        keyPathUninstall += commonArgs.GetGUID();
        RegistryDelete (commonArgs.GetInstallScope(), keyPathUninstall.Ptr());
      }
      progRemoveCleanup.SetCompleted (2);
    }

    // Write new files list (Install/Repair)
    if (doExtract)
    {
      std::unordered_set<MyUString> allFiles (extractedFiles.begin(), extractedFiles.end());
      allFiles.insert (previousFiles.begin(), previousFiles.end());

      // Add artifacts files, if given
      const wchar_t* artifactsFile;
      if (args.GetOption (L"-A", artifactsFile))
      {
        auto artifactsFileFull = GetExeDir() + artifactsFile;

        try
        {
          MyUString outDir = outDirArg ? outDirArg : outputDir.Ptr();
          NWindows::NFile::NName::NormalizeDirPathPrefix (outDir);
          InstalledFilesReader artifactsReader (artifactsFileFull);

          MyUString artifactFile;
          while (!(artifactFile = artifactsReader.GetFileName()).IsEmpty())
          {
            artifactFile = outDir + artifactFile;
            NormalizePath (artifactFile);
            allFiles.insert (artifactFile);
          }
        }
        catch (const HRESULTException& e)
        {
          fprintf (stderr, "Error reading artifacts from %ls: %ls\n", artifactsFileFull.Ptr(), GetHRESULTString (e.GetHR()).Ptr());
        }
        catch (const std::exception& e)
        {
          fprintf (stderr, "Error reading artifacts from %ls: %s\n", artifactsFileFull.Ptr(), e.what());
        }
      }

      // Generate file list
      InstalledFilesWriter listWriter (logLocation.GetFilename());
      try
      {
        auto& progWriteList = actionProgress.GetPhase (progPhaseWriteList);
        progWriteList.SetTotal (1);

        // Add output dir to list so it'll get deleted on uninstall
        std::vector<MyUString> allFiles_v (allFiles.begin(), allFiles.end());
        SortVec (allFiles_v,
          [](const MyUString& a, const MyUString& b)
          {
            return wmemcmp (a.Ptr (), b.Ptr (), std::min (a.Len (), b.Len ()) + 1);
          });
        listWriter.AddEntries (allFiles_v);

        // Write registry entries
        auto& progFinish = actionProgress.GetPhase (progPhaseFinish);
        progFinish.SetTotal (1);
        WriteToRegistry (commonArgs.GetInstallScope (), commonArgs.GetGUID (), listWriter.GetLogFileName(), outputDir.Ptr());
        progFinish.SetCompleted (1);
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

  if (FAILED(actionHR)) return actionHR;
  return ecSuccess;
}
