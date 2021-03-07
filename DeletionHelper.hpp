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

/**\file
 * Helper for file deletion. Deals with files in use.
 */
#ifndef DELETIONHELPER_HPP_
#define DELETIONHELPER_HPP_

#include "ArgsHelper.hpp"

#include <Windows.h>

class DeletionHelper
{
public:
  /// Init helper. Parses deletion options (file in use handling)
  DeletionHelper(const ArgsHelper& args);

  /**
   * Delete a file.
   * In addition to various error codes it may return ERROR_SUCCESS_REBOOT_REQUIRED,
   * indicating that MoveFileEx() with MOVEFILE_DELAY_UNTIL_REBOOT was
   * employed.
   */
  DWORD FileDelete(const wchar_t* file);
  /**
   * Delete a file. Takes attributes of file (useful if they're already known).
   * In addition to various error codes it may return ERROR_SUCCESS_REBOOT_REQUIRED,
   * indicating that MoveFileEx() with MOVEFILE_DELAY_UNTIL_REBOOT was
   * employed.
   */
  DWORD FileDelete(DWORD fileAttr, const wchar_t* file);

private:
  bool useMoveFileEx = false;

};

#endif // DELETIONHELPER_HPP_
