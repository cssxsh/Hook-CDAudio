// ReSharper disable CppParameterMayBeConst
#include <windows.h>
#include <stdio.h> // NOLINT(*-deprecated-headers)
#include "library.h"


HMODULE winmm = nullptr;

HWAVEOUT wave = nullptr;

HWND handle = nullptr;

LPWAVEHDR pwh = nullptr;

WaveOutFunctionTable* table = nullptr;

void CALLBACK WaveOutDone(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

BOOL WINAPI DllMain(HINSTANCE /*hInstance*/, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            TCHAR path[MAX_PATH];
            GetSystemDirectory(path, MAX_PATH);
            lstrcat(path, TEXT("\\winmm.dll"));
            winmm = LoadLibrary(path);
            if (winmm == nullptr) return FALSE;
            IMPLEMENT_PROXY(winmm, PlaySoundA)
            IMPLEMENT_PROXY(winmm, mciSendCommandA)
            IMPLEMENT_PROXY(winmm, mciSendStringA)
            IMPLEMENT_PROXY(winmm, mixerOpen)
            IMPLEMENT_PROXY(winmm, mixerClose)
            IMPLEMENT_PROXY(winmm, mixerGetControlDetailsA)
            IMPLEMENT_PROXY(winmm, mixerGetDevCapsA)
            IMPLEMENT_PROXY(winmm, mixerGetLineControlsA)
            IMPLEMENT_PROXY(winmm, mixerGetLineInfoA)
            IMPLEMENT_PROXY(winmm, mixerGetNumDevs)
            IMPLEMENT_PROXY(winmm, mixerSetControlDetails)
            IMPLEMENT_PROXY(winmm, timeGetTime)
            winmm_mciSendCommandA = mciSendCommandHook;
            winmm_mciSendStringA = mciSendStringHook;
            table = new WaveOutFunctionTable(winmm);
            pwh = new WAVEHDR;
        }
        break;
    case DLL_PROCESS_DETACH:
        {
            if (winmm == nullptr) return FALSE;
            delete pwh;
            delete table;
            FreeLibrary(winmm);
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    return TRUE;
}

WaveOutFunctionTable::WaveOutFunctionTable(HMODULE winmm)
{
    GetNumDevs = reinterpret_cast<WaveOutGetNumDevs>(GetProcAddress(winmm, "waveOutGetNumDevs"));
    GetDevCapsA = reinterpret_cast<WaveOutGetDevCapsA>(GetProcAddress(winmm, "waveOutGetDevCapsA"));
    GetVolume = reinterpret_cast<WaveOutGetVolume>(GetProcAddress(winmm, "waveOutGetVolume"));
    SetVolume = reinterpret_cast<WaveOutSetVolume>(GetProcAddress(winmm, "waveOutSetVolume"));
    GetErrorText = reinterpret_cast<WaveOutGetErrorTextA>(GetProcAddress(winmm, "waveOutGetErrorTextA"));
    Open = reinterpret_cast<WaveOutOpen>(GetProcAddress(winmm, "waveOutOpen"));
    Close = reinterpret_cast<WaveOutClose>(GetProcAddress(winmm, "waveOutClose"));
    PrepareHeader = reinterpret_cast<WaveOutPrepareHeader>(GetProcAddress(winmm, "waveOutPrepareHeader"));
    UnprepareHeader = reinterpret_cast<WaveOutUnprepareHeader>(GetProcAddress(winmm, "waveOutUnprepareHeader"));
    Write = reinterpret_cast<WaveOutWrite>(GetProcAddress(winmm, "waveOutWrite"));
    Pause = reinterpret_cast<WaveOutPause>(GetProcAddress(winmm, "waveOutPause"));
    Restart = reinterpret_cast<WaveOutRestart>(GetProcAddress(winmm, "waveOutRestart"));
    Reset = reinterpret_cast<WaveOutReset>(GetProcAddress(winmm, "waveOutReset"));
    BreakLoop = reinterpret_cast<WaveOutBreakLoop>(GetProcAddress(winmm, "waveOutBreakLoop"));
    GetPosition = reinterpret_cast<WaveOutGetPosition>(GetProcAddress(winmm, "waveOutGetPosition"));
    GetPitch = reinterpret_cast<WaveOutGetPitch>(GetProcAddress(winmm, "waveOutGetPitch"));
    SetPitch = reinterpret_cast<WaveOutSetPitch>(GetProcAddress(winmm, "waveOutSetPitch"));
    GetPlaybackRate = reinterpret_cast<WaveOutGetPlaybackRate>(GetProcAddress(winmm, "waveOutGetPlaybackRate"));
    SetPlaybackRate = reinterpret_cast<WaveOutSetPlaybackRate>(GetProcAddress(winmm, "waveOutSetPlaybackRate"));
    GetID = reinterpret_cast<WaveOutGetID>(GetProcAddress(winmm, "waveOutGetID"));
    Message = reinterpret_cast<WaveOutMessage>(GetProcAddress(winmm, "waveOutMessage"));
}

MCIERROR WINAPI mciSendCommandHook(MCIDEVICEID mciId, UINT uMsg, DWORD dwParam1, DWORD dwParam2)
{
    switch (uMsg)
    {
    case MCI_OPEN:
        {
            const auto params = reinterpret_cast<LPMCI_OPEN_PARMS>(dwParam2);
            switch (dwParam1)
            {
            case MCI_OPEN_TYPE:
                printf(">>MCI_OPEN %08X=(MCI_OPEN_TYPE) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("    wDeviceID=%08X => %08X,\n", params->wDeviceID, 0x00114514);
                printf("    lpstrDeviceType=%s,\n", params->lpstrDeviceType);
                printf("    lpstrElementName=%s,\n", params->lpstrElementName);
                printf("    lpstrAlias=%s\n", params->lpstrAlias);
                printf("}\n");
                params->wDeviceID = 0x00114514;
                break;
            case MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE:
                printf(">>MCI_OPEN %08X=(MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("    wDeviceID=%08X => %08X,\n", params->wDeviceID, 0x00114514);
                printf("    lpstrDeviceType=%s,\n", params->lpstrDeviceType);
                printf("    lpstrElementName=%s,\n", params->lpstrElementName);
                printf("    lpstrAlias=%s\n", params->lpstrAlias);
                printf("}\n");
                params->wDeviceID = 0x00114514;
                break;
            default:
                printf(">>MCI_OPEN %08X=??? \n", dwParam1);
                return -1;
            }
            fflush(stdout);
            
            constexpr auto format = WAVEFORMATEX{WAVE_FORMAT_PCM, 2, 44100, 2 * 16 / 8 * 44100, 2 * 16 / 8, 16, 0};
            const auto callback = reinterpret_cast<DWORD>(&WaveOutDone);
            const auto open = table->Open(&wave, WAVE_MAPPER, &format, callback, 0, CALLBACK_FUNCTION);
            printf("<<WaveOutOpen (%08X) => %p\n", open, wave);
            fflush(stdout);
            
            auto id = 0u;
            const auto get = table->GetID(wave, &id);
            printf("<<WaveOutGetID (%08X) => %08X\n", get, id);
            fflush(stdout);
        }
        break;
    case MCI_CLOSE:
        {
            const auto params = reinterpret_cast<LPMCI_GENERIC_PARMS>(dwParam2);
            switch (dwParam1)
            {
            case 0x00000000:
                printf(">>MCI_CLOSE %08X \n", dwParam1);
                break;
            case MCI_NOTIFY:
                printf(">>MCI_CLOSE %08X=(MCI_NOTIFY) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("}\n");
                break;
            case MCI_WAIT:
                printf(">>MCI_CLOSE %08X=(MCI_WAIT) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("}\n");
                break;
            default:
                printf(">>MCI_CLOSE %08X=??? \n", dwParam1);
                return -1;
            }
            fflush(stdout);
            
            const auto close = table->Close(wave);
            printf("<<WaveOutClose (%08X)\n", close);
            fflush(stdout);
        }
        break;
    case MCI_PLAY:
        {
            TCHAR buffer[MAX_PATH];
            const auto params = reinterpret_cast<LPMCI_PLAY_PARMS>(dwParam2);
            const auto track = MCI_TMSF_TRACK(params->dwFrom);
            switch (dwParam1)
            {
            case MCI_FROM | MCI_TO | MCI_NOTIFY:
                printf(">>MCI_PLAY %08X=(MCI_FROM | MCI_TO | MCI_NOTIFY) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("    dwFrom=%08X,\n", params->dwFrom);
                printf("    dwTo=%08X,\n", params->dwTo);
                printf("}\n");
                handle = reinterpret_cast<HWND>(params->dwCallback & 0x0000FFFF);
                break;
            case MCI_FROM | MCI_TO:
                printf(">>MCI_PLAY %08X=(MCI_TO) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("    dwFrom=%08X,\n", params->dwFrom);
                printf("    dwTo=%08X,\n", params->dwTo);
                printf("}\n");
                break;
            default:
                printf(">>MCI_PLAY %08X=??? \n", dwParam1);
                handle = nullptr;
                return -1;
            }
            fflush(stdout);
            
            pwh->reserved = MCI_NOTIFY_SUPERSEDED;
            const auto reset = table->Reset(wave);
            printf("<<WaveOutReset (%08X)\n", reset);
            fflush(stdout);
            
            sprintf(buffer, ".\\bgm\\Track%02d.wav", track);
            const auto file = fopen(buffer, "rb");
            if (file == nullptr)
            {
                MessageBox(nullptr, buffer, "Not Found File", MB_ICONERROR);
                return -1;
            }
            fseek(file, 0x00, SEEK_END);
            pwh->dwFlags = 0x00000000;
            pwh->dwBufferLength = static_cast<DWORD>(ftell(file) - 0x2C);
            pwh->lpData = static_cast<LPSTR>(malloc(pwh->dwBufferLength));
            pwh->reserved = MCI_NOTIFY_SUCCESSFUL;
            fseek(file, 0x2C, SEEK_SET);
            const auto size = fread(pwh->lpData, sizeof(BYTE), pwh->dwBufferLength, file);
            printf("<<fread(path=%s) => %08X\n", buffer, size);
            fclose(file);
            const auto prepare = table->PrepareHeader(wave, pwh, sizeof(WAVEHDR));
            printf("<<WaveOutPrepareHeader (%08X)\n", prepare);
            const auto write = table->Write(wave, pwh, sizeof(WAVEHDR));
            printf("<<WaveOutWrite (%08X)\n", write);
            fflush(stdout);
        }
        break;
    case MCI_SEEK:
        {
            const auto params = reinterpret_cast<LPMCI_SEEK_PARMS>(dwParam2);
            // const auto track = MCI_TMSF_TRACK(params->dwTo);
            switch (dwParam1)
            {
            case MCI_SEEK_TO_START:
                printf(">>MCI_SEEK %08X=(MCI_SEEK_TO_START) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("    dwTo=%08X,\n", params->dwTo);
                printf("}\n");
                break;
            case MCI_SEEK_TO_END:
                printf(">>MCI_SEEK %08X=(MCI_SEEK_TO_END) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("    dwTo=%08X,\n", params->dwTo);
                printf("}\n");
                break;
            case MCI_TO:
                printf(">>MCI_SEEK %08X=(MCI_TO) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("    dwTo=%08X,\n", params->dwTo);
                printf("}\n");
                break;
            default:
                printf(">>MCI_SEEK %08X=??? \n", dwParam1);
                return -1;
            }
            fflush(stdout);
            
            pwh->reserved = MCI_NOTIFY_SUPERSEDED;
            const auto reset = table->Reset(wave);
            printf("<<WaveOutReset (%08X)\n", reset);
            fflush(stdout);
        }
        break;
    case MCI_STOP:
        {
            const auto params = reinterpret_cast<LPMCI_GENERIC_PARMS>(dwParam2);
            switch (dwParam1)
            {
            case MCI_NOTIFY:
                printf(">>MCI_STOP %08X=(MCI_NOTIFY) %08X\n", dwParam1, dwParam2);
                params->dwCallback;
                break;
            case MCI_WAIT:
                printf(">>MCI_STOP %08X=(MCI_WAIT) %08X\n", dwParam1, dwParam2);
                params->dwCallback;
                break;
            default:
                printf(">>MCI_STOP %08X=??? \n", dwParam1);
                return -1;
            }
            fflush(stdout);

            pwh->reserved = MCI_NOTIFY_SUPERSEDED;
            const auto reset = table->Reset(wave);
            printf("<<WaveOutReset (%08X)\n", reset);
            fflush(stdout);
            const auto unprepare = table->UnprepareHeader(wave, pwh, sizeof(WAVEHDR));
            printf("<<WaveOutUnprepareHeader (%08X)\n", unprepare);
            fflush(stdout);
            free(pwh->lpData);
        }
        fflush(stdout);
        break;
    case MCI_SET:
        {
            const auto params = reinterpret_cast<LPMCI_SET_PARMS>(dwParam2);
            switch (dwParam1)
            {
            case MCI_SET_TIME_FORMAT:
                printf(">>MCI_SET %08X=(MCI_SET_TIME_FORMAT) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("    dwTimeFormat=%08X=MCI_FORMAT_TMSF,\n", params->dwTimeFormat);
                printf("    dwAudio=%08X,\n", params->dwAudio);
                printf("}\n");
                break;
            case MCI_SET_AUDIO:
                printf(">>MCI_SET %08X=(MCI_SET_AUDIO) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("    dwTimeFormat=%08X=MCI_FORMAT_TMSF,\n", params->dwTimeFormat);
                printf("    dwAudio=%08X,\n", params->dwAudio);
                printf("}\n");
                break;
            default:
                printf(">>MCI_SET %08X=??? \n", dwParam1);
                break;
            }
            fflush(stdout);
        }
        break;
    case MCI_STATUS:
        if (dwParam1 != MCI_STATUS_ITEM)
        {
            printf(">>MCI_STATUS %08X %08X\n", dwParam1, dwParam2);
            fflush(stdout);
            return -1;
        }
        {
            const auto params = reinterpret_cast<LPMCI_STATUS_PARMS>(dwParam2);
            switch (params->dwItem)
            {
            case MCI_STATUS_MEDIA_PRESENT:
                printf(">>MCI_STATUS %08X=(MCI_STATUS_ITEM) %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("    dwReturn=%08X => %08X,\n", params->dwReturn, TRUE);
                printf("    dwItem=%08X=MCI_STATUS_MEDIA_PRESENT,\n", params->dwItem);
                printf("    dwTrack=%08X,\n", params->dwTrack);
                printf("}\n");
                params->dwReturn = TRUE;
                break;
            default:
                printf(">>MCI_STATUS %08X=??? %08X={\n", dwParam1, dwParam2);
                printf("    dwCallback=%08X,\n", params->dwCallback);
                printf("    dwReturn=%08X => %08X,\n", params->dwReturn, TRUE);
                printf("    dwItem=%08X=???,\n", params->dwItem);
                printf("    dwTrack=%08X,\n", params->dwTrack);
                printf("}\n");
                break;
            }
            fflush(stdout);
        
            auto volume = 0ul;
            const auto get = table->GetVolume(wave, &volume);
            printf("<<WaveOutGetVolume (%08X) => %08X\n", get, volume);
            fflush(stdout);
        }
        break;
    default:
        printf(">>%08X %08X %08X %08X \n", mciId, uMsg, dwParam1, dwParam2);
        printf("<<%08X \n", -1);
        fflush(stdout);
        return -1;
    }
    return 0;
}

MCIERROR WINAPI mciSendStringHook(LPCSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback)
{
    printf("%s %p(%04X) %p\n\n", lpszCommand, lpszReturnString, cchReturn, hwndCallback);
    return 0;
}

void CALLBACK WaveOutDone(HWAVEOUT /*hwo*/, UINT uMsg, DWORD /*dwInstance*/, DWORD dwParam1, DWORD dwParam2)
{
    switch (uMsg)
    {
    case WOM_OPEN:
        printf("<<WOM_OPEN => %08X %08X\n", dwParam1, dwParam2);
        break;
    case WOM_DONE:
        printf("<<WOM_DONE => %08X %08X\n", dwParam1, dwParam2);
        {
            const auto pwh = reinterpret_cast<LPWAVEHDR>(dwParam1);
            SendNotifyMessage(handle, MM_MCINOTIFY, pwh->reserved, 0x00114514);
            printf(">>MM_MCINOTIFY\n");
        }
        break;
    case WOM_CLOSE:
        printf("<<WOM_CLOSE => %08X %08X\n", dwParam1, dwParam2);
        break;
    default:
        break;
    }
    fflush(stdout);
}
