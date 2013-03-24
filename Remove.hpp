/**\file
 * "remove" action
 */
#ifndef __7I_REMOVE_HPP__
#define __7I_REMOVE_HPP__

#include <vector>

class RemoveHelper
{
  std::vector<std::wstring> directories;
public:
  ~RemoveHelper();

  void ScheduleRemove (const wchar_t* path);
  void FlushDelayed ();
};

int DoRemove (int argc, const wchar_t* const argv[]);

// TODO: Move somewhere else
std::wstring ReadRegistryListFilePath (const wchar_t* guid);

#endif // __7I_REMOVE_HPP__
