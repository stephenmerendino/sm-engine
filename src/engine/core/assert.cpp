#include "engine/core/Assert.h"
#include "engine/platform/windows_include.h"

#include <cstdio>
#include <cstdlib>

bool report_assert_failure_msg(const char* expression, const char* msg, const char* filename, int line_number)
{
	char assert_msg[1024];
	sprintf_s(assert_msg, "%s\n\nFile: %s\nLine %i\nExpression \"%s\" failed.\n\nWould you like to debug? (Cancel quits program)", msg, filename, line_number, expression);
	int user_btn_pressed = MessageBoxEx(NULL, assert_msg, "Assertion Failed", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (user_btn_pressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (user_btn_pressed == IDYES);
}

bool report_assert_failure(const char* expression, const char* filename, int line_number)
{
	char assert_msg[1024];
	sprintf_s(assert_msg, "File: %s\nLine %i\nExpression \"%s\" failed.\n\nWould you like to debug? (Cancel quits program)", filename, line_number, expression);
	int user_btn_pressed = MessageBoxEx(NULL, assert_msg, "Assertion Failed", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (user_btn_pressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (user_btn_pressed == IDYES);
}

bool report_error_msg(const char* msg, const char* filename, int line_number)
{
	char assert_msg[1024];
	sprintf_s(assert_msg, "%s\n\nError triggered at File: %s\nLine %i\n\nWould you like to debug? (Cancel quits program)", msg, filename, line_number);
	int user_btn_pressed = MessageBoxEx(NULL, assert_msg, "Error Triggered", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (user_btn_pressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (user_btn_pressed == IDYES);
}

bool report_error(const char* filename, int line_number)
{
	char assert_msg[1024];
	sprintf_s(assert_msg, "Error triggered at File: %s\nLine %i\n\nWould you like to debug? (Cancel quits program)", filename, line_number);
	int user_btn_pressed = MessageBoxEx(NULL, assert_msg, "Error Triggered", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (user_btn_pressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (user_btn_pressed == IDYES);
}
