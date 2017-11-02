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

#include "Repair.hpp"

#include "ArgsHelper.hpp"
#include "CommonArgs.hpp"
#include "Error.hpp"
#include "ExitCode.hpp"
#include "Extract.hpp"
#include "InstalledFiles.hpp"
#include "Paths.hpp"
#include "RegistryLocations.hpp"
#include "Remove.hpp"

#include <memory>
#include <unordered_set>

int DoRepair (const ArgsHelper& args)
{
  CommonArgs commonArgs (args);
  if (!commonArgs.checkValid (CommonArgs::Archives::Required))
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
    // Grab previously used output directory
    MyUString outputDir (ReadRegistryOutputDir (commonArgs.GetInstallScope (), commonArgs.GetGUID ()));

    // Extract archive(s)
    std::vector<MyUString> extractedFiles;
    // Extract archives
    Extract (archives, outputDir.Ptr(), extractedFiles);
    // TODO: Should canonicalize extractedFiles

    // Get list of previously installed files
    MyUString listFilePath (ReadRegistryListFilePath (commonArgs.GetInstallScope (), commonArgs.GetGUID ()));
    std::unique_ptr<InstalledFilesReader> listReader;
    try
    {
      listReader.reset (new InstalledFilesReader (listFilePath.Ptr()));
    }
    catch(...)
    {
      // Ignore any errors; just means no 'old' files can be removed
    }

    // Remove all files that were in the old set, but have not been extracted this time
    if (listReader)
    {
      std::unordered_set<MyUString> extractedFilesSet;
      extractedFilesSet.insert (extractedFiles.begin(), extractedFiles.end());
      {
        RemoveHelper removeHelper;

        MyUString installedFile;
        while (!(installedFile = listReader->GetFileName()).IsEmpty())
        {
          // TODO: Should canonicalize file name
          if (extractedFilesSet.find (installedFile) == extractedFilesSet.end())
          {
            removeHelper.ScheduleRemove (installedFile.Ptr());
          }
        }

        removeHelper.FlushDelayed();
      }
    }
    // Need to close handle to list file
    listReader.reset ();

    // Generate file list
    InstalledFilesWriter listWriter (logLocation.GetFilename());
    try
    {
      // Add output dir to list so it'll get deleted on uninstall
      listWriter.AddEntries (extractedFiles);
      // Write registry entries
      WriteToRegistry (commonArgs.GetInstallScope (), commonArgs.GetGUID (), listWriter.GetLogFileName(), outputDir.Ptr());
    }
    catch(...)
    {
      listWriter.Discard ();
      throw;
    }
  }
  catch (const HRESULTException& e)
  {
    return e.GetHR();
  }

  return ecSuccess;
}
