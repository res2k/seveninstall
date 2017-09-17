/* Fake __unDName function */

#include <string.h>

extern "C" char* __unDName (char* outputString, const char* name, int maxLen,
                            void* (*alloc_fn)(size_t), void (*free_fn)(void*),
                            unsigned short flags)
{
  if (outputString)
  {
    strncpy_s (outputString, maxLen, name, _TRUNCATE);
    return outputString;
  }
  else
  {
    size_t new_size = strlen (name) + 1;
    auto str = reinterpret_cast<char*> (alloc_fn (new_size));
    strncpy_s (str, new_size, name, _TRUNCATE);
    return str;
  }
}
