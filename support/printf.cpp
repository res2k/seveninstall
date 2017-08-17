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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include <algorithm>
#include <cstdint>
#include <utility>

namespace printf_impl
{
    struct Sink
    {
        virtual ~Sink () {}

        virtual int operator() (const wchar_t* s, int n) = 0;
        virtual int operator() (const char* s, int n) = 0;
        virtual int ascii (const char* s, int n) = 0;
    };

    enum FormatFlags
    {
        ffLeftAlign = 1,
        ffAlwaysSign = 2,
        ffZeroPad = 4,
        ffBlankSign = 8,
        ffNumericalOpt = 16
    };

    template<typename FormatCh>
    static unsigned int parse_format_flags (const FormatCh*& p)
    {
        unsigned int format_flags = 0;
        bool break_check = false;
        while (*p && !break_check)
        {
            switch (*p)
            {
            case '-':
                // Left align
                format_flags |= ffLeftAlign;
                ++p;
                break;
            case '+':
                // Always show sign
                format_flags |= ffAlwaysSign;
                ++p;
                break;
            case '0':
                // Zero pad
                format_flags |= ffZeroPad;
                ++p;
                break;
            case ' ':
                // Blank instead of positive sign
                format_flags |= ffBlankSign;
                ++p;
                break;
            case '#':
                // Numerical output option
                format_flags |= ffNumericalOpt;
                ++p;
                break;
            default:
                break_check = true;
                break;
            }
        }
        return format_flags;
    }

    // Flags, Width, Precision
    struct FormatFWP
    {
        unsigned int flags = 0;
        int width = -1;
        int precision = -1;
    };

    template<typename FormatCh>
    static int parse_int (const FormatCh*& p)
    {
        int int_val = -1;
        while ((*p >= '0') && (*p <= '9'))
        {
            int v = *p - '0';
            if (int_val < 0)
                int_val = v;
            else
                int_val = int_val*10 + v;
            ++p;
        }

        return int_val;
    }

    enum SizeSpec
    {
        ssDefault = 0,
        ssChar,
        ssShort,
        ssI32,
        ssI64,
        ssIntMax,
        ssLongDouble,
        ssLong,
        ssLongLong,
        ssPtrDiff,
        ssSizeT,
        ssWide
    };

    template<typename FormatCh>
    static SizeSpec parse_size_specifier (const FormatCh*& p)
    {
        SizeSpec spec = ssDefault;
        switch (*p)
        {
        case 'h':
            {
                spec = ssShort;
                ++p;
                if (*p == 'h')
                {
                    spec = ssChar;
                    ++p;
                }
            }
            break;
        case 'I':
          {
              spec = ssIntMax;
              ++p;
              if ((*p == '6') && (*(p+1) == '4'))
              {
                  spec = ssI64;
                  p += 2;
                  break;
              }
              else if ((*p == '3') && (*(p+1) == '2'))
              {
                  spec = ssI32;
                  p += 2;
                  break;
              }
          }
        case 'j':
            spec = ssIntMax;
            ++p;
            break;
        case 'l':
            {
                spec = ssLong;
                ++p;
                if (*p == 'h')
                {
                    spec = ssLongLong;
                    ++p;
                }
            }
            break;
        case 'L':
            spec = ssLongDouble;
            ++p;
            break;
        case 't':
            spec = ssPtrDiff;
            ++p;
            break;
        case 'z':
            spec = ssSizeT;
            ++p;
            break;
        case 'w':
            spec = ssWide;
            ++p;
            break;
        }
        return spec;
    }

    namespace printer
    {
        class Char
        {
            SizeSpec size;
            int ch_value;
        public:
            Char (const FormatFWP& /*fwp*/, va_list& args, SizeSpec size)
            {
                switch (size)
                {
                case ssShort:
                    ch_value = va_arg (args, char);
                    break;
                case ssWide:
                case ssLong:
                    ch_value = va_arg (args, wchar_t);
                    break;
                }
            }

            int out_len() { return 1; }

            int print (Sink& sink)
            {
                switch (size)
                {
                case ssShort:
                    {
                        auto ch = static_cast<char> (ch_value);
                        return sink (&ch, 1);
                    }
                    break;
                case ssWide:
                case ssLong:
                    {
                        auto ch = static_cast<wchar_t> (ch_value);
                        return sink (&ch, 1);
                    }
                    break;
                }
                return -1;
            }
        };

