#include "Install.hpp"

#include "ArgsHelper.hpp"
#include "Error.hpp"
#include "ExitCode.hpp"
#include "Extract.hpp"
#include "InstalledFiles.hpp"
#include "Paths.hpp"
#include "Registry.hpp"

static void WriteToRegistry (RegistryKey& key, const InstalledFilesWriter& list, const wchar_t* directory)
{
  key.WriteString (regValLogFileName, list.GetLogFileName());
  key.WriteString (regValInstallDir, directory);
  key.WriteDWORD (L"SystemComponent", 1);
}

// TODO: Move somewhere else...
void WriteToRegistry (const wchar_t* guid, const InstalledFilesWriter& list, const wchar_t* directory)
{
  const REGSAM key_access (KEY_ALL_ACCESS | KEY_WOW64_64KEY);
  {
    std::wstring keyPathUninstall (regPathUninstallInfo);
    keyPathUninstall.append (guid);
    try
    {
      RegistryKey key (HKEY_LOCAL_MACHINE, keyPathUninstall.c_str(), key_access, RegistryKey::Create);
      WriteToRegistry (key, list, directory);
    }
    catch (const HRESULTException& e)
    {
      if (e.GetHR() == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
      {
        // Try again with HKCU
        RegistryKey key (HKEY_CURRENT_USER, keyPathUninstall.c_str(), key_access, RegistryKey::Create);
        WriteToRegistry (key, list, directory);
      }
      else
        throw;
    }
  }
  {
    std::wstring keyPathDependencies (regPathDependencyInfo);
    keyPathDependencies.append (guid);
    try
    {
      RegistryKey key (HKEY_LOCAL_MACHINE, keyPathDependencies.c_str(), key_access, RegistryKey::Create);
      // DisplayName?
      // Version?
    }
    catch (const HRESULTException& e)
    {
      if (e.GetHR() == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
      {
        // Try again with HKCU
        RegistryKey key (HKEY_CURRENT_USER, keyPathDependencies.c_str(), key_access, RegistryKey::Create);
      }
      else
        throw;
    }
  }
}

int DoInstall (int argc, const wchar_t* const argv[])
{
  const wchar_t* guid (nullptr);
  const wchar_t* out_dir (nullptr);
  std::vector<const wchar_t*> archives;

  ArgsHelper args (argc, argv);
  if (!args.GetOption (L"-g", guid) || (wcslen (guid) == 0))
  {
    wprintf (L"'-g<GUID>' argument is required\n");
    return ecArgsError;
  }
  if (!args.GetOption (L"-o", out_dir))
  {
    wprintf (L"'-o<DIR>' argument is required\n");
    return ecArgsError;
  }
  args.GetFreeArgs (archives);
  if (archives.empty())
  {
    wprintf (L"No archive files provided\n");
    return ecArgsError;
  }

  // FIXME: Check if GUID is an actual, valid GUID?
  // FIXME: Check (& possibly fail) if already installed? Or perhaps behave like a "repair" then? Both (choose from command line)?
  // FIXME: Rollback feature?
    
  try
  {
    // Set up storage of "installation log" (list of extracted files)
    InstalledFilesWriter listWriter (guid);
    try
    {
      std::vector<std::wstring> extractedFiles;
      // Extract archives
      Extract (archives, out_dir, extractedFiles);
      // Add output dir to list so it'll get deleted on uninstall
      listWriter.AddEntries (extractedFiles);
      // Record install in registry
      WriteToRegistry (guid, listWriter, out_dir);
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
