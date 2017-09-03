/*** This file is based on code from 7zip ***/
// ExtractCallback.cpp

// #undef sprintf

#include "ExtractCallback.hpp"

#include "7zip/UI/Console/ConsoleClose.h"

#include "Common/IntToString.h"
#include "Common/Wildcard.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/TimeUtils.h"
#include "Windows/ErrorMsg.h"
#include "Windows/PropVariantConv.h"

#include "7zip/Common/FilePathAutoRename.h"

#include "7zip/UI/Common/ExtractingFilePath.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;

static const char *kTestString    =  "Testing     ";
static const char *kExtractString =  "Extracting  ";
static const char *kSkipString    =  "Skipping    ";

// static const char *kCantAutoRename = "can not create file with auto name\n";
// static const char *kCantRenameFile = "can not rename existing file\n";
// static const char *kCantDeleteOutputFile = "can not delete output file ";
static const char *kError = "ERROR: ";
static const char *kMemoryExceptionMessage = "Can't allocate required memory!";

static const char *kProcessing = "Processing archive: ";
static const char *kEverythingIsOk = "Everything is Ok";
static const char *kNoFiles = "No files to process";

static const char *kUnsupportedMethod = "Unsupported Method";
static const char *kCrcFailed = "CRC Failed";
static const char *kCrcFailedEncrypted = "CRC Failed in encrypted file. Wrong password?";
static const char *kDataError = "Data Error";
static const char *kDataErrorEncrypted = "Data Error in encrypted file. Wrong password?";
static const char *kUnavailableData = "Unavailable data";
static const char *kUnexpectedEnd = "Unexpected end of data";
static const char *kDataAfterEnd = "There are some data after the end of the payload data";
static const char *kIsNotArc = "Is not archive";
static const char *kHeadersError = "Headers Error";

static const char *k_ErrorFlagsMessages[] =
{
    "Is not archive"
  , "Headers Error"
  , "Headers Error in encrypted archive. Wrong password?"
  , "Unavailable start of archive"
  , "Unconfirmed start of archive"
  , "Unexpected end of archive"
  , "There are data after the end of archive"
  , "Unsupported method"
  , "Unsupported feature"
  , "Data Error"
  , "CRC Error"
};

CExtractCallback::CExtractCallback (std::vector<std::wstring>& extractedFiles, const UString& outputDir)
  : extractedFiles (extractedFiles), outputDir (outputDir) {}

STDMETHODIMP CExtractCallback::SetTotal(UInt64)
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP CExtractCallback::SetCompleted(const UInt64 *)
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP CExtractCallback::AskOverwrite(
    const wchar_t *existName, const FILETIME *, const UInt64 *,
    const wchar_t *newName, const FILETIME *, const UInt64 *,
    Int32 *answer)
{
  /* FIXME: If we'd do some sort of 'rollback' support (ability to
   * 'undo' an extraction of some file), this would be the place */
  *answer = NOverwriteAnswer::kYes;
  return S_OK;
}

STDMETHODIMP CExtractCallback::PrepareOperation(const wchar_t *name, bool /* isFolder */, Int32 askExtractMode, const UInt64 *position)
{
  const char *s;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract: s = kExtractString; break;
    case NArchive::NExtract::NAskMode::kTest:    s = kTestString; break;
    case NArchive::NExtract::NAskMode::kSkip:    s = kSkipString; break;
    default: s = ""; // return E_FAIL;
  };
  wprintf (L"%hs %ls", s, name);
  if (position != 0)
    wprintf (L" <%lld>", *position);
  return S_OK;
}

STDMETHODIMP CExtractCallback::MessageError(const wchar_t *message)
{
  wprintf (L"%ls\n", message);
  NumFileErrorsInCurrent++;
  NumFileErrors++;
  return S_OK;
}

STDMETHODIMP CExtractCallback::SetOperationResult(Int32 operationResult, bool encrypted)
{
  switch (operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      extractedFiles.push_back (currentFile);
      break;
    default:
    {
      NumFileErrorsInCurrent++;
      NumFileErrors++;
      wprintf (L"  :  ");
      const char *s = NULL;
      switch (operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
          s = kUnsupportedMethod;
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          s = (encrypted ? kCrcFailedEncrypted : kCrcFailed);
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          s = (encrypted ? kDataErrorEncrypted : kDataError);
          break;
        case NArchive::NExtract::NOperationResult::kUnavailable:
          s = kUnavailableData;
          break;
        case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
          s = kUnexpectedEnd;
          break;
        case NArchive::NExtract::NOperationResult::kDataAfterEnd:
          s = kDataAfterEnd;
          break;
        case NArchive::NExtract::NOperationResult::kIsNotArc:
          s = kIsNotArc;
          break;
        case NArchive::NExtract::NOperationResult::kHeadersError:
          s = kHeadersError;
          break;
      }
      if (s)
        wprintf (L"Error : %hs", s);
      else
      {
        wprintf (L"Error #%d", operationResult);
      }
    }
  }
  wprintf (L"\n");
  return S_OK;
}

