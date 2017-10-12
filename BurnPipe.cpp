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

#include "BurnPipe.hpp"

#include "ArgsHelper.hpp"

#include "burn-pipe/precomp.h"

BurnPipe BurnPipe::Create (const ArgsHelper& args)
{
  const wchar_t* pipeName;
  const wchar_t* pipeSecret;
  const wchar_t* pipePPID;
  if (args.GetOption (ArgsHelper::burnEmbeddedPipeName, pipeName)
      && args.GetOption (ArgsHelper::burnEmbeddedPipeSecret, pipeSecret)
      && args.GetOption (ArgsHelper::burnEmbeddedPipePPID, pipePPID))
  {
    wchar_t* ppid_end;
    uint32_t ppid_val = wcstoul (pipePPID, &ppid_end, 10);
    if (ppid_end != pipePPID)
      return BurnPipe (pipeName, pipeSecret, ppid_val);
  }

  return BurnPipe ();
}

BurnPipe::BurnPipe () {}

BurnPipe::BurnPipe (const wchar_t* name, const wchar_t* secret, uint32_t processId)
{
  pipeConnection.reset (new BURN_PIPE_CONNECTION);
  PipeConnectionInitialize (pipeConnection.get());

  HRESULT hr;
  hr = StrAllocString(&pipeConnection->sczName, name, 0);
  if (SUCCEEDED(hr))
  {
    hr = StrAllocString(&pipeConnection->sczSecret, secret, 0);
  }
  pipeConnection->dwProcessId = processId;

  if (SUCCEEDED(hr))
  {
    hr = PipeChildConnect (pipeConnection.get(), false);
  }
  if (FAILED(hr))
  {
    fprintf (stderr, "Failed to establish pipe connection: %#x\n", hr);
  }
}

BurnPipe::BurnPipe (BurnPipe&& other) : pipeConnection (std::move (other.pipeConnection)) {}

BurnPipe::~BurnPipe ()
{
  if (pipeConnection)
  {
    PipeConnectionUninitialize (pipeConnection.get());
    pipeConnection.reset ();
  }
}

BurnPipe& BurnPipe::operator= (BurnPipe&& other)
{
  pipeConnection = std::move (other.pipeConnection);
  return *this;
}

bool BurnPipe::IsConnected () const
{
  return pipeConnection && (pipeConnection->hPipe != INVALID_HANDLE_VALUE);
}
