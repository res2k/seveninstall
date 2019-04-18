/*** This file is based on code from 7zip ***/
// ExtractCallback.cpp

// #undef sprintf
#include "ExtractCallback.hpp"

#include "Paths.hpp"
#include "ProgressReporter.hpp"

#include "Common/IntToString.h"
#include "Common/Wildcard.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/TimeUtils.h"
#include "Windows/ErrorMsg.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"

#ifndef _7ZIP_ST
#include "Windows/Synchronization.h"
#endif

#include "7zip/Common/FilePathAutoRename.h"

#include "7zip/UI/Common/ExtractingFilePath.h"
#include "7zip/UI/Common/PropIDUtils.h"

#include "7zip/UI/Console/ConsoleClose.h"
#include "7zip/UI/Console/UserInputUtils.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;
using namespace NCOM;

static HRESULT CheckBreak2()
{
  return NConsoleClose::TestBreakSignal() ? E_ABORT : S_OK;
}

static const char *kError = "ERROR: ";

#ifndef _7ZIP_ST
static NSynchronization::CCriticalSection g_CriticalSection;
#define MT_LOCK NSynchronization::CCriticalSectionLock lock(g_CriticalSection);
#else
#define MT_LOCK
#endif


static const char *kTestString    =  "T";
static const char *kExtractString =  "-";
static const char *kSkipString    =  ".";

// static const char *kCantAutoRename = "can not create file with auto name\n";
// static const char *kCantRenameFile = "can not rename existing file\n";
// static const char *kCantDeleteOutputFile = "can not delete output file ";

static const char *kMemoryExceptionMessage = "Can't allocate required memory!";

static const char *kExtracting = "Extracting archive: ";
static const char *kTesting = "Testing archive: ";

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
static const char *kWrongPassword = "Wrong password";

static const char * const k_ErrorFlagsMessages[] =
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

CExtractCallback::CExtractCallback (ProgressReporter& progress,
                                    std::vector<MyUString>& extractedFiles,
                                    const UString& outputDir)
  : progress (progress), extractedFiles (extractedFiles), outputDir (outputDir)
{
  NName::NormalizeDirPathPrefix (this->outputDir);
}

STDMETHODIMP CExtractCallback::SetTotal(UInt64 size)
{
  MT_LOCK

  progress.SetTotal (size);

  return CheckBreak2();
}

STDMETHODIMP CExtractCallback::SetCompleted(const UInt64* completeValue)
{
  MT_LOCK

  HRESULT hr = S_OK;
  if (completeValue)
  {
    if (progress.SetCompleted (*completeValue) == ProgressReporter::Processing::Cancel)
      hr = E_ABORT;
  }

  return SUCCEEDED(hr) ? CheckBreak2() : hr;
}

STDMETHODIMP CExtractCallback::AskOverwrite(
    const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
    const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
    Int32 *answer)
{
  /* FIXME: If we'd do some sort of 'rollback' support (ability to
   * 'undo' an extraction of some file), this would be the place */
  *answer = NOverwriteAnswer::kYes;
  return CheckBreak2();
}

STDMETHODIMP CExtractCallback::PrepareOperation(const wchar_t *name, Int32 /* isFolder */, Int32 askExtractMode, const UInt64 *position)
{
  MT_LOCK
  
  _currentName = name;
  
  const char *s;
  unsigned requiredLevel = 1;

  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract: s = kExtractString; break;
    case NArchive::NExtract::NAskMode::kTest:    s = kTestString; break;
    case NArchive::NExtract::NAskMode::kSkip:    s = kSkipString; requiredLevel = 2; break;
    default: s = "???"; requiredLevel = 2;
  };

  bool show2 = (LogLevel >= requiredLevel);

  if (show2)
  {
    printf (name ?  "%s " : "%s", s);

    if (name) printf ("%ls", name);
    if (position)
      printf (" <%llu>", *position);
    printf ("\n");
  }

  return CheckBreak2();
}

STDMETHODIMP CExtractCallback::MessageError(const wchar_t *message)
{
  MT_LOCK
  
  RINOK(CheckBreak2());

  NumFileErrors_in_Current++;
  NumFileErrors++;

  fprintf (stderr, "%s%ls\n", kError, message);

  return CheckBreak2();
}

