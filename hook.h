#ifndef HOOK_H
#define HOOK_H

#include <windows.h>

extern "C" __declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);

void OutputDebugPrintfA(_Printf_format_string_ LPCSTR format, ...);

void OutputDebugPrintfW(_Printf_format_string_ LPCWSTR format, ...);

#ifdef UNICODE
#define OutputDebugPrintf  OutputDebugPrintfW
#else
#define OutputDebugPrintf  OutputDebugPrintfA
#endif // !UNICODE

MCIERROR WINAPI mciSendCommandHook(MCIDEVICEID mciId, UINT uMsg, DWORD dwParam1, DWORD dwParam2);

MCIERROR WINAPI mciSendStringHook(LPCSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback);

#endif // HOOK_H
