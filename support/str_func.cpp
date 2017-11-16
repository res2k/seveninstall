#include <Shlwapi.h>

extern "C" const char* strchr (const char* str, int c)
{
  return StrChrA (str, c);
}

extern "C" const char* strrchr (const char* str, int c)
{
  return StrRChrA (str, nullptr, c);
}

extern "C" const char* strstr (const char* str, const char* strSearch)
{
  return StrStrA (str, strSearch);
}

extern "C" const wchar_t* wcschr (const wchar_t* str, wchar_t c)
{
  return StrChrW (str, c);
}

extern "C" const wchar_t* wcsrchr (const wchar_t* str, wchar_t c)
{
  return StrRChrW (str, nullptr, c);
}

extern "C" const wchar_t* wcsstr (const wchar_t* str, const wchar_t* strSearch)
{
  return StrStrW (str, strSearch);
}
