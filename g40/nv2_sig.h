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

#include <g40/nv2_util.h>

namespace nv2
{

#if _IS_WINDOWS

	//-----------------------------------------------------------------------------
	// simple handler for CTRL+C, 
	class SigHandler
	{
	public:

		// constructor installs handler function
		SigHandler()
		{
			SetConsoleCtrlHandler(SigHandler::HandlerRoutine, TRUE);
		}

		//
		static bool Continue(bool* parg = 0)
		{
			static bool halt;
			if (parg)
			{
				halt = true;
			}
			return (halt == false);
		}

		//
		static BOOL WINAPI HandlerRoutine(_In_ DWORD dwCtrlType)
		{
			bool halt = true;
			switch (dwCtrlType)
			{
			case CTRL_C_EVENT:
				SigHandler::Continue(&halt);
				// Signal is handled - don't pass it on to the next handler
				return TRUE;
			default:
				// Pass signal on to the next handler
				return FALSE;
			}
		}

	};	// sig handler

#else

	//-----------------------------------------------------------------------------
	// simple handler for CTRL+C,
	class SigHandler
	{
	public:

		// constructor installs handler function
		SigHandler()
		{
			signal(SIGINT, SigHandler::HandlerRoutine);
			signal(SIGKILL, SigHandler::HandlerRoutine);
			signal(SIGTERM, SigHandler::HandlerRoutine);
		}

		//
		static bool Continue(bool* parg = 0)
		{
			static bool halt;
			if (parg)
			{
				halt = true;
			}
			return (halt == false);
		}

		//
		static void HandlerRoutine(int sig)
		{
			bool halt = true;
			SigHandler::Continue(&halt);
			// DBMSG("Got signal " << sig);
		}

	};	// sig handler
#endif

}	// nv2

