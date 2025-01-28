// ReSharper disable CppParameterMayBeConst
#include <windows.h>
#include <stdio.h> // NOLINT(*-deprecated-headers)
#include "proxy.h"
#include "wave.h"
#include "hook.h"

HMODULE winmm = nullptr;

MciToWaveOut* m2w = nullptr;

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
            m2w = new MciToWaveOut;
        }
        break;
    case DLL_PROCESS_DETACH:
        {
            if (winmm == nullptr) return FALSE;
            delete m2w;
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

#define MCI_FLAG_MATCH(buffer, mask, flag) \
    if (mask == flag) lstrcpy(buffer, TEXT(#flag));

MciToWaveOut::MciToWaveOut()
{
    table = new WaveOutFunctionTable(winmm);
}

MciToWaveOut::~MciToWaveOut()
{
    delete table;
    for (const auto pwh : cache)
    {
        if (!pwh) continue;
        free(pwh->lpData);
        free(pwh);
    }
}

MCIDEVICEID MciToWaveOut::GetDeviceID() const
{
    if (wave == nullptr) return 0;
    MCIDEVICEID id;
    const auto get = table->GetID(wave, &id);
    if (get != MMSYSERR_NOERROR) return 0;
    return id;
}

BYTE MciToWaveOut::GetCurrentTrack() const
{
    return this->current;
}

DWORD MciToWaveOut::GetLength(BYTE track)
{
    if (cache[track - 0x01] == nullptr)
    {
        CHAR buffer[MAX_PATH];
        sprintf(buffer, ".\\bgm\\Track%02d.wav", track);
        const auto file = fopen(buffer, "rb");
        if (file == nullptr) return 0;
        fseek(file, 0x00, SEEK_END);
        cache[track - 0x01] = new WAVEHDR;
        cache[track - 0x01]->dwFlags = 0x00000000;
        cache[track - 0x01]->dwBufferLength = static_cast<DWORD>(ftell(file) - 0x2C);
        cache[track - 0x01]->lpData = static_cast<LPSTR>(malloc(cache[track - 0x01]->dwBufferLength));
        fseek(file, 0x2C, SEEK_SET);
        const auto size = fread(cache[track - 0x01]->lpData, sizeof(BYTE), cache[track - 0x01]->dwBufferLength, file);
        OutputDebugPrintf(TEXT("<<fread(path=%s) => %08X\n"), buffer, size);
        fclose(file);
    }
    const auto sec = cache[track - 0x01]->dwBufferLength / 176400;
    return MCI_MAKE_MSF(sec / 60, sec % 60, 0);
}

LPWAVEHDR MciToWaveOut::Track(BYTE index)
{
    if (index > 0x10) return nullptr;
    if (cache[index - 0x01] != nullptr) return cache[index - 0x01];
    CHAR buffer[MAX_PATH];
    sprintf(buffer, ".\\bgm\\Track%02d.wav", index);
    const auto file = fopen(buffer, "rb");
    if (file == nullptr)
    {
        MessageBoxA(nullptr, buffer, "Not Found File", MB_ICONERROR);
        return nullptr;
    }
    fseek(file, 0x00, SEEK_END);
    const auto hdr = new WAVEHDR;
    hdr->dwFlags = 0x00000000;
    hdr->dwBufferLength = static_cast<DWORD>(ftell(file) - 0x2C);
    hdr->lpData = static_cast<LPSTR>(malloc(hdr->dwBufferLength));
    fseek(file, 0x2C, SEEK_SET);
    const auto size = fread(hdr->lpData, sizeof(BYTE), hdr->dwBufferLength, file);
    OutputDebugPrintf(TEXT("<<fread(path=%s) => %08X\n"), buffer, size);
    fclose(file);
    cache[index - 0x01] = hdr;
    return hdr;
}

MMRESULT MciToWaveOut::Open()
{
    constexpr auto format = WAVEFORMATEX{WAVE_FORMAT_PCM, 2, 44100, 2 * 16 / 8 * 44100, 2 * 16 / 8, 16, 0};
    const auto open = table->Open(&wave, WAVE_MAPPER, &format,
                                  reinterpret_cast<DWORD>(&Done), reinterpret_cast<DWORD>(this),
                                  CALLBACK_FUNCTION);
    OutputDebugPrintf(TEXT("<<WaveOutOpen (%08X) => %p\n"), open, wave);
    return open;
}

MMRESULT MciToWaveOut::OpenNotify(HWND hwnd)
{
    GlobalLock(&callback);
    callback = hwnd;
    const auto open = Open();
    GlobalUnlock(&callback);
    return open;
}

MMRESULT MciToWaveOut::Close()
{
    const auto close = table->Close(wave);
    OutputDebugPrintf(TEXT("<<WaveOutClose (%08X)\n"), close);
    wave = nullptr;
    return close;
}

MMRESULT MciToWaveOut::CloseNotify(HWND hwnd)
{
    GlobalLock(&callback);
    callback = hwnd;
    const auto close = Close();
    GlobalUnlock(&callback);
    return close;
}

MMRESULT MciToWaveOut::Seek(BYTE track)
{
    const auto stop = Stop();
    if (stop != MMSYSERR_NOERROR) return stop;

    const auto pwh = Track(track);
    if (pwh == nullptr) return MMSYSERR_INVALPARAM;
    pwh->dwFlags = 0;
    const auto prepare = table->PrepareHeader(wave, pwh, sizeof(WAVEHDR));
    OutputDebugPrintf(TEXT("<<WaveOutPrepareHeader (%08X)\n"), prepare);
    current = track;
    return prepare;
}

MMRESULT MciToWaveOut::SeekNotify(BYTE track, HWND hwnd)
{
    GlobalLock(&callback);
    callback = hwnd;
    const auto seek = Seek(track);
    GlobalUnlock(&callback);
    return seek;
}

MMRESULT MciToWaveOut::Play(BYTE track)
{
    const auto stop = Stop();
    if (stop != MMSYSERR_NOERROR) return stop;

    const auto pwh = Track(track);
    if (pwh == nullptr) return MMSYSERR_INVALPARAM;
    if (pwh->dwFlags ^ WHDR_PREPARED)
    {
        pwh->dwFlags = 0;
        const auto prepare = table->PrepareHeader(wave, pwh, sizeof(WAVEHDR));
        OutputDebugPrintf(TEXT("<<WaveOutPrepareHeader (%08X)\n"), prepare);
        if (prepare != MMSYSERR_NOERROR) return prepare;
    }

    const auto write = table->Write(wave, pwh, sizeof(WAVEHDR));
    OutputDebugPrintf(TEXT("<<WaveOutWrite (%08X)\n"), write);
    current = track;
    return write;
}

MMRESULT MciToWaveOut::PlayNotify(BYTE track, HWND hwnd)
{
    GlobalLock(&callback);
    callback = hwnd;
    const auto play = Play(track);
    GlobalUnlock(&callback);
    return play;
}

MMRESULT MciToWaveOut::Stop()
{
    if (current < 0x02 || current > 0x10) return MMSYSERR_NOERROR;
    cache[current - 0x01]->dwBytesRecorded = TRUE;
    const auto reset = table->Reset(wave);
    OutputDebugPrintf(TEXT("<<WaveOutReset (%08X)\n"), reset);
    if (reset != MMSYSERR_NOERROR) return reset;
    
    const auto pwh = cache[current - 0x01];
    if (pwh && (pwh->dwFlags & WHDR_PREPARED))
    {
        const auto unprepare = table->UnprepareHeader(wave, pwh, sizeof(WAVEHDR));
        OutputDebugPrintf(TEXT("<<WaveOutUnprepareHeader (%08X)\n"), unprepare);
        if (unprepare != MMSYSERR_NOERROR) return unprepare;
    }
    
    return MMSYSERR_NOERROR;
}

MMRESULT MciToWaveOut::StopNotify(HWND hwnd)
{
    GlobalLock(&callback);
    callback = hwnd;
    const auto stop = Stop();
    GlobalUnlock(&callback);
    return stop;
}

void MciToWaveOut::Done(HDRVR hdr, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD /*dwParam2*/)
{
    const auto hwo = reinterpret_cast<HWAVEOUT>(hdr);
    const auto m2w = reinterpret_cast<MciToWaveOut*>(dwInstance);
    const auto pwh = reinterpret_cast<LPWAVEHDR>(dwParam1);

    switch (uMsg)
    {
    case WOM_OPEN:
        OutputDebugPrintf(TEXT("<<WOM_OPEN => %08X\n"), hwo);
        SendNotifyMessage(m2w->callback, MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, static_cast<LPARAM>(m2w->GetDeviceID()));
        break;
    case WOM_DONE:
        OutputDebugPrintf(TEXT("<<WOM_DONE => %08X %08X\n"), hwo, pwh->dwFlags);
        {
            const auto notify = pwh->dwBytesRecorded ? MCI_NOTIFY_SUPERSEDED : MCI_NOTIFY_SUCCESSFUL;
            SendNotifyMessage(m2w->callback, MM_MCINOTIFY, notify, static_cast<LPARAM>(m2w->GetDeviceID()));
        }
        break;
    case WOM_CLOSE:
        OutputDebugPrintf(TEXT("<<WOM_CLOSE => %08X\n"), hwo);
        SendNotifyMessage(m2w->callback, MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, static_cast<LPARAM>(m2w->GetDeviceID()));
        break;
    default:
        OutputDebugPrintf(TEXT("<<WOM => %08X %08X %08X %08X\n"), hdr, uMsg, dwInstance, hwo);
        break;
    }
}

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
            OutputDebugPrintf(TEXT("    wDeviceID=%08X,\n"), params->wDeviceID);
            OutputDebugPrintf(TEXT("    lpstrDeviceType=%p,\n"), params->lpstrDeviceType);
            OutputDebugPrintf(TEXT("    lpstrElementName=%s,\n"), params->lpstrElementName);
            OutputDebugPrintf(TEXT("    lpstrAlias=%p\n"), params->lpstrAlias);
            OutputDebugPrintf(TEXT("}\n"));
            if (m2w == nullptr) return MMSYSERR_NOTENABLED;
            const auto hwnd = reinterpret_cast<HWND>(params->dwCallback & 0x0000FFFF);

            if (dwParam1 & MCI_NOTIFY) return m2w->OpenNotify(hwnd);
            const auto result = m2w->Open();
            if (result != MMSYSERR_NOERROR) return result;
            params->wDeviceID = m2w->GetDeviceID();
        }
    case MCI_CLOSE:
        {
            OutputDebugPrintf(TEXT(">>%s %08X=(%s) %08X={\n"), "MCI_CLOSE", dwParam1, flags, dwParam2);
            const auto params = reinterpret_cast<LPMCI_GENERIC_PARMS>(dwParam2);
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("}\n"));
            if (m2w == nullptr) return MMSYSERR_NOTENABLED;
            if (m2w->GetDeviceID() != mciId) return MMSYSERR_NOERROR;
            const auto hwnd = reinterpret_cast<HWND>(params->dwCallback & 0x0000FFFF);

            return dwParam1 & MCI_NOTIFY
                       ? m2w->CloseNotify(hwnd)
                       : m2w->Close();
        }
    case MCI_PLAY:
        {
            OutputDebugPrintf(TEXT(">>%s %08X=(%s) %08X={\n"), "MCI_PLAY", dwParam1, flags, dwParam2);
            const auto params = reinterpret_cast<LPMCI_PLAY_PARMS>(dwParam2);
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("    dwFrom=%08X,\n"), params->dwFrom);
            OutputDebugPrintf(TEXT("    dwTo=%08X,\n"), params->dwTo);
            OutputDebugPrintf(TEXT("}\n"));
            if (m2w == nullptr) return MMSYSERR_NOTENABLED;
            const auto hwnd = reinterpret_cast<HWND>(params->dwCallback & 0x0000FFFF);
            const auto track = MCI_TMSF_TRACK(params->dwFrom);

            return dwParam1 & MCI_NOTIFY
                       ? m2w->PlayNotify(track, hwnd)
                       : m2w->Play(track);
        }
    case MCI_SEEK:
        MCI_FLAGS_CAT(flags, dwParam1, MCI_SEEK_TO_END)
        MCI_FLAGS_CAT(flags, dwParam1, MCI_SEEK_TO_START)
        {
            OutputDebugPrintf(TEXT(">>%s %08X=(%s) %08X={\n"), "MCI_SEEK", dwParam1, flags, dwParam2);
            const auto params = reinterpret_cast<LPMCI_SEEK_PARMS>(dwParam2);
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("    dwTo=%08X,\n"), params->dwTo);
            OutputDebugPrintf(TEXT("}\n"));
            if (m2w == nullptr) return MMSYSERR_NOTENABLED;
            const auto hwnd = reinterpret_cast<HWND>(params->dwCallback & 0x0000FFFF);
            const auto track = MCI_TMSF_TRACK(params->dwTo);

            return dwParam1 & MCI_NOTIFY
                       ? m2w->SeekNotify(track, hwnd)
                       : m2w->Seek(track);
        }
    case MCI_STOP:
        {
            OutputDebugPrintf(TEXT(">>%s %08X=(%s) %08X={\n"), "MCI_STOP", dwParam1, flags, dwParam2);
            const auto params = reinterpret_cast<LPMCI_GENERIC_PARMS>(dwParam2);
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("}\n"));
            if (m2w == nullptr) return MMSYSERR_NOTENABLED;
            const auto hwnd = reinterpret_cast<HWND>(params->dwCallback & 0x0000FFFF);

            return dwParam1 & MCI_NOTIFY
                       ? m2w->StopNotify(hwnd)
                       : m2w->Stop();
        }
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
            flags[0x00] = '\0';
            MCI_FLAG_MATCH(flags, params->dwItem, MCI_STATUS_LENGTH)
            MCI_FLAG_MATCH(flags, params->dwItem, MCI_STATUS_POSITION)
            MCI_FLAG_MATCH(flags, params->dwItem, MCI_STATUS_NUMBER_OF_TRACKS)
            MCI_FLAG_MATCH(flags, params->dwItem, MCI_STATUS_MODE)
            MCI_FLAG_MATCH(flags, params->dwItem, MCI_STATUS_MEDIA_PRESENT)
            MCI_FLAG_MATCH(flags, params->dwItem, MCI_STATUS_TIME_FORMAT)
            MCI_FLAG_MATCH(flags, params->dwItem, MCI_STATUS_READY)
            MCI_FLAG_MATCH(flags, params->dwItem, MCI_STATUS_CURRENT_TRACK)
            OutputDebugPrintf(TEXT("    dwCallback=%08X,\n"), params->dwCallback);
            OutputDebugPrintf(TEXT("    dwReturn=%08X,\n"), params->dwReturn);
            OutputDebugPrintf(TEXT("    dwItem=%s,\n"), flags);
            OutputDebugPrintf(TEXT("    dwTrack=%08X,\n"), params->dwTrack);
            OutputDebugPrintf(TEXT("}\n"));
            if (m2w == nullptr) return MMSYSERR_NOTENABLED;
            switch (params->dwItem)
            {
            case MCI_STATUS_LENGTH:
                // TODO MCI_STATUS_LENGTH
                params->dwReturn = m2w->GetLength(params->dwTrack);
                OutputDebugPrintf(TEXT("<<MSF => %08X\n"), params->dwReturn);
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
                params->dwReturn = m2w->GetCurrentTrack();
                break;
            default:
                break;
            }
        }
        break;
    default:
        OutputDebugPrintf(TEXT(">>MCI %08X %08X %08X %08X \n"), mciId, uMsg, dwParam1, dwParam2);
        OutputDebugPrintf(TEXT("<<%08X \n"), MMSYSERR_NOTSUPPORTED);
        return MMSYSERR_NOTSUPPORTED;
    }
    return MMSYSERR_NOERROR;
}

MCIERROR WINAPI mciSendStringHook(LPCSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback)
{
    OutputDebugPrintf(TEXT(">>MCI %s %p(%04X) %p\n\n"), lpszCommand, lpszReturnString, cchReturn, hwndCallback);
    // TODO MciSendString
    return MMSYSERR_NOERROR;
}
