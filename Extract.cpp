/*** This file is based on code from 7zip ***/

#include "Extract.hpp"
#include "Common/MyInitGuid.h" // Must be placed before any header originating from 7zip 

#include "Windows/DLL.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"

#include "7zip/UI/Common/ExitCode.h"
#include "7zip/UI/Common/Extract.h"
#include "7zip/UI/Common/ExtractingFilePath.h"
#include "7zip/UI/Common/SetProperties.h"

#include "7zip/UI/Console/ConsoleClose.h"
#include "7zip/UI/Console/OpenCallbackConsole.h"

#include "7zip/MyVersion.h"

#include "7zCrc.h"

#include <iostream>

#include "Error.hpp"
#include "ExtractCallback.hpp"

using namespace NWindows;
using namespace NFile;

int g_CodePage = CP_ACP;

const char extractCopyright[] = "Portions " MY_VERSION_COPYRIGHT_DATE;

static HRESULT DecompressArchive(
    const CArc &arc,
    UInt64 packSize,
    const CExtractOptions &options,
    IExtractCallbackUI *callback,
    CArchiveExtractCallback *extractCallbackSpec,
    UString &errorMessage)
{
  IInArchive *archive = arc.Archive;
  CRecordVector<UInt32> realIndices;
  UInt32 numItems;
  CHECK_HR(archive->GetNumberOfItems(&numItems));

  for (UInt32 i = 0; i < numItems; i++)
  {
    UString filePath;
    CHECK_HR(arc.GetItemPath(i, filePath));
    bool isFolder;
    CHECK_HR(IsArchiveItemFolder(archive, i, isFolder));
    realIndices.Add(i);
  }
  if (realIndices.Size() == 0)
  {
    callback->ThereAreNoFiles();
    return S_OK;
  }

  UStringVector removePathParts;

  FString outDir = options.OutputDir;
  outDir.Replace(FSTRING_ANY_MASK, us2fs(GetCorrectFsPath(arc.DefaultName)));
  #ifdef _WIN32
  // GetCorrectFullFsPath doesn't like "..".
  // outDir.TrimRight();
  // outDir = GetCorrectFullFsPath(outDir);
  #endif

  if (!outDir.IsEmpty())
    if (!NFile::NDirectory::CreateComplexDirectory(outDir))
    {
      HRESULT res = ::GetLastError();
      if (res == S_OK)
        res = E_FAIL;
      errorMessage = ((UString)L"Can not create output directory ") + fs2us(outDir);
      return res;
    }

  extractCallbackSpec->Init(
      NULL,
      &arc,
      callback,
      options.StdOutMode, options.TestMode, options.CalcCrc,
      outDir,
      removePathParts,
      packSize);

  #if !defined(_7ZIP_ST) && !defined(_SFX)
  CHECK_HR(SetProperties(archive, options.Properties));
  #endif

  HRESULT result;
  Int32 testMode = (options.TestMode && !options.CalcCrc) ? 1: 0;
  result = archive->Extract(&realIndices.Front(), realIndices.Size(), testMode, extractCallbackSpec);

  return callback->ExtractResult(result);
}

static void ExtractOneArchive (
    CCodecs *codecs,
    const UString& arcPath,
    const CExtractOptions &options,
    IOpenCallbackUI *openCallback,
    IExtractCallbackUI *extractCallback,
    UString &errorMessage)
{
  UInt64 totalPackSize = 0;

  NFile::NFind::CFileInfo fi;
  fi.Size = 0;
  const FString &arcPath_f = us2fs(arcPath);
  if (!fi.Find(arcPath_f)) THROW_HR(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
  if (fi.IsDir()) THROW_HR(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND/*ERROR_DIRECTORY_NOT_SUPPORTED - doc'ed but not defined*/));
  totalPackSize = fi.Size;
  UInt64 archiveSize = fi.Size;

  CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> ec(extractCallbackSpec);
  extractCallbackSpec->InitForMulti(false, options.PathMode, options.OverwriteMode);

  CHECK_HR(extractCallback->BeforeOpen(arcPath));
  
  CArchiveLink archiveLink;

  HRESULT result = archiveLink.Open2(codecs, CIntVector(), false, NULL, arcPath, openCallback);
  if (!SUCCEEDED(result))
  {
    CHECK_HR(extractCallback->OpenResult(arcPath, result, false));
    // return: below
  }

  if (archiveLink.VolumePaths.Size() != 0)
  {
    totalPackSize += archiveLink.VolumesSize;
    CHECK_HR(extractCallback->SetTotal(totalPackSize));
  }

  for (int v = 0; v < archiveLink.Arcs.Size(); v++)
  {
    const UString &s = archiveLink.Arcs[v].ErrorMessage;
    if (!s.IsEmpty())
    {
      extractCallback->MessageError(s);
      if (!FAILED(result)) result = E_FAIL;
    }
  }

  CHECK_HR(result);

  CArc &arc = archiveLink.Arcs.Back();
  arc.MTimeDefined = !fi.IsDevice;
  arc.MTime = fi.MTime;

  result = DecompressArchive(arc,
      fi.Size + archiveLink.VolumesSize,
      options, extractCallback, extractCallbackSpec, errorMessage);
  extractCallbackSpec->LocalProgressSpec->InSize += fi.Size + archiveLink.VolumesSize;
  extractCallbackSpec->LocalProgressSpec->OutSize = extractCallbackSpec->UnpackSize;
  CHECK_HR (result);
}

void Extract (const std::vector<const wchar_t*>& archives, const wchar_t* targetDir, std::vector<std::wstring>& extractedFiles)
{
  CrcGenerateTable();
  NConsoleClose::CCtrlHandlerSetter handle_control;

  UString outputDir;
  outputDir = targetDir;
  NName::NormalizeDirPathPrefix(outputDir);

  CCodecs *codecs = new CCodecs;
  CMyComPtr<IUnknown> compressCodecsInfo = codecs;
  CHECK_HR(codecs->Load());

  CExtractCallback* ecs = new CExtractCallback (extractedFiles, outputDir);
  CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;

  COpenCallbackConsole openCallback;
  openCallback.OutStream = nullptr; // Doesn't seem to be used if NO_CRYPTO is defined, anyway

  CExtractOptions eo;
  eo.StdOutMode = false;
  eo.PathMode = NExtract::NPathMode::kFullPathnames;
  eo.TestMode = false;
  eo.OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
  eo.OutputDir = outputDir;
  eo.YesToAll = false;

  for(const wchar_t* archivePath : archives)
  {
    UString errorMessage;

    ExtractOneArchive(
        codecs,
        archivePath,
        eo, &openCallback, ecs, errorMessage);
    if (!errorMessage.IsEmpty())
    {
      ecs->MessageError (errorMessage);
      THROW_HR(E_FAIL);
    }
  }
}
