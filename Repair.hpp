/**\file
 * "repair" action
 */
#ifndef __7I_REPAIR_HPP__
#define __7I_REPAIR_HPP__

#include <string>

// TODO: Move somewhere else...
std::wstring ReadRegistryOutputDir (const wchar_t* guid);
int DoRepair (int argc, const wchar_t* const argv[]);

#endif // __7I_REPAIR_HPP__
