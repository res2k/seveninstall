extern "C" size_t strnlen (const char* str, size_t numberOfElements)
{
  size_t n = 0;
  const char* p = str;
  while ((*p != 0) && (n < numberOfElements))
  {
    p++;
    n++;
  }
  return n;
}

extern "C" size_t wcslen (const wchar_t* str)
{
  size_t n = 0;
  const wchar_t* p = str;
  while (*p != 0)
  {
    p++;
    n++;
  }
  return n;
}

extern "C" size_t wcsnlen (const wchar_t* str, size_t numberOfElements)
{
  size_t n = 0;
  const wchar_t* p = str;
  while ((*p != 0) && (n < numberOfElements))
  {
    p++;
    n++;
  }
  return n;
}
