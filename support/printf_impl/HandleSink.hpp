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

#ifndef __SUPPORT_PRINTF_IMPL_HANDLESINK_HPP__
#define __SUPPORT_PRINTF_IMPL_HANDLESINK_HPP__

#include "Sink.hpp"

namespace printf_impl
{
    class HandleSink : public Sink
    {
        HANDLE handle;
        UINT codepage = CP_UTF8;
    public:
        HandleSink (HANDLE handle) : handle (handle)
        {
            if (GetFileType (handle) == FILE_TYPE_CHAR)
            {
                // Assume console
                codepage = GetConsoleOutputCP ();
            }
        }

        int operator()(const wchar_t* s, int n) override
        {
            // Convert to codepage
            int buf_req = WideCharToMultiByte  (codepage, 0, s, n, nullptr, 0, nullptr, nullptr);
            if (buf_req == 0) return -1;
            auto buf = static_cast<char*> (_malloca (buf_req));
            int ret = -1;
            if (WideCharToMultiByte (codepage, 0, s, n, buf, buf_req, nullptr, nullptr) != 0)
            {
                DWORD bytes_written = 0;
                if (!WriteFile (handle, buf, buf_req, &bytes_written, nullptr))
                    ret = -1;
                else
                    ret = static_cast<int> (bytes_written);
            }
            _freea (buf);
            return ret;
        }
        int operator()(const char* s, int n) override
        {
            if (n == 0) return 0;

            if (codepage == GetACP())
            {
                // No conversion necessary
                DWORD bytes_written = 0;
                if (!WriteFile (handle, s, n, &bytes_written, nullptr)) return -1;
                return static_cast<int> (bytes_written);
            }

            // Convert to wide char
            int buf_req = MultiByteToWideChar (GetACP(), 0, s, n, nullptr, 0);
            if (buf_req == 0) return -1;
            auto buf = static_cast<wchar_t*> (_malloca (buf_req * sizeof(wchar_t)));
            int ret = -1;
            if (MultiByteToWideChar (GetACP(), 0, s, n, buf, buf_req) != 0)
            {
                ret = operator()(buf, buf_req);
            }
            _freea (buf);
            return ret;
        }
        int ascii (const char* s, int n) override
        {
            if (n == 0) return 0;

            // No conversion necessary
            DWORD bytes_written = 0;
            if (!WriteFile (handle, s, n, &bytes_written, nullptr)) return -1;
            return static_cast<int> (bytes_written);
        }
    };
} // namespace printf_impl

#endif // __SUPPORT_PRINTF_IMPL_HANDLESINK_HPP__
