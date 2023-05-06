#include "engine/core/Assert.h"
#include "engine/platform/WindowsInclude.h"

#include <cstdio>
#include <cstdlib>

bool ReportAssertFailure(const char* expression, const char* fileName, int lineNumber)
{
	char assertMsg[1024];
	sprintf_s(assertMsg, "File: %s\nLine %i\nExpression \"%s\" failed.\n\nWould you like to debug? (Cancel quits program)", fileName, lineNumber, expression);
	int userButtonPressed = MessageBoxEx(NULL, assertMsg, "Assertion Failed", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (userButtonPressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (userButtonPressed == IDYES);
}