HRESULT CExtractCallback::BeforeOpen(const wchar_t *name)
{
  NumTryArcs++;
  ThereIsErrorInCurrent = false;
  ThereIsWarningInCurrent = false;
  NumFileErrorsInCurrent = 0;
  wprintf (L"%hs%ls\n", kProcessing, name);
  return S_OK;
}

HRESULT CExtractCallback::OpenResult(const wchar_t * /* name */, HRESULT result, bool encrypted)
{
  wprintf (L"\n");
  if (result != S_OK)
  {
    wprintf (L"Error: ");
    if (result == S_FALSE)
    {
      wprintf (encrypted ?
        L"Can not open encrypted archive" :
        L"Can not open file as archive");
    }
    else
    {
      if (result == E_OUTOFMEMORY)
        wprintf (L"Can't allocate required memory");
      else
        wprintf (L"%s", (const wchar_t*)NError::MyFormatMessage(result));
    }
    wprintf (L"\n");
    NumCantOpenArcs++;
    ThereIsErrorInCurrent = true;
  }
  return S_OK;
}

AString GetOpenArcErrorMessage(UInt32 errorFlags)
{
  AString s;
  for (unsigned i = 0; i < sizeof(k_ErrorFlagsMessages)/sizeof(k_ErrorFlagsMessages[0]); i++)
  {
    UInt32 f = (1 << i);
    if ((errorFlags & f) == 0)
      continue;
    const char *m = k_ErrorFlagsMessages[i];
    if (!s.IsEmpty())
      s += '\n';
    s += m;
    errorFlags &= ~f;
  }
  if (errorFlags != 0)
  {
    char sz[16];
    sz[0] = '0';
    sz[1] = 'x';
    ConvertUInt32ToHex(errorFlags, sz + 2);
    if (!s.IsEmpty())
      s += '\n';
    s += sz;
  }
  return s;
}


HRESULT CExtractCallback::SetError(int level, const wchar_t *name,
    UInt32 errorFlags, const wchar_t *errors,
    UInt32 warningFlags, const wchar_t *warnings)
{
  if (level != 0)
  {
    wprintf (L"%s", name);
  }

  if (errorFlags != 0)
  {
    wprintf (L"Errors: \n%hs\n", (const char*)GetOpenArcErrorMessage(errorFlags));
    NumOpenArcErrors++;
    ThereIsErrorInCurrent = true;
  }

  if (errors && wcslen(errors) != 0)
  {
    wprintf (L"Errors: \n%s\n", errors);
    NumOpenArcErrors++;
    ThereIsErrorInCurrent = true;
  }

  if (warningFlags != 0)
  {
    wprintf (L"Warnings: \n%hs\n", (const char*)GetOpenArcErrorMessage(warningFlags));
    NumOpenArcWarnings++;
    ThereIsWarningInCurrent = true;
  }

  if (warnings && wcslen(warnings) != 0)
  {
    wprintf (L"Warnings: \n%s\n", warnings);
    NumOpenArcWarnings++;
    ThereIsWarningInCurrent = true;
  }

  wprintf (L"\n");
  return S_OK;
}
  
HRESULT CExtractCallback::ThereAreNoFiles()
{
  wprintf (L"\n%hs\n", kNoFiles);
  return S_OK;
}

HRESULT CExtractCallback::ExtractResult(HRESULT result)
{
  if (result == S_OK)
  {
    wprintf (L"\n");

    if (NumFileErrorsInCurrent == 0 && !ThereIsErrorInCurrent)
    {
      if (ThereIsWarningInCurrent)
        NumArcsWithWarnings++;
      else
        NumOkArcs++;
      wprintf (L"%hs\n", kEverythingIsOk);
    }
    else
    {
      NumArcsWithError++;
      if (NumFileErrorsInCurrent != 0)
        wprintf (L"Sub items Errors: %llu\n", NumFileErrorsInCurrent);
    }
    return S_OK;
  }
  
  NumArcsWithError++;
  if (result == E_ABORT || result == ERROR_DISK_FULL)
    return result;
  wprintf (L"\n%hs", kError);
  if (result == E_OUTOFMEMORY)
    wprintf (L"%hs", kMemoryExceptionMessage);
  else
    wprintf (L"%s", (const wchar_t*)NError::MyFormatMessage(result));
  wprintf (L"\n");
  return S_OK;
}

HRESULT CExtractCallback::OpenTypeWarning(const wchar_t *name, const wchar_t *okType, const wchar_t *errorType)
{
  UString s = L"Warning:\n";
  if (wcscmp(okType, errorType) == 0)
  {
    s += L"The archive is open with offset";
  }
  else
  {
    s += name;
    s += L"\nCan not open the file as [";
    s += errorType;
    s += L"] archive\n";
    s += L"The file is open as [";
    s += okType;
    s += L"] archive";
  }
  wprintf (L"%s\n\n", (const wchar_t*)s);
 ThereIsWarningInCurrent = true;
  return S_OK;
}
