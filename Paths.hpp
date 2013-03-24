/**\file
 * Helper to locate default paths
 */

#ifndef __7I_PATHS_HPP__
#define __7I_PATHS_HPP__

#include <Windows.h>

#include <string>

// Generate installation log file for a given installation GUID
std::wstring InstallLogFile (const wchar_t* guid);

/**\name Registry paths
 * @{ */
extern const wchar_t regPathUninstallInfo[];
extern const wchar_t regValLogFileName[];
extern const wchar_t regValInstallDir[];
extern const wchar_t regPathDependencyInfo[];
extern const wchar_t regPathDependentsSubkey[];
/** @} */

#endif // __7I_PATHS_HPP__