void SetExtractErrorMessage(Int32 opRes, Int32 encrypted, AString &dest)
{
  dest.Empty();
    const char *s = NULL;
    
    switch (opRes)
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
      case NArchive::NExtract::NOperationResult::kWrongPassword:
        s = kWrongPassword;
        break;
    }
    
    dest += kError;
    if (s)
      dest += s;
    else
    {
      char temp[16];
      ConvertUInt32ToString(opRes, temp);
      dest += "Error #";
      dest += temp;
    }
}

STDMETHODIMP CExtractCallback::SetOperationResult(Int32 opRes, Int32 encrypted)
{
  MT_LOCK
  
  if (opRes == NArchive::NExtract::NOperationResult::kOK)
  {
    MyUString filename = (outputDir + _currentName);
    NormalizePath (filename);
    extractedFiles.emplace_back (std::move (filename));
  }
  else
  {
    NumFileErrors_in_Current++;
    NumFileErrors++;
    
    AString s;
    SetExtractErrorMessage(opRes, encrypted, s);

    fprintf (stderr, "%s", s.Ptr());
    if (!_currentName.IsEmpty ())
      fprintf (stderr, " : %ls", _currentName.Ptr ());
    fprintf (stderr, "\n");
  }
  
  return CheckBreak2();
}

STDMETHODIMP CExtractCallback::ReportExtractResult(Int32 opRes, Int32 encrypted, const wchar_t *name)
{
  if (opRes != NArchive::NExtract::NOperationResult::kOK)
  {
    _currentName = name;
    return SetOperationResult(opRes, encrypted);
  }

  return CheckBreak2();
}



#ifndef _NO_CRYPTO

HRESULT CExtractCallback::SetPassword(const UString &password)
{
  PasswordIsDefined = true;
  Password = password;
  return S_OK;
}

STDMETHODIMP CExtractCallback::CryptoGetTextPassword(BSTR *password)
{
  COM_TRY_BEGIN
  MT_LOCK
  return Open_CryptoGetTextPassword(password);
  COM_TRY_END
}

#endif

HRESULT CExtractCallback::BeforeOpen(const wchar_t *name, bool testMode)
{
  RINOK(CheckBreak2());

  NumTryArcs++;
  ThereIsError_in_Current = false;
  ThereIsWarning_in_Current = false;
  NumFileErrors_in_Current = 0;

  printf ("\n%s%ls\n", (testMode ? kTesting : kExtracting), name);
  if (kExtracting) printf ("... to: %ls\n", outputDir.Ptr());

  return S_OK;
}

static AString GetOpenArcErrorMessage(UInt32 errorFlags)
{
  AString s;

  for (unsigned i = 0; i < sizeof(k_ErrorFlagsMessages)/sizeof(k_ErrorFlagsMessages[0]); i++)
  {
    UInt32 f = (1 << i);
    if ((errorFlags & f) == 0)
      continue;
    const char *m = k_ErrorFlagsMessages[i];
    if (!s.IsEmpty())
      s.Add_LF();
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
      s.Add_LF();
    s += sz;
  }

  return s;
}

#if 0
static const char * const kPropIdToName[] =
{
  "0"
  , "1"
  , "2"
  , "Path"
  , "Name"
  , "Extension"
  , "Folder"
  , "Size"
  , "Packed Size"
  , "Attributes"
  , "Created"
  , "Accessed"
  , "Modified"
  , "Solid"
  , "Commented"
  , "Encrypted"
  , "Split Before"
  , "Split After"
  , "Dictionary Size"
  , "CRC"
  , "Type"
  , "Anti"
  , "Method"
  , "Host OS"
  , "File System"
  , "User"
  , "Group"
  , "Block"
  , "Comment"
  , "Position"
  , "Path Prefix"
  , "Folders"
  , "Files"
  , "Version"
  , "Volume"
  , "Multivolume"
  , "Offset"
  , "Links"
  , "Blocks"
  , "Volumes"
  , "Time Type"
  , "64-bit"
  , "Big-endian"
  , "CPU"
  , "Physical Size"
  , "Headers Size"
  , "Checksum"
  , "Characteristics"
  , "Virtual Address"
  , "ID"
  , "Short Name"
  , "Creator Application"
  , "Sector Size"
  , "Mode"
  , "Symbolic Link"
  , "Error"
  , "Total Size"
  , "Free Space"
  , "Cluster Size"
  , "Label"
  , "Local Name"
  , "Provider"
  , "NT Security"
  , "Alternate Stream"
  , "Aux"
  , "Deleted"
  , "Tree"
  , "SHA-1"
  , "SHA-256"
  , "Error Type"
  , "Errors"
  , "Errors"
  , "Warnings"
  , "Warning"
  , "Streams"
  , "Alternate Streams"
  , "Alternate Streams Size"
  , "Virtual Size"
  , "Unpack Size"
  , "Total Physical Size"
  , "Volume Index"
  , "SubType"
  , "Short Comment"
  , "Code Page"
  , "Is not archive type"
  , "Physical Size can't be detected"
  , "Zeros Tail Is Allowed"
  , "Tail Size"
  , "Embedded Stub Size"
  , "Link"
  , "Hard Link"
  , "iNode"
  , "Stream ID"
  , "Read-only"
  , "Out Name"
};
#endif

