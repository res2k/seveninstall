#include <basetsd.h>

extern "C" SIZE_T __stdcall RtlCompareMemory (const void*, const void*, SIZE_T) { return 0; }
extern "C" void __stdcall RtlMoveMemory (void*, const void*, SIZE_T) {}
