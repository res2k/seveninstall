/**\file
 * "install" action
 */
#ifndef __7I_INSTALL_HPP__
#define __7I_INSTALL_HPP__

class InstalledFilesWriter;

// TODO: Move somewhere else...
void WriteToRegistry (const wchar_t* guid, const InstalledFilesWriter& list, const wchar_t* directory);

int DoInstall (int argc, const wchar_t* const argv[]);

#endif // __7I_INSTALL_HPP__
