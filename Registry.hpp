/**\file
 * Registry access
 */
#ifndef __7I_REGISTRY_HPP__
#define __7I_REGISTRY_HPP__

#include <string>

#include <Windows.h>

/**
 * Helper class to access a registry key.
 */
class RegistryKey
{
  HKEY key;
public:
  struct CreateTag {};
  static CreateTag Create;

  /**
   * Open an existing registry key.
   * \param parent Parent key.
   * \param key Key path.
   * \param access Registry access flags.
   * \throws HRESULTException in case of an error.
   */
  RegistryKey (HKEY parent, const wchar_t* key, REGSAM access);
  /**
   * Create a new or open an existing registry key.
   * \param parent Parent key.
   * \param key Key path.
   * \param access Registry access flags.
   * \param tag Unused parameter; used for open vs create behaviour selection.
   * \throws HRESULTException in case of an error.
   */
  RegistryKey (HKEY parent, const wchar_t* key, REGSAM access, CreateTag& tag);
  ~RegistryKey ();

  /**
   * Write a DWORD value to the registry.
   * \param name Name of the value.
   * \param value Actual value.
   * \throws HRESULTException in case of an error.
   */
  void WriteDWORD (const wchar_t* name, DWORD value);
  /**
   * Write a string value to the registry.
   * \param name Name of the value.
   * \param value Actual value.
   * \throws HRESULTException in case of an error.
   */
  void WriteString (const wchar_t* name, const wchar_t* value);

  /**
   * Read a string value from the registry.
   * \param name Name of the value.
   * \returns Actual value.
   * \throws HRESULTException in case of an error.
   */
  std::wstring ReadString (const wchar_t* name);

  /**
   * Query number of subkeys under the key.
   * \returns Number of subkeys.
   * \throws HRESULTException in case of an error.
   */
  size_t NumSubkeys ();
};

#endif // __7I_REGISTRY_HPP__
