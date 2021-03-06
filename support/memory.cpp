#include <windows.h>

#undef RtlFillMemory
#undef RtlMoveMemory

#pragma function (memcmp)
#pragma function (memcpy)
#pragma function (memmove)
#pragma function (memset)

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

extern "C" NTSYSAPI VOID NTAPI RtlMoveMemory (VOID*, const VOID*, SIZE_T);

extern "C" void* memcpy (void* dest, const void* src, size_t num)
{
  RtlMoveMemory (dest, src, num);
  return dest;
}

extern "C" void* memmove (void* dest, const void* src, size_t num)
{
  RtlMoveMemory (dest, src, num);
  return dest;
}

extern "C" NTSYSAPI VOID NTAPI RtlFillMemory (VOID*, SIZE_T, UCHAR);

extern "C" void* memset (void* dst, int value, size_t num)
{
  RtlFillMemory (dst, num, static_cast<UCHAR> (value));
  return dst;
}
