#include "CommonArgs.hpp"

#include "ArgsHelper.hpp"
#include "GUID.hpp"

bool CommonArgs::GetGUID (const wchar_t*& guid, bool reportMissing)
{
  bool result (args.GetOption (L"-g", guid) && (wcslen (guid) != 0));
  if (!result && reportMissing)
  {
    wprintf (L"'-g<GUID>' argument is required\n");
  }
  if (result && !VerifyGUID (guid))
  {
    wprintf (L"Not an allowed GUID: '%ls'\n", guid);
    return false;
  }
  return result;
}
