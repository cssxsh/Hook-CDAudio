// ReSharper disable CppParameterMayBeConst
#include <windows.h>
#include <stdio.h> // NOLINT(*-deprecated-headers)
#include "proxy.h"
#include "wave.h"
#include "hook.h"

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
            TCHAR path[MAX_PATH] = {};
            GetSystemDirectory(path, MAX_PATH);
            lstrcat(path, TEXT("\\winmm.dll"));
            winmm = LoadLibrary(path);
            if (winmm == nullptr) return FALSE;
            WinmmProxy(winmm);
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

void OutputDebugPrintfA(_Printf_format_string_ LPCSTR const format, ...)
{
    CHAR buffer[0x1000] = {};
    va_list args;
    va_start(args, format);
    _vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    OutputDebugStringA(buffer);
}

void OutputDebugPrintfW(_Printf_format_string_ LPCWSTR const format, ...)
{
    WCHAR buffer[0x1000] = {};
    va_list args;
    va_start(args, format);
    _vsnwprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    OutputDebugStringW(buffer);
}

#define MCI_FLAGS_CAT(buffer, mask, flag) \
    if (mask & flag) lstrcat(buffer, buffer[0] ? TEXT(" | " #flag) : TEXT(#flag));

MCIERROR WINAPI mciSendCommandHook(MCIDEVICEID mciId, UINT uMsg, DWORD dwParam1, DWORD dwParam2)
{
    TCHAR flags[0x1000] = {};
    MCI_FLAGS_CAT(flags, dwParam1, MCI_NOTIFY)
    MCI_FLAGS_CAT(flags, dwParam1, MCI_WAIT)
    MCI_FLAGS_CAT(flags, dwParam1, MCI_FROM)
    MCI_FLAGS_CAT(flags, dwParam1, MCI_TO)
    MCI_FLAGS_CAT(flags, dwParam1, MCI_TRACK)

    switch (uMsg)
    {
    case MCI_OPEN:
        MCI_FLAGS_CAT(flags, dwParam1, MCI_OPEN_SHAREABLE)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_OPEN_ELEMENT)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_OPEN_ALIAS)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_OPEN_ELEMENT_ID)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_OPEN_TYPE_ID)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_OPEN_TYPE)
        {
            OutputDebugPrintf(TEXT(">>%s %08X=(%s) %08X={\n"), "MCI_OPEN", dwParam1, flags, dwParam2);
            const auto params = reinterpret_cast<LPMCI_OPEN_PARMS>(dwParam2);
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("    wDeviceID=%08X => %08X,\n"), params->wDeviceID, 0x00114514);
            OutputDebugPrintf(TEXT("    lpstrDeviceType=%p,\n"), params->lpstrDeviceType);
            OutputDebugPrintf(TEXT("    lpstrElementName=%s,\n"), params->lpstrElementName);
            OutputDebugPrintf(TEXT("    lpstrAlias=%p\n"), params->lpstrAlias);
            OutputDebugPrintf(TEXT("}\n"));
            params->wDeviceID = 0x00114514;

            constexpr auto format = WAVEFORMATEX{WAVE_FORMAT_PCM, 2, 44100, 2 * 16 / 8 * 44100, 2 * 16 / 8, 16, 0};
            const auto callback = reinterpret_cast<DWORD>(&WaveOutDone);
            const auto open = table->Open(&wave, WAVE_MAPPER, &format, callback, 0, CALLBACK_FUNCTION);
            OutputDebugPrintf(TEXT("<<WaveOutOpen (%08X) => %p\n"), open, wave);

            auto id = 0u;
            const auto get = table->GetID(wave, &id);
            OutputDebugPrintf(TEXT("<<WaveOutGetID (%08X) => %08X\n"), get, id);
        }
        break;
    case MCI_CLOSE:
        {
            OutputDebugPrintf(TEXT(">>%s %08X=(%s) %08X={\n"), "MCI_CLOSE", dwParam1, flags, dwParam2);
            const auto params = reinterpret_cast<LPMCI_GENERIC_PARMS>(dwParam2);
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("}\n"));

            const auto close = table->Close(wave);
            OutputDebugPrintf(TEXT("<<WaveOutClose (%08X)\n"), close);
        }
        break;
    case MCI_PLAY:
        {
            OutputDebugPrintf(TEXT(">>%s %08X=(%s) %08X={\n"), "MCI_PLAY", dwParam1, flags, dwParam2);
            const auto params = reinterpret_cast<LPMCI_PLAY_PARMS>(dwParam2);
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("    dwFrom=%08X,\n"), params->dwFrom);
            OutputDebugPrintf(TEXT("    dwTo=%08X,\n"), params->dwTo);
            OutputDebugPrintf(TEXT("}\n"));
            if (dwParam1 & MCI_NOTIFY) handle = reinterpret_cast<HWND>(params->dwCallback & 0x0000FFFF);
            const auto track = MCI_TMSF_TRACK(params->dwFrom);
            if (track != 0xFF) return 0;

            pwh->reserved = MCI_NOTIFY_SUPERSEDED;
            const auto reset = table->Reset(wave);
            OutputDebugPrintf(TEXT("<<WaveOutReset (%08X)\n"), reset);

            CHAR buffer[MAX_PATH];
            sprintf(buffer, ".\\bgm\\Track%02d.wav", track);
            const auto file = fopen(buffer, "rb");
            if (file == nullptr)
            {
                MessageBoxA(nullptr, buffer, "Not Found File", MB_ICONERROR);
                return -1;
            }
            fseek(file, 0x00, SEEK_END);
            pwh->dwFlags = 0x00000000;
            pwh->dwBufferLength = static_cast<DWORD>(ftell(file) - 0x2C);
            pwh->lpData = static_cast<LPSTR>(malloc(pwh->dwBufferLength));
            pwh->reserved = MCI_NOTIFY_SUCCESSFUL;
            fseek(file, 0x2C, SEEK_SET);
            const auto size = fread(pwh->lpData, sizeof(BYTE), pwh->dwBufferLength, file);
            OutputDebugPrintf(TEXT("<<fread(path=%s) => %08X\n"), buffer, size);
            fclose(file);
            const auto prepare = table->PrepareHeader(wave, pwh, sizeof(WAVEHDR));
            OutputDebugPrintf(TEXT("<<WaveOutPrepareHeader (%08X)\n"), prepare);
            const auto write = table->Write(wave, pwh, sizeof(WAVEHDR));
            OutputDebugPrintf(TEXT("<<WaveOutWrite (%08X)\n"), write);
        }
        break;
    case MCI_SEEK:
        MCI_FLAGS_CAT(flags, dwParam1, MCI_SEEK_TO_END)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_SEEK_TO_START)
        {
            OutputDebugPrintf(TEXT(">>%s %08X=(%s) %08X={\n"), "MCI_SEEK", dwParam1, flags, dwParam2);
            const auto params = reinterpret_cast<LPMCI_SEEK_PARMS>(dwParam2);
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("    dwTo=%08X,\n"), params->dwTo);
            OutputDebugPrintf(TEXT("}\n"));
            const auto track = MCI_TMSF_TRACK(params->dwTo);
            if (track != 0xFF) return 0;

            pwh->reserved = MCI_NOTIFY_SUPERSEDED;
            const auto reset = table->Reset(wave);
            OutputDebugPrintf(TEXT("<<WaveOutReset (%08X)\n"), reset);
        }
        break;
    case MCI_STOP:
        {
            OutputDebugPrintf(TEXT(">>%s %08X=(%s) %08X={\n"), "MCI_STOP", dwParam1, flags, dwParam2);
            const auto params = reinterpret_cast<LPMCI_GENERIC_PARMS>(dwParam2);
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("}\n"));

            pwh->reserved = MCI_NOTIFY_SUPERSEDED;
            const auto reset = table->Reset(wave);
            OutputDebugPrintf(TEXT("<<WaveOutReset (%08X)\n"), reset);
            const auto unprepare = table->UnprepareHeader(wave, pwh, sizeof(WAVEHDR));
            OutputDebugPrintf(TEXT("<<WaveOutUnprepareHeader (%08X)\n"), unprepare);
            free(pwh->lpData);
        }
        break;
    case MCI_SET:
        MCI_FLAGS_CAT(flags, dwParam1, MCI_SET_DOOR_OPEN)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_SET_DOOR_CLOSED)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_SET_TIME_FORMAT)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_SET_AUDIO)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_SET_VIDEO)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_SET_ON)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_SET_OFF)
        {
            OutputDebugPrintf(TEXT(">>%s %08X=(%s) %08X={\n"), "MCI_SET", dwParam1, flags, dwParam2);
            const auto params = reinterpret_cast<LPMCI_SET_PARMS>(dwParam2);
            OutputDebugPrintf(TEXT(">>MCI_SET %08X=(MCI_SET_TIME_FORMAT) %08X={\n"), dwParam1, dwParam2);
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("    dwTimeFormat=%08X,\n"), params->dwTimeFormat);
            OutputDebugPrintf(TEXT("    dwAudio=%08X,\n"), params->dwAudio);
            OutputDebugPrintf(TEXT("}\n"));
        }
        break;
    case MCI_STATUS:
        MCI_FLAGS_CAT(flags, dwParam1, MCI_STATUS_ITEM)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_STATUS_START)
        {
            OutputDebugPrintf(TEXT(">>%s %08X=(%s) %08X={\n"), "MCI_STATUS", dwParam1, flags, dwParam2);
            const auto params = reinterpret_cast<LPMCI_STATUS_PARMS>(dwParam2);
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("    dwReturn=%08X,\n"), params->dwReturn);
            OutputDebugPrintf(TEXT("    dwItem=%08X,\n"), params->dwItem);
            OutputDebugPrintf(TEXT("    dwTrack=%08X,\n"), params->dwTrack);
            OutputDebugPrintf(TEXT("}\n"));
            switch (params->dwItem)
            {
            case MCI_STATUS_LENGTH:
                // TODO MCI_STATUS_LENGTH
                params->dwReturn = 0x7FFFFFFF;
                break;
            case MCI_STATUS_POSITION:
                // TODO MCI_STATUS_POSITION
                params->dwReturn = 0x00000000;
                break;
            case MCI_STATUS_NUMBER_OF_TRACKS:
                // TODO MCI_STATUS_NUMBER_OF_TRACKS
                params->dwReturn = 0x10;
                break;
            case MCI_STATUS_MODE:
                // TODO MCI_STATUS_MODE
                params->dwReturn = MCI_MODE_OPEN;
                break;
            case MCI_STATUS_MEDIA_PRESENT:
                params->dwReturn = TRUE;
                break;
            case MCI_STATUS_TIME_FORMAT:
                params->dwReturn = MCI_FORMAT_TMSF;
                break;
            case MCI_STATUS_READY:
                params->dwReturn = TRUE;
                break;
            case MCI_STATUS_CURRENT_TRACK:
                // TODO MCI_STATUS_CURRENT_TRACK
                params->dwReturn = 0x02;
                break;
            default:
                break;
            }

            auto volume = 0ul;
            const auto get = table->GetVolume(wave, &volume);
            OutputDebugPrintf(TEXT("<<WaveOutGetVolume (%08X) => %08X\n"), get, volume);
        }
        break;
    default:
        OutputDebugPrintf(TEXT(">>%08X %08X %08X %08X \n"), mciId, uMsg, dwParam1, dwParam2);
        OutputDebugPrintf(TEXT("<<%08X \n"), -1);
        return -1;
    }
    return 0;
}

