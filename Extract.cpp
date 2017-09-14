/*** This file is based on code from 7zip ***/

#include "Extract.hpp"
#include "Common/MyInitGuid.h" // Must be placed before any header originating from 7zip 

#include "Windows/DLL.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"

#include "7zip/ICoder.h"
#include "7zip/UI/Common/ExitCode.h"
#include "7zip/UI/Common/Extract.h"
#include "7zip/UI/Common/ExtractingFilePath.h"
#include "7zip/UI/Common/SetProperties.h"

#include "7zip/UI/Console/ConsoleClose.h"

#include "7zip/MyVersion.h"

#include "7zCrc.h"

#include <iostream>

#include "Error.hpp"
#include "ExtractCallback.hpp"
#include "OpenCallback.hpp"

using namespace NWindows;
using namespace NFile;
using namespace NDir;

const char extractCopyright[] = "Based on 7zip " MY_VERSION " : Portions " MY_COPYRIGHT " : " MY_DATE;

static HRESULT DecompressArchive(
    CCodecs *codecs,
    const CArchiveLink &arcLink,
    UInt64 packSize,
    const CExtractOptions &options,
    bool calcCrc,
    IExtractCallbackUI *callback,
    CArchiveExtractCallback *ecs,
    UString &errorMessage)
{
  const CArc &arc = arcLink.Arcs.Back();
  IInArchive *archive = arc.Archive;
  CRecordVector<UInt32> realIndices;

  UStringVector removePathParts;

  FString outDir = options.OutputDir;
  UString replaceName = arc.DefaultName;

  outDir.Replace(FSTRING_ANY_MASK, us2fs(Get_Correct_FsFile_Name(replaceName)));

  UInt32 numItems;
  RINOK(archive->GetNumberOfItems(&numItems));

  #ifdef SUPPORT_ALT_STREAMS
  CReadArcItem item;
  #endif

  for (UInt32 i = 0; i < numItems; i++)
  {
    #ifdef SUPPORT_ALT_STREAMS
    item.IsAltStream = false;
    if (!options.NtOptions.AltStreams.Val && arc.Ask_AltStream)
    {
      RINOK(Archive_IsItem_AltStream(arc.Archive, i, item.IsAltStream));
    }
    if (!options.NtOptions.AltStreams.Val && item.IsAltStream)
      continue;
    #endif

    realIndices.Add(i);
  }

  if (realIndices.Size() == 0)
  {
    callback->ThereAreNoFiles();
    return callback->ExtractResult(S_OK);
  }

  #ifdef _WIN32
  // GetCorrectFullFsPath doesn't like "..".
  // outDir.TrimRight();
  // outDir = GetCorrectFullFsPath(outDir);
  #endif

  if (outDir.IsEmpty())
    outDir = FString(FTEXT(".")) + FString(FSTRING_PATH_SEPARATOR);
  else
    if (!CreateComplexDir(outDir))
    {
      HRESULT res = ::GetLastError();
      if (res == S_OK)
        res = E_FAIL;
      errorMessage = ((UString)L"Can not create output directory ") + fs2us(outDir);
      return res;
    }

  ecs->Init(
      options.NtOptions,
      NULL,
      &arc,
      callback,
      options.StdOutMode, options.TestMode,
      outDir,
      removePathParts, false,
      packSize);

  #ifdef SUPPORT_LINKS

  if (!options.TestMode &&
      options.NtOptions.HardLinks.Val)
  {
    RINOK(ecs->PrepareHardLinks(&realIndices));
  }

  #endif

  HRESULT result;
  Int32 testMode = (options.TestMode && !calcCrc) ? 1: 0;
  result = archive->Extract(&realIndices.Front(), realIndices.Size(), testMode, ecs);
  if (result == S_OK)
    result = ecs->SetDirsTimes();
  return callback->ExtractResult(result);
}

static void ExtractOneArchive (
    CCodecs *codecs,
    const CObjectVector<COpenType> &types,
    const UString& arcPath,
    const CExtractOptions &options,
    IOpenCallbackUI *openCallback,
    IExtractCallbackUI *extractCallback,
    #ifndef _SFX
    IHashCalc *hash,
    #endif
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

  CArchiveExtractCallback *ecs = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> ec(ecs);
  ecs->InitForMulti(false, options.PathMode, options.OverwriteMode);
  #ifndef _SFX
  ecs->SetHashMethods(hash);
  #endif

  CHECK_HR(extractCallback->BeforeOpen(arcPath, options.TestMode));
  
  CArchiveLink arcLink;

  CIntVector excludedFormats;
  COpenOptions op;
  #ifndef _SFX
  op.props = &options.Properties;
  #endif
  op.codecs = codecs;
  op.types = &types;
  op.excludedFormats = &excludedFormats;
  op.stdInMode = false;
  op.stream = NULL;
  op.filePath = arcPath;
  HRESULT result = arcLink.Open3(op, openCallback);
  if (result == E_ABORT)
    CHECK_HR(result);

  if (arcLink.NonOpen_ErrorInfo.ErrorFormatIndex >= 0)
    result = S_FALSE;

  CHECK_HR(extractCallback->OpenResult(codecs, arcLink, arcPath, result));

  if (arcLink.VolumePaths.Size() != 0)
  {
    totalPackSize += arcLink.VolumesSize;
    CHECK_HR(extractCallback->SetTotal(totalPackSize));
  }

  CHECK_HR(result);

  CArc &arc = arcLink.Arcs.Back();
  arc.MTimeDefined = !fi.IsDevice;
  arc.MTime = fi.MTime;

  bool calcCrc =
      #ifndef _SFX
        (hash != NULL);
      #else
        false;
      #endif

  result = DecompressArchive(codecs, arcLink,
      fi.Size + arcLink.VolumesSize,
      options, calcCrc, extractCallback, ecs, errorMessage);
  ecs->LocalProgressSpec->InSize += fi.Size + arcLink.VolumesSize;
  ecs->LocalProgressSpec->OutSize = ecs->UnpackSize;

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

  CObjectVector<COpenType> types;

  CExtractCallback* ecs = new CExtractCallback (extractedFiles, outputDir);
  CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;

  COpenCallback openCallback;

  CExtractOptions eo;
  eo.StdOutMode = false;
  eo.PathMode = NExtract::NPathMode::kFullPaths;
  eo.TestMode = false;
  eo.OverwriteMode = NExtract::NOverwriteMode::kAsk;
  eo.OutputDir = outputDir;
  eo.YesToAll = false;

  for(const wchar_t* archivePath : archives)
  {
    UString errorMessage;

    ExtractOneArchive(
        codecs,
        types,
        archivePath,
        eo, &openCallback, ecs,
        #ifndef _SFX
        nullptr,
        #endif
        errorMessage);
    if (!errorMessage.IsEmpty())
    {
      ecs->MessageError (errorMessage);
      THROW_HR(E_FAIL);
    }
  }
}
