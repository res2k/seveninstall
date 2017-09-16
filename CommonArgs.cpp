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

bool CommonArgs::GetGUID (const wchar_t*& guid, bool reportMissing) const
{
  bool result (args.GetOption (L"-g", guid) && (wcslen (guid) != 0));
  if (!result && reportMissing)
  {
    printf ("'-g<GUID>' argument is required\n");
  }
  if (result && !VerifyGUID (guid))
  {
    printf ("Not an allowed GUID: '%ls'\n", guid);
    return false;
  }
  return result;
}

std::optional<InstallScope> CommonArgs::GetInstallScope ()
{
  if (args.GetOption (L"-U"))
    return InstallScope::User;
  else if (args.GetOption (L"-M"))
    return InstallScope::Machine;
  else
    return std::nullopt;
}
