/**\file
 * Helper class to deal with command line args
 */
#ifndef __ARGSHELPER_HPP__
#define __ARGSHELPER_HPP__

#include <string>
#include <unordered_map>
#include <vector>

class ArgsHelper
{
    typedef std::unordered_map<std::wstring, const wchar_t*> OptionsMapType;
    OptionsMapType options;
    std::vector<const wchar_t*> freeArgs;
public:
    ArgsHelper (int argc, const wchar_t* const argv[]);

    bool GetOption (const wchar_t* prefix, const wchar_t*& value) const;
    bool GetOption (const wchar_t* option) const;
    void GetFreeArgs (std::vector<const wchar_t*>& args) const;
};

#endif // __ARGSHELPER_HPP__
