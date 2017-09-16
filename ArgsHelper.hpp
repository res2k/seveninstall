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

/**\file
 * Helper class to deal with command line args
 */
#ifndef __ARGSHELPER_HPP__
#define __ARGSHELPER_HPP__

#include "MyUString.hpp"

#include <unordered_map>
#include <vector>

class ArgsHelper
{
    typedef std::unordered_map<MyUString, const wchar_t*> OptionsMapType;
    OptionsMapType options;
    std::vector<const wchar_t*> freeArgs;
public:
    ArgsHelper (int argc, const wchar_t* const argv[]);

    bool GetOption (const wchar_t* prefix, const wchar_t*& value) const;
    bool GetOption (const wchar_t* option) const;
    void GetFreeArgs (std::vector<const wchar_t*>& args) const;
};

#endif // __ARGSHELPER_HPP__