MCIERROR WINAPI mciSendStringHook(LPCSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback)
{
    OutputDebugPrintf(TEXT(">>MCI %s %p(%04X) %p\n\n"), lpszCommand, lpszReturnString, cchReturn, hwndCallback);
    // TODO MciSendString
    return 0;
}

void CALLBACK WaveOutDone(HWAVEOUT /*hwo*/, UINT uMsg, DWORD /*dwInstance*/, DWORD dwParam1, DWORD dwParam2)
{
    switch (uMsg)
    {
    case WOM_OPEN:
        OutputDebugPrintf(TEXT("<<WOM_OPEN => %08X %08X\n"), dwParam1, dwParam2);
        break;
    case WOM_DONE:
        OutputDebugPrintf(TEXT("<<WOM_DONE => %08X %08X\n"), dwParam1, dwParam2);
        {
            const auto pwh = reinterpret_cast<LPWAVEHDR>(dwParam1);
            SendNotifyMessage(handle, MM_MCINOTIFY, pwh->reserved, 0x00114514);
            OutputDebugPrintf(TEXT(">>MM_MCINOTIFY\n"));
        }
        break;
    case WOM_CLOSE:
        OutputDebugPrintf(TEXT("<<WOM_CLOSE => %08X %08X\n"), dwParam1, dwParam2);
        break;
    default:
        break;
    }
    fflush(stdout);
}
