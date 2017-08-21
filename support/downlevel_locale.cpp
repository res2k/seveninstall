#include <windows.h>

extern "C" LCID __cdecl __acrt_DownlevelLocaleNameToLCID (const wchar_t* locale_name)
{
  if (wcslen (locale_name) == 0)
    return LOCALE_INVARIANT;

  abort();
}

extern "C" int __cdecl __acrt_DownlevelLCIDToLocaleName (LCID lcid, wchar_t* out_locale_name, int locale_name_len)
{
  switch (lcid)
  {
  case LOCALE_INVARIANT:
  case LOCALE_SYSTEM_DEFAULT:
  case LOCALE_USER_DEFAULT:
    return 0;
  }

  abort();
}
