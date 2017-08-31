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

#ifndef __SUPPORT_PRINTF_IMPL_PRINT_HPP__
#define __SUPPORT_PRINTF_IMPL_PRINT_HPP__

#include "printf_impl/Sink.hpp"

namespace printf_impl
{
    template<typename Printer, typename... Arg>
    static int print (Sink& sink, const FormatFWP& fwp, va_list& args, Arg... printer_arg)
    {
        Printer printer (fwp, args, std::forward<Arg> (printer_arg)...);

        int return_val = 0;
        int out_width = printer.out_len();
        bool need_pad = (out_width < fwp.width);
        bool left_pad = need_pad & ((fwp.flags & ffLeftAlign) == 0);
        bool right_pad = need_pad & ((fwp.flags & ffLeftAlign) != 0);
        char pad_char = ((fwp.flags & (ffZeroPad | ffLeftAlign)) == ffZeroPad) ? '0' : ' ';
        int pad_num = fwp.width - out_width;
        if (left_pad)
        {
            for (int i = 0; i < pad_num; i++)
            {
                int pad_ret = sink.ascii (&pad_char, 1);
                if (pad_ret < 0) return pad_ret;
                return_val += pad_ret;
            }
        }

        int print_ret = printer.print (sink);
        if (print_ret < 0) return print_ret;
        return_val += print_ret;

        if (right_pad)
        {
            for (int i = 0; i < pad_num; i++)
            {
                int pad_ret = sink.ascii (&pad_char, 1);
                if (pad_ret < 0) return pad_ret;
                return_val += pad_ret;
            }
        }
        return return_val;
    }

    template<typename Ch> struct CharSizes;

    template<> struct CharSizes<char>
    {
        static const SizeSpec default_spec = ssShort;
        static const SizeSpec other_spec = ssLong;
    };
    template<> struct CharSizes<wchar_t>
    {
        static const SizeSpec default_spec = ssLong;
        static const SizeSpec other_spec = ssShort;
    };

    template<typename FormatCh>
    static int handle_format_specifier (Sink& sink, const FormatCh*& p, va_list& args)
    {
        const auto pct_ch = static_cast<FormatCh> ('%');
        if ((*p == 0) || (*p == pct_ch))
        {
            ++p;
            return sink (&pct_ch, 1);
        }

        // Flags, Width, Precision
        FormatFWP format_fwp;
        format_fwp.flags = parse_format_flags (p);
        // Width
        format_fwp.width = parse_int (p);
        // Precision
        if (*p == '.')
        {
            ++p;
            format_fwp.precision = parse_int (p);
        }
        // Size specifier
        SizeSpec size_spec = parse_size_specifier (p);
        // Type specifier
        switch (*p)
        {
        case 'c':
        case 'C':
            {
                SizeSpec effective_size = size_spec;
                if (effective_size == ssDefault)
                {
                    if (*p == 'c')
                        effective_size = CharSizes<FormatCh>::default_spec;
                    else
                        effective_size = CharSizes<FormatCh>::other_spec;
                }
                ++p;
                return print<printer::Char> (sink, format_fwp, args, effective_size);
            }
        case 'd':
        case 'i':
            ++p;
            return print<printer::Int> (sink, format_fwp, args, size_spec, 10);
        case 'o':
            ++p;
            return print<printer::Int> (sink, format_fwp, args, size_spec, 8, printer::Int::Unsigned);
        case 'u':
            ++p;
            return print<printer::Int> (sink, format_fwp, args, size_spec, 10, printer::Int::Unsigned);
        case 'x':
            ++p;
            return print<printer::Int> (sink, format_fwp, args, size_spec, 16, printer::Int::Unsigned);
        case 'X':
            ++p;
            return print<printer::Int> (sink, format_fwp, args, size_spec, 16, printer::Int::Unsigned | printer::Int::Uppercase);
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
        case 'a':
        case 'A':
            ++p;
            return print<printer::Float> (sink, format_fwp, args, size_spec);
        case 'p':
            ++p;
            return print<printer::Pointer> (sink, format_fwp, args);
        case 's':
        case 'S':
            {
                SizeSpec effective_size = size_spec;
                if (effective_size == ssDefault)
                {
                    if (*p == 's')
                        effective_size = CharSizes<FormatCh>::default_spec;
                    else
                        effective_size = CharSizes<FormatCh>::other_spec;
                }
                ++p;
                return print<printer::String> (sink, format_fwp, args, effective_size);
            }
        }
        ++p;
        return 0;
    }

    template<typename FormatCh>
    static int print (Sink& sink, const FormatCh* format, va_list args)
    {
        int total_out = 0;
        const FormatCh* p = format;
        const FormatCh* span_begin = format;
        int span_len = 0;

        auto flush_span =
            [&]()
            {
                if (span_len > 0)
                {
                    auto sink_ret = sink (span_begin, span_len);
                    span_begin = p;
                    span_len = 0;
                    return sink_ret;
                }
                return 0;
            };

        while (*p != 0)
        {
            if (*p == '%')
            {
                {
                    auto flush_ret = flush_span ();
                    if (flush_ret < 0) return flush_ret;
                    total_out += flush_ret;
                }
                ++p;
                auto fmt_ret = handle_format_specifier (sink, p, args);
                if (fmt_ret < 0) return fmt_ret;
                total_out += fmt_ret;
                span_begin = p;
            }
            else
            {
                ++span_len;
                ++p;
            }
        }

        {
            auto flush_ret = flush_span ();
            if (flush_ret < 0) return flush_ret;
            total_out += flush_ret;
        }
        return total_out;
    }
} // namespace printf_impl

#endif // __SUPPORT_PRINTF_IMPL_PRINT_HPP__
