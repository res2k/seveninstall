#include "GUID.hpp"

#include <Windows.h>

bool VerifyGUID (const wchar_t* guid)
{
  /* Allowed in GUIDs are:
   * - alphanumeric ASCII characters
   * - anything in guidAllowedSpecial
   */
  const wchar_t guidAllowedSpecial[] = L"{}()[]_-.,;$%!#~=";
  wchar_t ch;
  while ((ch = *guid++) != 0)
  {
    if (ch >= 128) return false;
    if (!IsCharAlphaNumericW (ch)) return false;
    if (!wcschr (guidAllowedSpecial, ch)) return false;
  }
  return true;
}