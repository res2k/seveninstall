#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#define ExitTrace LogErrorString
#define ExitTrace1 LogErrorString
#define ExitTrace2 LogErrorString
#define ExitTrace3 LogErrorString

#include <windows.h>
#include <strsafe.h>
#include <intsafe.h>

#include "dutil.h"
#include "buffutil.h"
#include "fileutil.h"
#include "logutil.h"
#include "memutil.h"
#include "regutil.h"
#include "strutil.h"

#include "pipe.h"
