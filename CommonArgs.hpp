/**\file
 * Common command argument handling
 */
#ifndef __7I_COMMONARGS_HPP__
#define __7I_COMMONARGS_HPP__

class ArgsHelper;

/**
 * Class for common command argument handling
 */
class CommonArgs
{
  const ArgsHelper& args;
public:
  CommonArgs (const ArgsHelper& args) : args (args) {}

  /**
   * Get GUID (\c -g) argument.
   * \param guid Returns a pointer to the GUID argument, if one is present.
   * \param required If \c true, prints a message if the argument is missing.
   * \returns Whether a GUID (\c -g) argument was given.
   */
  bool GetGUID (const wchar_t*& guid, bool reportMissing = true);
};

#endif // __7I_COMMONARGS_HPP__
