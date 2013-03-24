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
