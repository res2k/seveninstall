/**\file
 * Exit code definitions
 */
#ifndef __EXITCODE_HPP__
#define __EXITCODE_HPP__

#include <WinError.h>

enum
{
    ecSuccess = __HRESULT_FROM_WIN32(ERROR_SUCCESS),

    ecArgsError = __HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS),
    ecHasDependencies = __HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)
};

#endif // __EXITCODE_HPP__
