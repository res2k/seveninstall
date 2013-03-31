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
    wprintf (L"7z installer\n"
             L"%hs\n"
             L"\n", extractCopyright);
}

static void PrintHelp (const wchar_t* exe)
{
    wprintf (L"Syntax:\n");
    wprintf (L"\t%ls install -g<GUID> -o<DIR> <archive.7z>...\n", exe);
    wprintf (L"\t%ls change -g<GUID> <archive.7z>...\n", exe);
    wprintf (L"\t%ls remove -g<GUID> [--ignore-dependents]\n", exe);
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
        wprintf (L"Too few arguments, expected command\n\n");
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
      wprintf (L"Unknown command %ls\n", argv[1]);
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
