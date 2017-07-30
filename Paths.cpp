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

#include "Paths.hpp"

#include "Error.hpp"

#include <ShlObj.h>

static std::wstring GetProgramDataPath ()
{
  wchar_t pathBuf[MAX_PATH];
  CHECK_HR (SHGetFolderPath (0, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, 0, SHGFP_TYPE_CURRENT, pathBuf));
  return pathBuf;
}

static void EnsureDirectoriesExist (const wchar_t* fullPath)
{
  size_t path_len (wcslen (fullPath));
  if ((path_len == 0) || ((path_len == 2) && (fullPath[1] == ':')))
  {
    return;
  }

  {
    const wchar_t* lastComp (wcsrchr (fullPath, '\\'));
    if (lastComp)
    {
      std::wstring parentPath (fullPath, lastComp-fullPath);
      EnsureDirectoriesExist(parentPath.c_str());
    }
  }

  if (!CreateDirectoryW (fullPath, nullptr))
  {
    DWORD err (GetLastError());
    if (err != ERROR_ALREADY_EXISTS)
      throw HRESULTException (HRESULT_FROM_WIN32(err));
  }
}

/// Enable file system compression on the given path
static void SetCompression (const wchar_t* path)
{
  HANDLE hFile (CreateFileW (path,  GENERIC_READ | GENERIC_WRITE,
			      FILE_SHARE_READ | FILE_SHARE_WRITE,
			      nullptr, OPEN_EXISTING,
			      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL));
  if (hFile == INVALID_HANDLE_VALUE)
  {
    // Ignore errors
    return;
  }
  USHORT compressionState (COMPRESSION_FORMAT_DEFAULT);
  DWORD bytesReturned;
  DeviceIoControl (hFile, FSCTL_SET_COMPRESSION, &compressionState, sizeof (compressionState),
		    nullptr, 0, &bytesReturned, nullptr);
  // Again, don't really care if it succeeded or not.
  CloseHandle (hFile);
}

std::wstring InstallLogFile (const wchar_t* guid)
{
  std::wstring logFilePath (GetProgramDataPath ());
  logFilePath.append (L"\\SevenInstall");
  EnsureDirectoriesExist (logFilePath.c_str());
  SetCompression (logFilePath.c_str());
  logFilePath.append (L"\\");
  // We trust the GUID string since it has supposedly passed VerifyGUID() earlier.
  logFilePath.append (guid);
  logFilePath.append (L".txt");
  return logFilePath;
}

const wchar_t regPathUninstallInfo[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
const wchar_t regPathDependencyInfo[] = L"Software\\Classes\\Installer\\Dependencies\\";
const wchar_t regPathDependentsSubkey[] = L"\\Dependents";
const wchar_t regValLogFileName[] = L"LogFileName";
const wchar_t regValInstallDir[] = L"InstallDir";
