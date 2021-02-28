/*** This file is based on code from 7zip ***/
// ExtractCallback.h

#ifndef __7I_EXTRACTCALLBACK_HPP__
#define __7I_EXTRACTCALLBACK_HPP__

#include <Windows.h>

#include "Common/Common.h"
#include "Common/MyString.h"
#include "7zip/Common/FileStreams.h"
#include "7zip/Archive/IArchive.h"
#include "7zip/UI/Common/ArchiveExtractCallback.h"

#include "MyUString.hpp"

#include <vector>

struct IArchiveCallback;
struct ProgressReporter;

class CExtractCallback :
  public IExtractCallbackUI,
  public CMyUnknownImp
{
  UString _currentName;
public:
  MY_QUERYINTERFACE_BEGIN2(IFolderArchiveExtractCallback)
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  CExtractCallback (ProgressReporter& progress, std::vector<MyUString>& extractedFiles, const UString& outputDir);

  STDMETHOD(SetTotal)(UInt64 total) override;
  STDMETHOD(SetCompleted)(const UInt64 *completeValue) override;


  INTERFACE_IFolderArchiveExtractCallback(;)

  INTERFACE_IExtractCallbackUI(;)
  // INTERFACE_IArchiveExtractCallbackMessage(;)
  INTERFACE_IFolderArchiveExtractCallback2(;)

  HRESULT GetExtractHR() const;

  unsigned LogLevel = 1;

  ProgressReporter& progress;
  std::vector<MyUString>& extractedFiles;
  // map from item name to desired full path
  std::unordered_map<MyUString, MyUString> renamesRequested;
  UInt64 NumTryArcs = 0;
  bool ThereIsError_in_Current;
  bool ThereIsWarning_in_Current;
  HRESULT extractHR = S_OK;
  UInt64 NumCantOpenArcs = 0;
  UInt64 NumOkArcs = 0;
  UInt64 NumArcsWithError = 0;
  UInt64 NumArcsWithWarnings = 0;
  UInt64 NumOpenArcErrors = 0;
  UInt64 NumOpenArcWarnings = 0;
  UInt64 NumFileErrors = 0;
  UInt64 NumFileErrors_in_Current = 0;
  UString outputDir;
};

#endif // __7I_EXTRACTCALLBACK_HPP__
