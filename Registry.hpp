/**\file
 * Registry access
 */
#ifndef __7I_REGISTRY_HPP__
#define __7I_REGISTRY_HPP__

#include <string>

#include <Windows.h>

class RegistryKey
{
  HKEY key;
public:
  struct CreateTag {};
  static CreateTag Create;

  RegistryKey (HKEY parent, const wchar_t* key, REGSAM access);
  RegistryKey (HKEY parent, const wchar_t* key, REGSAM access, CreateTag&);
  ~RegistryKey ();

  void WriteDWORD (const wchar_t* name, DWORD value);
  void WriteString (const wchar_t* name, const wchar_t* value);

  std::wstring ReadString (const wchar_t* name);

  size_t NumSubkeys ();
};

#endif // __7I_REGISTRY_HPP__