static void PrintPropName_and_Eq(FILE* f, PROPID propID)
{
  const char *s;
  char temp[16];
#if 0
  if (propID < sizeof(kPropIdToName)/sizeof(kPropIdToName[0]))
    s = kPropIdToName[propID];
  else
#endif
  {
    ConvertUInt32ToString(propID, temp);
    s = temp;
  }
  fprintf (f, "%s = ", s);
}

static void PrintPropNameAndNumber(FILE* f, PROPID propID, UInt64 val)
{
  PrintPropName_and_Eq(f, propID);
  fprintf (f, "%llu\n", val);
}

static void PrintPropNameAndNumber_Signed(FILE* f, PROPID propID, Int64 val)
{
  PrintPropName_and_Eq(f, propID);
  fprintf (f, "%lld\n", val);
}

static void PrintPropPair(FILE* f, const char *name, const wchar_t *val)
{
  fprintf (f, "%s = %ls\n", name, val);
}

static void GetPropName(PROPID propID, const wchar_t *name, AString &nameA, UString &nameU)
{
#if 0
  if (propID < sizeof(kPropIdToName)/sizeof(kPropIdToName[0]))
  {
    nameA = kPropIdToName[propID];
    return;
  }
#endif
  if (name)
    nameU = name;
  else
  {
    char s[16];
    ConvertUInt32ToString(propID, s);
    nameA = s;
  }
}

static void PrintPropertyPair2(FILE* f, PROPID propID, const wchar_t *name, const CPropVariant &prop)
{
  UString s;
  ConvertPropertyToString2(s, prop, propID);
  if (!s.IsEmpty())
  {
    AString nameA;
    UString nameU;
    GetPropName(propID, name, nameA, nameU);
    if (!nameA.IsEmpty())
      PrintPropPair(f, nameA, s);
    else
      fprintf (f, "%ls = %ls\n", nameU.Ptr (), s.Ptr ());
  }
}

static HRESULT PrintArcProp(FILE* f, IInArchive *archive, PROPID propID, const wchar_t *name)
{
  CPropVariant prop;
  RINOK(archive->GetArchiveProperty(propID, &prop));
  PrintPropertyPair2(f, propID, name, prop);
  return S_OK;
}

static void PrintArcTypeError(FILE* f, const UString &type, bool isWarning)
{
  fprintf (f, "Open %s: : Can not open the file as [%ls] archive\n",
    (isWarning ? "WARNING" : "ERROR"), type.Ptr ());
}

static void PrintErrorFlags(FILE* f, const char *s, UInt32 errorFlags)
{
  if (errorFlags == 0)
    return;
  fprintf (f, "%s\n%s\n", s, GetOpenArcErrorMessage (errorFlags).Ptr());
}

static void ErrorInfo_Print(FILE* f, const CArcErrorInfo &er)
{
  PrintErrorFlags(f, "ERRORS:", er.GetErrorFlags());
  if (!er.ErrorMessage.IsEmpty())
    PrintPropPair(f, "ERROR", er.ErrorMessage);

  PrintErrorFlags(f, "WARNINGS:", er.GetWarningFlags());
  if (!er.WarningMessage.IsEmpty())
    PrintPropPair(f, "WARNING", er.WarningMessage);
}

