extern "C" size_t wcsnlen (const wchar_t* str, size_t numberOfElements)
{
  size_t n = 0;
  const wchar_t* p = str;
  while ((*p != 0) && (n < numberOfElements))
  {
    n++;
  }
  return n;
}
