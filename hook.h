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

class MciToWaveOut
{
    HWAVEOUT wave = nullptr;
    WaveOutFunctionTable* table = nullptr;
    LPWAVEHDR cache[0x10] = {};
    HWND callback = nullptr;
    BYTE current = 0x00;

    LPWAVEHDR Track(BYTE index);

public:
    MciToWaveOut();
    ~MciToWaveOut();

    MCIDEVICEID GetDeviceID() const;
    BYTE GetCurrentTrack() const;
    DWORD GetLength(BYTE track);
    MMRESULT Open();
    MMRESULT OpenNotify(HWND hwnd);
    MMRESULT Close();
    MMRESULT CloseNotify(HWND hwnd);
    MMRESULT Seek(BYTE track);
    MMRESULT SeekNotify(BYTE track, HWND hwnd);
    MMRESULT Play(BYTE track);
    MMRESULT PlayNotify(BYTE track, HWND hwnd);
    MMRESULT Stop();
    MMRESULT StopNotify(HWND hwnd);

private:
    static void CALLBACK Done(HDRVR hdr, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
};

MCIERROR WINAPI mciSendCommandHook(MCIDEVICEID mciId, UINT uMsg, DWORD dwParam1, DWORD dwParam2);

MCIERROR WINAPI mciSendStringHook(LPCSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback);

#endif // HOOK_H
