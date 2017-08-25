#include "base/win/dump.h"

#include <windows.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

bool DumpException(const base::char16* path, const EXCEPTION_POINTERS& exc) {
	HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return FALSE;

	MINIDUMP_EXCEPTION_INFORMATION info;
	info.ThreadId = GetCurrentThreadId();
	info.ClientPointers = FALSE;
	info.ExceptionPointers = (EXCEPTION_POINTERS*)&exc;

	BOOL res = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
		MiniDumpWithDataSegs /*MiniDumpWithFullMemory*/, &info, NULL, NULL);

	CloseHandle(file);
	return res != 0;
}
