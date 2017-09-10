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

/**
 * SevenInstall - "install" a bunch of files from a 7z archive
 */
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "Error.hpp"
#include "ExitCode.hpp"
#include "Extract.hpp"

#include "Install.hpp"
#include "Remove.hpp"
#include "Repair.hpp"

static void PrintBanner ()
{
    printf ("7z installer\n"
            "%s\n"
            "\n", extractCopyright);
}

static void PrintHelp (const wchar_t* exe)
{
    printf ("Syntax:\n");
    printf ("\t%ls install -g<GUID> -o<DIR> <archive.7z>...\n", exe);
    printf ("\t%ls change -g<GUID> <archive.7z>...\n", exe);
    printf ("\t%ls remove -g<GUID> [--ignore-dependents]\n", exe);
}

enum ECommand
{
    cmdUnknown,
    cmdInstall,
    cmdRepair,
    cmdRemove
};

int wmain (int argc, const wchar_t* const argv[])
{
    PrintBanner ();

    /*
     Operations:
        install -g<GUID> -o<DIR> <archive.7z> ...
         - Extract archive to given output dir
         - GUID is used to identify contents for uninstall later
        repair -g<GUID> <archive.7z> ...
         (almost synonymous for install, uses previously set output dir)
        remove -g<GUID> [--ignore-dependents]
         - Uninstall previously installed files
         - --ignore-dependents - ignore registry dependency infos
         - -r - Remove output directory used at install time

     TODO: extract
         - no logging
     */

    if (argc < 2)
    {
        printf ("Too few arguments, expected command\n\n");
        PrintHelp (argv[0]);
        return ecArgsError;
    }

    ECommand cmd (cmdUnknown);
    if (wcscmp (argv[1], L"install") == 0)
    {
        cmd = cmdInstall;
    }
    else if (wcscmp (argv[1], L"repair") == 0)
    {
      cmd = cmdRepair;
    }
    else if (wcscmp (argv[1], L"remove") == 0)
    {
      cmd = cmdRemove;
    }
    else
    {
      printf ("Unknown command %ls\n", argv[1]);
      return ecArgsError;
    }

    // Arguments w/o initial command
    const wchar_t** filtered_args = (const wchar_t**)_alloca ((argc-1) * sizeof (wchar_t*));
    filtered_args[0] = argv[0];
    for (int i = 2; i < argc; i++)
    {
        filtered_args[i-1] = argv[i];
    }

    switch (cmd)
    {
    case cmdInstall:
        return DoInstall (argc-1, filtered_args);
    case cmdRepair:
        return DoRepair (argc-1, filtered_args);
    case cmdRemove:
        return DoRemove (argc-1, filtered_args);
    }

    return 0;
}
