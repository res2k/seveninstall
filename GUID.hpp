/**\file
 * GUID verification
 */
#ifndef __7I_GUID_HPP__
#define __7I_GUID_HPP__

/**
 * Verify if string is a valid GUID.
 * \remarks A SevenInstall "GUID" is defined very loosely - it can be an arbitrary
 * string, although the set of allowed characters is restricted to alphanumeric ASCII
 * characters and some special characters.
 */
bool VerifyGUID (const wchar_t* guid);

#endif // __7I_GUID_HPP__
