#include "ArgsHelper.hpp"

ArgsHelper::ArgsHelper (int argc, const wchar_t* const argv[])
{
    for (int a = 1; a < argc; a++)
    {
        const wchar_t* arg (argv[a]);
        if (arg[0] == '-')
        {
            if (arg[1] == '-')
            {
                // --long option - no value
                options[arg] = nullptr;
            }
            else
            {
                // -s ...hort option - value after 1st letter
                std::wstring opt_str (arg, 2);
                options[opt_str] = arg+2;
            }
        }
        else
        {
            freeArgs.push_back (arg);
        }
    }
}

bool ArgsHelper::GetOption (const wchar_t* prefix, const wchar_t*& value) const
{
    OptionsMapType::const_iterator opt (options.find (prefix));
    if (opt == options.end()) return false;
    value = opt->second;
    return true;
}

bool ArgsHelper::GetOption (const wchar_t* option) const
{
    OptionsMapType::const_iterator opt (options.find (option));
    return opt != options.end();
}

void ArgsHelper::GetFreeArgs (std::vector<const wchar_t*>& args) const
{
    args = freeArgs;
}

