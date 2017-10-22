#include "MulDiv64.hpp"

#include "stdint.h"

#include <windows.h>
#include <winnt.h>

#if !defined(UnsignedMultiply128)

template<typename T>
static bool add_overflowed (T& dest, const T src)
{
  T old_dest = dest;
  dest += src;
  return dest < old_dest;
}

// Only provided on 64 bit; Roll our own version otherwise
static uint64_t UnsignedMultiply128 (uint64_t multiplier, uint64_t multiplicand, uint64_t* result_high)
{
  union uint64_to_uint32
  {
    uint64_t u64;
    struct
    {
      uint32_t lo, hi;
    };
  };
  uint64_to_uint32 a, b;
  uint64_to_uint32 result_lo, result_hi;

  a.u64 = multiplier;
  b.u64 = multiplicand;

  /* Compute result:
     C = 2^32
     (a.hi*C + a.lo) * (b.hi*C + b.lo)
     = (a.hi*b.hi*C^2) + (a.hi*b.lo*C) + (a.lo*b.hi*C) + (a.lo*b.lo)
  */
  result_hi.u64 = UInt32x32To64 (a.hi, b.hi); // (a.hi*b.hi*C^2)
  result_lo.u64 = UInt32x32To64 (a.lo, b.lo); // (a.lo*b.lo)

  // (a.hi*b.lo*C)
  uint64_to_uint32 mid1;
  mid1.u64 = UInt32x32To64 (a.hi, b.lo);
  result_hi.u64 += mid1.hi;
  if (add_overflowed (result_lo.hi, mid1.lo)) ++result_hi.u64;

  // (a.lo*b.hi*C)
  uint64_to_uint32 mid2;
  mid2.u64 = UInt32x32To64 (a.lo, b.hi);
  result_hi.u64 += mid2.hi;
  if (add_overflowed (result_lo.hi, mid2.lo)) ++result_hi.u64;

  *result_high = result_hi.u64;
  return result_lo.u64;
}
#endif // !defined(UnsignedMultiply128)

namespace
{
  /* The code in this namespace is based off code from the
   * "Hacker's Delight" web page:
   * http://www.hackersdelight.org/hdcodetxt/divlu.c.txt */
  int nlz (uint64_t x) {
  #if defined(BitScanReverse64)
    unsigned long idx;
    return BitScanReverse64 (&idx, x) != 0 ? (63 - idx) : 64;
  #else
    unsigned long index;
    union
    {
      unsigned long long ull;
      unsigned long ul[2];
    } u;
    u.ull = x;
    // Scanning from left, check high dword first
    if (BitScanReverse (&index, u.ul[1])) return 31 - index;
    return BitScanReverse (&index, u.ul[0]) != 0 ? (63 - index) : 64;
  #endif

  }

  uint64_t divlu (uint64_t u1, uint64_t u0, uint64_t v)
  {
    const uint64_t b = UINT64_C(0x100000000); // Number base (32 bits).
    uint32_t vn1, vn0;                        // Norm. divisor digits.
    uint64_t un32, un21, un10;                // Dividend digit pairs.
    uint64_t q1, q0;                          // Quotient.
    uint64_t un1, un0;                        // Dividend as fullwords.
    uint64_t rhat;                            // A remainder.
    int s;

    if (u1 >= v) {                            // If overflow, return the largest
      return UINT64_MAX;                      // possible quotient.
    }

    s = nlz(v);                               // 0 <= s <= 63.
    v = v << s;                               // Normalize divisor.
    vn1 = v >> 32;                            // Break divisor up into
    vn0 = static_cast<uint32_t> (v);          // two 32-bit halves.


    un32 = (u1 << s) | (u0 >> (64 - s)) & (int64_t (-s) >> 63);
    un10 = u0 << s;                           // Shift dividend left.

    un1 = un10 >> 32;                         // Break right half of
    un0 = static_cast<uint32_t> (un10);       // dividend into two digits.

    q1 = un32/vn1;                            // Compute the first
    rhat = un32 - q1*vn1;                     // quotient digit, q1.
    while (q1 >= b || q1*vn0 > b*rhat + un1) {
      q1 = q1 - 1;
      rhat = rhat + vn1;
      if (rhat >= b) break;
    }

    un21 = un32*b + un1 - q1*v;               // Multiply and subtract.

    q0 = un21/vn1;                            // Compute the second
    rhat = un21 - q0*vn1;                     // quotient digit, q0.
    while (q0 >= b || q0*vn0 > b*rhat + un0) {
      q0 = q0 - 1;
      rhat = rhat + vn1;
      if (rhat >= b) break;
    }

    return q1*b + q0;
  }
}

// Compute: (a*b)/c. Returns UINT64_MAX in case of overflow
uint64_t MulDiv64 (uint64_t a, uint64_t b, uint64_t c)
{
  // Compute directly if only limited precision is required
  if ((a <= UINT32_MAX) && (b <= UINT32_MAX))
  {
    return (a * b) / c;
  }

  uint64_t prod_lo, prod_hi;
  prod_lo = UnsignedMultiply128 (a, b, &prod_hi);

  // And divide.
  return divlu (prod_hi, prod_lo, c);
}
