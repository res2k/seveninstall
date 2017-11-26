/*
    SevenInstall
    Copyright (c) 2013-2017 Frank Richter

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org>
 */

/**\file
 * Wrapper around Burn pipe communication
 */
#ifndef SEVENI_BURNPIPE_HPP_
#define SEVENI_BURNPIPE_HPP_

#include <memory>

class ArgsHelper;
struct _BURN_PIPE_CONNECTION;

extern const char burnPipeCopyright[];

class BurnPipe
{
public:
  /// Create a BurnPipe instance
  static BurnPipe Create (const ArgsHelper& args);

  BurnPipe (BurnPipe&& other);
  ~BurnPipe ();

  BurnPipe& operator= (BurnPipe&& other);

  bool IsConnected () const;

  enum struct Processing { Continue, Cancel };
  /// Send progress over pipe
  Processing SendProgress (unsigned int percent);
private:
  std::unique_ptr<_BURN_PIPE_CONNECTION> pipeConnection;

  BurnPipe ();
  BurnPipe (const wchar_t* name, const wchar_t* secret, uint32_t processId);
};

#endif // SEVENI_BURNPIPE_HPP_
