// Locale-related dummies, as we won't call setlocale()

#include <assert.h>
#include <locale.h>
#include <string.h>

extern "C" long __acrt_locale_changed_data = 0;

extern "C" void __acrt_set_locale_changed ()
{
  __acrt_locale_changed_data = 1;
}

extern "C" int _configthreadlocale (int i)
{
  assert (i == 0);
  return _DISABLE_PER_THREAD_LOCALE;
}

extern "C" wchar_t* __acrt_copy_locale_name (const wchar_t* name)
{
  return _wcsdup (name);
}
