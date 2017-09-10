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

#include "Install.hpp"

#include "ArgsHelper.hpp"
#include "CommonArgs.hpp"
#include "Error.hpp"
#include "ExitCode.hpp"
#include "Extract.hpp"
#include "InstalledFiles.hpp"
#include "Paths.hpp"
#include "Registry.hpp"

// TODO: Move somewhere else...
void WriteToRegistry (const wchar_t* guid, const InstalledFilesWriter& list, const wchar_t* directory)
{
  const REGSAM key_access (KEY_ALL_ACCESS | KEY_WOW64_64KEY);
  {
    std::wstring keyPathUninstall (regPathUninstallInfo);
    keyPathUninstall.append (guid);
    AutoRootRegistryKey key (keyPathUninstall.c_str(), key_access, RegistryKey::Create);
    key.WriteString (regValLogFileName, list.GetLogFileName());
    key.WriteString (regValInstallDir, directory);
    key.WriteDWORD (L"SystemComponent", 1);
  }
  {
    std::wstring keyPathDependencies (regPathDependencyInfo);
    keyPathDependencies.append (guid);
    AutoRootRegistryKey key (keyPathDependencies.c_str(), key_access, RegistryKey::Create);
    // DisplayName?
    // Version?
  }
}

int DoInstall (int argc, const wchar_t* const argv[])
{
  const wchar_t* guid (nullptr);
  const wchar_t* out_dir (nullptr);
  std::vector<const wchar_t*> archives;

  ArgsHelper args (argc, argv);
  {
    CommonArgs commonArgs (args);
    if (!commonArgs.GetGUID (guid))
    {
      return ecArgsError;
    }
  }
  if (!args.GetOption (L"-o", out_dir))
  {
    printf ("'-o<DIR>' argument is required\n");
    return ecArgsError;
  }
  args.GetFreeArgs (archives);
  if (archives.empty())
  {
    printf ("No archive files provided\n");
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
      // Actually write list
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
