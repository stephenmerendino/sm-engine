#include "sm/core/assert.h"

#include "Engine/Platform/WindowsInclude.h" //???/

#include <cstdio>
#include <cstdlib>

#define MAX_ASSERT_MSG_LEN 1024

bool report_assert_failure_msg(const char* expression, const char* msg, const char* filename, int line_number)
{
	char assert_msg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assert_msg, "%s\n\nFile: %s\nLine %i\nExpression \"%s\" failed.\n\nWould you like to debug? (Cancel quits program)", msg, filename, line_number, expression);

	// Convert from Multi-Byte to Unicode string
	size_t num_chars_converted = 0;
	wchar_t converted_unicode_str[MAX_ASSERT_MSG_LEN];
	mbstowcs_s(&num_chars_converted, converted_unicode_str, assert_msg, MAX_ASSERT_MSG_LEN);

	int user_btn_pressed = MessageBoxEx(NULL, (LPCWSTR)converted_unicode_str, L"Assertion Failed", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (user_btn_pressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (user_btn_pressed == IDYES);
}

bool report_assert_failure(const char* expression, const char* filename, int line_number)
{
	char assert_msg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assert_msg, "File: %s\nLine %i\nExpression \"%s\" failed.\n\nWould you like to debug? (Cancel quits program)", filename, line_number, expression);

	// Convert from Multi-Byte to Unicode string
	size_t num_chars_converted = 0;
	wchar_t converted_unicode_str[MAX_ASSERT_MSG_LEN];
	mbstowcs_s(&num_chars_converted, converted_unicode_str, assert_msg, MAX_ASSERT_MSG_LEN);

	int user_btn_pressed = MessageBoxEx(NULL, (LPCWSTR)converted_unicode_str, L"Assertion Failed", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (user_btn_pressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (user_btn_pressed == IDYES);
}

bool report_error_msg(const char* msg, const char* filename, int line_number)
{
	char assert_msg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assert_msg, "%s\n\nError triggered at File: %s\nLine %i\n\nWould you like to debug? (Cancel quits program)", msg, filename, line_number);

	// Convert from Multi-Byte to Unicode string
	size_t num_chars_converted = 0;
	wchar_t converted_unicode_str[MAX_ASSERT_MSG_LEN];
	mbstowcs_s(&num_chars_converted, converted_unicode_str, assert_msg, MAX_ASSERT_MSG_LEN);

	int user_btn_pressed = MessageBoxEx(NULL, (LPCWSTR)converted_unicode_str, L"Error Triggered", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (user_btn_pressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (user_btn_pressed == IDYES);
}

bool report_error(const char* filename, int line_number)
{
	char assert_msg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assert_msg, "Error triggered at File: %s\nLine %i\n\nWould you like to debug? (Cancel quits program)", filename, line_number);

	// Convert from Multi-Byte to Unicode string
	size_t num_chars_converted = 0;
	wchar_t converted_unicode_str[MAX_ASSERT_MSG_LEN];
	mbstowcs_s(&num_chars_converted, converted_unicode_str, assert_msg, MAX_ASSERT_MSG_LEN);

	int user_btn_pressed = MessageBoxEx(NULL, (LPCWSTR)converted_unicode_str, L"Error Triggered", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (user_btn_pressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (user_btn_pressed == IDYES);
}
