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

/*
 * Alternative printf implementations.
 * Simplified to gain smaller binaries:
 * - no float support!
 * - no positional argument support
 */

#include "printf.hpp"

#include "printf_impl/CharBufferSink.hpp"
#include "printf_impl/FileSink.hpp"
#include "printf_impl/HandleSink.hpp"
#include "printf_impl/WCharBufferSink.hpp"
#include "printf_impl/parsers.hpp"
#include "printf_impl/print.hpp"
#include "printf_impl/printer.hpp"

#include <assert.h>

#define _WSTRING(str)    __WSTRING2(str)
#define __WSTRING2(str)   L ## str

#if defined(_DEBUG)
    #define _CALL_WASSERT(C, F, L)              _wassert ((C), (F), (L))
    #define _CALL_INVALID_PARAM(C, Fn, Fl, L)   _invalid_parameter ((C), (Fn), (Fl), L, 0)
#else
    #define _CALL_WASSERT(C, F, L)              (void)0
    #define _CALL_INVALID_PARAM(C, Fn, Fl, L)   _invalid_parameter_noinfo ()
#endif

#define CHECK_PARAM(condition, errno_code, return_val)                      \
    {                                                                       \
        bool condition_met = !!(condition);                                 \
        if (!condition_met)                                                 \
        {                                                                   \
            _CALL_WASSERT (_WSTRING(#condition), _WSTRING(__FILE__),        \
                __LINE__);                                                  \
            _CALL_INVALID_PARAM (_WSTRING(#condition),                      \
                _WSTRING(__FUNCTION__), _WSTRING(__FILE__), __LINE__);      \
            errno = (errno_code);                                           \
            return (return_val);                                            \
        }                                                                   \
    }

extern "C" int __stdio_common_vfprintf (uint64_t options, FILE* file, const char* format, _locale_t locale, va_list args)
{
    CHECK_PARAM(file, EINVAL, -1);
    CHECK_PARAM(format, EINVAL, -1);

    printf_impl::FileSink sink (file);
    return printf_impl::print (sink, format, args);
}

extern "C" int __stdio_common_vfwprintf (uint64_t options, FILE* file, const wchar_t* format, _locale_t locale, va_list args)
{
    CHECK_PARAM(file, EINVAL, -1);
    CHECK_PARAM(format, EINVAL, -1);

    printf_impl::FileSink sink (file);
    return printf_impl::print (sink, format, args);
}

extern "C" int __stdio_common_vsprintf_s (uint64_t options, char* buffer, size_t bufferCount, const char* format, _locale_t locale, va_list argptr)
{
    CHECK_PARAM(buffer, EINVAL, -1);
    CHECK_PARAM(format, EINVAL, -1);
    CHECK_PARAM(bufferCount >= 0, EINVAL, -1);

    printf_impl::CharBufferSink sink (buffer, bufferCount);
    int ret = printf_impl::print (sink, format, argptr);

    sink.null_terminate();
    bool buffer_too_small = sink.buffer_too_small();
    if (buffer_too_small)
    {
        if (bufferCount > 0) buffer[0] = 0;
        CHECK_PARAM(buffer_too_small, ERANGE, -1);
    }
    return ret;
}

extern "C" int __stdio_common_vsnprintf_s (uint64_t options, char* buffer, size_t sizeOfBuffer, size_t count, const char* format, _locale_t locale, va_list argptr)
{
    CHECK_PARAM(buffer, EINVAL, -1);
    CHECK_PARAM(format, EINVAL, -1);
    CHECK_PARAM((count >= 0) || (count == _TRUNCATE), EINVAL, -1);

    printf_impl::CharBufferSink sink (buffer, sizeOfBuffer);
    int ret = printf_impl::print (sink, format, argptr);

    sink.null_terminate();
    bool buffer_too_small = sink.buffer_too_small();
    if (buffer_too_small && (count != _TRUNCATE))
    {
        if (sizeOfBuffer > 0) buffer[0] = 0;
        CHECK_PARAM(buffer_too_small && (count != _TRUNCATE), ERANGE, -1);
    }
    return ret;
}

extern "C" int __stdio_common_vswprintf (uint64_t options, wchar_t* buffer, size_t bufferCount, const wchar_t* format, _locale_t locale, va_list argptr)
{
    CHECK_PARAM(buffer, EINVAL, -1);
    CHECK_PARAM(format, EINVAL, -1);
    CHECK_PARAM(bufferCount >= 0, EINVAL, -1);

    printf_impl::WCharBufferSink sink (buffer, bufferCount);
    int ret = printf_impl::print (sink, format, argptr);

    sink.null_terminate();
    return ret;
}

extern "C" int __stdio_common_vswprintf_s (uint64_t options, wchar_t* buffer, size_t bufferCount, const wchar_t* format, _locale_t locale, va_list argptr)
{
    CHECK_PARAM(buffer, EINVAL, -1);
    CHECK_PARAM(format, EINVAL, -1);
    CHECK_PARAM(bufferCount >= 0, EINVAL, -1);

    printf_impl::WCharBufferSink sink (buffer, bufferCount);
    int ret = printf_impl::print (sink, format, argptr);

    sink.null_terminate();
    bool buffer_too_small = sink.buffer_too_small();
    if (buffer_too_small)
    {
        if (bufferCount > 0) buffer[0] = 0;
        CHECK_PARAM(buffer_too_small, ERANGE, -1);
    }
    return ret;
}

extern "C" int __stdio_common_vsnwprintf_s (uint64_t options, wchar_t* buffer, size_t sizeOfBuffer, size_t count, const wchar_t* format, _locale_t locale, va_list argptr)
{
    CHECK_PARAM(buffer, EINVAL, -1);
    CHECK_PARAM(format, EINVAL, -1);
    CHECK_PARAM((count >= 0) || (count == _TRUNCATE), EINVAL, -1);

    printf_impl::WCharBufferSink sink (buffer, sizeOfBuffer);
    int ret = printf_impl::print (sink, format, argptr);

    sink.null_terminate();
    bool buffer_too_small = sink.buffer_too_small();
    if (buffer_too_small && (count != _TRUNCATE))
    {
        if (sizeOfBuffer > 0) buffer[0] = 0;
        CHECK_PARAM(buffer_too_small && (count != _TRUNCATE), ERANGE, -1);
    }
    return ret;
}

int Hwprintf (HANDLE handle, const wchar_t* format, ...)
{
    CHECK_PARAM(handle != INVALID_HANDLE_VALUE, EINVAL, -1);
    CHECK_PARAM(format, EINVAL, -1);

    printf_impl::HandleSink sink (handle);
    va_list args;
    va_start (args, format);
    int ret = printf_impl::print (sink, format, args);
    va_end (args);

    return ret;
}

int vHprintf (HANDLE handle, const wchar_t* format, va_list args)
{
    CHECK_PARAM(handle != INVALID_HANDLE_VALUE, EINVAL, -1);
    CHECK_PARAM(format, EINVAL, -1);

    printf_impl::HandleSink sink (handle);
    return printf_impl::print (sink, format, args);
}

int vHwprintf (HANDLE handle, const wchar_t* format, va_list args)
{
    CHECK_PARAM(handle != INVALID_HANDLE_VALUE, EINVAL, -1);
    CHECK_PARAM(format, EINVAL, -1);

    printf_impl::HandleSink sink (handle);
    return printf_impl::print (sink, format, args);
}
