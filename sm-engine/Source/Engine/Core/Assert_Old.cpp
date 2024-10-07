#include "Engine/Core/Assert.h"
#include "Engine/Platform/WindowsInclude.h"

#include <cstdio>
#include <cstdlib>

#define MAX_ASSERT_MSG_LEN 1024

bool ReportAssertFailureMsg(const char* expression, const char* msg, const char* filename, int lineNumber)
{
	char assertMsg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assertMsg, "%s\n\nFile: %s\nLine %i\nExpression \"%s\" failed.\n\nWould you like to debug? (Cancel quits program)", msg, filename, lineNumber, expression);

	// Convert from Multi-Byte to Unicode string
	size_t numCharsConverted = 0;
	wchar_t convertedUnicodeStr[MAX_ASSERT_MSG_LEN];
	mbstowcs_s(&numCharsConverted, convertedUnicodeStr, assertMsg, MAX_ASSERT_MSG_LEN);

	int userBtnPressed = MessageBoxEx(NULL, (LPCWSTR)convertedUnicodeStr, L"Assertion Failed", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (userBtnPressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (userBtnPressed == IDYES);
}

bool ReportAssertFailure(const char* expression, const char* filename, int lineNumber)
{
	char assertMsg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assertMsg, "File: %s\nLine %i\nExpression \"%s\" failed.\n\nWould you like to debug? (Cancel quits program)", filename, lineNumber, expression);

	// Convert from Multi-Byte to Unicode string
	size_t numCharsConverted = 0;
	wchar_t convertedUnicodeStr[MAX_ASSERT_MSG_LEN];
	mbstowcs_s(&numCharsConverted, convertedUnicodeStr, assertMsg, MAX_ASSERT_MSG_LEN);

	int userBtnPressed= MessageBoxEx(NULL, (LPCWSTR)convertedUnicodeStr, L"Assertion Failed", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (userBtnPressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (userBtnPressed == IDYES);
}

bool ReportErrorMsg(const char* msg, const char* filename, int lineNumber)
{
	char assertMsg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assertMsg, "%s\n\nError triggered at File: %s\nLine %i\n\nWould you like to debug? (Cancel quits program)", msg, filename, lineNumber);

	// Convert from Multi-Byte to Unicode string
	size_t numCharsConverted = 0;
	wchar_t convertedUnicodeStr[MAX_ASSERT_MSG_LEN];
	mbstowcs_s(&numCharsConverted, convertedUnicodeStr, assertMsg, MAX_ASSERT_MSG_LEN);

	int userBtnPressed = MessageBoxEx(NULL, (LPCWSTR)convertedUnicodeStr, L"Error Triggered", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (userBtnPressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (userBtnPressed == IDYES);
}

bool ReportError(const char* filename, int lineNumber)
{
	char assert_msg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assert_msg, "Error triggered at File: %s\nLine %i\n\nWould you like to debug? (Cancel quits program)", filename, lineNumber);

	// Convert from Multi-Byte to Unicode string
	size_t numCharsConverted = 0;
	wchar_t convertedUnicodeStr[MAX_ASSERT_MSG_LEN];
	mbstowcs_s(&numCharsConverted, convertedUnicodeStr, assert_msg, MAX_ASSERT_MSG_LEN);

	int userBtnPressed = MessageBoxEx(NULL, (LPCWSTR)convertedUnicodeStr, L"Error Triggered", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (userBtnPressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (userBtnPressed == IDYES);
}
