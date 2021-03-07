/*
    SevenInstall
    Copyright (c) 2013-2021 Frank Richter

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

#include "DeletionHelper.hpp"

DeletionHelper::DeletionHelper(const ArgsHelper& args)
{
  const wchar_t* inuseOptions = nullptr;
  if (args.GetOption(L"-i", inuseOptions)) {
    const auto* o = inuseOptions;
    while (*o) {
      switch (*o) {
      case 'm':
        useMoveFileEx = true;
        break;
      }
      ++o;
    }
  }
}

DWORD DeletionHelper::FileDelete(const wchar_t* file)
{
  DWORD fileAttr(GetFileAttributesW(file));
  if (fileAttr == INVALID_FILE_ATTRIBUTES) {
    // Weird.
    DWORD result(GetLastError());
    return result;
  } else {
    // Remove it
    return FileDelete(fileAttr, file);
  }
}

DWORD DeletionHelper::FileDelete(DWORD fileAttr, const wchar_t* file)
{
  if ((fileAttr & FILE_ATTRIBUTE_READONLY) != 0) {
    SetFileAttributesW(file, fileAttr & ~FILE_ATTRIBUTE_READONLY);
    // Ignore errors, assume DeleteFile() will fail anyway
  }
  if (!DeleteFileW(file)) {
    DWORD result = GetLastError();
    if((result == ERROR_ACCESS_DENIED) && useMoveFileEx) {
      // Try to remove file via MoveFileEx()
      if(MoveFileExW(file, nullptr, MOVEFILE_DELAY_UNTIL_REBOOT))
        return ERROR_SUCCESS_REBOOT_REQUIRED;
      // else: probably no admin permissions
    }
    return result;
  }
  return ERROR_SUCCESS;
}
