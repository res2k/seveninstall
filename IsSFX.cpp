/*
    SevenInstall
    Copyright (c) 2013-2017 Frank Richter

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org>
 */

#include "IsSFX.hpp"

#include "7z.h"
#include "7zCrc.h"
#include "CpuArch.h"

const Byte k7zSignature[k7zSignatureSize] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};

// FindSignature() adapted from 7zipInstall
#define kBufSize (1 << 15)

#define kSignatureSearchLimit (1 << 22)

static bool FindSignature (HANDLE file, uint64_t& resPos)
{
  uint8_t buf[kBufSize];
  size_t numPrevBytes = 0;
  resPos = 0;

  for (;;)
  {
    size_t processed, pos;
    if (resPos > kSignatureSearchLimit)
      return false;
    processed = kBufSize - numPrevBytes;

    DWORD bytesRead = 0;
    BOOL res = ReadFile (file, buf + numPrevBytes, processed, &bytesRead, NULL);
    if (!res) return false;
    processed = bytesRead;
    
    processed += numPrevBytes;
    if (processed < k7zStartHeaderSize ||
        (processed == k7zStartHeaderSize && numPrevBytes != 0))
      return false;
    processed -= k7zStartHeaderSize;
    for (pos = 0; pos <= processed; pos++)
    {
      for (; pos <= processed && buf[pos] != '7'; pos++);
      if (pos > processed)
        break;
      if (memcmp (buf + pos, k7zSignature, k7zSignatureSize) == 0)
        if (CrcCalc (buf + pos + 12, 20) == GetUi32(buf + pos + 8))
        {
          resPos += pos;
          return true;
        }
    }
    resPos += processed;
    numPrevBytes = k7zStartHeaderSize;
    memmove (buf, buf + processed, k7zStartHeaderSize);
  }
}

static MyUString cached_exe_path;
static bool cached_sfx_result;

static MyUString GetExePath ()
{
  MyUString result;

  wchar_t buf[MAX_PATH];
  DWORD bufSize = sizeof(buf)/sizeof(buf[0]);
  auto needSize = GetModuleFileNameW (nullptr, buf, bufSize);
  if (needSize+1 > bufSize)
  {
    bufSize = needSize+1;
    GetModuleFileNameW (nullptr, result.GetBuf (needSize), bufSize);
    result.ReleaseBuf_CalcLen (bufSize);
  }
  else
  {
    result = buf;
  }
  return result;
}

bool IsSFX (MyUString& exePath)
{
  auto this_exe_path = GetExePath();

  if (this_exe_path != cached_exe_path)
  {
    bool sfx_res = false;
    HANDLE file = CreateFileW (this_exe_path.Ptr(), GENERIC_READ, FILE_SHARE_READ,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE)
    {
      uint64_t sig_pos;
      sfx_res = FindSignature (file, sig_pos);
      CloseHandle (file);
    }

    cached_exe_path = this_exe_path;
    cached_sfx_result = sfx_res;
  }

  if (cached_sfx_result) exePath = this_exe_path;
  return cached_sfx_result;
}
