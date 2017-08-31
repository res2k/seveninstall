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

#ifndef __SUPPORT_PRINTF_IMPL_CHARBUFFERSINK_HPP__
#define __SUPPORT_PRINTF_IMPL_CHARBUFFERSINK_HPP__

#include "Sink.hpp"

#include <stdio.h>
#include <wchar.h>

#include <algorithm>

namespace printf_impl
{
    struct CharBufferSink : public Sink
    {
        char* buf_p;
        char* buf_end;
        bool overflow = false;
    public:
        CharBufferSink (char* buffer, size_t buffer_size)
            : buf_p (buffer), buf_end (buffer + buffer_size) {}

        int operator()(const wchar_t* s, int n) override
        {
            const wchar_t* input = s;
            const wchar_t* input_end = s + n;
            mbstate_t mbs = mbstate_t ();
            int num_written = 0;

            while ((input < input_end) && (buf_p + 1 < buf_end))
            {
                size_t conv_res;
                wcrtomb_s (&conv_res, buf_p, buf_end - (buf_p + 1), *input, &mbs);
                if (conv_res == (size_t)-1)
                {
                    // Encoding error
                    *buf_p++ = '?';
                    input++;
                    mbs = mbstate_t ();
                    num_written++;
                }
                else
                {
                    // Input was consumed, output generated
                    buf_p += conv_res;
                    input++;
                    num_written += conv_res;
                }
            }
            overflow |= (buf_p + 1 == buf_end);
            auto num_converted = input - s;
            return (num_converted >= n) ? num_written : -1;
        }

        int operator()(const char* s, int n) override
        {
            auto buf_remaining = buf_end - buf_p;
            auto copy_size = std::min (static_cast<ptrdiff_t> (n), buf_remaining - 1);
            if (copy_size > 0)
            {
                memcpy (buf_p, s, copy_size);
                buf_p += copy_size;
            }
            overflow |= copy_size < n;
            return (copy_size >= n) ? copy_size : -1;
        }

        int ascii (const char* s, int n) override { return operator() (s, n); }

        void null_terminate()
        {
            if (buf_p < buf_end) *buf_p++ = 0;
        }
        bool buffer_too_small () const { return overflow; }
    };
} // namespace printf_impl

#endif // __SUPPORT_PRINTF_IMPL_CHARBUFFERSINK_HPP__
