#ifndef HOOK_H
#define HOOK_H

#ifdef _DETOURS_H_
#pragma comment(lib, "detours.lib")
#endif // _DETOURS_H_

typedef UINT (WINAPI *WaveOutGetNumDevs)();

typedef MMRESULT (WINAPI *WaveOutGetDevCapsA)(UINT, LPWAVEOUTCAPSA, UINT);

typedef MMRESULT (WINAPI *WaveOutGetVolume)(HWAVEOUT, LPDWORD);

typedef MMRESULT (WINAPI *WaveOutSetVolume)(HWAVEOUT, DWORD);

typedef MMRESULT (WINAPI *WaveOutGetErrorTextA)(MMRESULT, LPSTR, UINT);

typedef MMRESULT (WINAPI *WaveOutOpen)(LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD, DWORD, DWORD);

typedef MMRESULT (WINAPI *WaveOutClose)(HWAVEOUT);

typedef MMRESULT (WINAPI *WaveOutPrepareHeader)(HWAVEOUT, LPWAVEHDR, UINT);

typedef MMRESULT (WINAPI *WaveOutUnprepareHeader)(HWAVEOUT, LPWAVEHDR, UINT);

typedef MMRESULT (WINAPI *WaveOutWrite)(HWAVEOUT, LPWAVEHDR, UINT);

typedef MMRESULT (WINAPI *WaveOutPause)(HWAVEOUT);

typedef MMRESULT (WINAPI *WaveOutRestart)(HWAVEOUT);

typedef MMRESULT (WINAPI *WaveOutReset)(HWAVEOUT);

typedef MMRESULT (WINAPI *WaveOutBreakLoop)(HWAVEOUT);

typedef MMRESULT (WINAPI *WaveOutGetPosition)(HWAVEOUT, LPMMTIME, UINT);

typedef MMRESULT (WINAPI *WaveOutGetPitch)(HWAVEOUT, LPDWORD);

typedef MMRESULT (WINAPI *WaveOutSetPitch)(HWAVEOUT, DWORD);

typedef MMRESULT (WINAPI *WaveOutGetPlaybackRate)(HWAVEOUT, LPDWORD);

typedef MMRESULT (WINAPI *WaveOutSetPlaybackRate)(HWAVEOUT, DWORD);

typedef MMRESULT (WINAPI *WaveOutGetID)(HWAVEOUT, LPUINT);

typedef MMRESULT (WINAPI *WaveOutMessage)(HWAVEOUT, UINT, DWORD, DWORD);

struct WaveOutFunctionTable
{
    WaveOutGetNumDevs GetNumDevs;
    WaveOutGetDevCapsA GetDevCapsA;
    WaveOutGetVolume GetVolume;
    WaveOutSetVolume SetVolume;
    WaveOutGetErrorTextA GetErrorText;
    WaveOutOpen Open;
    WaveOutClose Close;
    WaveOutPrepareHeader PrepareHeader;
    WaveOutUnprepareHeader UnprepareHeader;
    WaveOutWrite Write;
    WaveOutPause Pause;
    WaveOutRestart Restart;
    WaveOutReset Reset;
    WaveOutBreakLoop BreakLoop;
    WaveOutGetPosition GetPosition;
    WaveOutGetPitch GetPitch;
    WaveOutSetPitch SetPitch;
    WaveOutGetPlaybackRate GetPlaybackRate;
    WaveOutSetPlaybackRate SetPlaybackRate;
    WaveOutGetID GetID;
    WaveOutMessage Message;

    explicit WaveOutFunctionTable(HMODULE winmm);
};

#define DECLARE_WARP(module, function) \
    PVOID module##_##function; \
    __declspec(naked) void WINAPIV _jmp_##module##_##function##(void) { __asm { jmp module##_##function } } \
    __pragma(comment(linker, "/EXPORT:" #function "=?_jmp_" #module "_" #function "@@YAXXZ"))
    
#define IMPLEMENT_WARP(module, function) \
    module##_##function = GetProcAddress(module, #function);

DECLARE_WARP(winmm, PlaySoundA)
DECLARE_WARP(winmm, mciSendCommandA)
DECLARE_WARP(winmm, mciSendStringA)
DECLARE_WARP(winmm, mixerOpen)
DECLARE_WARP(winmm, mixerClose)
DECLARE_WARP(winmm, mixerGetControlDetailsA)
DECLARE_WARP(winmm, mixerGetDevCapsA)
DECLARE_WARP(winmm, mixerGetLineControlsA)
DECLARE_WARP(winmm, mixerGetLineInfoA)
DECLARE_WARP(winmm, mixerGetNumDevs)
DECLARE_WARP(winmm, mixerSetControlDetails)
DECLARE_WARP(winmm, timeGetTime)

extern "C" __declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);

MCIERROR WINAPI mciSendCommandHook(MCIDEVICEID mciId, UINT uMsg, DWORD dwParam1, DWORD dwParam2);

MCIERROR WINAPI mciSendStringHook(LPCSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback);

#endif // HOOK_H
