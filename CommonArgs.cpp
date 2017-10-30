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

#include "CommonArgs.hpp"

#include "ArgsHelper.hpp"
#include "GUID.hpp"
#include "IsSFX.hpp"

CommonArgs::CommonArgs (const ArgsHelper& args, Archives archivesMode)
{
  {
    bool need_archives = archivesMode == Archives::Required;

    std::vector<const wchar_t*> archive_args;
    args.GetFreeArgs (archive_args);
    if (IsSFX (sfx_exe))
    {
      if (!archive_args.empty())
      {
        printf ("Executable is SFX, but archive files were provided\n");
      }
      else
      {
        archives.emplace_back (sfx_exe.Ptr());
      }
    }
    else
    {
      if (archive_args.empty() && need_archives)
      {
        printf ("No archive files provided\n");
      }
      archives = std::move (archive_args);
    }
  }

  bool result (args.GetOption (L"-g", guid) && (wcslen (guid) != 0));
  if (!result)
  {
    printf ("'-g<GUID>' argument is required\n");
  }
  else if (result && !VerifyGUID (guid))
  {
    printf ("Not an allowed GUID: '%ls'\n", guid);
  }

  if (args.GetOption (L"-U"))
    installScope = InstallScope::User;
  else if (args.GetOption (L"-M"))
    installScope = InstallScope::Machine;

  args.GetOption (L"-D", fullDataDir);
  args.GetOption (L"-d", dataDirName);
}

bool CommonArgs::isValid (Archives archivesMode) const
{
  bool archives_flag;
  switch (archivesMode)
  {
  case Archives::Required:
    archives_flag = !archives.empty();
    break;
  case Archives::None:
    archives_flag = archives.empty() || (archives[0] == sfx_exe.Ptr());
    break;
  case Archives::DontCare:
    archives_flag = true;
    break;
  }
  return (guid != nullptr) && archives_flag;
}

const std::vector<const wchar_t*>& CommonArgs::GetArchives() const { return archives; }

const wchar_t* CommonArgs::GetGUID () const { return guid; }

InstallScope CommonArgs::GetInstallScope () const { return installScope; }

const wchar_t* CommonArgs::GetFullDataDir () const { return fullDataDir; }

const wchar_t* CommonArgs::GetDataDirName () const { return dataDirName; }
