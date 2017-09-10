/*** This file is based on code from 7zip ***/
// OpenCallbackConsole.cpp

//#include "StdAfx.h"

#include "OpenCallback.hpp"

#include "7zip/UI/Console/ConsoleClose.h"
#include "7zip/UI/Console/UserInputUtils.h"

static HRESULT CheckBreak2()
{
  return NConsoleClose::TestBreakSignal() ? E_ABORT : S_OK;
}

HRESULT COpenCallback::Open_CheckBreak()
{
  return CheckBreak2();
}

HRESULT COpenCallback::Open_SetTotal(const UInt64 *files, const UInt64 *bytes)
{
  if (!MutiArcMode)
  {
    if (files)
    {
      _totalFilesDefined = true;
      // _totalFiles = *files;
    }
    else
      _totalFilesDefined = false;

    if (bytes)
    {
      _totalBytesDefined = true;
      // _totalBytes = *bytes;
    }
    else
      _totalBytesDefined = false;
  }

  return CheckBreak2();
}

HRESULT COpenCallback::Open_SetCompleted(const UInt64 *files, const UInt64 *bytes)
{
  return CheckBreak2();
}

HRESULT COpenCallback::Open_Finished()
{
  return S_OK;
}


#ifndef _NO_CRYPTO

HRESULT COpenCallback::Open_CryptoGetTextPassword(BSTR *password)
{
  *password = NULL;
  RINOK(CheckBreak2());

  if (!PasswordIsDefined)
  {
    Password = GetPassword(_so);
    PasswordIsDefined = true;
  }
  return StringToBstr(Password, password);
}

/*
HRESULT COpenCallback::Open_GetPasswordIfAny(bool &passwordIsDefined, UString &password)
{
  passwordIsDefined = PasswordIsDefined;
  password = Password;
  return S_OK;
}

bool COpenCallback::Open_WasPasswordAsked()
{
  return PasswordWasAsked;
}

void COpenCallback::Open_Clear_PasswordWasAsked_Flag ()
{
  PasswordWasAsked = false;
}
*/

#endif
