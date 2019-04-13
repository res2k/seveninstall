/* We disabled environment initialization via noenv.obj, so dummy out some init functions
 * that are still reference but not called */

extern "C" int _initialize_narrow_environment ()
{
  return 0;
}

extern "C" int _initialize_wide_environment ()
{
  return 0;
}

extern "C" wchar_t** _get_initial_wide_environment ()
{
  return nullptr;
}

extern "C" void __dcrt_uninitialize_environments_nolock ()
{}
