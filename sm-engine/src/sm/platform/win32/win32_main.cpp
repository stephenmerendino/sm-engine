#include "sm/platform/win32/win32_include.h"
#include "sm/core/helpers.h"

#include "sm/app.h"

int WINAPI WinMain(_In_		HINSTANCE app, 
				   _In_opt_ HINSTANCE prevApp, 
				   _In_		LPSTR args, 
				   _In_		int show)
{
	UNUSED(app);
	UNUSED(prevApp);
	UNUSED(args);
	UNUSED(show);

	app_run();

	return 0;
}
