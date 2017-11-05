// CRT argv expansion functions. We don't expand wildcards in argv, so dummy them out
#include <errno.h>

extern "C" errno_t __acrt_expand_narrow_argv_wildcards (char**, char***)
{
  return EINVAL;
}

extern "C" errno_t __acrt_expand_wide_argv_wildcards (wchar_t**, wchar_t***)
{
  return EINVAL;
}
