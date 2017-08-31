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

#ifndef __SUPPORT_PRINTF_IMPL_WCHARBUFFERSINK_HPP__
#define __SUPPORT_PRINTF_IMPL_WCHARBUFFERSINK_HPP__

#include "Sink.hpp"

#include <stdio.h>
#include <wchar.h>

#include <algorithm>

namespace printf_impl
{
    struct WCharBufferSink : public Sink
    {
        wchar_t* buf_p;
        wchar_t* buf_end;
        bool overflow = false;
    public:
        WCharBufferSink (wchar_t* buffer, size_t buffer_size)
            : buf_p (buffer), buf_end (buffer + buffer_size) {}

        int operator()(const wchar_t* s, int n) override
        {
            auto buf_remaining = buf_end - buf_p;
            auto copy_size = std::min (static_cast<ptrdiff_t> (n), buf_remaining - 1);
            if (copy_size > 0)
            {
                wmemcpy (buf_p, s, copy_size);
                buf_p += copy_size;
            }
            overflow |= copy_size < n;
            return (copy_size >= n) ? copy_size : -1;
        }

        int operator()(const char* s, int n) override
        {
            const char* input = s;
            const char* input_end = s + n;
            mbstate_t mbs = mbstate_t ();
            int num_written = 0;

            while ((input < input_end) && (buf_p + 1 < buf_end))
            {
                size_t conv_res = mbrtowc (buf_p, input, input_end - input, &mbs);
                switch (conv_res)
                {
                default:
                    // Input was consumed, output generated
                    buf_p++;
                    input += conv_res;
                    num_written++;
                    break;
                case 0:
                    // Null terminator encountered
                    input = input_end;
                    break;
                case (size_t)-1:
                    // Encoding error
                    *buf_p++ = '?';
                    input++;
                    mbs = mbstate_t ();
                    num_written++;
                    break;
                case (size_t)-2:
                    // Input incomplete
                    *buf_p++ = '?';
                    input = input_end;
                    num_written++;
                    break;
                case (size_t)-3:
                    // No input consumed, but output generated
                    buf_p++;
                    num_written++;
                    break;
                }
            }
            overflow |= (buf_p + 1 == buf_end);
            auto num_converted = input - s;
            return (num_converted >= n) ? num_written : -1;
        }

        int ascii (const char* s, int n) override
        {
            auto buf_remaining = buf_end - buf_p;
            auto copy_size = std::min (static_cast<ptrdiff_t> (n), buf_remaining - 1);
            if (copy_size > 0)
            {
                for (int i = 0; i < copy_size; i++)
                {
                    *buf_p++ = s[i];
                }
            }
            overflow |= copy_size < n;
            return (copy_size >= n) ? copy_size : -1;
        }

        void null_terminate()
        {
            if (buf_p < buf_end) *buf_p++ = 0;
        }
        bool buffer_too_small () const { return overflow; }
    };
} // namespace printf_impl

#endif // __SUPPORT_PRINTF_IMPL_WCHARBUFFERSINK_HPP__
