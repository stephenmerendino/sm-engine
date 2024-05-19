#pragma once

#include <cstdlib>
#include <cstring>

inline 
wchar_t* AllocAndConvertUnicodeStr(const char* str)
{
	size_t strLen = strlen(str);
	size_t numCharsConverted = 0;
	wchar_t* unicodeStr = new wchar_t[strLen + 1]; // + 1 for null terminator
	mbstowcs_s(&numCharsConverted, unicodeStr, strLen + 1, str, strLen);
	return unicodeStr;
}
