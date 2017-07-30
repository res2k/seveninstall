/*** This file is based on code from 7zip ***/
// ExtractCallback.h

#ifndef __7I_EXTRACTCALLBACK_HPP__
#define __7I_EXTRACTCALLBACK_HPP__

#include "Common/MyString.h"
#include "7zip/Common/FileStreams.h"
#include "7zip/Archive/IArchive.h"
#include "7zip/UI/Common/ArchiveExtractCallback.h"

#include <string>
#include <vector>

struct IArchiveCallback;

class CExtractCallback :
  public IExtractCallbackUI,
  public CMyUnknownImp
{
public:
  MY_QUERYINTERFACE_BEGIN2(IFolderArchiveExtractCallback)
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  CExtractCallback (std::vector<std::wstring>& extractedFiles, const UString& outputDir);

  STDMETHOD(SetTotal)(UInt64 total);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  // IFolderArchiveExtractCallback
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
      Int32 *answer);
  STDMETHOD (PrepareOperation)(const wchar_t *name, bool isFolder, Int32 askExtractMode, const UInt64 *position);

  STDMETHOD(MessageError)(const wchar_t *message);
  STDMETHOD(SetOperationResult)(Int32 operationResult, bool encrypted);

  HRESULT BeforeOpen(const wchar_t *name);
  HRESULT OpenResult(const wchar_t *name, HRESULT result, bool encrypted);
  HRESULT ThereAreNoFiles();
  HRESULT ExtractResult(HRESULT result);

 
  std::vector<std::wstring>& extractedFiles;
  UInt64 NumArchiveErrors;
  UInt64 NumFileErrorsInCurrentArchive;
  std::wstring currentFile;
  UString outputDir;
};

#endif // __7I_EXTRACTCALLBACK_HPP__
