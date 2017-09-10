/*** This file is based on code from 7zip ***/
// OpenCallback.h

#ifndef __7I_OPENCALLBACK_HPP__
#define __7I_OPENCALLBACK_HPP__

#include "7zip/UI/Common/ArchiveOpenCallback.h"

class COpenCallback: public IOpenCallbackUI
{
protected:
  bool _totalFilesDefined;
  bool _totalBytesDefined;
  // UInt64 _totalFiles;
  // UInt64 _totalBytes;

  bool NeedPercents() const { return false; }

public:

  bool MutiArcMode;

  void ClosePercents()
  {
  }

  COpenCallback():
      _totalFilesDefined(false),
      _totalBytesDefined(false),
      MutiArcMode(false)
      
      #ifndef _NO_CRYPTO
      , PasswordIsDefined(false)
      // , PasswordWasAsked(false)
      #endif
      
      {}
  
  void Init()
  {
  }

  INTERFACE_IOpenCallbackUI(;)
  
  #ifndef _NO_CRYPTO
  bool PasswordIsDefined;
  // bool PasswordWasAsked;
  UString Password;
  #endif
};

#endif // __7I_OPENCALLBACK_HPP__
