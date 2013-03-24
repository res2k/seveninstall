#include "Error.hpp"

namespace
{
  template<typename T>
  struct LocalBuffer
  {
    T* p;

    LocalBuffer() : p (nullptr) {}
    ~LocalBuffer()
    {
      if (p) LocalFree (p);
    }
  };
}

std::wstring GetErrorString (DWORD error)
{
  wchar_t buf[256];
  DWORD n = FormatMessageW (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            nullptr, error, 0, buf, sizeof(buf)/sizeof(buf[0]), nullptr);
  if (n == 0)
  {
    DWORD err (GetLastError());
    if (err == ERROR_MORE_DATA)
    {
      LocalBuffer<wchar_t> newBuffer;
      n = FormatMessageW (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                          nullptr, error, 0, (LPWSTR)&(newBuffer.p), 0, nullptr);
      if (n != 0)
      {
        return newBuffer.p;
      }
    }
  }
  if (n == 0)
    _snwprintf_s (buf, _TRUNCATE, L"0x%lx", long (error));
  return buf;
}

std::wstring GetHRESULTString (HRESULT hr)
{
  if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
    return GetErrorString (HRESULT_CODE(hr));
  wchar_t buf[16];
  _snwprintf_s (buf, _TRUNCATE, L"0x%lx", long (hr));
  return buf;
}
