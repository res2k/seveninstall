#include <Windows.h>

extern "C" int wcsncmp (const wchar_t* a, const wchar_t* b, size_t n)
{
  return CompareStringOrdinal  (a, n, b, n, false) - 2;
}

extern "C" int _wcsnicmp (const wchar_t* a, const wchar_t* b, size_t n)
{
  return CompareStringOrdinal  (a, n, b, n, true) - 2;
}
