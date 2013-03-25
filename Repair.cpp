#include "Repair.hpp"

#include "ArgsHelper.hpp"
#include "Error.hpp"
#include "ExitCode.hpp"
#include "Extract.hpp"
#include "Install.hpp"
#include "InstalledFiles.hpp"
#include "Paths.hpp"
#include "Registry.hpp"
#include "Remove.hpp"

#include <unordered_set>

static std::wstring ReadRegistryOutputDir (const wchar_t* guid)
{
  const REGSAM key_access (KEY_READ | KEY_WOW64_64KEY);
  {
    std::wstring keyPathUninstall (regPathUninstallInfo);
    keyPathUninstall.append (guid);
    AutoRootRegistryKey key (keyPathUninstall.c_str(), key_access);
    return key.ReadString (regValInstallDir);
  }
}

int DoRepair (int argc, const wchar_t* const argv[])
{
  const wchar_t* guid (nullptr);
  std::vector<const wchar_t*> archives;

  ArgsHelper args (argc, argv);
  if (!args.GetOption (L"-g", guid) || (wcslen (guid) == 0))
  {
    wprintf (L"'-g<GUID>' argument is required\n");
    return ecArgsError;
  }
  args.GetFreeArgs (archives);
  if (archives.empty())
  {
    wprintf (L"No archive files provided\n");
    return ecArgsError;
  }

  try
  {
    // Grab previously used output directory
    std::wstring outputDir (ReadRegistryOutputDir (guid));

    // Extract archive(s)
    std::vector<std::wstring> extractedFiles;
    // Extract archives
    Extract (archives, outputDir.c_str(), extractedFiles);
    // TODO: Should canonicalize extractedFiles

    // Get list of previously installed files
    std::wstring listFilePath (ReadRegistryListFilePath (guid));
    InstalledFilesReader listReader (listFilePath.c_str());

    // Remove all files that were in the old set, but have not been extracted this time
    {
      std::unordered_set<std::wstring> extractedFilesSet;
      extractedFilesSet.insert (extractedFiles.begin(), extractedFiles.end());
      {
        RemoveHelper removeHelper;

        std::wstring installedFile;
        while (!(installedFile = listReader.GetFileName()).empty())
        {
          // TODO: Should canonicalize file name
          if (extractedFilesSet.find (installedFile) == extractedFilesSet.end())
          {
            removeHelper.ScheduleRemove (installedFile.c_str());
          }
        }

        removeHelper.FlushDelayed();
      }
    }

    // Generate file list
    InstalledFilesWriter listWriter (guid);
    try
    {
      // Add output dir to list so it'll get deleted on uninstall
      listWriter.AddEntries (extractedFiles);
      // Write registry entries
      WriteToRegistry (guid, listWriter, outputDir.c_str());
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
