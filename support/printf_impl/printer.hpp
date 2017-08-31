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

#ifndef __SUPPORT_PRINTF_IMPL_PRINTER_HPP__
#define __SUPPORT_PRINTF_IMPL_PRINTER_HPP__

#include <algorithm>
#include <cstdarg>
#include <cstdint>

namespace printf_impl
{
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
} // namespace printf_impl

#endif // __SUPPORT_PRINTF_IMPL_PRINTER_HPP__
