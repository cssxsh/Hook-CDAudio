#include "wave.h"

#define IMPLEMENT_WAVEOUT(name) \
    name = reinterpret_cast<WaveOut##name##>(GetProcAddress(winmm, "waveOut" #name));

WaveOutFunctionTable::WaveOutFunctionTable(const HMODULE winmm)
{
    IMPLEMENT_WAVEOUT(GetNumDevs);
    IMPLEMENT_WAVEOUT(GetDevCapsA);
    IMPLEMENT_WAVEOUT(GetDevCapsW);
    IMPLEMENT_WAVEOUT(GetVolume);
    IMPLEMENT_WAVEOUT(SetVolume);
    IMPLEMENT_WAVEOUT(GetErrorTextA);
    IMPLEMENT_WAVEOUT(GetErrorTextW);
    IMPLEMENT_WAVEOUT(Open);
    IMPLEMENT_WAVEOUT(Close);
    IMPLEMENT_WAVEOUT(PrepareHeader);
    IMPLEMENT_WAVEOUT(UnprepareHeader);
    IMPLEMENT_WAVEOUT(Write);
    IMPLEMENT_WAVEOUT(Pause);
    IMPLEMENT_WAVEOUT(Restart);
    IMPLEMENT_WAVEOUT(Reset);
    IMPLEMENT_WAVEOUT(BreakLoop);
    IMPLEMENT_WAVEOUT(GetPosition);
    IMPLEMENT_WAVEOUT(GetPitch);
    IMPLEMENT_WAVEOUT(SetPitch);
    IMPLEMENT_WAVEOUT(GetPlaybackRate);
    IMPLEMENT_WAVEOUT(SetPlaybackRate);
    IMPLEMENT_WAVEOUT(GetID);
    IMPLEMENT_WAVEOUT(Message);
#ifdef UNICODE
    GetDevCaps = GetDevCapsW;
    GetErrorText = GetErrorTextW;
#else
    GetDevCaps = GetDevCapsA;
    GetErrorText = GetErrorTextA;
#endif // !UNICODE
}