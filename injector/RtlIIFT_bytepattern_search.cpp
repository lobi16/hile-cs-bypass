/*
*	OXWARE developed by oxiKKK
*	Copyright (c) 2023
*
*	This program is licensed under the MIT license. By downloading, copying,
*	installing or using this software you agree to this license.
*
*	License Agreement
*
*	Permission is hereby granted, free of charge, to any person obtaining a
*	copy of this software and associated documentation files (the "Software"),
*	to deal in the Software without restriction, including without limitation
*	the rights to use, copy, modify, merge, publish, distribute, sublicense,
*	and/or sell copies of the Software, and to permit persons to whom the
*	Software is furnished to do so, subject to the following conditions:
*
*	The above copyright notice and this permission notice shall be included
*	in all copies or substantial portions of the Software.
*
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
*	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
*	IN THE SOFTWARE.
*/

//
// RtlIIFT_bytepattern_search.cpp -- Helper class for searching for RtlInsertInvertedFunctionTable inside ntdll.
//

#include "precompiled.h"

bool RtlIIFT_BytePattern_Search::resolve_bytepatterns()
{
	// damn, it sucks that Microsoft didn't export this function.. Oh well, whatever. Now we need to do all this shit, 
	// which isn't guaranteed that it will work every time...

	if (!m_RtlIIFT_bytepattern.empty())
	{
		return true;
	}

	// byte pattern for RtlInsertInvertedFunctionTable inside ntdll.
	CBytePatternWithLengthConstexpr<k_max_mmapper_pattern_length> RtlIIFT_pattern;

	//-----------------------------------------------------
	// Windows 10 - newer builds
	// This byte pattern is the same on:
	//	ntdll.dll (10.0.19041.2788)
	//	ntdll.dll (10.0.18362.387)
	//	ntdll.dll (10.0.17134.254)
	RtlIIFT_pattern = CBytePatternWithLengthConstexpr<k_max_mmapper_pattern_length>("\x8B\xFF\x55\x8B\xEC\x83\xEC\x0C\x53\x56\x57\x8D\x45\xF8\x8B\xFA");
	if (try_to_find_function_in_ntdll(RtlIIFT_pattern))
	{
		m_RtlIIFT_bytepattern = RtlIIFT_pattern.get_pattern_raw();
		m_RtlIIFT_bytepattern_mask = RtlIIFT_pattern.get_mask();
		CConsole::the().info("Found byte pattern for newer Win10 RtlInsertInvertedFunctionTable: '{}'", RtlIIFT_pattern.pattern_as_string());
		return true;
	}

	//-----------------------------------------------------
	// Windows 10 - older builds
	// This byte pattern is the same on:
	//	ntdll.dll (10.0.14393.0)
	//	ntdll.dll (10.0.10586.20)
	//	ntdll.dll (10.0.10240.16430)
	RtlIIFT_pattern = CBytePatternWithLengthConstexpr<k_max_mmapper_pattern_length>("\x8B\xFF\x55\x8B\xEC\x83\xEC\x0C\x8D\x45\xF8\x53\x56\x57\x8B");
	if (try_to_find_function_in_ntdll(RtlIIFT_pattern))
	{
		m_RtlIIFT_bytepattern = RtlIIFT_pattern.get_pattern_raw();
		m_RtlIIFT_bytepattern_mask = RtlIIFT_pattern.get_mask();
		CConsole::the().info("Found byte pattern for older Win10 RtlInsertInvertedFunctionTable: '{}'", RtlIIFT_pattern.pattern_as_string());
		return true;
	}

	auto ntdll_path = CGenericUtil::the().get_system_directory("ntdll.dll");

	auto ntdll_ver = CGenericUtil::the().get_file_version(ntdll_path.string());

	CMessageBox::display_error("Couldn't find RtlInsertInvertedFunctionTable function for your system."
							   "\nThis function is mandatory. Aborting injection...\n\n"
							   "Your Windows version is: {}\nntdll.dll version: {}\n\n"
							   "Please, report this error to the developers of this cheat in order to make this cheat available for your system.", 
							   CGenericUtil::the().get_os_version_str(),
							   ntdll_ver.to_string());
	return false;
}

bool RtlIIFT_BytePattern_Search::try_to_find_function_in_ntdll(const CBytePatternWithLengthConstexpr<k_max_mmapper_pattern_length>& pattern)
{
	DWORD ntdll_base = (DWORD)GetModuleHandleA("ntdll.dll");
	DWORD size_of_ntdll_image = (DWORD)((PIMAGE_NT_HEADERS)((uint8_t*)ntdll_base + ((PIMAGE_DOS_HEADER)ntdll_base)->e_lfanew))->OptionalHeader.SizeOfImage;

	if (!pattern.search_in_loaded_address_space(ntdll_base, ntdll_base + size_of_ntdll_image))
	{
		return false;
	}

	return true;
}
