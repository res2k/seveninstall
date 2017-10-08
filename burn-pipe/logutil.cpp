// Portions Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"

HRESULT LogErrorString (HRESULT hrError, LPCSTR szFormat, ...)
{
  HRESULT hr;
  
  va_list args;
  va_start (args, szFormat);
  hr = LogErrorStringArgs (hrError, szFormat, args);
  va_end (args);

  return hr;
}

HRESULT DAPI LogErrorStringArgs (HRESULT hrError, LPCSTR szFormat, va_list args)
{
  HRESULT hr;
  LPWSTR sczFormat = nullptr;
  LPWSTR sczMessage = nullptr;

  hr = StrAllocStringAnsi(&sczFormat, szFormat, 0, CP_ACP);
  ExitOnFailure(hr, "Failed to convert format string to wide character string");

  // format the string as a unicode string - this is necessary to be able to include
  // international characters in our output string. This does have the counterintuitive effect
  // that the caller's "%s" is interpreted differently
  // (so callers should use %hs for LPSTR and %ls for LPWSTR)
  hr = StrAllocFormattedArgs(&sczMessage, sczFormat, args);
  ExitOnFailure1(hr, "Failed to format error message: \"%ls\"", sczFormat);

  int ret = fprintf (stderr, "Error 0x%x: %ls", hrError, sczMessage);
  hr = ret < 0 ? E_FAIL : S_OK;

LExit:
  ReleaseStr(sczFormat);
  ReleaseStr(sczMessage);

  return hr;

}

HRESULT LogStringWorkRaw (LPCSTR szLogData)
{
  int ret = fprintf (stderr, "%s", szLogData);
  return ret < 0 ? E_FAIL : S_OK;
}

