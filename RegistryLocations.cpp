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

#include "RegistryLocations.hpp"

#include "Paths.hpp"
#include "Registry.hpp"

const wchar_t regPathUninstallInfo[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
const wchar_t regPathDependencyInfo[] = L"Software\\Classes\\Installer\\Dependencies\\";
const wchar_t regPathDependentsSubkey[] = L"\\Dependents";
const wchar_t regValLogFileName[] = L"LogFileName";
const wchar_t regValInstallDir[] = L"InstallDir";

std::wstring ReadRegistryListFilePath (InstallScope installScope, const wchar_t* guid)
{
  const REGSAM key_access (KEY_READ | KEY_WOW64_64KEY);
  {
    std::wstring keyPathUninstall (regPathUninstallInfo);
    keyPathUninstall.append (guid);
    RegistryKey key (RegistryParent (installScope), keyPathUninstall.c_str(), key_access);
    return key.ReadString (regValLogFileName);
  }
}

std::wstring ReadRegistryOutputDir (InstallScope installScope, const wchar_t* guid)
{
  const REGSAM key_access (KEY_READ | KEY_WOW64_64KEY);
  {
    std::wstring keyPathUninstall (regPathUninstallInfo);
    keyPathUninstall.append (guid);
    RegistryKey key (RegistryParent (installScope), keyPathUninstall.c_str(), key_access);
    return key.ReadString (regValInstallDir);
  }
}

void WriteToRegistry (InstallScope installScope, const wchar_t* guid, const wchar_t* listFileName,const wchar_t* directory)
{
  HKEY reg_parent = RegistryParent (installScope);
  const REGSAM key_access (KEY_ALL_ACCESS | KEY_WOW64_64KEY);
  {
    std::wstring keyPathUninstall (regPathUninstallInfo);
    keyPathUninstall.append (guid);
    RegistryKey key (reg_parent, keyPathUninstall.c_str(), key_access, RegistryKey::Create);
    key.WriteString (regValLogFileName, listFileName);
    key.WriteString (regValInstallDir, directory);
    key.WriteDWORD (L"SystemComponent", 1);
  }
  {
    std::wstring keyPathDependencies (regPathDependencyInfo);
    keyPathDependencies.append (guid);
    RegistryKey key (reg_parent, keyPathDependencies.c_str(), key_access, RegistryKey::Create);
  // DisplayName?
  // Version?
  }
}
