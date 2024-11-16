/*

   Portable memory mapped files for Windows and POSIX compliant OS.

	Visit https://github.com/g40

	Copyright (c) Jerry Evans, 1995-2024

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

//-----------------------------------------------------------------------------
// Simple read-only file mapper. i.e. access via data()
//-----------------------------------------------------------------------------

namespace nv2
{

template<typename T>
class MMapFile
{
#if _IS_WINDOWS
	//
	typedef HANDLE FILE_HANDLE;

	// always returns false
	bool SaveError()
	{
		return false;
	}

	// open an existing read-only file in mapped mode
	bool _Open(const char_t* pszFilename)
	{
		m_size = 0;
		
		//
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = 0;

		// attempt to open file
		m_hFile = ::CreateFile(pszFilename,
			GENERIC_READ,
			FILE_SHARE_READ,
			&sa,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH, NULL);
		if (m_hFile == INVALID_HANDLE_VALUE)
		{
			return SaveError();
		}

		// what is the file size?
		LARGE_INTEGER liFileSize;
		if (::GetFileSizeEx(m_hFile, &liFileSize) == FALSE)
		{
			return SaveError();
		}
		m_size = liFileSize.QuadPart;
		// create the mapping
		m_hMapping = ::CreateFileMapping(m_hFile,&sa,PAGE_READONLY,0,0,0);
		if (m_hMapping == NULL)
		{
			SaveError();
			Close();
			return false;
		}

		// and actually map it ...
		m_pData = (T*) ::MapViewOfFile(m_hMapping,FILE_MAP_READ,0,0,0);
		if (m_pData == NULL)
		{
			SaveError();
			Close();
			return false;
		}
		// OK.
		return true;
	}

	//
	bool _Close()
	{
		bool ret = true;
		if (m_pData != NULL)
		{
			ret &= (::UnmapViewOfFile(m_pData) == TRUE);
			m_pData = NULL;
		}
		if (m_hMapping != NULL)
		{
			ret &= (::CloseHandle(m_hMapping) == TRUE);
			m_hMapping = NULL;
		}
		if (m_hFile != INVALID_HANDLE_VALUE)
		{
			ret &= (::CloseHandle(m_hFile) == TRUE);
			m_hFile = INVALID_HANDLE_VALUE;
		}
		return ret;
	}

#else
	//
	typedef int FILE_HANDLE;
	
	bool _Open(const char_t* pszFilename)
	{
		m_hFile = open(pszFilename,O_RDONLY);
		if (m_hFile == -1) 
		{
			return false;
		}

		struct stat sbuf;
		if (stat(pszFilename,&sbuf) == -1) 
		{
			Close();
			return false;
		}
		
		//
		void* p = mmap(0,sbuf.st_size,PROT_READ,MAP_SHARED,m_hFile,0);
		if (p == 0)
		{
			Close();
			return false;
		}
		//
		m_size = sbuf.st_size;
		m_pData = reinterpret_cast<T*>(p);
		//
		return true;
	}

	// close up and clear file handle
	bool _Close()
	{
		bool ret = false;
		if (m_hFile > 0)
		{
			// close the handle, check return
			ret = (close(m_hFile) == 0);
			m_hFile = 0;
		}
		return ret;
	}

#endif

	T* m_pData;
	uint64_t m_size;
	uint64_t m_position;
	FILE_HANDLE m_hFile;
	FILE_HANDLE m_hMapping;
	
public:

	//
	MMapFile<T>()
		: m_pData(0), m_size(0), m_position(0), m_hFile(0), m_hMapping(0)
	{
	}

	//
	~MMapFile()
	{
		Close();
	}

	//--------------------------------------------------------
	// open the mapping
	bool Open(const char_t* pszFilename)
	{
		return _Open(pszFilename);
	}

	//--------------------------------------------------------
	// open the mapping
	bool Open(const string_t& filename)
	{
		return _Open(filename.c_str());
	}

	//--------------------------------------------------------
	// close it ...
	bool Close()
	{
		return _Close();
	}

	//--------------------------------------------------------
	//
	bool IsOpen() const
	{
		return (m_pData != 0 && m_hMapping != 0 && m_hFile != 0);
	}

	//--------------------------------------------------------
	// actually get the data. const so you get the idea writing will cause an exception
	const T* data() const
	{
		return m_pData;
	}

	//--------------------------------------------------------
	// total size
	uint64_t size() const 
	{ 
		return m_size; 
	}

	//--------------------------------------------------------
	// pointer to start of mapping
	const T* begin() const
	{
		return m_pData;
	}

	//--------------------------------------------------------
	// pointer to end of mapping
	const T* end() const
	{
		return m_pData + size();
	}

	//--------------------------------------------------------
	// any error message
	const char_t* GetLastError() const	
	{ 
		return 0; 
	}
	
};	// class


}	// namespace

