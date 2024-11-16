/*

    Some Win32 wrappers for simplicity and RAII.                                                

    Visit https://github.com/g40

    Copyright (c) Jerry Evans, 1999-2024

    All rights reserved.

    The MIT License (MIT)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

*/

#pragma once

#include <g40/nv2_util.h>

namespace uw32
{
    //---------------------------------------------------------------------
    // convert to human units
    static const uint64_t _1MB = 1024ull * 1024ull;
    static const uint64_t _1GB = 1024ull * 1024ull * 1024ull;

    //---------------------------------------------------------------------
    //
    static
        DWORD Win32FromHResult(HRESULT hr)
    {
        if ((hr & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0))
        {
            return HRESULT_CODE(hr);
        }

        if (hr == S_OK)
        {
            return ERROR_SUCCESS;
        }

        // Not a Win32 HRESULT so return a generic error code.
        return ERROR_CAN_NOT_COMPLETE;
    }

    //-------------------------------------------------------------------------
    static void
        trace_hresult(const char* p, HRESULT result)
    {
        if (result != S_OK) {
            DBMSG(p << nv2::s_error(Win32FromHResult(result)));
        }
    }

    //-------------------------------------------------------------------------
    //
    static void throw_on_fail(const char* p, bool failed) {
        if (failed)
            throw std::runtime_error(p);
    }

    //-------------------------------------------------------------------------
    // returns a GUID as in string: {dd4fbabe-eb72-11e4-9bf7-806e6f6e6963}
    static
    std::wstring
    to_string(const GUID& guid)
    {
        const int bufferSize = 128;
        wchar_t buffer[bufferSize] = { 0 };
        StringFromGUID2(guid, &buffer[0], bufferSize);
        return buffer;
    }

    //-------------------------------------------------------------------------
    // arg must be GUID delimited, i.e. {dd4fbabe-eb72-11e4-9bf7-806e6f6e6963}
    // Wrong: \\?\Volume{dd4fbabe-eb72-11e4-9bf7-806e6f6e6963}\ 
    // Right: dd4fbabe-eb72-11e4-9bf7-806e6f6e6963
    static
    GUID
    to_guid(const std::wstring& arg)
    {
        GUID guid{ 0 };
        size_t ib = arg.find_first_of(_T('{'));
        if (ib == std::string::npos)
        {
            throw std::runtime_error("uw32::to_guid No '{' found");
        }
        size_t ie = arg.find_last_of(_T('}'));
        if (ie == std::string::npos)
        {
            throw std::runtime_error("uw32::to_guid No '}' found");
        }
        //
        std::wstring vn = arg.substr(ib + 1, ie - ib - 1);
        //
        RPC_STATUS s = UuidFromString((RPC_WSTR)vn.c_str(), &guid);
        if (s != RPC_S_OK)
        {
            throw std::runtime_error("uw32::to_guid UuidFromString failed. Check arg format");
        }
        //
        return guid;
    }

    //-------------------------------------------------------------------------
    // RAII to ensure closure and release.
    class Handle
    {
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
    protected:

        HANDLE m_handle = INVALID_HANDLE_VALUE;
        DWORD m_dwError = 0;

    public:
        explicit Handle() noexcept {}
        explicit Handle(HANDLE handle) noexcept : m_handle(handle) {}
        ~Handle() {
            if (!(m_handle == 0 || m_handle == INVALID_HANDLE_VALUE)) 
            {
                BOOL ok = ::CloseHandle(m_handle);
                if (ok) 
                {
                    m_dwError =  0;
                }
                else
                {
                    m_dwError = ::GetLastError();
                }
                //
                m_handle = INVALID_HANDLE_VALUE;
            }
        }

        // if (handle) ...
        operator bool() const {
            return (m_handle != INVALID_HANDLE_VALUE);
        }

        // for use by foreign forces
        HANDLE handle() const {
            return m_handle;
        }

        DWORD GetErrorCode() const {
            return m_dwError;
        }
    };

    //-----------------------------------------------------------------------------
    // Wrapper for Windows synch handle
    class ManualHandle : public Handle
    {
        std::wstring m_tag = _T("man");

    public:

        explicit ManualHandle(BOOL signaled = FALSE, const char_t* tag = 0)
        {
            if (m_handle == INVALID_HANDLE_VALUE) {
                SECURITY_ATTRIBUTES* psa = nullptr;
                m_handle = CreateEvent(psa, TRUE, signaled, 0);
                if (tag) {
                    m_tag = tag;
                }
            }
        }

        // wait on the thread handle
        DWORD Wait(DWORD dwTimeout = INFINITE) {
            return WaitForSingleObject(m_handle, dwTimeout);
        }

        bool Set() {
            return (SetEvent(m_handle) == TRUE);
        }

        bool Reset() {
            return (ResetEvent(m_handle) == TRUE);
        }
    };

    //-----------------------------------------------------------------------------
    // 
    class FileHandle : public Handle
    {
        void _Create(const wchar_t* filename,
            DWORD dwAccess,
            DWORD dwShare,
            DWORD dwMode,
            DWORD dwAttribute)
        {
            //
            SECURITY_ATTRIBUTES sa;
            sa.nLength = sizeof(sa);
            sa.lpSecurityDescriptor = NULL;
            sa.bInheritHandle = 0;

            // attempt to open file
            m_handle = ::CreateFileW(filename,
                dwAccess,
                dwShare,
                &sa,
                dwMode,
                dwAttribute,
                NULL);
            if (m_handle == INVALID_HANDLE_VALUE)
            {
                m_dwError = ::GetLastError();
            }
        }

    public:

        //
    
        //
        explicit FileHandle(const wchar_t* filename,
                    DWORD dwAccess = GENERIC_READ|GENERIC_WRITE,
                    DWORD dwShare = FILE_SHARE_READ| FILE_SHARE_WRITE,
                    DWORD dwMode = OPEN_EXISTING,
                    DWORD dwAttrib = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING)
        {
            _Create(filename,dwAccess,dwShare,dwMode,dwAttrib);
        }
        //
        explicit FileHandle(const std::wstring& filename,
            DWORD dwAccess = GENERIC_READ | GENERIC_WRITE,
            DWORD dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE,
            DWORD dwMode = OPEN_EXISTING,
            DWORD dwAttrib = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING)
        {
            _Create(filename.c_str(), dwAccess, dwShare, dwMode, dwAttrib);
        }
    };

    //-----------------------------------------------------------------------------
    static
    BOOL IsProcessElevated()
    {

        BOOL fIsElevated = FALSE;
        HANDLE hToken = NULL;
        TOKEN_ELEVATION elevation{0};
        DWORD dwSize = 0;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            printf("\n Failed to get Process Token :%d.", GetLastError());
            goto Cleanup;  // if Failed, we treat as False
        }

        if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
        {
            printf("\nFailed to get Token Information :%d.", GetLastError());
            goto Cleanup;// if Failed, we treat as False
        }

        fIsElevated = elevation.TokenIsElevated;

    Cleanup:
        if (hToken)
        {
            CloseHandle(hToken);
            hToken = NULL;
        }
        return fIsElevated;
    }

}   // uw32