        class Int
        {
        public:
            enum Flags
            {
                Unsigned = 1,
                Uppercase = 2
            };
        private:
            static const int max_digits_len =
                (((sizeof(uintmax_t)*8)+2) / 3) // digits (enough for octal)
                + 1; // Null terminator
            char buf[max_digits_len];
            const char* digits_ptr = nullptr;
            int digits_len = 0;
            int zero_pad = 0;
            char prefix[2]; // Sign or prefix
            int prefix_len = 0;

            static uintmax_t fetch_unsigned (va_list& args, SizeSpec size)
            {
                switch (size)
                {
                case ssDefault:
                    return va_arg (args, unsigned int);
                case ssChar:
                    return va_arg (args, unsigned char);
                case ssShort:
                    return va_arg (args, unsigned short);
                case ssI32:
                    return va_arg (args, uint32_t);
                case ssI64:
                    return va_arg (args, uint64_t);
                case ssIntMax:
                    return va_arg (args, uintmax_t);
                case ssLong:
                    return va_arg (args, unsigned long);
                case ssLongLong:
                    return va_arg (args, unsigned long long);
                case ssPtrDiff:
                    return va_arg (args, ptrdiff_t);
                case ssSizeT:
                    return va_arg (args, size_t);
                }
                return 0;
            }

            void convert_unsigned (uintmax_t value, const FormatFWP& fwp, int base, unsigned int flags)
            {
                int pad_width = fwp.precision;
                if ((fwp.flags & ffZeroPad) != 0) pad_width = std::max (pad_width, fwp.width);
                char* out_ptr = buf + max_digits_len-1;
                *out_ptr-- = 0;
                if (value == 0)
                {
                    *out_ptr = '0';
                    if (pad_width > 1) zero_pad = pad_width-1;
                }
                else
                {
                    while ((value != 0) && (digits_len < max_digits_len))
                    {
                        auto new_value = value / base;
                        char digit_val = static_cast<char> (value % base);
                        char digit_char;
                        if (digit_val >= 10)
                        {
                            digit_char = (digit_val - 10) + (((flags & Uppercase) != 0) ? 'A' : 'a');
                        }
                        else
                        {
                            digit_char = digit_val + '0';
                        }
                        *out_ptr-- = digit_char;
                        digits_len++;
                        value = new_value;
                    }
                    if ((fwp.flags & ffNumericalOpt) != 0)
                    {
                        // FIXME?: Pass in as arg?
                        if (base == 8)
                        {
                            prefix[0] = '0';
                            prefix_len = 1;
                        }
                        else if (base == 16)
                        {
                            prefix[0] = '0';
                            prefix[1] = ((flags & Uppercase) != 0) ? 'X' : 'x';
                            prefix_len = 2;
                        }
                    }
                }
                digits_ptr = out_ptr;
            }

            static intmax_t fetch_signed (va_list& args, SizeSpec size)
            {
                switch (size)
                {
                case ssDefault:
                    return va_arg (args, int);
                case ssChar:
                    return va_arg (args, char);
                case ssShort:
                    return va_arg (args, short);
                case ssI32:
                    return va_arg (args, int32_t);
                case ssI64:
                    return va_arg (args, int64_t);
                case ssIntMax:
                    return va_arg (args, intmax_t);
                case ssLong:
                    return va_arg (args, long);
                case ssLongLong:
                    return va_arg (args, long long);
                case ssPtrDiff:
                    return va_arg (args, ptrdiff_t);
                case ssSizeT:
                    return va_arg (args, size_t);
                }
                return 0;
            }

            void convert_signed (intmax_t value, const FormatFWP& fwp, int base, unsigned int flags)
            {
                int pad_width = fwp.precision;
                if ((fwp.flags & ffZeroPad) != 0) pad_width = std::max (pad_width, fwp.width);
                char* out_ptr = buf + max_digits_len-1;
                *out_ptr-- = 0;
                if (value == 0)
                {
                    *out_ptr = '0';
                    if (pad_width > 1) zero_pad = pad_width-1;
                }
                else
                {
                    bool negative = false;
                    uintmax_t abs_value;
                    if (value >= 0)
                    {
                        abs_value = value;
                        abs_value = static_cast<uintmax_t> (value);
                    }
                    else
                    {
                        negative = true;
                        abs_value = static_cast<uintmax_t> (-value);
                    }
                    while ((abs_value != 0) && (digits_len < max_digits_len))
                    {
                        auto new_value = abs_value / base;
                        char digit_val = static_cast<char> (abs_value % base);
                        char digit_char;
                        if (digit_val >= 10)
                        {
                            digit_char = (digit_val - 10) + (((flags & Uppercase) != 0) ? 'A' : 'a');
                        }
                        else
                        {
                            digit_char = digit_val + '0';
                        }
                        *out_ptr-- = digit_char;
                        digits_len++;
                        abs_value = new_value;
                    }
                    if (negative)
                    {
                        prefix[0] = '-';
                        prefix_len = 1;
                    }
                    else if ((fwp.flags & ffAlwaysSign) != 0)
                    {
                        prefix[0] = '+';
                        prefix_len = 1;
                    }
                    else if ((fwp.flags & ffBlankSign) != 0)
                    {
                        prefix[0] = ' ';
                        prefix_len = 1;
                    }
                }
                digits_ptr = out_ptr;
            }
        public:
            Int (const FormatFWP& fwp, va_list& args, SizeSpec size, int base, unsigned int flags = 0)
            {
                if (flags & Unsigned)
                {
                    auto value = fetch_unsigned (args, size);
                    convert_unsigned (value, fwp, base, flags);
                }
                else
                {
                    auto value = fetch_signed (args, size);
                    convert_signed (value, fwp, base, flags);
                }
            }

