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

#include "Registry.hpp"

#include <memory>

#include "Error.hpp"

RegistryKey::CreateTag RegistryKey::Create;

RegistryKey::RegistryKey (HKEY parent, const wchar_t* subKey, REGSAM access) : key (0)
{
  LONG err = RegOpenKeyExW (parent, subKey, 0, access, &key);
  if (err != ERROR_SUCCESS)
    THROW_HR(HRESULT_FROM_WIN32(err));
}

RegistryKey::RegistryKey (HKEY parent, const wchar_t* subKey, REGSAM access, CreateTag&) : key (0)
{
  LONG err = RegCreateKeyExW (parent, subKey, 0, nullptr, 0, access, nullptr, &key, nullptr);
  if (err != ERROR_SUCCESS)
    THROW_HR(HRESULT_FROM_WIN32(err));
}

RegistryKey::~RegistryKey ()
{
  if (key) RegCloseKey (key);
}

void RegistryKey::WriteDWORD (const wchar_t* name, DWORD value)
{
  LONG err = RegSetValueExW (key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>  (&value), sizeof (value));
  if (err != ERROR_SUCCESS)
    THROW_HR(HRESULT_FROM_WIN32(err));
}

void RegistryKey::WriteString (const wchar_t* name, const wchar_t* value)
{
  LONG err = RegSetValueExW (key, name, 0, REG_SZ, reinterpret_cast<const BYTE*> (value), (wcslen (value) + 1) * sizeof(wchar_t));
  if (err != ERROR_SUCCESS)
    THROW_HR(HRESULT_FROM_WIN32(err));
}

std::wstring RegistryKey::ReadString (const wchar_t* name)
{
  wchar_t buf[256];
  std::unique_ptr<wchar_t[]> new_buf;
  DWORD dataType, dataSize;
  dataSize = sizeof (buf) - sizeof (wchar_t);
  LONG err = RegQueryValueExW (key, name, nullptr, &dataType, reinterpret_cast<BYTE*> (buf), &dataSize);
  if (dataType != REG_SZ)
    THROW_HR(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
  if (err == ERROR_SUCCESS)
  {
    buf[dataSize/sizeof(wchar_t)] = 0;
    return buf;
  }
  else if (err == ERROR_MORE_DATA)
  {
    dataSize += 1 + sizeof(wchar_t);
    new_buf.reset (new wchar_t[dataSize / sizeof(wchar_t)]);
    err = RegQueryValueExW (key, name, nullptr, &dataType, reinterpret_cast<BYTE*> (new_buf.get()), &dataSize);
  }
  CHECK_HR(HRESULT_FROM_WIN32(err));
  (new_buf.get())[dataSize/sizeof(wchar_t)] = 0;
  return new_buf.get();
}

size_t RegistryKey::NumSubkeys ()
{
  DWORD numSubKeys (0);
  LONG err (RegQueryInfoKeyW (key, nullptr, nullptr, nullptr, &numSubKeys, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));
  CHECK_HR(HRESULT_FROM_WIN32(err));
  return numSubKeys;
}

//---------------------------------------------------------------------------

AutoRootRegistryKey::AutoRootRegistryKey (const wchar_t* key, REGSAM access)
{
  try
  {
    new (&wrappedKey) RegistryKey (HKEY_LOCAL_MACHINE, key, access);
  }
  catch (const HRESULTException& e)
  {
    if ((e.GetHR() == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
        || (e.GetHR() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)))
    {
      // Try again with HKCU
      new (&wrappedKey) RegistryKey (HKEY_CURRENT_USER, key, access);
    }
    else
      throw;
  }
}

AutoRootRegistryKey::AutoRootRegistryKey (const wchar_t* key, REGSAM access, CreateTag& tag)
{
  try
  {
    new (&wrappedKey) RegistryKey (HKEY_LOCAL_MACHINE, key, access, RegistryKey::Create);
  }
  catch (const HRESULTException& e)
  {
    if (e.GetHR() == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
    {
      // Try again with HKCU
      new (&wrappedKey) RegistryKey (HKEY_CURRENT_USER, key, access, RegistryKey::Create);
    }
    else
      throw;
  }
}
