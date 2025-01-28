#ifndef WAVE_H
#define WAVE_H

#include <windows.h>

typedef UINT (WINAPI *WaveOutGetNumDevs)();

typedef MMRESULT (WINAPI *WaveOutGetDevCapsA)(UINT, LPWAVEOUTCAPSA, UINT);

typedef MMRESULT (WINAPI *WaveOutGetDevCapsW)(UINT, LPWAVEOUTCAPSW, UINT);

typedef MMRESULT (WINAPI *WaveOutGetVolume)(HWAVEOUT, LPDWORD);

typedef MMRESULT (WINAPI *WaveOutSetVolume)(HWAVEOUT, DWORD);

typedef MMRESULT (WINAPI *WaveOutGetErrorTextA)(MMRESULT, LPSTR, UINT);

typedef MMRESULT (WINAPI *WaveOutGetErrorTextW)(MMRESULT, LPWSTR, UINT);

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
    WaveOutGetDevCapsW GetDevCapsW;
    WaveOutGetVolume GetVolume;
    WaveOutSetVolume SetVolume;
    WaveOutGetErrorTextA GetErrorTextA;
    WaveOutGetErrorTextW GetErrorTextW;
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
#ifdef UNICODE
    WaveOutGetDevCapsW GetDevCaps;
    WaveOutGetErrorTextW GetErrorText;
#else
    WaveOutGetDevCapsA GetDevCaps;
    WaveOutGetErrorTextA GetErrorText;
#endif // !UNICODE

    explicit WaveOutFunctionTable(HMODULE winmm);
};

#endif // WAVE_H
