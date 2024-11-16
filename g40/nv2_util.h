/*

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

#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <math.h>

// absolute lowest level header.
#ifdef _MSC_VER
#ifndef _IS_WINDOWS
#define _IS_WINDOWS 1
#endif
#else
// linux ...
#define DWORD unsigned long
#define CP_UTF8 0
#define __T(arg) arg
#define _T(arg) __T(arg)
#endif

#if _IS_WINDOWS
#include <tchar.h>
#include <mbctype.h>
#include <atlbase.h>
#include <atlcom.h>
#include <comdef.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

//-------------------------------------------------------------------------
// two macros ensures any macro passed will
// be expanded before being stringified
#define STRINGIZE_INT(x) #x
#define STRINGIZE(x) STRINGIZE_INT(x)
#define LFL __FILE__ "(" STRINGIZE(__LINE__) "): "

#ifdef _UNICODE
typedef wchar_t char_t;
typedef std::wstring string_t;
#else
typedef char char_t;
typedef std::string string_t;
#endif

// this is emphatically UNICODE only
#define __W(arg) L ## arg
#define _W(arg) __W(arg)

namespace nv2
{

	//-----------------------------------------------------------------------------
	// convert wide to ASCII (narrow)
	inline std::string w2n(const wchar_t* warg, int wchars, int lcid = CP_UTF8)
	{
		if (!warg || wchars <= 0)
			return std::string();
#if _IS_WINDOWS
		// wide character string to multibyte character string 
		const int nchars = WideCharToMultiByte(lcid, 0, warg, wchars, 0, 0, 0, 0);
		if (!nchars)
			return std::string();
		std::string nret(nchars,0);
		WideCharToMultiByte(lcid, 0, warg, wchars, &nret[0], nchars, 0, 0);
#else
		std::string nret(wchars, 0);
		wcstombs(&nret[0], warg, wchars);
#endif
		// OK
		return nret;
	}

	//-----------------------------------------------------------------------------
	inline std::string w2n(const std::wstring& arg, int lcid = CP_UTF8)
	{
		return w2n(arg.c_str(), static_cast<int>(arg.size()), lcid);
	}

	//-----------------------------------------------------------------------------
	// char to wchar_t (narrow to wide)
	inline std::wstring n2w(const char* narg, int nchars, int lcid = CP_UTF8)
	{
		if (!narg || nchars <= 0)
			return std::wstring();
#if _IS_WINDOWS
		// how large a buffer is required for the wide version?
		const int wchars = MultiByteToWideChar(lcid, 0, narg, nchars, 0, 0);
		if (!wchars)
			return std::wstring();
		// exclude '\0'
		std::wstring wret(wchars,0);
		// do the conversion
		MultiByteToWideChar(lcid, 0, narg, nchars, &wret[0], wchars);
#else
		std::wstring wret(nchars, 0);
		mbstowcs(&wret[0], narg, wret.size());
#endif		
		return wret;
	}

	//-----------------------------------------------------------------------------
	inline std::wstring n2w(const std::string& arg, int lcid = CP_UTF8)
	{
		return n2w(arg.c_str(), static_cast<int>(arg.size()), lcid);
	}

	//-----------------------------------------------------------------------------
	inline std::string t2n(const string_t& arg)
	{
#ifdef _UNICODE
		return w2n(arg);
#else
		return arg;
#endif // _UNICODE
	}

	//-----------------------------------------------------------------------------
	inline string_t n2t(const std::string& arg)
	{
#ifdef _UNICODE
		return n2w(arg);
#else
		return arg;
#endif // _UNICODE
	}

	//-----------------------------------------------------------------------------
	inline std::wstring t2w(const string_t& arg)
	{
#ifdef _UNICODE
		return arg;
#else
		return n2w(arg);
#endif // _UNICODE
	}

	//-----------------------------------------------------------------------------
	inline string_t w2t(const std::wstring& arg)
	{
#ifdef _UNICODE
		return arg;
#else
		return w2n(arg);
#endif // _UNICODE
	}

	//-----------------------------------------------------------------------------
	static
		std::string to_string(const std::wstring& wstr)
	{
		return w2n(wstr.c_str());
		// using convert_t = std::codecvt_utf8<wchar_t>;
		//static std::wstring_convert<convert_t, wchar_t> strconverter;
		//return strconverter.to_bytes(wstr);
	}

	//-----------------------------------------------------------------------------
	static
		std::wstring to_wstring(const std::string& str)
	{
		return n2w(str.c_str());
		// using convert_t = std::codecvt_utf8<wchar_t>;
		//static std::wstring_convert<convert_t, wchar_t> strconverter;
		//return strconverter.from_bytes(str);
	}

	//-----------------------------------------------------------------------------
	// must be integral type.
	template <typename I>
	static
		std::string
		to_hex(I w, size_t hex_len = sizeof(I) << 1)
	{
		static const char* digits = "0123456789ABCDEF";
		std::string rc(hex_len, '0');
		for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
			rc[i] = digits[(w >> j) & 0x0f];
		// prefix
		rc.insert(0, "0x");
		// trailing space.
		rc.push_back(' ');
		return rc;
	}

	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	// trim the right hand of the string of whitespace characters
	static std::string trim_right(const std::string& arg, const std::string& delim)
	{
		std::string ret = arg;
		if (delim.empty())
			return ret;
		size_t idx = ret.find_last_not_of(delim.c_str());
		if (idx != std::string::npos)
		{
			ret.erase(++idx);
		}
		return ret;
	}

	//-----------------------------------------------------------------------------
	// trim the right hand of the string of whitespace characters
	static std::string trim_left(const std::string& arg, const std::string& delim)
	{
		std::string ret = arg;
		if (delim.empty())
			return ret;
		size_t idx = ret.find_first_not_of(delim.c_str());
		if (idx != std::string::npos)
		{
			ret.erase(0, idx);
		}
		else
		{
			ret.erase();
		}
		return ret;
	}

	//-----------------------------------------------------------------------------
	// trim from both ends of the string
	inline std::string trim(const std::string& arg, const char* delim = " \t\r\n")
	{
		std::string ret = trim_left(arg, delim);
		ret = trim_right(ret, delim);
		return ret;
	}

	//-------------------------------------------------------------------------
	// split by one of these delimiters
	static std::vector<std::string> split(const std::string& arg, const char* delimiters)
	{
		size_t last = 0;
		size_t next = 0;
		std::vector<std::string> parts;
		while ((next = arg.find_first_of(delimiters, last)) != std::string::npos)
		{
			if (next - last > 0)
			{
				std::string part = trim(arg.substr(last, next - last));
				// 
				parts.push_back(part);
			}
			last = next + 1;
		}
		//
		if (last < arg.size())
		{
			std::string part = trim(arg.substr(last));
			parts.push_back(part);
		}
		//
		return parts;
	}

	//-----------------------------------------------------------------------------
	// trim the right hand of the string of whitespace characters
	static std::wstring trim_right(const std::wstring& arg, const wchar_t* delim = L" \t\r\n")
	{
		std::wstring ret = arg;
		if (!delim)
			return ret;
		size_t idx = ret.find_last_not_of(delim);
		if (idx != std::wstring::npos)
		{
			ret.erase(++idx);
		}
		return ret;
	}

	//-----------------------------------------------------------------------------
	// trim the right hand of the string of whitespace characters
	static std::wstring trim_left(const std::wstring& arg, const wchar_t* delim)
	{
		std::wstring ret = arg;
		if (!delim)
			return ret;
		size_t idx = ret.find_first_not_of(delim);
		if (idx != std::wstring::npos)
		{
			ret.erase(0, idx);
		}
		else
		{
			ret.erase();
		}
		return ret;
	}

	//-----------------------------------------------------------------------------
	// trim from both ends of the string
	inline std::wstring trim(const std::wstring& arg, const wchar_t* delim = L" \t\r\n")
	{
		std::wstring ret = trim_left(arg, delim);
		ret = trim_right(ret, delim);
		return ret;
	}

	//-------------------------------------------------------------------------
	// split by one of these delimiters
	static std::vector<std::wstring> split(const std::wstring& arg, const wchar_t* delimiters = L" \r\t\n")
	{
		size_t last = 0;
		size_t next = 0;
		std::vector<std::wstring> parts;
		while ((next = arg.find_first_of(delimiters, last)) != std::wstring::npos)
		{
			if (next - last > 0)
			{
				std::wstring part = trim(arg.substr(last, next - last));
				// 
				parts.push_back(part);
			}
			last = next + 1;
		}
		//
		if (last < arg.size())
		{
			std::wstring part = trim(arg.substr(last));
			parts.push_back(part);
		}
		//
		return parts;
	}

	//-----------------------------------------------------------------------------
	//
	template <typename T = std::string>
	inline T replace(const T& in,
		const T& replaceMe,
		const T& withMe)
	{
		T result;
		size_t beginPos = 0;
		size_t endPos = in.find(replaceMe);
		if (endPos == T::npos)
		{
			return in; // can't find anything to replace
		}

		if (replaceMe.empty())
		{
			return in; // can't replace nothing with something -- can do reverse
		}

		result.erase();
		while (endPos != T::npos)
		{
			// push the  part up to
			result += in.substr(beginPos, endPos - beginPos);
			result += withMe;
			beginPos = endPos + replaceMe.size();
			endPos = in.find(replaceMe, beginPos);
		}
		//
		result += in.substr(beginPos);
		//
		return result;
	}

	//-----------------------------------------------------------------------------
	template< typename... Args >
	static std::string sprintf(const char* format, Args... args)
	{
#if _IS_WINDOWS
#pragma warning ( suppress: 4996 )
		int length = _snprintf(nullptr, 0, format, args...);
#else		
		int length = ::snprintf(nullptr, 0, format, args...);
#endif		
		std::string ret(length + 1, 0);
#if _IS_WINDOWS
#pragma warning ( suppress: 4996 )
		_snprintf(&ret[0], ret.size(), format, args...);
#else
		::snprintf(&ret[0], ret.size(), format, args...);
#endif
		// trim trailing null from snprintf
		ret.pop_back();
		//
		return ret;
	}

	//-----------------------------------------------------------------------------
	// really simple light-weight string stream 
	class acc
	{
		std::string m_data;
	public:

		acc() {}

		//
		acc(const acc& arg) : m_data(arg.m_data) {}
		acc(const std::string& arg) : m_data(arg) {}
		acc(const std::wstring& arg) : m_data(nv2::to_string(arg)) {}

		//
		acc(const char* arg) 
		{
			if (arg)
				m_data = arg;
		}

		// for VS style debug aware code
		acc(const char* arg, int line)
		{
			m_data = arg;
			m_data += "(";
			m_data += std::to_string(line);
			m_data += "): ";
		}

		acc(const wchar_t* arg)
		{
			if (arg)
				m_data = to_string(arg);
		}

		//
		acc& operator=(const acc& arg)
		{
			if (!arg.m_data.empty())
				m_data = arg.m_data;
			return (*this);
		}
		//
		acc& operator=(const char* arg)
		{
			if (arg)
				m_data = arg;
			return (*this);
		}
		//
		acc& operator=(const wchar_t* arg)
		{
			if (arg)
				m_data = to_string(arg);
			return (*this);
		}

		//
		acc& operator<<(char* arg)
		{
			if (arg)
				m_data += arg;
			return (*this);
		}
		acc& operator<<(wchar_t* arg)
		{
			if (arg)
				m_data += nv2::to_string(arg);
			return (*this);
		}

		//
		acc& operator<<(const char* arg)
		{
			if (arg)
				m_data += arg;
			return (*this);
		}
		acc& operator<<(const wchar_t* arg)
		{
			if (arg)
				m_data += nv2::to_string(arg);
			return (*this);
		}

		acc& operator<<(const acc& arg)
		{
			if (!arg.m_data.empty())
				m_data += arg.m_data;
			return (*this);
		}
		acc& operator<<(const std::string& arg)
		{
			if (!arg.empty())
				m_data += arg;
			return (*this);
		}

		acc& operator<<(const std::wstring& arg)
		{
			if (!arg.empty())
				m_data += nv2::to_string(arg);
			return (*this);
		}
		//
		acc& operator<<(const char& arg)
		{
			m_data += arg;
			return (*this);
		}
		// 
		acc& operator<<(const wchar_t& arg)
		{
			m_data += nv2::to_string(std::wstring(1, arg));
			return (*this);
		}

		//
		acc& operator<<(const bool& arg)
		{
			m_data += (arg ? "true " : "false ");
			return (*this);
		}
		//
		acc& operator<<(void* arg)
		{
			uint64_t a = (uint64_t)arg;
			m_data += nv2::to_hex(a);
			return (*this);
		}
#if _IS_WINDOWS
		//-----------------------------------------------------------------------------
		acc& operator<<(GUID arg) {
			OLECHAR buffer[64] = { 0 };
			int ret = StringFromGUID2(arg, buffer, 64);
			if (ret)
				*this << buffer;
			return (*this);
		}
#endif
		//
		template <typename T>
		acc& operator<<(T arg)
		{
			m_data += std::to_string(arg);
			return (*this);
		}
		//
		std::string str() const
		{
			return m_data;
		}
		//
		const char* c_str() const { return m_data.c_str(); }
		//
		size_t size() const { return m_data.size(); }
		//
		std::wstring wstr()
		{
			return nv2::to_wstring(m_data);
		}
		const std::wstring wstr() const
		{
			return nv2::to_wstring(m_data);
		}
	};

	//-------------------------------------------------------------------------
	// does the directory exist?
	inline bool DirectoryExists(const string_t& arg)
	{
		bool ret = false;
#if _IS_WINDOWS
		typedef struct _stat64i32 portable_stat;
		portable_stat st;
		const unsigned short mode = _S_IFCHR | _S_IFDIR;
#ifdef _UNICODE
		if (_wstat(arg.c_str(), &st) == 0 && (st.st_mode & mode))
		{
			ret = true;
		}
#else
		if (_stat(arg.c_str(), &st) == 0 && (st.st_mode & mode))
		{
			ret = true;
		}
#endif
#else
#endif
		return ret;
	}

	//-------------------------------------------------------------------------
	// does the file exist?
	inline bool FileExists(const string_t& arg)
	{
		bool ret = false;
#if _IS_WINDOWS
		typedef struct _stat64i32 portable_stat;
		portable_stat st;
		const unsigned short mode = _S_IFCHR | _S_IFMT;
#ifdef _UNICODE
		if (_wstat(arg.c_str(), &st) == 0 && (st.st_mode & mode))
		{
			ret = true;
		}
#else
		if (_stat(arg.c_str(), &st) == 0 && (st.st_mode & mode))
		{
			ret = true;
		}
#endif
#else
#endif
		return ret;
	}

	//-----------------------------------------------------------------------------
	// Wide-only
	static
		std::wstring GetCurrentDirectory(char_t)
	{
		std::wstring ret;
#if _IS_WINDOWS
		// this is totally arbitrary
		const int bufferSize = 8 * 1204;
		// https://msdn.microsoft.com/en-us/library/sf98bd4y.aspx
		wchar_t* p = _wgetcwd(0, bufferSize);
		if (p)
		{
			// assign
			ret = p;
			// and delete temporary
			free(p);
		}
#endif
		//
		return ret;
	}

	//-----------------------------------------------------------------------------
	// Win32!
	static
		string_t exePath(bool chop = false)
	{
#if _IS_WINDOWS
		TCHAR buffer[MAX_PATH] = { 0 };
		GetModuleFileName(NULL, buffer, MAX_PATH);
		string_t ret(buffer);
		if (chop)
		{
			string_t::size_type pos = string_t(buffer).find_last_of(_T("\\/"));
			ret = ret.substr(0, pos);
		}
#else
		string_t ret = _T("path/to/executable");
#endif
		return ret;
	}

	//-----------------------------------------------------------------------------
	// Win32!
	static
		string_t exeName()
	{
#if _IS_WINDOWS
		string_t ret = exePath();
		string_t::size_type pos = ret.find_last_of(_T("\\/"));
		if (pos != string_t::npos)
			ret = ret.substr(pos + 1, ret.size());
#else
		string_t ret = _T("executable");
#endif
		return ret;
	}

	//-----------------------------------------------------------------------------
	// read into a string
	template <typename T>
	static
		T
		read_file(const std::string& ipname, bool)
	{
		T ret;
		// read file
		std::ifstream file(ipname, std::ios::in | std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			return ret;
		}
		// establish file size, allocate buffer
		std::streampos size = file.tellg();
		ret.resize(size);
		file.seekg(0, std::ios::beg);
		// read
		file.read((char*)ret.data(), (int)ret.size());
		file.close();
		// and return data
		return ret;
	}

	//-----------------------------------------------------------------------------
	template <typename T>
	static
		std::vector<T>
		read_file(const std::string& ipname)
	{
		std::vector<T> ret;
		// read file
		std::ifstream file(ipname, std::ios::in | std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			return ret;
		}
		// establish file size, allocate buffer
		std::streampos size = file.tellg();
		ret.resize(size);
		file.seekg(0, std::ios::beg);
		// read
		file.read((char*)ret.data(), (int)ret.size());
		file.close();
		// and return data
		return ret;
	}

	//-----------------------------------------------------------------------------
	// create a file from previously serialized data
	template <typename T>
	static
		bool
		create_file(const T& data, const std::string& opname)
	{
		std::ofstream opfile{ opname, std::ios::binary | std::ios::trunc };
		if (!opfile.good())
			return false;
		opfile.write(data.data(), data.size());
		return true;
	}

	//-----------------------------------------------------------------------------
	// create a file from previously serialized data
	template <typename T>
	static
		bool
		create_file(const std::vector<T>& data, const std::string& opname)
	{
		std::ofstream opfile{ opname, std::ios::binary | std::ios::trunc };
		if (!opfile.good())
			return false;
		opfile.write((const char*)data.data(), data.size());
		return true;
	}

#if _IS_WINDOWS
	//-----------------------------------------------------------------------------
	static
	void
	test()
	{
		//
		using pair_t = std::pair<std::wstring,int>;
		std::vector<pair_t> td = {
			{ L"Hello World \u00A9", CP_UTF8 },	// (C)
			{ L"Hello World \u00AE", CP_UTF7 },	// (R)
			{ L"Hello World \u00A3", CP_ACP },	// £
			{ L"Hello World \u0024", CP_ACP },		// $
			{ L"ANSI Hello World \u00A9", CP_ACP },	// (C)
			{ L"ANSI Hello World \u00AE", CP_ACP },	// (R)
		};

		bool ok = true;
		for (auto& s : td)
		{
			// does a round-trip produce the same string?
			std::wstring ws = s.first;
			std::string ns = nv2::w2n(ws,s.second);
			std::wstring ws2 = nv2::n2w(ns, s.second);
			ok &= (ws == ws2);
		}
	}
#endif

	//-----------------------------------------------------------------------------
	// get Windows error message text
	static std::string s_error(DWORD error)
	{
		std::string ret = nv2::to_hex<>(error);
#if _IS_WINDOWS
		LPVOID lpMsgBuf = nullptr;
		const DWORD len = ::FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&lpMsgBuf,
			0, NULL);
		if (len > 0 && lpMsgBuf)
		{
			LPSTR p = (LPSTR)lpMsgBuf;
			// create the return string
			ret += p;
			// feee system allocated buffer
			::LocalFree(lpMsgBuf);
			// clean up
			if (ret.size() && ret.back() == '\n')
				ret.pop_back();
			if (ret.size() && ret.back() == '\r')
				ret.pop_back();
		}
#else
		ret = strerror(error);
#endif
		//
		return ret;
	}

	//-----------------------------------------------------------------------------
	static
		void throw_if(bool failed, const nv2::acc& arg)
	{
		if (failed)
			throw std::runtime_error(arg.str());
	}

}

#if _IS_LINUX
#define _USE_CONSOLE
#endif

//-----------------------------------------------------------------------------
//
static void Debug(const nv2::acc& ac)
{
#if _IS_LINUX
	std::cout << ac.str();
#else
	std::wstring ws = ac.wstr();
#if defined(_USE_CONSOLE)
	std::wcout << ws;
#endif
	OutputDebugString(ws.c_str());
#endif
}

#define DBMSG(arg) Debug(nv2::acc(__FILE__,__LINE__) << arg << "\r\n")
#define DBMSG2(arg)
