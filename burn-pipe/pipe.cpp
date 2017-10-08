// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"

static const DWORD PIPE_64KB = 64 * 1024;
static const DWORD PIPE_WAIT_FOR_CONNECTION = 100;   // wait a 10th of a second,
static const DWORD PIPE_RETRY_FOR_CONNECTION = 1800; // for up to 3 minutes.

static const LPCWSTR PIPE_NAME_FORMAT_STRING = L"\\\\.\\pipe\\%ls";
static const LPCWSTR CACHE_PIPE_NAME_FORMAT_STRING = L"\\\\.\\pipe\\%ls.Cache";

static HRESULT AllocatePipeMessage(
    __in DWORD dwMessage,
    __in_bcount_opt(cbData) LPVOID pvData,
    __in DWORD cbData,
    __out_bcount(cb) LPVOID* ppvMessage,
    __out DWORD* cbMessage
    );
static void FreePipeMessage(
    __in BURN_PIPE_MESSAGE *pMsg
    );
static HRESULT WritePipeMessage(
    __in HANDLE hPipe,
    __in DWORD dwMessage,
    __in_bcount_opt(cbData) LPVOID pvData,
    __in DWORD cbData
    );
static HRESULT GetPipeMessage(
    __in HANDLE hPipe,
    __in BURN_PIPE_MESSAGE* pMsg
    );
static HRESULT ChildPipeConnected(
    __in HANDLE hPipe,
    __in_z LPCWSTR wzSecret,
    __inout DWORD* pdwProcessId
    );



/*******************************************************************
 PipeConnectionInitialize - initialize pipe connection data.

*******************************************************************/
void PipeConnectionInitialize(
    __in BURN_PIPE_CONNECTION* pConnection
    )
{
    memset(pConnection, 0, sizeof(BURN_PIPE_CONNECTION));
    pConnection->hPipe = INVALID_HANDLE_VALUE;
    pConnection->hCachePipe = INVALID_HANDLE_VALUE;
}

/*******************************************************************
 PipeConnectionUninitialize - free data in a pipe connection.

*******************************************************************/
void PipeConnectionUninitialize(
    __in BURN_PIPE_CONNECTION* pConnection
    )
{
    ReleaseFileHandle(pConnection->hCachePipe);
    ReleaseFileHandle(pConnection->hPipe);
    ReleaseHandle(pConnection->hProcess);
    ReleaseStr(pConnection->sczSecret);
    ReleaseStr(pConnection->sczName);

    memset(pConnection, 0, sizeof(BURN_PIPE_CONNECTION));
    pConnection->hPipe = INVALID_HANDLE_VALUE;
    pConnection->hCachePipe = INVALID_HANDLE_VALUE;
}

/*******************************************************************
 PipeSendMessage - 

*******************************************************************/
extern "C" HRESULT PipeSendMessage(
    __in HANDLE hPipe,
    __in DWORD dwMessage,
    __in_bcount_opt(cbData) LPVOID pvData,
    __in DWORD cbData,
    __in_opt PFN_PIPE_MESSAGE_CALLBACK pfnCallback,
    __in_opt LPVOID pvContext,
    __out DWORD* pdwResult
    )
{
    HRESULT hr = S_OK;
    BURN_PIPE_RESULT result = { };

    hr = WritePipeMessage(hPipe, dwMessage, pvData, cbData);
    ExitOnFailure(hr, "Failed to write send message to pipe.");

    hr = PipePumpMessages(hPipe, pfnCallback, pvContext, &result);
    ExitOnFailure(hr, "Failed to pump messages during send message to pipe.");

    *pdwResult = result.dwResult;

LExit:
    return hr;
}

