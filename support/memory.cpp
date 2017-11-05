#include <windows.h>

#pragma function (memcmp)

/* Default memory functions implemented using NT runtime functions.
 * Using the NT runtime functions reduces the implementation footprint. */

extern "C" int memcmp (const void* ptr1, const void* ptr2, size_t num)
{
  size_t equal_size = RtlCompareMemory (ptr1, ptr2, num);
  if (equal_size == num) return 0;
  unsigned char c1 = reinterpret_cast<const unsigned char*> (ptr1)[equal_size];
  unsigned char c2 = reinterpret_cast<const unsigned char*> (ptr2)[equal_size];
  return c1 < c2 ? -1 : 1;
}
