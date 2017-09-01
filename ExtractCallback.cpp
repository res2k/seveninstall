/*** This file is based on code from 7zip ***/
// ExtractCallback.cpp

#include "ExtractCallback.hpp"

#include "Extract.hpp"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Time.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/Error.h"
#include "Windows/PropVariantConversions.h"

#include "7zip/Common/FilePathAutoRename.h"

#include "7zip/UI/Console/ConsoleClose.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;

static const wchar_t *kTestString    =  L"Testing     ";
static const wchar_t *kExtractString =  L"Extracting  ";
static const wchar_t *kSkipString    =  L"Skipping    ";

// static const char *kCantAutoRename = "can not create file with auto name\n";
// static const char *kCantRenameFile = "can not rename existing file\n";
// static const char *kCantDeleteOutputFile = "can not delete output file ";
static const wchar_t *kError = L"ERROR: ";
static const wchar_t *kMemoryExceptionMessage = L"Can't allocate required memory!";

static const wchar_t *kProcessing = L"Processing archive: ";
static const wchar_t *kEverythingIsOk = L"Everything is Ok";
static const wchar_t *kNoFiles = L"No files to process";

static const wchar_t *kUnsupportedMethod = L"Unsupported Method";
static const wchar_t *kCrcFailed = L"CRC Failed";
static const wchar_t *kDataError = L"Data Error";
static const wchar_t *kUnknownError = L"Unknown Error";

CExtractCallback::CExtractCallback (std::vector<std::wstring>& extractedFiles, const UString& outputDir)
  : extractedFiles (extractedFiles), NumArchiveErrors (0), NumFileErrorsInCurrentArchive (0), outputDir (outputDir)
{
}


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
  UString fullName (outputDir);
  fullName += name;
  currentFile = fullName;
  switch (askExtractMode)
  {
  case NArchive::NExtract::NAskMode::kExtract: wprintf (kExtractString); break;
  case NArchive::NExtract::NAskMode::kTest:    wprintf (kTestString); break;
  case NArchive::NExtract::NAskMode::kSkip:    wprintf (kSkipString); break;
  };
  wprintf (L"%ls", name);
  //fflush (stdout); // Not needed since our wprintf() implementation bypasses stdio
  return S_OK;
}

STDMETHODIMP CExtractCallback::MessageError(const wchar_t *message)
{
  wprintf (L"%ls\n", message);
  NumFileErrorsInCurrentArchive++;
  return S_OK;
}

STDMETHODIMP CExtractCallback::SetOperationResult(Int32 operationResult, bool encrypted)
{
  switch(operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      extractedFiles.push_back (currentFile);
      break;
    default:
    {
      NumFileErrorsInCurrentArchive++;
      wprintf (L"     ");
      switch(operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          wprintf (kUnsupportedMethod);
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          wprintf (kCrcFailed);
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          wprintf (kDataError);
          break;
        default:
          wprintf (kUnknownError);
      }
    }
  }
  wprintf (L"\n");
  return S_OK;
}

HRESULT CExtractCallback::BeforeOpen(const wchar_t *name)
{
  NumFileErrorsInCurrentArchive = 0;
  wprintf (L"\n%s%s\n", kProcessing, name);
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
      wprintf (L"Can not open file as archive");
    }
    else
    {
      if (result == E_OUTOFMEMORY)
        wprintf (kMemoryExceptionMessage);
      else
        wprintf (L"%s", (const wchar_t*)(NError::MyFormatMessage(result)));
    }
    wprintf (L"\n");
  }
  return S_OK;
}
  
HRESULT CExtractCallback::ThereAreNoFiles()
{
  wprintf (L"\n%s\n", kNoFiles);
  return S_OK;
}

HRESULT CExtractCallback::ExtractResult(HRESULT result)
{
  if (result == S_OK)
  {
    wprintf (L"\n");
    if (NumFileErrorsInCurrentArchive == 0)
      wprintf (L"%s\n", kEverythingIsOk);
    else
    {
      wprintf (L"Sub items Errors: %llu\n", NumFileErrorsInCurrentArchive);
    }
  }
  if (result == S_OK)
    return result;
  if (result == E_ABORT || result == ERROR_DISK_FULL)
    return result;
  wprintf (L"\n%s", kError);
  if (result == E_OUTOFMEMORY)
    wprintf (kMemoryExceptionMessage);
  else
  {
    UString message;
    NError::MyFormatMessage(result, message);
    wprintf (L"%s", (const wchar_t*)(message));
  }
  wprintf (L"\n");
  return S_OK;
}