/*******************************************************************
 PipePumpMessages - 

*******************************************************************/
extern "C" HRESULT PipePumpMessages(
    __in HANDLE hPipe,
    __in_opt PFN_PIPE_MESSAGE_CALLBACK pfnCallback,
    __in_opt LPVOID pvContext,
    __in BURN_PIPE_RESULT* pResult
    )
{
    HRESULT hr = S_OK;
    BURN_PIPE_MESSAGE msg = { };
    SIZE_T iData = 0;
    LPSTR sczMessage = NULL;
    DWORD dwResult = 0;

    // Pump messages from child process.
    while (S_OK == (hr = GetPipeMessage(hPipe, &msg)))
    {
        switch (msg.dwMessage)
        {
        case BURN_PIPE_MESSAGE_TYPE_LOG:
            iData = 0;

            hr = BuffReadStringAnsi((BYTE*)msg.pvData, msg.cbData, &iData, &sczMessage);
            ExitOnFailure(hr, "Failed to read log message.");

            hr = LogStringWorkRaw(sczMessage);
            ExitOnFailure1(hr, "Failed to write log message:'%hs'.", sczMessage);

            dwResult = static_cast<DWORD>(hr);
            break;

        case BURN_PIPE_MESSAGE_TYPE_COMPLETE:
            if (!msg.pvData || sizeof(DWORD) != msg.cbData)
            {
                hr = E_INVALIDARG;
                ExitOnRootFailure(hr, "No status returned to PipePumpMessages()");
            }

            pResult->dwResult = *static_cast<DWORD*>(msg.pvData);
            ExitFunction1(hr = S_OK); // exit loop.

        case BURN_PIPE_MESSAGE_TYPE_TERMINATE:
            iData = 0;

            hr = BuffReadNumber(static_cast<BYTE*>(msg.pvData), msg.cbData, &iData, &pResult->dwResult);
            ExitOnFailure(hr, "Failed to read returned result to PipePumpMessages()");

            if (sizeof(DWORD) * 2 == msg.cbData)
            {
                hr = BuffReadNumber(static_cast<BYTE*>(msg.pvData), msg.cbData, &iData, (DWORD*)&pResult->fRestart);
                ExitOnFailure(hr, "Failed to read returned restart to PipePumpMessages()");
            }

            ExitFunction1(hr = S_OK); // exit loop.

        default:
            if (pfnCallback)
            {
                hr = pfnCallback(&msg, pvContext, &dwResult);
            }
            else
            {
                hr = E_INVALIDARG;
            }
            ExitOnFailure1(hr, "Failed to process message: %u", msg.dwMessage);
            break;
        }

        // post result
        hr = WritePipeMessage(hPipe, static_cast<DWORD>(BURN_PIPE_MESSAGE_TYPE_COMPLETE), &dwResult, sizeof(dwResult));
        ExitOnFailure(hr, "Failed to post result to child process.");

        FreePipeMessage(&msg);
    }
    ExitOnFailure(hr, "Failed to get message over pipe");

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

LExit:
    ReleaseStr(sczMessage);
    FreePipeMessage(&msg);

    return hr;
}

