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

#include "CommonArgs.hpp"
#include "Error.hpp"

#include <assert.h>
#include <ShlObj.h>

static MyUString GetDataDir (const CommonArgs& commonArgs)
{
  if (auto fullDir = commonArgs.GetFullDataDir())
    return fullDir;

  auto installScope = commonArgs.GetInstallScope();

  MyUString path;
  {
    wchar_t pathBuf[MAX_PATH];
    const int folder = (installScope == InstallScope::User) ? CSIDL_LOCAL_APPDATA : CSIDL_COMMON_APPDATA;
    CHECK_HR (SHGetFolderPath (0, folder | CSIDL_FLAG_CREATE, 0, SHGFP_TYPE_CURRENT, pathBuf));
    path = pathBuf;
  }
  path += L"\\";
  if (auto dataDirName = commonArgs.GetDataDirName())
    path += dataDirName;
  else
    path += L"SevenInstall";
  return path;
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
      MyUString parentPath (fullPath, lastComp-fullPath);
      EnsureDirectoriesExist(parentPath.Ptr());
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

//---------------------------------------------------------------------------

bool InstallLogLocation::Init (const CommonArgs& commonArgs)
{
  assert (commonArgs.GetGUID () != nullptr);

  logsDir = GetDataDir (commonArgs);
  EnsureDirectoriesExist (logsDir.Ptr());
  SetCompression (logsDir.Ptr());
  filename = logsDir;
  filename += L"\\";
  // We trust the GUID string since it has supposedly passed VerifyGUID() earlier.
  filename += commonArgs.GetGUID();
  filename += L".txt";
  return true;
}

const MyUString& InstallLogLocation::GetLogsPath() const
{
  if (logsDir.IsEmpty()) throw HRESULTException (E_FAIL);
  return logsDir;
}

const MyUString& InstallLogLocation::GetFilename() const
{
  if (filename.IsEmpty()) throw HRESULTException (E_FAIL);
  return filename;
}

//---------------------------------------------------------------------------

void NormalizePath (MyUString& path)
{
  MyUString longPath;
  DWORD needBuf = GetLongPathName (path.Ptr(),
                                   longPath.GetBuf (path.Len()),
                                   path.Len() + 1);
  if (needBuf > path.Len() + 1)
  {
    GetLongPathName (path.Ptr(),
                     longPath.GetBuf (needBuf),
                     needBuf);
  }
  longPath.ReleaseBuf_CalcLen (needBuf);
  CharLower (longPath.Ptr());
  path = std::move (longPath);
}

MyUString GetExePath ()
{
  MyUString result;

  wchar_t buf[MAX_PATH];
  DWORD bufSize = sizeof(buf)/sizeof(buf[0]);
  auto needSize = GetModuleFileNameW (nullptr, buf, bufSize);
  if (needSize+1 > bufSize)
  {
    bufSize = needSize+1;
    GetModuleFileNameW (nullptr, result.GetBuf (needSize), bufSize);
    result.ReleaseBuf_CalcLen (bufSize);
  }
  else
  {
    result = buf;
  }
  return result;
}
