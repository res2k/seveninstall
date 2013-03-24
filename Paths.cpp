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
  // FIXME: This is probably too much trust in the GUID string.
  logFilePath.append (guid);
  logFilePath.append (L".txt");
  return logFilePath;
}

const wchar_t regPathUninstallInfo[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
const wchar_t regPathDependencyInfo[] = L"Software\\Classes\\Installer\\Dependencies\\";
const wchar_t regPathDependentsSubkey[] = L"\\Dependents";
const wchar_t regValLogFileName[] = L"LogFileName";
const wchar_t regValInstallDir[] = L"InstallDir";