            int out_len()
            {
                return prefix_len + zero_pad + digits_len;
            }
            int print (Sink& sink)
            {
                if (!digits_ptr) return 0;
                int total = 0;
                if (prefix_len > 0)
                {
                    auto prefix_ret = sink.ascii (prefix, prefix_len);
                    if (prefix_ret < 0) return prefix_ret;
                    total += prefix_ret;
                }
                for (int i = 0; i < zero_pad; i++)
                {
                    const char zero = '0';
                    int pad_ret = sink.ascii (&zero, 1);
                    if (pad_ret < 0) return pad_ret;
                    total += pad_ret;
                }
                int print_ret = sink.ascii (digits_ptr, digits_len);
                if (print_ret < 0) return print_ret;
                total += print_ret;
                return total;
            }
        };

        class Float : public Int
        {
            const SizeSpec MapSizeSpec (SizeSpec size)
            {
                static_assert (sizeof (long double) == sizeof (uint64_t), "unsupport 'long double' size");
                static_assert (sizeof (double) == sizeof (uint64_t), "unsupport 'double' size");
                return ssI64;
            }
        public:
            Float (const FormatFWP& fwp, va_list& args, SizeSpec size)
                : Int (fwp, args, MapSizeSpec (size), 16) {}
        };

        class Pointer : public Int
        {
            const SizeSpec GetSizeSpec ()
            {
                static_assert ((sizeof(void*) == 4) || (sizeof(void*) == 8), "unsupported pointer size");
                if (sizeof(void*) == 4)
                    return ssI32;
                else
                    return ssI64;
            }
            const FormatFWP MapFWP (const FormatFWP& fwp)
            {
                FormatFWP new_fwp = fwp;
                new_fwp.flags = ffNumericalOpt;
                new_fwp.precision = sizeof(void*) * 2;
                return new_fwp;
            }
        public:
            Pointer (const FormatFWP& fwp, va_list& args)
                : Int (MapFWP (fwp), args, GetSizeSpec (), 16) {}
        };

        class String
        {
            SizeSpec size;
            const void* string_ptr = nullptr;
            int len = 0;
        public:
            String (const FormatFWP& fwp, va_list& args, SizeSpec size) : size (size)
            {
                switch (size)
                {
                case ssShort:
                    {
                        auto real_p = va_arg (args, const char*);
                        len = strlen (real_p); 
                        string_ptr = real_p;
                    }
                    break;
                case ssWide:
                case ssLong:
                    {
                        auto real_p = va_arg (args, const wchar_t*);
                        len = wcslen (real_p); 
                        string_ptr = real_p;
                    }
                    break;
                }
            }

            int out_len()
            {
                return len;
            }
            int print (Sink& sink)
            {
                if (!string_ptr) return 0;
                switch (size)
                {
                case ssShort:
                    return sink (static_cast<const char*> (string_ptr), len);
                case ssWide:
                case ssLong:
                    return sink (static_cast<const wchar_t*> (string_ptr), len);
                }
                return 0;
            }
        };
    } // namespace printer

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

    class FileSink : public Sink
    {
        FILE* file;
    public:
        FileSink (FILE* file) : file (file) {}

        int operator()(const wchar_t* s, int n) override
        {
            int p = 0;
            for (; p < n; p++) { if (putwc (s[p], file) == WEOF) break; }
            return p;
        };
        int operator()(const char* s, int n) override
        {
            int p = 0;
            for (; p < n; p++) { if (putc (s[p], file) == WEOF) break; }
            return p;
        };
        int ascii (const char* s, int n) override { return operator()(s, n); }
    };

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