static HRESULT Print_OpenArchive_Props(FILE* f, const CCodecs *codecs, const CArchiveLink &arcLink)
{
  FOR_VECTOR (r, arcLink.Arcs)
  {
    const CArc &arc = arcLink.Arcs[r];
    const CArcErrorInfo &er = arc.ErrorInfo;
    
    fprintf (f, "--\n");
    PrintPropPair(f, "Path", arc.Path);
    if (er.ErrorFormatIndex >= 0)
    {
      if (er.ErrorFormatIndex == arc.FormatIndex)
        fprintf (f, "Warning: The archive is open with offset\n");
      else
        PrintArcTypeError(f, codecs->GetFormatNamePtr(er.ErrorFormatIndex), true);
    }
    PrintPropPair(f, "Type", codecs->GetFormatNamePtr(arc.FormatIndex));
    
    ErrorInfo_Print(f, er);
    
    Int64 offset = arc.GetGlobalOffset();
    if (offset != 0)
      PrintPropNameAndNumber_Signed(f, kpidOffset, offset);
    IInArchive *archive = arc.Archive;
    RINOK(PrintArcProp(f, archive, kpidPhySize, NULL));
    if (er.TailSize != 0)
      PrintPropNameAndNumber(f, kpidTailSize, er.TailSize);
    UInt32 numProps;
    RINOK(archive->GetNumberOfArchiveProperties(&numProps));
    
    {
      for (UInt32 j = 0; j < numProps; j++)
      {
        CMyComBSTR name;
        PROPID propID;
        VARTYPE vt;
        RINOK(archive->GetArchivePropertyInfo(j, &name, &propID, &vt));
        RINOK(PrintArcProp(f, archive, propID, name));
      }
    }
    
    if (r != arcLink.Arcs.Size() - 1)
    {
      UInt32 numProps;
      fprintf (f, "----\n");
      if (archive->GetNumberOfProperties(&numProps) == S_OK)
      {
        UInt32 mainIndex = arcLink.Arcs[r + 1].SubfileIndex;
        for (UInt32 j = 0; j < numProps; j++)
        {
          CMyComBSTR name;
          PROPID propID;
          VARTYPE vt;
          RINOK(archive->GetPropertyInfo(j, &name, &propID, &vt));
          CPropVariant prop;
          RINOK(archive->GetProperty(mainIndex, propID, &prop));
          PrintPropertyPair2(f, propID, name, prop);
        }
      }
    }
  }
  return S_OK;
}

static HRESULT Print_OpenArchive_Error(FILE* f, const CCodecs *codecs, const CArchiveLink &arcLink)
{
  #ifndef _NO_CRYPTO
  if (arcLink.PasswordWasAsked)
    fprintf (f, L"Can not open encrypted archive. Wrong password?");
  else
  #endif
  {
    if (arcLink.NonOpen_ErrorInfo.ErrorFormatIndex >= 0)
    {
      fprintf (f, "%ls\n", arcLink.NonOpen_ArcPath.Ptr());
      PrintArcTypeError(f, codecs->Formats[arcLink.NonOpen_ErrorInfo.ErrorFormatIndex].Name, false);
    }
    else
    fprintf (f, "Can not open the file as archive");
  }

  fprintf (f, "\n\n");
  ErrorInfo_Print(f, arcLink.NonOpen_ErrorInfo);

  return S_OK;
}

void Add_Messsage_Pre_ArcType(UString &s, const char *pre, const wchar_t *arcType)
{
  s.Add_LF();
  s += pre;
  s += " as [";
  s += arcType;
  s += "] archive";
}

void Print_ErrorFormatIndex_Warning(FILE* f, const CCodecs *codecs, const CArc &arc)
{
  const CArcErrorInfo &er = arc.ErrorInfo;
  
  UString s = L"WARNING:\n";
  s += arc.Path;
  if (arc.FormatIndex == er.ErrorFormatIndex)
  {
    s.Add_LF();
    s += "The archive is open with offset";
  }
  else
  {
    Add_Messsage_Pre_ArcType(s, "Can not open the file", codecs->GetFormatNamePtr(er.ErrorFormatIndex));
    Add_Messsage_Pre_ArcType(s, "The file is open", codecs->GetFormatNamePtr(arc.FormatIndex));
  }

  fprintf (f, "%ls\n\n", s.Ptr ());
}

