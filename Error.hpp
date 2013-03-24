/**\file
 * Error handling helpers
 */

#ifndef __7I_ERROR_HPP__
#define __7I_ERROR_HPP__

#include <exception>
#include <string>

#include <Windows.h>

class HRESULTException : public std::exception
{
  HRESULT hr;
public:
  // TODO?: Track file & line?

  HRESULTException (HRESULT hr) : hr (hr) {}
  HRESULT GetHR() const { return hr; }
};

#define THROW_HR(hr)      { throw HRESULTException (hr); }
#define CHECK_HR(expr)    { HRESULT _hr_ ((expr)); if (!SUCCEEDED(_hr_)) THROW_HR (_hr_); }

/// Format a Windows error code
std::wstring GetErrorString (DWORD error);
/// Format a HRESULT
std::wstring GetHRESULTString (HRESULT hr);

#endif // __7I_ERROR_HPP__
