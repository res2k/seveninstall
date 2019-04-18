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

#ifndef __SUPPORT_PRINTF_IMPL_PARSERS_HPP__
#define __SUPPORT_PRINTF_IMPL_PARSERS_HPP__

namespace printf_impl
{
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
                if (*p == 'l')
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
} // namespace printf_impl

#endif // __SUPPORT_PRINTF_IMPL_PARSERS_HPP__