HRESULT CExtractCallback::OpenResult(
    const CCodecs *codecs, const CArchiveLink &arcLink,
    const wchar_t *name, HRESULT result)
{
  FOR_VECTOR (level, arcLink.Arcs)
  {
    const CArc &arc = arcLink.Arcs[level];
    const CArcErrorInfo &er = arc.ErrorInfo;
    
    UInt32 errorFlags = er.GetErrorFlags();

    if (errorFlags != 0 || !er.ErrorMessage.IsEmpty())
    {
      fprintf (stderr, "\n");
      if (level != 0)
        fprintf (stderr, "%ls", arc.Path.Ptr());
      
      if (errorFlags != 0)
      {
        PrintErrorFlags(stderr, "ERRORS:", errorFlags);
        NumOpenArcErrors++;
        ThereIsError_in_Current = true;
      }
      
      if (!er.ErrorMessage.IsEmpty())
      {
        fprintf (stderr, "ERRORS:\n%ls\n", er.ErrorMessage.Ptr ());
        NumOpenArcErrors++;
        ThereIsError_in_Current = true;
      }
      
      fprintf (stderr, "\n");
    }
    
    UInt32 warningFlags = er.GetWarningFlags();

    if (warningFlags != 0 || !er.WarningMessage.IsEmpty())
    {
      fprintf (stdout, "\n");
      if (level != 0)
        fprintf (stdout, "%ls", arc.Path.Ptr());
      
      if (warningFlags != 0)
      {
        PrintErrorFlags(stdout, "WARNINGS:", warningFlags);
        NumOpenArcWarnings++;
        ThereIsWarning_in_Current = true;
      }
      
      if (!er.WarningMessage.IsEmpty())
      {
        fprintf (stdout, "WARNINGS:\n%ls\n", er.WarningMessage.Ptr ());
        NumOpenArcWarnings++;
        ThereIsWarning_in_Current = true;
      }
      
      fprintf (stdout, "\n");
    }

  
    if (er.ErrorFormatIndex >= 0)
    {
      Print_ErrorFormatIndex_Warning(stdout, codecs, arc);
      ThereIsWarning_in_Current = true;
    }
  }
      
  if (result == S_OK)
  {
    RINOK(Print_OpenArchive_Props(stdout, codecs, arcLink));
    fprintf (stdout, "\n");
  }
  else
  {
    NumCantOpenArcs++;
    fprintf (stderr, "%s%ls\n", kError, name);
    HRESULT res = Print_OpenArchive_Error(stderr, codecs, arcLink);
    RINOK(res);
    if (result == S_FALSE)
    {
    }
    else
    {
      if (result == E_OUTOFMEMORY)
        fprintf (stderr, "Can't allocate required memory");
      else
        fprintf (stderr, "%ls", NError::MyFormatMessage(result).Ptr());
      fprintf (stderr, "\n");
    }
  }
  
  
  return CheckBreak2();
}
  
HRESULT CExtractCallback::ThereAreNoFiles()
{
  fprintf (stdout, "\n%s\n", kNoFiles);
  return CheckBreak2();
}

HRESULT CExtractCallback::ExtractResult(HRESULT result)
{
  MT_LOCK
  
  if (result == S_OK)
  {
    if (NumFileErrors_in_Current == 0 && !ThereIsError_in_Current)
    {
      if (ThereIsWarning_in_Current)
        NumArcsWithWarnings++;
      else
        NumOkArcs++;
      fprintf (stdout, "%s\n", kEverythingIsOk);
    }
    else
    {
      NumArcsWithError++;
      fprintf (stdout, "\n");
      if (NumFileErrors_in_Current != 0)
        fprintf (stdout, "Sub items Errors: %llu\n", NumFileErrors_in_Current);
    }
  }
  else
  {
    NumArcsWithError++;
    if (result == E_ABORT || result == ERROR_DISK_FULL)
      return result;
    
    fprintf (stderr, "\n%s", kError);
    if (result == E_OUTOFMEMORY)
      fprintf (stderr, "%s", kMemoryExceptionMessage);
    else
      fprintf (stderr, "%ls", NError::MyFormatMessage(result).Ptr());
    fprintf (stderr, "\n");
  }

  return CheckBreak2();
}

bool CExtractCallback::AnyErrors() const
{
  return NumArcsWithError > 0;
}