/*******************************************************************
 PipeChildConnect - Called from the child process to connect back
                    to the pipe provided by the parent process.

*******************************************************************/
extern "C" HRESULT PipeChildConnect(
    __in BURN_PIPE_CONNECTION* pConnection,
    __in BOOL fConnectCachePipe
    )
{
    Assert(pConnection->sczName);
    Assert(pConnection->sczSecret);
    Assert(!pConnection->hProcess);
    Assert(INVALID_HANDLE_VALUE == pConnection->hPipe);
    Assert(INVALID_HANDLE_VALUE == pConnection->hCachePipe);

    HRESULT hr = S_OK;
    LPWSTR sczPipeName = NULL;

    // Try to connect to the parent.
    hr = StrAllocFormatted(&sczPipeName, PIPE_NAME_FORMAT_STRING, pConnection->sczName);
    ExitOnFailure(hr, "Failed to allocate name of parent pipe.");

    hr = E_UNEXPECTED;
    for (DWORD cRetry = 0; FAILED(hr) && cRetry < PIPE_RETRY_FOR_CONNECTION; ++cRetry)
    {
        pConnection->hPipe = ::CreateFileW(sczPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (INVALID_HANDLE_VALUE == pConnection->hPipe)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            if (E_FILENOTFOUND == hr) // if the pipe isn't created, call it a timeout waiting on the parent.
            {
                hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            }

            ::Sleep(PIPE_WAIT_FOR_CONNECTION);
        }
        else // we have a connection, go with it.
        {
            hr = S_OK;
        }
    }
    ExitOnRootFailure1(hr, "Failed to open parent pipe: %ls", sczPipeName)

    // Verify the parent and notify it that the child connected.
    hr = ChildPipeConnected(pConnection->hPipe, pConnection->sczSecret, &pConnection->dwProcessId);
    ExitOnFailure1(hr, "Failed to verify parent pipe: %ls", sczPipeName);

    if (fConnectCachePipe)
    {
        // Connect to the parent for the cache pipe.
        hr = StrAllocFormatted(&sczPipeName, CACHE_PIPE_NAME_FORMAT_STRING, pConnection->sczName);
        ExitOnFailure(hr, "Failed to allocate name of parent cache pipe.");

        pConnection->hCachePipe = ::CreateFileW(sczPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (INVALID_HANDLE_VALUE == pConnection->hCachePipe)
        {
            ExitWithLastError1(hr, "Failed to open parent pipe: %ls", sczPipeName)
        }

        // Verify the parent and notify it that the child connected.
        hr = ChildPipeConnected(pConnection->hCachePipe, pConnection->sczSecret, &pConnection->dwProcessId);
        ExitOnFailure1(hr, "Failed to verify parent pipe: %ls", sczPipeName);
    }

    pConnection->hProcess = ::OpenProcess(SYNCHRONIZE, FALSE, pConnection->dwProcessId);
    ExitOnNullWithLastError1(pConnection->hProcess, hr, "Failed to open companion process with PID: %u", pConnection->dwProcessId);

LExit:
    ReleaseStr(sczPipeName);

    return hr;
}


static HRESULT AllocatePipeMessage(
    __in DWORD dwMessage,
    __in_bcount_opt(cbData) LPVOID pvData,
    __in DWORD cbData,
    __out_bcount(cb) LPVOID* ppvMessage,
    __out DWORD* cbMessage
    )
{
    HRESULT hr = S_OK;
    LPVOID pv = NULL;
    DWORD cb = 0;

    // If no data was provided, ensure the count of bytes is zero.
    if (!pvData)
    {
        cbData = 0;
    }

    // Allocate the message.
    cb = sizeof(dwMessage) + sizeof(cbData) + cbData;
    pv = MemAlloc(cb, FALSE);
    ExitOnNull(pv, hr, E_OUTOFMEMORY, "Failed to allocate memory for message.");

    memcpy_s(pv, cb, &dwMessage, sizeof(dwMessage));
    memcpy_s(static_cast<BYTE*>(pv) + sizeof(dwMessage), cb - sizeof(dwMessage), &cbData, sizeof(cbData));
    if (cbData)
    {
        memcpy_s(static_cast<BYTE*>(pv) + sizeof(dwMessage) + sizeof(cbData), cb - sizeof(dwMessage) - sizeof(cbData), pvData, cbData);
    }

    *cbMessage = cb;
    *ppvMessage = pv;
    pv = NULL;

LExit:
    ReleaseMem(pv);
    return hr;
}

static void FreePipeMessage(
    __in BURN_PIPE_MESSAGE *pMsg
    )
{
    if (pMsg->fAllocatedData)
    {
        ReleaseNullMem(pMsg->pvData);
        pMsg->fAllocatedData = FALSE;
    }
}

static HRESULT WritePipeMessage(
    __in HANDLE hPipe,
    __in DWORD dwMessage,
    __in_bcount_opt(cbData) LPVOID pvData,
    __in DWORD cbData
    )
{
    HRESULT hr = S_OK;
    LPVOID pv = NULL;
    DWORD cb = 0;

    hr = AllocatePipeMessage(dwMessage, pvData, cbData, &pv, &cb);
    ExitOnFailure(hr, "Failed to allocate message to write.");

    // Write the message.
    DWORD cbWrote = 0;
    DWORD cbTotalWritten = 0;
    while (cbTotalWritten < cb)
    {
        if (!::WriteFile(hPipe, pv, cb - cbTotalWritten, &cbWrote, NULL))
        {
            ExitWithLastError(hr, "Failed to write message type to pipe.");
        }

        cbTotalWritten += cbWrote;
    }

LExit:
    ReleaseMem(pv);
    return hr;
}

static HRESULT GetPipeMessage(
    __in HANDLE hPipe,
    __in BURN_PIPE_MESSAGE* pMsg
    )
{
    HRESULT hr = S_OK;
    DWORD rgdwMessageAndByteCount[2] = { };
    DWORD cb = 0;
    DWORD cbRead = 0;

    while (cbRead < sizeof(rgdwMessageAndByteCount))
    {
        if (!::ReadFile(hPipe, reinterpret_cast<BYTE*>(rgdwMessageAndByteCount) + cbRead, sizeof(rgdwMessageAndByteCount) - cbRead, &cb, NULL))
        {
            DWORD er = ::GetLastError();
            if (ERROR_MORE_DATA == er)
            {
                hr = S_OK;
            }
            else if (ERROR_BROKEN_PIPE == er) // parent process shut down, time to exit.
            {
                memset(rgdwMessageAndByteCount, 0, sizeof(rgdwMessageAndByteCount));
                hr = S_FALSE;
                break;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(er);
            }
            ExitOnRootFailure(hr, "Failed to read message from pipe.");
        }

        cbRead += cb;
    }

    pMsg->dwMessage = rgdwMessageAndByteCount[0];
    pMsg->cbData = rgdwMessageAndByteCount[1];
    if (pMsg->cbData)
    {
        pMsg->pvData = MemAlloc(pMsg->cbData, FALSE);
        ExitOnNull(pMsg->pvData, hr, E_OUTOFMEMORY, "Failed to allocate data for message.");

        if (!::ReadFile(hPipe, pMsg->pvData, pMsg->cbData, &cb, NULL))
        {
            ExitWithLastError(hr, "Failed to read data for message.");
        }

        pMsg->fAllocatedData = TRUE;
    }

LExit:
    if (!pMsg->fAllocatedData && pMsg->pvData)
    {
        MemFree(pMsg->pvData);
    }

    return hr;
}

static HRESULT ChildPipeConnected(
    __in HANDLE hPipe,
    __in_z LPCWSTR wzSecret,
    __inout DWORD* pdwProcessId
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczVerificationSecret = NULL;
    DWORD cbVerificationSecret = 0;
    DWORD dwVerificationProcessId = 0;
    DWORD dwRead = 0;
    DWORD dwAck = ::GetCurrentProcessId(); // send our process id as the ACK.
    DWORD cb = 0;

    // Read the verification secret.
    if (!::ReadFile(hPipe, &cbVerificationSecret, sizeof(cbVerificationSecret), &dwRead, NULL))
    {
        ExitWithLastError(hr, "Failed to read size of verification secret from parent pipe.");
    }

    if (255 < cbVerificationSecret / sizeof(WCHAR))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        ExitOnRootFailure(hr, "Verification secret from parent is too big.");
    }

    hr = StrAlloc(&sczVerificationSecret, cbVerificationSecret / sizeof(WCHAR) + 1);
    ExitOnFailure(hr, "Failed to allocate buffer for verification secret.");

    if (!::ReadFile(hPipe, sczVerificationSecret, cbVerificationSecret, &dwRead, NULL))
    {
        ExitWithLastError(hr, "Failed to read verification secret from parent pipe.");
    }

    // Verify the secrets match.
    if (CSTR_EQUAL != ::CompareStringW(LOCALE_NEUTRAL, 0, sczVerificationSecret, -1, wzSecret, -1))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        ExitOnRootFailure(hr, "Verification secret from parent does not match.");
    }

    // Read the verification process id.
    if (!::ReadFile(hPipe, &dwVerificationProcessId, sizeof(dwVerificationProcessId), &dwRead, NULL))
    {
        ExitWithLastError(hr, "Failed to read verification process id from parent pipe.");
    }

    // If a process id was not provided, we'll trust the process id from the parent.
    if (*pdwProcessId == 0)
    {
        *pdwProcessId = dwVerificationProcessId;
    }
    else if (*pdwProcessId != dwVerificationProcessId) // verify the ids match.
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        ExitOnRootFailure(hr, "Verification process id from parent does not match.");
    }

    // All is well, tell the parent process.
    if (!::WriteFile(hPipe, &dwAck, sizeof(dwAck), &cb, NULL))
    {
        ExitWithLastError(hr, "Failed to inform parent process that child is running.");
    }

LExit:
    ReleaseStr(sczVerificationSecret);
    return hr;
}
